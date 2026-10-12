#include "errors.h"

#include <QCoreApplication>

static QString tr(const char *s)
{
    return QCoreApplication::translate("UpdateError", s);
}

UpdateError UpdateError::make(Code code, const QString &detail, const QString &arg)
{
    UpdateError e;
    e.code = code;
    e.detail = detail;
    switch (code) {
    case None:
        break;
    case Offline:
        e.message = tr("CustomOS couldn't reach GitHub to check for updates. Check that you're connected to the internet, then try again.");
        break;
    case Tls:
        e.message = tr("A secure connection to the update server couldn't be established. Make sure the date and time are correct, then try again.");
        break;
    case TlsUnavailable:
        e.message = tr("This build of CustomOS has no TLS support, so it can't download updates securely.");
        break;
    case RateLimited:
        e.message = arg.isEmpty()
            ? tr("GitHub is limiting how often updates can be checked. Try again in a few minutes.")
            : tr("GitHub is limiting how often updates can be checked. Try again after %1.").arg(arg);
        break;
    case RepositoryNotFound:
        e.message = tr("The update source “%1” wasn't found on GitHub. If the repository is private, an access token is needed (see TokenFile in /etc/customos/update.conf).").arg(arg);
        break;
    case Unauthorized:
        e.message = tr("GitHub rejected the access token configured for updates. Replace the token in the file named by TokenFile in /etc/customos/update.conf.");
        break;
    case NotConfigured:
        e.message = tr("No update source is configured. Set Repository in /etc/customos/update.conf.");
        break;
    case ServerError:
        e.message = tr("The update server returned an error. Try again later.");
        break;
    case InvalidRelease:
        e.message = tr("The release %1 on GitHub is incomplete (its update files are missing), so it can't be installed.").arg(arg);
        break;
    case InvalidManifest:
        e.message = tr("The description of release %1 is damaged or invalid, so it can't be installed.").arg(arg);
        break;
    case Incompatible:
        e.message = tr("CustomOS %1 is built for a different kind of system and can't be installed on this computer.").arg(arg);
        break;
    case TooOld:
        e.message = tr("CustomOS %1 can't be installed directly from the version you have. Install the intermediate update first, or reinstall from the latest ISO.").arg(arg);
        break;
    case NoSpace:
        e.message = tr("There isn't enough free space to download the update (%1 needed). Free up space on the data partition and try again.").arg(arg);
        break;
    case DownloadFailed:
        e.message = tr("The download was interrupted. Select Retry to continue where it stopped.");
        break;
    case ChecksumMismatch:
        e.message = tr("The downloaded update is damaged (its checksum doesn't match). It was deleted; select Retry to download it again.");
        break;
    case SignatureInvalid:
        e.message = tr("The update isn't signed by a trusted CustomOS key, so it was not installed. Your system was not changed.");
        break;
    case VersionMismatch:
        e.message = tr("The update package doesn't match the release it was published in, so it was not installed.");
        break;
    case Downgrade:
        e.message = tr("This update would install an older version (%1). Downgrades are blocked to protect your system.").arg(arg);
        break;
    case InstallFailed:
        e.message = tr("Installing the update failed. Your current system was not changed and keeps working.");
        break;
    case RaucUnavailable:
        e.message = tr("The system update service (RAUC) isn't running, so updates can't be installed right now.");
        break;
    case UnsupportedLive:
        e.message = tr("You're running CustomOS from the live image. Install CustomOS to a disk to receive updates.");
        break;
    case UnsupportedLegacy:
        e.message = tr("This computer uses the single-partition layout of CustomOS 0.1, which can't be updated safely. Back up your files and reinstall from the latest CustomOS ISO to get automatic updates.");
        break;
    case Busy:
        e.message = tr("An update operation is already in progress.");
        break;
    case PermissionDenied:
        e.message = tr("Only administrators can download and install updates.");
        break;
    case RolledBack:
        e.message = tr("CustomOS %1 didn't start correctly, so your computer went back to the previous version. Nothing was lost.").arg(arg);
        break;
    case NothingToInstall:
        e.message = tr("There is no update to install. Check for updates first.");
        break;
    case RebootFailed:
        e.message = tr("The computer couldn't be restarted automatically. Restart it from the Start menu.");
        break;
    case RollbackUnavailable:
        e.message = tr("There's no previous version to go back to.");
        break;
    case Cancelled:
        e.message = tr("The update was cancelled.");
        break;
    case Internal:
        e.message = tr("Something went wrong in the update service.");
        break;
    }
    return e;
}

QString UpdateError::codeName(Code code)
{
    switch (code) {
    case None: return QStringLiteral("none");
    case Offline: return QStringLiteral("offline");
    case Tls: return QStringLiteral("tls");
    case TlsUnavailable: return QStringLiteral("tls-unavailable");
    case RateLimited: return QStringLiteral("rate-limited");
    case RepositoryNotFound: return QStringLiteral("repository-not-found");
    case Unauthorized: return QStringLiteral("unauthorized");
    case NotConfigured: return QStringLiteral("not-configured");
    case ServerError: return QStringLiteral("server-error");
    case InvalidRelease: return QStringLiteral("invalid-release");
    case InvalidManifest: return QStringLiteral("invalid-manifest");
    case Incompatible: return QStringLiteral("incompatible");
    case TooOld: return QStringLiteral("too-old");
    case NoSpace: return QStringLiteral("no-space");
    case DownloadFailed: return QStringLiteral("download-failed");
    case ChecksumMismatch: return QStringLiteral("checksum-mismatch");
    case SignatureInvalid: return QStringLiteral("signature-invalid");
    case VersionMismatch: return QStringLiteral("version-mismatch");
    case Downgrade: return QStringLiteral("downgrade");
    case InstallFailed: return QStringLiteral("install-failed");
    case RaucUnavailable: return QStringLiteral("rauc-unavailable");
    case UnsupportedLive: return QStringLiteral("unsupported-live");
    case UnsupportedLegacy: return QStringLiteral("unsupported-legacy");
    case Busy: return QStringLiteral("busy");
    case PermissionDenied: return QStringLiteral("permission-denied");
    case RolledBack: return QStringLiteral("rolled-back");
    case NothingToInstall: return QStringLiteral("nothing-to-install");
    case RebootFailed: return QStringLiteral("reboot-failed");
    case RollbackUnavailable: return QStringLiteral("rollback-unavailable");
    case Cancelled: return QStringLiteral("cancelled");
    case Internal: return QStringLiteral("internal");
    }
    return QStringLiteral("internal");
}
