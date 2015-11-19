#ifndef OXTWITTER_H
#define OXTWITTER_H

#include "o1twitter.h"

class OXTwitter: public O1Twitter {
    Q_OBJECT

public:
    explicit OXTwitter(QObject *parent = 0);
    /// Twitter XAuth login parameters
    /// XAuth Username
    Q_PROPERTY(QString username READ username WRITE setUsername NOTIFY usernameChanged)
    QString username();
    void setUsername(const QString &username);

    /// XAuth Password
    Q_PROPERTY(QString password READ password WRITE setPassword NOTIFY passwordChanged)
    QString password();
    void setPassword(const QString &username);

public slots:
    /// Authenticate.
    Q_INVOKABLE virtual void link();

signals:
    void usernameChanged();
    void passwordChanged();

private:
    QList<O1RequestParameter> xAuthParams_;
    QString username_;
    QString password_;
};

#endif // OXTWITTER_H
