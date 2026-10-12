#pragma once

#include "config.h"
#include "errors.h"
#include "version.h"

#include <QDateTime>
#include <QList>
#include <QNetworkRequest>
#include <QObject>
#include <QUrl>

#include <functional>

class QNetworkAccessManager;
class QNetworkReply;

struct ReleaseAsset
{
    QString name;
    qint64 size = 0;
    QUrl browserUrl;     // https://github.com/<repo>/releases/download/<tag>/<name>
    QUrl apiUrl;         // https://api.github.com/repos/<repo>/releases/assets/<id>
    QByteArray sha256;   // GitHub's own asset digest when present ("sha256:<hex>")
};

struct ReleaseInfo
{
    QString tag;
    Version version;     // parsed from the tag ("v0.2.0" -> 0.2.0)
    QString name;
    QString body;        // release notes (Markdown)
    QUrl htmlUrl;
    QDateTime published;
    bool prerelease = false;
    bool draft = false;
    QList<ReleaseAsset> assets;

    const ReleaseAsset *asset(const QString &name) const;
    // The first asset whose name ends with `suffix`.
    const ReleaseAsset *assetWithSuffix(const QString &suffix) const;
};

// Talks to the GitHub REST API (releases) for the configured repository.
// GitHub is only the distribution server: everything fetched here is
// verified before use (see UpdateService).
class GitHubClient : public QObject
{
    Q_OBJECT
public:
    GitHubClient(QNetworkAccessManager *nam, const UpdateConfig &config, QObject *parent = nullptr);

    void setConfig(const UpdateConfig &config) { m_config = config; }

    // Finds the newest release for the configured channel. `release` is
    // invalid (version not valid) when the repository has no release yet.
    using ReleaseCallback = std::function<void(const ReleaseInfo &release, const UpdateError &error)>;
    void findLatestRelease(ReleaseCallback callback);

    // Downloads a small asset (manifest, SHA256SUMS) into memory.
    using DataCallback = std::function<void(const QByteArray &data, const UpdateError &error)>;
    void fetchSmallAsset(const ReleaseAsset &asset, qint64 maxBytes, DataCallback callback);

    // Request used to download an asset (also by Downloader). Uses the API
    // URL with the token for private repositories, otherwise the public URL.
    QNetworkRequest assetRequest(const ReleaseAsset &asset) const;
    // Builds the follow-up request for a redirect: the token is only sent
    // to the API host, never to the storage host the asset redirects to.
    QNetworkRequest redirectRequest(const QNetworkRequest &previous, const QUrl &location) const;
    bool urlAllowed(const QUrl &url) const;

    static bool parseRelease(const QJsonObject &o, ReleaseInfo *out);
    static QList<ReleaseInfo> parseReleaseList(const QByteArray &json);
    // Maps a finished reply (network error or HTTP status) to an UpdateError.
    static UpdateError classify(QNetworkReply *reply, const QString &repository, bool download);
    static QNetworkRequest baseRequest(const QUrl &url, const QString &accept);

private:
    using ReplyCallback = std::function<void(QNetworkReply *reply)>;
    void get(const QNetworkRequest &request, qint64 maxBytes, ReplyCallback callback, int redirects = 0);
    QUrl apiUrl(const QString &path) const;
    void addAuth(QNetworkRequest &request) const;

    QNetworkAccessManager *m_nam;
    UpdateConfig m_config;
};
