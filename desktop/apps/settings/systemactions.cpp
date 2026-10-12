#include "systemactions.h"

#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QQmlEngine>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QVariantMap>

#include <pwd.h>
#include <unistd.h>

SystemActions *SystemActions::create(QQmlEngine *, QJSEngine *)
{
    static SystemActions *s = new SystemActions();
    QQmlEngine::setObjectOwnership(s, QQmlEngine::CppOwnership);
    return s;
}

SystemActions::SystemActions(QObject *parent)
    : QObject(parent)
{
    refreshTime();
}

void SystemActions::run(const QString &action, const QString &program, const QStringList &args,
                        const QByteArray &input, const QString &okMessage)
{
    if (QStandardPaths::findExecutable(program).isEmpty()) {
        emit finished(action, false, tr("%1 is not available on this system.").arg(program));
        return;
    }
    auto *p = new QProcess(this);
    p->setProcessChannelMode(QProcess::MergedChannels);
    connect(p, &QProcess::finished, this, [this, p, action, okMessage](int code, QProcess::ExitStatus st) {
        QString out = QString::fromUtf8(p->readAll()).trimmed();
        p->deleteLater();
        const bool ok = st == QProcess::NormalExit && code == 0;
        if (!ok && out.isEmpty())
            out = tr("The operation failed.");
        // sudo's prompts are noise in an error message.
        out.remove(QRegularExpression(QStringLiteral("\\[sudo\\][^\\n]*\\n?")));
        emit finished(action, ok, ok ? okMessage : out.trimmed());
        refreshTime();
    });
    p->start(program, args);
    if (!input.isEmpty()) {
        p->write(input);
        p->closeWriteChannel();
    }
}

void SystemActions::refreshTime()
{
    // /etc/localtime -> ../usr/share/zoneinfo/Region/City
    const QString target = QFileInfo(QStringLiteral("/etc/localtime")).symLinkTarget();
    const int i = target.indexOf(QLatin1String("zoneinfo/"));
    QString tz = i >= 0 ? target.mid(i + 9) : QStringLiteral("UTC");
    bool ntp = m_ntp;
    if (!QStandardPaths::findExecutable(QStringLiteral("timedatectl")).isEmpty()) {
        QProcess p;
        p.start(QStringLiteral("timedatectl"), {QStringLiteral("show"), QStringLiteral("--property=NTP"),
                                                 QStringLiteral("--property=Timezone")});
        if (p.waitForFinished(1500)) {
            for (const QString &line : QString::fromUtf8(p.readAllStandardOutput()).split(QLatin1Char('\n'))) {
                if (line.startsWith(QLatin1String("NTP="))) ntp = line.endsWith(QLatin1String("yes"));
                if (line.startsWith(QLatin1String("Timezone=")) && line.size() > 9) tz = line.mid(9);
            }
        }
    }
    if (tz != m_timezone || ntp != m_ntp) {
        m_timezone = tz;
        m_ntp = ntp;
        emit timeChanged();
    }
}

QStringList SystemActions::timezones() const
{
    QStringList out;
    const QString base = QStringLiteral("/usr/share/zoneinfo/");
    QDirIterator it(base, QDir::Files, QDirIterator::Subdirectories);
    static const QRegularExpression ok(QStringLiteral("^[A-Z][A-Za-z_\\-]+(/[A-Za-z0-9_\\-+]+)+$"));
    while (it.hasNext()) {
        const QString rel = it.next().mid(base.size());
        if (rel.startsWith(QLatin1String("posix/")) || rel.startsWith(QLatin1String("right/"))
            || rel.startsWith(QLatin1String("Etc/")) || rel.startsWith(QLatin1String("SystemV/")))
            continue;
        if (ok.match(rel).hasMatch())
            out << rel;
    }
    out << QStringLiteral("UTC");
    out.sort();
    out.removeDuplicates();
    return out;
}

void SystemActions::setTimezone(const QString &tz)
{
    run(QStringLiteral("timezone"), QStringLiteral("timedatectl"), {QStringLiteral("set-timezone"), tz},
        {}, tr("Time zone set to %1.").arg(tz));
}

void SystemActions::setNtp(bool on)
{
    run(QStringLiteral("ntp"), QStringLiteral("timedatectl"),
        {QStringLiteral("set-ntp"), on ? QStringLiteral("true") : QStringLiteral("false")},
        {}, on ? tr("Clock will be synchronised over the network.") : tr("Network time turned off."));
}

void SystemActions::setHostname(const QString &name)
{
    static const QRegularExpression valid(QStringLiteral("^[a-zA-Z0-9]([a-zA-Z0-9-]{0,61}[a-zA-Z0-9])?$"));
    if (!valid.match(name).hasMatch()) {
        emit finished(QStringLiteral("hostname"), false,
                      tr("Use letters, digits and hyphens only (not at the start or end)."));
        return;
    }
    run(QStringLiteral("hostname"), QStringLiteral("hostnamectl"), {QStringLiteral("set-hostname"), name},
        {}, tr("Computer name changed to %1.").arg(name));
}

void SystemActions::changePassword(const QString &current, const QString &next)
{
    if (next.size() < 4) {
        emit finished(QStringLiteral("password"), false, tr("Choose a password of at least 4 characters."));
        return;
    }
    if (next.contains(QLatin1Char('\n')) || next.contains(QLatin1Char(':'))) {
        emit finished(QStringLiteral("password"), false, tr("The password can't contain ':' or line breaks."));
        return;
    }
    QString user;
    if (const passwd *pw = getpwuid(getuid()))
        user = QString::fromLocal8Bit(pw->pw_name);
    // sudo reads the current password from the first line of stdin
    // (-S, -k: never use a cached credential); chpasswd reads the rest.
    const QByteArray input = current.toUtf8() + '\n' + user.toUtf8() + ':' + next.toUtf8() + '\n';
    run(QStringLiteral("password"), QStringLiteral("sudo"),
        {QStringLiteral("-S"), QStringLiteral("-k"), QStringLiteral("-p"), QString(), QStringLiteral("chpasswd")},
        input, tr("Your password has been changed."));
}

static QVariantList readXkbSection(const QString &section, const QString &layoutFilter)
{
    QVariantList out;
    QFile f(QStringLiteral("/usr/share/X11/xkb/rules/base.lst"));
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return out;
    bool in = false;
    while (!f.atEnd()) {
        const QString line = QString::fromUtf8(f.readLine()).trimmed();
        if (line.startsWith(QLatin1Char('!'))) {
            in = line == QLatin1String("! ") + section;
            continue;
        }
        if (!in || line.isEmpty())
            continue;
        const int sp = line.indexOf(QRegularExpression(QStringLiteral("\\s")));
        if (sp < 0) continue;
        const QString code = line.left(sp);
        QString desc = line.mid(sp).trimmed();
        if (section == QLatin1String("variant")) {
            // "dvorak          us: English (Dvorak)"
            const QString layout = desc.section(QLatin1Char(':'), 0, 0).trimmed();
            if (layout != layoutFilter) continue;
            desc = desc.section(QLatin1Char(':'), 1).trimmed();
        }
        out << QVariantMap{{QStringLiteral("code"), code}, {QStringLiteral("name"), desc}};
    }
    return out;
}

QVariantList SystemActions::keyboardLayouts() const
{
    QVariantList l = readXkbSection(QStringLiteral("layout"), {});
    if (l.isEmpty()) {
        // Minimal fallback if the XKB database is missing.
        for (const auto &p : {std::pair{"us", "English (US)"}, {"gb", "English (UK)"}, {"de", "German"},
                              {"fr", "French"}, {"es", "Spanish"}, {"it", "Italian"}, {"tr", "Turkish"},
                              {"gr", "Greek"}, {"ru", "Russian"}})
            l << QVariantMap{{QStringLiteral("code"), QString::fromLatin1(p.first)},
                             {QStringLiteral("name"), QString::fromLatin1(p.second)}};
    }
    std::sort(l.begin(), l.end(), [](const QVariant &a, const QVariant &b) {
        return a.toMap().value(QStringLiteral("name")).toString().localeAwareCompare(
                   b.toMap().value(QStringLiteral("name")).toString()) < 0;
    });
    return l;
}

QVariantList SystemActions::keyboardVariants(const QString &layout) const
{
    QVariantList l{QVariantMap{{QStringLiteral("code"), QString()}, {QStringLiteral("name"), tr("Default")}}};
    l << readXkbSection(QStringLiteral("variant"), layout);
    return l;
}
