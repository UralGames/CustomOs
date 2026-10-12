#pragma once

#include <QObject>
#include <QtQml/qqmlregistration.h>

class QLocalServer;
class QQmlEngine;
class QJSEngine;

// Control socket so the compositor's key bindings (and scripts) can drive
// the shell:  meridian-shell --action launcher
class ShellIpc : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(ShellIpc)
    QML_SINGLETON

public:
    static ShellIpc *instance();
    static ShellIpc *create(QQmlEngine *, QJSEngine *);

    static QString socketPath();
    // Client side: send one action to the running shell. Returns false if no
    // shell is listening.
    static bool send(const QString &action);
    bool listen();

signals:
    void action(const QString &name);

private:
    explicit ShellIpc(QObject *parent = nullptr);
    QLocalServer *m_server = nullptr;
};
