#pragma once

#include <QObject>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

class QTimer;
class QQmlEngine;
class QJSEngine;

// Output volume and device selection through PipeWire/WirePlumber.
// Uses `wpctl` for volume/mute/default and `pw-dump` (JSON) to list sinks.
// If PipeWire is not running, `available` is false and the UI says so.
class AudioControl : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Audio)
    QML_SINGLETON
    Q_PROPERTY(bool available READ available NOTIFY changed)
    Q_PROPERTY(int volume READ volume NOTIFY changed)
    Q_PROPERTY(bool muted READ muted NOTIFY changed)
    Q_PROPERTY(QVariantList sinks READ sinks NOTIFY sinksChanged)
    Q_PROPERTY(QString defaultSinkName READ defaultSinkName NOTIFY sinksChanged)
    // While a UI showing audio is open, poll faster.
    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged)

public:
    static AudioControl *instance();
    static AudioControl *create(QQmlEngine *, QJSEngine *);

    bool available() const { return m_available; }
    int volume() const { return m_volume; }
    bool muted() const { return m_muted; }
    QVariantList sinks() const { return m_sinks; }
    QString defaultSinkName() const { return m_defaultSinkName; }
    bool active() const { return m_active; }
    void setActive(bool a);

    Q_INVOKABLE void setVolume(int percent);
    Q_INVOKABLE void changeVolume(int deltaPercent);
    Q_INVOKABLE void setMuted(bool m);
    Q_INVOKABLE void toggleMute() { setMuted(!m_muted); }
    Q_INVOKABLE void setDefaultSink(int id);
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void refreshDevices();

signals:
    void changed();
    void sinksChanged();
    void activeChanged();
    // Emitted for user-initiated changes (drives the on-screen indicator).
    void volumeAdjusted();

private:
    explicit AudioControl(QObject *parent = nullptr);
    void run(const QStringList &args);

    bool m_available = false;
    int m_volume = 0;
    bool m_muted = false;
    bool m_active = false;
    bool m_queryRunning = false;
    QVariantList m_sinks;
    QString m_defaultSinkName;
    QTimer *m_timer = nullptr;
    QTimer *m_setTimer = nullptr;
    int m_pendingVolume = -1;
};
