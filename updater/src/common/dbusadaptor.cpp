#include "dbusadaptor.h"

#include "updateservice.h"

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusReply>
#include <QVector>

#include <grp.h>
#include <pwd.h>
#include <unistd.h>

UpdateDBusObject::UpdateDBusObject(UpdateService *service, bool checkCallers, QObject *parent)
    : QObject(parent), m_service(service), m_checkCallers(checkCallers)
{
    connect(service, &UpdateService::statusChanged, this, &UpdateDBusObject::StatusChanged);
    connect(service, &UpdateService::updateAvailable, this, &UpdateDBusObject::UpdateAvailable);
}

bool isAdministrator(uint uid)
{
    if (uid == 0)
        return true;
    const passwd *pw = getpwuid(uid);
    const group *wheel = getgrnam("wheel");
    if (!pw || !wheel)
        return false;
    int count = 64;
    QVector<gid_t> groups(count);
    if (getgrouplist(pw->pw_name, pw->pw_gid, groups.data(), &count) < 0) {
        groups.resize(count);
        if (getgrouplist(pw->pw_name, pw->pw_gid, groups.data(), &count) < 0)
            return false;
    }
    for (int i = 0; i < count; ++i)
        if (groups.at(i) == wheel->gr_gid)
            return true;
    return false;
}

bool UpdateDBusObject::authorised()
{
    if (!m_checkCallers || !calledFromDBus())
        return true;
    const QDBusReply<uint> uid = connection().interface()->serviceUid(message().service());
    if (uid.isValid() && isAdministrator(uid.value()))
        return true;
    reply(UpdateError::make(UpdateError::PermissionDenied));
    return false;
}

void UpdateDBusObject::reply(const UpdateError &error)
{
    if (!error.isError() || !calledFromDBus())
        return;
    // Error name in CamelCase from the stable code: "no-space" -> "NoSpace".
    QString name;
    for (const QString &part : UpdateError::codeName(error.code).split(QLatin1Char('-')))
        name += part.left(1).toUpper() + part.mid(1);
    sendErrorReply(QStringLiteral(CUSTOMOS_UPDATE_ERROR_PREFIX) + name, error.message);
}

QVariantMap UpdateDBusObject::GetStatus() { return m_service->status(); }
QString UpdateDBusObject::GetHistory() { return m_service->historyJson(); }

void UpdateDBusObject::CheckForUpdates()
{
    if (authorised())
        reply(m_service->checkForUpdates(true));
}

void UpdateDBusObject::Download()
{
    if (authorised())
        reply(m_service->download());
}

void UpdateDBusObject::Install()
{
    if (authorised())
        reply(m_service->install());
}

void UpdateDBusObject::Cancel()
{
    if (authorised())
        reply(m_service->cancel());
}

void UpdateDBusObject::Reboot()
{
    if (authorised())
        reply(m_service->reboot());
}

void UpdateDBusObject::Rollback()
{
    if (authorised())
        reply(m_service->rollback());
}

void UpdateDBusObject::DismissError()
{
    if (authorised())
        m_service->dismissError();
}
