#include <QCryptographicHash>
#include <QByteArray>

#include "o2settingsstore.h"

static quint64 getHash(const QString &encryptionKey) {
    return QCryptographicHash::hash(encryptionKey.toLatin1(), QCryptographicHash::Sha1).toULongLong();
}

O2SettingsStore::O2SettingsStore(const QString &encryptionKey, QObject *parent):
    O2AbstractStore(parent), crypt_(getHash(encryptionKey)) {
    settings_ = new QSettings(this);
}

O2SettingsStore::O2SettingsStore(QSettings *settings, const QString &encryptionKey, QObject *parent):
    O2AbstractStore(parent), crypt_(getHash(encryptionKey)) {
    settings_ = settings;
    settings_->setParent(this);
}

O2SettingsStore::~O2SettingsStore() {
}

QString O2SettingsStore::groupKey() const {
    return groupKey_;
}

void O2SettingsStore::setGroupKey(const QString &groupKey) {
    if (groupKey_ == groupKey) {
        return;
    }
    groupKey_ = groupKey;
    emit groupKeyChanged();
}

QString O2SettingsStore::value(const QString &key, const QString &defaultValue) {
    QString fullKey = groupKey_.isEmpty() ? key : (groupKey_ + '/' + key);
    if (!settings_->contains(fullKey)) {
        return defaultValue;
    }
    return crypt_.decryptToString(settings_->value(fullKey).toString());
}

void O2SettingsStore::setValue(const QString &key, const QString &value) {
    QString fullKey = groupKey_.isEmpty() ? key : (groupKey_ + '/' + key);
    settings_->setValue(fullKey, crypt_.encryptToString(value));
}
