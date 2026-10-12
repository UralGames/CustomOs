#include "workspacemodel.h"

#include <QQmlEngine>
#include <QVariantMap>

WorkspaceHandle::WorkspaceHandle(struct ::ext_workspace_handle_v1 *object)
    : QtWayland::ext_workspace_handle_v1(object)
{
}

WorkspaceHandle::~WorkspaceHandle()
{
    if (isInitialized())
        destroy();
}

WorkspaceManager::WorkspaceManager()
    : QWaylandClientExtensionTemplate<WorkspaceManager>(1)
{
    initialize();
}

WorkspaceModel *WorkspaceModel::create(QQmlEngine *, QJSEngine *)
{
    static WorkspaceModel *m = new WorkspaceModel();
    QQmlEngine::setObjectOwnership(m, QQmlEngine::CppOwnership);
    return m;
}

WorkspaceModel::WorkspaceModel(QObject *parent)
    : QObject(parent)
{
    m_manager = new WorkspaceManager();
    m_manager->setParent(this);
    connect(m_manager, &WorkspaceManager::workspace, this, [this](struct ::ext_workspace_handle_v1 *w) {
        auto *h = new WorkspaceHandle(w);
        h->setParent(this);
        h->order = m_order++;
        m_items << h;
        connect(h, &WorkspaceHandle::removed, this, [this, h] {
            m_items.removeOne(h);
            h->deleteLater();
            emit changed();
        });
    });
    // Every batch of changes ends with `done`.
    connect(m_manager, &WorkspaceManager::done, this, [this] {
        const int cur = current();
        emit changed();
        if (cur != m_lastCurrent && m_lastCurrent >= 0 && cur >= 0)
            emit switched(cur, m_items.at(cur)->name);
        m_lastCurrent = cur;
    });
}

QVariantList WorkspaceModel::workspaces() const
{
    QVariantList out;
    for (int i = 0; i < m_items.size(); ++i)
        out << QVariantMap{{QStringLiteral("index"), i},
                           {QStringLiteral("name"), m_items.at(i)->name},
                           {QStringLiteral("active"), m_items.at(i)->active}};
    return out;
}

int WorkspaceModel::current() const
{
    for (int i = 0; i < m_items.size(); ++i)
        if (m_items.at(i)->active)
            return i;
    return -1;
}

void WorkspaceModel::activate(int index)
{
    if (index < 0 || index >= m_items.size())
        return;
    m_items.at(index)->activate();
    m_manager->commit();
}

void WorkspaceModel::next()
{
    if (!m_items.isEmpty())
        activate((current() + 1) % m_items.size());
}

void WorkspaceModel::previous()
{
    if (!m_items.isEmpty())
        activate((current() - 1 + m_items.size()) % m_items.size());
}
