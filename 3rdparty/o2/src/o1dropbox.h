#ifndef O1DROPBOX_H
#define O1DROPBOX_H

#include "o1.h"

class O1Dropbox: public O1 {
    Q_OBJECT

public:
    explicit O1Dropbox(QObject *parent = 0): O1(parent) {
        setRequestTokenUrl(QUrl("https://api.dropbox.com/1/oauth/request_token"));
        setAuthorizeUrl(QUrl("https://www.dropbox.com/1/oauth/authorize?display=mobile"));
        setAccessTokenUrl(QUrl("https://api.dropbox.com/1/oauth/access_token"));
        setLocalPort(1965);
    }
};

#endif // O1DROPBOX_H
