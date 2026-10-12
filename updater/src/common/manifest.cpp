#include "manifest.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>

bool isSha256Hex(const QByteArray &hex)
{
    static const QRegularExpression re(QStringLiteral("^[0-9a-f]{64}$"));
    return re.match(QString::fromLatin1(hex)).hasMatch();
}

static bool fail(QString *error, const QString &message)
{
    if (error)
        *error = message;
    return false;
}

bool ReleaseManifest::parse(const QByteArray &json, ReleaseManifest *out, QString *error)
{
    QJsonParseError pe;
    const QJsonDocument doc = QJsonDocument::fromJson(json, &pe);
    if (pe.error != QJsonParseError::NoError || !doc.isObject())
        return fail(error, QStringLiteral("not valid JSON (%1)").arg(pe.errorString()));
    const QJsonObject o = doc.object();

    ReleaseManifest m;
    m.schema = o.value(QLatin1String("schema")).toInt(0);
    if (m.schema != 1)
        return fail(error, QStringLiteral("unsupported manifest schema %1").arg(m.schema));
    m.product = o.value(QLatin1String("product")).toString();
    if (m.product != QLatin1String("CustomOS"))
        return fail(error, QStringLiteral("the manifest is for \"%1\", not CustomOS").arg(m.product));
    m.version = Version::parse(o.value(QLatin1String("version")).toString());
    if (!m.version.isValid())
        return fail(error, QStringLiteral("missing or invalid \"version\""));
    m.channel = o.value(QLatin1String("channel")).toString(QStringLiteral("stable"));
    m.compatible = o.value(QLatin1String("compatible")).toString();
    if (m.compatible.isEmpty())
        return fail(error, QStringLiteral("missing \"compatible\""));
    m.released = QDateTime::fromString(o.value(QLatin1String("released")).toString(), Qt::ISODate);
    if (o.contains(QLatin1String("min_version"))) {
        m.minVersion = Version::parse(o.value(QLatin1String("min_version")).toString());
        if (!m.minVersion.isValid())
            return fail(error, QStringLiteral("invalid \"min_version\""));
    }
    m.rebootRequired = o.value(QLatin1String("reboot_required")).toBool(true);

    const QJsonObject b = o.value(QLatin1String("bundle")).toObject();
    m.bundleName = b.value(QLatin1String("name")).toString();
    // Only a plain file name: it is used to build a path in the cache directory.
    static const QRegularExpression safeName(QStringLiteral("^[A-Za-z0-9][A-Za-z0-9._+-]*\\.raucb$"));
    if (!safeName.match(m.bundleName).hasMatch())
        return fail(error, QStringLiteral("missing or unsafe bundle file name \"%1\"").arg(m.bundleName));
    const double size = b.value(QLatin1String("size")).toDouble(-1);
    if (size <= 0 || size > 64.0 * 1024 * 1024 * 1024)
        return fail(error, QStringLiteral("invalid bundle size"));
    m.bundleSize = qint64(size);
    m.bundleSha256 = b.value(QLatin1String("sha256")).toString().toLower().toLatin1();
    if (!isSha256Hex(m.bundleSha256))
        return fail(error, QStringLiteral("invalid bundle sha256"));

    m.changelog = o.value(QLatin1String("changelog")).toString();
    const QJsonObject build = o.value(QLatin1String("build")).toObject();
    m.buildId = build.value(QLatin1String("id")).toString();
    m.gitCommit = build.value(QLatin1String("git_commit")).toString();

    *out = m;
    return true;
}

Sha256Sums Sha256Sums::parse(const QByteArray &text)
{
    Sha256Sums sums;
    for (const QByteArray &rawLine : text.split('\n')) {
        const QByteArray line = rawLine.trimmed();
        if (line.isEmpty() || line.startsWith('#'))
            continue;
        const int sp = line.indexOf(' ');
        if (sp != 64)
            continue;
        const QByteArray hex = line.left(64).toLower();
        if (!isSha256Hex(hex))
            continue;
        QByteArray name = line.mid(65).trimmed();
        if (name.startsWith('*'))   // binary-mode marker
            name = name.mid(1);
        if (!name.isEmpty())
            sums.m_sums.insert(QString::fromUtf8(name), hex);
    }
    return sums;
}
