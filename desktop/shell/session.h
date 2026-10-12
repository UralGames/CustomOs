#pragma once

#include <QObject>
#include <QtQml/qqmlregistration.h>

class QQmlEngine;
class QJSEngine;
class QTimer;

// Session services for the shell: power actions through systemd-logind,
// sign-out, locking (swaylock), screenshots (grim), and keeping the
// compositor's configuration (labwc rc.xml, environment) in sync with the
// desktop settings.
class Session : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Session)
    QML_SINGLETON

public:
    static Session *instance();
    static Session *create(QQmlEngine *, QJSEngine *);

    // Called once at start-up.
    Q_INVOKABLE void start();
    Q_INVOKABLE void logout();
    Q_INVOKABLE void powerOff();
    Q_INVOKABLE void reboot();
    Q_INVOKABLE void lock();
    // Saves a PNG to ~/Pictures/Screenshots; returns its path or "".
    Q_INVOKABLE void screenshot();
    // Regenerates labwc's config from Settings and asks labwc to reload.
    Q_INVOKABLE void writeCompositorConfig();

    static QString renderRcXml(const QString &templ);

signals:
    void screenshotSaved(const QString &path);
    void screenshotFailed(const QString &reason);

private:
    explicit Session(QObject *parent = nullptr);
    bool callLogind(const QString &method);
    void reconfigureCompositor();

    QTimer *m_configTimer = nullptr;
};
