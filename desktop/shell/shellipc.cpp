#include "shellipc.h"

#include <QDir>
#include <QLocalServer>
#include <QLocalSocket>
#include <QQmlEngine>
#include <QStandardPaths>

namespace {
ShellIpc *s_instance = nullptr;
}

ShellIpc *ShellIpc::instance()
{
    if (!s_instance)
        s_instance = new ShellIpc();
    return s_instance;
}

ShellIpc *ShellIpc::create(QQmlEngine *, QJSEngine *)
{
    ShellIpc *i = instance();
    QQmlEngine::setObjectOwnership(i, QQmlEngine::CppOwnership);
    return i;
}

ShellIpc::ShellIpc(QObject *parent)
    : QObject(parent)
{
}

QString ShellIpc::socketPath()
{
    QString dir = qEnvironmentVariable("XDG_RUNTIME_DIR");
    if (dir.isEmpty())
        dir = QDir::tempPath();
    QString display = qEnvironmentVariable("WAYLAND_DISPLAY");
    if (display.isEmpty())
        display = QStringLiteral("wayland-0");
    return dir + QStringLiteral("/meridian-shell-") + display + QStringLiteral(".sock");
}

bool ShellIpc::listen()
{
    if (m_server)
        return true;
    m_server = new QLocalServer(this);
    m_server->setSocketOptions(QLocalServer::UserAccessOption);
    QLocalServer::removeServer(socketPath());
    if (!m_server->listen(socketPath())) {
        qWarning("meridian-shell: cannot listen on %s", qPrintable(socketPath()));
        return false;
    }
    connect(m_server, &QLocalServer::newConnection, this, [this] {
        while (QLocalSocket *c = m_server->nextPendingConnection()) {
            connect(c, &QLocalSocket::readyRead, this, [this, c] {
                while (c->canReadLine()) {
                    const QString line = QString::fromUtf8(c->readLine()).trimmed();
                    if (!line.isEmpty())
                        emit action(line);
                }
            });
            connect(c, &QLocalSocket::disconnected, c, &QObject::deleteLater);
        }
    });
    return true;
}

bool ShellIpc::send(const QString &action)
{
    QLocalSocket s;
    s.connectToServer(socketPath());
    if (!s.waitForConnected(1000))
        return false;
    s.write(action.toUtf8() + '\n');
    s.flush();
    s.waitForBytesWritten(1000);
    s.disconnectFromServer();
    return true;
}
