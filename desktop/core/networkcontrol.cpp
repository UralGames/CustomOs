#include "networkcontrol.h"

#include <QProcess>
#include <QQmlEngine>
#include <QStandardPaths>
#include <QTimer>

#include <functional>

NetworkControl *NetworkControl::create(QQmlEngine *, QJSEngine *)
{
    static NetworkControl *n = new NetworkControl();
    QQmlEngine::setObjectOwnership(n, QQmlEngine::CppOwnership);
    return n;
}

NetworkControl::NetworkControl(QObject *parent)
    : QObject(parent)
{
    m_timer = new QTimer(this);
    m_timer->setInterval(10000);
    connect(m_timer, &QTimer::timeout, this, &NetworkControl::refresh);
    m_timer->start();
    QTimer::singleShot(300, this, &NetworkControl::refresh);
}

void NetworkControl::setActive(bool a)
{
    if (a == m_active)
        return;
    m_active = a;
    m_timer->setInterval(a ? 3000 : 10000);
    if (a)
        refresh();
    emit activeChanged();
}

QStringList NetworkControl::splitTerse(const QString &line)
{
    // nmcli -t separates fields with ':' and escapes ':' and '\' with '\'.
    QStringList out;
    QString cur;
    for (int i = 0; i < line.size(); ++i) {
        const QChar c = line.at(i);
        if (c == QLatin1Char('\\') && i + 1 < line.size()) {
            cur += line.at(++i);
        } else if (c == QLatin1Char(':')) {
            out << cur;
            cur.clear();
        } else {
            cur += c;
        }
    }
    out << cur;
    return out;
}

void NetworkControl::nmcli(const QStringList &args, std::function<void(int, const QString &, const QString &)> done)
{
    auto *p = new QProcess(this);
    p->setProcessChannelMode(QProcess::SeparateChannels);
    connect(p, &QProcess::finished, this, [p, done](int code, QProcess::ExitStatus st) {
        done(st == QProcess::NormalExit ? code : -1,
             QString::fromUtf8(p->readAllStandardOutput()),
             QString::fromUtf8(p->readAllStandardError()).trimmed());
        p->deleteLater();
    });
    connect(p, &QProcess::errorOccurred, this, [p, done](QProcess::ProcessError e) {
        if (e == QProcess::FailedToStart) {
            done(-1, {}, QStringLiteral("nmcli is not installed"));
            p->deleteLater();
        }
    });
    p->start(QStringLiteral("nmcli"), args);
}

void NetworkControl::refresh()
{
    if (m_refreshing)
        return;
    if (QStandardPaths::findExecutable(QStringLiteral("nmcli")).isEmpty()) {
        if (m_available) { m_available = false; m_state = QStringLiteral("unavailable"); emit changed(); }
        return;
    }
    m_refreshing = true;
    nmcli({QStringLiteral("-t"), QStringLiteral("-f"),
           QStringLiteral("RUNNING,STATE,NETWORKING,WIFI-HW,WIFI"), QStringLiteral("general")},
          [this](int code, const QString &out, const QString &) {
        if (code != 0) {
            m_refreshing = false;
            m_available = false;
            m_state = QStringLiteral("unavailable");
            emit changed();
            return;
        }
        const QStringList f = splitTerse(out.trimmed());
        m_available = f.value(0) == QLatin1String("running");
        const QString st = f.value(1);
        m_state = st.startsWith(QLatin1String("connected")) ? QStringLiteral("connected")
                  : st.startsWith(QLatin1String("connecting")) ? QStringLiteral("connecting")
                  : QStringLiteral("disconnected");
        m_networking = f.value(2) == QLatin1String("enabled");
        m_wifi = f.value(4) == QLatin1String("enabled");

        nmcli({QStringLiteral("-t"), QStringLiteral("-f"), QStringLiteral("DEVICE,TYPE,STATE,CONNECTION"),
               QStringLiteral("device")},
              [this](int code, const QString &out, const QString &) {
            m_refreshing = false;
            QVariantList devs;
            bool wifiHw = false;
            QString pType = QStringLiteral("none"), pName;
            QString wifiDev;
            if (code == 0) {
                for (const QString &line : out.split(QLatin1Char('\n'), Qt::SkipEmptyParts)) {
                    const QStringList f = splitTerse(line);
                    const QString type = f.value(1);
                    if (type != QLatin1String("ethernet") && type != QLatin1String("wifi"))
                        continue;   // skip loopback, bridges, p2p
                    if (type == QLatin1String("wifi")) wifiHw = true;
                    const QString state = f.value(2);
                    devs << QVariantMap{{QStringLiteral("device"), f.value(0)},
                                        {QStringLiteral("type"), type},
                                        {QStringLiteral("state"), state},
                                        {QStringLiteral("connection"), f.value(3)}};
                    if (state.startsWith(QLatin1String("connected")) && pType == QLatin1String("none")) {
                        pType = type;
                        pName = f.value(3);
                        if (type == QLatin1String("wifi")) wifiDev = f.value(0);
                    }
                }
            }
            m_devices = devs;
            m_wifiHw = wifiHw;
            m_primaryType = pType;
            m_primaryName = pName;
            emit changed();
            if (pType == QLatin1String("wifi")) {
                nmcli({QStringLiteral("-t"), QStringLiteral("-f"), QStringLiteral("IN-USE,SIGNAL"),
                       QStringLiteral("device"), QStringLiteral("wifi"), QStringLiteral("list"),
                       QStringLiteral("--rescan"), QStringLiteral("no")},
                      [this](int, const QString &out, const QString &) {
                    for (const QString &line : out.split(QLatin1Char('\n'), Qt::SkipEmptyParts)) {
                        const QStringList f = splitTerse(line);
                        if (f.value(0) == QLatin1String("*")) {
                            m_wifiSignal = f.value(1).toInt();
                            emit changed();
                        }
                    }
                });
            }
        });
    });
}

void NetworkControl::scanWifi()
{
    if (m_scanning || !m_wifiHw)
        return;
    m_scanning = true;
    emit accessPointsChanged();
    nmcli({QStringLiteral("-t"), QStringLiteral("-f"), QStringLiteral("IN-USE,SSID,SIGNAL,SECURITY"),
           QStringLiteral("device"), QStringLiteral("wifi"), QStringLiteral("list"),
           QStringLiteral("--rescan"), QStringLiteral("auto")},
          [this](int code, const QString &out, const QString &) {
        QVariantList aps;
        QStringList seen;
        if (code == 0) {
            for (const QString &line : out.split(QLatin1Char('\n'), Qt::SkipEmptyParts)) {
                const QStringList f = splitTerse(line);
                const QString ssid = f.value(1);
                if (ssid.isEmpty() || seen.contains(ssid))
                    continue;   // hidden networks and duplicate BSSIDs
                seen << ssid;
                const QString sec = f.value(3);
                aps << QVariantMap{{QStringLiteral("ssid"), ssid},
                                   {QStringLiteral("signal"), f.value(2).toInt()},
                                   {QStringLiteral("secure"), !sec.isEmpty() && sec != QLatin1String("--")},
                                   {QStringLiteral("inUse"), f.value(0) == QLatin1String("*")}};
            }
        }
        m_aps = aps;
        m_scanning = false;
        emit accessPointsChanged();
    });
}

void NetworkControl::action(const QStringList &args, const QString &successMsg)
{
    m_busy = true;
    emit busyChanged();
    nmcli(args, [this, successMsg](int code, const QString &, const QString &err) {
        m_busy = false;
        emit busyChanged();
        QString msg = err;
        msg.remove(QStringLiteral("Error: "));
        emit actionFinished(code == 0, code == 0 ? successMsg : msg);
        refresh();
        if (m_wifiHw)
            QTimer::singleShot(800, this, &NetworkControl::scanWifi);
    });
}

void NetworkControl::setWifiEnabled(bool on)
{
    m_wifi = on;
    emit changed();
    action({QStringLiteral("radio"), QStringLiteral("wifi"), on ? QStringLiteral("on") : QStringLiteral("off")},
           on ? tr("Wi-Fi turned on") : tr("Wi-Fi turned off"));
}

void NetworkControl::setNetworkingEnabled(bool on)
{
    m_networking = on;
    emit changed();
    action({QStringLiteral("networking"), on ? QStringLiteral("on") : QStringLiteral("off")},
           on ? tr("Networking enabled") : tr("Networking disabled"));
}

void NetworkControl::connectWifi(const QString &ssid, const QString &password)
{
    QStringList args{QStringLiteral("device"), QStringLiteral("wifi"), QStringLiteral("connect"), ssid};
    if (!password.isEmpty())
        args << QStringLiteral("password") << password;
    action(args, tr("Connected to %1").arg(ssid));
}

void NetworkControl::disconnectDevice(const QString &device)
{
    action({QStringLiteral("device"), QStringLiteral("disconnect"), device}, tr("Disconnected"));
}

void NetworkControl::connectDevice(const QString &device)
{
    action({QStringLiteral("device"), QStringLiteral("connect"), device}, tr("Connected"));
}

void NetworkControl::requestDetails(const QString &device)
{
    nmcli({QStringLiteral("-t"), QStringLiteral("-f"),
           QStringLiteral("GENERAL.HWADDR,IP4.ADDRESS,IP4.GATEWAY,IP4.DNS,IP6.ADDRESS"),
           QStringLiteral("device"), QStringLiteral("show"), device},
          [this, device](int, const QString &out, const QString &) {
        QVariantMap d;
        QStringList ip4, dns, ip6;
        for (const QString &line : out.split(QLatin1Char('\n'), Qt::SkipEmptyParts)) {
            const int c = line.indexOf(QLatin1Char(':'));
            const QString key = line.left(c);
            QString val = line.mid(c + 1);
            val.replace(QLatin1String("\\:"), QLatin1String(":"));
            if (val.isEmpty()) continue;
            if (key == QLatin1String("GENERAL.HWADDR")) d[QStringLiteral("mac")] = val;
            else if (key.startsWith(QLatin1String("IP4.ADDRESS"))) ip4 << val;
            else if (key == QLatin1String("IP4.GATEWAY")) d[QStringLiteral("gateway")] = val;
            else if (key.startsWith(QLatin1String("IP4.DNS"))) dns << val;
            else if (key.startsWith(QLatin1String("IP6.ADDRESS"))) ip6 << val;
        }
        d[QStringLiteral("ipv4")] = ip4.join(QStringLiteral(", "));
        d[QStringLiteral("ipv6")] = ip6.join(QStringLiteral(", "));
        d[QStringLiteral("dns")] = dns.join(QStringLiteral(", "));
        emit detailsReady(device, d);
    });
}
