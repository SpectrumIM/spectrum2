#ifndef O2SETTINGSSTORE_H
#define O2SETTINGSSTORE_H

#include <QSettings>
#include <QString>

#include "o2abstractstore.h"
#include "simplecrypt.h"

class O2SettingsStore: public O2AbstractStore
{
    Q_OBJECT

public:

    explicit O2SettingsStore(const QString &encryptionKey, QObject *parent = 0);

    explicit O2SettingsStore(QSettings *settings, const QString &encryptionKey, QObject *parent = 0);

    ~O2SettingsStore();

    Q_PROPERTY(QString groupKey READ groupKey WRITE setGroupKey NOTIFY groupKeyChanged)
    QString groupKey() const;
    void setGroupKey(const QString &groupKey);

    QString value(const QString &key, const QString &defaultValue = QString());
    void setValue(const QString &key, const QString &value);

signals:
    // Property change signals
    void groupKeyChanged();

protected:
    QSettings* settings_;
    QString groupKey_;
    SimpleCrypt crypt_;
};

#endif // O2SETTINGSSTORE_H
