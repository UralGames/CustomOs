#pragma once

#include "dbusnames.h"

#include <QDBusContext>
#include <QObject>
#include <QVariantMap>

class UpdateService;
struct UpdateError;

// org.customos.Update1 at /org/customos/Update1 on the system bus.
//
//   GetStatus() -> a{sv}        everyone
//   GetHistory() -> s (JSON)    everyone
//   CheckForUpdates()           administrators (root or group "wheel")
//   Download(), Install(), Cancel(), Reboot(), Rollback(), DismissError()
//   signal StatusChanged(a{sv}), signal UpdateAvailable(s version, s name)
//
// The D-Bus policy (org.customos.Update1.conf) enforces the same split; the
// service checks the caller again so a lax policy can't open it up.
class UpdateDBusObject : public QObject, protected QDBusContext
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", CUSTOMOS_UPDATE_INTERFACE)
public:
    UpdateDBusObject(UpdateService *service, bool checkCallers, QObject *parent = nullptr);

public slots:
    Q_SCRIPTABLE QVariantMap GetStatus();
    Q_SCRIPTABLE QString GetHistory();
    Q_SCRIPTABLE void CheckForUpdates();
    Q_SCRIPTABLE void Download();
    Q_SCRIPTABLE void Install();
    Q_SCRIPTABLE void Cancel();
    Q_SCRIPTABLE void Reboot();
    Q_SCRIPTABLE void Rollback();
    Q_SCRIPTABLE void DismissError();

signals:
    Q_SCRIPTABLE void StatusChanged(const QVariantMap &status);
    Q_SCRIPTABLE void UpdateAvailable(const QString &version, const QString &name);

private:
    bool authorised();
    void reply(const UpdateError &error);

    UpdateService *m_service;
    bool m_checkCallers;
};

// True for root and for members of the administrators' group ("wheel").
bool isAdministrator(uint uid);
