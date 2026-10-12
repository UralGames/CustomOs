#include "statestore.h"
#include "systemstate.h"

#include <QFile>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QTest>

class TestStateStore : public QObject
{
    Q_OBJECT
private slots:
    void roundTrip()
    {
        QTemporaryDir dir;
        {
            StateStore s(dir.path() + "/state");
            QVERIFY(s.load());
            PendingChange p;
            p.kind = "update";
            p.fromVersion = "0.2.0";
            p.toVersion = "0.3.0";
            p.fromSlot = "A";
            p.targetSlot = "B";
            p.bootId = "boot-1";
            p.when = QDateTime::currentDateTimeUtc();
            s.setPending(p);
            s.setLastSuccess("0.2.0", QDateTime(QDate(2026, 10, 1), QTime(12, 0), Qt::UTC));
            s.addHistory("install", "pending", "0.2.0", "0.3.0", "Installed into slot B");
            QVERIFY(s.save());
        }
        StateStore s(dir.path() + "/state");
        QVERIFY(s.load());
        QVERIFY(s.pending().isValid());
        QCOMPARE(s.pending().targetSlot, QStringLiteral("B"));
        QCOMPARE(s.pending().bootId, QStringLiteral("boot-1"));
        QCOMPARE(s.lastSuccessVersion(), QStringLiteral("0.2.0"));
        QCOMPARE(s.lastSuccessTime().date(), QDate(2026, 10, 1));
        QCOMPARE(s.history().size(), 1);
        QCOMPARE(s.history().at(0).toObject().value("to_version").toString(), QStringLiteral("0.3.0"));
        s.clearPending();
        QVERIFY(s.save());
        StateStore again(dir.path() + "/state");
        again.load();
        QVERIFY(!again.pending().isValid());
    }

    void historyIsBounded()
    {
        QTemporaryDir dir;
        StateStore s(dir.path());
        for (int i = 0; i < StateStore::MaxHistory + 25; ++i)
            s.addHistory("update", "success", "0.1.0", QString::number(i), "x");
        QCOMPARE(s.history().size(), StateStore::MaxHistory);
        QCOMPARE(s.history().last().toObject().value("to_version").toString(),
                 QString::number(StateStore::MaxHistory + 24));
    }

    void bootReport()
    {
        QTemporaryDir dir;
        QFile f(dir.filePath("last-boot.env"));
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write("SLOT=B\nVERSION=0.3.0\nRESULT=bad\nREASON=\"did not start: meridian-login.service\"\nTIME=1791000000\n");
        f.close();
        StateStore s(dir.path());
        const BootReport r = s.lastBootReport();
        QVERIFY(r.isValid());
        QCOMPARE(r.slot, QStringLiteral("B"));
        QCOMPARE(r.result, QStringLiteral("bad"));
        QCOMPARE(r.reason, QStringLiteral("did not start: meridian-login.service"));
        QCOMPARE(r.when.toSecsSinceEpoch(), qint64(1791000000));
    }

    void systemModes()
    {
        QTemporaryDir dir;
        auto write = [&](const QString &name, const QByteArray &data) {
            QFile f(dir.filePath(name));
            QVERIFY(f.open(QIODevice::WriteOnly));
            f.write(data);
        };
        write("os-release", "NAME=\"CustomOS\"\nPRETTY_NAME=\"CustomOS 0.2.0 (Meridian)\"\nVERSION_ID=0.2.0\nBUILD_ID=\"20261012\"\n");
        write("cmdline-ab", "BOOT_IMAGE=/boot/bzImage root=PARTUUID=x ro rauc.slot=B quiet\n");
        write("cmdline-legacy", "root=PARTUUID=y rw quiet\n");
        write("live", "");

        SystemState ab = SystemState::probeFrom(dir.filePath("os-release"), dir.filePath("cmdline-ab"), dir.filePath("nope"));
        QCOMPARE(ab.mode, SystemState::Mode::AB);
        QCOMPARE(ab.bootedSlot, QStringLiteral("B"));
        QCOMPARE(ab.version.toString(), QStringLiteral("0.2.0"));
        QCOMPARE(ab.prettyName, QStringLiteral("CustomOS 0.2.0 (Meridian)"));
        QCOMPARE(ab.buildId, QStringLiteral("20261012"));

        SystemState legacy = SystemState::probeFrom(dir.filePath("os-release"), dir.filePath("cmdline-legacy"), dir.filePath("nope"));
        QCOMPARE(legacy.mode, SystemState::Mode::Legacy);

        SystemState live = SystemState::probeFrom(dir.filePath("os-release"), dir.filePath("cmdline-ab"), dir.filePath("live"));
        QCOMPARE(live.mode, SystemState::Mode::Live);
    }
};

QTEST_APPLESS_MAIN(TestStateStore)
#include "tst_statestore.moc"
