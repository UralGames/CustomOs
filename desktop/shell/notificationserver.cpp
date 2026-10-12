#include "notificationserver.h"
#include "settings.h"

#include <QDBusAbstractAdaptor>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QQmlEngine>
#include <QRegularExpression>

namespace {
NotificationServer *s_instance = nullptr;
}

// D-Bus face of the server. Kept separate so only the specification's
// methods are exported.
class NotificationsAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.Notifications")

public:
    explicit NotificationsAdaptor(NotificationServer *server)
        : QDBusAbstractAdaptor(server), m_server(server)
    {
        connect(server, &NotificationServer::notificationClosed, this, &NotificationsAdaptor::NotificationClosed);
        connect(server, &NotificationServer::actionInvoked, this, &NotificationsAdaptor::ActionInvoked);
    }

public slots:
    QStringList GetCapabilities()
    {
        return {QStringLiteral("body"), QStringLiteral("actions"), QStringLiteral("persistence"),
                QStringLiteral("icon-static")};
    }
    uint Notify(const QString &app_name, uint replaces_id, const QString &app_icon,
                const QString &summary, const QString &body, const QStringList &actions,
                const QVariantMap &hints, int expire_timeout)
    {
        return m_server->notify(app_name, replaces_id, app_icon, summary, body, actions, hints, expire_timeout);
    }
    void CloseNotification(uint id) { m_server->closeById(id, 3); }
    QString GetServerInformation(QString &vendor, QString &version, QString &spec_version)
    {
        vendor = QStringLiteral("Meridian");
        version = QStringLiteral(MERIDIAN_VERSION);
        spec_version = QStringLiteral("1.2");
        return QStringLiteral("Meridian Shell");
    }

signals:
    void NotificationClosed(uint id, uint reason);
    void ActionInvoked(uint id, const QString &action_key);

private:
    NotificationServer *m_server;
};

NotificationServer *NotificationServer::instance()
{
    if (!s_instance)
        s_instance = new NotificationServer();
    return s_instance;
}

NotificationServer *NotificationServer::create(QQmlEngine *, QJSEngine *)
{
    NotificationServer *n = instance();
    QQmlEngine::setObjectOwnership(n, QQmlEngine::CppOwnership);
    return n;
}

NotificationServer::NotificationServer(QObject *parent)
    : QAbstractListModel(parent)
{
    new NotificationsAdaptor(this);
    QDBusConnection bus = QDBusConnection::sessionBus();
    if (bus.isConnected()) {
        const bool obj = bus.registerObject(QStringLiteral("/org/freedesktop/Notifications"), this);
        const auto reply = bus.interface()->registerService(
            QStringLiteral("org.freedesktop.Notifications"),
            QDBusConnectionInterface::ReplaceExistingService,
            QDBusConnectionInterface::AllowReplacement);
        m_registered = obj && reply.isValid() && reply.value() == QDBusConnectionInterface::ServiceRegistered;
    }
    if (!m_registered)
        qWarning("meridian-shell: could not register org.freedesktop.Notifications on the session bus");
}

int NotificationServer::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_items.size();
}

QVariant NotificationServer::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_items.size())
        return {};
    const Item &it = m_items.at(index.row());
    switch (role) {
    case IdRole: return it.id;
    case AppNameRole: return it.appName;
    case IconRole: return it.appIcon;
    case SummaryRole: return it.summary;
    case BodyRole: return it.body;
    case ActionsRole: return toMap(it).value(QStringLiteral("actions"));
    case UrgencyRole: return it.urgency;
    case TimeRole: return it.time;
    }
    return {};
}

QHash<int, QByteArray> NotificationServer::roleNames() const
{
    return {{IdRole, "notificationId"}, {AppNameRole, "appName"}, {IconRole, "appIcon"},
            {SummaryRole, "summary"}, {BodyRole, "body"}, {ActionsRole, "actions"},
            {UrgencyRole, "urgency"}, {TimeRole, "time"}};
}

QVariantMap NotificationServer::toMap(const Item &it) const
{
    QVariantList actions;
    for (int i = 0; i + 1 < it.actions.size(); i += 2) {
        if (it.actions.at(i) == QLatin1String("default"))
            continue;   // the default action is invoked by clicking the toast
        actions << QVariantMap{{QStringLiteral("key"), it.actions.at(i)},
                               {QStringLiteral("label"), it.actions.at(i + 1)}};
    }
    return {{QStringLiteral("notificationId"), it.id},
            {QStringLiteral("appName"), it.appName},
            {QStringLiteral("appIcon"), it.appIcon},
            {QStringLiteral("summary"), it.summary},
            {QStringLiteral("body"), it.body},
            {QStringLiteral("actions"), actions},
            {QStringLiteral("hasDefault"), it.actions.contains(QStringLiteral("default"))},
            {QStringLiteral("urgency"), it.urgency},
            {QStringLiteral("time"), it.time}};
}

int NotificationServer::indexOf(uint id) const
{
    for (int i = 0; i < m_items.size(); ++i)
        if (m_items.at(i).id == id)
            return i;
    return -1;
}

static QString plainBody(QString body)
{
    // The spec allows a small markup subset; keep it readable as plain text.
    body.replace(QStringLiteral("<br/>"), QStringLiteral("\n"));
    body.replace(QStringLiteral("<br>"), QStringLiteral("\n"));
    body.remove(QRegularExpression(QStringLiteral("<[^>]*>")));
    body.replace(QStringLiteral("&lt;"), QStringLiteral("<"));
    body.replace(QStringLiteral("&gt;"), QStringLiteral(">"));
    body.replace(QStringLiteral("&quot;"), QStringLiteral("\""));
    body.replace(QStringLiteral("&apos;"), QStringLiteral("'"));
    body.replace(QStringLiteral("&amp;"), QStringLiteral("&"));
    return body;
}

uint NotificationServer::notify(const QString &appName, uint replacesId, const QString &appIcon,
                                const QString &summary, const QString &body, const QStringList &actions,
                                const QVariantMap &hints, int /*expireTimeout*/)
{
    Item it;
    it.appName = appName.isEmpty() ? tr("Application") : appName;
    it.appIcon = appIcon;
    if (it.appIcon.startsWith(QLatin1String("file://")))
        it.appIcon = it.appIcon.mid(7);
    it.desktopEntry = hints.value(QStringLiteral("desktop-entry")).toString();
    if (it.appIcon.isEmpty())
        it.appIcon = hints.value(QStringLiteral("image-path")).toString();
    if (it.appIcon.isEmpty() && !it.desktopEntry.isEmpty())
        it.appIcon = it.desktopEntry;
    it.summary = summary;
    it.body = plainBody(body);
    it.actions = actions;
    it.urgency = hints.value(QStringLiteral("urgency"), 1).toInt();
    it.resident = hints.value(QStringLiteral("resident")).toBool();
    it.time = QDateTime::currentDateTime();

    const int existing = replacesId ? indexOf(replacesId) : -1;
    if (existing >= 0) {
        it.id = replacesId;
        m_items[existing] = it;
        emit dataChanged(index(existing), index(existing));
    } else {
        it.id = m_nextId++;
        beginInsertRows({}, 0, 0);
        m_items.prepend(it);
        endInsertRows();
        emit countChanged();
        // Keep the centre bounded.
        if (m_items.size() > 100) {
            beginRemoveRows({}, m_items.size() - 1, m_items.size() - 1);
            m_items.removeLast();
            endRemoveRows();
            emit countChanged();
        }
    }

    const bool dnd = Settings::instance()->value(QStringLiteral("doNotDisturb")).toBool();
    if (!dnd || it.urgency >= 2)
        emit toast(toMap(it));
    return it.id;
}

uint NotificationServer::post(const QString &appName, const QString &summary, const QString &body,
                              const QString &icon)
{
    return notify(appName, 0, icon, summary, body, {}, {}, -1);
}

void NotificationServer::closeById(uint id, uint reason)
{
    const int i = indexOf(id);
    if (i < 0)
        return;
    beginRemoveRows({}, i, i);
    m_items.removeAt(i);
    endRemoveRows();
    emit countChanged();
    emit notificationClosed(id, reason);
}

void NotificationServer::dismiss(uint id) { closeById(id, 2); }

void NotificationServer::expire(uint id)
{
    // Toast timed out; the notification stays in the centre. Nothing to do
    // on the bus: the spec only requires NotificationClosed when it is gone.
    Q_UNUSED(id)
}

void NotificationServer::clearAll()
{
    const QList<Item> items = m_items;
    beginResetModel();
    m_items.clear();
    endResetModel();
    emit countChanged();
    for (const Item &it : items)
        emit notificationClosed(it.id, 2);
}

void NotificationServer::invokeAction(uint id, const QString &key)
{
    const int i = indexOf(id);
    if (i < 0)
        return;
    emit actionInvoked(id, key);
    if (!m_items.at(i).resident)
        closeById(id, 2);
}

#include "notificationserver.moc"
