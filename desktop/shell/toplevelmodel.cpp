#include "toplevelmodel.h"
#include "appmodel.h"

#include <QGuiApplication>
#include <QQmlEngine>

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
#include <QtGui/qguiapplication_platform.h>
#else
#include <qpa/qplatformnativeinterface.h>
#endif

static wl_seat *waylandSeat()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    if (auto *app = qGuiApp->nativeInterface<QNativeInterface::QWaylandApplication>())
        return app->seat();
    return nullptr;
#else
    return static_cast<wl_seat *>(
        QGuiApplication::platformNativeInterface()->nativeResourceForIntegration("wl_seat"));
#endif
}

// ---- handle -----------------------------------------------------------------

ToplevelHandle::ToplevelHandle(struct ::zwlr_foreign_toplevel_handle_v1 *object, int id_)
    : QtWayland::zwlr_foreign_toplevel_handle_v1(object), id(id_)
{
}

ToplevelHandle::~ToplevelHandle()
{
    if (isInitialized())
        destroy();
}

void ToplevelHandle::zwlr_foreign_toplevel_handle_v1_title(const QString &t) { m_title = t; }
void ToplevelHandle::zwlr_foreign_toplevel_handle_v1_app_id(const QString &a) { m_appId = a; }

void ToplevelHandle::zwlr_foreign_toplevel_handle_v1_state(wl_array *state)
{
    m_activated = m_minimized = m_maximized = m_fullscreen = false;
    const auto *s = static_cast<const uint32_t *>(state->data);
    const size_t n = state->size / sizeof(uint32_t);
    for (size_t i = 0; i < n; ++i) {
        switch (s[i]) {
        case state_maximized: m_maximized = true; break;
        case state_minimized: m_minimized = true; break;
        case state_activated: m_activated = true; break;
        case state_fullscreen: m_fullscreen = true; break;
        }
    }
}

void ToplevelHandle::zwlr_foreign_toplevel_handle_v1_done()
{
    title = m_title;
    appId = m_appId;
    activated = m_activated;
    minimized = m_minimized;
    maximized = m_maximized;
    fullscreen = m_fullscreen;
    ready = true;
    emit changed();
}

void ToplevelHandle::zwlr_foreign_toplevel_handle_v1_closed()
{
    emit closed();
}

// ---- manager ----------------------------------------------------------------

ToplevelManager::ToplevelManager()
    : QWaylandClientExtensionTemplate<ToplevelManager>(3)
{
    initialize();
}

void ToplevelManager::zwlr_foreign_toplevel_manager_v1_toplevel(struct ::zwlr_foreign_toplevel_handle_v1 *t)
{
    emit toplevel(t);
}

// ---- model ------------------------------------------------------------------

ToplevelModel *ToplevelModel::create(QQmlEngine *, QJSEngine *)
{
    static ToplevelModel *m = new ToplevelModel();
    QQmlEngine::setObjectOwnership(m, QQmlEngine::CppOwnership);
    return m;
}

ToplevelModel::ToplevelModel(QObject *parent)
    : QAbstractListModel(parent)
{
    m_manager = new ToplevelManager();
    m_manager->setParent(this);
    connect(m_manager, &ToplevelManager::toplevel, this, &ToplevelModel::add);
    connect(m_manager, &QWaylandClientExtension::activeChanged, this, &ToplevelModel::availableChanged);
    connect(AppModel::instance(), &AppModel::countChanged, this, &ToplevelModel::bump);
}

bool ToplevelModel::available() const
{
    return m_manager && m_manager->isActive();
}

void ToplevelModel::bump()
{
    ++m_revision;
    emit revisionChanged();
}

void ToplevelModel::add(struct ::zwlr_foreign_toplevel_handle_v1 *handle)
{
    auto *h = new ToplevelHandle(handle, m_nextId++);
    h->setParent(this);
    m_pending << h;

    connect(h, &ToplevelHandle::changed, this, [this, h] {
        if (m_pending.removeOne(h)) {
            beginInsertRows({}, m_items.size(), m_items.size());
            m_items << h;
            endInsertRows();
            emit countChanged();
        } else {
            const int r = rowOf(h);
            if (r >= 0)
                emit dataChanged(index(r), index(r));
        }
        emit activeChanged();
        bump();
    });
    connect(h, &ToplevelHandle::closed, this, [this, h] {
        m_pending.removeOne(h);
        m_desktopStash.removeAll(h->id);
        const int r = rowOf(h);
        if (r >= 0) {
            beginRemoveRows({}, r, r);
            m_items.removeAt(r);
            endRemoveRows();
            emit countChanged();
            emit activeChanged();
            bump();
        }
        h->deleteLater();
    });
}

int ToplevelModel::rowOf(ToplevelHandle *h) const { return m_items.indexOf(h); }

ToplevelHandle *ToplevelModel::byId(int id) const
{
    for (ToplevelHandle *h : m_items)
        if (h->id == id)
            return h;
    return nullptr;
}

int ToplevelModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_items.size();
}

QString ToplevelModel::desktopIdFor(const QString &appId) const
{
    return AppModel::instance()->idForAppId(appId);
}

QVariantMap ToplevelModel::toMap(const ToplevelHandle *h) const
{
    const QString desktopId = desktopIdFor(h->appId);
    const QVariantMap app = desktopId.isEmpty() ? QVariantMap() : AppModel::instance()->get(desktopId);
    QString name = app.value(QStringLiteral("name")).toString();
    if (name.isEmpty()) name = h->title.isEmpty() ? h->appId : h->title;
    QString icon = app.value(QStringLiteral("iconName")).toString();
    if (icon.isEmpty()) icon = h->appId.isEmpty() ? QStringLiteral("meridian-app") : h->appId;
    return {{QStringLiteral("id"), h->id},
            {QStringLiteral("title"), h->title},
            {QStringLiteral("appId"), h->appId},
            {QStringLiteral("desktopId"), desktopId},
            {QStringLiteral("name"), name},
            {QStringLiteral("iconName"), icon},
            {QStringLiteral("activated"), h->activated},
            {QStringLiteral("minimized"), h->minimized},
            {QStringLiteral("maximized"), h->maximized},
            {QStringLiteral("fullscreen"), h->fullscreen}};
}

QVariant ToplevelModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_items.size())
        return {};
    const ToplevelHandle *h = m_items.at(index.row());
    switch (role) {
    case IdRole: return h->id;
    case Qt::DisplayRole:
    case TitleRole: return h->title;
    case AppIdRole: return h->appId;
    case DesktopIdRole: return desktopIdFor(h->appId);
    case ActivatedRole: return h->activated;
    case MinimizedRole: return h->minimized;
    case MaximizedRole: return h->maximized;
    }
    return {};
}

QHash<int, QByteArray> ToplevelModel::roleNames() const
{
    return {{IdRole, "windowId"}, {TitleRole, "title"}, {AppIdRole, "appId"},
            {DesktopIdRole, "desktopId"}, {ActivatedRole, "activated"},
            {MinimizedRole, "minimized"}, {MaximizedRole, "maximized"}};
}

QVariantList ToplevelModel::windows() const
{
    QVariantList out;
    for (const ToplevelHandle *h : m_items)
        out << toMap(h);
    return out;
}

int ToplevelModel::activeId() const
{
    for (const ToplevelHandle *h : m_items)
        if (h->activated)
            return h->id;
    return 0;
}

bool ToplevelModel::fullscreenActive() const
{
    for (const ToplevelHandle *h : m_items)
        if (h->activated && h->fullscreen)
            return true;
    return false;
}

void ToplevelModel::activate(int id)
{
    ToplevelHandle *h = byId(id);
    wl_seat *seat = waylandSeat();
    if (!h || !seat)
        return;
    if (h->minimized)
        h->unset_minimized();
    h->QtWayland::zwlr_foreign_toplevel_handle_v1::activate(seat);
    m_desktopStash.clear();
}

void ToplevelModel::minimize(int id)
{
    if (ToplevelHandle *h = byId(id))
        h->set_minimized();
}

void ToplevelModel::toggle(int id)
{
    ToplevelHandle *h = byId(id);
    if (!h)
        return;
    if (h->activated && !h->minimized)
        h->set_minimized();
    else
        activate(id);
}

void ToplevelModel::close(int id)
{
    if (ToplevelHandle *h = byId(id))
        h->QtWayland::zwlr_foreign_toplevel_handle_v1::close();
}

void ToplevelModel::setMaximized(int id, bool on)
{
    if (ToplevelHandle *h = byId(id)) {
        if (on) h->set_maximized();
        else h->unset_maximized();
    }
}

void ToplevelModel::showDesktop()
{
    QList<ToplevelHandle *> visible;
    for (ToplevelHandle *h : m_items)
        if (!h->minimized)
            visible << h;
    if (!visible.isEmpty()) {
        m_desktopStash.clear();
        for (ToplevelHandle *h : visible) {
            m_desktopStash << h->id;
            h->set_minimized();
        }
        return;
    }
    // Nothing visible: bring back what we hid, last one focused.
    const QList<int> stash = m_desktopStash;
    m_desktopStash.clear();
    for (int id : stash)
        if (ToplevelHandle *h = byId(id))
            h->unset_minimized();
    if (!stash.isEmpty())
        activate(stash.last());
}
