#pragma once

#include "desktopentry.h"

#include <QAbstractListModel>
#include <QHash>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

class QFileSystemWatcher;
class QTimer;
class QQmlEngine;
class QJSEngine;

// All installed applications (from XDG .desktop files), sorted by name.
// Exposed to QML as the `Apps` singleton: a list model plus launching,
// lookup by Wayland app_id, MIME default handling and search.
class AppModel : public QAbstractListModel
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Apps)
    QML_SINGLETON
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    enum Roles { IdRole = Qt::UserRole + 1, NameRole, GenericNameRole, CommentRole,
                 IconRole, CategoriesRole, TerminalRole };

    static AppModel *instance();
    static AppModel *create(QQmlEngine *, QJSEngine *);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Launching ----------------------------------------------------------
    Q_INVOKABLE bool launch(const QString &id, const QStringList &files = {});
    Q_INVOKABLE bool runCommand(const QString &program, const QStringList &args = {},
                                const QString &workingDir = {});
    Q_INVOKABLE bool openTerminal(const QString &workingDir = {});
    // Opens a file or folder with its default application.
    Q_INVOKABLE bool openFile(const QString &path);
    Q_INVOKABLE bool openFileWith(const QString &id, const QString &path);

    // Lookup ---------------------------------------------------------------
    Q_INVOKABLE QVariantMap get(const QString &id) const;
    Q_INVOKABLE bool exists(const QString &id) const { return m_byId.contains(id); }
    // Maps a Wayland app_id (e.g. "foot", "org.gnome.Calculator") to a desktop id.
    Q_INVOKABLE QString idForAppId(const QString &appId) const;
    Q_INVOKABLE QVariantList search(const QString &text, int limit = 24) const;
    Q_INVOKABLE QVariantList all() const;

    // MIME handling -------------------------------------------------------
    Q_INVOKABLE QString mimeTypeFor(const QString &path) const;
    Q_INVOKABLE QString mimeDescription(const QString &path) const;
    Q_INVOKABLE QVariantList appsForFile(const QString &path) const;
    Q_INVOKABLE QString defaultAppForFile(const QString &path) const;
    Q_INVOKABLE void setDefaultApp(const QString &mimeType, const QString &id);

    // XDG autostart (called once by the shell at session start).
    Q_INVOKABLE void runAutostart();

    static QStringList applicationDirs();
    QList<DesktopEntry> entries() const { return m_entries; }

signals:
    void countChanged();
    void launched(const QString &id);
    void launchFailed(const QString &name, const QString &reason);

public slots:
    void reload();

private:
    explicit AppModel(QObject *parent = nullptr);
    bool startEntry(const DesktopEntry &e, const QStringList &files);
    QVariantMap toMap(const DesktopEntry &e) const;
    QStringList defaultsFor(const QString &mime) const;

    QList<DesktopEntry> m_entries;          // visible apps, sorted
    QHash<QString, DesktopEntry> m_byId;    // all apps incl. NoDisplay ones
    QFileSystemWatcher *m_watcher = nullptr;
    QTimer *m_reloadTimer = nullptr;
};
