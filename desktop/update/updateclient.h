#pragma once

#include <QColor>
#include <QDBusConnection>
#include <QObject>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

class QQmlEngine;
class QJSEngine;
class QDBusServiceWatcher;

// QML access to the CustomOS update service (customos-updated,
// org.customos.Update1 on the system bus). The service does all the work;
// this object mirrors its status and forwards the buttons' actions.
//
//   Updates.installed     the update service is part of this system (the
//                         base edition has none; then nothing is offered)
//   Updates.status        the service's GetStatus() map (state, versions,
//                         progress, error, ...), updated live
//   Updates.history       update history, newest first
//   Updates.checkForUpdates() / download() / install() / cancel() /
//   reboot() / rollback() / dismissError()
//
// CUSTOMOS_UPDATE_BUS=session in the environment talks to a service on the
// session bus instead (development and screenshots).
class UpdateClient : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Updates)
    QML_SINGLETON
    Q_PROPERTY(bool installed READ installed NOTIFY installedChanged)
    Q_PROPERTY(bool serviceAvailable READ serviceAvailable NOTIFY serviceAvailableChanged)
    Q_PROPERTY(QVariantMap status READ status NOTIFY statusChanged)
    Q_PROPERTY(QVariantList history READ history NOTIFY historyChanged)
    Q_PROPERTY(bool administrator READ administrator CONSTANT)
    Q_PROPERTY(bool requestPending READ requestPending NOTIFY requestPendingChanged)

public:
    static UpdateClient *create(QQmlEngine *, QJSEngine *);
    explicit UpdateClient(QObject *parent = nullptr);

    bool installed() const { return m_installed; }
    bool serviceAvailable() const { return m_available; }
    QVariantMap status() const { return m_status; }
    QVariantList history() const { return m_history; }
    bool administrator() const { return m_admin; }
    bool requestPending() const { return m_pending > 0; }

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void refreshHistory();
    Q_INVOKABLE void checkForUpdates() { call(QStringLiteral("CheckForUpdates")); }
    Q_INVOKABLE void download() { call(QStringLiteral("Download")); }
    Q_INVOKABLE void install() { call(QStringLiteral("Install")); }
    Q_INVOKABLE void cancel() { call(QStringLiteral("Cancel")); }
    Q_INVOKABLE void reboot() { call(QStringLiteral("Reboot")); }
    Q_INVOKABLE void rollback() { call(QStringLiteral("Rollback")); }
    Q_INVOKABLE void dismissError() { call(QStringLiteral("DismissError")); }

    // Markdown release notes as HTML for a RichText Text, in the theme's colours.
    Q_INVOKABLE QString renderNotes(const QString &markdown, const QColor &text, const QColor &link) const;

    static bool isAdministrator();

signals:
    void installedChanged();
    void serviceAvailableChanged();
    void statusChanged();
    void historyChanged();
    void requestPendingChanged();
    // The service refused an action (permission, busy, unsupported system).
    void actionFailed(const QString &message);
    void updateAvailable(const QString &version, const QString &name);

private slots:
    void onStatusChanged(const QVariantMap &status);
    void onUpdateAvailable(const QString &version, const QString &name);

private:
    void call(const QString &method);
    void setStatus(const QVariantMap &status);
    void setAvailable(bool available);

    QDBusConnection m_bus;
    QDBusServiceWatcher *m_watcher;
    QTimer m_poll;
    QVariantMap m_status;
    QVariantList m_history;
    bool m_installed = false;
    bool m_available = false;
    bool m_admin = false;
    int m_pending = 0;
    QString m_lastState;
};
