#include "updateclient.h"

#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusServiceWatcher>
#include <QJsonArray>
#include <QJsonDocument>
#include <QQmlEngine>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextFragment>
#include <QVector>

#include <grp.h>
#include <pwd.h>
#include <unistd.h>

static const char Service[] = "org.customos.Update1";
static const char Path[] = "/org/customos/Update1";
static const char Interface[] = "org.customos.Update1";

static QDBusConnection updateBus()
{
    return qEnvironmentVariable("CUSTOMOS_UPDATE_BUS") == QLatin1String("session") ? QDBusConnection::sessionBus()
                                                                                   : QDBusConnection::systemBus();
}

UpdateClient *UpdateClient::create(QQmlEngine *, QJSEngine *)
{
    static UpdateClient *c = new UpdateClient();
    QQmlEngine::setObjectOwnership(c, QQmlEngine::CppOwnership);
    return c;
}

UpdateClient::UpdateClient(QObject *parent)
    : QObject(parent),
      m_bus(updateBus()),
      m_watcher(new QDBusServiceWatcher(QString::fromLatin1(Service), m_bus,
                                        QDBusServiceWatcher::WatchForRegistration
                                            | QDBusServiceWatcher::WatchForUnregistration, this)),
      m_admin(isAdministrator())
{
    m_bus.connect(QString::fromLatin1(Service), QString::fromLatin1(Path), QString::fromLatin1(Interface),
                  QStringLiteral("StatusChanged"), this, SLOT(onStatusChanged(QVariantMap)));
    m_bus.connect(QString::fromLatin1(Service), QString::fromLatin1(Path), QString::fromLatin1(Interface),
                  QStringLiteral("UpdateAvailable"), this, SLOT(onUpdateAvailable(QString,QString)));
    connect(m_watcher, &QDBusServiceWatcher::serviceRegistered, this, [this] { refresh(); refreshHistory(); });
    connect(m_watcher, &QDBusServiceWatcher::serviceUnregistered, this, [this] { setAvailable(false); });
    // The status arrives as signals; polling only covers a missed signal and
    // keeps "last checked" fresh.
    m_poll.setInterval(30000);
    connect(&m_poll, &QTimer::timeout, this, &UpdateClient::refresh);
    m_poll.start();
    refresh();
    refreshHistory();
}

bool UpdateClient::isAdministrator()
{
    const uid_t uid = getuid();
    if (uid == 0)
        return true;
    const passwd *pw = getpwuid(uid);
    const group *wheel = getgrnam("wheel");
    if (!pw || !wheel)
        return false;
    int count = 64;
    QVector<gid_t> groups(count);
    if (getgrouplist(pw->pw_name, pw->pw_gid, groups.data(), &count) < 0) {
        groups.resize(count);
        if (getgrouplist(pw->pw_name, pw->pw_gid, groups.data(), &count) < 0)
            return false;
    }
    for (int i = 0; i < count; ++i)
        if (groups.at(i) == wheel->gr_gid)
            return true;
    return false;
}

QString UpdateClient::renderNotes(const QString &markdown, const QColor &text, const QColor &link) const
{
    QTextDocument doc;
    doc.setMarkdown(markdown, QTextDocument::MarkdownDialectGitHub);
    // Recolour the text and the links of every fragment.
    for (QTextBlock block = doc.begin(); block.isValid(); block = block.next()) {
        for (auto it = block.begin(); !it.atEnd(); ++it) {
            const QTextFragment f = it.fragment();
            if (!f.isValid())
                continue;
            QTextCursor c(&doc);
            c.setPosition(f.position());
            c.setPosition(f.position() + f.length(), QTextCursor::KeepAnchor);
            QTextCharFormat fmt;
            fmt.setForeground(f.charFormat().isAnchor() ? link : text);
            c.mergeCharFormat(fmt);
        }
    }
    QString html = doc.toHtml();
    // Let the Text item's own font apply.
    html.remove(QStringLiteral(" font-family:'Sans Serif';"));
    return html;
}

void UpdateClient::setAvailable(bool available)
{
    if (m_available == available)
        return;
    m_available = available;
    emit serviceAvailableChanged();
}

void UpdateClient::setStatus(const QVariantMap &status)
{
    const QString state = status.value(QStringLiteral("state")).toString();
    m_status = status;
    emit statusChanged();
    // The history changes when an operation ends.
    if (state != m_lastState) {
        m_lastState = state;
        refreshHistory();
    }
}

void UpdateClient::refresh()
{
    QDBusMessage m = QDBusMessage::createMethodCall(QString::fromLatin1(Service), QString::fromLatin1(Path),
                                                    QString::fromLatin1(Interface), QStringLiteral("GetStatus"));
    auto *w = new QDBusPendingCallWatcher(m_bus.asyncCall(m, 15000), this);
    connect(w, &QDBusPendingCallWatcher::finished, this, [this, w] {
        w->deleteLater();
        QDBusPendingReply<QVariantMap> r = *w;
        if (r.isError()) {
            setAvailable(false);
            return;
        }
        setAvailable(true);
        setStatus(r.value());
    });
}

void UpdateClient::refreshHistory()
{
    QDBusMessage m = QDBusMessage::createMethodCall(QString::fromLatin1(Service), QString::fromLatin1(Path),
                                                    QString::fromLatin1(Interface), QStringLiteral("GetHistory"));
    auto *w = new QDBusPendingCallWatcher(m_bus.asyncCall(m, 15000), this);
    connect(w, &QDBusPendingCallWatcher::finished, this, [this, w] {
        w->deleteLater();
        QDBusPendingReply<QString> r = *w;
        if (r.isError())
            return;
        const QJsonArray arr = QJsonDocument::fromJson(r.value().toUtf8()).array();
        QVariantList list;
        for (int i = arr.size() - 1; i >= 0; --i)   // newest first
            list << arr.at(i).toVariant();
        m_history = list;
        emit historyChanged();
    });
}

void UpdateClient::call(const QString &method)
{
    QDBusMessage m = QDBusMessage::createMethodCall(QString::fromLatin1(Service), QString::fromLatin1(Path),
                                                    QString::fromLatin1(Interface), method);
    ++m_pending;
    emit requestPendingChanged();
    auto *w = new QDBusPendingCallWatcher(m_bus.asyncCall(m, 30000), this);
    connect(w, &QDBusPendingCallWatcher::finished, this, [this, w] {
        w->deleteLater();
        --m_pending;
        emit requestPendingChanged();
        const QDBusMessage reply = w->reply();
        if (reply.type() == QDBusMessage::ErrorMessage) {
            const QString name = reply.errorName();
            QString message = reply.errorMessage();
            if (name == QLatin1String("org.freedesktop.DBus.Error.AccessDenied"))
                message = tr("Only administrators can install updates.");
            else if (name == QLatin1String("org.freedesktop.DBus.Error.ServiceUnknown"))
                message = tr("The update service isn't running.");
            else if (message.isEmpty())
                message = tr("The update service didn't accept the request.");
            emit actionFailed(message);
        }
        refresh();
    });
}

void UpdateClient::onStatusChanged(const QVariantMap &status)
{
    setAvailable(true);
    setStatus(status);
}

void UpdateClient::onUpdateAvailable(const QString &version, const QString &name)
{
    emit updateAvailable(version, name);
}
