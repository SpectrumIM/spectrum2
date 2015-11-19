#include <QList>
#include <QPair>
#include <QDebug>
#include <QTcpServer>
#include <QMap>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QScriptEngine>
#include <QScriptValueIterator>
#include <QDateTime>
#include <QCryptographicHash>
#include <QTimer>
#if QT_VERSION >= 0x050000
#include <QUrlQuery>
#endif

#include "o2.h"
#include "o2replyserver.h"
#include "o2globals.h"
#include "o2settingsstore.h"

#define trace() if (1) qDebug()
// define trace() if (0) qDebug()

O2::O2(QObject *parent): QObject(parent) {
    manager_ = new QNetworkAccessManager(this);
    replyServer_ = new O2ReplyServer(this);
    grantFlow_ = GrantFlowAuthorizationCode;
    localPort_ = 0;
    localhostPolicy_ = QString(O2_CALLBACK_URL);
    qRegisterMetaType<QNetworkReply::NetworkError>("QNetworkReply::NetworkError");
    connect(replyServer_, SIGNAL(verificationReceived(QMap<QString,QString>)),
            this, SLOT(onVerificationReceived(QMap<QString,QString>)));
    store_ = new O2SettingsStore(O2_ENCRYPTION_KEY, this);
}

O2::~O2() {
}

void O2::setStore(O2AbstractStore *store) {
    if (!store) {
        qWarning() << "Store object is null! Using default O2SettingsStore";
        return;
    }
    // Delete the previously stored object
    store_->deleteLater();
    store_ = store;
    // re-parent it to this class as we take ownership of it now
    store_->setParent(this);
}

O2::GrantFlow O2::grantFlow() {
    return grantFlow_;
}

void O2::setGrantFlow(O2::GrantFlow value) {
    grantFlow_ = value;
    emit grantFlowChanged();
}

QString O2::clientId() {
    return clientId_;
}

void O2::setClientId(const QString &value) {
    clientId_ = value;
    emit clientIdChanged();
}

QString O2::clientSecret() {
    return clientSecret_;
}

void O2::setClientSecret(const QString &value) {
    clientSecret_ = value;
    emit clientSecretChanged();
}

QString O2::scope() {
    return scope_;
}

void O2::setScope(const QString &value) {
    scope_ = value;
    emit scopeChanged();
}

QString O2::requestUrl() {
    return requestUrl_.toString();
}

void O2::setRequestUrl(const QString &value) {
    requestUrl_ = value;
    emit requestUrlChanged();
}

QString O2::tokenUrl() {
    return tokenUrl_.toString();
}

void O2::setTokenUrl(const QString &value) {
    tokenUrl_= value;
    emit tokenUrlChanged();
}

QString O2::refreshTokenUrl() {
    return refreshTokenUrl_.toString();
}

void O2::setRefreshTokenUrl(const QString &value) {
    refreshTokenUrl_ = value;
    emit refreshTokenUrlChanged();
}

int O2::localPort() {
    return localPort_;
}

void O2::setLocalPort(int value) {
    localPort_ = value;
    emit localPortChanged();
}

QVariantMap O2::extraTokens() const {
    return extraTokens_;
}

void O2::setExtraTokens(QVariantMap extraTokens) {
    extraTokens_ = extraTokens;
}

void O2::link() {
    trace() << "O2::link";
    if (linked()) {
        trace() << " Linked already";
        emit linkingSucceeded();
        return;
    }

    // Start listening to authentication replies
    replyServer_->listen(QHostAddress::Any, localPort_);

    // Save redirect URI, as we have to reuse it when requesting the access token
    redirectUri_ = localhostPolicy_.arg(replyServer_->serverPort());

    // Assemble intial authentication URL
    QList<QPair<QString, QString> > parameters;
    parameters.append(qMakePair(QString(O2_OAUTH2_RESPONSE_TYPE), (grantFlow_ == GrantFlowAuthorizationCode) ? QString(O2_OAUTH2_CODE) : QString(O2_OAUTH2_TOKEN)));
    parameters.append(qMakePair(QString(O2_OAUTH2_CLIENT_ID), clientId_));
    parameters.append(qMakePair(QString(O2_OAUTH2_REDIRECT_URI), redirectUri_));
    // parameters.append(qMakePair(QString(OAUTH2_REDIRECT_URI), QString(QUrl::toPercentEncoding(redirectUri_))));
    parameters.append(qMakePair(QString(O2_OAUTH2_SCOPE), scope_));

    // Show authentication URL with a web browser
    QUrl url(requestUrl_);
#if QT_VERSION < 0x050000
    url.setQueryItems(parameters);
#else
    QUrlQuery query(url);
    query.setQueryItems(parameters);
    url.setQuery(query);
#endif

    trace() << "Emit openBrowser" << url.toString();
    emit openBrowser(url);
}

void O2::unlink() {
    if (!linked()) {
        return;
    }
    setToken(QString());
    setRefreshToken(QString());
    setExpires(0);
    emit linkedChanged();
    emit linkingSucceeded();
}

bool O2::linked() {
    return token().length();
}

void O2::onVerificationReceived(const QMap<QString, QString> response) {
    trace() << "O2::onVerificationReceived";
    trace() << "" << response;

    emit closeBrowser();
    if (response.contains("error")) {
        trace() << " Verification failed";
        emit linkingFailed();
        return;
    }

    if (grantFlow_ == GrantFlowAuthorizationCode) {
        // Save access code
        setCode(response.value(QString(O2_OAUTH2_CODE)));

        // Exchange access code for access/refresh tokens
        QNetworkRequest tokenRequest(tokenUrl_);
        tokenRequest.setHeader(QNetworkRequest::ContentTypeHeader, O2_MIME_TYPE_XFORM);
        QMap<QString, QString> parameters;
        parameters.insert(O2_OAUTH2_CODE, code());
        parameters.insert(O2_OAUTH2_CLIENT_ID, clientId_);
        parameters.insert(O2_OAUTH2_CLIENT_SECRET, clientSecret_);
        parameters.insert(O2_OAUTH2_REDIRECT_URI, redirectUri_);
        parameters.insert(O2_OAUTH2_GRANT_TYPE, O2_AUTHORIZATION_CODE);
        QByteArray data = buildRequestBody(parameters);
        QNetworkReply *tokenReply = manager_->post(tokenRequest, data);
        timedReplies_.add(tokenReply);
        connect(tokenReply, SIGNAL(finished()), this, SLOT(onTokenReplyFinished()), Qt::QueuedConnection);
        connect(tokenReply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(onTokenReplyError(QNetworkReply::NetworkError)), Qt::QueuedConnection);
    } else {
        setToken(response.value(O2_OAUTH2_ACCESS_TOKEN));
        setRefreshToken(response.value(O2_OAUTH2_REFRESH_TOKEN));
    }
}

QString O2::code() {
    QString key = QString(O2_KEY_CODE).arg(clientId_);
    return store_->value(key);
}

void O2::setCode(const QString &c) {
    QString key = QString(O2_KEY_CODE).arg(clientId_);
    store_->setValue(key, c);
}

void O2::onTokenReplyFinished() {
    trace() << "O2::onTokenReplyFinished";
    QNetworkReply *tokenReply = qobject_cast<QNetworkReply *>(sender());
    if (tokenReply->error() == QNetworkReply::NoError) {
        QByteArray replyData = tokenReply->readAll();
        QScriptEngine engine;
        QScriptValueIterator it(engine.evaluate("(" + QString(replyData) + ")"));
        QVariantMap tokens;

        while (it.hasNext()) {
            it.next();
            tokens.insert(it.name(), it.value().toVariant());
        }
        // Check for mandatory tokens
        if (tokens.contains(O2_OAUTH2_ACCESS_TOKEN)) {
            setToken(tokens.take(O2_OAUTH2_ACCESS_TOKEN).toString());
            bool ok = false;
            int expiresIn = tokens.take(O2_OAUTH2_EXPIRES_IN).toInt(&ok);
            if (ok) {
                trace() << "Token expires in" << expiresIn << "seconds";
                setExpires(QDateTime::currentMSecsSinceEpoch() / 1000 + expiresIn);
            }
            setRefreshToken(tokens.take(O2_OAUTH2_REFRESH_TOKEN).toString());
            // Set extra tokens if any
            if (!tokens.isEmpty()) {
                setExtraTokens(tokens);
            }
            timedReplies_.remove(tokenReply);
            emit linkedChanged();
            emit tokenChanged();
            emit linkingSucceeded();
        } else {
            qWarning() << "O2::onTokenReplyFinished: oauth_token missing from response" << replyData;
            emit linkedChanged();
            emit tokenChanged();
            emit linkingFailed();
        }
    }
    tokenReply->deleteLater();
}

void O2::onTokenReplyError(QNetworkReply::NetworkError error) {
    QNetworkReply *tokenReply = qobject_cast<QNetworkReply *>(sender());
    trace() << "O2::onTokenReplyError" << error << tokenReply->errorString();
    trace() << "" << tokenReply->readAll();
    setToken(QString());
    setRefreshToken(QString());
    timedReplies_.remove(tokenReply);
    emit linkedChanged();
    emit tokenChanged();
    emit linkingFailed();
}

QByteArray O2::buildRequestBody(const QMap<QString, QString> &parameters) {
    QByteArray body;
    bool first = true;
    foreach (QString key, parameters.keys()) {
        if (first) {
            first = false;
        } else {
            body.append("&");
        }
        QString value = parameters.value(key);
        body.append(QUrl::toPercentEncoding(key) + QString("=").toUtf8() + QUrl::toPercentEncoding(value));
    }
    return body;
}

QString O2::token() {
    QString key = QString(O2_KEY_TOKEN).arg(clientId_);
    return store_->value(key);
}

void O2::setToken(const QString &v) {
    QString key = QString(O2_KEY_TOKEN).arg(clientId_);
    store_->setValue(key, v);
}

int O2::expires() {
    QString key = QString(O2_KEY_EXPIRES).arg(clientId_);
    return store_->value(key).toInt();
}

void O2::setExpires(int v) {
    QString key = QString(O2_KEY_EXPIRES).arg(clientId_);
    store_->setValue(key, QString::number(v));
}

QString O2::refreshToken() {
    QString key = QString(O2_KEY_REFRESH_TOKEN).arg(clientId_);
    return store_->value(key);
}

void O2::setRefreshToken(const QString &v) {
    trace() << "O2::setRefreshToken" << v.left(4) << "...";
    QString key = QString(O2_KEY_REFRESH_TOKEN).arg(clientId_);
    store_->setValue(key, v);
}

void O2::refresh() {
    trace() << "O2::refresh: Token: ..." << refreshToken().right(7);

    if (refreshToken().isEmpty()) {
        qWarning() << "O2::refresh: No refresh token";
        onRefreshError(QNetworkReply::AuthenticationRequiredError);
        return;
    }
    if (refreshTokenUrl_.isEmpty()) {
        qWarning() << "O2::refresh: Refresh token URL not set";
        onRefreshError(QNetworkReply::AuthenticationRequiredError);
        return;
    }

    QNetworkRequest refreshRequest(refreshTokenUrl_);
    refreshRequest.setHeader(QNetworkRequest::ContentTypeHeader, O2_MIME_TYPE_XFORM);
    QMap<QString, QString> parameters;
    parameters.insert(O2_OAUTH2_CLIENT_ID, clientId_);
    parameters.insert(O2_OAUTH2_CLIENT_SECRET, clientSecret_);
    parameters.insert(O2_OAUTH2_REFRESH_TOKEN, refreshToken());
    parameters.insert(O2_OAUTH2_GRANT_TYPE, O2_OAUTH2_REFRESH_TOKEN);
    QByteArray data = buildRequestBody(parameters);
    QNetworkReply *refreshReply = manager_->post(refreshRequest, data);
    timedReplies_.add(refreshReply);
    connect(refreshReply, SIGNAL(finished()), this, SLOT(onRefreshFinished()), Qt::QueuedConnection);
    connect(refreshReply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(onRefreshError(QNetworkReply::NetworkError)), Qt::QueuedConnection);
}

void O2::onRefreshFinished() {
    QNetworkReply *refreshReply = qobject_cast<QNetworkReply *>(sender());
    trace() << "O2::onRefreshFinished: Error" << (int)refreshReply->error() << refreshReply->errorString();
    if (refreshReply->error() == QNetworkReply::NoError) {
        QByteArray reply = refreshReply->readAll();
        QScriptValue value;
        QScriptEngine engine;
        value = engine.evaluate("(" + QString(reply) + ")");
        setToken(value.property(O2_OAUTH2_ACCESS_TOKEN).toString());
        setExpires(QDateTime::currentMSecsSinceEpoch() / 1000 + value.property(O2_OAUTH2_EXPIRES_IN).toInteger());
        setRefreshToken(value.property(O2_OAUTH2_REFRESH_TOKEN).toString());
        timedReplies_.remove(refreshReply);
        emit linkingSucceeded();
        emit tokenChanged();
        emit linkedChanged();
        emit refreshFinished(QNetworkReply::NoError);
        trace() << " New token expires in" << expires() << "seconds";
    }
    refreshReply->deleteLater();
}

void O2::onRefreshError(QNetworkReply::NetworkError error) {
    QNetworkReply *refreshReply = qobject_cast<QNetworkReply *>(sender());
    qWarning() << "O2::onRefreshFailed: Error" << error << ", resetting tokens";
    setToken(QString());
    setRefreshToken(QString());
    timedReplies_.remove(refreshReply);
    emit tokenChanged();
    emit linkingFailed();
    emit linkedChanged();
    emit refreshFinished(error);
}

QString O2::localhostPolicy() const
{
    return localhostPolicy_;
}

void O2::setLocalhostPolicy(const QString& value)
{
    localhostPolicy_ = value;
}
