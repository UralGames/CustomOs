#include "version.h"

#include <QTest>

class TestVersion : public QObject
{
    Q_OBJECT
private slots:
    void parse_data()
    {
        QTest::addColumn<QString>("text");
        QTest::addColumn<bool>("valid");
        QTest::addColumn<QString>("normalised");
        QTest::newRow("plain") << "0.2.0" << true << "0.2.0";
        QTest::newRow("tag") << "v1.10.3" << true << "1.10.3";
        QTest::newRow("prerelease") << "v0.3.0-rc.1" << true << "0.3.0-rc.1";
        QTest::newRow("build") << "0.3.0+20261012.abc" << true << "0.3.0+20261012.abc";
        QTest::newRow("both") << "1.0.0-beta.2+exp.sha.5114f85" << true << "1.0.0-beta.2+exp.sha.5114f85";
        QTest::newRow("spaces") << "  0.1.0 \n" << true << "0.1.0";
        QTest::newRow("two parts") << "1.0" << false << "";
        QTest::newRow("leading zero") << "01.2.3" << false << "";
        QTest::newRow("prerelease leading zero") << "1.2.3-01" << false << "";
        QTest::newRow("empty") << "" << false << "";
        QTest::newRow("garbage") << "latest" << false << "";
        QTest::newRow("overflow") << "99999999999.0.0" << false << "";
    }
    void parse()
    {
        QFETCH(QString, text);
        QFETCH(bool, valid);
        QFETCH(QString, normalised);
        const Version v = Version::parse(text);
        QCOMPARE(v.isValid(), valid);
        QCOMPARE(v.toString(), normalised);
    }

    void order()
    {
        // SemVer 2.0.0 section 11 example, ascending.
        const QStringList ordered{
            "1.0.0-alpha", "1.0.0-alpha.1", "1.0.0-alpha.beta", "1.0.0-beta", "1.0.0-beta.2",
            "1.0.0-beta.11", "1.0.0-rc.1", "1.0.0", "1.0.1", "1.1.0", "2.0.0", "10.0.0"};
        for (int i = 0; i + 1 < ordered.size(); ++i) {
            const Version a = Version::parse(ordered.at(i));
            const Version b = Version::parse(ordered.at(i + 1));
            QVERIFY2(a < b, qPrintable(ordered.at(i) + " < " + ordered.at(i + 1)));
            QVERIFY(b > a);
            QVERIFY(a != b);
        }
    }

    void equalityIgnoresBuild()
    {
        QVERIFY(Version::parse("0.2.0+a") == Version::parse("v0.2.0+b"));
        QVERIFY(Version::parse("0.2.0") <= Version::parse("0.2.0"));
    }

    void invalidSortsFirst()
    {
        QVERIFY(Version() < Version::parse("0.0.1"));
        QVERIFY(Version() == Version());
    }
};

QTEST_APPLESS_MAIN(TestVersion)
#include "tst_version.moc"
