#include "greeterbackend.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QSocketNotifier>
#include <QSysInfo>
#include <QTimer>
#include <QVariantMap>

#include <pwd.h>
#include <unistd.h>

GreeterBackend::GreeterBackend(QObject *parent)
    : QObject(parent)
{
    // People who can sign in: regular accounts with a real shell.
    setpwent();
    while (const passwd *pw = getpwent()) {
        if (pw->pw_uid < 1000 || pw->pw_uid >= 60000)
            continue;
        const QString shell = QString::fromLocal8Bit(pw->pw_shell);
        if (shell.endsWith(QLatin1String("nologin")) || shell.endsWith(QLatin1String("false")))
            continue;
        const QString name = QString::fromLocal8Bit(pw->pw_name);
        QString full = QString::fromLocal8Bit(pw->pw_gecos).section(QLatin1Char(','), 0, 0);
        if (full.isEmpty()) full = name;
        m_users << QVariantMap{{QStringLiteral("name"), name}, {QStringLiteral("fullName"), full}};
    }
    endpwent();

    m_notifier = new QSocketNotifier(STDIN_FILENO, QSocketNotifier::Read, this);
    connect(m_notifier, &QSocketNotifier::activated, this, [this] {
        char buf[1024];
        const ssize_t n = ::read(STDIN_FILENO, buf, sizeof buf);
        if (n <= 0) {
            // The login service went away; nothing more to do.
            m_notifier->setEnabled(false);
            QCoreApplication::exit(0);
            return;
        }
        m_buffer.append(buf, int(n));
        int nl;
        while ((nl = m_buffer.indexOf('\n')) >= 0) {
            const QByteArray line = m_buffer.left(nl);
            m_buffer.remove(0, nl + 1);
            m_busy = false;
            emit busyChanged();
            if (line == "OK") {
                emit succeeded();
                // Leave the screen to the session promptly.
                QTimer::singleShot(150, qApp, [] { QCoreApplication::exit(0); });
            } else if (line.startsWith("FAIL\t")) {
                emit failed(QString::fromUtf8(line.mid(5)));
            }
        }
    });
}

QString GreeterBackend::hostName() const
{
    return QSysInfo::machineHostName();
}

bool GreeterBackend::live() const
{
    return QFileInfo::exists(QStringLiteral("/etc/customos-live"));
}

void GreeterBackend::send(const QByteArray &line)
{
    QByteArray l = line + '\n';
    const char *p = l.constData();
    qsizetype left = l.size();
    while (left > 0) {
        const ssize_t w = ::write(STDOUT_FILENO, p, size_t(left));
        if (w <= 0) break;
        p += w;
        left -= w;
    }
    l.fill('\0');
}

void GreeterBackend::login(const QString &user, const QString &password)
{
    if (m_busy || user.isEmpty())
        return;
    if (user.contains(QLatin1Char('\t')) || password.contains(QLatin1Char('\n'))) {
        emit failed(tr("Invalid characters."));
        return;
    }
    m_busy = true;
    emit busyChanged();
    QByteArray line = "LOGIN\t" + user.toUtf8() + '\t' + password.toUtf8();
    send(line);
    line.fill('\0');
}

void GreeterBackend::powerOff() { send("POWEROFF"); }
void GreeterBackend::reboot() { send("REBOOT"); }
