#include "config.h"
#include "manifest.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QTest>

static const QByteArray Sha = "4f2d5b0c6a1e8f3b9d7c2e1a0b4f6d8c3e5a7b9c1d2e3f4a5b6c7d8e9f0a1b2c";

static QJsonObject validManifest()
{
    return QJsonObject{
        {"schema", 1},
        {"product", "CustomOS"},
        {"version", "0.2.0"},
        {"channel", "stable"},
        {"compatible", "customos-x86_64-ab"},
        {"released", "2026-10-12T09:00:00Z"},
        {"reboot_required", true},
        {"bundle", QJsonObject{{"name", "CustomOS-v0.2.0.raucb"}, {"size", 123456789}, {"sha256", QString::fromLatin1(Sha)}}},
        {"changelog", "## Changes\n- Updates"},
        {"build", QJsonObject{{"id", "20261012.1"}, {"git_commit", "abc123"}}}};
}

class TestManifest : public QObject
{
    Q_OBJECT
private slots:
    void valid()
    {
        ReleaseManifest m;
        QString err;
        QVERIFY2(ReleaseManifest::parse(QJsonDocument(validManifest()).toJson(), &m, &err), qPrintable(err));
        QCOMPARE(m.version.toString(), QStringLiteral("0.2.0"));
        QCOMPARE(m.compatible, QStringLiteral("customos-x86_64-ab"));
        QCOMPARE(m.bundleName, QStringLiteral("CustomOS-v0.2.0.raucb"));
        QCOMPARE(m.bundleSize, qint64(123456789));
        QCOMPARE(m.bundleSha256, Sha);
        QVERIFY(m.rebootRequired);
        QVERIFY(!m.minVersion.isValid());
        QCOMPARE(m.released.date().year(), 2026);
    }

    void invalid_data()
    {
        QTest::addColumn<QString>("key");
        QTest::addColumn<QJsonValue>("value");
        QTest::newRow("schema") << "schema" << QJsonValue(2);
        QTest::newRow("product") << "product" << QJsonValue("OtherOS");
        QTest::newRow("version") << "version" << QJsonValue("latest");
        QTest::newRow("compatible") << "compatible" << QJsonValue("");
        QTest::newRow("min_version") << "min_version" << QJsonValue("x");
        QTest::newRow("path traversal") << "bundle"
            << QJsonValue(QJsonObject{{"name", "../../etc/passwd.raucb"}, {"size", 10}, {"sha256", QString::fromLatin1(Sha)}});
        QTest::newRow("not a bundle") << "bundle"
            << QJsonValue(QJsonObject{{"name", "rootfs.img"}, {"size", 10}, {"sha256", QString::fromLatin1(Sha)}});
        QTest::newRow("size") << "bundle"
            << QJsonValue(QJsonObject{{"name", "CustomOS-v0.2.0.raucb"}, {"size", -1}, {"sha256", QString::fromLatin1(Sha)}});
        QTest::newRow("sha") << "bundle"
            << QJsonValue(QJsonObject{{"name", "CustomOS-v0.2.0.raucb"}, {"size", 10}, {"sha256", "abc"}});
    }
    void invalid()
    {
        QFETCH(QString, key);
        QFETCH(QJsonValue, value);
        QJsonObject o = validManifest();
        o.insert(key, value);
        ReleaseManifest m;
        QString err;
        QVERIFY(!ReleaseManifest::parse(QJsonDocument(o).toJson(), &m, &err));
        QVERIFY(!err.isEmpty());
    }

    void notJson()
    {
        ReleaseManifest m;
        QString err;
        QVERIFY(!ReleaseManifest::parse("<html>404</html>", &m, &err));
    }

    void sums()
    {
        const QByteArray text = Sha + "  CustomOS-v0.2.0.raucb\n"
                                + QByteArray(64, 'a') + " *CustomOS-x86_64.iso\n"
                                + "# comment\n"
                                + "nonsense line\n"
                                + QByteArray(63, 'b') + "  short.txt\n";
        const Sha256Sums s = Sha256Sums::parse(text);
        QCOMPARE(s.size(), 2);
        QCOMPARE(s.value("CustomOS-v0.2.0.raucb"), Sha);
        QCOMPARE(s.value("CustomOS-x86_64.iso"), QByteArray(64, 'a'));
        QVERIFY(!s.contains("short.txt"));
    }

    void repository_data()
    {
        QTest::addColumn<QString>("in");
        QTest::addColumn<QString>("out");
        QTest::newRow("owner/name") << "UralGames/CustomOs" << "UralGames/CustomOs";
        QTest::newRow("url") << "https://github.com/UralGames/CustomOs" << "UralGames/CustomOs";
        QTest::newRow("git url") << "https://github.com/UralGames/CustomOs.git" << "UralGames/CustomOs";
        QTest::newRow("trailing slash") << "github.com/UralGames/CustomOs/" << "UralGames/CustomOs";
        QTest::newRow("empty") << "" << "";
        QTest::newRow("bad") << "not a repo" << "";
        QTest::newRow("too deep") << "https://github.com/a/b/c" << "";
    }
    void repository()
    {
        QFETCH(QString, in);
        QFETCH(QString, out);
        QCOMPARE(UpdateConfig::normaliseRepository(in), out);
    }

    void configOverrides()
    {
        QTemporaryDir dir;
        QFile main(dir.filePath("update.conf"));
        QVERIFY(main.open(QIODevice::WriteOnly));
        main.write("[Source]\nRepository=https://github.com/UralGames/CustomOs\nChannel=Stable\n"
                   "[Updates]\nCheckIntervalHours=12\nAutomaticCheck=no\n");
        main.close();
        QDir().mkpath(dir.filePath("update.conf.d"));
        QFile drop(dir.filePath("update.conf.d/50-local.conf"));
        QVERIFY(drop.open(QIODevice::WriteOnly));
        drop.write("[Source]\nRepository=someone/fork\n[Updates]\nCheckIntervalHours=0\n");
        drop.close();
        const UpdateConfig c = UpdateConfig::loadFrom(main.fileName(), dir.filePath("update.conf.d"));
        QCOMPARE(c.repository, QStringLiteral("someone/fork"));
        QCOMPARE(c.channel, QStringLiteral("stable"));
        QCOMPARE(c.checkIntervalHours, 12);   // 0 is out of range and ignored
        QCOMPARE(c.automaticCheck, false);
        QVERIFY(!c.allowInsecureHttp);
    }
};

QTEST_APPLESS_MAIN(TestManifest)
#include "tst_manifest.moc"
