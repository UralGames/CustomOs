#include "stats.h"

#include <QCollator>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QStorageInfo>
#include <QTimer>
#include <QVariantMap>

#include <cerrno>
#include <csignal>
#include <cstring>
#include <pwd.h>
#include <unistd.h>

static const int kHistory = 60;

static QByteArray readAll(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return {};
    return f.readAll();
}

static void push(QVariantList &list, double v)
{
    list << v;
    while (list.size() > kHistory)
        list.removeFirst();
}

// ---- SystemStats ------------------------------------------------------------

SystemStats::SystemStats(QObject *parent)
    : QObject(parent)
{
    for (int i = 0; i < kHistory; ++i) {
        m_cpuHist << 0.0;
        m_memHist << 0.0;
        m_netHist << 0.0;
    }
    m_timer = new QTimer(this);
    m_timer->setInterval(1000);
    connect(m_timer, &QTimer::timeout, this, &SystemStats::sample);
    m_timer->start();
    sample();
}

bool SystemStats::running() const { return m_timer->isActive(); }

void SystemStats::setRunning(bool r)
{
    if (r == running()) return;
    r ? m_timer->start() : m_timer->stop();
    emit runningChanged();
}

void SystemStats::sample()
{
    // CPU: /proc/stat "cpu" and "cpuN" lines
    const QList<QByteArray> lines = readAll(QStringLiteral("/proc/stat")).split('\n');
    QList<CpuTimes> now;
    for (const QByteArray &l : lines) {
        if (!l.startsWith("cpu"))
            continue;
        const QList<QByteArray> f = l.simplified().split(' ');
        CpuTimes t;
        for (int i = 1; i < f.size(); ++i)
            t.total += f.at(i).toULongLong();
        // idle + iowait
        t.idle = f.value(4).toULongLong() + f.value(5).toULongLong();
        now << t;
    }
    m_perCore.clear();
    for (int i = 0; i < now.size(); ++i) {
        double pct = 0;
        if (i < m_prevCpu.size()) {
            const quint64 dt = now.at(i).total - m_prevCpu.at(i).total;
            const quint64 di = now.at(i).idle - m_prevCpu.at(i).idle;
            pct = dt ? 100.0 * double(dt - di) / double(dt) : 0;
        }
        if (i == 0) m_cpu = pct;
        else m_perCore << pct;
    }
    m_prevCpu = now;
    push(m_cpuHist, m_cpu);

    // Memory
    QHash<QByteArray, double> mem;
    for (const QByteArray &l : readAll(QStringLiteral("/proc/meminfo")).split('\n')) {
        const int c = l.indexOf(':');
        if (c > 0)
            mem.insert(l.left(c), l.mid(c + 1).simplified().split(' ').value(0).toDouble() * 1024.0);
    }
    m_memTotal = mem.value("MemTotal");
    const double avail = mem.value("MemAvailable", mem.value("MemFree"));
    m_memUsed = m_memTotal - avail;
    m_memCached = mem.value("Cached") + mem.value("Buffers");
    m_swapTotal = mem.value("SwapTotal");
    m_swapUsed = m_swapTotal - mem.value("SwapFree");
    push(m_memHist, m_memTotal > 0 ? 100.0 * m_memUsed / m_memTotal : 0);

    // Network: sum of non-loopback interfaces
    quint64 rx = 0, tx = 0;
    for (const QByteArray &l : readAll(QStringLiteral("/proc/net/dev")).split('\n')) {
        const int c = l.indexOf(':');
        if (c < 0) continue;
        const QByteArray iface = l.left(c).trimmed();
        if (iface == "lo") continue;
        const QList<QByteArray> f = l.mid(c + 1).simplified().split(' ');
        rx += f.value(0).toULongLong();
        tx += f.value(8).toULongLong();
    }
    const qint64 ms = QDateTime::currentMSecsSinceEpoch();
    if (m_prevNetMs > 0 && ms > m_prevNetMs) {
        const double secs = (ms - m_prevNetMs) / 1000.0;
        m_rx = rx >= m_prevRx ? (rx - m_prevRx) / secs : 0;
        m_tx = tx >= m_prevTx ? (tx - m_prevTx) / secs : 0;
    }
    m_prevRx = rx;
    m_prevTx = tx;
    m_prevNetMs = ms;
    push(m_netHist, m_rx + m_tx);

    // Disks: real block-device file systems
    m_disks.clear();
    for (const QStorageInfo &s : QStorageInfo::mountedVolumes()) {
        if (!s.isValid() || !s.isReady() || s.bytesTotal() <= 0)
            continue;
        const QByteArray dev = s.device();
        if (!dev.startsWith("/dev/") && s.rootPath() != QLatin1String("/"))
            continue;
        m_disks << QVariantMap{{QStringLiteral("mount"), s.rootPath()},
                               {QStringLiteral("device"), QString::fromUtf8(dev)},
                               {QStringLiteral("type"), QString::fromUtf8(s.fileSystemType())},
                               {QStringLiteral("total"), double(s.bytesTotal())},
                               {QStringLiteral("free"), double(s.bytesAvailable())}};
    }

    m_load = QString::fromUtf8(readAll(QStringLiteral("/proc/loadavg"))).section(QLatin1Char(' '), 0, 2);
    m_procs = 0;
    for (const QString &e : QDir(QStringLiteral("/proc")).entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        bool ok = false;
        e.toInt(&ok);
        if (ok) ++m_procs;
    }
    emit updated();
}

// ---- ProcessModel ------------------------------------------------------------

ProcessModel::ProcessModel(QObject *parent)
    : QAbstractListModel(parent)
{
    m_timer = new QTimer(this);
    m_timer->setInterval(2000);
    connect(m_timer, &QTimer::timeout, this, &ProcessModel::refresh);
    m_timer->start();
    refresh();
}

bool ProcessModel::running() const { return m_timer->isActive(); }

void ProcessModel::setRunning(bool r)
{
    if (r == running()) return;
    r ? m_timer->start() : m_timer->stop();
    if (r) refresh();
    emit runningChanged();
}

void ProcessModel::refresh()
{
    // Total CPU ticks across all cores since boot (for the delta).
    quint64 total = 0;
    const QByteArray statAll = readAll(QStringLiteral("/proc/stat"));
    {
        const QList<QByteArray> f = statAll.left(statAll.indexOf('\n')).simplified().split(' ');
        for (int i = 1; i < f.size(); ++i) total += f.at(i).toULongLong();
    }
    const quint64 dTotal = m_prevTotal ? total - m_prevTotal : 0;
    const long pageSize = sysconf(_SC_PAGESIZE);
    static QHash<uint, QString> userNames;

    QList<Proc> procs;
    QHash<int, quint64> ticks;
    for (const QString &e : QDir(QStringLiteral("/proc")).entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        bool ok = false;
        const int pid = e.toInt(&ok);
        if (!ok) continue;
        const QString base = QStringLiteral("/proc/") + e;
        const QByteArray stat = readAll(base + QStringLiteral("/stat"));
        if (stat.isEmpty()) continue;
        // "pid (comm) state ppid ..." - comm may contain spaces/parens.
        const int lp = stat.indexOf('('), rp = stat.lastIndexOf(')');
        if (lp < 0 || rp < 0) continue;
        Proc p;
        p.pid = pid;
        p.name = QString::fromUtf8(stat.mid(lp + 1, rp - lp - 1));
        const QList<QByteArray> f = stat.mid(rp + 2).split(' ');
        p.state = QString::fromLatin1(f.value(0));
        const quint64 utime = f.value(11).toULongLong(), stime = f.value(12).toULongLong();
        p.threads = f.value(17).toInt();
        p.memory = double(f.value(21).toLongLong()) * pageSize;   // rss pages
        const quint64 t = utime + stime;
        ticks.insert(pid, t);
        if (dTotal > 0 && m_prevTicks.contains(pid))
            p.cpu = 100.0 * double(t - m_prevTicks.value(pid)) / double(dTotal);

        // Owner
        for (const QByteArray &l : readAll(base + QStringLiteral("/status")).split('\n')) {
            if (l.startsWith("Uid:")) {
                p.uid = l.mid(4).simplified().split(' ').value(0).toUInt();
                break;
            }
        }
        if (!userNames.contains(p.uid)) {
            const passwd *pw = getpwuid(p.uid);
            userNames.insert(p.uid, pw ? QString::fromLocal8Bit(pw->pw_name) : QString::number(p.uid));
        }
        p.user = userNames.value(p.uid);

        QByteArray cmd = readAll(base + QStringLiteral("/cmdline"));
        cmd.replace('\0', ' ');
        p.command = QString::fromUtf8(cmd).trimmed();
        if (p.command.isEmpty()) p.command = QLatin1Char('[') + p.name + QLatin1Char(']');
        procs << p;
    }
    m_prevTicks = ticks;
    m_prevTotal = total;
    m_all = procs;
    apply();
}

void ProcessModel::apply()
{
    const uint me = getuid();
    QList<Proc> rows;
    for (const Proc &p : std::as_const(m_all)) {
        if (m_onlyMine && p.uid != me) continue;
        if (!m_filter.isEmpty() && !p.name.contains(m_filter, Qt::CaseInsensitive)
            && !p.command.contains(m_filter, Qt::CaseInsensitive) && QString::number(p.pid) != m_filter)
            continue;
        rows << p;
    }
    QCollator coll;
    coll.setCaseSensitivity(Qt::CaseInsensitive);
    const QString key = m_sortKey;
    const bool desc = m_desc;
    std::stable_sort(rows.begin(), rows.end(), [&](const Proc &a, const Proc &b) {
        int c = 0;
        if (key == QLatin1String("memory")) c = a.memory < b.memory ? -1 : a.memory > b.memory ? 1 : 0;
        else if (key == QLatin1String("pid")) c = a.pid < b.pid ? -1 : a.pid > b.pid ? 1 : 0;
        else if (key == QLatin1String("name")) c = coll.compare(a.name, b.name);
        else if (key == QLatin1String("user")) c = coll.compare(a.user, b.user);
        else c = a.cpu < b.cpu ? -1 : a.cpu > b.cpu ? 1 : 0;
        if (c == 0) c = a.pid < b.pid ? -1 : 1;
        return desc ? c > 0 : c < 0;
    });
    beginResetModel();
    m_rows = rows;
    endResetModel();
    emit countChanged();
}

int ProcessModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_rows.size();
}

QVariant ProcessModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_rows.size())
        return {};
    const Proc &p = m_rows.at(index.row());
    switch (role) {
    case PidRole: return p.pid;
    case Qt::DisplayRole:
    case NameRole: return p.name;
    case CommandRole: return p.command;
    case UserRole: return p.user;
    case CpuRole: return p.cpu;
    case MemoryRole: return p.memory;
    case StateRole: return p.state;
    case ThreadsRole: return p.threads;
    case MineRole: return p.uid == getuid();
    }
    return {};
}

QHash<int, QByteArray> ProcessModel::roleNames() const
{
    return {{PidRole, "pid"}, {NameRole, "name"}, {CommandRole, "command"}, {UserRole, "user"},
            {CpuRole, "cpu"}, {MemoryRole, "memory"}, {StateRole, "procState"}, {ThreadsRole, "threads"},
            {MineRole, "mine"}};
}

void ProcessModel::setSortKey(const QString &k)
{
    if (k == m_sortKey) return;
    m_sortKey = k;
    m_desc = (k == QLatin1String("cpu") || k == QLatin1String("memory"));
    emit sortChanged();
    apply();
}

void ProcessModel::setSortDescending(bool d)
{
    if (d == m_desc) return;
    m_desc = d;
    emit sortChanged();
    apply();
}

void ProcessModel::setFilter(const QString &f)
{
    if (f == m_filter) return;
    m_filter = f;
    emit filterChanged();
    apply();
}

void ProcessModel::setOnlyMine(bool m)
{
    if (m == m_onlyMine) return;
    m_onlyMine = m;
    emit filterChanged();
    apply();
}

QString ProcessModel::endProcess(int pid, bool force)
{
    if (pid <= 1)
        return tr("This process cannot be ended.");
    if (::kill(pid_t(pid), force ? SIGKILL : SIGTERM) == 0) {
        QTimer::singleShot(400, this, &ProcessModel::refresh);
        return {};
    }
    if (errno == EPERM)
        return tr("You don't have permission to end process %1 (it belongs to another user).").arg(pid);
    if (errno == ESRCH)
        return tr("Process %1 has already exited.").arg(pid);
    return QString::fromLocal8Bit(strerror(errno));
}
