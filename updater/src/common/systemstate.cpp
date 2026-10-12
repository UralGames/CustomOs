#include "systemstate.h"

#include <QFile>
#include <QFileInfo>
#include <QHash>

static QHash<QString, QString> readOsRelease(const QString &path)
{
    QHash<QString, QString> out;
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return out;
    while (!f.atEnd()) {
        const QString line = QString::fromUtf8(f.readLine()).trimmed();
        if (line.isEmpty() || line.startsWith(QLatin1Char('#')))
            continue;
        const int eq = line.indexOf(QLatin1Char('='));
        if (eq <= 0)
            continue;
        QString value = line.mid(eq + 1).trimmed();
        if (value.size() >= 2 && (value.startsWith(QLatin1Char('"')) || value.startsWith(QLatin1Char('\'')))
            && value.endsWith(value.at(0)))
            value = value.mid(1, value.size() - 2);
        value.replace(QLatin1String("\\\""), QLatin1String("\""));
        out.insert(line.left(eq), value);
    }
    return out;
}

SystemState SystemState::probeFrom(const QString &osReleasePath, const QString &cmdlinePath,
                                   const QString &liveMarkerPath)
{
    SystemState s;
    const auto os = readOsRelease(osReleasePath);
    s.osName = os.value(QStringLiteral("NAME"), QStringLiteral("CustomOS"));
    s.prettyName = os.value(QStringLiteral("PRETTY_NAME"), s.osName);
    s.versionText = os.value(QStringLiteral("VERSION_ID"));
    s.version = Version::parse(s.versionText);
    s.buildId = os.value(QStringLiteral("BUILD_ID"));

    QFile cmd(cmdlinePath);
    QString cmdline;
    if (cmd.open(QIODevice::ReadOnly))
        cmdline = QString::fromUtf8(cmd.readAll());
    for (const QString &arg : cmdline.split(QLatin1Char(' '), Qt::SkipEmptyParts)) {
        if (arg.startsWith(QLatin1String("rauc.slot=")))
            s.bootedSlot = arg.mid(10).trimmed();
    }

    if (QFileInfo::exists(liveMarkerPath))
        s.mode = Mode::Live;
    else if (s.bootedSlot == QLatin1String("A") || s.bootedSlot == QLatin1String("B"))
        s.mode = Mode::AB;
    else
        s.mode = Mode::Legacy;
    return s;
}

SystemState SystemState::probe()
{
    const QString root = qEnvironmentVariable("CUSTOMOS_UPDATE_SYSROOT");   // tests only
    return probeFrom(root + QStringLiteral("/etc/os-release"), root + QStringLiteral("/proc/cmdline"),
                     root + QStringLiteral("/etc/customos-live"));
}

QString SystemState::modeName(Mode m)
{
    switch (m) {
    case Mode::AB: return QStringLiteral("ab");
    case Mode::Live: return QStringLiteral("live");
    case Mode::Legacy: return QStringLiteral("legacy");
    case Mode::Unknown: break;
    }
    return QStringLiteral("unknown");
}
