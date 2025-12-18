#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <QString>
#include <QStringList>
#include <QStandardPaths>
#include <QDir>

namespace App {

const QString APP_NAME = "CardCab";
const QString APP_VERSION = "1.0.0";

// PostgreSQL
const QString DB_HOST = "localhost";
const int DB_PORT = 5436;
const QString DB_NAME = "cardcab_db";
const QString DB_USER = "cardcab_user";
const QString DB_PASSWORD = "cardcab_secure_password_2025";

// Cross-platform path to user's home directory
// On Windows: C:/Users/<username>
// On Linux: /home/<username>
// On macOS: /Users/<username>
inline QString getDefaultPath() {
    QString home = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    if (home.isEmpty()) {
        home = QDir::homePath();
    }
    return home;
}

// For backward compatibility - now returns cross-platform path
#define HOST_HOME (App::getDefaultPath())

// Бинарный формат с подписью
const quint32 BINARY_MAGIC = 0xCAC0CAFE;
const quint32 BINARY_VERSION = 2;  // Версия 2 - с подписью (устаревшая)
const quint32 BINARY_VERSION_SETTINGS = 3;  // Версия 3 - с подписью и настройками фильтров (от админа)
const quint32 BINARY_VERSION_USER = 4;  // Версия 4 - от пользователя (без настроек фильтров)
const QString SIGNATURE_KEY = "CardCab_SecretKey_2025_NSTU";

// Настройки "запомнить меня"
const QString SETTINGS_ORG = "NSTU";
const QString SETTINGS_APP = "CardCab";
const QString SETTINGS_USER = "saved_username";
const QString SETTINGS_PASS = "saved_password";
const QString SETTINGS_REMEMBER = "remember_me";

const QStringList PARTICIPANTS = {
    "Seonghwa", "Hongjoong", "Yunho", "San", "Mingi", "Wooyoung", "Jongho"
};

const QStringList SOURCES = {
    "POBs", "Album Photocards", "JPN POBs", "JPN Album Photocards",
    "Broadcast Photocards", "Winner", "Merch Photocards"
};

const QStringList ALBUMS = {
    "TREASURE EP.1 ALL TO ZERO", "TREASURE EP.2 Zero to One",
    "TREASURE EP.3 ONE TO ALL", "TREASURE EP.FIN All To Action",
    "TREASURE EPILOGUE Action To Answer", "ZERO FEVER Part.1",
    "ZERO FEVER Part.2", "ZERO FEVER Part.3", "Zero Fever Epilogue",
    "WORLD EP.1 MOVEMENT", "Spin Off From The Witness",
    "The World EP.2 Outlaw", "The World EP.Fin Will",
    "GOLDEN HOUR Part.1", "GOLDEN HOUR Part.2", "GOLDEN HOUR Part.3",
    "GOLDEN HOUR Part.3 IN YOUR FANTASY EDITION"
};

}

#endif
