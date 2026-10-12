#include "downloader.h"
#include "githubclient.h"
#include "testutil.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

static QJsonObject releaseJson(const QString &tag, const QString &assetBase, bool prerelease = false)
{
    return QJsonObject{
        {"tag_name", tag},
        {"name", "CustomOS " + tag},
        {"body", "Release notes for " + tag},
        {"html_url", "https://github.com/owner/repo/releases/tag/" + tag},
        {"published_at", "2026-10-12T09:00:00Z"},
        {"draft", false},
        {"prerelease", prerelease},
        {"assets", QJsonArray{
            QJsonObject{{"name", "CustomOS-" + tag + ".raucb"}, {"size", 11},
                        {"browser_download_url", assetBase + "/download/CustomOS-" + tag + ".raucb"},
                        {"url", assetBase + "/api-asset/1"},
                        {"digest", "sha256:" + QString(64, QLatin1Char('a'))}},
            QJsonObject{{"name", "SHA256SUMS"}, {"size", 5},
                        {"browser_download_url", assetBase + "/download/SHA256SUMS"},
                        {"url", assetBase + "/api-asset/2"}}}}};
}

class TestGitHub : public QObject
{
    Q_OBJECT

    UpdateConfig config(const MiniHttpServer &api)
    {
        UpdateConfig c;
        c.repository = "owner/repo";
        c.apiUrl = QUrl(api.base());
        c.allowInsecureHttp = true;
        return c;
    }

    struct Result { ReleaseInfo release; UpdateError error; bool done = false; };

    Result latest(GitHubClient &gh)
    {
        Result r;
        gh.findLatestRelease([&r](const ReleaseInfo &rel, const UpdateError &e) {
            r.release = rel;
            r.error = e;
            r.done = true;
        });
        [&] { QTRY_VERIFY_WITH_TIMEOUT(r.done, 10000); }();
        return r;
    }

private slots:
    void parseRelease()
    {
        ReleaseInfo r;
        QVERIFY(GitHubClient::parseRelease(releaseJson("v0.3.0", "https://x"), &r));
        QCOMPARE(r.version.toString(), QStringLiteral("0.3.0"));
        QCOMPARE(r.assets.size(), 2);
        QVERIFY(r.asset("SHA256SUMS"));
        QCOMPARE(r.assetWithSuffix(".raucb")->sha256, QByteArray(64, 'a'));
        QCOMPARE(r.published.date(), QDate(2026, 10, 12));
    }

    void latestStable()
    {
        MiniHttpServer api;
        HttpRoute route;
        route.body = QJsonDocument(releaseJson("v0.3.0", api.base())).toJson();
        api.routes.insert("/repos/owner/repo/releases/latest", route);
        QNetworkAccessManager nam;
        GitHubClient gh(&nam, config(api));
        const Result r = latest(gh);
        QVERIFY2(!r.error.isError(), qPrintable(r.error.detail));
        QCOMPARE(r.release.version.toString(), QStringLiteral("0.3.0"));
        QCOMPARE(api.log.at(0).headers.value("accept"), QByteArray("application/vnd.github+json"));
        QVERIFY(api.log.at(0).headers.value("user-agent").startsWith("CustomOS-Updater/"));
    }

    void betaChannelPicksHighest()
    {
        MiniHttpServer api;
        HttpRoute route;
        route.body = QJsonDocument(QJsonArray{releaseJson("v0.3.0", api.base()),
                                              releaseJson("v0.4.0-beta.1", api.base(), true),
                                              releaseJson("v0.2.0", api.base())}).toJson();
        api.routes.insert("/repos/owner/repo/releases", route);
        QNetworkAccessManager nam;
        UpdateConfig c = config(api);
        c.channel = "beta";
        GitHubClient gh(&nam, c);
        const Result r = latest(gh);
        QVERIFY(!r.error.isError());
        QCOMPARE(r.release.version.toString(), QStringLiteral("0.4.0-beta.1"));
    }

    void noReleaseYet()
    {
        MiniHttpServer api;
        api.routes.insert("/repos/owner/repo", HttpRoute{200, "{\"full_name\":\"owner/repo\"}", {}, {}, true, -1});
        QNetworkAccessManager nam;
        GitHubClient gh(&nam, config(api));
        const Result r = latest(gh);
        QVERIFY(!r.error.isError());
        QVERIFY(!r.release.version.isValid());
    }

    void errors_data()
    {
        QTest::addColumn<int>("status");
        QTest::addColumn<QByteArray>("remaining");
        QTest::addColumn<int>("code");
        QTest::newRow("not found") << 404 << QByteArray() << int(UpdateError::RepositoryNotFound);
        QTest::newRow("bad token") << 401 << QByteArray() << int(UpdateError::Unauthorized);
        QTest::newRow("rate limit") << 403 << QByteArray("0") << int(UpdateError::RateLimited);
        QTest::newRow("forbidden") << 403 << QByteArray("12") << int(UpdateError::Unauthorized);
        QTest::newRow("server") << 502 << QByteArray() << int(UpdateError::ServerError);
    }
    void errors()
    {
        QFETCH(int, status);
        QFETCH(QByteArray, remaining);
        QFETCH(int, code);
        MiniHttpServer api;
        HttpRoute route;
        route.status = status;
        route.body = "{}";
        if (!remaining.isEmpty())
            route.headers = {{"x-ratelimit-remaining", remaining}, {"x-ratelimit-reset", "1791000000"}};
        api.routes.insert("/repos/owner/repo/releases/latest", route);
        api.routes.insert("/repos/owner/repo", route);
        QNetworkAccessManager nam;
        GitHubClient gh(&nam, config(api));
        const Result r = latest(gh);
        QCOMPARE(int(r.error.code), code);
        QVERIFY(!r.error.message.isEmpty());
    }

    void offline()
    {
        quint16 port;
        {
            MiniHttpServer tmp;
            port = tmp.serverPort();
        }
        QNetworkAccessManager nam;
        UpdateConfig c;
        c.repository = "owner/repo";
        c.apiUrl = QUrl(QStringLiteral("http://127.0.0.1:%1").arg(port));
        c.allowInsecureHttp = true;
        GitHubClient gh(&nam, c);
        const Result r = latest(gh);
        QCOMPARE(r.error.code, UpdateError::Offline);
    }

    void refusesPlainHttp()
    {
        MiniHttpServer api;
        QNetworkAccessManager nam;
        UpdateConfig c = config(api);
        c.allowInsecureHttp = false;
        GitHubClient gh(&nam, c);
        const Result r = latest(gh);
        QVERIFY(r.error.isError());
        QVERIFY(api.log.isEmpty());   // never contacted
    }

    void tokenNotLeakedToStorage()
    {
        MiniHttpServer api, storage;
        QTemporaryDir dir;
        QFile token(dir.filePath("token"));
        QVERIFY(token.open(QIODevice::WriteOnly));
        token.write("ghp_secret\n");
        token.close();

        HttpRoute redirect;
        redirect.redirectTo = storage.base("localhost") + "/blob/1";
        api.routes.insert("/api-asset/1", redirect);
        storage.routes.insert("/blob/1", HttpRoute{200, "hello world", {}, {}, true, -1});

        QNetworkAccessManager nam;
        UpdateConfig c = config(api);
        c.tokenFile = token.fileName();
        GitHubClient gh(&nam, c);
        ReleaseAsset a;
        a.name = "x.raucb";
        a.apiUrl = QUrl(api.base() + "/api-asset/1");
        a.browserUrl = QUrl(api.base() + "/download/x.raucb");
        QByteArray got;
        bool done = false;
        gh.fetchSmallAsset(a, 1024, [&](const QByteArray &d, const UpdateError &e) {
            QVERIFY(!e.isError());
            got = d;
            done = true;
        });
        QTRY_VERIFY_WITH_TIMEOUT(done, 10000);
        QCOMPARE(got, QByteArray("hello world"));
        QCOMPARE(api.log.at(0).headers.value("authorization"), QByteArray("Bearer ghp_secret"));
        QCOMPARE(api.log.at(0).headers.value("accept"), QByteArray("application/octet-stream"));
        QVERIFY(!storage.log.at(0).headers.contains("authorization"));
    }

    void downloadResumes()
    {
        MiniHttpServer storage;
        QByteArray payload;
        for (int i = 0; i < 200000; ++i)
            payload.append(char('a' + i % 26));
        HttpRoute r;
        r.body = payload;
        r.dropAfter = 70000;   // the first transfer breaks off
        storage.routes.insert("/blob", r);
        HttpRoute redirect;
        redirect.redirectTo = storage.base("localhost") + "/blob";
        storage.routes.insert("/download/b.raucb", redirect);

        QTemporaryDir dir;
        QNetworkAccessManager nam;
        UpdateConfig c = config(storage);
        GitHubClient gh(&nam, c);
        Downloader dl(&nam, &gh);
        ReleaseAsset a;
        a.name = "b.raucb";
        a.browserUrl = QUrl(storage.base() + "/download/b.raucb");
        QSignalSpy finished(&dl, &Downloader::finished);
        const QString target = dir.filePath("b.raucb");

        dl.start(a, target, payload.size());
        QTRY_COMPARE_WITH_TIMEOUT(finished.count(), 1, 10000);
        QVERIFY(finished.at(0).at(0).value<UpdateError>().isError());
        QVERIFY(QFileInfo(target + ".part").size() > 0);
        QVERIFY(!QFileInfo::exists(target));

        dl.start(a, target, payload.size());
        QTRY_COMPARE_WITH_TIMEOUT(finished.count(), 2, 10000);
        QVERIFY(!finished.at(1).at(0).value<UpdateError>().isError());
        QFile f(target);
        QVERIFY(f.open(QIODevice::ReadOnly));
        QCOMPARE(f.readAll(), payload);
        QVERIFY(!QFileInfo::exists(target + ".part"));
        // The second transfer asked only for the missing part.
        bool sawRange = false;
        for (const HttpRequestLog &l : storage.log)
            if (l.path == "/blob" && l.headers.value("range").startsWith("bytes="))
                sawRange = l.headers.value("range") != "bytes=0-";
        QVERIFY(sawRange);
    }

    void downloadRejectsOversize()
    {
        MiniHttpServer storage;
        storage.routes.insert("/download/c.raucb", HttpRoute{200, QByteArray(5000, 'x'), {}, {}, false, -1});
        QTemporaryDir dir;
        QNetworkAccessManager nam;
        GitHubClient gh(&nam, config(storage));
        Downloader dl(&nam, &gh);
        ReleaseAsset a;
        a.name = "c.raucb";
        a.browserUrl = QUrl(storage.base() + "/download/c.raucb");
        QSignalSpy finished(&dl, &Downloader::finished);
        dl.start(a, dir.filePath("c.raucb"), 1000);
        QTRY_COMPARE_WITH_TIMEOUT(finished.count(), 1, 10000);
        const UpdateError e = finished.at(0).at(0).value<UpdateError>();
        QCOMPARE(e.code, UpdateError::DownloadFailed);
        QVERIFY(!QFileInfo::exists(dir.filePath("c.raucb")));
    }
};

QTEST_GUILESS_MAIN(TestGitHub)
#include "tst_github.moc"
