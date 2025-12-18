#include "mainwindow.h"
#include "carddialog.h"
#include "constants.h"
#include <QApplication>
#include <QMenuBar>
#include <QMenu>
#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QTimer>
#include <QCloseEvent>

MainWindow::MainWindow(const User& user, QWidget* parent)
    : QMainWindow(parent), m_user(user) {
    setupUi();
    setupMenu();
    applyStyle();
    
    setWindowTitle(App::APP_NAME + " - Менеджер коллекционных карт");
    setMinimumSize(900, 650);
    resize(1100, 750);
    
    QTimer::singleShot(100, this, &MainWindow::loadCards);
}

MainWindow::~MainWindow() { clearCards(); }

void MainWindow::closeEvent(QCloseEvent* event) {
    if (m_logoutRequested) {
        // Logout - just close, login dialog will be shown
        event->accept();
        QApplication::exit(42);  // Special code for logout
        return;
    }
    
    // Normal close - ask to quit
    if (QMessageBox::question(this, "Выход", "Закрыть программу?",
            QMessageBox::Yes | QMessageBox::No) == QMessageBox::No) {
        event->ignore();
        return;
    }
    
    event->accept();
    QApplication::exit(0);  // Normal exit - close everything
}

QString MainWindow::getHostPath(const QString& title, bool save, const QString& filter) {
    // Use HOST_HOME to access host filesystem instead of Docker filesystem
    if (save) {
        return QFileDialog::getSaveFileName(this, title, App::getDefaultPath(), filter);
    } else {
        return QFileDialog::getOpenFileName(this, title, App::getDefaultPath(), filter);
    }
}

void MainWindow::setupUi() {
    m_central = new QWidget();
    setCentralWidget(m_central);
    
    m_mainLayout = new QVBoxLayout(m_central);
    m_mainLayout->setSpacing(12);
    m_mainLayout->setContentsMargins(15, 15, 15, 15);
    
    QHBoxLayout* header = new QHBoxLayout();
    QLabel* title = new QLabel(App::APP_NAME);
    title->setObjectName("mainTitle");
    header->addWidget(title);
    header->addStretch();
    m_userLabel = new QLabel();
    m_userLabel->setObjectName("userInfo");
    header->addWidget(m_userLabel);
    m_mainLayout->addLayout(header);
    
    setupToolbar();
    setupFilterPanel();
    setupCardList();
    setupStatusBar();
    updateStatus();
}

void MainWindow::setupMenu() {
    QMenuBar* mb = menuBar();
    
    QMenu* fileMenu = mb->addMenu("Файл");
    
    if (m_user.isAdmin) {
        QAction* addAct = fileMenu->addAction("Добавить карту");
        addAct->setShortcut(QKeySequence::New);
        connect(addAct, &QAction::triggered, this, &MainWindow::onAddCard);
        fileMenu->addSeparator();
    }
    
    QAction* exportSelAct = fileMenu->addAction("Экспорт выбранных");
    connect(exportSelAct, &QAction::triggered, this, &MainWindow::onExportSelected);
    
    QAction* exportAllAct = fileMenu->addAction("Экспорт всех");
    connect(exportAllAct, &QAction::triggered, this, &MainWindow::onExportAll);
    
    if (m_user.isAdmin) {
        QAction* importAct = fileMenu->addAction("Импорт из файла (админ)");
        connect(importAct, &QAction::triggered, this, &MainWindow::onImport);
    } else {
        QAction* userImportAct = fileMenu->addAction("Импорт из файла");
        connect(userImportAct, &QAction::triggered, this, &MainWindow::onUserImport);
    }
    
    QAction* viewAct = fileMenu->addAction("Просмотр бинарного файла");
    connect(viewAct, &QAction::triggered, this, &MainWindow::onViewBinaryFile);
    
    fileMenu->addSeparator();
    
    QAction* refreshAct = fileMenu->addAction("Обновить");
    refreshAct->setShortcut(QKeySequence::Refresh);
    connect(refreshAct, &QAction::triggered, this, &MainWindow::onRefresh);
    
    fileMenu->addSeparator();
    
    QAction* logoutAct = fileMenu->addAction("Выйти из аккаунта");
    connect(logoutAct, &QAction::triggered, this, &MainWindow::onLogout);
    
    QAction* exitAct = fileMenu->addAction("Закрыть программу");
    exitAct->setShortcut(QKeySequence::Quit);
    connect(exitAct, &QAction::triggered, [this]() {
        m_logoutRequested = false;
        close();
    });
    
    if (m_user.isAdmin) {
        QMenu* adminMenu = mb->addMenu("Администрирование");
        QAction* credAct = adminMenu->addAction("Изменить логин/пароль");
        connect(credAct, &QAction::triggered, this, &MainWindow::onChangeAdminCreds);
        
        adminMenu->addSeparator();
        
        QAction* addSourceAct = adminMenu->addAction("Добавить источник");
        connect(addSourceAct, &QAction::triggered, this, &MainWindow::onAddCustomSource);
        
        QAction* addAlbumAct = adminMenu->addAction("Добавить альбом");
        connect(addAlbumAct, &QAction::triggered, this, &MainWindow::onAddCustomAlbum);
        
        adminMenu->addSeparator();
        
        QAction* delSourcesAct = adminMenu->addAction("Удаление источников");
        connect(delSourcesAct, &QAction::triggered, this, &MainWindow::onDeleteSources);
        
        QAction* delAlbumsAct = adminMenu->addAction("Удаление альбомов");
        connect(delAlbumsAct, &QAction::triggered, this, &MainWindow::onDeleteAlbums);
    }
    
    QMenu* helpMenu = mb->addMenu("Справка");
    QAction* aboutAct = helpMenu->addAction("О программе");
    connect(aboutAct, &QAction::triggered, this, &MainWindow::onAbout);
}

void MainWindow::setupToolbar() {
    QHBoxLayout* toolbar = new QHBoxLayout();
    toolbar->setSpacing(10);
    
    if (m_user.isAdmin) {
        QPushButton* addBtn = new QPushButton("+ Добавить карту");
        addBtn->setObjectName("primary");
        connect(addBtn, &QPushButton::clicked, this, &MainWindow::onAddCard);
        toolbar->addWidget(addBtn);
    }
    
    m_toggleFilterBtn = new QPushButton("Фильтры");
    m_toggleFilterBtn->setObjectName("toggleBtn");
    connect(m_toggleFilterBtn, &QPushButton::clicked, this, &MainWindow::onToggleFilterPanel);
    toolbar->addWidget(m_toggleFilterBtn);
    
    m_searchEdit = new QLineEdit();
    m_searchEdit->setPlaceholderText("Поиск по названию...");
    m_searchEdit->setMinimumWidth(200);
    m_searchEdit->setMaximumWidth(350);
    connect(m_searchEdit, &QLineEdit::returnPressed, this, &MainWindow::onApplyFilters);
    toolbar->addWidget(m_searchEdit);
    
    toolbar->addStretch();
    
    QPushButton* exportBtn = new QPushButton("Экспорт выбранных");
    connect(exportBtn, &QPushButton::clicked, this, &MainWindow::onExportSelected);
    toolbar->addWidget(exportBtn);
    
    if (m_user.isAdmin) {
        QPushButton* importBtn = new QPushButton("Импорт (админ)");
        connect(importBtn, &QPushButton::clicked, this, &MainWindow::onImport);
        toolbar->addWidget(importBtn);
    } else {
        QPushButton* userImportBtn = new QPushButton("Импорт");
        connect(userImportBtn, &QPushButton::clicked, this, &MainWindow::onUserImport);
        toolbar->addWidget(userImportBtn);
    }
    
    QPushButton* refreshBtn = new QPushButton("Обновить");
    connect(refreshBtn, &QPushButton::clicked, this, &MainWindow::onRefresh);
    toolbar->addWidget(refreshBtn);
    
    m_mainLayout->addLayout(toolbar);
}

void MainWindow::setupFilterPanel() {
    m_filterPanel = new QWidget();
    m_filterPanel->setObjectName("filterPanel");
    m_filterPanel->setVisible(false);
    
    QVBoxLayout* layout = new QVBoxLayout(m_filterPanel);
    layout->setContentsMargins(15, 15, 15, 15);
    layout->setSpacing(12);
    
    QLabel* title = new QLabel("Параметры фильтрации");
    title->setObjectName("filterTitle");
    layout->addWidget(title);
    
    QHBoxLayout* row = new QHBoxLayout();
    row->setSpacing(20);
    
    // Checkboxes for obtained filter
    QVBoxLayout* obtLayout = new QVBoxLayout();
    m_filterObtainedCheck = new QCheckBox("Только полученные");
    obtLayout->addWidget(m_filterObtainedCheck);
    m_filterNotObtainedCheck = new QCheckBox("Только не полученные");
    obtLayout->addWidget(m_filterNotObtainedCheck);
    // Make them mutually exclusive
    connect(m_filterObtainedCheck, &QCheckBox::toggled, [this](bool checked) {
        if (checked) m_filterNotObtainedCheck->setChecked(false);
    });
    connect(m_filterNotObtainedCheck, &QCheckBox::toggled, [this](bool checked) {
        if (checked) m_filterObtainedCheck->setChecked(false);
    });
    row->addLayout(obtLayout);
    
    QVBoxLayout* partLayout = new QVBoxLayout();
    partLayout->addWidget(new QLabel("Участник:"));
    m_filterParticipantCombo = new QComboBox();
    m_filterParticipantCombo->addItem("Все", "");
    for (const QString& p : App::PARTICIPANTS) m_filterParticipantCombo->addItem(p, p);
    m_filterParticipantCombo->setMinimumWidth(140);
    partLayout->addWidget(m_filterParticipantCombo);
    row->addLayout(partLayout);
    
    QVBoxLayout* srcLayout = new QVBoxLayout();
    srcLayout->addWidget(new QLabel("Источник:"));
    m_filterSourceCombo = new QComboBox();
    m_filterSourceCombo->addItem("Все", "");
    for (const QString& s : Database::get().getAllSources()) m_filterSourceCombo->addItem(s, s);
    m_filterSourceCombo->setMinimumWidth(160);
    srcLayout->addWidget(m_filterSourceCombo);
    row->addLayout(srcLayout);
    
    QVBoxLayout* albLayout = new QVBoxLayout();
    albLayout->addWidget(new QLabel("Альбом:"));
    m_filterAlbumCombo = new QComboBox();
    m_filterAlbumCombo->addItem("Все", "");
    for (const QString& a : Database::get().getAllAlbums()) m_filterAlbumCombo->addItem(a, a);
    m_filterAlbumCombo->setMinimumWidth(220);
    albLayout->addWidget(m_filterAlbumCombo);
    row->addLayout(albLayout);
    
    QVBoxLayout* checkLayout = new QVBoxLayout();
    m_filterFavoriteCheck = new QCheckBox("★ Только избранные");
    checkLayout->addWidget(m_filterFavoriteCheck);
    row->addLayout(checkLayout);
    
    row->addStretch();
    layout->addLayout(row);
    
    QHBoxLayout* btns = new QHBoxLayout();
    QPushButton* applyBtn = new QPushButton("Применить");
    applyBtn->setObjectName("primary");
    connect(applyBtn, &QPushButton::clicked, this, &MainWindow::onApplyFilters);
    btns->addWidget(applyBtn);
    
    QPushButton* clearBtn = new QPushButton("Сбросить");
    connect(clearBtn, &QPushButton::clicked, this, &MainWindow::onClearFilters);
    btns->addWidget(clearBtn);
    btns->addStretch();
    layout->addLayout(btns);
    
    m_mainLayout->addWidget(m_filterPanel);
}

void MainWindow::setupCardList() {
    m_scroll = new QScrollArea();
    m_scroll->setWidgetResizable(true);
    m_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scroll->setObjectName("cardScroll");
    
    m_cardContainer = new QWidget();
    m_cardContainer->setObjectName("cardContainer");
    
    m_cardLayout = new QVBoxLayout(m_cardContainer);
    m_cardLayout->setSpacing(12);
    m_cardLayout->setContentsMargins(5, 5, 5, 5);
    m_cardLayout->addStretch();
    
    m_scroll->setWidget(m_cardContainer);
    m_mainLayout->addWidget(m_scroll, 1);
}

void MainWindow::setupStatusBar() {
    QHBoxLayout* status = new QHBoxLayout();
    status->setSpacing(20);
    
    m_countLabel = new QLabel("Карт: 0");
    m_countLabel->setObjectName("statusLabel");
    status->addWidget(m_countLabel);
    
    m_selectedLabel = new QLabel("Выбрано: 0");
    m_selectedLabel->setObjectName("statusLabel");
    status->addWidget(m_selectedLabel);
    
    m_statsLabel = new QLabel();
    m_statsLabel->setObjectName("statusLabel");
    status->addWidget(m_statsLabel);
    
    status->addStretch();
    m_mainLayout->addLayout(status);
}

void MainWindow::applyStyle() {
    setStyleSheet(R"(
        QMainWindow { background: #1a1b2e; }
        QWidget { background: #1a1b2e; color: #e0e0e0; }
        QLabel { color: #e0e0e0; }
        QLabel#mainTitle { font-size: 26px; font-weight: bold; color: #ff4757; }
        QLabel#userInfo { font-size: 13px; color: #7f8c8d; }
        QLabel#filterTitle { font-size: 15px; font-weight: bold; }
        QLabel#statusLabel { font-size: 12px; color: #7f8c8d; }
        QLineEdit { padding: 10px; border: 2px solid #3d3d5c; border-radius: 6px; background: #2d2d44; color: #e0e0e0; }
        QLineEdit:focus { border-color: #ff4757; }
        QPushButton { background: #3d3d5c; color: #e0e0e0; border: none; padding: 10px 18px; border-radius: 6px; font-size: 13px; }
        QPushButton:hover { background: #4d4d6c; }
        QPushButton#primary { background: #ff4757; color: white; font-weight: bold; }
        QPushButton#primary:hover { background: #ff6b81; }
        QPushButton#toggleBtn { background: #2d2d44; border: 2px solid #3d3d5c; }
        QPushButton#toggleBtn:hover { border-color: #ff4757; }
        QWidget#filterPanel { background: #2d2d44; border: 2px solid #3d3d5c; border-radius: 10px; }
        QComboBox { background: #3d3d5c; color: #e0e0e0; border: 2px solid #4d4d6c; border-radius: 6px; padding: 8px; min-height: 20px; }
        QComboBox:disabled { background: #2d2d44; color: #5d5d7c; }
        QComboBox QAbstractItemView { background: #3d3d5c; color: #e0e0e0; selection-background-color: #ff4757; }
        QCheckBox { color: #e0e0e0; font-size: 13px; spacing: 8px; }
        QCheckBox::indicator { width: 18px; height: 18px; border-radius: 4px; border: 2px solid #3d3d5c; background: #2d2d44; }
        QCheckBox::indicator:checked { background: #ff4757; border-color: #ff4757; }
        QScrollArea#cardScroll { border: none; background: transparent; }
        QWidget#cardContainer { background: transparent; }
        QScrollBar:vertical { background: #2d2d44; width: 12px; border-radius: 6px; }
        QScrollBar::handle:vertical { background: #3d3d5c; border-radius: 5px; min-height: 30px; }
        QScrollBar::handle:vertical:hover { background: #4d4d6c; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
        QMenuBar { background: #2d2d44; color: #e0e0e0; padding: 5px; }
        QMenuBar::item { padding: 8px 15px; border-radius: 4px; }
        QMenuBar::item:selected { background: #3d3d5c; }
        QMenu { background: #2d2d44; color: #e0e0e0; border: 2px solid #3d3d5c; border-radius: 6px; padding: 5px; }
        QMenu::item { padding: 8px 25px; border-radius: 4px; }
        QMenu::item:selected { background: #ff4757; }
        QMenu::separator { height: 1px; background: #3d3d5c; margin: 5px 10px; }
    )");
}

void MainWindow::updateStatus() {
    QString role = m_user.isAdmin ? "Администратор" : "Пользователь";
    m_userLabel->setText(role + ": " + m_user.username);
    
    int total = Database::get().countCards();
    int obtained = Database::get().countObtained(m_user.id);
    int favorites = Database::get().countFavorites(m_user.id);
    
    m_statsLabel->setText(QString("Получено: %1/%2 | Избранное: %3")
        .arg(obtained).arg(total).arg(favorites));
}

void MainWindow::clearCards() {
    for (auto* w : m_widgets) delete w;
    m_widgets.clear();
    m_selected.clear();
}

CardFilter MainWindow::buildFilter() const {
    CardFilter f;
    // If "only obtained" is checked, filter for obtained
    // If "only not obtained" is checked, filter for not obtained
    if (m_filterObtainedCheck->isChecked()) {
        f.filterObtained = true;
        f.obtainedValue = true;
    } else if (m_filterNotObtainedCheck->isChecked()) {
        f.filterObtained = true;
        f.obtainedValue = false;
    }
    f.filterFavorite = m_filterFavoriteCheck->isChecked();
    f.participant = m_filterParticipantCombo->currentData().toString();
    f.source = m_filterSourceCombo->currentData().toString();
    f.album = m_filterAlbumCombo->currentData().toString();
    f.searchText = m_searchEdit->text().trimmed();
    return f;
}

void MainWindow::displayCards(const QVector<CardWithStatus>& cards) {
    clearCards();
    
    QLayoutItem* item;
    while ((item = m_cardLayout->takeAt(0)) != nullptr) delete item;
    
    for (const CardWithStatus& cws : cards) {
        CardWidget* w = new CardWidget(cws, m_user.isAdmin);
        
        connect(w, &CardWidget::favoriteClicked, this, &MainWindow::onToggleFavorite);
        connect(w, &CardWidget::obtainedClicked, this, &MainWindow::onToggleObtained);
        connect(w, &CardWidget::editClicked, this, &MainWindow::onEditCard);
        connect(w, &CardWidget::deleteClicked, this, &MainWindow::onDeleteCard);
        connect(w, &CardWidget::selectionChanged, this, &MainWindow::onSelectionChanged);
        
        m_cardLayout->addWidget(w);
        m_widgets[cws.card.id] = w;
    }
    
    m_cardLayout->addStretch();
    m_countLabel->setText(QString("Карт: %1").arg(cards.size()));
    m_selectedLabel->setText(QString("Выбрано: %1").arg(m_selected.size()));
}

void MainWindow::loadCards() {
    CardFilter f = buildFilter();
    m_cards = Database::get().getCardsWithStatus(m_user.id, f);
    displayCards(m_cards);
    updateStatus();
}

void MainWindow::onAddCard() {
    if (!m_user.isAdmin) return;
    
    CardDialog dlg(this);
    dlg.setCard(Card());
    
    if (dlg.exec() == QDialog::Accepted) {
        Card c = dlg.getCard();
        if (Database::get().addCard(c)) {
            loadCards();
            QMessageBox::information(this, "Успех", "Карта добавлена");
        } else {
            QMessageBox::critical(this, "Ошибка", Database::get().lastError());
        }
    }
}

void MainWindow::onEditCard(qint64 id) {
    if (!m_user.isAdmin) return;
    
    Card c = Database::get().getCard(id);
    if (c.id == 0) return;
    
    CardDialog dlg(this);
    dlg.setCard(c);
    
    if (dlg.exec() == QDialog::Accepted) {
        Card updated = dlg.getCard();
        if (Database::get().updateCard(updated)) {
            loadCards();
            QMessageBox::information(this, "Успех", "Карта обновлена");
        } else {
            QMessageBox::critical(this, "Ошибка", Database::get().lastError());
        }
    }
}

void MainWindow::onDeleteCard(qint64 id) {
    if (!m_user.isAdmin) return;
    
    Card c = Database::get().getCard(id);
    
    if (QMessageBox::question(this, "Подтверждение",
            QString("Удалить карту \"%1\"?").arg(c.cardName),
            QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
        if (Database::get().deleteCard(id)) {
            m_selected.remove(id);
            loadCards();
            QMessageBox::information(this, "Успех", "Карта удалена");
        } else {
            QMessageBox::critical(this, "Ошибка", Database::get().lastError());
        }
    }
}

void MainWindow::onToggleFavorite(qint64 id) {
    if (Database::get().toggleFavorite(id, m_user.id)) {
        if (m_widgets.contains(id)) {
            CardWithStatus cws;
            cws.card = Database::get().getCard(id);
            cws.status = Database::get().getCardStatus(id, m_user.id);
            m_widgets[id]->setData(cws);
        }
        updateStatus();
        if (m_filterFavoriteCheck->isChecked()) loadCards();
    }
}

void MainWindow::onToggleObtained(qint64 id) {
    if (Database::get().toggleObtained(id, m_user.id)) {
        if (m_widgets.contains(id)) {
            CardWithStatus cws;
            cws.card = Database::get().getCard(id);
            cws.status = Database::get().getCardStatus(id, m_user.id);
            m_widgets[id]->setData(cws);
        }
        updateStatus();
        if (m_filterObtainedCheck->isChecked()) loadCards();
    }
}

void MainWindow::onSelectionChanged(qint64 id, bool sel) {
    if (sel) m_selected.insert(id);
    else m_selected.remove(id);
    m_selectedLabel->setText(QString("Выбрано: %1").arg(m_selected.size()));
}

void MainWindow::onApplyFilters() { loadCards(); }

void MainWindow::onClearFilters() {
    m_searchEdit->clear();
    m_filterObtainedCheck->setChecked(false);
    m_filterNotObtainedCheck->setChecked(false);
    m_filterParticipantCombo->setCurrentIndex(0);
    m_filterSourceCombo->setCurrentIndex(0);
    m_filterAlbumCombo->setCurrentIndex(0);
    m_filterFavoriteCheck->setChecked(false);
    loadCards();
}

void MainWindow::onToggleFilterPanel() {
    bool vis = !m_filterPanel->isVisible();
    m_filterPanel->setVisible(vis);
    m_toggleFilterBtn->setText(vis ? "Скрыть фильтры" : "Фильтры");
}

void MainWindow::onExportSelected() {
    if (m_selected.isEmpty()) {
        QMessageBox::warning(this, "Внимание", "Выберите карты для экспорта (кликните на них)");
        return;
    }
    
    QString path = getHostPath("Экспорт", true, "CardCab (*.ccb);;All (*)");
    if (path.isEmpty()) return;
    
    if (!path.endsWith(".ccb")) path += ".ccb";
    
    QVector<qint64> ids(m_selected.begin(), m_selected.end());
    bool success;
    if (m_user.isAdmin) {
        // Admin export includes filter settings (custom sources/albums)
        success = Database::get().exportBinaryWithSettings(path, ids);
    } else {
        // User export includes card statuses (favorite/obtained)
        success = Database::get().exportBinaryWithStatuses(path, m_user.id, ids);
    }
    
    if (success) {
        m_lastExport = path;
        QString msg = QString("Экспортировано %1 карт").arg(ids.size());
        if (m_user.isAdmin) {
            msg += "\n(включая настройки фильтров)";
        } else {
            msg += "\n(включая настройки избранного/получено)";
        }
        msg += QString("\n\nФайл: %1").arg(path);
        QMessageBox::information(this, "Экспорт", msg);
    } else {
        QMessageBox::critical(this, "Ошибка", Database::get().lastError());
    }
}

void MainWindow::onExportAll() {
    if (m_cards.isEmpty()) {
        QMessageBox::warning(this, "Внимание", "Нет карт для экспорта");
        return;
    }
    
    QString path = getHostPath("Экспорт всех", true, "CardCab (*.ccb);;All (*)");
    if (path.isEmpty()) return;
    
    if (!path.endsWith(".ccb")) path += ".ccb";
    
    bool success;
    if (m_user.isAdmin) {
        // Admin export includes filter settings (custom sources/albums)
        success = Database::get().exportBinaryWithSettings(path);
    } else {
        // User export includes card statuses (favorite/obtained)
        success = Database::get().exportBinaryWithStatuses(path, m_user.id);
    }
    
    if (success) {
        m_lastExport = path;
        QString msg = "Все карты экспортированы";
        if (m_user.isAdmin) {
            msg += "\n(включая настройки фильтров)";
        } else {
            msg += "\n(включая настройки избранного/получено)";
        }
        msg += QString("\n\nФайл: %1").arg(path);
        QMessageBox::information(this, "Экспорт", msg);
    } else {
        QMessageBox::critical(this, "Ошибка", Database::get().lastError());
    }
}

void MainWindow::onImport() {
    if (!m_user.isAdmin) return;
    
    QString path = getHostPath("Импорт", false, "CardCab (*.ccb);;All (*)");
    if (path.isEmpty()) return;
    
    try {
        Database::ImportResult result = Database::get().importBinaryWithSettings(path);
        
        if (result.cards.isEmpty()) {
            QMessageBox::information(this, "Импорт", "Файл не содержит карт");
            return;
        }
        
        if (QMessageBox::question(this, "Импорт",
                QString("Найдено %1 карт.\nПодпись файла проверена.\n\n"
                        "Существующие карты (по названию) будут обновлены,\n"
                        "новые карты будут добавлены.\n\nИмпортировать?").arg(result.cards.size()),
                QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
            
            int importedCards = 0;
            int updatedCards = 0;
            bool success = true;
            
            for (Card c : result.cards) {
                Card existing = Database::get().getCardByName(c.cardName);
                if (existing.id > 0) {
                    // Карта существует - обновляем
                    c.id = existing.id;
                    if (Database::get().updateCard(c)) {
                        updatedCards++;
                    } else {
                        success = false;
                        break;
                    }
                } else {
                    // Новая карта - добавляем
                    c.id = 0;
                    if (Database::get().addCard(c)) {
                        importedCards++;
                    } else {
                        success = false;
                        break;
                    }
                }
            }
            
            if (success) {
                loadCards();
                QMessageBox::information(this, "Импорт", 
                    QString("Импортировано:\n- Добавлено: %1 карт\n- Обновлено: %2 карт")
                        .arg(importedCards).arg(updatedCards));
            } else {
                QMessageBox::critical(this, "Ошибка", Database::get().lastError());
            }
        }
    } catch (const DbException& e) {
        QMessageBox::critical(this, "Ошибка импорта", QString::fromStdString(e.what()));
    }
}

void MainWindow::onUserImport() {
    // Regular user import - can import cards and settings from admin exports
    QString path = getHostPath("Импорт", false, "CardCab (*.ccb);;All (*)");
    if (path.isEmpty()) return;
    
    try {
        Database::ImportResult result = Database::get().importBinaryWithSettings(path);
        
        if (result.cards.isEmpty() && !result.hasSettings) {
            QMessageBox::information(this, "Импорт", "Файл не содержит данных");
            return;
        }
        
        QString msg;
        if (result.isFromAdmin) {
            // Файл от админа
            msg = QString("Файл от администратора.\n\nНайдено:\n- Карт: %1").arg(result.cards.size());
            if (result.hasSettings) {
                msg += QString("\n- Пользовательских источников: %1").arg(result.customSources.size());
                msg += QString("\n- Пользовательских альбомов: %1").arg(result.customAlbums.size());
            }
            msg += "\n\nПодпись файла проверена.\n\nИмпортировать данные?";
            msg += "\n\n(Примечание: существующие карты не будут затронуты,";
            msg += "\nтолько новые карты будут добавлены без меток 'избранное' и 'получено',";
            msg += "\nфильтры администратора будут добавлены)";
        } else {
            // Файл от другого пользователя
            msg = QString("Файл от пользователя.\n\nНайдено:\n- Карт: %1").arg(result.cards.size());
            msg += "\n\nПодпись файла проверена.\n\nИмпортировать данные?";
            msg += "\n\n(Примечание: существующие карты будут обновлены данными из файла,";
            msg += "\nвключая все настройки импортируемых карт)";
        }
        
        if (QMessageBox::question(this, "Импорт", msg,
                QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
            
            bool success = true;
            int importedCards = 0;
            int updatedCards = 0;
            int skippedCards = 0;
            
            if (result.isFromAdmin) {
                // Импорт от админа
                
                // Сначала применяем настройки фильтров (источники/альбомы)
                if (result.hasSettings) {
                    Database::get().applyImportedSettings(result);
                    // Refresh filter combos
                    m_filterSourceCombo->clear();
                    m_filterSourceCombo->addItem("Все", "");
                    for (const QString& s : Database::get().getAllSources()) {
                        m_filterSourceCombo->addItem(s, s);
                    }
                    m_filterAlbumCombo->clear();
                    m_filterAlbumCombo->addItem("Все", "");
                    for (const QString& a : Database::get().getAllAlbums()) {
                        m_filterAlbumCombo->addItem(a, a);
                    }
                }
                
                // Импортируем только новые карты
                for (Card c : result.cards) {
                    if (Database::get().cardExistsByName(c.cardName)) {
                        skippedCards++;
                        continue;
                    }
                    c.id = 0;
                    if (Database::get().addCard(c)) {
                        // Сбрасываем избранное и получено
                        Database::get().setFavorite(c.id, m_user.id, false);
                        Database::get().setObtained(c.id, m_user.id, false);
                        importedCards++;
                    } else {
                        success = false;
                        break;
                    }
                }
            } else {
                // Импорт от пользователя - замена существующих карт с применением статусов
                for (int i = 0; i < result.cards.size(); ++i) {
                    Card c = result.cards[i];
                    UserCardStatus status;
                    if (i < result.cardStatuses.size()) {
                        status = result.cardStatuses[i];
                    }
                    
                    Card existing = Database::get().getCardByName(c.cardName);
                    if (existing.id > 0) {
                        // Карта существует - обновляем данные карты и статусы
                        c.id = existing.id;
                        if (Database::get().updateCard(c)) {
                            // Применяем статусы из импортируемого файла
                            Database::get().setFavorite(c.id, m_user.id, status.isFavorite);
                            Database::get().setObtained(c.id, m_user.id, status.isObtained);
                            updatedCards++;
                        } else {
                            success = false;
                            break;
                        }
                    } else {
                        // Новая карта - добавляем
                        c.id = 0;
                        if (Database::get().addCard(c)) {
                            // Применяем статусы из импортируемого файла
                            Database::get().setFavorite(c.id, m_user.id, status.isFavorite);
                            Database::get().setObtained(c.id, m_user.id, status.isObtained);
                            importedCards++;
                        } else {
                            success = false;
                            break;
                        }
                    }
                }
            }
            
            if (success) {
                loadCards();
                QString successMsg;
                if (result.isFromAdmin) {
                    successMsg = QString("Импорт от администратора завершён:\n- Карт добавлено: %1\n- Карт пропущено (уже есть): %2")
                        .arg(importedCards).arg(skippedCards);
                    if (result.hasSettings) {
                        successMsg += QString("\n- Источников добавлено: %1").arg(result.customSources.size());
                        successMsg += QString("\n- Альбомов добавлено: %1").arg(result.customAlbums.size());
                    }
                } else {
                    successMsg = QString("Импорт от пользователя завершён:\n- Карт добавлено: %1\n- Карт обновлено: %2\n\n(Настройки избранного/получено применены)")
                        .arg(importedCards).arg(updatedCards);
                }
                QMessageBox::information(this, "Импорт", successMsg);
            } else {
                QMessageBox::critical(this, "Ошибка", Database::get().lastError());
            }
        }
    } catch (const DbException& e) {
        QMessageBox::critical(this, "Ошибка импорта", QString::fromStdString(e.what()));
    }
}

void MainWindow::onViewBinaryFile() {
    QString path = getHostPath("Выбрать файл", false, "CardCab (*.ccb);;All (*)");
    if (path.isEmpty()) return;
    
    try {
        // Use importBinaryWithSettings which supports v2, v3 and v4 formats
        Database::ImportResult result = Database::get().importBinaryWithSettings(path);
        
        QString info = QString("Файл: %1\nКарт: %2\nПодпись: OK\n").arg(path).arg(result.cards.size());
        
        // Информация об источнике файла
        if (result.isFromAdmin) {
            info += "Источник: Администратор\n";
        } else {
            info += "Источник: Пользователь\n";
        }
        info += "\n";
        
        if (result.hasSettings) {
            info += QString("Пользовательских источников: %1\n").arg(result.customSources.size());
            info += QString("Пользовательских альбомов: %1\n\n").arg(result.customAlbums.size());
            
            if (!result.customSources.isEmpty()) {
                info += "Источники: " + result.customSources.join(", ") + "\n";
            }
            if (!result.customAlbums.isEmpty()) {
                info += "Альбомы: " + result.customAlbums.join(", ") + "\n";
            }
            info += "\n";
        }
        
        for (int i = 0; i < result.cards.size(); ++i) {
            const Card& c = result.cards[i];
            info += QString("--- Карта #%1 ---\n").arg(i + 1);
            info += QString("Название: %1\n").arg(c.cardName);
            info += QString("Участник: %1\n").arg(c.participantName.isEmpty() ? "-" : c.participantName);
            info += QString("Источник: %1\n").arg(c.source.isEmpty() ? "-" : c.source);
            info += QString("Альбом: %1\n").arg(c.albumName.isEmpty() ? "-" : c.albumName);
            info += QString("Изображение: %1\n\n").arg(c.imageData.isEmpty() ? "нет" : QString("%1 байт").arg(c.imageData.size()));
        }
        
        QString summary = QString("Файл содержит %1 карт\nПодпись файла проверена").arg(result.cards.size());
        summary += QString("\nИсточник: %1").arg(result.isFromAdmin ? "Администратор" : "Пользователь");
        if (result.hasSettings) {
            summary += QString("\n\nДополнительно:\n- Источников: %1\n- Альбомов: %2")
                .arg(result.customSources.size())
                .arg(result.customAlbums.size());
        }
        
        QMessageBox msg(this);
        msg.setWindowTitle("Содержимое файла");
        msg.setText(summary);
        msg.setDetailedText(info);
        msg.exec();
    } catch (const DbException& e) {
        QMessageBox::critical(this, "Ошибка", QString::fromStdString(e.what()));
    }
}

void MainWindow::onChangeAdminCreds() {
    if (!m_user.isAdmin) return;
    
    bool ok;
    QString newUser = QInputDialog::getText(this, "Новый логин", "Введите новый логин:",
        QLineEdit::Normal, m_user.username, &ok);
    if (!ok || newUser.trimmed().isEmpty()) return;
    
    QString newPass = QInputDialog::getText(this, "Новый пароль", "Введите новый пароль:",
        QLineEdit::Password, QString(), &ok);
    if (!ok || newPass.isEmpty()) return;
    
    QString confirm = QInputDialog::getText(this, "Подтверждение", "Подтвердите пароль:",
        QLineEdit::Password, QString(), &ok);
    if (!ok || newPass != confirm) {
        QMessageBox::warning(this, "Ошибка", "Пароли не совпадают");
        return;
    }
    
    if (Database::get().changeAdminCreds(m_user.id, newUser.trimmed(), newPass)) {
        m_user.username = newUser.trimmed();
        updateStatus();
        QMessageBox::information(this, "Успех", "Данные обновлены");
    } else {
        QMessageBox::critical(this, "Ошибка", Database::get().lastError());
    }
}

void MainWindow::onAddCustomSource() {
    if (!m_user.isAdmin) return;
    
    bool ok;
    QString name = QInputDialog::getText(this, "Новый источник", 
        "Введите название нового источника:", QLineEdit::Normal, QString(), &ok);
    
    if (ok && !name.trimmed().isEmpty()) {
        if (Database::get().addCustomSource(name.trimmed())) {
            // Update filter combo
            m_filterSourceCombo->clear();
            m_filterSourceCombo->addItem("Все", "");
            for (const QString& s : Database::get().getAllSources()) {
                m_filterSourceCombo->addItem(s, s);
            }
            QMessageBox::information(this, "Успех", "Источник добавлен: " + name.trimmed());
        } else {
            QMessageBox::warning(this, "Ошибка", "Не удалось добавить источник");
        }
    }
}

void MainWindow::onAddCustomAlbum() {
    if (!m_user.isAdmin) return;
    
    bool ok;
    QString name = QInputDialog::getText(this, "Новый альбом", 
        "Введите название нового альбома:", QLineEdit::Normal, QString(), &ok);
    
    if (ok && !name.trimmed().isEmpty()) {
        if (Database::get().addCustomAlbum(name.trimmed())) {
            // Update filter combo
            m_filterAlbumCombo->clear();
            m_filterAlbumCombo->addItem("Все", "");
            for (const QString& a : Database::get().getAllAlbums()) {
                m_filterAlbumCombo->addItem(a, a);
            }
            QMessageBox::information(this, "Успех", "Альбом добавлен: " + name.trimmed());
        } else {
            QMessageBox::warning(this, "Ошибка", "Не удалось добавить альбом");
        }
    }
}

void MainWindow::onDeleteSources() {
    if (!m_user.isAdmin) return;
    
    QStringList custom = Database::get().getCustomSources();
    if (custom.isEmpty()) {
        QMessageBox::information(this, "Удаление источников", "Пользовательских источников нет.\nИспользуйте 'Добавить источник' для создания.");
        return;
    }
    
    bool ok;
    QString selected = QInputDialog::getItem(this, "Удаление источника",
        "Выберите источник для удаления:", custom, 0, false, &ok);
    
    if (ok && !selected.isEmpty()) {
        if (QMessageBox::question(this, "Подтверждение",
                QString("Удалить источник '%1'?").arg(selected),
                QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
            if (Database::get().removeCustomSource(selected)) {
                m_filterSourceCombo->clear();
                m_filterSourceCombo->addItem("Все", "");
                for (const QString& s : Database::get().getAllSources()) {
                    m_filterSourceCombo->addItem(s, s);
                }
                QMessageBox::information(this, "Успех", "Источник удалён");
            }
        }
    }
}

void MainWindow::onDeleteAlbums() {
    if (!m_user.isAdmin) return;
    
    QStringList custom = Database::get().getCustomAlbums();
    if (custom.isEmpty()) {
        QMessageBox::information(this, "Удаление альбомов", "Пользовательских альбомов нет.\nИспользуйте 'Добавить альбом' для создания.");
        return;
    }
    
    bool ok;
    QString selected = QInputDialog::getItem(this, "Удаление альбома",
        "Выберите альбом для удаления:", custom, 0, false, &ok);
    
    if (ok && !selected.isEmpty()) {
        if (QMessageBox::question(this, "Подтверждение",
                QString("Удалить альбом '%1'?").arg(selected),
                QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
            if (Database::get().removeCustomAlbum(selected)) {
                m_filterAlbumCombo->clear();
                m_filterAlbumCombo->addItem("Все", "");
                for (const QString& a : Database::get().getAllAlbums()) {
                    m_filterAlbumCombo->addItem(a, a);
                }
                QMessageBox::information(this, "Успех", "Альбом удалён");
            }
        }
    }
}

void MainWindow::onLogout() {
    if (QMessageBox::question(this, "Выход из аккаунта", "Выйти из аккаунта?\nВы вернётесь на экран входа.",
            QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
        m_logoutRequested = true;
        close();
    }
}

void MainWindow::onRefresh() {
    loadCards();
}

void MainWindow::onAbout() {
    QMessageBox::about(this, "О программе " + App::APP_NAME,
        QString("<h2>%1 v%2</h2>"
                "<p><b>Менеджер коллекционных карт</b></p>"
                "<hr>"
                "<p><b>Возможности:</b></p>"
                "<ul>"
                "<li>Управление каталогом карт (админ)</li>"
                "<li>Фильтрация по участнику, источнику, альбому</li>"
                "<li>Поиск по названию</li>"
                "<li>Отметки: избранное, получено</li>"
                "<li>Экспорт/импорт с подписью</li>"
                "</ul>"
                "<p><b>Технологии:</b> C++17, Qt5, PostgreSQL</p>"
                "<hr>"
                "<p>НГТУ, кафедра Защиты информации, 2025</p>")
            .arg(App::APP_NAME, App::APP_VERSION));
}
