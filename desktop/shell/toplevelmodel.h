#pragma once

#include <QAbstractListModel>
#include <QPointer>
#include <QWaylandClientExtension>
#include <QtQml/qqmlregistration.h>

#include "qwayland-wlr-foreign-toplevel-management-unstable-v1.h"

// Wayland proxies are opaque; let Qt pass pointers to them in signals.
Q_DECLARE_OPAQUE_POINTER(struct ::zwlr_foreign_toplevel_handle_v1 *)

class QQmlEngine;
class QJSEngine;

// One window reported by the compositor.
class ToplevelHandle : public QObject, public QtWayland::zwlr_foreign_toplevel_handle_v1
{
    Q_OBJECT
public:
    explicit ToplevelHandle(struct ::zwlr_foreign_toplevel_handle_v1 *object, int id);
    ~ToplevelHandle() override;

    int id;
    QString title, appId;
    bool activated = false, minimized = false, maximized = false, fullscreen = false;
    bool ready = false;

signals:
    void changed();
    void closed();

protected:
    void zwlr_foreign_toplevel_handle_v1_title(const QString &title) override;
    void zwlr_foreign_toplevel_handle_v1_app_id(const QString &app_id) override;
    void zwlr_foreign_toplevel_handle_v1_state(wl_array *state) override;
    void zwlr_foreign_toplevel_handle_v1_done() override;
    void zwlr_foreign_toplevel_handle_v1_closed() override;

private:
    QString m_title, m_appId;
    bool m_activated = false, m_minimized = false, m_maximized = false, m_fullscreen = false;
};

class ToplevelManager : public QWaylandClientExtensionTemplate<ToplevelManager>,
                        public QtWayland::zwlr_foreign_toplevel_manager_v1
{
    Q_OBJECT
public:
    ToplevelManager();
signals:
    void toplevel(struct ::zwlr_foreign_toplevel_handle_v1 *handle);
protected:
    void zwlr_foreign_toplevel_manager_v1_toplevel(struct ::zwlr_foreign_toplevel_handle_v1 *toplevel) override;
};

// The compositor's window list (wlr-foreign-toplevel-management), exposed to
// QML as `Toplevels`. Powers taskbar buttons, running indicators, task view
// and "show desktop".
class ToplevelModel : public QAbstractListModel
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Toplevels)
    QML_SINGLETON
    Q_PROPERTY(bool available READ available NOTIFY availableChanged)
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(int activeId READ activeId NOTIFY activeChanged)
    Q_PROPERTY(bool fullscreenActive READ fullscreenActive NOTIFY activeChanged)
    // Bumped on any change, so QML bindings over windows() re-evaluate.
    Q_PROPERTY(int revision READ revision NOTIFY revisionChanged)

public:
    enum Roles { IdRole = Qt::UserRole + 1, TitleRole, AppIdRole, DesktopIdRole, ActivatedRole,
                 MinimizedRole, MaximizedRole };

    static ToplevelModel *create(QQmlEngine *, QJSEngine *);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    bool available() const;
    int activeId() const;
    bool fullscreenActive() const;
    int revision() const { return m_revision; }

    Q_INVOKABLE QVariantList windows() const;
    Q_INVOKABLE void activate(int id);
    Q_INVOKABLE void minimize(int id);
    Q_INVOKABLE void toggle(int id);          // focus, or minimise if already focused
    Q_INVOKABLE void close(int id);
    Q_INVOKABLE void setMaximized(int id, bool on);
    Q_INVOKABLE void showDesktop();           // minimise all; again to restore
    Q_INVOKABLE QString desktopIdFor(const QString &appId) const;

signals:
    void availableChanged();
    void countChanged();
    void activeChanged();
    void revisionChanged();

private:
    explicit ToplevelModel(QObject *parent = nullptr);
    void add(struct ::zwlr_foreign_toplevel_handle_v1 *handle);
    ToplevelHandle *byId(int id) const;
    int rowOf(ToplevelHandle *h) const;
    QVariantMap toMap(const ToplevelHandle *h) const;
    void bump();

    ToplevelManager *m_manager = nullptr;
    QList<ToplevelHandle *> m_items;   // only windows that have sent `done`
    QList<ToplevelHandle *> m_pending;
    QList<int> m_desktopStash;         // windows minimised by showDesktop()
    int m_nextId = 1;
    int m_revision = 0;
};
