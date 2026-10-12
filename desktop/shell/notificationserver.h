#pragma once

#include <QAbstractListModel>
#include <QDateTime>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

class QQmlEngine;
class QJSEngine;

// Implements the freedesktop.org Desktop Notifications service
// (org.freedesktop.Notifications on the session bus), so any Linux
// application's notifications (notify-send, libnotify, Qt, GTK) appear in
// the shell. Exposed to QML as the `Notifications` singleton list model.
class NotificationServer : public QAbstractListModel
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Notifications)
    QML_SINGLETON
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(bool registered READ registered CONSTANT)

public:
    struct Item {
        uint id = 0;
        QString appName, appIcon, summary, body, desktopEntry;
        QStringList actions;   // key, label, key, label, ...
        int urgency = 1;
        bool resident = false;
        QDateTime time;
    };
    enum Roles { IdRole = Qt::UserRole + 1, AppNameRole, IconRole, SummaryRole, BodyRole,
                 ActionsRole, UrgencyRole, TimeRole };

    static NotificationServer *instance();
    static NotificationServer *create(QQmlEngine *, QJSEngine *);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    int count() const { return m_items.size(); }
    bool registered() const { return m_registered; }

    // Called from D-Bus and from the shell itself.
    uint notify(const QString &appName, uint replacesId, const QString &appIcon,
                const QString &summary, const QString &body, const QStringList &actions,
                const QVariantMap &hints, int expireTimeout);

    Q_INVOKABLE uint post(const QString &appName, const QString &summary, const QString &body,
                          const QString &icon = {});
    Q_INVOKABLE void dismiss(uint id);           // user closed it (reason 2)
    Q_INVOKABLE void clearAll();
    Q_INVOKABLE void invokeAction(uint id, const QString &key);
    Q_INVOKABLE void expire(uint id);            // toast timed out; keep it in the centre
    void closeById(uint id, uint reason);

signals:
    void countChanged();
    // A new or replaced notification that should pop up as a toast.
    void toast(const QVariantMap &notification);
    void notificationClosed(uint id, uint reason);
    void actionInvoked(uint id, const QString &actionKey);

private:
    explicit NotificationServer(QObject *parent = nullptr);
    QVariantMap toMap(const Item &it) const;
    int indexOf(uint id) const;

    QList<Item> m_items;   // newest first
    uint m_nextId = 1;
    bool m_registered = false;
};
