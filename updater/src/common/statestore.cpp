#include "statestore.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>

StateStore::StateStore(const QString &directory)
    : m_dir(directory)
{
}

static QDateTime parseTime(const QJsonValue &v)
{
    return QDateTime::fromString(v.toString(), Qt::ISODate);
}

static QString formatTime(const QDateTime &t)
{
    return t.isValid() ? t.toUTC().toString(Qt::ISODate) : QString();
}

bool StateStore::load()
{
    m_error.clear();
    QFile sf(m_dir + QStringLiteral("/state.json"));
    if (sf.open(QIODevice::ReadOnly)) {
        const QJsonObject o = QJsonDocument::fromJson(sf.readAll()).object();
        const QJsonObject p = o.value(QLatin1String("pending")).toObject();
        m_pending.kind = p.value(QLatin1String("kind")).toString();
        m_pending.fromVersion = p.value(QLatin1String("from_version")).toString();
        m_pending.toVersion = p.value(QLatin1String("to_version")).toString();
        m_pending.fromSlot = p.value(QLatin1String("from_slot")).toString();
        m_pending.targetSlot = p.value(QLatin1String("target_slot")).toString();
        m_pending.bootId = p.value(QLatin1String("boot_id")).toString();
        m_pending.when = parseTime(p.value(QLatin1String("time")));
        const QJsonObject s = o.value(QLatin1String("last_success")).toObject();
        m_lastSuccessVersion = s.value(QLatin1String("version")).toString();
        m_lastSuccessTime = parseTime(s.value(QLatin1String("time")));
        m_lastCheck = parseTime(o.value(QLatin1String("last_check")));
    }
    QFile hf(m_dir + QStringLiteral("/history.json"));
    if (hf.open(QIODevice::ReadOnly))
        m_history = QJsonDocument::fromJson(hf.readAll()).array();
    return true;
}

bool StateStore::writeJson(const QString &name, const QByteArray &data)
{
    if (!QDir().mkpath(m_dir)) {
        m_error = QStringLiteral("cannot create %1").arg(m_dir);
        return false;
    }
    QSaveFile f(m_dir + QLatin1Char('/') + name);
    if (!f.open(QIODevice::WriteOnly) || f.write(data) != data.size() || !f.commit()) {
        m_error = QStringLiteral("cannot write %1/%2: %3").arg(m_dir, name, f.errorString());
        return false;
    }
    return true;
}

bool StateStore::save()
{
    QJsonObject o;
    if (m_pending.isValid()) {
        o.insert(QStringLiteral("pending"), QJsonObject{
            {QStringLiteral("kind"), m_pending.kind},
            {QStringLiteral("from_version"), m_pending.fromVersion},
            {QStringLiteral("to_version"), m_pending.toVersion},
            {QStringLiteral("from_slot"), m_pending.fromSlot},
            {QStringLiteral("target_slot"), m_pending.targetSlot},
            {QStringLiteral("boot_id"), m_pending.bootId},
            {QStringLiteral("time"), formatTime(m_pending.when)}});
    }
    if (!m_lastSuccessVersion.isEmpty()) {
        o.insert(QStringLiteral("last_success"), QJsonObject{
            {QStringLiteral("version"), m_lastSuccessVersion},
            {QStringLiteral("time"), formatTime(m_lastSuccessTime)}});
    }
    if (m_lastCheck.isValid())
        o.insert(QStringLiteral("last_check"), formatTime(m_lastCheck));
    return writeJson(QStringLiteral("state.json"), QJsonDocument(o).toJson())
        && writeJson(QStringLiteral("history.json"), QJsonDocument(m_history).toJson());
}

void StateStore::setLastSuccess(const QString &version, const QDateTime &when)
{
    m_lastSuccessVersion = version;
    m_lastSuccessTime = when;
}

void StateStore::addHistory(const QString &event, const QString &result, const QString &fromVersion,
                            const QString &toVersion, const QString &message)
{
    m_history.append(QJsonObject{
        {QStringLiteral("time"), formatTime(QDateTime::currentDateTimeUtc())},
        {QStringLiteral("event"), event},
        {QStringLiteral("result"), result},
        {QStringLiteral("from_version"), fromVersion},
        {QStringLiteral("to_version"), toVersion},
        {QStringLiteral("message"), message}});
    while (m_history.size() > MaxHistory)
        m_history.removeFirst();
}

BootReport StateStore::lastBootReport() const
{
    BootReport r;
    QFile f(m_dir + QStringLiteral("/last-boot.env"));
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return r;
    while (!f.atEnd()) {
        const QString line = QString::fromUtf8(f.readLine()).trimmed();
        const int eq = line.indexOf(QLatin1Char('='));
        if (eq <= 0)
            continue;
        const QString key = line.left(eq);
        QString value = line.mid(eq + 1);
        if (value.size() >= 2 && value.startsWith(QLatin1Char('"')) && value.endsWith(QLatin1Char('"')))
            value = value.mid(1, value.size() - 2);
        if (key == QLatin1String("SLOT")) r.slot = value;
        else if (key == QLatin1String("VERSION")) r.version = value;
        else if (key == QLatin1String("RESULT")) r.result = value;
        else if (key == QLatin1String("REASON")) r.reason = value;
        else if (key == QLatin1String("TIME")) r.when = QDateTime::fromSecsSinceEpoch(value.toLongLong()).toUTC();
    }
    return r;
}
