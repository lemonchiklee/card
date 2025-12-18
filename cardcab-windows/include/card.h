#ifndef CARD_H
#define CARD_H

#include <QString>
#include <QByteArray>
#include <QDataStream>
#include <QDateTime>

struct Card {
    qint64 id = 0;
    QString cardName;
    QString participantName;
    QString source;
    QString albumName;
    QByteArray imageData;
    QDateTime createdAt;
    QDateTime updatedAt;
    
    bool isValid() const { return id > 0 && !cardName.trimmed().isEmpty(); }
    
    friend QDataStream& operator<<(QDataStream& out, const Card& c) {
        out << c.id << c.cardName << c.participantName << c.source 
            << c.albumName << c.imageData << c.createdAt << c.updatedAt;
        return out;
    }
    
    friend QDataStream& operator>>(QDataStream& in, Card& c) {
        in >> c.id >> c.cardName >> c.participantName >> c.source 
           >> c.albumName >> c.imageData >> c.createdAt >> c.updatedAt;
        return in;
    }
};

struct UserCardStatus {
    qint64 cardId = 0;
    qint64 userId = 0;
    bool isFavorite = false;
    bool isObtained = false;
    
    friend QDataStream& operator<<(QDataStream& out, const UserCardStatus& s) {
        out << s.isFavorite << s.isObtained;
        return out;
    }
    
    friend QDataStream& operator>>(QDataStream& in, UserCardStatus& s) {
        in >> s.isFavorite >> s.isObtained;
        return in;
    }
};

#endif
