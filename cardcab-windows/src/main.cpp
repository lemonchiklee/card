#include <QApplication>
#include <QMessageBox>
#include <QThread>
#include <QDebug>
#include <QSettings>

// Platform-specific signal handling
#ifdef _WIN32
    #include <windows.h>
#else
    #include <csignal>
#endif

#include "constants.h"
#include "database.h"
#include "logindialog.h"
#include "mainwindow.h"

// Global flag for clean shutdown
static bool g_shuttingDown = false;

#ifdef _WIN32
// Windows console control handler
BOOL WINAPI ConsoleHandler(DWORD signal) {
    if (signal == CTRL_C_EVENT || signal == CTRL_CLOSE_EVENT || 
        signal == CTRL_BREAK_EVENT || signal == CTRL_LOGOFF_EVENT || 
        signal == CTRL_SHUTDOWN_EVENT) {
        if (!g_shuttingDown) {
            g_shuttingDown = true;
            Database::get().close();
            QCoreApplication::quit();
        }
        return TRUE;
    }
    return FALSE;
}
#else
// POSIX signal handler
void signalHandler(int) {
    if (!g_shuttingDown) {
        g_shuttingDown = true;
        Database::get().close();
        QCoreApplication::quit();
    }
}
#endif

bool waitForPostgres(const DbConfig& cfg, int maxTries = 30) {
    qInfo() << "Waiting for PostgreSQL...";
    
    for (int i = 1; i <= maxTries; ++i) {
        if (Database::get().connect(cfg)) {
            qInfo() << "PostgreSQL ready!";
            return true;
        }
        qInfo() << "Attempt" << i << "/" << maxTries;
        QThread::sleep(1);
        Database::get().close();
    }
    return false;
}

int main(int argc, char *argv[]) {
    // Setup platform-specific signal handlers
#ifdef _WIN32
    SetConsoleCtrlHandler(ConsoleHandler, TRUE);
#else
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
#endif
    
    QApplication app(argc, argv);
    
    app.setApplicationName(App::APP_NAME);
    app.setApplicationVersion(App::APP_VERSION);
    app.setOrganizationName(App::SETTINGS_ORG);
    
    qInfo() << "====================================";
    qInfo() << " " << App::APP_NAME << "v" << App::APP_VERSION;
    qInfo() << "  Card Collection Manager";
    qInfo() << "====================================";
    
    DbConfig cfg;
    cfg.host = qEnvironmentVariable("DB_HOST", App::DB_HOST);
    
    bool portOk = false;
    int envPort = qEnvironmentVariableIntValue("DB_PORT", &portOk);
    cfg.port = portOk ? envPort : App::DB_PORT;
    
    cfg.database = qEnvironmentVariable("DB_NAME", App::DB_NAME);
    cfg.user = qEnvironmentVariable("DB_USER", App::DB_USER);
    cfg.password = qEnvironmentVariable("DB_PASSWORD", App::DB_PASSWORD);
    
    qInfo() << "PostgreSQL:" << cfg.host << ":" << cfg.port << "/" << cfg.database;
    
    if (!waitForPostgres(cfg)) {
        QMessageBox::critical(nullptr, "Error",
            QString("Cannot connect to PostgreSQL.\n\nHost: %1:%2\nDB: %3\n\n%4")
                .arg(cfg.host).arg(cfg.port).arg(cfg.database)
                .arg(Database::get().lastError()));
        return 1;
    }
    
    qInfo() << "Connected to database!";
    
    int result = 0;
    
    // Main application loop - handles logout and re-login
    // Exit codes:
    //   0 = normal exit (close everything)
    //   42 = logout (show login dialog again)
    while (!g_shuttingDown) {
        LoginDialog login;
        
        if (login.exec() != QDialog::Accepted) {
            // User cancelled login - exit
            qInfo() << "Login cancelled, exiting";
            break;
        }
        
        User user = login.getUser();
        if (!user.isValid()) {
            QMessageBox::critical(nullptr, "Error", "Authentication error");
            continue;
        }
        
        qInfo() << "Login:" << user.username << "(Admin:" << user.isAdmin << ")";
        
        {
            MainWindow mainWin(user);
            mainWin.show();
            
            result = app.exec();
            
            qInfo() << "MainWindow closed with code:" << result;
        } // MainWindow destroyed here
        
        // exitCode 42 = logout requested, show login again
        // other codes = exit application
        if (result != 42) {
            break;
        }
        
        qInfo() << "Logout, showing login dialog again";
    }
    
    // Close database before exit
    g_shuttingDown = true;
    Database::get().close();
    qInfo() << "Application finished";
    
    return 0;
}
