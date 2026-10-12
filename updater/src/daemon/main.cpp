// customos-updated: the CustomOS over-the-air update service.
//
//   customos-updated [--session]
//
// Runs as root, owns org.customos.Update1 on the system bus (--session uses
// the session bus, for development and tests) and is started by systemd
// (customos-updated.service) or on demand by D-Bus activation.

#include "config.h"
#include "dbusadaptor.h"
#include "dbusnames.h"
#include "updateservice.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusError>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("customos-updated"));
    QCoreApplication::setApplicationVersion(QStringLiteral(CUSTOMOS_UPDATER_VERSION));

    const bool session = app.arguments().contains(QStringLiteral("--session"))
                         || qEnvironmentVariable("CUSTOMOS_UPDATE_BUS") == QLatin1String("session");
    QDBusConnection bus = session ? QDBusConnection::sessionBus() : QDBusConnection::systemBus();
    if (!bus.isConnected()) {
        qCritical("customos-updated: cannot connect to the D-Bus %s bus: %s", session ? "session" : "system",
                  qPrintable(bus.lastError().message()));
        return 1;
    }

    const UpdateConfig config = UpdateConfig::load();
    qInfo("customos-updated %s: repository %s, channel %s, compatible %s (config: %s)", CUSTOMOS_UPDATER_VERSION,
          qPrintable(config.repository.isEmpty() ? QStringLiteral("<none>") : config.repository),
          qPrintable(config.channel), qPrintable(config.compatible),
          qPrintable(config.loadedFrom.isEmpty() ? QStringLiteral("built-in defaults") : config.loadedFrom));

    UpdateService service(config, bus);
    // On the session bus every caller is the developer running the tests.
    UpdateDBusObject object(&service, !session);
    if (!bus.registerObject(QStringLiteral(CUSTOMOS_UPDATE_PATH), &object,
                            QDBusConnection::ExportScriptableSlots | QDBusConnection::ExportScriptableSignals)) {
        qCritical("customos-updated: cannot register %s", CUSTOMOS_UPDATE_PATH);
        return 1;
    }
    if (!bus.registerService(QStringLiteral(CUSTOMOS_UPDATE_SERVICE))) {
        qCritical("customos-updated: cannot own %s: %s (already running?)", CUSTOMOS_UPDATE_SERVICE,
                  qPrintable(bus.lastError().message()));
        return 1;
    }
    service.start();
    return app.exec();
}
