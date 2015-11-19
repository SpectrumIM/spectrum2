#ifndef O2ABSTRACTSTORE_H
#define O2ABSTRACTSTORE_H

#include <QObject>
#include <QString>

class O2AbstractStore: public QObject
{
    Q_OBJECT

public:

    explicit O2AbstractStore(QObject *parent = 0): QObject(parent) {
    }

    virtual ~O2AbstractStore() {
    }

    virtual QString value(const QString &key, const QString &defaultValue = QString()) = 0;

    virtual void setValue(const QString &key, const QString &value) = 0;
};

#endif // O2ABSTRACTSTORE_H
