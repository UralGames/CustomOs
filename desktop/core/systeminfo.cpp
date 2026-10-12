#include "systeminfo.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QQmlEngine>
#include <QStorageInfo>
#include <QSysInfo>
#include <QThread>
#include <QVariantMap>

#include <grp.h>
#include <pwd.h>
#include <unistd.h>

static QString readFirstLine(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};
    return QString::fromUtf8(f.readLine()).trimmed();
}

SystemInfo *SystemInfo::create(QQmlEngine *, QJSEngine *)
{
    static SystemInfo *s = new SystemInfo();
    QQmlEngine::setObjectOwnership(s, QQmlEngine::CppOwnership);
    return s;
}

SystemInfo::SystemInfo(QObject *parent)
    : QObject(parent)
{
    // /etc/os-release
    QFile osr(QStringLiteral("/etc/os-release"));
    if (osr.open(QIODevice::ReadOnly | QIODevice::Text)) {
        while (!osr.atEnd()) {
            const QString line = QString::fromUtf8(osr.readLine()).trimmed();
            auto val = [&line] { QString v = line.section(QLatin1Char('='), 1); v.remove(QLatin1Char('"')); return v; };
            if (line.startsWith(QLatin1String("NAME="))) m_osName = val();
            else if (line.startsWith(QLatin1String("VERSION="))) m_osVersion = val();
            else if (line.startsWith(QLatin1String("VERSION_ID=")) && m_osVersion.isEmpty()) m_osVersion = val();
        }
    }
    if (m_osName.isEmpty())
        m_osName = QSysInfo::productType();

    m_kernel = QSysInfo::kernelType() + QLatin1Char(' ') + QSysInfo::kernelVersion();
    m_arch = QSysInfo::currentCpuArchitecture();
    m_cpuCount = QThread::idealThreadCount();

    QFile cpu(QStringLiteral("/proc/cpuinfo"));
    if (cpu.open(QIODevice::ReadOnly | QIODevice::Text)) {
        while (!cpu.atEnd()) {
            const QString line = QString::fromUtf8(cpu.readLine());
            if (line.startsWith(QLatin1String("model name"))) {
                m_cpuModel = line.section(QLatin1Char(':'), 1).simplified();
                break;
            }
        }
    }

    QFile mem(QStringLiteral("/proc/meminfo"));
    if (mem.open(QIODevice::ReadOnly | QIODevice::Text)) {
        const QString line = QString::fromUtf8(mem.readLine());   // MemTotal:  8000000 kB
        m_memTotal = line.section(QLatin1Char(':'), 1).simplified().section(QLatin1Char(' '), 0, 0).toDouble() * 1024.0;
    }

    // Kernel DRM driver of the first card with a connected output.
    const QDir drm(QStringLiteral("/sys/class/drm"));
    for (const QString &card : drm.entryList({QStringLiteral("card?")}, QDir::Dirs | QDir::System)) {
        const QString drv = QFileInfo(drm.absoluteFilePath(card + QStringLiteral("/device/driver"))).symLinkTarget();
        if (!drv.isEmpty()) {
            m_gpuDriver = QFileInfo(drv).fileName();
            break;
        }
    }

    const QString vendor = readFirstLine(QStringLiteral("/sys/class/dmi/id/sys_vendor"));
    const QString product = readFirstLine(QStringLiteral("/sys/class/dmi/id/product_name"));
    m_machine = (vendor + QLatin1Char(' ') + product).simplified();
}

bool SystemInfo::softwareRendering() const
{
    return qEnvironmentVariable("LIBGL_ALWAYS_SOFTWARE") == QLatin1String("1");
}

QString SystemInfo::qtVersion() const { return QString::fromLatin1(qVersion()); }
QString SystemInfo::desktopVersion() const { return QStringLiteral(MERIDIAN_VERSION); }

qint64 SystemInfo::uptimeSeconds() const
{
    return qint64(readFirstLine(QStringLiteral("/proc/uptime")).section(QLatin1Char(' '), 0, 0).toDouble());
}

QVariantMap SystemInfo::diskUsage(const QString &path) const
{
    QStorageInfo si(path);
    return {{QStringLiteral("total"), double(si.bytesTotal())},
            {QStringLiteral("free"), double(si.bytesAvailable())},
            {QStringLiteral("used"), double(si.bytesTotal() - si.bytesFree())},
            {QStringLiteral("device"), QString::fromUtf8(si.device())},
            {QStringLiteral("fsType"), QString::fromUtf8(si.fileSystemType())}};
}

QStringList SystemInfo::userGroups() const
{
    QStringList out;
    const passwd *pw = getpwuid(getuid());
    if (!pw)
        return out;
    int n = 64;
    QList<gid_t> groups(n);
    if (getgrouplist(pw->pw_name, pw->pw_gid, groups.data(), &n) < 0) {
        groups.resize(n);
        getgrouplist(pw->pw_name, pw->pw_gid, groups.data(), &n);
    }
    for (int i = 0; i < n; ++i)
        if (const group *g = getgrgid(groups.at(i)))
            out << QString::fromLocal8Bit(g->gr_name);
    return out;
}
