// End-to-end test of UpdateService against a mock GitHub (MiniHttpServer) and
// a fake RAUC on the session bus. Run under dbus-run-session.

#include "testutil.h"
#include "updateservice.h"

#include <QCryptographicHash>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

class TestService : public QObject
{
    Q_OBJECT

    QTemporaryDir m_dir;
    MiniHttpServer m_api;
    MiniHttpServer m_storage;
    FakeRauc *m_rauc = nullptr;

    QString root() const { return m_dir.path() + "/sysroot"; }
    QString stateDir() const { return m_dir.path() + "/data/customos/update"; }

    void writeFile(const QString &path, const QByteArray &data)
    {
        QDir().mkpath(QFileInfo(path).absolutePath());
        QFile f(path);
        QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Truncate));
        f.write(data);
    }

    // The machine as it is during one boot.
    void boot(const QString &version, const QString &slot, const QString &bootId, bool live = false)
    {
        writeFile(root() + "/etc/os-release",
                  "NAME=\"CustomOS\"\nPRETTY_NAME=\"CustomOS " + version.toUtf8() + " (Meridian)\"\nVERSION_ID="
                      + version.toUtf8() + "\n");
        writeFile(root() + "/proc/cmdline", "root=PARTUUID=1 ro rauc.slot=" + slot.toUtf8() + " quiet\n");
        writeFile(root() + "/proc/sys/kernel/random/boot_id", bootId.toUtf8() + "\n");
        writeFile(root() + "/proc/uptime", "30.00 10.00\n");
        if (live)
            writeFile(root() + "/etc/customos-live", "");
        else
            QFile::remove(root() + "/etc/customos-live");
        m_rauc->booted = slot;
    }

    // Publishes a release on the mock GitHub. bundle is the (fake) bundle file.
    void publish(const QString &version, const QByteArray &bundle, const QString &compatible = "customos-x86_64-ab",
                 const QByteArray &shaOverride = QByteArray())
    {
        const QString tag = "v" + version;
        const QString bundleName = "CustomOS-" + tag + ".raucb";
        const QByteArray sha = shaOverride.isEmpty() ? QCryptographicHash::hash(bundle, QCryptographicHash::Sha256).toHex()
                                                     : shaOverride;
        const QJsonObject manifest{
            {"schema", 1}, {"product", "CustomOS"}, {"version", version}, {"channel", "stable"},
            {"compatible", compatible}, {"released", "2026-10-12T09:00:00Z"}, {"reboot_required", true},
            {"bundle", QJsonObject{{"name", bundleName}, {"size", bundle.size()}, {"sha256", QString::fromLatin1(sha)}}},
            {"changelog", "## CustomOS " + version + "\n- Faster boot\n- Fixes"}};
        const QByteArray manifestBytes = QJsonDocument(manifest).toJson();
        const QByteArray sums = sha + "  " + bundleName.toUtf8() + "\n";
        auto asset = [this, &tag](const QString &name, qint64 size) {
            return QJsonObject{{"name", name}, {"size", size},
                               {"browser_download_url", m_api.base() + "/owner/repo/releases/download/" + tag + "/" + name}};
        };
        const QJsonObject release{
            {"tag_name", tag}, {"name", "CustomOS " + version}, {"body", "GitHub release body"},
            {"html_url", "https://github.com/owner/repo/releases/tag/" + tag},
            {"published_at", "2026-10-12T09:00:00Z"}, {"draft", false}, {"prerelease", false},
            {"assets", QJsonArray{asset(bundleName, bundle.size()), asset("CustomOS-" + tag + "-manifest.json", manifestBytes.size()),
                                  asset("SHA256SUMS", sums.size())}}};
        m_api.routes.insert("/repos/owner/repo/releases/latest", HttpRoute{200, QJsonDocument(release).toJson(), {}, {}, true, -1});
        // Assets redirect to a different host, as on GitHub.
        for (const auto &f : {std::pair<QString, QByteArray>{bundleName, bundle},
                              {"CustomOS-" + tag + "-manifest.json", manifestBytes}, {"SHA256SUMS", sums}}) {
            HttpRoute redirect;
            redirect.redirectTo = m_storage.base("localhost") + "/blob/" + tag + "/" + f.first;
            m_api.routes.insert("/owner/repo/releases/download/" + tag + "/" + f.first, redirect);
            m_storage.routes.insert("/blob/" + tag + "/" + f.first, HttpRoute{200, f.second, {}, {}, true, -1});
        }
    }

    static QByteArray fakeBundle(const QString &version, bool signedOk = true, bool installOk = true)
    {
        return "compatible=customos-x86_64-ab\nversion=" + version.toUtf8() + "\nsigned=" + (signedOk ? "yes" : "no")
               + "\ninstall=" + (installOk ? "ok" : "fail") + "\npadding=" + QByteArray(4096, 'p') + "\n";
    }

    UpdateConfig config() const
    {
        UpdateConfig c;
        c.repository = "owner/repo";
        c.apiUrl = QUrl(m_api.base());
        c.allowInsecureHttp = true;
        c.automaticCheck = false;
        c.stateDirectory = stateDir();
        c.cacheDirectory = stateDir() + "/cache";
        c.compatible = "customos-x86_64-ab";
        return c;
    }

    QJsonArray history(UpdateService &s)
    {
        return QJsonDocument::fromJson(s.historyJson().toUtf8()).array();
    }

private slots:
    void initTestCase()
    {
        if (!QDBusConnection::sessionBus().isConnected())
            QSKIP("no D-Bus session bus (run under dbus-run-session)");
        QVERIFY(m_dir.isValid());
        qputenv("CUSTOMOS_UPDATE_SYSROOT", root().toUtf8());
        m_rauc = new FakeRauc(QDBusConnection::sessionBus());
    }
    void cleanupTestCase() { delete m_rauc; }

    void init()
    {
        QDir(m_dir.path() + "/data").removeRecursively();
        m_api.routes.clear();
        m_storage.routes.clear();
        m_api.log.clear();
        m_storage.log.clear();
        m_rauc->installed.clear();
        m_rauc->otherGood = false;
        m_rauc->otherVersion.clear();
    }

    void upToDate()
    {
        boot("0.2.0", "A", "boot-1");
        publish("0.2.0", fakeBundle("0.2.0"));
        UpdateService s(config(), QDBusConnection::sessionBus());
        s.start();
        QVERIFY(!s.checkForUpdates().isError());
        QTRY_COMPARE_WITH_TIMEOUT(s.state(), UpdateService::State::UpToDate, 10000);
        QCOMPARE(s.status().value("updateAvailable").toBool(), false);
        QVERIFY(s.status().value("lastCheck").toLongLong() > 0);
    }

    void fullUpdateAndConfirmation()
    {
        boot("0.2.0", "A", "boot-1");
        const QByteArray bundle = fakeBundle("0.3.0");
        publish("0.3.0", bundle);
        {
            UpdateService s(config(), QDBusConnection::sessionBus());
            QSignalSpy available(&s, &UpdateService::updateAvailable);
            s.start();
            s.checkForUpdates();
            QTRY_COMPARE_WITH_TIMEOUT(s.state(), UpdateService::State::UpdateAvailable, 10000);
            QCOMPARE(available.count(), 1);
            const QVariantMap st = s.status();
            QCOMPARE(st.value("availableVersion").toString(), QStringLiteral("0.3.0"));
            QCOMPARE(st.value("downloadSize").toULongLong(), qulonglong(bundle.size()));
            QVERIFY(st.value("releaseNotes").toString().contains("Faster boot"));   // manifest changelog wins
            QVERIFY(st.value("canInstall").toBool());

            QVERIFY(!s.install().isError());
            QTRY_COMPARE_WITH_TIMEOUT(s.state(), UpdateService::State::RebootRequired, 15000);
            QCOMPARE(m_rauc->installed.size(), 1);
            QVERIFY(!QFileInfo::exists(m_rauc->installed.at(0)));   // cache cleaned after install
            QCOMPARE(s.status().value("pendingVersion").toString(), QStringLiteral("0.3.0"));
            // The asset storage host never saw a token header (none configured) and was reached via redirect.
            QVERIFY(!m_storage.log.isEmpty());
        }
        {
            // The service restarts during the same boot: still waiting for the restart.
            UpdateService s(config(), QDBusConnection::sessionBus());
            s.start();
            QCOMPARE(s.state(), UpdateService::State::RebootRequired);
        }
        // Reboot into slot B with the new version; the health check confirms it.
        boot("0.3.0", "B", "boot-2");
        writeFile(stateDir() + "/last-boot.env",
                  "SLOT=B\nVERSION=0.3.0\nRESULT=good\nREASON=\"trial boot passed\"\nTIME="
                      + QByteArray::number(QDateTime::currentSecsSinceEpoch()) + "\n");
        UpdateService s(config(), QDBusConnection::sessionBus());
        s.start();
        QTRY_COMPARE_WITH_TIMEOUT(s.state(), UpdateService::State::UpToDate, 10000);
        QCOMPARE(s.status().value("lastSuccessVersion").toString(), QStringLiteral("0.3.0"));
        const QJsonArray h = history(s);
        QCOMPARE(h.last().toObject().value("result").toString(), QStringLiteral("success"));
        QCOMPARE(h.last().toObject().value("to_version").toString(), QStringLiteral("0.3.0"));
    }

    void failedBootIsRolledBack()
    {
        boot("0.3.0", "B", "boot-3");
        publish("0.4.0", fakeBundle("0.4.0"));
        {
            UpdateService s(config(), QDBusConnection::sessionBus());
            s.start();
            s.checkForUpdates();
            QTRY_COMPARE_WITH_TIMEOUT(s.state(), UpdateService::State::UpdateAvailable, 10000);
            s.install();
            QTRY_COMPARE_WITH_TIMEOUT(s.state(), UpdateService::State::RebootRequired, 15000);
        }
        // Slot A (0.4.0) failed its trial and the machine came back on slot B.
        writeFile(stateDir() + "/last-boot.env",
                  "SLOT=A\nVERSION=0.4.0\nRESULT=bad\nREASON=\"did not start: meridian-login.service\"\nTIME="
                      + QByteArray::number(QDateTime::currentSecsSinceEpoch()) + "\n");
        boot("0.3.0", "B", "boot-4");
        UpdateService s(config(), QDBusConnection::sessionBus());
        s.start();
        QCOMPARE(s.state(), UpdateService::State::Error);
        const QVariantMap st = s.status();
        QCOMPARE(st.value("errorCode").toString(), QStringLiteral("rolled-back"));
        QVERIFY(st.value("errorMessage").toString().contains("0.4.0"));
        QVERIFY(st.value("errorDetail").toString().contains("meridian-login"));
        QCOMPARE(history(s).last().toObject().value("result").toString(), QStringLiteral("rolled-back"));
        s.dismissError();
        QVERIFY(s.state() != UpdateService::State::Error);
    }

    void untrustedSignatureIsRejected()
    {
        boot("0.2.0", "A", "boot-5");
        publish("0.3.0", fakeBundle("0.3.0", /*signed*/ false));
        UpdateService s(config(), QDBusConnection::sessionBus());
        s.start();
        s.checkForUpdates();
        QTRY_COMPARE_WITH_TIMEOUT(s.state(), UpdateService::State::UpdateAvailable, 10000);
        s.install();
        QTRY_COMPARE_WITH_TIMEOUT(s.state(), UpdateService::State::Error, 15000);
        QCOMPARE(s.status().value("errorCode").toString(), QStringLiteral("signature-invalid"));
        QVERIFY(m_rauc->installed.isEmpty());
        QVERIFY(!QFileInfo::exists(stateDir() + "/cache/CustomOS-v0.3.0.raucb"));
    }

    void corruptedDownloadIsRejected()
    {
        boot("0.2.0", "A", "boot-6");
        const QByteArray wrong = QCryptographicHash::hash("something else", QCryptographicHash::Sha256).toHex();
        publish("0.3.0", fakeBundle("0.3.0"), "customos-x86_64-ab", wrong);
        UpdateService s(config(), QDBusConnection::sessionBus());
        s.start();
        s.checkForUpdates();
        QTRY_COMPARE_WITH_TIMEOUT(s.state(), UpdateService::State::UpdateAvailable, 10000);
        s.download();
        QTRY_COMPARE_WITH_TIMEOUT(s.state(), UpdateService::State::Error, 15000);
        QCOMPARE(s.status().value("errorCode").toString(), QStringLiteral("checksum-mismatch"));
        QVERIFY(m_rauc->installed.isEmpty());
    }

    void signedVersionMustMatchRelease()
    {
        boot("0.2.0", "A", "boot-7");
        publish("0.3.0", fakeBundle("0.2.5"));   // validly signed, but an older build re-labelled
        UpdateService s(config(), QDBusConnection::sessionBus());
        s.start();
        s.checkForUpdates();
        QTRY_COMPARE_WITH_TIMEOUT(s.state(), UpdateService::State::UpdateAvailable, 10000);
        s.install();
        QTRY_COMPARE_WITH_TIMEOUT(s.state(), UpdateService::State::Error, 15000);
        QCOMPARE(s.status().value("errorCode").toString(), QStringLiteral("version-mismatch"));
        QVERIFY(m_rauc->installed.isEmpty());
    }

    void incompatibleReleaseIsReported()
    {
        boot("0.2.0", "A", "boot-8");
        publish("0.3.0", fakeBundle("0.3.0"), "customos-aarch64-ab");
        UpdateService s(config(), QDBusConnection::sessionBus());
        s.start();
        s.checkForUpdates();
        QTRY_COMPARE_WITH_TIMEOUT(s.state(), UpdateService::State::Error, 10000);
        QCOMPARE(s.status().value("errorCode").toString(), QStringLiteral("incompatible"));
    }

    void failedInstallKeepsSystem()
    {
        boot("0.2.0", "A", "boot-9");
        publish("0.3.0", fakeBundle("0.3.0", true, /*install*/ false));
        UpdateService s(config(), QDBusConnection::sessionBus());
        s.start();
        s.checkForUpdates();
        QTRY_COMPARE_WITH_TIMEOUT(s.state(), UpdateService::State::UpdateAvailable, 10000);
        s.install();
        QTRY_COMPARE_WITH_TIMEOUT(s.state(), UpdateService::State::Error, 15000);
        QCOMPARE(s.status().value("errorCode").toString(), QStringLiteral("install-failed"));
        QVERIFY(s.status().value("errorDetail").toString().contains("No space"));
        QCOMPARE(history(s).last().toObject().value("result").toString(), QStringLiteral("failed"));
    }

    void liveSystemCannotInstall()
    {
        boot("0.2.0", "A", "boot-10", /*live*/ true);
        publish("0.3.0", fakeBundle("0.3.0"));
        UpdateService s(config(), QDBusConnection::sessionBus());
        s.start();
        s.checkForUpdates();
        QTRY_COMPARE_WITH_TIMEOUT(s.state(), UpdateService::State::UpdateAvailable, 10000);
        QCOMPARE(s.status().value("supported").toBool(), false);
        QCOMPARE(s.status().value("canInstall").toBool(), false);
        QCOMPARE(s.install().code, UpdateError::UnsupportedLive);
    }

    void manualRollback()
    {
        boot("0.3.0", "B", "boot-11");
        m_rauc->otherGood = true;
        m_rauc->otherVersion = "0.2.0";
        UpdateService s(config(), QDBusConnection::sessionBus());
        s.start();
        QTRY_VERIFY_WITH_TIMEOUT(s.status().value("rollbackAvailable").toBool(), 10000);
        QCOMPARE(s.status().value("otherSlotVersion").toString(), QStringLiteral("0.2.0"));
        QVERIFY(!s.rollback().isError());
        QTRY_COMPARE_WITH_TIMEOUT(s.state(), UpdateService::State::RebootRequired, 10000);
        QVERIFY(m_rauc->marks.contains("active other"));
        QCOMPARE(s.status().value("pendingKind").toString(), QStringLiteral("rollback"));
    }
};

QTEST_GUILESS_MAIN(TestService)
#include "tst_service.moc"
