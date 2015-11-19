#ifndef O2HUBIC_H
#define O2HUBIC_H

#include "o2.h"

/// Hubic's dialect of OAuth 2.0
class O2Hubic: public O2 {
    Q_OBJECT

public:
    /// Constructor.
    /// @param  parent  Parent object.
    explicit O2Hubic(QObject *parent = 0);

};

#endif // O2_HUBIC
