#ifndef O2REPLYSERVER_H
#define O2REPLYSERVER_H

#include <QTcpServer>
#include <QMap>
#include <QByteArray>
#include <QString>

/// HTTP server to process authentication response.
class O2ReplyServer: public QTcpServer {
    Q_OBJECT

public:
    explicit O2ReplyServer(QObject *parent = 0);
    ~O2ReplyServer();

signals:
    void verificationReceived(QMap<QString, QString>);

public slots:
    void onIncomingConnection();
    void onBytesReady();
    QMap<QString, QString> parseQueryParams(QByteArray *data);
};

#endif // O2REPLYSERVER_H
