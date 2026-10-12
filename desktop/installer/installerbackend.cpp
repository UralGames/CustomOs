#include "installerbackend.h"

#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QRegularExpression>
#include <QVariantMap>

#ifndef MERIDIAN_LIBEXECDIR
#define MERIDIAN_LIBEXECDIR "/usr/libexec"
#endif

InstallerBackend::InstallerBackend(QObject *parent)
    : QObject(parent)
{
    refreshDisks();
}

bool InstallerBackend::uefi() const
{
    return QFileInfo::exists(QStringLiteral("/sys/firmware/efi"));
}

static QString humanSize(double b)
{
    const char *u[] = {"B", "KB", "MB", "GB", "TB"};
    int i = 0;
    while (b >= 1000 && i < 4) { b /= 1000; ++i; }   // disks are sold in decimal units
    return QStringLiteral("%1 %2").arg(b, 0, 'f', b < 10 ? 1 : 0).arg(QLatin1String(u[i]));
}

void InstallerBackend::refreshDisks()
{
    QProcess p;
    p.start(QStringLiteral("lsblk"), {QStringLiteral("-J"), QStringLiteral("-b"), QStringLiteral("-o"),
                                      QStringLiteral("NAME,PATH,SIZE,MODEL,TYPE,RM,RO,TRAN,MOUNTPOINT"),
                                      QStringLiteral("-d")});
    p.waitForFinished(5000);
    const QJsonArray devs = QJsonDocument::fromJson(p.readAllStandardOutput()).object()
                                .value(QLatin1String("blockdevices")).toArray();
    QVariantList out;
    for (const QJsonValue &v : devs) {
        const QJsonObject d = v.toObject();
        const QString type = d.value(QLatin1String("type")).toString();
        if (type != QLatin1String("disk"))
            continue;   // skip loop devices, CD-ROMs (the live medium), RAM disks
        if (d.value(QLatin1String("ro")).toVariant().toBool())
            continue;
        const QString name = d.value(QLatin1String("name")).toString();
        if (name.startsWith(QLatin1String("zram")) || name.startsWith(QLatin1String("ram")))
            continue;
        const double size = d.value(QLatin1String("size")).toVariant().toDouble();
        QString model = d.value(QLatin1String("model")).toString().trimmed();
        if (model.isEmpty()) model = tr("Disk");
        QString path = d.value(QLatin1String("path")).toString();
        if (path.isEmpty()) path = QStringLiteral("/dev/") + name;
        out << QVariantMap{{QStringLiteral("path"), path},
                           {QStringLiteral("name"), name},
                           {QStringLiteral("model"), model},
                           {QStringLiteral("size"), size},
                           {QStringLiteral("sizeText"), humanSize(size)},
                           {QStringLiteral("removable"), d.value(QLatin1String("rm")).toVariant().toBool()},
                           {QStringLiteral("transport"), d.value(QLatin1String("tran")).toString()},
                           {QStringLiteral("bigEnough"), size >= 8.0 * 1024 * 1024 * 1024}};
    }
    m_disks = out;
    emit disksChanged();
}

QString InstallerBackend::suggestUserName(const QString &fullName) const
{
    QString first = fullName.trimmed().section(QLatin1Char(' '), 0, 0).toLower();
    first = first.normalized(QString::NormalizationForm_KD);
    first.remove(QRegularExpression(QStringLiteral("[^a-z0-9_-]")));
    if (first.isEmpty() || !first.at(0).isLetter())
        first = QStringLiteral("user") + first;
    return first.left(32);
}

void InstallerBackend::install(const QString &disk, const QString &confirmDisk, const QString &user,
                               const QString &fullName, const QString &hostName, const QString &password)
{
    if (m_running)
        return;
    m_running = true;
    m_progress = 0;
    m_status = tr("Starting…");
    m_log.clear();
    m_error.clear();
    emit runningChanged();
    emit progressChanged();
    emit logChanged();

    m_proc = new QProcess(this);
    m_proc->setProcessChannelMode(QProcess::MergedChannels);
    connect(m_proc, &QProcess::readyRead, this, [this] {
        while (m_proc->canReadLine()) {
            const QString line = QString::fromUtf8(m_proc->readLine()).trimmed();
            if (line.startsWith(QLatin1String("PROGRESS "))) {
                m_progress = line.section(QLatin1Char(' '), 1, 1).toInt();
                m_status = line.section(QLatin1Char(' '), 2);
                emit progressChanged();
            } else if (line.startsWith(QLatin1String("ERROR "))) {
                m_error = line.mid(6);
            }
            if (!line.isEmpty() && !line.startsWith(QLatin1String("PROGRESS "))) {
                m_log += line + QLatin1Char('\n');
                emit logChanged();
            }
        }
    });
    connect(m_proc, &QProcess::finished, this, [this](int code, QProcess::ExitStatus st) {
        m_running = false;
        emit runningChanged();
        const bool ok = st == QProcess::NormalExit && code == 0;
        if (!ok && m_error.isEmpty())
            m_error = tr("The installer stopped unexpectedly (exit code %1).").arg(code);
        emit finished(ok, ok ? tr("CustomOS has been installed.") : m_error);
        m_proc->deleteLater();
        m_proc = nullptr;
    });
    // Images with over-the-air updates ship the A/B installer (customos-updater);
    // it lays the disk out with two system slots and a data partition.
    const QString abScript = QStringLiteral(MERIDIAN_LIBEXECDIR "/customos-install-ab");
    const QString script = QFileInfo::exists(abScript) ? abScript
                                                       : QStringLiteral(MERIDIAN_LIBEXECDIR "/customos-install");
    m_proc->start(QStringLiteral("sudo"),
                  {QStringLiteral("-n"), script,
                   QStringLiteral("--disk"), disk, QStringLiteral("--confirm"), confirmDisk,
                   QStringLiteral("--user"), user, QStringLiteral("--fullname"), fullName,
                   QStringLiteral("--hostname"), hostName});
    QByteArray pw = password.toUtf8() + '\n';
    m_proc->write(pw);
    pw.fill('\0');
    m_proc->closeWriteChannel();
}

void InstallerBackend::reboot()
{
    QProcess::startDetached(QStringLiteral("systemctl"), {QStringLiteral("reboot")});
}
