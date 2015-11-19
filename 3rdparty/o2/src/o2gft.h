#ifndef O2GFT_H
#define O2GFT_H

#include "o2.h"

/// Google Fusion Tables' dialect of OAuth 2.0
class O2Gft: public O2 {
    Q_OBJECT

public:
    explicit O2Gft(QObject *parent = 0);
};

#endif // O2GFT_H
