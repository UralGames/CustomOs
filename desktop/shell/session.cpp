#include "session.h"
#include "appmodel.h"
#include "displaycontrol.h"
#include "meridian.h"
#include "notificationserver.h"
#include "settings.h"

#include <QDateTime>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QProcess>
#include <QQmlEngine>
#include <QSaveFile>
#include <QStandardPaths>
#include <QTimer>

#include <csignal>
#include <sys/types.h>

namespace {
Session *s_instance = nullptr;

QString configHome()
{
    QString c = qEnvironmentVariable("XDG_CONFIG_HOME");
    return c.isEmpty() ? QDir::homePath() + QStringLiteral("/.config") : c;
}

bool writeIfChanged(const QString &path, const QByteArray &content)
{
    QFile cur(path);
    if (cur.open(QIODevice::ReadOnly) && cur.readAll() == content)
        return false;
    cur.close();
    QDir().mkpath(QFileInfo(path).absolutePath());
    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly))
        return false;
    f.write(content);
    return f.commit();
}

QString xmlEscape(QString s)
{
    s.replace(QLatin1Char('&'), QLatin1String("&amp;"));
    s.replace(QLatin1Char('<'), QLatin1String("&lt;"));
    s.replace(QLatin1Char('>'), QLatin1String("&gt;"));
    s.replace(QLatin1Char('"'), QLatin1String("&quot;"));
    return s;
}
} // namespace

Session *Session::instance()
{
    if (!s_instance)
        s_instance = new Session();
    return s_instance;
}

Session *Session::create(QQmlEngine *, QJSEngine *)
{
    Session *s = instance();
    QQmlEngine::setObjectOwnership(s, QQmlEngine::CppOwnership);
    return s;
}

Session::Session(QObject *parent)
    : QObject(parent)
{
    m_configTimer = new QTimer(this);
    m_configTimer->setSingleShot(true);
    m_configTimer->setInterval(300);
    connect(m_configTimer, &QTimer::timeout, this, &Session::writeCompositorConfig);
}

void Session::start()
{
    // Settings that the compositor applies (theme, input, desktops).
    static const QStringList compositorKeys{
        QStringLiteral("darkMode"), QStringLiteral("roundedCorners"), QStringLiteral("desktopCount"),
        QStringLiteral("keyboardLayout"), QStringLiteral("keyboardVariant"), QStringLiteral("repeatRate"),
        QStringLiteral("repeatDelay"), QStringLiteral("naturalScroll"), QStringLiteral("leftHanded"),
        QStringLiteral("pointerSpeed"), QStringLiteral("tapToClick")};
    connect(Settings::instance(), &Settings::settingChanged, this, [this](const QString &key) {
        if (compositorKeys.contains(key))
            m_configTimer->start();
    });
    writeCompositorConfig();
    DisplayControl::instance()->applySaved();

    if (qEnvironmentVariableIsEmpty("MERIDIAN_NO_AUTOSTART"))
        AppModel::instance()->runAutostart();
}

// Fills the rc.xml template shipped in <datadir>/labwc/rc.xml.in.
QString Session::renderRcXml(const QString &templ)
{
    Settings *s = Settings::instance();
    auto boolStr = [](bool b) { return b ? QStringLiteral("yes") : QStringLiteral("no"); };
    QString out = templ;

    const int desktops = qBound(1, s->value(QStringLiteral("desktopCount")).toInt(), 9);
    QString names;
    for (int i = 1; i <= desktops; ++i)
        names += QStringLiteral("      <name>%1</name>\n").arg(xmlEscape(tr("Desktop %1").arg(i)));

    out.replace(QStringLiteral("@THEME@"), s->value(QStringLiteral("darkMode")).toBool()
                                               ? QStringLiteral("Meridian-Dark") : QStringLiteral("Meridian-Light"));
    out.replace(QStringLiteral("@CORNER_RADIUS@"), s->value(QStringLiteral("roundedCorners")).toBool()
                                                       ? QStringLiteral("10") : QStringLiteral("0"));
    out.replace(QStringLiteral("@DESKTOP_NAMES@"), names.trimmed());
    out.replace(QStringLiteral("@REPEAT_RATE@"), QString::number(qBound(5, s->value(QStringLiteral("repeatRate")).toInt(), 80)));
    out.replace(QStringLiteral("@REPEAT_DELAY@"), QString::number(qBound(150, s->value(QStringLiteral("repeatDelay")).toInt(), 1500)));
    out.replace(QStringLiteral("@NATURAL_SCROLL@"), boolStr(s->value(QStringLiteral("naturalScroll")).toBool()));
    out.replace(QStringLiteral("@LEFT_HANDED@"), boolStr(s->value(QStringLiteral("leftHanded")).toBool()));
    out.replace(QStringLiteral("@TAP@"), boolStr(s->value(QStringLiteral("tapToClick")).toBool()));
    out.replace(QStringLiteral("@POINTER_SPEED@"),
                QString::number(qBound(-1.0, s->value(QStringLiteral("pointerSpeed")).toDouble(), 1.0), 'f', 2));
    return out;
}

void Session::writeCompositorConfig()
{
    QFile templ(Meridian::dataDir() + QStringLiteral("/labwc/rc.xml.in"));
    if (!templ.open(QIODevice::ReadOnly)) {
        qWarning("meridian-shell: labwc template not found in %s", qPrintable(Meridian::dataDir()));
        return;
    }
    const QString rc = renderRcXml(QString::fromUtf8(templ.readAll()));
    bool changed = writeIfChanged(configHome() + QStringLiteral("/labwc/rc.xml"), rc.toUtf8());

    Settings *s = Settings::instance();
    QString env;
    env += QStringLiteral("# Generated by Meridian from ~/.config/meridian/desktop.conf; edits are overwritten.\n");
    env += QStringLiteral("XKB_DEFAULT_LAYOUT=%1\n").arg(s->value(QStringLiteral("keyboardLayout")).toString());
    const QString variant = s->value(QStringLiteral("keyboardVariant")).toString();
    if (!variant.isEmpty())
        env += QStringLiteral("XKB_DEFAULT_VARIANT=%1\n").arg(variant);
    env += QStringLiteral("XCURSOR_THEME=Adwaita\nXCURSOR_SIZE=24\n");
    changed |= writeIfChanged(configHome() + QStringLiteral("/labwc/environment"), env.toUtf8());

    if (changed)
        reconfigureCompositor();
}

void Session::reconfigureCompositor()
{
    // labwc reloads rc.xml, themes and the environment file on SIGHUP.
    bool ok = false;
    const qint64 pid = qEnvironmentVariable("LABWC_PID").toLongLong(&ok);
    if (ok && pid > 1)
        ::kill(pid_t(pid), SIGHUP);
}

bool Session::callLogind(const QString &method)
{
    QDBusInterface logind(QStringLiteral("org.freedesktop.login1"), QStringLiteral("/org/freedesktop/login1"),
                          QStringLiteral("org.freedesktop.login1.Manager"), QDBusConnection::systemBus());
    if (logind.isValid()) {
        const QDBusMessage reply = logind.call(method, true);
        if (reply.type() != QDBusMessage::ErrorMessage)
            return true;
        qWarning("meridian-shell: logind %s failed: %s", qPrintable(method), qPrintable(reply.errorMessage()));
    }
    const QString verb = method == QLatin1String("PowerOff") ? QStringLiteral("poweroff") : QStringLiteral("reboot");
    return QProcess::startDetached(QStringLiteral("systemctl"), {verb});
}

void Session::logout()
{
    // Ends the compositor; the login service then shows the sign-in screen.
    if (!QProcess::startDetached(QStringLiteral("labwc"), {QStringLiteral("--exit")}))
        QGuiApplication::quit();
}

void Session::powerOff()
{
    if (!callLogind(QStringLiteral("PowerOff")))
        NotificationServer::instance()->post(tr("System"), tr("Could not shut down"),
                                             tr("The system refused the request."), QStringLiteral("power"));
}

void Session::reboot()
{
    if (!callLogind(QStringLiteral("Reboot")))
        NotificationServer::instance()->post(tr("System"), tr("Could not restart"),
                                             tr("The system refused the request."), QStringLiteral("restart"));
}

void Session::lock()
{
    // swaylock implements ext-session-lock: if anything crashes the session
    // stays locked. Its look comes from /etc/swaylock/config.
    QStringList args{QStringLiteral("-f")};
    const QString wp = Settings::instance()->value(QStringLiteral("wallpaper")).toString();
    if (QFileInfo::exists(wp))
        args << QStringLiteral("-i") << wp << QStringLiteral("-s") << QStringLiteral("fill");
    if (!QProcess::startDetached(QStringLiteral("swaylock"), args))
        NotificationServer::instance()->post(tr("System"), tr("Cannot lock the screen"),
                                             tr("swaylock is not installed."), QStringLiteral("warning"));
}

void Session::screenshot()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation)
                        + QStringLiteral("/Screenshots");
    QDir().mkpath(dir);
    const QString path = dir + QStringLiteral("/Screenshot_")
                         + QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd_HH-mm-ss"))
                         + QStringLiteral(".png");
    auto *p = new QProcess(this);
    p->setProcessChannelMode(QProcess::MergedChannels);
    connect(p, &QProcess::finished, this, [this, p, path](int code) {
        const QString out = QString::fromUtf8(p->readAll()).trimmed();
        p->deleteLater();
        if (code == 0 && QFileInfo::exists(path)) emit screenshotSaved(path);
        else emit screenshotFailed(out);
    });
    connect(p, &QProcess::errorOccurred, this, [this, p](QProcess::ProcessError e) {
        if (e != QProcess::FailedToStart) return;
        p->deleteLater();
        emit screenshotFailed(tr("grim is not installed."));
    });
    p->start(QStringLiteral("grim"), {path});
}
