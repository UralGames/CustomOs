#pragma once

#include <QQmlPropertyMap>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

class QFileSystemWatcher;
class QQmlEngine;
class QJSEngine;

// Desktop-wide user preferences, stored in ~/.config/meridian/desktop.conf.
//
// Exposed to QML as the `Settings` singleton; every key below is a bindable
// property (Settings.darkMode, Settings.wallpaper, ...). Writes from QML are
// persisted immediately. The file is watched, so a change made by the
// Settings app is picked up live by the shell and by every other app.
class Settings : public QQmlPropertyMap
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Settings)
    QML_SINGLETON

public:
    static Settings *instance();
    static Settings *create(QQmlEngine *, QJSEngine *);

    QString filePath() const { return m_path; }
    Q_INVOKABLE void set(const QString &key, const QVariant &value);
    Q_INVOKABLE void resetToDefaults();
    Q_INVOKABLE QVariant defaultValue(const QString &key) const { return m_defaults.value(key); }

signals:
    void settingChanged(const QString &key);

protected:
    QVariant updateValue(const QString &key, const QVariant &input) override;

private:
    explicit Settings(QObject *parent = nullptr);
    void reload();
    void save();
    void watch();
    QVariant coerce(const QString &key, const QVariant &raw) const;

    QString m_path;
    QVariantMap m_defaults;
    QFileSystemWatcher *m_watcher = nullptr;
    bool m_saving = false;
};
