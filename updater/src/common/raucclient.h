#pragma once

#include <QDBusConnection>
#include <QList>
#include <QObject>
#include <QString>
#include <QVariantMap>

#include <functional>

// Thin asynchronous client for RAUC's D-Bus API (de.pengutronix.rauc.Installer).
// RAUC does the security-critical work: it verifies the bundle signature
// against /etc/rauc/keyring.pem, writes the image into the inactive slot and
// switches the boot order in the GRUB environment.
class RaucClient : public QObject
{
    Q_OBJECT
public:
    struct BundleInfo
    {
        QString compatible;
        QString version;
        QString build;
        QString description;
        QString format;
    };
    struct Slot
    {
        QString name;          // "rootfs.0"
        QString bootname;      // "A"
        QString state;         // "booted", "active", "inactive"
        QString bootStatus;    // "good", "bad"
        QString bundleVersion;
        QString installedTimestamp;
        QString device;
    };

    explicit RaucClient(const QDBusConnection &bus, QObject *parent = nullptr);

    bool isAvailable() const;
    QString compatible() const;
    QString bootSlot() const;
    QString lastError() const;
    QString operation() const;

    // Verifies the signature and reads the signed manifest of a bundle.
    void inspect(const QString &bundlePath, std::function<void(const BundleInfo &, const QString &error)> done);
    // Starts an installation; progress and completion arrive as signals.
    void install(const QString &bundlePath, std::function<void(const QString &error)> started);
    // state: "good", "bad" or "active"; slot: "booted", "other" or a slot name.
    void mark(const QString &state, const QString &slot,
              std::function<void(const QString &slotName, const QString &message, const QString &error)> done);
    void slotStatus(std::function<void(const QList<Slot> &slotList, const QString &error)> done);

    static const char *const Service;
    static const char *const Path;
    static const char *const Interface;

signals:
    void progressChanged(int percent, const QString &message);
    void completed(int result);

private slots:
    void onPropertiesChanged(const QString &interface, const QVariantMap &changed, const QStringList &invalidated);
    void onCompleted(int result);

private:
    QVariant property(const char *name) const;

    QDBusConnection m_bus;
};
