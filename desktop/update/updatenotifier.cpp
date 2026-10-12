#include "updatenotifier.h"

#include <QCoreApplication>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusServiceWatcher>
#include <QDir>
#include <QProcess>
#include <QTimer>

static const char Service[] = "org.customos.Update1";
static const char Path[] = "/org/customos/Update1";
static const char Interface[] = "org.customos.Update1";

static QString stateFile()
{
    // $XDG_STATE_HOME (QStandardPaths only knows it from Qt 6.7 on).
    QString dir = qEnvironmentVariable("XDG_STATE_HOME");
    if (dir.isEmpty())
        dir = QDir::homePath() + QStringLiteral("/.local/state");
    return dir + QStringLiteral("/meridian/update-notifier.conf");
}

UpdateNotifier::UpdateNotifier(QObject *parent)
    : QObject(parent),
      m_system(qEnvironmentVariable("CUSTOMOS_UPDATE_BUS") == QLatin1String("session") ? QDBusConnection::sessionBus()
                                                                                       : QDBusConnection::systemBus()),
      m_session(QDBusConnection::sessionBus()),
      m_state(stateFile(), QSettings::IniFormat)
{
    m_system.connect(QString::fromLatin1(Service), QString::fromLatin1(Path), QString::fromLatin1(Interface),
                     QStringLiteral("StatusChanged"), this, SLOT(onStatusChanged(QVariantMap)));
    m_session.connect(QStringLiteral("org.freedesktop.Notifications"), QStringLiteral("/org/freedesktop/Notifications"),
                      QStringLiteral("org.freedesktop.Notifications"), QStringLiteral("ActionInvoked"), this,
                      SLOT(onActionInvoked(uint,QString)));
    // The desktop shell owns the notification service; wait for it.
    auto *shell = new QDBusServiceWatcher(QStringLiteral("org.freedesktop.Notifications"), m_session,
                                          QDBusServiceWatcher::WatchForRegistration, this);
    connect(shell, &QDBusServiceWatcher::serviceRegistered, this, &UpdateNotifier::query);
    QTimer::singleShot(8000, this, &UpdateNotifier::query);
}

void UpdateNotifier::query()
{
    QDBusMessage m = QDBusMessage::createMethodCall(QString::fromLatin1(Service), QString::fromLatin1(Path),
                                                    QString::fromLatin1(Interface), QStringLiteral("GetStatus"));
    auto *w = new QDBusPendingCallWatcher(m_system.asyncCall(m, 15000), this);
    connect(w, &QDBusPendingCallWatcher::finished, this, [this, w] {
        w->deleteLater();
        QDBusPendingReply<QVariantMap> r = *w;
        if (!r.isError())
            onStatusChanged(r.value());
    });
}

void UpdateNotifier::onStatusChanged(const QVariantMap &s)
{
    const QString state = s.value(QStringLiteral("state")).toString();
    const QString name = s.value(QStringLiteral("osName")).toString().isEmpty()
                             ? QStringLiteral("CustomOS") : s.value(QStringLiteral("osName")).toString();
    if (state == QLatin1String("update-available") && s.value(QStringLiteral("updateAvailable")).toBool()) {
        const QString v = s.value(QStringLiteral("availableVersion")).toString();
        notify(QStringLiteral("available-") + v, tr("%1 %2 is available").arg(name, v),
               s.value(QStringLiteral("supported")).toBool()
                   ? tr("Open Settings › System › Updates to see what's new and install it.")
                   : s.value(QStringLiteral("unsupportedMessage")).toString());
    } else if (state == QLatin1String("reboot-required")) {
        const QString v = s.value(QStringLiteral("pendingVersion")).toString();
        notify(QStringLiteral("reboot-") + v, tr("Restart to finish the update"),
               s.value(QStringLiteral("pendingKind")).toString() == QLatin1String("rollback")
                   ? tr("The previous version of %1 starts after a restart.").arg(name)
                   : tr("%1 %2 is installed and starts after a restart.").arg(name, v));
    } else if (state == QLatin1String("error") && s.value(QStringLiteral("errorCode")).toString() == QLatin1String("rolled-back")) {
        notify(QStringLiteral("rolled-back-") + s.value(QStringLiteral("errorDetail")).toString(),
               tr("The update didn't work"), s.value(QStringLiteral("errorMessage")).toString());
    }
}

void UpdateNotifier::notify(const QString &key, const QString &summary, const QString &body)
{
    if (m_state.value(QStringLiteral("announced/") + key).toBool())
        return;
    QDBusMessage m = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.Notifications"),
                                                    QStringLiteral("/org/freedesktop/Notifications"),
                                                    QStringLiteral("org.freedesktop.Notifications"),
                                                    QStringLiteral("Notify"));
    m << QStringLiteral("Meridian Update") << uint(0) << QStringLiteral("meridian-update") << summary << body
      << QStringList{QStringLiteral("default"), tr("Open"), QStringLiteral("open"), tr("Open Updates")}
      << QVariantMap{{QStringLiteral("desktop-entry"), QStringLiteral("meridian-update")},
                     {QStringLiteral("resident"), false}}
      << int(-1);
    auto *w = new QDBusPendingCallWatcher(m_session.asyncCall(m, 10000), this);
    connect(w, &QDBusPendingCallWatcher::finished, this, [this, w, key] {
        w->deleteLater();
        QDBusPendingReply<uint> r = *w;
        if (r.isError())
            return;   // no notification server yet; announce on the next event
        m_ids << r.value();
        m_state.setValue(QStringLiteral("announced/") + key, true);
        m_state.sync();
    });
}

void UpdateNotifier::onActionInvoked(uint id, const QString &action)
{
    if (!m_ids.contains(id))
        return;
    if (action == QLatin1String("open") || action == QLatin1String("default"))
        QProcess::startDetached(QStringLiteral("meridian-settings"), {QStringLiteral("updates")});
}
