#pragma once

#include <QAbstractListModel>
#include <QDateTime>
#include <QFileInfo>
#include <QtQml/qqmlregistration.h>

class QFileSystemWatcher;
class QTimer;

// Contents of one folder, sorted folders-first. Watches the folder and
// refreshes itself when files appear, change or disappear.
class DirModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString path READ path WRITE setPath NOTIFY pathChanged)
    Q_PROPERTY(bool showHidden READ showHidden WRITE setShowHidden NOTIFY showHiddenChanged)
    Q_PROPERTY(QString sortKey READ sortKey WRITE setSortKey NOTIFY sortChanged)          // name|size|modified|type
    Q_PROPERTY(bool sortDescending READ sortDescending WRITE setSortDescending NOTIFY sortChanged)
    Q_PROPERTY(QString filter READ filter WRITE setFilter NOTIFY filterChanged)
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    Q_PROPERTY(bool isTrash READ isTrash NOTIFY pathChanged)

public:
    enum Roles { NameRole = Qt::UserRole + 1, PathRole, IsDirRole, SizeRole, SizeTextRole, ModifiedRole,
                 ModifiedTextRole, TypeTextRole, IconRole, CategoryRole, HiddenRole, LinkRole, MimeRole };

    explicit DirModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString path() const { return m_path; }
    void setPath(const QString &p);
    bool showHidden() const { return m_showHidden; }
    void setShowHidden(bool v);
    QString sortKey() const { return m_sortKey; }
    void setSortKey(const QString &k);
    bool sortDescending() const { return m_desc; }
    void setSortDescending(bool d);
    QString filter() const { return m_filter; }
    void setFilter(const QString &f);
    QString error() const { return m_error; }
    bool isTrash() const;

    Q_INVOKABLE void reload();
    Q_INVOKABLE QString pathAt(int row) const;
    Q_INVOKABLE QVariantMap get(int row) const;
    Q_INVOKABLE int indexOfPath(const QString &path) const;
    Q_INVOKABLE double totalSize(const QStringList &paths) const;

    // Classifies a file for its icon and colour (folder, image, text, ...).
    static QString categoryFor(const QFileInfo &fi, const QString &mime);

signals:
    void pathChanged();
    void showHiddenChanged();
    void sortChanged();
    void filterChanged();
    void countChanged();
    void errorChanged();

private:
    struct Entry {
        QFileInfo info;
        QString mime, mimeComment, category;
    };
    void sortEntries();
    QList<Entry> m_all;      // everything in the folder (before filtering)
    QList<Entry> m_entries;  // what the view shows
    QString m_path;
    QString m_sortKey = QStringLiteral("name");
    QString m_filter;
    QString m_error;
    bool m_desc = false;
    bool m_showHidden = false;
    QFileSystemWatcher *m_watcher = nullptr;
    QTimer *m_reloadTimer = nullptr;
};
