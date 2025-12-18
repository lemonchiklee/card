#ifndef DATABASE_H
#define DATABASE_H

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QString>
#include <QVector>
#include <QMutex>
#include <stdexcept>

#include "card.h"
#include "user.h"

class DbException : public std::runtime_error {
public:
    explicit DbException(const QString& msg) : std::runtime_error(msg.toStdString()) {}
};

struct DbConfig {
    QString host = "localhost";
    int port = 5432;
    QString database;
    QString user;
    QString password;
};

struct CardFilter {
    bool filterObtained = false;
    bool obtainedValue = false;
    bool filterFavorite = false;
    QString participant;
    QString source;
    QString album;
    QString searchText;
    
    bool hasAny() const {
        return filterObtained || filterFavorite ||
               !participant.isEmpty() || !source.isEmpty() ||
               !album.isEmpty() || !searchText.isEmpty();
    }
};

struct CardWithStatus {
    Card card;
    UserCardStatus status;
};

class Database {
public:
    static Database& get();
    
    bool connect(const DbConfig& config);
    bool isOpen() const;
    void close();
    
    static QString hashPwd(const QString& password);
    
    // Пользователи
    bool registerUser(const QString& username, const QString& password);
    bool authenticate(const QString& username, const QString& password, User& outUser);
    bool authenticateByHash(const QString& username, const QString& hash, User& outUser);
    bool changeAdminCreds(qint64 adminId, const QString& newUser, const QString& newPass);
    bool userExists(const QString& username);
    
    // Карты (только админ)
    bool addCard(Card& card);
    bool updateCard(const Card& card);
    bool deleteCard(qint64 cardId);
    bool deleteCardByName(const QString& name);
    Card getCard(qint64 cardId);
    QVector<Card> getAllCards();
    
    // Пользовательские статусы
    UserCardStatus getCardStatus(qint64 cardId, qint64 userId);
    bool setFavorite(qint64 cardId, qint64 userId, bool value);
    bool setObtained(qint64 cardId, qint64 userId, bool value);
    bool toggleFavorite(qint64 cardId, qint64 userId);
    bool toggleObtained(qint64 cardId, qint64 userId);
    
    // Получение карт с фильтрами
    QVector<CardWithStatus> getCardsWithStatus(qint64 userId, const CardFilter& filter = CardFilter());
    
    // Статистика
    int countCards();
    int countObtained(qint64 userId);
    int countFavorites(qint64 userId);
    
    // Пользовательские источники и альбомы (только админ)
    QStringList getCustomSources();
    QStringList getCustomAlbums();
    bool addCustomSource(const QString& name);
    bool addCustomAlbum(const QString& name);
    bool removeCustomSource(const QString& name);
    bool removeCustomAlbum(const QString& name);
    QStringList getAllSources();  // builtin + custom
    QStringList getAllAlbums();   // builtin + custom
    
    // Экспорт/импорт с подписью
    bool exportBinary(const QString& path, const QVector<qint64>& cardIds = {});
    bool exportBinaryWithSettings(const QString& path, const QVector<qint64>& cardIds = {});  // Admin export with filter settings
    QVector<Card> importBinary(const QString& path);  // throws on invalid signature
    bool importCards(const QVector<Card>& cards);
    
    // Структура для импорта с настройками
    struct ImportResult {
        QVector<Card> cards;
        QVector<UserCardStatus> cardStatuses;  // Статусы карт (избранное/получено) - для импорта от пользователя
        QStringList customSources;
        QStringList customAlbums;
        QStringList customParticipants;  // Участники (фильтры админа)
        bool hasSettings = false;
        bool isFromAdmin = false;  // true если файл экспортирован админом (версия 3+)
    };
    ImportResult importBinaryWithSettings(const QString& path);  // Import with filter settings (for users)
    bool applyImportedSettings(const ImportResult& result);  // Apply custom sources/albums from import
    
    // Экспорт со статусами (для пользователя)
    bool exportBinaryWithStatuses(const QString& path, qint64 userId, const QVector<qint64>& cardIds = {});
    
    // Импорт карт с разной логикой в зависимости от источника
    bool importCardsFromUser(const QVector<Card>& cards, qint64 userId);  // Замена существующих карт, сохранение настроек импортируемых
    bool importCardsFromAdmin(const QVector<Card>& cards, qint64 userId);  // Добавление только новых, сброс избранного/получено
    
    // Проверка существования карты по имени
    bool cardExistsByName(const QString& name);
    Card getCardByName(const QString& name);
    
    // Проверка подписи файла
    static QByteArray calculateSignature(const QByteArray& data);
    static bool verifySignature(const QByteArray& data, const QByteArray& signature);
    
    QString lastError() const { return m_error; }
    
private:
    Database();
    ~Database();
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;
    
    bool exec(QSqlQuery& q, const QString& desc = QString());
    void ensureUserCardStatus(qint64 cardId, qint64 userId);
    
    QSqlDatabase m_db;
    QString m_error;
    mutable QMutex m_mutex;
    bool m_connected = false;
};

#endif
