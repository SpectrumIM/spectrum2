#ifndef O1TWITTER_H
#define O1TWITTER_H

#include "o1.h"

class O1Twitter: public O1 {
    Q_OBJECT

public:
    explicit O1Twitter(QObject *parent = 0): O1(parent) {
        setRequestTokenUrl(QUrl("https://api.twitter.com/oauth/request_token"));
        setAuthorizeUrl(QUrl("https://api.twitter.com/oauth/authenticate"));
        setAccessTokenUrl(QUrl("https://api.twitter.com/oauth/access_token"));
    }
};

#endif // O1TWITTER_H
