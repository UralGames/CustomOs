#pragma once

#include <QString>
#include <QUrl>

// Run-time configuration: /etc/customos/update.conf, then every
// /etc/customos/update.conf.d/*.conf in name order (later files win).
// CUSTOMOS_UPDATE_CONFIG in the environment replaces the main file (tests).
struct UpdateConfig
{
    // GitHub repository that publishes the releases, as "owner/name".
    QString repository;
    QUrl apiUrl{QStringLiteral("https://api.github.com")};
    QString channel{QStringLiteral(CUSTOMOS_DEFAULT_CHANNEL)};
    // Optional file holding a GitHub token (needed for private repositories).
    QString tokenFile;
    QString compatible{QStringLiteral(CUSTOMOS_DEFAULT_COMPATIBLE)};

    bool automaticCheck = true;
    int checkIntervalHours = 6;
    QString stateDirectory{QStringLiteral("/data/customos/update")};
    QString cacheDirectory{QStringLiteral("/data/customos/update/cache")};
    bool allowDowngrade = false;
    // Testing only: accept http:// API and download URLs.
    bool allowInsecureHttp = false;

    QString loadedFrom;   // for diagnostics

    static UpdateConfig load();
    static UpdateConfig loadFrom(const QString &mainFile, const QString &dropInDir);

    // Accepts "owner/name", "https://github.com/owner/name(.git)" and
    // "github.com/owner/name"; returns "owner/name" or an empty string.
    static QString normaliseRepository(const QString &text);

    QString token() const;   // trimmed contents of tokenFile, or empty
};
