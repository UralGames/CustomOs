#pragma once

#include <QObject>
#include <QVariantList>
#include <functional>
#include <QtQml/qqmlregistration.h>

class QTimer;
class QQmlEngine;
class QJSEngine;

// Network status and control through NetworkManager's `nmcli`.
class NetworkControl : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Network)
    QML_SINGLETON
    Q_PROPERTY(bool available READ available NOTIFY changed)
    Q_PROPERTY(bool networkingEnabled READ networkingEnabled NOTIFY changed)
    Q_PROPERTY(bool wifiEnabled READ wifiEnabled NOTIFY changed)
    Q_PROPERTY(bool wifiHardware READ wifiHardware NOTIFY changed)   // a Wi-Fi adapter exists
    Q_PROPERTY(QString state READ state NOTIFY changed)              // connected / connecting / disconnected / unavailable
    Q_PROPERTY(QString primaryType READ primaryType NOTIFY changed)  // ethernet / wifi / none
    Q_PROPERTY(QString primaryName READ primaryName NOTIFY changed)
    Q_PROPERTY(int wifiSignal READ wifiSignal NOTIFY changed)
    Q_PROPERTY(QVariantList devices READ devices NOTIFY changed)
    Q_PROPERTY(QVariantList accessPoints READ accessPoints NOTIFY accessPointsChanged)
    Q_PROPERTY(bool scanning READ scanning NOTIFY accessPointsChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged)

public:
    static NetworkControl *create(QQmlEngine *, QJSEngine *);

    bool available() const { return m_available; }
    bool networkingEnabled() const { return m_networking; }
    bool wifiEnabled() const { return m_wifi; }
    bool wifiHardware() const { return m_wifiHw; }
    QString state() const { return m_state; }
    QString primaryType() const { return m_primaryType; }
    QString primaryName() const { return m_primaryName; }
    int wifiSignal() const { return m_wifiSignal; }
    QVariantList devices() const { return m_devices; }
    QVariantList accessPoints() const { return m_aps; }
    bool scanning() const { return m_scanning; }
    bool busy() const { return m_busy; }
    bool active() const { return m_active; }
    void setActive(bool a);

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void scanWifi();
    Q_INVOKABLE void setWifiEnabled(bool on);
    Q_INVOKABLE void setNetworkingEnabled(bool on);
    Q_INVOKABLE void connectWifi(const QString &ssid, const QString &password = {});
    Q_INVOKABLE void disconnectDevice(const QString &device);
    Q_INVOKABLE void connectDevice(const QString &device);
    // IP details for one device: address, gateway, dns, mac.
    Q_INVOKABLE void requestDetails(const QString &device);

    static QStringList splitTerse(const QString &line);

signals:
    void changed();
    void accessPointsChanged();
    void busyChanged();
    void activeChanged();
    void actionFinished(bool ok, const QString &message);
    void detailsReady(const QString &device, const QVariantMap &details);

private:
    explicit NetworkControl(QObject *parent = nullptr);
    void nmcli(const QStringList &args, std::function<void(int, const QString &, const QString &)> done);
    void action(const QStringList &args, const QString &successMsg);

    bool m_available = false, m_networking = false, m_wifi = false, m_wifiHw = false;
    QString m_state = QStringLiteral("unavailable"), m_primaryType = QStringLiteral("none"), m_primaryName;
    int m_wifiSignal = 0;
    QVariantList m_devices, m_aps;
    bool m_scanning = false, m_busy = false, m_active = false, m_refreshing = false;
    QTimer *m_timer = nullptr;
};
