#ifndef USER_H
#define USER_H

#include <QString>
#include <QDateTime>

struct User {
    qint64 id = 0;
    QString username;
    QString passwordHash;
    bool isAdmin = false;
    QDateTime createdAt;
    
    bool isValid() const { return id > 0 && !username.isEmpty(); }
};

#endif // USER_H
