#ifndef O2_H
#define O2_H

#include <QObject>
#include <QString>
#include <QUrl>
#include <QMap>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QList>
#include <QPair>

#include "o2reply.h"
#include "o2abstractstore.h"

class O2ReplyServer;

/// Simple OAuth2 authenticator.
class O2: public QObject {
    Q_OBJECT
    Q_ENUMS(GrantFlow)

public:
    enum GrantFlow {GrantFlowAuthorizationCode, GrantFlowImplicit};

    /// Authorization flow: Authorization Code (default, see http://tools.ietf.org/html/draft-ietf-oauth-v2-15#section-4.1) or Implicit (see http://tools.ietf.org/html/draft-ietf-oauth-v2-15#section-4.2)
    Q_PROPERTY(GrantFlow grantFlow READ grantFlow WRITE setGrantFlow NOTIFY grantFlowChanged)
    GrantFlow grantFlow();
    void setGrantFlow(GrantFlow value);

    /// Are we authenticated?
    Q_PROPERTY(bool linked READ linked NOTIFY linkedChanged)
    bool linked();

    /// Extra tokens available after a successful OAuth exchange
    Q_PROPERTY(QMap extraTokens READ extraTokens)
    QVariantMap extraTokens() const;

    /// Authentication token.
    Q_PROPERTY(QString token READ token WRITE setToken NOTIFY tokenChanged)
    QString token();
    void setToken(const QString &v);

    /// Client ID.
    /// O2 instances with the same (client ID, client secret) share the same "linked" and "token" properties.
    Q_PROPERTY(QString clientId READ clientId WRITE setClientId NOTIFY clientIdChanged)
    QString clientId();
    void setClientId(const QString &value);

    /// Client secret.
    /// O2 instances with the same (client ID, client secret) share the same "linked" and "token" properties.
    Q_PROPERTY(QString clientSecret READ clientSecret WRITE setClientSecret NOTIFY clientSecretChanged)
    QString clientSecret();
    void setClientSecret(const QString &value);

    /// Scope of authentication.
    Q_PROPERTY(QString scope READ scope WRITE setScope NOTIFY scopeChanged)
    QString scope();
    void setScope(const QString &value);

    /// Request URL.
    Q_PROPERTY(QString requestUrl READ requestUrl WRITE setRequestUrl NOTIFY requestUrlChanged)
    QString requestUrl();
    void setRequestUrl(const QString &value);

    /// Token request URL.
    Q_PROPERTY(QString tokenUrl READ tokenUrl WRITE setTokenUrl NOTIFY tokenUrlChanged)
    QString tokenUrl();
    void setTokenUrl(const QString &value);

    /// Token refresh URL.
    Q_PROPERTY(QString refreshTokenUrl READ refreshTokenUrl WRITE setRefreshTokenUrl NOTIFY refreshTokenUrlChanged)
    QString refreshTokenUrl();
    void setRefreshTokenUrl(const QString &value);

    /// TCP port number to use in local redirections.
    /// The OAuth 2.0 "redirect_uri" will be set to "http://localhost:<localPort>/".
    /// If localPort is set to 0 (default), O2 will replace it with a free one.
    Q_PROPERTY(int localPort READ localPort WRITE setLocalPort NOTIFY localPortChanged)
    int localPort();
    void setLocalPort(int value);

    /// Localhost policy. By default it's value is http://127.0.0.1:%1/, however some services may
    /// require the use of http://localhost:%1/ or any other value.
    Q_PROPERTY(QString localhostPolicy READ localhostPolicy WRITE setLocalhostPolicy)
    QString localhostPolicy() const;
    void setLocalhostPolicy(const QString &value);

public:
    /// Constructor.
    /// @param  parent  Parent object.
    explicit O2(QObject *parent = 0);

    /// Destructor.
    virtual ~O2();

    /// Get authentication code.
    QString code();

    /// Get refresh token.
    QString refreshToken();

    /// Get token expiration time (seconds from Epoch).
    int expires();

    /// Sets the storage object to use for storing the OAuth tokens on a peristent medium
    void setStore(O2AbstractStore *store);

public slots:
    /// Authenticate.
    Q_INVOKABLE virtual void link();

    /// De-authenticate.
    Q_INVOKABLE virtual void unlink();

    /// Refresh token.
    void refresh();

signals:
    /// Emitted when client needs to open a web browser window, with the given URL.
    void openBrowser(const QUrl &url);

    /// Emitted when client can close the browser window.
    void closeBrowser();

    /// Emitted when authentication/deauthentication succeeded.
    void linkingSucceeded();

    /// Emitted when authentication/deauthentication failed.
    void linkingFailed();

    /// Emitted when a token refresh has been completed or failed.
    void refreshFinished(QNetworkReply::NetworkError error);

    // Property change signals
    void grantFlowChanged();
    void linkedChanged();
    void tokenChanged();
    void clientIdChanged();
    void clientSecretChanged();
    void scopeChanged();
    void requestUrlChanged();
    void tokenUrlChanged();
    void refreshTokenUrlChanged();
    void localPortChanged();

protected slots:
    /// Handle verification response.
    virtual void onVerificationReceived(QMap<QString, QString>);

    /// Handle completion of a token request.
    virtual void onTokenReplyFinished();

    /// Handle failure of a token request.
    virtual void onTokenReplyError(QNetworkReply::NetworkError error);

    /// Handle completion of a refresh request.
    virtual void onRefreshFinished();

    /// Handle failure of a refresh request.
    virtual void onRefreshError(QNetworkReply::NetworkError error);

protected:
    /// Build HTTP request body.
    QByteArray buildRequestBody(const QMap<QString, QString> &parameters);

    /// Set authentication code.
    void setCode(const QString &v);

    /// Set refresh token.
    void setRefreshToken(const QString &v);

    /// Set token expiration time.
    void setExpires(int v);

    /// Set extra tokens found in OAuth response
    void setExtraTokens(QVariantMap extraTokens);

protected:
    QString clientId_;
    QString clientSecret_;
    QString scope_;
    QString code_;
    QString redirectUri_;
    QString localhostPolicy_;
    QUrl requestUrl_;
    QUrl tokenUrl_;
    QUrl refreshTokenUrl_;
    QNetworkAccessManager *manager_;
    O2ReplyServer *replyServer_;
    O2ReplyList timedReplies_;
    quint16 localPort_;
    GrantFlow grantFlow_;
    O2AbstractStore *store_;
    QVariantMap extraTokens_;
};

#endif // O2_H
