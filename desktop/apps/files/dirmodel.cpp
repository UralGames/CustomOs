#include "dirmodel.h"

#include <QCollator>
#include <QDir>
#include <QFileSystemWatcher>
#include <QLocale>
#include <QMimeDatabase>
#include <QStandardPaths>
#include <QTimer>

static QString formatSize(qint64 bytes)
{
    static const char *units[] = {"bytes", "KB", "MB", "GB", "TB"};
    double v = double(bytes);
    int u = 0;
    while (v >= 1024.0 && u < 4) { v /= 1024.0; ++u; }
    if (u == 0)
        return QStringLiteral("%1 %2").arg(bytes).arg(QLatin1String(bytes == 1 ? "byte" : "bytes"));
    return QStringLiteral("%1 %2").arg(v, 0, 'f', v < 10 ? 1 : 0).arg(QLatin1String(units[u]));
}

DirModel::DirModel(QObject *parent)
    : QAbstractListModel(parent)
{
    m_watcher = new QFileSystemWatcher(this);
    m_reloadTimer = new QTimer(this);
    m_reloadTimer->setSingleShot(true);
    m_reloadTimer->setInterval(150);
    connect(m_reloadTimer, &QTimer::timeout, this, &DirModel::reload);
    connect(m_watcher, &QFileSystemWatcher::directoryChanged, m_reloadTimer, qOverload<>(&QTimer::start));
}

bool DirModel::isTrash() const
{
    const QString trash = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
                          + QStringLiteral("/Trash/files");
    return QDir::cleanPath(m_path) == QDir::cleanPath(trash);
}

QString DirModel::categoryFor(const QFileInfo &fi, const QString &mime)
{
    if (fi.isDir()) return QStringLiteral("folder");
    if (mime.startsWith(QLatin1String("image/"))) return QStringLiteral("image");
    if (mime.startsWith(QLatin1String("audio/"))) return QStringLiteral("audio");
    if (mime.startsWith(QLatin1String("video/"))) return QStringLiteral("video");
    static const QStringList archives{QStringLiteral("application/zip"), QStringLiteral("application/x-tar"),
        QStringLiteral("application/gzip"), QStringLiteral("application/x-xz"), QStringLiteral("application/x-bzip2"),
        QStringLiteral("application/x-7z-compressed"), QStringLiteral("application/zstd"),
        QStringLiteral("application/x-compressed-tar"), QStringLiteral("application/x-xz-compressed-tar"),
        QStringLiteral("application/vnd.rar"), QStringLiteral("application/x-iso9660-image")};
    if (archives.contains(mime)) return QStringLiteral("archive");
    if (mime == QLatin1String("application/x-executable") || mime == QLatin1String("application/x-sharedlib")
        || mime == QLatin1String("application/x-pie-executable"))
        return QStringLiteral("exec");
    static const QStringList code{QStringLiteral("text/x-c"), QStringLiteral("text/x-csrc"), QStringLiteral("text/x-chdr"),
        QStringLiteral("text/x-c++src"), QStringLiteral("text/x-python"), QStringLiteral("application/x-shellscript"),
        QStringLiteral("text/x-shellscript"), QStringLiteral("application/json"), QStringLiteral("application/xml"),
        QStringLiteral("text/html"), QStringLiteral("text/css"), QStringLiteral("application/javascript"),
        QStringLiteral("text/x-qml"), QStringLiteral("text/x-cmake"), QStringLiteral("text/x-makefile")};
    if (code.contains(mime)) return QStringLiteral("code");
    if (mime.startsWith(QLatin1String("text/"))) return QStringLiteral("text");
    if (mime == QLatin1String("application/pdf")) return QStringLiteral("document");
    return QStringLiteral("other");
}

void DirModel::setPath(const QString &p)
{
    QString clean = QDir::cleanPath(p.isEmpty() ? QDir::homePath() : p);
    if (clean.startsWith(QLatin1String("~")))
        clean.replace(0, 1, QDir::homePath());
    if (clean == m_path)
        return;
    if (!m_watcher->directories().isEmpty())
        m_watcher->removePaths(m_watcher->directories());
    m_path = clean;
    m_filter.clear();
    emit filterChanged();
    emit pathChanged();
    reload();
    if (QFileInfo(m_path).isDir())
        m_watcher->addPath(m_path);
}

void DirModel::setShowHidden(bool v)
{
    if (v == m_showHidden) return;
    m_showHidden = v;
    emit showHiddenChanged();
    reload();
}

void DirModel::setSortKey(const QString &k)
{
    if (k == m_sortKey) return;
    m_sortKey = k;
    emit sortChanged();
    beginResetModel();
    sortEntries();
    endResetModel();
}

void DirModel::setSortDescending(bool d)
{
    if (d == m_desc) return;
    m_desc = d;
    emit sortChanged();
    beginResetModel();
    sortEntries();
    endResetModel();
}

void DirModel::setFilter(const QString &f)
{
    if (f == m_filter) return;
    m_filter = f;
    emit filterChanged();
    beginResetModel();
    sortEntries();
    endResetModel();
    emit countChanged();
}

void DirModel::reload()
{
    QDir dir(m_path);
    QString err;
    QList<Entry> all;
    if (!dir.exists()) {
        err = tr("This folder does not exist.");
    } else if (!QFileInfo(m_path).isReadable()) {
        err = tr("You don't have permission to view this folder.");
    } else {
        QDir::Filters f = QDir::AllEntries | QDir::NoDotAndDotDot | QDir::System;
        if (m_showHidden) f |= QDir::Hidden;
        static QMimeDatabase db;
        const QFileInfoList list = dir.entryInfoList(f, QDir::NoSort);
        all.reserve(list.size());
        for (const QFileInfo &fi : list) {
            Entry e;
            e.info = fi;
            if (fi.isDir()) {
                e.mime = QStringLiteral("inode/directory");
                e.mimeComment = tr("Folder");
            } else {
                const QMimeType mt = db.mimeTypeForFile(fi, QMimeDatabase::MatchExtension);
                e.mime = mt.name();
                e.mimeComment = mt.comment();
            }
            e.category = categoryFor(fi, e.mime);
            all << e;
        }
    }
    beginResetModel();
    m_all = all;
    sortEntries();
    endResetModel();
    emit countChanged();
    if (err != m_error) {
        m_error = err;
        emit errorChanged();
    }
}

void DirModel::sortEntries()
{
    m_entries.clear();
    for (const Entry &e : std::as_const(m_all))
        if (m_filter.isEmpty() || e.info.fileName().contains(m_filter, Qt::CaseInsensitive))
            m_entries << e;

    QCollator coll;
    coll.setNumericMode(true);
    coll.setCaseSensitivity(Qt::CaseInsensitive);
    const QString key = m_sortKey;
    const bool desc = m_desc;
    std::stable_sort(m_entries.begin(), m_entries.end(), [&](const Entry &a, const Entry &b) {
        if (a.info.isDir() != b.info.isDir())
            return a.info.isDir();   // folders first, regardless of direction
        int c = 0;
        if (key == QLatin1String("size"))
            c = a.info.size() < b.info.size() ? -1 : a.info.size() > b.info.size() ? 1 : 0;
        else if (key == QLatin1String("modified"))
            c = a.info.lastModified() < b.info.lastModified() ? -1 : a.info.lastModified() > b.info.lastModified() ? 1 : 0;
        else if (key == QLatin1String("type"))
            c = coll.compare(a.mimeComment, b.mimeComment);
        if (c == 0)
            c = coll.compare(a.info.fileName(), b.info.fileName());
        return desc ? c > 0 : c < 0;
    });
}

int DirModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_entries.size();
}

QVariant DirModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_entries.size())
        return {};
    const Entry &e = m_entries.at(index.row());
    const QFileInfo &fi = e.info;
    switch (role) {
    case Qt::DisplayRole:
    case NameRole: return fi.fileName();
    case PathRole: return fi.absoluteFilePath();
    case IsDirRole: return fi.isDir();
    case SizeRole: return double(fi.isDir() ? 0 : fi.size());
    case SizeTextRole: return fi.isDir() ? QString() : formatSize(fi.size());
    case ModifiedRole: return fi.lastModified();
    case ModifiedTextRole: return QLocale().toString(fi.lastModified(), QLocale::ShortFormat);
    case TypeTextRole: return e.mimeComment;
    case CategoryRole: return e.category;
    case IconRole: {
        static const QHash<QString, QString> icons{
            {QStringLiteral("folder"), QStringLiteral("folder")}, {QStringLiteral("image"), QStringLiteral("image")},
            {QStringLiteral("audio"), QStringLiteral("music")}, {QStringLiteral("video"), QStringLiteral("videos")},
            {QStringLiteral("archive"), QStringLiteral("harddisk")}, {QStringLiteral("exec"), QStringLiteral("terminal")},
            {QStringLiteral("code"), QStringLiteral("code")}, {QStringLiteral("text"), QStringLiteral("file-text")},
            {QStringLiteral("document"), QStringLiteral("documents")}};
        return icons.value(e.category, QStringLiteral("file"));
    }
    case HiddenRole: return fi.isHidden();
    case LinkRole: return fi.isSymLink();
    case MimeRole: return e.mime;
    }
    return {};
}

QHash<int, QByteArray> DirModel::roleNames() const
{
    return {{NameRole, "fileName"}, {PathRole, "filePath"}, {IsDirRole, "isDir"}, {SizeRole, "fileSize"},
            {SizeTextRole, "sizeText"}, {ModifiedRole, "modified"}, {ModifiedTextRole, "modifiedText"},
            {TypeTextRole, "typeText"}, {IconRole, "iconName"}, {CategoryRole, "category"},
            {HiddenRole, "isHidden"}, {LinkRole, "isLink"}, {MimeRole, "mimeType"}};
}

QString DirModel::pathAt(int row) const
{
    return row >= 0 && row < m_entries.size() ? m_entries.at(row).info.absoluteFilePath() : QString();
}

QVariantMap DirModel::get(int row) const
{
    QVariantMap m;
    if (row < 0 || row >= m_entries.size())
        return m;
    const QHash<int, QByteArray> roles = roleNames();
    for (auto it = roles.cbegin(); it != roles.cend(); ++it)
        m.insert(QString::fromLatin1(it.value()), data(index(row), it.key()));
    return m;
}

int DirModel::indexOfPath(const QString &path) const
{
    for (int i = 0; i < m_entries.size(); ++i)
        if (m_entries.at(i).info.absoluteFilePath() == path)
            return i;
    return -1;
}

double DirModel::totalSize(const QStringList &paths) const
{
    double total = 0;
    for (const QString &p : paths) {
        const QFileInfo fi(p);
        if (fi.isFile()) total += double(fi.size());
    }
    return total;
}
