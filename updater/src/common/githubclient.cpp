#include "githubclient.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPointer>

static const char *const UserAgent = "CustomOS-Updater/" CUSTOMOS_UPDATER_VERSION;

const ReleaseAsset *ReleaseInfo::asset(const QString &name) const
{
    for (const ReleaseAsset &a : assets)
        if (a.name == name)
            return &a;
    return nullptr;
}

const ReleaseAsset *ReleaseInfo::assetWithSuffix(const QString &suffix) const
{
    for (const ReleaseAsset &a : assets)
        if (a.name.endsWith(suffix))
            return &a;
    return nullptr;
}

GitHubClient::GitHubClient(QNetworkAccessManager *nam, const UpdateConfig &config, QObject *parent)
    : QObject(parent), m_nam(nam), m_config(config)
{
}

QNetworkRequest GitHubClient::baseRequest(const QUrl &url, const QString &accept)
{
    QNetworkRequest r(url);
    r.setRawHeader("User-Agent", UserAgent);
    r.setRawHeader("Accept", accept.toLatin1());
    r.setRawHeader("X-GitHub-Api-Version", "2022-11-28");
    // Redirects are followed by hand (see get()) so the token never leaks to
    // the storage host that release assets redirect to.
    r.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
    // Abort a transfer that makes no progress for 60 seconds.
    r.setTransferTimeout(60000);
    return r;
}

QUrl GitHubClient::apiUrl(const QString &path) const
{
    QUrl u = m_config.apiUrl;
    QString base = u.path();
    while (base.endsWith(QLatin1Char('/')))
        base.chop(1);
    u.setPath(base + path);
    return u;
}

void GitHubClient::addAuth(QNetworkRequest &request) const
{
    const QString token = m_config.token();
    if (!token.isEmpty())
        request.setRawHeader("Authorization", "Bearer " + token.toUtf8());
}

bool GitHubClient::urlAllowed(const QUrl &url) const
{
    if (url.scheme() == QLatin1String("https"))
        return true;
    return m_config.allowInsecureHttp && url.scheme() == QLatin1String("http");
}

QNetworkRequest GitHubClient::assetRequest(const ReleaseAsset &asset) const
{
    const bool privateRepo = !m_config.token().isEmpty();
    QNetworkRequest r = baseRequest(privateRepo && asset.apiUrl.isValid() ? asset.apiUrl : asset.browserUrl,
                                    QStringLiteral("application/octet-stream"));
    if (privateRepo && asset.apiUrl.isValid())
        addAuth(r);
    return r;
}

QNetworkRequest GitHubClient::redirectRequest(const QNetworkRequest &previous, const QUrl &location) const
{
    QNetworkRequest r = baseRequest(location, QString::fromLatin1(previous.rawHeader("Accept")));
    if (previous.hasRawHeader("Range"))
        r.setRawHeader("Range", previous.rawHeader("Range"));
    const QUrl api = m_config.apiUrl;
    if (previous.hasRawHeader("Authorization") && location.host() == api.host()
        && location.scheme() == api.scheme())
        r.setRawHeader("Authorization", previous.rawHeader("Authorization"));
    return r;
}

void GitHubClient::get(const QNetworkRequest &request, qint64 maxBytes, ReplyCallback callback, int redirects)
{
    if (!urlAllowed(request.url())) {
        callback(nullptr);
        return;
    }
    QNetworkReply *reply = m_nam->get(request);
    if (maxBytes > 0) {
        connect(reply, &QNetworkReply::downloadProgress, reply, [reply, maxBytes](qint64 received, qint64) {
            if (received > maxBytes)
                reply->abort();
        });
    }
    connect(reply, &QNetworkReply::finished, this, [this, reply, request, maxBytes, callback, redirects] {
        reply->deleteLater();
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if ((status == 301 || status == 302 || status == 303 || status == 307 || status == 308) && redirects < 8) {
            const QUrl location = request.url().resolved(QUrl(QString::fromUtf8(reply->rawHeader("Location"))));
            if (location.isValid()) {
                get(redirectRequest(request, location), maxBytes, callback, redirects + 1);
                return;
            }
        }
        callback(reply);
    });
}

bool GitHubClient::parseRelease(const QJsonObject &o, ReleaseInfo *out)
{
    ReleaseInfo r;
    r.tag = o.value(QLatin1String("tag_name")).toString();
    r.version = Version::parse(r.tag);
    r.name = o.value(QLatin1String("name")).toString();
    if (r.name.isEmpty())
        r.name = r.tag;
    r.body = o.value(QLatin1String("body")).toString();
    r.htmlUrl = QUrl(o.value(QLatin1String("html_url")).toString());
    r.published = QDateTime::fromString(o.value(QLatin1String("published_at")).toString(), Qt::ISODate);
    r.prerelease = o.value(QLatin1String("prerelease")).toBool();
    r.draft = o.value(QLatin1String("draft")).toBool();
    const QJsonArray assets = o.value(QLatin1String("assets")).toArray();
    for (const QJsonValue &v : assets) {
        const QJsonObject a = v.toObject();
        ReleaseAsset asset;
        asset.name = a.value(QLatin1String("name")).toString();
        asset.size = qint64(a.value(QLatin1String("size")).toDouble());
        asset.browserUrl = QUrl(a.value(QLatin1String("browser_download_url")).toString());
        asset.apiUrl = QUrl(a.value(QLatin1String("url")).toString());
        const QString digest = a.value(QLatin1String("digest")).toString();
        if (digest.startsWith(QLatin1String("sha256:")))
            asset.sha256 = digest.mid(7).toLower().toLatin1();
        if (!asset.name.isEmpty())
            r.assets.append(asset);
    }
    *out = r;
    return !r.tag.isEmpty();
}

QList<ReleaseInfo> GitHubClient::parseReleaseList(const QByteArray &json)
{
    QList<ReleaseInfo> out;
    const QJsonArray arr = QJsonDocument::fromJson(json).array();
    for (const QJsonValue &v : arr) {
        ReleaseInfo r;
        if (parseRelease(v.toObject(), &r))
            out.append(r);
    }
    return out;
}

UpdateError GitHubClient::classify(QNetworkReply *reply, const QString &repository, bool download)
{
    if (!reply)
        return UpdateError::make(UpdateError::ServerError,
                                 QStringLiteral("refused to use a non-HTTPS URL"));
    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (reply->error() == QNetworkReply::NoError && status >= 200 && status < 300)
        return UpdateError();
    const QString detail = status > 0
        ? QStringLiteral("HTTP %1 from %2").arg(status).arg(reply->url().host())
        : QStringLiteral("%1 (%2)").arg(reply->errorString(), reply->url().host());

    if (status == 401)
        return UpdateError::make(UpdateError::Unauthorized, detail);
    if (status == 403 || status == 429) {
        if (status == 429 || reply->rawHeader("x-ratelimit-remaining") == "0") {
            const qint64 reset = reply->rawHeader("x-ratelimit-reset").toLongLong();
            const QString when = reset > 0
                ? QDateTime::fromSecsSinceEpoch(reset).toLocalTime().toString(QStringLiteral("HH:mm"))
                : QString();
            return UpdateError::make(UpdateError::RateLimited, detail, when);
        }
        return UpdateError::make(UpdateError::Unauthorized, detail);
    }
    if (status == 404)
        return UpdateError::make(download ? UpdateError::InvalidRelease : UpdateError::RepositoryNotFound,
                                 detail, repository);
    if (status >= 300)
        return UpdateError::make(download ? UpdateError::DownloadFailed : UpdateError::ServerError, detail);

    switch (reply->error()) {
    case QNetworkReply::ProtocolUnknownError:
        return UpdateError::make(UpdateError::TlsUnavailable, detail);
    case QNetworkReply::SslHandshakeFailedError:
        return UpdateError::make(UpdateError::Tls, detail);
    case QNetworkReply::OperationCanceledError:
    case QNetworkReply::TimeoutError:
    case QNetworkReply::HostNotFoundError:
    case QNetworkReply::ConnectionRefusedError:
    case QNetworkReply::RemoteHostClosedError:
    case QNetworkReply::TemporaryNetworkFailureError:
    case QNetworkReply::NetworkSessionFailedError:
    case QNetworkReply::UnknownNetworkError:
    case QNetworkReply::ProxyConnectionRefusedError:
    case QNetworkReply::ProxyConnectionClosedError:
    case QNetworkReply::ProxyNotFoundError:
    case QNetworkReply::ProxyTimeoutError:
        return UpdateError::make(download ? UpdateError::DownloadFailed : UpdateError::Offline, detail);
    default:
        return UpdateError::make(download ? UpdateError::DownloadFailed : UpdateError::ServerError, detail);
    }
}

void GitHubClient::findLatestRelease(ReleaseCallback callback)
{
    const QString repo = m_config.repository;
    if (repo.isEmpty()) {
        callback(ReleaseInfo(), UpdateError::make(UpdateError::NotConfigured));
        return;
    }
    const QString accept = QStringLiteral("application/vnd.github+json");

    if (m_config.channel == QLatin1String("stable")) {
        // /releases/latest is the newest non-draft, non-prerelease release.
        QNetworkRequest req = baseRequest(apiUrl(QStringLiteral("/repos/%1/releases/latest").arg(repo)), accept);
        addAuth(req);
        get(req, 4 * 1024 * 1024, [this, repo, accept, callback](QNetworkReply *reply) {
            const int status = reply ? reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() : 0;
            if (status == 404) {
                // Either the repository has no release yet or it doesn't exist
                // (GitHub also says 404 for private repositories).
                QNetworkRequest repoReq = baseRequest(apiUrl(QStringLiteral("/repos/%1").arg(repo)), accept);
                addAuth(repoReq);
                get(repoReq, 1024 * 1024, [repo, callback](QNetworkReply *r2) {
                    const UpdateError e = classify(r2, repo, false);
                    callback(ReleaseInfo(), e);   // no error: repository exists, no release
                });
                return;
            }
            const UpdateError e = classify(reply, repo, false);
            if (e.isError()) {
                callback(ReleaseInfo(), e);
                return;
            }
            ReleaseInfo r;
            const QJsonObject o = QJsonDocument::fromJson(reply->readAll()).object();
            if (!parseRelease(o, &r) || r.draft || r.prerelease) {
                callback(ReleaseInfo(), UpdateError());
                return;
            }
            callback(r, UpdateError());
        });
        return;
    }

    // Other channels (reserved for "beta"): newest release including
    // pre-releases, by version precedence.
    QNetworkRequest req = baseRequest(apiUrl(QStringLiteral("/repos/%1/releases").arg(repo)), accept);
    QUrl u = req.url();
    u.setQuery(QStringLiteral("per_page=30"));
    req.setUrl(u);
    addAuth(req);
    get(req, 8 * 1024 * 1024, [repo, callback](QNetworkReply *reply) {
        const UpdateError e = classify(reply, repo, false);
        if (e.isError()) {
            callback(ReleaseInfo(), e);
            return;
        }
        ReleaseInfo best;
        for (const ReleaseInfo &r : parseReleaseList(reply->readAll())) {
            if (r.draft || !r.version.isValid())
                continue;
            if (!best.version.isValid() || r.version > best.version)
                best = r;
        }
        callback(best, UpdateError());
    });
}

void GitHubClient::fetchSmallAsset(const ReleaseAsset &asset, qint64 maxBytes, DataCallback callback)
{
    const QString repo = m_config.repository;
    get(assetRequest(asset), maxBytes, [repo, maxBytes, callback](QNetworkReply *reply) {
        const UpdateError e = classify(reply, repo, true);
        if (e.isError()) {
            callback(QByteArray(), e);
            return;
        }
        const QByteArray data = reply->readAll();
        if (data.size() > maxBytes) {
            callback(QByteArray(), UpdateError::make(UpdateError::InvalidRelease, QStringLiteral("asset too large")));
            return;
        }
        callback(data, UpdateError());
    });
}
