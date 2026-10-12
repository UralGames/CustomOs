#include "fileops.h"

#include <QClipboard>
#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QLocale>
#include <QMimeData>
#include <QMimeDatabase>
#include <QPointer>
#include <QQmlEngine>
#include <QSettings>
#include <QStandardPaths>
#include <QThread>
#include <QUrl>

#include <grp.h>
#include <pwd.h>
#include <sys/stat.h>

namespace {

QString trashBase()
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/Trash");
}

QString uniqueIn(const QString &dir, const QString &name)
{
    QDir d(dir);
    if (!d.exists(name))
        return d.filePath(name);
    QFileInfo fi(name);
    // Keep compound suffixes like .tar.gz together.
    QString base = fi.completeBaseName(), suffix = fi.suffix();
    if (name.startsWith(QLatin1Char('.')) && base.isEmpty()) { base = name; suffix.clear(); }
    if (suffix.isEmpty() || QFileInfo(d.filePath(name)).isDir()) { base = name; suffix.clear(); }
    for (int i = 2; i < 10000; ++i) {
        const QString candidate = suffix.isEmpty() ? QStringLiteral("%1 (%2)").arg(base).arg(i)
                                                   : QStringLiteral("%1 (%2).%3").arg(base).arg(i).arg(suffix);
        if (!d.exists(candidate))
            return d.filePath(candidate);
    }
    return d.filePath(name + QStringLiteral(" (copy)"));
}

// Recursive copy. Returns an error message or "".
QString copyTree(const QString &src, const QString &dst)
{
    const QFileInfo fi(src);
    if (fi.isSymLink()) {
        if (!QFile::link(fi.symLinkTarget(), dst))
            return QObject::tr("Could not copy the link \"%1\".").arg(fi.fileName());
        return {};
    }
    if (fi.isDir()) {
        if (!QDir().mkpath(dst))
            return QObject::tr("Could not create the folder \"%1\".").arg(QFileInfo(dst).fileName());
        QFile::setPermissions(dst, QFile::permissions(src));
        const QFileInfoList children = QDir(src).entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System);
        for (const QFileInfo &c : children) {
            const QString err = copyTree(c.absoluteFilePath(), dst + QLatin1Char('/') + c.fileName());
            if (!err.isEmpty())
                return err;
        }
        return {};
    }
    if (!QFile::copy(src, dst))
        return QObject::tr("Could not copy \"%1\".").arg(fi.fileName());
    return {};
}

QString removeTree(const QString &path)
{
    const QFileInfo fi(path);
    if (fi.isDir() && !fi.isSymLink()) {
        if (!QDir(path).removeRecursively())
            return QObject::tr("Could not delete \"%1\".").arg(fi.fileName());
    } else if (!QFile::remove(path)) {
        return QObject::tr("Could not delete \"%1\".").arg(fi.fileName());
    }
    return {};
}

QString moveTo(const QString &src, const QString &dst)
{
    if (QDir().rename(src, dst))
        return {};
    // Different file system: copy, then delete the original.
    const QString err = copyTree(src, dst);
    if (!err.isEmpty())
        return err;
    return removeTree(src);
}

QString permString(QFile::Permissions p, bool isDir)
{
    QString s = isDir ? QStringLiteral("d") : QStringLiteral("-");
    s += p & QFile::ReadOwner ? 'r' : '-'; s += p & QFile::WriteOwner ? 'w' : '-'; s += p & QFile::ExeOwner ? 'x' : '-';
    s += p & QFile::ReadGroup ? 'r' : '-'; s += p & QFile::WriteGroup ? 'w' : '-'; s += p & QFile::ExeGroup ? 'x' : '-';
    s += p & QFile::ReadOther ? 'r' : '-'; s += p & QFile::WriteOther ? 'w' : '-'; s += p & QFile::ExeOther ? 'x' : '-';
    return s;
}

} // namespace

FileOps *FileOps::create(QQmlEngine *, QJSEngine *)
{
    static FileOps *f = new FileOps();
    QQmlEngine::setObjectOwnership(f, QQmlEngine::CppOwnership);
    return f;
}

FileOps::FileOps(QObject *parent)
    : QObject(parent)
{
    connect(QGuiApplication::clipboard(), &QClipboard::dataChanged, this, &FileOps::clipboardChanged);
}

QString FileOps::trashPath() const
{
    return trashBase() + QStringLiteral("/files");
}

QStringList FileOps::clipboardPaths() const
{
    // Prefer our own state; fall back to file URLs another app put there.
    if (!m_clip.isEmpty())
        return m_clip;
    QStringList out;
    if (const QMimeData *md = QGuiApplication::clipboard()->mimeData())
        for (const QUrl &u : md->urls())
            if (u.isLocalFile()) out << u.toLocalFile();
    return out;
}

void FileOps::setClipboard(const QStringList &paths, bool cut)
{
    m_clip = paths;
    m_cut = cut;
    auto *md = new QMimeData;
    QList<QUrl> urls;
    for (const QString &p : paths)
        urls << QUrl::fromLocalFile(p);
    md->setUrls(urls);
    md->setText(paths.join(QLatin1Char('\n')));
    // GNOME/GTK file managers understand this format for cut vs copy.
    md->setData(QStringLiteral("x-special/gnome-copied-files"),
                ((cut ? QStringLiteral("cut") : QStringLiteral("copy")) + QLatin1Char('\n')
                 + [&] { QStringList s; for (const QUrl &u : urls) s << u.toString(); return s.join(QLatin1Char('\n')); }())
                    .toUtf8());
    QGuiApplication::clipboard()->setMimeData(md);
    emit clipboardChanged();
}

bool FileOps::canPaste() const
{
    return !clipboardPaths().isEmpty();
}

void FileOps::paste(const QString &destDir)
{
    const QStringList paths = clipboardPaths();
    if (paths.isEmpty())
        return;
    if (m_cut && !m_clip.isEmpty()) {
        move(paths, destDir);
        m_clip.clear();
        m_cut = false;
        QGuiApplication::clipboard()->clear();
        emit clipboardChanged();
    } else {
        copy(paths, destDir);
    }
}

void FileOps::copy(const QStringList &s, const QString &d) { runJob(Kind::Copy, s, d); }
void FileOps::move(const QStringList &s, const QString &d) { runJob(Kind::Move, s, d); }
void FileOps::trash(const QStringList &p) { runJob(Kind::Trash, p, {}); }
void FileOps::deletePermanently(const QStringList &p) { runJob(Kind::Delete, p, {}); }
void FileOps::restore(const QStringList &p) { runJob(Kind::Restore, p, {}); }
void FileOps::emptyTrash() { runJob(Kind::EmptyTrash, {}, {}); }

void FileOps::ensureTrash()
{
    QDir().mkpath(trashBase() + QStringLiteral("/files"));
    QDir().mkpath(trashBase() + QStringLiteral("/info"));
}

void FileOps::runJob(Kind kind, const QStringList &paths, const QString &dest)
{
    if (m_busy) {
        emit finished(false, tr("Another file operation is still running."));
        return;
    }
    m_busy = true;
    m_progress = 0;
    m_status = kind == Kind::Copy ? tr("Copying…") : kind == Kind::Move ? tr("Moving…")
             : kind == Kind::Trash ? tr("Moving to Trash…") : kind == Kind::Restore ? tr("Restoring…")
             : tr("Deleting…");
    emit busyChanged();
    emit progressChanged();

    QPointer<FileOps> self(this);
    auto report = [self](double p, const QString &status) {
        QMetaObject::invokeMethod(qApp, [self, p, status] {
            if (!self) return;
            self->m_progress = p;
            if (!status.isEmpty()) self->m_status = status;
            emit self->progressChanged();
        });
    };

    QThread *t = QThread::create([self, kind, paths, dest, report] {
        QStringList errors;
        int done = 0;
        QStringList items = paths;
        if (kind == Kind::EmptyTrash) {
            for (const QFileInfo &fi : QDir(trashBase() + QStringLiteral("/files"))
                                          .entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System))
                items << fi.absoluteFilePath();
        }
        const int total = qMax(1, int(items.size()));
        for (const QString &src : std::as_const(items)) {
            const QFileInfo fi(src);
            const QString name = fi.fileName();
            report(double(done) / total, QString());
            QString err;
            switch (kind) {
            case Kind::Copy:
            case Kind::Move: {
                const QString destDir = QDir::cleanPath(dest);
                if (fi.isDir() && (destDir == fi.absoluteFilePath() || destDir.startsWith(fi.absoluteFilePath() + QLatin1Char('/')))) {
                    err = QObject::tr("Cannot put the folder \"%1\" inside itself.").arg(name);
                    break;
                }
                if (kind == Kind::Move && QDir::cleanPath(fi.absolutePath()) == destDir)
                    break;   // already there
                const QString target = uniqueIn(destDir, name);
                err = kind == Kind::Copy ? copyTree(fi.absoluteFilePath(), target) : moveTo(fi.absoluteFilePath(), target);
                break;
            }
            case Kind::Trash: {
                const QString filesDir = trashBase() + QStringLiteral("/files");
                const QString infoDir = trashBase() + QStringLiteral("/info");
                QDir().mkpath(filesDir);
                QDir().mkpath(infoDir);
                const QString target = uniqueIn(filesDir, name);
                const QString trashedName = QFileInfo(target).fileName();
                QFile info(infoDir + QLatin1Char('/') + trashedName + QStringLiteral(".trashinfo"));
                if (info.open(QIODevice::WriteOnly | QIODevice::Text)) {
                    info.write("[Trash Info]\nPath=");
                    info.write(QUrl::toPercentEncoding(fi.absoluteFilePath(), "/"));
                    info.write("\nDeletionDate=");
                    info.write(QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-ddTHH:mm:ss")).toUtf8());
                    info.write("\n");
                    info.close();
                }
                err = moveTo(fi.absoluteFilePath(), target);
                if (!err.isEmpty())
                    info.remove();
                break;
            }
            case Kind::Restore: {
                const QString infoPath = trashBase() + QStringLiteral("/info/") + name + QStringLiteral(".trashinfo");
                QSettings ini(infoPath, QSettings::IniFormat);
                const QString orig = QUrl::fromPercentEncoding(ini.value(QStringLiteral("Trash Info/Path")).toByteArray());
                if (orig.isEmpty()) {
                    err = QObject::tr("Don't know where \"%1\" came from.").arg(name);
                    break;
                }
                const QFileInfo of(orig);
                QDir().mkpath(of.absolutePath());
                err = moveTo(fi.absoluteFilePath(), uniqueIn(of.absolutePath(), of.fileName()));
                if (err.isEmpty())
                    QFile::remove(infoPath);
                break;
            }
            case Kind::Delete:
            case Kind::EmptyTrash:
                err = removeTree(fi.absoluteFilePath());
                if (err.isEmpty() && fi.absolutePath() == QDir::cleanPath(trashBase() + QStringLiteral("/files")))
                    QFile::remove(trashBase() + QStringLiteral("/info/") + name + QStringLiteral(".trashinfo"));
                break;
            }
            if (!err.isEmpty())
                errors << err;
            ++done;
        }
        report(1.0, QString());
        QMetaObject::invokeMethod(qApp, [self, errors, done, kind] {
            if (!self) return;
            self->m_busy = false;
            emit self->busyChanged();
            if (!errors.isEmpty()) {
                emit self->finished(false, errors.size() == 1 ? errors.first()
                                                              : QObject::tr("%1 items failed. First error: %2")
                                                                    .arg(errors.size()).arg(errors.first()));
                return;
            }
            QString msg;
            const QString items = done == 1 ? QObject::tr("1 item") : QObject::tr("%1 items").arg(done);
            switch (kind) {
            case Kind::Copy: msg = QObject::tr("Copied %1").arg(items); break;
            case Kind::Move: msg = QObject::tr("Moved %1").arg(items); break;
            case Kind::Trash: msg = QObject::tr("Moved %1 to Trash").arg(items); break;
            case Kind::Restore: msg = QObject::tr("Restored %1").arg(items); break;
            case Kind::Delete: msg = QObject::tr("Deleted %1").arg(items); break;
            case Kind::EmptyTrash: msg = QObject::tr("Trash emptied"); break;
            }
            emit self->finished(true, msg);
        });
    });
    connect(t, &QThread::finished, t, &QObject::deleteLater);
    t->start();
}

QString FileOps::rename(const QString &path, const QString &newName)
{
    const QString n = newName.trimmed();
    if (n.isEmpty() || n.contains(QLatin1Char('/')))
        return tr("A name can't be empty or contain \"/\".");
    if (n == QLatin1String(".") || n == QLatin1String(".."))
        return tr("That name is reserved.");
    const QFileInfo fi(path);
    const QString target = fi.absolutePath() + QLatin1Char('/') + n;
    if (fi.fileName() == n)
        return {};
    if (QFileInfo::exists(target))
        return tr("\"%1\" already exists here.").arg(n);
    if (!QDir().rename(path, target))
        return tr("Could not rename \"%1\". Check that you have permission.").arg(fi.fileName());
    return {};
}

QString FileOps::newFolder(const QString &dir, const QString &name)
{
    const QString n = name.trimmed();
    if (n.isEmpty() || n.contains(QLatin1Char('/')))
        return tr("A name can't be empty or contain \"/\".");
    if (QFileInfo::exists(dir + QLatin1Char('/') + n))
        return tr("\"%1\" already exists here.").arg(n);
    if (!QDir(dir).mkdir(n))
        return tr("Could not create the folder. Check that you have permission.");
    return {};
}

QString FileOps::newFile(const QString &dir, const QString &name)
{
    const QString n = name.trimmed();
    if (n.isEmpty() || n.contains(QLatin1Char('/')))
        return tr("A name can't be empty or contain \"/\".");
    QFile f(dir + QLatin1Char('/') + n);
    if (f.exists())
        return tr("\"%1\" already exists here.").arg(n);
    if (!f.open(QIODevice::WriteOnly))
        return tr("Could not create the file. Check that you have permission.");
    return {};
}

QString FileOps::uniqueName(const QString &dir, const QString &name) const
{
    return QFileInfo(uniqueIn(dir, name)).fileName();
}

QVariantMap FileOps::properties(const QString &path) const
{
    const QFileInfo fi(path);
    static QMimeDatabase db;
    const QMimeType mt = db.mimeTypeForFile(fi);
    QVariantMap m;
    m[QStringLiteral("name")] = fi.fileName().isEmpty() ? path : fi.fileName();
    m[QStringLiteral("path")] = fi.absoluteFilePath();
    m[QStringLiteral("location")] = fi.absolutePath();
    m[QStringLiteral("isDir")] = fi.isDir();
    m[QStringLiteral("type")] = fi.isDir() ? tr("Folder") : mt.comment();
    m[QStringLiteral("mime")] = mt.name();
    m[QStringLiteral("size")] = double(fi.size());
    m[QStringLiteral("modified")] = QLocale().toString(fi.lastModified(), QLocale::LongFormat);
    m[QStringLiteral("accessed")] = QLocale().toString(fi.lastRead(), QLocale::LongFormat);
    const QDateTime born = fi.birthTime();
    m[QStringLiteral("created")] = born.isValid() ? QLocale().toString(born, QLocale::LongFormat) : tr("Unknown");
    m[QStringLiteral("owner")] = fi.owner();
    m[QStringLiteral("group")] = fi.group();
    m[QStringLiteral("permissions")] = permString(fi.permissions(), fi.isDir());
    m[QStringLiteral("isLink")] = fi.isSymLink();
    m[QStringLiteral("linkTarget")] = fi.isSymLink() ? fi.symLinkTarget() : QString();
    m[QStringLiteral("readable")] = fi.isReadable();
    m[QStringLiteral("writable")] = fi.isWritable();
    m[QStringLiteral("executable")] = fi.isExecutable() && !fi.isDir();
    return m;
}

void FileOps::computeSize(const QString &path)
{
    QPointer<FileOps> self(this);
    QThread *t = QThread::create([self, path] {
        double bytes = 0;
        int files = 0, folders = 0;
        QDirIterator it(path, QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System,
                        QDirIterator::Subdirectories);
        while (it.hasNext()) {
            it.next();
            const QFileInfo fi = it.fileInfo();
            if (fi.isSymLink()) continue;
            if (fi.isDir()) ++folders;
            else { ++files; bytes += double(fi.size()); }
        }
        QMetaObject::invokeMethod(qApp, [self, path, bytes, files, folders] {
            if (self) emit self->sizeComputed(path, bytes, files, folders);
        });
    });
    connect(t, &QThread::finished, t, &QObject::deleteLater);
    t->start();
}
