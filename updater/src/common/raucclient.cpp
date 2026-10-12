#include "raucclient.h"

#include <QDBusArgument>
#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusReply>
#include <QDBusVariant>

const char *const RaucClient::Service = "de.pengutronix.rauc";
const char *const RaucClient::Path = "/";
const char *const RaucClient::Interface = "de.pengutronix.rauc.Installer";

// Long-running calls (signature checks of large bundles) get a generous timeout.
static constexpr int LongTimeoutMs = 10 * 60 * 1000;

RaucClient::RaucClient(const QDBusConnection &bus, QObject *parent)
    : QObject(parent), m_bus(bus)
{
    m_bus.connect(QString::fromLatin1(Service), QString::fromLatin1(Path),
                  QStringLiteral("org.freedesktop.DBus.Properties"), QStringLiteral("PropertiesChanged"),
                  this, SLOT(onPropertiesChanged(QString,QVariantMap,QStringList)));
    m_bus.connect(QString::fromLatin1(Service), QString::fromLatin1(Path), QString::fromLatin1(Interface),
                  QStringLiteral("Completed"), this, SLOT(onCompleted(int)));
}

bool RaucClient::isAvailable() const
{
    QDBusConnectionInterface *iface = m_bus.interface();
    if (!iface)
        return false;
    if (iface->isServiceRegistered(QString::fromLatin1(Service)))
        return true;
    // RAUC is D-Bus activatable: starting it on demand is fine.
    const QDBusReply<void> r = iface->startService(QString::fromLatin1(Service));
    return r.isValid() && iface->isServiceRegistered(QString::fromLatin1(Service));
}

QVariant RaucClient::property(const char *name) const
{
    QDBusMessage m = QDBusMessage::createMethodCall(QString::fromLatin1(Service), QString::fromLatin1(Path),
                                                    QStringLiteral("org.freedesktop.DBus.Properties"),
                                                    QStringLiteral("Get"));
    m << QString::fromLatin1(Interface) << QString::fromLatin1(name);
    const QDBusMessage reply = m_bus.call(m, QDBus::Block, 5000);
    if (reply.type() != QDBusMessage::ReplyMessage || reply.arguments().isEmpty())
        return QVariant();
    return reply.arguments().constFirst().value<QDBusVariant>().variant();
}

QString RaucClient::compatible() const { return property("Compatible").toString(); }
QString RaucClient::bootSlot() const { return property("BootSlot").toString(); }
QString RaucClient::lastError() const { return property("LastError").toString(); }
QString RaucClient::operation() const { return property("Operation").toString(); }

static QVariantMap toMap(const QVariant &v)
{
    if (v.canConvert<QDBusArgument>())
        return qdbus_cast<QVariantMap>(v.value<QDBusArgument>());
    if (v.canConvert<QDBusVariant>())
        return toMap(v.value<QDBusVariant>().variant());
    return v.toMap();
}

static QString errorText(const QDBusError &e)
{
    // RAUC puts its own explanation in the message ("signature verification failed: ...").
    return e.message().isEmpty() ? e.name() : e.message();
}

void RaucClient::inspect(const QString &bundlePath, std::function<void(const BundleInfo &, const QString &)> done)
{
    QDBusMessage m = QDBusMessage::createMethodCall(QString::fromLatin1(Service), QString::fromLatin1(Path),
                                                    QString::fromLatin1(Interface), QStringLiteral("InspectBundle"));
    m << bundlePath << QVariant::fromValue(QVariantMap());
    auto *w = new QDBusPendingCallWatcher(m_bus.asyncCall(m, LongTimeoutMs), this);
    connect(w, &QDBusPendingCallWatcher::finished, this, [this, w, bundlePath, done] {
        w->deleteLater();
        const QDBusMessage reply = w->reply();
        if (reply.type() == QDBusMessage::ErrorMessage) {
            const QDBusError err(reply);
            if (err.type() == QDBusError::UnknownMethod) {
                // RAUC < 1.10: the older Info method (verifies the signature too).
                QDBusMessage legacy = QDBusMessage::createMethodCall(
                    QString::fromLatin1(Service), QString::fromLatin1(Path), QString::fromLatin1(Interface),
                    QStringLiteral("Info"));
                legacy << bundlePath;
                auto *w2 = new QDBusPendingCallWatcher(m_bus.asyncCall(legacy, LongTimeoutMs), this);
                connect(w2, &QDBusPendingCallWatcher::finished, this, [w2, done] {
                    w2->deleteLater();
                    QDBusPendingReply<QString, QString> r = *w2;
                    BundleInfo info;
                    if (r.isError()) {
                        done(info, errorText(r.error()));
                        return;
                    }
                    info.compatible = r.argumentAt<0>();
                    info.version = r.argumentAt<1>();
                    done(info, QString());
                });
                return;
            }
            done(BundleInfo(), errorText(err));
            return;
        }
        const QVariantMap root = reply.arguments().isEmpty() ? QVariantMap() : toMap(reply.arguments().constFirst());
        const QVariantMap update = toMap(root.value(QStringLiteral("update")));
        const QVariantMap bundle = toMap(root.value(QStringLiteral("bundle")));
        BundleInfo info;
        info.compatible = update.value(QStringLiteral("compatible")).toString();
        info.version = update.value(QStringLiteral("version")).toString();
        info.build = update.value(QStringLiteral("build")).toString();
        info.description = update.value(QStringLiteral("description")).toString();
        info.format = bundle.value(QStringLiteral("format")).toString();
        done(info, QString());
    });
}

void RaucClient::install(const QString &bundlePath, std::function<void(const QString &)> started)
{
    QDBusMessage m = QDBusMessage::createMethodCall(QString::fromLatin1(Service), QString::fromLatin1(Path),
                                                    QString::fromLatin1(Interface), QStringLiteral("InstallBundle"));
    m << bundlePath << QVariant::fromValue(QVariantMap());
    auto *w = new QDBusPendingCallWatcher(m_bus.asyncCall(m, LongTimeoutMs), this);
    connect(w, &QDBusPendingCallWatcher::finished, this, [w, started] {
        w->deleteLater();
        const QDBusMessage reply = w->reply();
        started(reply.type() == QDBusMessage::ErrorMessage ? errorText(QDBusError(reply)) : QString());
    });
}

void RaucClient::mark(const QString &state, const QString &slot,
                      std::function<void(const QString &, const QString &, const QString &)> done)
{
    QDBusMessage m = QDBusMessage::createMethodCall(QString::fromLatin1(Service), QString::fromLatin1(Path),
                                                    QString::fromLatin1(Interface), QStringLiteral("Mark"));
    m << state << slot;
    auto *w = new QDBusPendingCallWatcher(m_bus.asyncCall(m, 60000), this);
    connect(w, &QDBusPendingCallWatcher::finished, this, [w, done] {
        w->deleteLater();
        QDBusPendingReply<QString, QString> r = *w;
        if (r.isError())
            done(QString(), QString(), errorText(r.error()));
        else
            done(r.argumentAt<0>(), r.argumentAt<1>(), QString());
    });
}

void RaucClient::slotStatus(std::function<void(const QList<Slot> &, const QString &)> done)
{
    QDBusMessage m = QDBusMessage::createMethodCall(QString::fromLatin1(Service), QString::fromLatin1(Path),
                                                    QString::fromLatin1(Interface), QStringLiteral("GetSlotStatus"));
    auto *w = new QDBusPendingCallWatcher(m_bus.asyncCall(m, 30000), this);
    connect(w, &QDBusPendingCallWatcher::finished, this, [w, done] {
        w->deleteLater();
        const QDBusMessage reply = w->reply();
        QList<Slot> slotList;
        if (reply.type() == QDBusMessage::ErrorMessage || reply.arguments().isEmpty()) {
            done(slotList, errorText(QDBusError(reply)));
            return;
        }
        // a(sa{sv})
        const QDBusArgument arg = reply.arguments().constFirst().value<QDBusArgument>();
        arg.beginArray();
        while (!arg.atEnd()) {
            QString name;
            QVariantMap map;
            arg.beginStructure();
            arg >> name >> map;
            arg.endStructure();
            Slot s;
            s.name = name;
            s.bootname = map.value(QStringLiteral("bootname")).toString();
            s.state = map.value(QStringLiteral("state")).toString();
            s.bootStatus = map.value(QStringLiteral("boot-status")).toString();
            s.bundleVersion = map.value(QStringLiteral("bundle.version")).toString();
            s.installedTimestamp = map.value(QStringLiteral("installed.timestamp")).toString();
            s.device = map.value(QStringLiteral("device")).toString();
            slotList.append(s);
        }
        arg.endArray();
        done(slotList, QString());
    });
}

void RaucClient::onPropertiesChanged(const QString &interface, const QVariantMap &changed, const QStringList &)
{
    if (interface != QLatin1String(Interface))
        return;
    const auto it = changed.constFind(QStringLiteral("Progress"));
    if (it == changed.constEnd())
        return;
    // (isi): percentage, message, nesting depth
    QVariant v = it.value();
    if (v.canConvert<QDBusVariant>())
        v = v.value<QDBusVariant>().variant();
    const QDBusArgument arg = v.value<QDBusArgument>();
    int percent = 0, depth = 0;
    QString message;
    arg.beginStructure();
    arg >> percent >> message >> depth;
    arg.endStructure();
    emit progressChanged(percent, message);
}

void RaucClient::onCompleted(int result)
{
    emit completed(result);
}
