#pragma once

#include <QDBusConnection>
#include <QObject>
#include <QSettings>
#include <QVariantMap>

// meridian-update --notify: runs in the background of every desktop session
// (started from labwc's autostart) and turns update-service events into
// desktop notifications: a new version is available, an installed update
// needs a restart, an update was rolled back. Each event is announced once.
class UpdateNotifier : public QObject
{
    Q_OBJECT
public:
    explicit UpdateNotifier(QObject *parent = nullptr);

private slots:
    void onStatusChanged(const QVariantMap &status);
    void onActionInvoked(uint id, const QString &action);

private:
    void query();
    void notify(const QString &key, const QString &summary, const QString &body);

    QDBusConnection m_system;
    QDBusConnection m_session;
    QSettings m_state;
    QList<uint> m_ids;
};
