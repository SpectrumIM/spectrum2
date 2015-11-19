#ifndef O1_H
#define O1_H

#include <QObject>
#include <QString>
#include <QMap>
#include <QList>
#include <QByteArray>
#include <QNetworkAccessManager>
#include <QUrl>
#include <QNetworkReply>

#include "o2abstractstore.h"

class O2ReplyServer;

/// Request parameter (name-value pair) participating in authentication.
struct O1RequestParameter {
    O1RequestParameter(const QByteArray &n, const QByteArray &v): name(n), value(v) {
    }
    bool operator <(const O1RequestParameter &other) const {
        return (name == other.name)? (value < other.value): (name < other.name);
    }
    QByteArray name;
    QByteArray value;
};

/// Simple OAuth 1.0 authenticator.
class O1: public QObject {
    Q_OBJECT

public:
    /// Are we authenticated?
    Q_PROPERTY(bool linked READ linked NOTIFY linkedChanged)
    bool linked();

    /// Authentication token.
    QString token();

    /// Authentication token secret.
    QString tokenSecret();

    /// Extra tokens available after a successful OAuth exchange
    Q_PROPERTY(QMap extraTokens READ extraTokens)
    QVariantMap extraTokens() const;

    /// Client application ID.
    /// O1 instances with the same (client ID, client secret) share the same "linked", "token" and "tokenSecret" properties.
    Q_PROPERTY(QString clientId READ clientId WRITE setClientId NOTIFY clientIdChanged)
    QString clientId();
    void setClientId(const QString &value);

    /// Client application secret.
    /// O1 instances with the same (client ID, client secret) share the same "linked", "token" and "tokenSecret" properties.
    Q_PROPERTY(QString clientSecret READ clientSecret WRITE setClientSecret NOTIFY clientSecretChanged)
    QString clientSecret();
    void setClientSecret(const QString &value);

    /// Token request URL.
    Q_PROPERTY(QUrl requestTokenUrl READ requestTokenUrl WRITE setRequestTokenUrl NOTIFY requestTokenUrlChanged)
    QUrl requestTokenUrl();
    void setRequestTokenUrl(const QUrl &value);

    /// Authorization URL.
    Q_PROPERTY(QUrl authorizeUrl READ authorizeUrl WRITE setAuthorizeUrl NOTIFY authorizeUrlChanged)
    QUrl authorizeUrl();
    void setAuthorizeUrl(const QUrl &value);

    /// Access token URL.
    Q_PROPERTY(QUrl accessTokenUrl READ accessTokenUrl WRITE setAccessTokenUrl NOTIFY accessTokenUrlChanged)
    QUrl accessTokenUrl();
    void setAccessTokenUrl(const QUrl &value);

    /// Signature method
    Q_PROPERTY(QString signatureMethod READ signatureMethod WRITE setSignatureMethod NOTIFY signatureMethodChanged)
    QString signatureMethod();
    void setSignatureMethod(const QString &value);

    /// TCP port number to use in local redirections.
    /// The OAuth "redirect_uri" will be set to "http://localhost:<localPort>/".
    /// If localPort is set to 0 (default), O1 will replace it with a free one.
    Q_PROPERTY(int localPort READ localPort WRITE setLocalPort NOTIFY localPortChanged)
    int localPort();
    void setLocalPort(int value);

    /// Constructor.
    /// @param  parent  Parent object.
    explicit O1(QObject *parent = 0);

    /// Destructor.
    virtual ~O1();

    /// Sets the storage object to use for storing the OAuth tokens on a peristent medium
    void setStore(O2AbstractStore *store);

    /// Parse a URL-encoded response string.
    static QMap<QString, QString> parseResponse(const QByteArray &response);

    /// Build the value of the "Authorization:" header.
    static QByteArray buildAuthorizationHeader(const QList<O1RequestParameter> &oauthParams);

    /// Create unique bytes to prevent replay attacks.
    static QByteArray nonce();


    /// Generate signature string depending on signature method type
    QByteArray generateSignature(const QList<O1RequestParameter> headers, const QNetworkRequest &req, const QList<O1RequestParameter> &signingParameters, QNetworkAccessManager::Operation operation);

    /// Calculate the HMAC-SHA1 signature of a request.
    /// @param  oauthParams     OAuth parameters.
    /// @param  otherParams     Other parameters participating in signing.
    /// @param  URL             Request URL. May contain query parameters, but they will not be used for signing.
    /// @param  op              HTTP operation.
    /// @param  consumerSecret  Consumer (application) secret.
    /// @param  tokenSecret     Authorization token secret (empty if not yet available).
    /// @return Signature that can be used as the value of the "oauth_signature" parameter.
    static QByteArray sign(const QList<O1RequestParameter> &oauthParams, const QList<O1RequestParameter> &otherParams, const QUrl &url, QNetworkAccessManager::Operation op, const QString &consumerSecret, const QString &tokenSecret);

    /// Build a base string for signing.
    static QByteArray getRequestBase(const QList<O1RequestParameter> &oauthParams, const QList<O1RequestParameter> &otherParams, const QUrl &url, QNetworkAccessManager::Operation op);

    /// Build a concatenated/percent-encoded string from a list of headers.
    static QByteArray encodeHeaders(const QList<O1RequestParameter> &headers);

    /// Construct query string from list of headers
    static QByteArray createQueryParams(const QList<O1RequestParameter> &params);

public slots:
    /// Authenticate.
    Q_INVOKABLE virtual void link();

    /// De-authenticate.
    Q_INVOKABLE virtual void unlink();

signals:
    /// Emitted when client needs to open a web browser window, with the given URL.
    void openBrowser(const QUrl &url);

    /// Emitted when client can close the browser window.
    void closeBrowser();

    /// Emitted when authentication/deauthentication succeeded.
    void linkingSucceeded();

    /// Emitted when authentication/deauthentication failed.
    void linkingFailed();

    // Property change signals

    void linkedChanged();
    void clientIdChanged();
    void clientSecretChanged();
    void requestTokenUrlChanged();
    void authorizeUrlChanged();
    void accessTokenUrlChanged();
    void localPortChanged();
    void signatureMethodChanged();

protected slots:
    /// Handle verification received from the reply server.
    virtual void onVerificationReceived(QMap<QString,QString> params);

    /// Handle token request error.
    virtual void onTokenRequestError(QNetworkReply::NetworkError error);

    /// Handle token request finished.
    virtual void onTokenRequestFinished();

    /// Handle token exchange error.
    void onTokenExchangeError(QNetworkReply::NetworkError error);

    /// Handle token exchange finished.
    void onTokenExchangeFinished();

protected:
    /// Set authentication token.
    void setToken(const QString &v);

    /// Set authentication token secret.
    void setTokenSecret(const QString &v);

    /// Exchange token for authorizaton token.
    virtual void exchangeToken();

    /// Set extra tokens found in OAuth response
    void setExtraTokens(QVariantMap extraTokens);

protected:
    QString clientId_;
    QString clientSecret_;
    QString scope_;
    QString code_;
    QString redirectUri_;
    QString requestToken_;
    QString requestTokenSecret_;
    QString verifier_;
    QString signatureMethod_;
    QUrl requestTokenUrl_;
    QUrl authorizeUrl_;
    QUrl accessTokenUrl_;
    QNetworkAccessManager *manager_;
    O2ReplyServer *replyServer_;
    quint16 localPort_;
    O2AbstractStore *store_;
    QVariantMap extraTokens_;
};

#endif // O1_H
