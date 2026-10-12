#pragma once

#include <QMetaType>
#include <QString>

// Every failure the updater reports has a stable code (for scripts and the
// UI), a sentence a person can act on, and an optional technical detail.
struct UpdateError
{
    enum Code {
        None,
        Offline,              // no route / DNS / connection refused
        Tls,                  // certificate or TLS handshake problem
        TlsUnavailable,       // Qt built without a TLS backend
        RateLimited,          // GitHub API rate limit
        RepositoryNotFound,   // 404 on the repository (or private without token)
        Unauthorized,         // token rejected
        NotConfigured,        // no repository configured
        ServerError,          // other HTTP failure
        InvalidRelease,       // release lacks the expected assets
        InvalidManifest,      // manifest JSON fails validation
        Incompatible,         // built for another system
        TooOld,               // installed version below the release's min_version
        NoSpace,              // not enough space for the download
        DownloadFailed,       // transfer error / interrupted
        ChecksumMismatch,     // SHA-256 does not match
        SignatureInvalid,     // RAUC rejected the bundle signature
        VersionMismatch,      // signed bundle version differs from the release
        Downgrade,            // bundle is not newer than the running system
        InstallFailed,        // RAUC installation failed
        RaucUnavailable,      // RAUC service not reachable
        UnsupportedLive,      // running from the live ISO
        UnsupportedLegacy,    // 0.1 single-partition installation
        Busy,                 // another operation running
        PermissionDenied,     // caller is not an administrator
        RolledBack,           // the new version failed to boot; old one restored
        NothingToInstall,     // no update available
        RebootFailed,
        RollbackUnavailable,
        Cancelled,
        Internal
    };

    Code code = None;
    QString message;   // user facing
    QString detail;    // technical (HTTP status, RAUC error text, ...)

    bool isError() const { return code != None; }
    static UpdateError make(Code code, const QString &detail = QString(), const QString &arg = QString());
    static QString codeName(Code code);
};

Q_DECLARE_METATYPE(UpdateError)
