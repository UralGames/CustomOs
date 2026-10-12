#include "config.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSettings>

static void apply(UpdateConfig &c, const QString &file)
{
    if (!QFileInfo::exists(file))
        return;
    QSettings s(file, QSettings::IniFormat);
    auto str = [&s](const char *key, const QString &def) {
        return s.value(QLatin1String(key), def).toString().trimmed();
    };
    auto boolean = [&s](const char *key, bool def) {
        const QString v = s.value(QLatin1String(key)).toString().trimmed().toLower();
        if (v.isEmpty()) return def;
        return v == QLatin1String("true") || v == QLatin1String("yes") || v == QLatin1String("1");
    };

    if (s.contains(QStringLiteral("Source/Repository")))
        c.repository = UpdateConfig::normaliseRepository(str("Source/Repository", c.repository));
    const QUrl api(str("Source/ApiUrl", c.apiUrl.toString()));
    if (api.isValid() && !api.scheme().isEmpty())
        c.apiUrl = api;
    c.channel = str("Source/Channel", c.channel).toLower();
    c.tokenFile = str("Source/TokenFile", c.tokenFile);
    c.compatible = str("Source/Compatible", c.compatible);

    c.automaticCheck = boolean("Updates/AutomaticCheck", c.automaticCheck);
    bool ok = false;
    const int hours = s.value(QStringLiteral("Updates/CheckIntervalHours")).toInt(&ok);
    if (ok && hours >= 1 && hours <= 24 * 30)
        c.checkIntervalHours = hours;
    c.stateDirectory = str("Updates/StateDirectory", c.stateDirectory);
    c.cacheDirectory = str("Updates/CacheDirectory", c.cacheDirectory);
    c.allowDowngrade = boolean("Updates/AllowDowngrade", c.allowDowngrade);
    c.allowInsecureHttp = boolean("Updates/AllowInsecureHttp", c.allowInsecureHttp);

    if (!c.loadedFrom.isEmpty())
        c.loadedFrom += QLatin1String(", ");
    c.loadedFrom += file;
}

UpdateConfig UpdateConfig::loadFrom(const QString &mainFile, const QString &dropInDir)
{
    UpdateConfig c;
    c.repository = normaliseRepository(QStringLiteral(CUSTOMOS_DEFAULT_REPOSITORY));
    apply(c, mainFile);
    if (!dropInDir.isEmpty()) {
        const QStringList files = QDir(dropInDir).entryList({QStringLiteral("*.conf")}, QDir::Files, QDir::Name);
        for (const QString &f : files)
            apply(c, dropInDir + QLatin1Char('/') + f);
    }
    return c;
}

UpdateConfig UpdateConfig::load()
{
    const QString env = qEnvironmentVariable("CUSTOMOS_UPDATE_CONFIG");
    if (!env.isEmpty())
        return loadFrom(env, QString());
    const QString base = QStringLiteral(CUSTOMOS_SYSCONFDIR "/customos");
    return loadFrom(base + QStringLiteral("/update.conf"), base + QStringLiteral("/update.conf.d"));
}

QString UpdateConfig::normaliseRepository(const QString &text)
{
    QString t = text.trimmed();
    t.remove(QRegularExpression(QStringLiteral("^(https?://)?(www\\.)?github\\.com/")));
    if (t.endsWith(QLatin1String(".git")))
        t.chop(4);
    while (t.endsWith(QLatin1Char('/')))
        t.chop(1);
    // GitHub owner and repository names: letters, digits, '-', '_' and '.'.
    static const QRegularExpression re(QStringLiteral("^[A-Za-z0-9](?:[A-Za-z0-9-]{0,38})/[A-Za-z0-9._-]{1,100}$"));
    return re.match(t).hasMatch() ? t : QString();
}

QString UpdateConfig::token() const
{
    if (tokenFile.isEmpty())
        return QString();
    QFile f(tokenFile);
    if (!f.open(QIODevice::ReadOnly))
        return QString();
    return QString::fromUtf8(f.read(4096)).trimmed();
}
