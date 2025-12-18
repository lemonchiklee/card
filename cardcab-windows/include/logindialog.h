#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QTabWidget>
#include <QCheckBox>
#include <QEvent>
#include "user.h"

class LoginDialog : public QDialog {
    Q_OBJECT
public:
    explicit LoginDialog(QWidget* parent = nullptr);
    User getUser() const { return m_user; }

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    void onLogin();
    void onRegister();

private:
    void setupUi();
    void applyStyle();
    void loadSavedCredentials();
    void saveCredentials();
    void clearSavedCredentials();
    
    QTabWidget* m_tabs;
    QLineEdit* m_loginUser;
    QLineEdit* m_loginPass;
    QCheckBox* m_rememberCheck;
    QLineEdit* m_regUser;
    QLineEdit* m_regPass;
    QLineEdit* m_regConfirm;
    QPushButton* m_loginBtn;
    QPushButton* m_regBtn;
    User m_user;
};

#endif
