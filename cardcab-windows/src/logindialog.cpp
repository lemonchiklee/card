#include "logindialog.h"
#include "database.h"
#include "constants.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QMessageBox>
#include <QSettings>
#include <QKeyEvent>

LoginDialog::LoginDialog(QWidget* parent) : QDialog(parent) {
    setupUi();
    applyStyle();
    loadSavedCredentials();
    setWindowTitle(App::APP_NAME + " - Вход");
    setMinimumSize(400, 380);
}

void LoginDialog::setupUi() {
    QVBoxLayout* main = new QVBoxLayout(this);
    main->setSpacing(15);
    main->setContentsMargins(20, 20, 20, 20);
    
    QLabel* title = new QLabel(App::APP_NAME);
    title->setObjectName("title");
    title->setAlignment(Qt::AlignCenter);
    main->addWidget(title);
    
    QLabel* subtitle = new QLabel("Менеджер коллекционных карт v" + App::APP_VERSION);
    subtitle->setObjectName("subtitle");
    subtitle->setAlignment(Qt::AlignCenter);
    main->addWidget(subtitle);
    
    m_tabs = new QTabWidget();
    
    // Login tab
    QWidget* loginTab = new QWidget();
    QVBoxLayout* loginLayout = new QVBoxLayout(loginTab);
    loginLayout->setContentsMargins(15, 15, 15, 15);
    
    QFormLayout* loginForm = new QFormLayout();
    m_loginUser = new QLineEdit();
    m_loginUser->setPlaceholderText("Имя пользователя");
    m_loginUser->setMinimumHeight(35);
    loginForm->addRow("Логин:", m_loginUser);
    
    m_loginPass = new QLineEdit();
    m_loginPass->setEchoMode(QLineEdit::Password);
    m_loginPass->setPlaceholderText("Пароль");
    m_loginPass->setMinimumHeight(35);
    loginForm->addRow("Пароль:", m_loginPass);
    
    loginLayout->addLayout(loginForm);
    
    m_rememberCheck = new QCheckBox("Запомнить меня");
    loginLayout->addWidget(m_rememberCheck);
    loginLayout->addSpacing(10);
    
    m_loginBtn = new QPushButton("Войти");
    m_loginBtn->setObjectName("primary");
    m_loginBtn->setMinimumHeight(40);
    loginLayout->addWidget(m_loginBtn);
    loginLayout->addStretch();
    
    // Register tab
    QWidget* regTab = new QWidget();
    QVBoxLayout* regLayout = new QVBoxLayout(regTab);
    regLayout->setContentsMargins(15, 15, 15, 15);
    
    QFormLayout* regForm = new QFormLayout();
    m_regUser = new QLineEdit();
    m_regUser->setPlaceholderText("Мин. 3 символа");
    m_regUser->setMinimumHeight(35);
    regForm->addRow("Логин:", m_regUser);
    
    m_regPass = new QLineEdit();
    m_regPass->setEchoMode(QLineEdit::Password);
    m_regPass->setPlaceholderText("Мин. 4 символа");
    m_regPass->setMinimumHeight(35);
    regForm->addRow("Пароль:", m_regPass);
    
    m_regConfirm = new QLineEdit();
    m_regConfirm->setEchoMode(QLineEdit::Password);
    m_regConfirm->setPlaceholderText("Повторите пароль");
    m_regConfirm->setMinimumHeight(35);
    regForm->addRow("Подтверждение:", m_regConfirm);
    
    regLayout->addLayout(regForm);
    regLayout->addSpacing(10);
    
    m_regBtn = new QPushButton("Зарегистрироваться");
    m_regBtn->setObjectName("primary");
    m_regBtn->setMinimumHeight(40);
    regLayout->addWidget(m_regBtn);
    
    QLabel* note = new QLabel("Регистрация только для обычных пользователей.");
    note->setObjectName("note");
    note->setWordWrap(true);
    regLayout->addWidget(note);
    regLayout->addStretch();
    
    m_tabs->addTab(loginTab, "Вход");
    m_tabs->addTab(regTab, "Регистрация");
    main->addWidget(m_tabs);
    
    connect(m_loginBtn, &QPushButton::clicked, this, &LoginDialog::onLogin);
    connect(m_regBtn, &QPushButton::clicked, this, &LoginDialog::onRegister);
    connect(m_loginPass, &QLineEdit::returnPressed, this, &LoginDialog::onLogin);
    connect(m_regConfirm, &QLineEdit::returnPressed, this, &LoginDialog::onRegister);
    
    // Set tab order
    setTabOrder(m_loginUser, m_loginPass);
    setTabOrder(m_loginPass, m_rememberCheck);
    setTabOrder(m_rememberCheck, m_loginBtn);
    setTabOrder(m_regUser, m_regPass);
    setTabOrder(m_regPass, m_regConfirm);
    setTabOrder(m_regConfirm, m_regBtn);
    
    // Install event filter for arrow key navigation
    m_loginUser->installEventFilter(this);
    m_loginPass->installEventFilter(this);
    m_regUser->installEventFilter(this);
    m_regPass->installEventFilter(this);
    m_regConfirm->installEventFilter(this);
}

void LoginDialog::applyStyle() {
    setStyleSheet(R"(
        QDialog { background-color: #1a1b2e; }
        QLabel { color: #e0e0e0; font-size: 13px; }
        QLabel#title { font-size: 28px; font-weight: bold; color: #ff4757; }
        QLabel#subtitle { font-size: 12px; color: #7f8c8d; }
        QLabel#note { font-size: 11px; color: #7f8c8d; font-style: italic; padding: 10px; background: #2d2d44; border-radius: 5px; }
        QLineEdit { padding: 10px; border: 2px solid #3d3d5c; border-radius: 6px; background: #2d2d44; color: #e0e0e0; }
        QLineEdit:focus { border-color: #ff4757; }
        QPushButton#primary { background: #ff4757; color: white; border: none; padding: 12px; border-radius: 6px; font-size: 14px; font-weight: bold; }
        QPushButton#primary:hover { background: #ff6b81; }
        QCheckBox { color: #e0e0e0; spacing: 8px; }
        QCheckBox::indicator { width: 18px; height: 18px; border-radius: 4px; border: 2px solid #3d3d5c; background: #2d2d44; }
        QCheckBox::indicator:checked { background: #ff4757; border-color: #ff4757; }
        QTabWidget::pane { border: 2px solid #3d3d5c; border-radius: 6px; background: #2d2d44; }
        QTabBar::tab { background: #1a1b2e; color: #7f8c8d; padding: 10px 25px; border: 2px solid #3d3d5c; border-bottom: none; border-top-left-radius: 6px; border-top-right-radius: 6px; }
        QTabBar::tab:selected { background: #2d2d44; color: #ff4757; font-weight: bold; }
    )");
}

void LoginDialog::loadSavedCredentials() {
    QSettings settings(App::SETTINGS_ORG, App::SETTINGS_APP);
    
    if (settings.value(App::SETTINGS_REMEMBER, false).toBool()) {
        QString username = settings.value(App::SETTINGS_USER).toString();
        QString pass = settings.value(App::SETTINGS_PASS).toString();
        
        if (!username.isEmpty()) {
            m_loginUser->setText(username);
            m_loginPass->setText(pass);
            m_rememberCheck->setChecked(true);
            // Focus on login button so user can just press Enter to confirm
            // but they can also change credentials if needed
            m_loginBtn->setFocus();
        }
    }
}

void LoginDialog::saveCredentials() {
    QSettings settings(App::SETTINGS_ORG, App::SETTINGS_APP);
    
    if (m_rememberCheck->isChecked()) {
        settings.setValue(App::SETTINGS_REMEMBER, true);
        settings.setValue(App::SETTINGS_USER, m_loginUser->text().trimmed());
        settings.setValue(App::SETTINGS_PASS, m_loginPass->text());
    } else {
        clearSavedCredentials();
    }
}

void LoginDialog::clearSavedCredentials() {
    QSettings settings(App::SETTINGS_ORG, App::SETTINGS_APP);
    settings.setValue(App::SETTINGS_REMEMBER, false);
    settings.remove(App::SETTINGS_USER);
    settings.remove(App::SETTINGS_PASS);
}

void LoginDialog::onLogin() {
    QString user = m_loginUser->text().trimmed();
    QString pass = m_loginPass->text();
    
    if (user.isEmpty() || pass.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Заполните все поля");
        return;
    }
    
    if (Database::get().authenticate(user, pass, m_user)) {
        saveCredentials();
        accept();
    } else {
        QMessageBox::warning(this, "Ошибка", "Неверный логин или пароль");
        m_loginPass->clear();
        clearSavedCredentials();
    }
}

void LoginDialog::onRegister() {
    QString user = m_regUser->text().trimmed();
    QString pass = m_regPass->text();
    QString confirm = m_regConfirm->text();
    
    if (user.length() < 3) {
        QMessageBox::warning(this, "Ошибка", "Логин минимум 3 символа");
        return;
    }
    if (pass.length() < 4) {
        QMessageBox::warning(this, "Ошибка", "Пароль минимум 4 символа");
        return;
    }
    if (pass != confirm) {
        QMessageBox::warning(this, "Ошибка", "Пароли не совпадают");
        return;
    }
    
    if (Database::get().registerUser(user, pass)) {
        QMessageBox::information(this, "Успех", "Регистрация успешна! Теперь войдите.");
        m_tabs->setCurrentIndex(0);
        m_loginUser->setText(user);
        m_loginPass->setFocus();
        m_regUser->clear();
        m_regPass->clear();
        m_regConfirm->clear();
    } else {
        QMessageBox::warning(this, "Ошибка", "Пользователь уже существует");
    }
}

bool LoginDialog::eventFilter(QObject* obj, QEvent* event) {
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        
        if (keyEvent->key() == Qt::Key_Down || keyEvent->key() == Qt::Key_Up) {
            QWidget* current = qobject_cast<QWidget*>(obj);
            if (current) {
                QWidget* next = nullptr;
                
                // Login tab navigation
                if (current == m_loginUser && keyEvent->key() == Qt::Key_Down) {
                    next = m_loginPass;
                } else if (current == m_loginPass && keyEvent->key() == Qt::Key_Up) {
                    next = m_loginUser;
                }
                // Register tab navigation
                else if (current == m_regUser && keyEvent->key() == Qt::Key_Down) {
                    next = m_regPass;
                } else if (current == m_regPass && keyEvent->key() == Qt::Key_Up) {
                    next = m_regUser;
                } else if (current == m_regPass && keyEvent->key() == Qt::Key_Down) {
                    next = m_regConfirm;
                } else if (current == m_regConfirm && keyEvent->key() == Qt::Key_Up) {
                    next = m_regPass;
                }
                
                if (next) {
                    next->setFocus();
                    return true;
                }
            }
        }
    }
    return QDialog::eventFilter(obj, event);
}
