#include "displaycontrol.h"
#include "settings.h"

#include <QProcess>
#include <QQmlEngine>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QVariantMap>

namespace {
DisplayControl *s_instance = nullptr;
}

DisplayControl *DisplayControl::instance()
{
    if (!s_instance)
        s_instance = new DisplayControl();
    return s_instance;
}

DisplayControl *DisplayControl::create(QQmlEngine *, QJSEngine *)
{
    DisplayControl *d = instance();
    QQmlEngine::setObjectOwnership(d, QQmlEngine::CppOwnership);
    return d;
}

DisplayControl::DisplayControl(QObject *parent)
    : QObject(parent)
{
}

// Parses the human-readable output of `wlr-randr`, e.g.
//   Virtual-1 "VMware Virtual ..."
//     Enabled: yes
//     Modes:
//       1920x1080 px, 60.000000 Hz (preferred, current)
//     Scale: 1.000000
QVariantList DisplayControl::parse(const QString &text)
{
    QVariantList outputs;
    QVariantMap cur;
    QVariantList modes;
    bool inModes = false;
    static const QRegularExpression modeRe(
        QStringLiteral("^\\s+(\\d+)x(\\d+) px(?:, ([\\d.]+) Hz)?(?: \\(([^)]*)\\))?"));

    auto flush = [&] {
        if (!cur.isEmpty()) {
            cur[QStringLiteral("modes")] = modes;
            outputs << cur;
        }
        cur.clear();
        modes.clear();
        inModes = false;
    };

    for (const QString &line : text.split(QLatin1Char('\n'))) {
        if (line.isEmpty())
            continue;
        if (!line.startsWith(QLatin1Char(' '))) {
            flush();
            cur[QStringLiteral("name")] = line.section(QLatin1Char(' '), 0, 0);
            QString desc = line.section(QLatin1Char(' '), 1);
            desc.remove(QLatin1Char('"'));
            cur[QStringLiteral("description")] = desc;
            cur[QStringLiteral("scale")] = 1.0;
            cur[QStringLiteral("enabled")] = true;
            continue;
        }
        const QString t = line.trimmed();
        if (t.startsWith(QLatin1String("Modes:"))) { inModes = true; continue; }
        const auto m = modeRe.match(line);
        if (inModes && m.hasMatch()) {
            const QString flags = m.captured(4);
            const int w = m.captured(1).toInt(), h = m.captured(2).toInt();
            const double hz = m.captured(3).toDouble();
            QVariantMap mode{{QStringLiteral("width"), w}, {QStringLiteral("height"), h},
                             {QStringLiteral("refresh"), hz},
                             {QStringLiteral("preferred"), flags.contains(QLatin1String("preferred"))},
                             {QStringLiteral("current"), flags.contains(QLatin1String("current"))},
                             {QStringLiteral("id"), hz > 0 ? QStringLiteral("%1x%2@%3").arg(w).arg(h).arg(m.captured(3))
                                                           : QStringLiteral("%1x%2").arg(w).arg(h)},
                             {QStringLiteral("label"), hz > 0 ? QStringLiteral("%1 × %2 (%3 Hz)").arg(w).arg(h).arg(qRound(hz))
                                                              : QStringLiteral("%1 × %2").arg(w).arg(h)}};
            modes << mode;
            if (mode.value(QStringLiteral("current")).toBool())
                cur[QStringLiteral("currentMode")] = mode.value(QStringLiteral("id"));
            continue;
        }
        inModes = false;
        if (t.startsWith(QLatin1String("Scale:")))
            cur[QStringLiteral("scale")] = t.section(QLatin1Char(' '), 1).toDouble();
        else if (t.startsWith(QLatin1String("Enabled:")))
            cur[QStringLiteral("enabled")] = t.endsWith(QLatin1String("yes"));
    }
    flush();
    return outputs;
}

void DisplayControl::refresh()
{
    if (QStandardPaths::findExecutable(QStringLiteral("wlr-randr")).isEmpty()
        || qEnvironmentVariableIsEmpty("WAYLAND_DISPLAY")) {
        if (m_available) { m_available = false; emit changed(); }
        return;
    }
    auto *p = new QProcess(this);
    connect(p, &QProcess::finished, this, [this, p](int code) {
        const QString out = QString::fromUtf8(p->readAllStandardOutput());
        p->deleteLater();
        m_available = code == 0;
        m_outputs = m_available ? parse(out) : QVariantList();
        emit changed();
    });
    connect(p, &QProcess::errorOccurred, p, &QObject::deleteLater);
    p->start(QStringLiteral("wlr-randr"), {});
}

void DisplayControl::run(const QStringList &args, const QString &okMessage)
{
    m_busy = true;
    emit busyChanged();
    auto *p = new QProcess(this);
    p->setProcessChannelMode(QProcess::MergedChannels);
    connect(p, &QProcess::finished, this, [this, p, okMessage](int code) {
        const QString out = QString::fromUtf8(p->readAll()).trimmed();
        p->deleteLater();
        m_busy = false;
        emit busyChanged();
        emit applied(code == 0, code == 0 ? okMessage : out);
        refresh();
    });
    connect(p, &QProcess::errorOccurred, this, [this, p](QProcess::ProcessError e) {
        if (e != QProcess::FailedToStart) return;
        p->deleteLater();
        m_busy = false;
        emit busyChanged();
        emit applied(false, tr("wlr-randr is not installed."));
    });
    p->start(QStringLiteral("wlr-randr"), args);
}

void DisplayControl::setMode(const QString &output, const QString &mode, bool remember)
{
    QStringList args{QStringLiteral("--output"), output};
    if (mode.isEmpty()) {
        args << QStringLiteral("--preferred");
    } else {
        QString m = mode;
        if (m.contains(QLatin1Char('@')) && !m.endsWith(QLatin1String("Hz")))
            m += QStringLiteral("Hz");
        args << QStringLiteral("--mode") << m;
    }
    if (remember)
        Settings::instance()->set(QStringLiteral("displayMode"), mode.isEmpty() ? QString() : output + QLatin1Char(' ') + mode);
    run(args, tr("Display resolution changed."));
}

void DisplayControl::setScale(const QString &output, double scale, bool remember)
{
    if (remember)
        Settings::instance()->set(QStringLiteral("displayScale"), scale);
    run({QStringLiteral("--output"), output, QStringLiteral("--scale"), QString::number(scale, 'f', 2)},
        tr("Display scale changed."));
}

void DisplayControl::applySaved()
{
    const QString saved = Settings::instance()->value(QStringLiteral("displayMode")).toString();
    const double scale = Settings::instance()->value(QStringLiteral("displayScale")).toDouble();
    if (!saved.isEmpty()) {
        const QString output = saved.section(QLatin1Char(' '), 0, 0);
        const QString mode = saved.section(QLatin1Char(' '), 1);
        setMode(output, mode, false);
    } else {
        // No resolution chosen yet. meridian-session exports
        // MERIDIAN_DEFAULT_MODE (WIDTHxHEIGHT) inside virtual machines, whose
        // virtual GPUs otherwise start at a small mode such as 1024x768.
        // It is applied only if the first output actually offers that mode.
        const QString wanted = qEnvironmentVariable("MERIDIAN_DEFAULT_MODE");
        static const QRegularExpression sizeRe(QStringLiteral("^(\\d+)x(\\d+)$"));
        const auto m = sizeRe.match(wanted);
        if (m.hasMatch()) {
            const int w = m.captured(1).toInt(), h = m.captured(2).toInt();
            auto *conn = new QMetaObject::Connection;
            *conn = connect(this, &DisplayControl::changed, this, [this, w, h, conn] {
                disconnect(*conn);
                delete conn;
                if (m_outputs.isEmpty())
                    return;
                const QVariantMap out = m_outputs.first().toMap();
                for (const QVariant &v : out.value(QStringLiteral("modes")).toList()) {
                    const QVariantMap mode = v.toMap();
                    if (mode.value(QStringLiteral("width")).toInt() != w
                        || mode.value(QStringLiteral("height")).toInt() != h)
                        continue;
                    if (!mode.value(QStringLiteral("current")).toBool())
                        setMode(out.value(QStringLiteral("name")).toString(),
                                mode.value(QStringLiteral("id")).toString(), false);
                    break;
                }
            });
        }
    }
    if (scale > 0 && qAbs(scale - 1.0) > 0.01) {
        // Apply scale to the first output once its list is known.
        auto *conn = new QMetaObject::Connection;
        *conn = connect(this, &DisplayControl::changed, this, [this, scale, conn] {
            disconnect(*conn);
            delete conn;
            if (!m_outputs.isEmpty())
                setScale(m_outputs.first().toMap().value(QStringLiteral("name")).toString(), scale, false);
        });
    }
    refresh();
}
