#include "audiocontrol.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QQmlEngine>
#include <QStandardPaths>
#include <QTimer>

namespace {
AudioControl *s_instance = nullptr;
const QString kSink = QStringLiteral("@DEFAULT_AUDIO_SINK@");
}

AudioControl *AudioControl::instance()
{
    if (!s_instance)
        s_instance = new AudioControl();
    return s_instance;
}

AudioControl *AudioControl::create(QQmlEngine *, QJSEngine *)
{
    AudioControl *a = instance();
    QQmlEngine::setObjectOwnership(a, QQmlEngine::CppOwnership);
    return a;
}

AudioControl::AudioControl(QObject *parent)
    : QObject(parent)
{
    m_timer = new QTimer(this);
    m_timer->setInterval(8000);
    connect(m_timer, &QTimer::timeout, this, &AudioControl::refresh);
    m_timer->start();

    // Coalesce rapid slider movement into one wpctl call per 60 ms.
    m_setTimer = new QTimer(this);
    m_setTimer->setSingleShot(true);
    m_setTimer->setInterval(60);
    connect(m_setTimer, &QTimer::timeout, this, [this] {
        if (m_pendingVolume < 0)
            return;
        run({QStringLiteral("set-volume"), kSink,
             QString::number(m_pendingVolume / 100.0, 'f', 2)});
        m_pendingVolume = -1;
    });

    QTimer::singleShot(500, this, [this] { refresh(); refreshDevices(); });
}

void AudioControl::setActive(bool a)
{
    if (m_active == a)
        return;
    m_active = a;
    m_timer->setInterval(a ? 1500 : 8000);
    if (a) {
        refresh();
        refreshDevices();
    }
    emit activeChanged();
}

void AudioControl::run(const QStringList &args)
{
    auto *p = new QProcess(this);
    connect(p, &QProcess::finished, p, &QObject::deleteLater);
    connect(p, &QProcess::errorOccurred, p, &QObject::deleteLater);
    p->start(QStringLiteral("wpctl"), args);
}

void AudioControl::refresh()
{
    if (m_queryRunning)
        return;
    if (QStandardPaths::findExecutable(QStringLiteral("wpctl")).isEmpty()) {
        if (m_available) { m_available = false; emit changed(); }
        return;
    }
    m_queryRunning = true;
    auto *p = new QProcess(this);
    connect(p, &QProcess::finished, this, [this, p](int code) {
        m_queryRunning = false;
        // Output: "Volume: 0.40" or "Volume: 0.40 [MUTED]"
        const QString out = QString::fromUtf8(p->readAllStandardOutput()).trimmed();
        p->deleteLater();
        const bool ok = code == 0 && out.startsWith(QLatin1String("Volume:"));
        if (!ok) {
            if (m_available) { m_available = false; emit changed(); }
            return;
        }
        const int vol = qRound(out.section(QLatin1Char(' '), 1, 1).toDouble() * 100.0);
        const bool muted = out.contains(QLatin1String("[MUTED]"));
        // Ignore stale readings while the user is dragging the slider.
        if (m_pendingVolume >= 0 || m_setTimer->isActive())
            return;
        if (!m_available || vol != m_volume || muted != m_muted) {
            m_available = true;
            m_volume = vol;
            m_muted = muted;
            emit changed();
        }
    });
    connect(p, &QProcess::errorOccurred, this, [this, p] {
        m_queryRunning = false;
        p->deleteLater();
        if (m_available) { m_available = false; emit changed(); }
    });
    p->start(QStringLiteral("wpctl"), {QStringLiteral("get-volume"), kSink});
}

void AudioControl::refreshDevices()
{
    if (QStandardPaths::findExecutable(QStringLiteral("pw-dump")).isEmpty())
        return;
    auto *p = new QProcess(this);
    connect(p, &QProcess::finished, this, [this, p] {
        const QJsonArray objs = QJsonDocument::fromJson(p->readAllStandardOutput()).array();
        p->deleteLater();
        QString defaultName;
        for (const QJsonValue &v : objs) {
            const QJsonObject o = v.toObject();
            if (o.value(QLatin1String("type")).toString() != QLatin1String("PipeWire:Interface:Metadata"))
                continue;
            if (o.value(QLatin1String("props")).toObject().value(QLatin1String("metadata.name")).toString()
                != QLatin1String("default"))
                continue;
            for (const QJsonValue &m : o.value(QLatin1String("metadata")).toArray()) {
                const QJsonObject mo = m.toObject();
                if (mo.value(QLatin1String("key")).toString() == QLatin1String("default.audio.sink"))
                    defaultName = mo.value(QLatin1String("value")).toObject().value(QLatin1String("name")).toString();
            }
        }
        QVariantList sinks;
        for (const QJsonValue &v : objs) {
            const QJsonObject o = v.toObject();
            if (o.value(QLatin1String("type")).toString() != QLatin1String("PipeWire:Interface:Node"))
                continue;
            const QJsonObject props = o.value(QLatin1String("info")).toObject().value(QLatin1String("props")).toObject();
            if (props.value(QLatin1String("media.class")).toString() != QLatin1String("Audio/Sink"))
                continue;
            const QString nodeName = props.value(QLatin1String("node.name")).toString();
            QString desc = props.value(QLatin1String("node.description")).toString();
            if (desc.isEmpty()) desc = props.value(QLatin1String("node.nick")).toString();
            if (desc.isEmpty()) desc = nodeName;
            sinks << QVariantMap{{QStringLiteral("id"), o.value(QLatin1String("id")).toInt()},
                                 {QStringLiteral("name"), desc},
                                 {QStringLiteral("isDefault"), nodeName == defaultName}};
            if (nodeName == defaultName)
                defaultName = desc;
        }
        if (sinks != m_sinks || defaultName != m_defaultSinkName) {
            m_sinks = sinks;
            m_defaultSinkName = defaultName;
            emit sinksChanged();
        }
    });
    connect(p, &QProcess::errorOccurred, p, &QObject::deleteLater);
    p->start(QStringLiteral("pw-dump"), {});
}

void AudioControl::setVolume(int percent)
{
    percent = qBound(0, percent, 100);
    if (percent == m_volume && m_pendingVolume < 0)
        return;
    m_volume = percent;
    m_pendingVolume = percent;
    m_setTimer->start();
    if (m_muted && percent > 0) {
        m_muted = false;
        run({QStringLiteral("set-mute"), kSink, QStringLiteral("0")});
    }
    emit changed();
    emit volumeAdjusted();
}

void AudioControl::changeVolume(int delta)
{
    setVolume(m_volume + delta);
}

void AudioControl::setMuted(bool m)
{
    m_muted = m;
    run({QStringLiteral("set-mute"), kSink, m ? QStringLiteral("1") : QStringLiteral("0")});
    emit changed();
    emit volumeAdjusted();
}

void AudioControl::setDefaultSink(int id)
{
    auto *p = new QProcess(this);
    connect(p, &QProcess::finished, this, [this, p] {
        p->deleteLater();
        refresh();
        refreshDevices();
    });
    connect(p, &QProcess::errorOccurred, p, &QObject::deleteLater);
    p->start(QStringLiteral("wpctl"), {QStringLiteral("set-default"), QString::number(id)});
}
