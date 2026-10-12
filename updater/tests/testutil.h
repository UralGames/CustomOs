#pragma once
// Test doubles: a tiny HTTP/1.1 server (stands in for api.github.com and the
// release-asset storage host) and a fake RAUC D-Bus service.

#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusContext>
#include <QDBusMessage>
#include <QDBusMetaType>
#include <QFile>
#include <QHash>
#include <QPointer>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <QVariantMap>

// ---- HTTP -------------------------------------------------------------------------

struct HttpRoute
{
    int status = 200;
    QByteArray body;
    QList<QPair<QByteArray, QByteArray>> headers;
    QString redirectTo;          // absolute URL: answer 302
    bool ranges = true;          // honour "Range: bytes=N-"
    qint64 dropAfter = -1;       // close the connection after this many body bytes (once)
};

struct HttpRequestLog
{
    QString path;
    QHash<QByteArray, QByteArray> headers;   // lower-case names
};

class MiniHttpServer : public QTcpServer
{
public:
    MiniHttpServer() { listen(QHostAddress::LocalHost); }

    QHash<QString, HttpRoute> routes;
    QList<HttpRequestLog> log;

    QString base(const QString &host = QStringLiteral("127.0.0.1")) const
    {
        return QStringLiteral("http://%1:%2").arg(host).arg(serverPort());
    }

protected:
    void incomingConnection(qintptr fd) override
    {
        auto *s = new QTcpSocket(this);
        s->setSocketDescriptor(fd);
        auto *buf = new QByteArray;
        connect(s, &QTcpSocket::disconnected, s, [s, buf] { delete buf; s->deleteLater(); });
        connect(s, &QTcpSocket::readyRead, s, [this, s, buf] {
            buf->append(s->readAll());
            const int end = buf->indexOf("\r\n\r\n");
            if (end < 0)
                return;
            const QList<QByteArray> lines = buf->left(end).split('\n');
            buf->remove(0, end + 4);
            const QList<QByteArray> first = lines.value(0).trimmed().split(' ');
            HttpRequestLog req;
            req.path = QString::fromLatin1(first.value(1));
            for (int i = 1; i < lines.size(); ++i) {
                const int c = lines.at(i).indexOf(':');
                if (c > 0)
                    req.headers.insert(lines.at(i).left(c).trimmed().toLower(), lines.at(i).mid(c + 1).trimmed());
            }
            log.append(req);
            respond(s, req);
        });
    }

private:
    void respond(QTcpSocket *s, const HttpRequestLog &req)
    {
        const QString path = req.path.section(QLatin1Char('?'), 0, 0);
        if (!routes.contains(path)) {
            write(s, 404, {}, "{\"message\":\"Not Found\"}");
            return;
        }
        HttpRoute &r = routes[path];
        if (!r.redirectTo.isEmpty()) {
            write(s, 302, {{"Location", r.redirectTo.toLatin1()}}, QByteArray());
            return;
        }
        QByteArray body = r.body;
        int status = r.status;
        QList<QPair<QByteArray, QByteArray>> headers = r.headers;
        const QByteArray range = req.headers.value("range");
        if (status == 200 && r.ranges && range.startsWith("bytes=")) {
            const qint64 from = range.mid(6).split('-').value(0).toLongLong();
            if (from >= body.size()) {
                write(s, 416, {}, QByteArray());
                return;
            }
            headers.append({"Content-Range", "bytes " + QByteArray::number(from) + '-'
                                                 + QByteArray::number(body.size() - 1) + '/'
                                                 + QByteArray::number(body.size())});
            body = body.mid(from);
            status = 206;
        }
        if (r.dropAfter >= 0 && r.dropAfter < body.size()) {
            const qint64 n = r.dropAfter;
            r.dropAfter = -1;
            QByteArray head = statusLine(status) + "Content-Length: " + QByteArray::number(body.size()) + "\r\n";
            for (const auto &h : headers)
                head += h.first + ": " + h.second + "\r\n";
            s->write(head + "\r\n" + body.left(n));
            s->flush();
            QTimer::singleShot(50, s, [s] { s->abort(); });
            return;
        }
        write(s, status, headers, body);
    }

    static QByteArray statusLine(int status)
    {
        return "HTTP/1.1 " + QByteArray::number(status) + " X\r\n";
    }

    static void write(QTcpSocket *s, int status, const QList<QPair<QByteArray, QByteArray>> &headers, const QByteArray &body)
    {
        QByteArray out = statusLine(status) + "Content-Length: " + QByteArray::number(body.size()) + "\r\n";
        for (const auto &h : headers)
            out += h.first + ": " + h.second + "\r\n";
        out += "Connection: close\r\n\r\n" + body;
        s->write(out);
        s->disconnectFromHost();
    }
};

// ---- fake RAUC ------------------------------------------------------------------------

struct RaucProgressStruct
{
    int percent = 0;
    QString message;
    int depth = 1;
};
Q_DECLARE_METATYPE(RaucProgressStruct)

inline QDBusArgument &operator<<(QDBusArgument &a, const RaucProgressStruct &p)
{
    a.beginStructure();
    a << p.percent << p.message << p.depth;
    a.endStructure();
    return a;
}
inline const QDBusArgument &operator>>(const QDBusArgument &a, RaucProgressStruct &p)
{
    a.beginStructure();
    a >> p.percent >> p.message >> p.depth;
    a.endStructure();
    return a;
}

struct RaucSlotEntry
{
    QString name;
    QVariantMap status;
};
Q_DECLARE_METATYPE(RaucSlotEntry)
typedef QList<RaucSlotEntry> RaucSlotList;
Q_DECLARE_METATYPE(RaucSlotList)

inline QDBusArgument &operator<<(QDBusArgument &a, const RaucSlotEntry &e)
{
    a.beginStructure();
    a << e.name << e.status;
    a.endStructure();
    return a;
}
inline const QDBusArgument &operator>>(const QDBusArgument &a, RaucSlotEntry &e)
{
    a.beginStructure();
    a >> e.name >> e.status;
    a.endStructure();
    return a;
}

// Bundles are text files "compatible=...\nversion=...\nsigned=yes|no\ninstall=ok|fail".
class FakeRauc : public QObject, protected QDBusContext
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "de.pengutronix.rauc.Installer")
    Q_PROPERTY(QString Operation READ operation)
    Q_PROPERTY(QString LastError READ lastError)
    Q_PROPERTY(QString Compatible READ compatible)
    Q_PROPERTY(QString BootSlot READ bootSlot)
public:
    explicit FakeRauc(const QDBusConnection &bus) : m_bus(bus)
    {
        qDBusRegisterMetaType<RaucProgressStruct>();
        qDBusRegisterMetaType<RaucSlotEntry>();
        qDBusRegisterMetaType<RaucSlotList>();
        m_bus.registerObject(QStringLiteral("/"), this,
                             QDBusConnection::ExportAllSlots | QDBusConnection::ExportAllSignals
                                 | QDBusConnection::ExportAllProperties);
        m_bus.registerService(QStringLiteral("de.pengutronix.rauc"));
    }
    ~FakeRauc() override
    {
        m_bus.unregisterService(QStringLiteral("de.pengutronix.rauc"));
        m_bus.unregisterObject(QStringLiteral("/"));
    }

    QString compatibleValue = QStringLiteral("customos-x86_64-ab");
    QString booted = QStringLiteral("A");
    QString otherVersion;
    bool otherGood = false;
    QStringList installed;   // bundle paths
    QStringList marks;       // "state slot"

    QString operation() const { return m_operation; }
    QString lastError() const { return m_lastError; }
    QString compatible() const { return compatibleValue; }
    QString bootSlot() const { return booted; }

public slots:
    QVariantMap InspectBundle(const QString &source, const QVariantMap &)
    {
        const QHash<QString, QString> b = readBundle(source);
        if (b.isEmpty()) {
            sendErrorReply(QDBusError::Failed, QStringLiteral("Failed to open bundle"));
            return {};
        }
        if (b.value(QStringLiteral("signed")) != QLatin1String("yes")) {
            sendErrorReply(QDBusError::Failed, QStringLiteral("signature verification failed: certificate verify failed"));
            return {};
        }
        return {{QStringLiteral("update"), QVariantMap{{QStringLiteral("compatible"), b.value(QStringLiteral("compatible"))},
                                                       {QStringLiteral("version"), b.value(QStringLiteral("version"))}}},
                {QStringLiteral("bundle"), QVariantMap{{QStringLiteral("format"), QStringLiteral("verity")}}}};
    }

    void InstallBundle(const QString &source, const QVariantMap &)
    {
        const QHash<QString, QString> b = readBundle(source);
        m_operation = QStringLiteral("installing");
        installed << source;
        const bool ok = b.value(QStringLiteral("install"), QStringLiteral("ok")) == QLatin1String("ok");
        for (int i = 1; i <= 4; ++i)
            QTimer::singleShot(i * 30, this, [this, i] { emitProgress(i * 25, QStringLiteral("Copying image to rootfs")); });
        QTimer::singleShot(160, this, [this, ok] {
            m_operation = QStringLiteral("idle");
            m_lastError = ok ? QString() : QStringLiteral("Failed writing image: No space left on device");
            emit Completed(ok ? 0 : 1);
        });
    }

    QString Mark(const QString &state, const QString &slot, QString &message)
    {
        marks << state + QLatin1Char(' ') + slot;
        message = QStringLiteral("marked slot %1 as %2").arg(slot, state);
        return slot == QLatin1String("other") ? QStringLiteral("rootfs.1") : QStringLiteral("rootfs.0");
    }

    RaucSlotList GetSlotStatus()
    {
        const QString other = booted == QLatin1String("A") ? QStringLiteral("B") : QStringLiteral("A");
        return {{QStringLiteral("rootfs.0"), slotMap(QStringLiteral("A"))},
                {QStringLiteral("rootfs.1"), slotMap(QStringLiteral("B"))}};
    }

signals:
    void Completed(int result);

private:
    QVariantMap slotMap(const QString &bootname) const
    {
        const bool isBooted = bootname == booted;
        QVariantMap m{{QStringLiteral("bootname"), bootname},
                      {QStringLiteral("state"), isBooted ? QStringLiteral("booted") : QStringLiteral("inactive")},
                      {QStringLiteral("boot-status"), isBooted || otherGood ? QStringLiteral("good") : QStringLiteral("bad")}};
        if (!isBooted && !otherVersion.isEmpty())
            m.insert(QStringLiteral("bundle.version"), otherVersion);
        return m;
    }

    void emitProgress(int percent, const QString &message)
    {
        RaucProgressStruct p{percent, message, 1};
        QDBusMessage sig = QDBusMessage::createSignal(QStringLiteral("/"), QStringLiteral("org.freedesktop.DBus.Properties"),
                                                      QStringLiteral("PropertiesChanged"));
        sig << QStringLiteral("de.pengutronix.rauc.Installer")
            << QVariantMap{{QStringLiteral("Progress"), QVariant::fromValue(p)}} << QStringList();
        m_bus.send(sig);
    }

    static QHash<QString, QString> readBundle(const QString &path)
    {
        QHash<QString, QString> out;
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly))
            return out;
        for (const QByteArray &line : f.readAll().split('\n')) {
            const int eq = line.indexOf('=');
            if (eq > 0)
                out.insert(QString::fromUtf8(line.left(eq)), QString::fromUtf8(line.mid(eq + 1)).trimmed());
        }
        return out;
    }

    QDBusConnection m_bus;
    QString m_operation = QStringLiteral("idle");
    QString m_lastError;
};
