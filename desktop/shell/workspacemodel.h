#pragma once

#include <QObject>
#include <QVariantList>
#include <QWaylandClientExtension>
#include <QtQml/qqmlregistration.h>

#include "qwayland-ext-workspace-v1.h"

// Wayland proxies are opaque; let Qt pass pointers to them in signals.
Q_DECLARE_OPAQUE_POINTER(struct ::ext_workspace_handle_v1 *)

class QQmlEngine;
class QJSEngine;

class WorkspaceHandle : public QObject, public QtWayland::ext_workspace_handle_v1
{
    Q_OBJECT
public:
    explicit WorkspaceHandle(struct ::ext_workspace_handle_v1 *object);
    ~WorkspaceHandle() override;
    QString name;
    bool active = false;
    int order = 0;
signals:
    void removed();
protected:
    void ext_workspace_handle_v1_name(const QString &name) override { this->name = name; }
    void ext_workspace_handle_v1_state(uint32_t state) override { active = state & state_active; }
    void ext_workspace_handle_v1_removed() override { emit removed(); }
};

class WorkspaceManager : public QWaylandClientExtensionTemplate<WorkspaceManager>,
                         public QtWayland::ext_workspace_manager_v1
{
    Q_OBJECT
public:
    WorkspaceManager();
signals:
    void workspace(struct ::ext_workspace_handle_v1 *handle);
    void done();
protected:
    void ext_workspace_manager_v1_workspace(struct ::ext_workspace_handle_v1 *w) override { emit workspace(w); }
    void ext_workspace_manager_v1_done() override { emit done(); }
};

// Virtual desktops from the compositor (ext-workspace-v1), exposed to QML as
// `Workspaces`: the list, the current one, and switching.
class WorkspaceModel : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Workspaces)
    QML_SINGLETON
    Q_PROPERTY(bool available READ available NOTIFY changed)
    Q_PROPERTY(QVariantList workspaces READ workspaces NOTIFY changed)
    Q_PROPERTY(int current READ current NOTIFY changed)
    Q_PROPERTY(int count READ count NOTIFY changed)

public:
    static WorkspaceModel *create(QQmlEngine *, QJSEngine *);

    bool available() const { return !m_items.isEmpty(); }
    QVariantList workspaces() const;
    int current() const;
    int count() const { return m_items.size(); }

    Q_INVOKABLE void activate(int index);
    Q_INVOKABLE void next();
    Q_INVOKABLE void previous();

signals:
    void changed();
    void switched(int index, const QString &name);

private:
    explicit WorkspaceModel(QObject *parent = nullptr);
    WorkspaceManager *m_manager = nullptr;
    QList<WorkspaceHandle *> m_items;
    int m_lastCurrent = -1;
    int m_order = 0;
};
