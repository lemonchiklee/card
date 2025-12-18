#include "database.h"
#include "constants.h"
#include <QSqlError>
#include <QCryptographicHash>
#include <QFile>
#include <QDataStream>
#include <QMutexLocker>
#include <QDebug>
#include <QMessageAuthenticationCode>

Database& Database::get() {
    static Database instance;
    return instance;
}

Database::Database() : m_connected(false) {}

Database::~Database() {
    // Don't call close() in destructor - it should be called explicitly
    // This prevents issues with Qt cleanup order
    if (m_connected && m_db.isOpen()) {
        m_db.close();
    }
}

QString Database::hashPwd(const QString& password) {
    return QString(QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256).toHex());
}

QByteArray Database::calculateSignature(const QByteArray& data) {
    return QMessageAuthenticationCode::hash(data, App::SIGNATURE_KEY.toUtf8(), QCryptographicHash::Sha256);
}

bool Database::verifySignature(const QByteArray& data, const QByteArray& signature) {
    return calculateSignature(data) == signature;
}

bool Database::connect(const DbConfig& cfg) {
    QMutexLocker lock(&m_mutex);
    if (m_connected) return true;
    
    m_db = QSqlDatabase::addDatabase("QPSQL", "CardCabConn");
    m_db.setHostName(cfg.host);
    m_db.setPort(cfg.port);
    m_db.setDatabaseName(cfg.database);
    m_db.setUserName(cfg.user);
    m_db.setPassword(cfg.password);
    
    if (!m_db.open()) {
        m_error = m_db.lastError().text();
        return false;
    }
    m_connected = true;
    return true;
}

bool Database::isOpen() const { return m_connected && m_db.isOpen(); }

void Database::close() {
    QMutexLocker lock(&m_mutex);
    
    if (!m_connected) return;  // Already closed
    
    QString conn = m_db.connectionName();
    
    if (m_db.isOpen()) {
        m_db.close();
    }
    
    m_connected = false;
    m_db = QSqlDatabase();
    
    if (!conn.isEmpty()) {
        QSqlDatabase::removeDatabase(conn);
    }
}

bool Database::exec(QSqlQuery& q, const QString& desc) {
    if (!q.exec()) {
        m_error = desc + ": " + q.lastError().text();
        return false;
    }
    return true;
}

// === Users ===

bool Database::registerUser(const QString& username, const QString& password) {
    QMutexLocker lock(&m_mutex);
    QSqlQuery q(m_db);
    q.prepare("INSERT INTO users (username, password_hash, is_admin) VALUES (:u, :p, FALSE)");
    q.bindValue(":u", username);
    q.bindValue(":p", hashPwd(password));
    return exec(q, "registerUser");
}

bool Database::authenticate(const QString& username, const QString& password, User& out) {
    return authenticateByHash(username, hashPwd(password), out);
}

bool Database::authenticateByHash(const QString& username, const QString& hash, User& out) {
    QMutexLocker lock(&m_mutex);
    QSqlQuery q(m_db);
    q.prepare("SELECT id, username, password_hash, is_admin, created_at FROM users WHERE username = :u AND password_hash = :h");
    q.bindValue(":u", username);
    q.bindValue(":h", hash);
    if (!exec(q, "authenticate")) return false;
    
    if (q.next()) {
        out.id = q.value("id").toLongLong();
        out.username = q.value("username").toString();
        out.passwordHash = q.value("password_hash").toString();
        out.isAdmin = q.value("is_admin").toBool();
        out.createdAt = q.value("created_at").toDateTime();
        return true;
    }
    m_error = "Invalid credentials";
    return false;
}

bool Database::changeAdminCreds(qint64 adminId, const QString& newUser, const QString& newPass) {
    QMutexLocker lock(&m_mutex);
    QSqlQuery q(m_db);
    q.prepare("UPDATE users SET username = :u, password_hash = :p WHERE id = :id AND is_admin = TRUE");
    q.bindValue(":u", newUser);
    q.bindValue(":p", hashPwd(newPass));
    q.bindValue(":id", adminId);
    return exec(q, "changeAdminCreds");
}

bool Database::userExists(const QString& username) {
    QMutexLocker lock(&m_mutex);
    QSqlQuery q(m_db);
    q.prepare("SELECT COUNT(*) FROM users WHERE username = :u");
    q.bindValue(":u", username);
    return exec(q, "userExists") && q.next() && q.value(0).toInt() > 0;
}

// === Cards ===

bool Database::addCard(Card& card) {
    QMutexLocker lock(&m_mutex);
    QSqlQuery q(m_db);
    q.prepare("INSERT INTO cards (card_name, participant_name, source, album_name, image_data) "
              "VALUES (:name, :part, :src, :album, :img) RETURNING id, created_at, updated_at");
    q.bindValue(":name", card.cardName);
    q.bindValue(":part", card.participantName);
    q.bindValue(":src", card.source);
    q.bindValue(":album", card.albumName);
    q.bindValue(":img", card.imageData);
    
    if (exec(q, "addCard") && q.next()) {
        card.id = q.value("id").toLongLong();
        card.createdAt = q.value("created_at").toDateTime();
        card.updatedAt = q.value("updated_at").toDateTime();
        return true;
    }
    return false;
}

bool Database::updateCard(const Card& card) {
    QMutexLocker lock(&m_mutex);
    QSqlQuery q(m_db);
    q.prepare("UPDATE cards SET card_name = :name, participant_name = :part, "
              "source = :src, album_name = :album, image_data = :img WHERE id = :id");
    q.bindValue(":name", card.cardName);
    q.bindValue(":part", card.participantName);
    q.bindValue(":src", card.source);
    q.bindValue(":album", card.albumName);
    q.bindValue(":img", card.imageData);
    q.bindValue(":id", card.id);
    return exec(q, "updateCard");
}

bool Database::deleteCard(qint64 cardId) {
    QMutexLocker lock(&m_mutex);
    QSqlQuery q(m_db);
    q.prepare("DELETE FROM cards WHERE id = :id");
    q.bindValue(":id", cardId);
    return exec(q, "deleteCard");
}

bool Database::deleteCardByName(const QString& name) {
    QMutexLocker lock(&m_mutex);
    QSqlQuery q(m_db);
    q.prepare("DELETE FROM cards WHERE card_name = :name");
    q.bindValue(":name", name);
    return exec(q, "deleteCardByName");
}

Card Database::getCard(qint64 cardId) {
    QMutexLocker lock(&m_mutex);
    Card c;
    QSqlQuery q(m_db);
    q.prepare("SELECT * FROM cards WHERE id = :id");
    q.bindValue(":id", cardId);
    if (exec(q, "getCard") && q.next()) {
        c.id = q.value("id").toLongLong();
        c.cardName = q.value("card_name").toString();
        c.participantName = q.value("participant_name").toString();
        c.source = q.value("source").toString();
        c.albumName = q.value("album_name").toString();
        c.imageData = q.value("image_data").toByteArray();
        c.createdAt = q.value("created_at").toDateTime();
        c.updatedAt = q.value("updated_at").toDateTime();
    }
    return c;
}

QVector<Card> Database::getAllCards() {
    QMutexLocker lock(&m_mutex);
    QVector<Card> cards;
    QSqlQuery q(m_db);
    q.prepare("SELECT * FROM cards ORDER BY card_name");
    if (exec(q, "getAllCards")) {
        while (q.next()) {
            Card c;
            c.id = q.value("id").toLongLong();
            c.cardName = q.value("card_name").toString();
            c.participantName = q.value("participant_name").toString();
            c.source = q.value("source").toString();
            c.albumName = q.value("album_name").toString();
            c.imageData = q.value("image_data").toByteArray();
            c.createdAt = q.value("created_at").toDateTime();
            c.updatedAt = q.value("updated_at").toDateTime();
            cards.append(c);
        }
    }
    return cards;
}

// === User Card Status ===

void Database::ensureUserCardStatus(qint64 cardId, qint64 userId) {
    QSqlQuery q(m_db);
    q.prepare("INSERT INTO user_card_status (user_id, card_id) VALUES (:u, :c) ON CONFLICT DO NOTHING");
    q.bindValue(":u", userId);
    q.bindValue(":c", cardId);
    q.exec();
}

UserCardStatus Database::getCardStatus(qint64 cardId, qint64 userId) {
    QMutexLocker lock(&m_mutex);
    UserCardStatus s;
    s.cardId = cardId;
    s.userId = userId;
    
    QSqlQuery q(m_db);
    q.prepare("SELECT is_favorite, is_obtained FROM user_card_status WHERE card_id = :c AND user_id = :u");
    q.bindValue(":c", cardId);
    q.bindValue(":u", userId);
    if (exec(q, "getCardStatus") && q.next()) {
        s.isFavorite = q.value("is_favorite").toBool();
        s.isObtained = q.value("is_obtained").toBool();
    }
    return s;
}

bool Database::setFavorite(qint64 cardId, qint64 userId, bool value) {
    QMutexLocker lock(&m_mutex);
    ensureUserCardStatus(cardId, userId);
    QSqlQuery q(m_db);
    q.prepare("UPDATE user_card_status SET is_favorite = :v WHERE card_id = :c AND user_id = :u");
    q.bindValue(":v", value);
    q.bindValue(":c", cardId);
    q.bindValue(":u", userId);
    return exec(q, "setFavorite");
}

bool Database::setObtained(qint64 cardId, qint64 userId, bool value) {
    QMutexLocker lock(&m_mutex);
    ensureUserCardStatus(cardId, userId);
    QSqlQuery q(m_db);
    q.prepare("UPDATE user_card_status SET is_obtained = :v WHERE card_id = :c AND user_id = :u");
    q.bindValue(":v", value);
    q.bindValue(":c", cardId);
    q.bindValue(":u", userId);
    return exec(q, "setObtained");
}

bool Database::toggleFavorite(qint64 cardId, qint64 userId) {
    QMutexLocker lock(&m_mutex);
    ensureUserCardStatus(cardId, userId);
    QSqlQuery q(m_db);
    q.prepare("UPDATE user_card_status SET is_favorite = NOT is_favorite WHERE card_id = :c AND user_id = :u");
    q.bindValue(":c", cardId);
    q.bindValue(":u", userId);
    return exec(q, "toggleFavorite");
}

bool Database::toggleObtained(qint64 cardId, qint64 userId) {
    QMutexLocker lock(&m_mutex);
    ensureUserCardStatus(cardId, userId);
    QSqlQuery q(m_db);
    q.prepare("UPDATE user_card_status SET is_obtained = NOT is_obtained WHERE card_id = :c AND user_id = :u");
    q.bindValue(":c", cardId);
    q.bindValue(":u", userId);
    return exec(q, "toggleObtained");
}

QVector<CardWithStatus> Database::getCardsWithStatus(qint64 userId, const CardFilter& filter) {
    QMutexLocker lock(&m_mutex);
    QVector<CardWithStatus> result;
    
    QString sql = "SELECT c.*, COALESCE(s.is_favorite, FALSE) as is_favorite, "
                  "COALESCE(s.is_obtained, FALSE) as is_obtained "
                  "FROM cards c LEFT JOIN user_card_status s ON c.id = s.card_id AND s.user_id = :uid WHERE 1=1";
    
    QStringList conds;
    if (filter.filterObtained) conds << "COALESCE(s.is_obtained, FALSE) = :obt";
    if (filter.filterFavorite) conds << "COALESCE(s.is_favorite, FALSE) = TRUE";
    if (!filter.participant.isEmpty()) conds << "c.participant_name = :part";
    if (!filter.source.isEmpty()) conds << "c.source = :src";
    if (!filter.album.isEmpty()) conds << "c.album_name = :alb";
    if (!filter.searchText.isEmpty()) conds << "c.card_name ILIKE :search";
    
    if (!conds.isEmpty()) sql += " AND " + conds.join(" AND ");
    sql += " ORDER BY c.card_name";
    
    QSqlQuery q(m_db);
    q.prepare(sql);
    q.bindValue(":uid", userId);
    if (filter.filterObtained) q.bindValue(":obt", filter.obtainedValue);
    if (!filter.participant.isEmpty()) q.bindValue(":part", filter.participant);
    if (!filter.source.isEmpty()) q.bindValue(":src", filter.source);
    if (!filter.album.isEmpty()) q.bindValue(":alb", filter.album);
    if (!filter.searchText.isEmpty()) q.bindValue(":search", "%" + filter.searchText + "%");
    
    if (exec(q, "getCardsWithStatus")) {
        while (q.next()) {
            CardWithStatus cws;
            cws.card.id = q.value("id").toLongLong();
            cws.card.cardName = q.value("card_name").toString();
            cws.card.participantName = q.value("participant_name").toString();
            cws.card.source = q.value("source").toString();
            cws.card.albumName = q.value("album_name").toString();
            cws.card.imageData = q.value("image_data").toByteArray();
            cws.card.createdAt = q.value("created_at").toDateTime();
            cws.card.updatedAt = q.value("updated_at").toDateTime();
            cws.status.cardId = cws.card.id;
            cws.status.userId = userId;
            cws.status.isFavorite = q.value("is_favorite").toBool();
            cws.status.isObtained = q.value("is_obtained").toBool();
            result.append(cws);
        }
    }
    return result;
}

// === Statistics ===

int Database::countCards() {
    QMutexLocker lock(&m_mutex);
    QSqlQuery q(m_db);
    q.prepare("SELECT COUNT(*) FROM cards");
    return exec(q, "countCards") && q.next() ? q.value(0).toInt() : 0;
}

int Database::countObtained(qint64 userId) {
    QMutexLocker lock(&m_mutex);
    QSqlQuery q(m_db);
    q.prepare("SELECT COUNT(*) FROM user_card_status WHERE user_id = :u AND is_obtained = TRUE");
    q.bindValue(":u", userId);
    return exec(q, "countObtained") && q.next() ? q.value(0).toInt() : 0;
}

int Database::countFavorites(qint64 userId) {
    QMutexLocker lock(&m_mutex);
    QSqlQuery q(m_db);
    q.prepare("SELECT COUNT(*) FROM user_card_status WHERE user_id = :u AND is_favorite = TRUE");
    q.bindValue(":u", userId);
    return exec(q, "countFavorites") && q.next() ? q.value(0).toInt() : 0;
}

// === Custom Sources and Albums ===

QStringList Database::getCustomSources() {
    QMutexLocker lock(&m_mutex);
    QStringList list;
    QSqlQuery q(m_db);
    q.prepare("SELECT name FROM custom_sources ORDER BY name");
    if (exec(q, "getCustomSources")) {
        while (q.next()) list << q.value(0).toString();
    }
    return list;
}

QStringList Database::getCustomAlbums() {
    QMutexLocker lock(&m_mutex);
    QStringList list;
    QSqlQuery q(m_db);
    q.prepare("SELECT name FROM custom_albums ORDER BY name");
    if (exec(q, "getCustomAlbums")) {
        while (q.next()) list << q.value(0).toString();
    }
    return list;
}

bool Database::addCustomSource(const QString& name) {
    QMutexLocker lock(&m_mutex);
    QSqlQuery q(m_db);
    q.prepare("INSERT INTO custom_sources (name) VALUES (:n) ON CONFLICT DO NOTHING");
    q.bindValue(":n", name.trimmed());
    return exec(q, "addCustomSource");
}

bool Database::addCustomAlbum(const QString& name) {
    QMutexLocker lock(&m_mutex);
    QSqlQuery q(m_db);
    q.prepare("INSERT INTO custom_albums (name) VALUES (:n) ON CONFLICT DO NOTHING");
    q.bindValue(":n", name.trimmed());
    return exec(q, "addCustomAlbum");
}

bool Database::removeCustomSource(const QString& name) {
    QMutexLocker lock(&m_mutex);
    QSqlQuery q(m_db);
    q.prepare("DELETE FROM custom_sources WHERE name = :n");
    q.bindValue(":n", name);
    return exec(q, "removeCustomSource");
}

bool Database::removeCustomAlbum(const QString& name) {
    QMutexLocker lock(&m_mutex);
    QSqlQuery q(m_db);
    q.prepare("DELETE FROM custom_albums WHERE name = :n");
    q.bindValue(":n", name);
    return exec(q, "removeCustomAlbum");
}

QStringList Database::getAllSources() {
    QStringList all = App::SOURCES;
    all.append(getCustomSources());
    all.removeDuplicates();
    all.sort();
    return all;
}

QStringList Database::getAllAlbums() {
    QStringList all = App::ALBUMS;
    all.append(getCustomAlbums());
    all.removeDuplicates();
    all.sort();
    return all;
}

// === Export/Import with signature ===

bool Database::exportBinary(const QString& path, const QVector<qint64>& cardIds) {
    QVector<Card> cards;
    if (cardIds.isEmpty()) {
        cards = getAllCards();
    } else {
        for (qint64 id : cardIds) {
            Card c = getCard(id);
            if (c.id > 0) cards.append(c);
        }
    }
    
    // Serialize cards to buffer
    QByteArray buffer;
    QDataStream bufStream(&buffer, QIODevice::WriteOnly);
    bufStream.setVersion(QDataStream::Qt_5_15);
    bufStream << static_cast<quint32>(cards.size());
    for (const Card& c : cards) bufStream << c;
    
    // Calculate signature
    QByteArray signature = calculateSignature(buffer);
    
    // Write to file
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        m_error = "Cannot open file: " + path;
        return false;
    }
    
    QDataStream out(&file);
    out.setVersion(QDataStream::Qt_5_15);
    // Используем версию 4 для пользовательского экспорта
    out << App::BINARY_MAGIC << App::BINARY_VERSION_USER;
    out << signature;
    out.writeRawData(buffer.constData(), buffer.size());
    
    file.close();
    return true;
}

QVector<Card> Database::importBinary(const QString& path) {
    QVector<Card> cards;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        throw DbException("Cannot open file: " + path);
    }
    
    QDataStream in(&file);
    in.setVersion(QDataStream::Qt_5_15);
    
    quint32 magic, version;
    in >> magic >> version;
    
    if (magic != App::BINARY_MAGIC) {
        throw DbException("Invalid file format (wrong magic number)");
    }
    
    if (version == 1) {
        // Old format without signature
        throw DbException("Old file format without signature. Re-export required.");
    }
    
    if (version != App::BINARY_VERSION) {
        throw DbException("Unsupported file version");
    }
    
    // Read signature
    QByteArray signature;
    in >> signature;
    
    // Read data
    QByteArray buffer = file.readAll();
    file.close();
    
    // Verify signature
    if (!verifySignature(buffer, signature)) {
        throw DbException("File signature verification failed! File may be corrupted or tampered.");
    }
    
    // Parse cards
    QDataStream bufStream(&buffer, QIODevice::ReadOnly);
    bufStream.setVersion(QDataStream::Qt_5_15);
    
    quint32 count;
    bufStream >> count;
    
    for (quint32 i = 0; i < count; ++i) {
        Card c;
        bufStream >> c;
        c.id = 0;
        cards.append(c);
    }
    
    return cards;
}

bool Database::importCards(const QVector<Card>& cards) {
    for (Card c : cards) {
        c.id = 0;
        if (!addCard(c)) return false;
    }
    return true;
}

// Export with custom sources/albums settings (for admin)
bool Database::exportBinaryWithSettings(const QString& path, const QVector<qint64>& cardIds) {
    QVector<Card> cards;
    if (cardIds.isEmpty()) {
        cards = getAllCards();
    } else {
        for (qint64 id : cardIds) {
            Card c = getCard(id);
            if (c.id > 0) cards.append(c);
        }
    }
    
    // Get custom sources and albums
    QStringList customSources = getCustomSources();
    QStringList customAlbums = getCustomAlbums();
    
    // Serialize all data to buffer
    QByteArray buffer;
    QDataStream bufStream(&buffer, QIODevice::WriteOnly);
    bufStream.setVersion(QDataStream::Qt_5_15);
    
    // Write cards
    bufStream << static_cast<quint32>(cards.size());
    for (const Card& c : cards) bufStream << c;
    
    // Write custom sources
    bufStream << static_cast<quint32>(customSources.size());
    for (const QString& s : customSources) bufStream << s;
    
    // Write custom albums
    bufStream << static_cast<quint32>(customAlbums.size());
    for (const QString& a : customAlbums) bufStream << a;
    
    // Calculate signature
    QByteArray signature = calculateSignature(buffer);
    
    // Write to file
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        m_error = "Cannot open file: " + path;
        return false;
    }
    
    QDataStream out(&file);
    out.setVersion(QDataStream::Qt_5_15);
    out << App::BINARY_MAGIC << App::BINARY_VERSION_SETTINGS;
    out << signature;
    out.writeRawData(buffer.constData(), buffer.size());
    
    file.close();
    return true;
}

// Export with user statuses (for user export - includes favorite/obtained)
bool Database::exportBinaryWithStatuses(const QString& path, qint64 userId, const QVector<qint64>& cardIds) {
    QVector<Card> cards;
    QVector<UserCardStatus> statuses;
    
    if (cardIds.isEmpty()) {
        cards = getAllCards();
    } else {
        for (qint64 id : cardIds) {
            Card c = getCard(id);
            if (c.id > 0) cards.append(c);
        }
    }
    
    // Получаем статусы для каждой карты
    for (const Card& c : cards) {
        UserCardStatus status = getCardStatus(c.id, userId);
        statuses.append(status);
    }
    
    // Serialize all data to buffer
    QByteArray buffer;
    QDataStream bufStream(&buffer, QIODevice::WriteOnly);
    bufStream.setVersion(QDataStream::Qt_5_15);
    
    // Write cards count
    bufStream << static_cast<quint32>(cards.size());
    
    // Write cards with their statuses
    for (int i = 0; i < cards.size(); ++i) {
        bufStream << cards[i];
        bufStream << statuses[i];
    }
    
    // Calculate signature
    QByteArray signature = calculateSignature(buffer);
    
    // Write to file
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        m_error = "Cannot open file: " + path;
        return false;
    }
    
    QDataStream out(&file);
    out.setVersion(QDataStream::Qt_5_15);
    out << App::BINARY_MAGIC << App::BINARY_VERSION_USER;
    out << signature;
    out.writeRawData(buffer.constData(), buffer.size());
    
    file.close();
    return true;
}

// Import with settings (for regular users)
Database::ImportResult Database::importBinaryWithSettings(const QString& path) {
    ImportResult result;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        throw DbException("Cannot open file: " + path);
    }
    
    QDataStream in(&file);
    in.setVersion(QDataStream::Qt_5_15);
    
    quint32 magic, version;
    in >> magic >> version;
    
    if (magic != App::BINARY_MAGIC) {
        throw DbException("Invalid file format (wrong magic number)");
    }
    
    if (version == 1) {
        throw DbException("Old file format without signature. Re-export required.");
    }
    
    if (version != App::BINARY_VERSION && version != App::BINARY_VERSION_SETTINGS && version != App::BINARY_VERSION_USER) {
        throw DbException("Unsupported file version");
    }
    
    // Read signature
    QByteArray signature;
    in >> signature;
    
    // Read data
    QByteArray buffer = file.readAll();
    file.close();
    
    // Verify signature
    if (!verifySignature(buffer, signature)) {
        throw DbException("File signature verification failed! File may be corrupted or tampered.");
    }
    
    // Parse data
    QDataStream bufStream(&buffer, QIODevice::ReadOnly);
    bufStream.setVersion(QDataStream::Qt_5_15);
    
    // Read cards count
    quint32 count;
    bufStream >> count;
    
    // Определяем источник файла и читаем данные соответственно
    if (version == App::BINARY_VERSION_USER) {
        // Версия 4 - от пользователя с статусами
        result.isFromAdmin = false;
        result.hasSettings = false;
        
        // Читаем карты вместе со статусами
        for (quint32 i = 0; i < count; ++i) {
            Card c;
            UserCardStatus status;
            bufStream >> c;
            bufStream >> status;
            c.id = 0;
            result.cards.append(c);
            result.cardStatuses.append(status);
        }
    } else if (version == App::BINARY_VERSION_SETTINGS) {
        // Версия 3 - от админа с настройками
        result.hasSettings = true;
        result.isFromAdmin = true;
        
        // Читаем карты
        for (quint32 i = 0; i < count; ++i) {
            Card c;
            bufStream >> c;
            c.id = 0;
            result.cards.append(c);
        }
        
        // Read custom sources
        quint32 srcCount;
        bufStream >> srcCount;
        for (quint32 i = 0; i < srcCount; ++i) {
            QString s;
            bufStream >> s;
            result.customSources.append(s);
        }
        
        // Read custom albums
        quint32 albCount;
        bufStream >> albCount;
        for (quint32 i = 0; i < albCount; ++i) {
            QString a;
            bufStream >> a;
            result.customAlbums.append(a);
        }
    } else {
        // Версия 2 - старая, считаем от пользователя без статусов
        result.isFromAdmin = false;
        result.hasSettings = false;
        
        for (quint32 i = 0; i < count; ++i) {
            Card c;
            bufStream >> c;
            c.id = 0;
            result.cards.append(c);
        }
    }
    
    return result;
}

// Apply imported settings (custom sources/albums)
bool Database::applyImportedSettings(const ImportResult& result) {
    if (!result.hasSettings) return true;
    
    // Add custom sources
    for (const QString& s : result.customSources) {
        addCustomSource(s);
    }
    
    // Add custom albums
    for (const QString& a : result.customAlbums) {
        addCustomAlbum(a);
    }
    
    return true;
}

// Проверка существования карты по имени
bool Database::cardExistsByName(const QString& name) {
    QMutexLocker lock(&m_mutex);
    QSqlQuery q(m_db);
    q.prepare("SELECT COUNT(*) FROM cards WHERE card_name = :name");
    q.bindValue(":name", name);
    return exec(q, "cardExistsByName") && q.next() && q.value(0).toInt() > 0;
}

// Получение карты по имени
Card Database::getCardByName(const QString& name) {
    QMutexLocker lock(&m_mutex);
    Card c;
    QSqlQuery q(m_db);
    q.prepare("SELECT * FROM cards WHERE card_name = :name");
    q.bindValue(":name", name);
    if (exec(q, "getCardByName") && q.next()) {
        c.id = q.value("id").toLongLong();
        c.cardName = q.value("card_name").toString();
        c.participantName = q.value("participant_name").toString();
        c.source = q.value("source").toString();
        c.albumName = q.value("album_name").toString();
        c.imageData = q.value("image_data").toByteArray();
        c.createdAt = q.value("created_at").toDateTime();
        c.updatedAt = q.value("updated_at").toDateTime();
    }
    return c;
}

// Импорт карт от пользователя - замена существующих, сохранение настроек импортируемых
bool Database::importCardsFromUser(const QVector<Card>& cards, qint64 userId) {
    for (Card c : cards) {
        // Проверяем, существует ли карта с таким именем
        Card existing = getCardByName(c.cardName);
        if (existing.id > 0) {
            // Карта существует - обновляем её данными из импорта
            c.id = existing.id;
            if (!updateCard(c)) return false;
        } else {
            // Новая карта - добавляем
            c.id = 0;
            if (!addCard(c)) return false;
        }
    }
    return true;
}

// Импорт карт от админа - добавление только новых, сброс избранного/получено
bool Database::importCardsFromAdmin(const QVector<Card>& cards, qint64 userId) {
    for (Card c : cards) {
        // Проверяем, существует ли карта с таким именем
        Card existing = getCardByName(c.cardName);
        if (existing.id > 0) {
            // Карта уже существует - пропускаем, не трогаем настройки пользователя
            continue;
        }
        
        // Новая карта - добавляем
        c.id = 0;
        if (!addCard(c)) return false;
        
        // Сбрасываем избранное и получено для нового пользователя
        // (по умолчанию они и так false, но явно указываем для ясности)
        setFavorite(c.id, userId, false);
        setObtained(c.id, userId, false);
    }
    return true;
}
