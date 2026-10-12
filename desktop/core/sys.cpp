#include "sys.h"
#include "meridian.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QQmlEngine>
#include <QStandardPaths>
#include <QSysInfo>

#include <pwd.h>
#include <unistd.h>

Sys *Sys::create(QQmlEngine *, QJSEngine *)
{
    static Sys *s = new Sys();
    QQmlEngine::setObjectOwnership(s, QQmlEngine::CppOwnership);
    return s;
}

QString Sys::homePath() const { return QDir::homePath(); }

QString Sys::userName() const
{
    if (const passwd *pw = getpwuid(getuid()))
        return QString::fromLocal8Bit(pw->pw_name);
    return qEnvironmentVariable("USER");
}

QString Sys::userFullName() const
{
    if (const passwd *pw = getpwuid(getuid())) {
        const QString gecos = QString::fromLocal8Bit(pw->pw_gecos).section(QLatin1Char(','), 0, 0);
        if (!gecos.isEmpty())
            return gecos;
    }
    return userName();
}

QString Sys::dataDir() const { return Meridian::dataDir(); }
bool Sys::isLive() const { return Meridian::isLiveSystem(); }
QString Sys::platformName() const { return QGuiApplication::platformName(); }
QString Sys::hostName() const { return QSysInfo::machineHostName(); }
QString Sys::env(const QString &name) const { return qEnvironmentVariable(name.toLocal8Bit().constData()); }
bool Sys::fileExists(const QString &path) const { return QFileInfo::exists(path); }
bool Sys::isDir(const QString &path) const { return QFileInfo(path).isDir(); }
QString Sys::fileName(const QString &path) const { return QFileInfo(path).fileName(); }

QString Sys::parentDir(const QString &path) const
{
    QDir d(QFileInfo(path).absoluteFilePath());
    return d.cdUp() ? d.absolutePath() : QStringLiteral("/");
}

QUrl Sys::fileUrl(const QString &path) const { return QUrl::fromLocalFile(path); }
QString Sys::urlToPath(const QUrl &url) const { return url.isLocalFile() ? url.toLocalFile() : url.toString(); }

QString Sys::standardDir(const QString &which) const
{
    static const QHash<QString, QStandardPaths::StandardLocation> map{
        {QStringLiteral("desktop"), QStandardPaths::DesktopLocation},
        {QStringLiteral("documents"), QStandardPaths::DocumentsLocation},
        {QStringLiteral("downloads"), QStandardPaths::DownloadLocation},
        {QStringLiteral("music"), QStandardPaths::MusicLocation},
        {QStringLiteral("pictures"), QStandardPaths::PicturesLocation},
        {QStringLiteral("videos"), QStandardPaths::MoviesLocation},
    };
    auto it = map.constFind(which);
    if (it == map.cend())
        return QDir::homePath();
    return QStandardPaths::writableLocation(*it);
}

QString Sys::formatBytes(double bytes) const
{
    static const char *units[] = {"B", "KB", "MB", "GB", "TB"};
    int u = 0;
    while (bytes >= 1024.0 && u < 4) {
        bytes /= 1024.0;
        ++u;
    }
    if (u == 0)
        return QStringLiteral("%1 %2").arg(qint64(bytes)).arg(QLatin1String(units[u]));
    return QStringLiteral("%1 %2").arg(bytes, 0, 'f', bytes < 10 ? 1 : 0).arg(QLatin1String(units[u]));
}

QString Sys::formatDuration(qint64 s) const
{
    const qint64 d = s / 86400, h = (s % 86400) / 3600, m = (s % 3600) / 60;
    if (d > 0) return tr("%1 d %2 h").arg(d).arg(h);
    if (h > 0) return tr("%1 h %2 min").arg(h).arg(m);
    return tr("%1 min").arg(m);
}

QStringList Sys::wallpapers() const
{
    QStringList out;
    const QStringList filters{QStringLiteral("*.jpg"), QStringLiteral("*.jpeg"),
                              QStringLiteral("*.png"), QStringLiteral("*.webp")};
    const QStringList dirs{Meridian::dataDir() + QStringLiteral("/wallpapers"),
                           QStandardPaths::writableLocation(QStandardPaths::PicturesLocation)
                               + QStringLiteral("/Wallpapers")};
    for (const QString &d : dirs) {
        QDir dir(d);
        for (const QString &f : dir.entryList(filters, QDir::Files, QDir::Name))
            out << dir.absoluteFilePath(f);
    }
    return out;
}

QString Sys::importWallpaper(const QString &path) const
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation)
                        + QStringLiteral("/Wallpapers");
    QDir().mkpath(dir);
    const QFileInfo fi(path);
    const QString target = dir + QLatin1Char('/') + fi.fileName();
    if (fi.absoluteFilePath() == QFileInfo(target).absoluteFilePath())
        return target;
    QFile::remove(target);
    return QFile::copy(path, target) ? target : QString();
}

bool Sys::hasExecutable(const QString &name) const
{
    return !QStandardPaths::findExecutable(name).isEmpty();
}
