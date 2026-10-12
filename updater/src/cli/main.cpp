// customos-update: command-line client of the CustomOS update service.

#include "dbusnames.h"
#include "version.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextStream>
#include <QThread>

#include <cstdio>
#include <unistd.h>

static const char Usage[] =
    "Usage: customos-update [--session] [--json] COMMAND\n"
    "\n"
    "Commands:\n"
    "  status             show the update status (default)\n"
    "  check              check GitHub for a newer CustomOS release\n"
    "  download           download and verify the available update\n"
    "  install [--yes]    download, verify and install the update into the inactive slot\n"
    "  reboot             restart the computer to finish an installed update\n"
    "  rollback [--yes]   start the previous version on the next restart\n"
    "  cancel             stop a running download (it resumes later)\n"
    "  history            list installed updates and failures\n"
    "  dismiss            clear the last error\n"
    "  compare-versions A B\n"
    "                     exit 0 if A > B, 1 if A == B, 2 if A < B, 3 if a version is invalid\n";

static QTextStream out(stdout);
static QTextStream err(stderr);

static QString formatBytes(double b)
{
    const char *units[] = {"B", "KB", "MB", "GB"};
    int i = 0;
    while (b >= 1000 && i < 3) { b /= 1000; ++i; }
    return QStringLiteral("%1 %2").arg(b, 0, 'f', i == 0 ? 0 : 1).arg(QLatin1String(units[i]));
}

static QString formatTime(qint64 secs)
{
    return secs > 0 ? QDateTime::fromSecsSinceEpoch(secs).toString(QStringLiteral("yyyy-MM-dd HH:mm")) : QStringLiteral("never");
}

static bool confirm(const QString &question)
{
    if (!isatty(STDIN_FILENO)) {
        err << question << " [refused: not a terminal; use --yes]\n";
        err.flush();
        return false;
    }
    out << question << " [y/N] ";
    out.flush();
    char buf[16] = {0};
    if (!fgets(buf, sizeof buf, stdin))
        return false;
    return buf[0] == 'y' || buf[0] == 'Y';
}

static void printStatus(const QVariantMap &s)
{
    const QString state = s.value(QStringLiteral("state")).toString();
    out << s.value(QStringLiteral("prettyName")).toString() << "\n";
    out << "  Installed version:   " << s.value(QStringLiteral("currentVersion")).toString();
    const QString slot = s.value(QStringLiteral("bootedSlot")).toString();
    if (!slot.isEmpty())
        out << " (slot " << slot << ")";
    out << "\n";
    out << "  Update source:       " << s.value(QStringLiteral("repository")).toString() << " ("
        << s.value(QStringLiteral("channel")).toString() << ")\n";
    out << "  Last check:          " << formatTime(s.value(QStringLiteral("lastCheck")).toLongLong()) << "\n";
    const QString lastOk = s.value(QStringLiteral("lastSuccessVersion")).toString();
    out << "  Last update:         "
        << (lastOk.isEmpty() ? QStringLiteral("none")
                             : lastOk + QStringLiteral(" on ") + formatTime(s.value(QStringLiteral("lastSuccessTime")).toLongLong()))
        << "\n";
    if (s.value(QStringLiteral("rollbackAvailable")).toBool())
        out << "  Previous version:    " << s.value(QStringLiteral("otherSlotVersion")).toString()
            << " (slot " << s.value(QStringLiteral("otherSlot")).toString() << ", can be restored)\n";
    out << "  State:               " << state << "\n";
    if (s.value(QStringLiteral("updateAvailable")).toBool()) {
        out << "  Available:           " << s.value(QStringLiteral("availableVersion")).toString() << " — "
            << s.value(QStringLiteral("releaseName")).toString() << ", "
            << formatBytes(s.value(QStringLiteral("downloadSize")).toDouble())
            << (s.value(QStringLiteral("downloaded")).toBool() ? ", downloaded and verified" : "") << "\n";
    }
    if (!s.value(QStringLiteral("supported")).toBool())
        out << "  Note:                " << s.value(QStringLiteral("unsupportedMessage")).toString() << "\n";
    if (state == QLatin1String("error")) {
        out << "  Error:               " << s.value(QStringLiteral("errorMessage")).toString() << "\n";
        const QString detail = s.value(QStringLiteral("errorDetail")).toString();
        if (!detail.isEmpty())
            out << "  Details:             " << detail << "\n";
    }
    if (state == QLatin1String("reboot-required"))
        out << "  Restart the computer to start " << s.value(QStringLiteral("pendingVersion")).toString()
            << " (customos-update reboot).\n";
    out.flush();
}

static void printJson(const QVariantMap &s)
{
    out << QJsonDocument(QJsonObject::fromVariantMap(s)).toJson();
    out.flush();
}

// Follows the service until it leaves the given states; prints progress.
static QVariantMap follow(QDBusInterface &iface, const QStringList &busyStates, bool quiet)
{
    QVariantMap s;
    QString lastLine;
    const bool tty = isatty(STDOUT_FILENO);
    for (int i = 0; i < 20 * 60 * 60 * 3; ++i) {   // up to 3 hours
        const QDBusReply<QVariantMap> r = iface.call(QStringLiteral("GetStatus"));
        if (!r.isValid()) {
            err << "customos-update: lost the connection to the update service: " << r.error().message() << "\n";
            err.flush();
            return QVariantMap();
        }
        s = r.value();
        const QString state = s.value(QStringLiteral("state")).toString();
        if (!busyStates.contains(state))
            break;
        if (!quiet) {
            const int pct = s.value(QStringLiteral("progress")).toInt();
            QString line = s.value(QStringLiteral("progressText")).toString();
            if (pct >= 0)
                line = QStringLiteral("[%1%] ").arg(pct, 3) + line;
            const double rate = s.value(QStringLiteral("downloadRate")).toDouble();
            if (state == QLatin1String("downloading") && rate > 0)
                line += QStringLiteral(" (%1/s)").arg(formatBytes(rate));
            if (line != lastLine) {
                if (tty)
                    out << "\r\033[K" << line;
                else
                    out << line << "\n";
                out.flush();
                lastLine = line;
            }
        }
        QThread::msleep(300);
    }
    if (!quiet && tty && !lastLine.isEmpty())
        out << "\n";
    return s;
}

static int resultOf(const QVariantMap &s)
{
    if (s.isEmpty())
        return 1;
    if (s.value(QStringLiteral("state")).toString() == QLatin1String("error")) {
        err << "Error: " << s.value(QStringLiteral("errorMessage")).toString() << "\n";
        const QString detail = s.value(QStringLiteral("errorDetail")).toString();
        if (!detail.isEmpty())
            err << "Details: " << detail << "\n";
        err.flush();
        return 1;
    }
    return 0;
}

static bool callOk(QDBusInterface &iface, const QString &method)
{
    const QDBusMessage r = iface.call(method);
    if (r.type() == QDBusMessage::ErrorMessage) {
        err << "Error: " << r.errorMessage() << "\n";
        err.flush();
        return false;
    }
    return true;
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QStringList args = app.arguments().mid(1);
    const bool session = args.removeAll(QStringLiteral("--session")) > 0
                         || qEnvironmentVariable("CUSTOMOS_UPDATE_BUS") == QLatin1String("session");
    const bool json = args.removeAll(QStringLiteral("--json")) > 0;
    const bool yes = args.removeAll(QStringLiteral("--yes")) > 0 || args.removeAll(QStringLiteral("-y")) > 0;
    const QString cmd = args.isEmpty() ? QStringLiteral("status") : args.takeFirst();

    if (cmd == QLatin1String("-h") || cmd == QLatin1String("--help") || cmd == QLatin1String("help")) {
        out << Usage;
        return 0;
    }
    if (cmd == QLatin1String("compare-versions")) {
        if (args.size() != 2) {
            err << Usage;
            return 3;
        }
        const Version a = Version::parse(args.at(0)), b = Version::parse(args.at(1));
        if (!a.isValid() || !b.isValid())
            return 3;
        return a > b ? 0 : (a == b ? 1 : 2);
    }

    QDBusConnection bus = session ? QDBusConnection::sessionBus() : QDBusConnection::systemBus();
    QDBusInterface iface(QStringLiteral(CUSTOMOS_UPDATE_SERVICE), QStringLiteral(CUSTOMOS_UPDATE_PATH),
                         QStringLiteral(CUSTOMOS_UPDATE_INTERFACE), bus);
    if (!iface.isValid()) {
        err << "customos-update: the update service is not running (" << iface.lastError().message() << ").\n";
        return 1;
    }
    iface.setTimeout(30000);

    auto status = [&iface]() {
        const QDBusReply<QVariantMap> r = iface.call(QStringLiteral("GetStatus"));
        return r.isValid() ? r.value() : QVariantMap();
    };

    if (cmd == QLatin1String("status")) {
        const QVariantMap s = status();
        if (s.isEmpty())
            return 1;
        json ? printJson(s) : printStatus(s);
        return 0;
    }
    if (cmd == QLatin1String("history")) {
        const QDBusReply<QString> r = iface.call(QStringLiteral("GetHistory"));
        if (!r.isValid()) {
            err << "Error: " << r.error().message() << "\n";
            return 1;
        }
        const QJsonArray h = QJsonDocument::fromJson(r.value().toUtf8()).array();
        if (json) {
            out << QJsonDocument(h).toJson();
            return 0;
        }
        if (h.isEmpty())
            out << "No updates have been installed yet.\n";
        for (const QJsonValue &v : h) {
            const QJsonObject e = v.toObject();
            const QDateTime t = QDateTime::fromString(e.value(QLatin1String("time")).toString(), Qt::ISODate).toLocalTime();
            out << t.toString(QStringLiteral("yyyy-MM-dd HH:mm")) << "  " << e.value(QLatin1String("event")).toString().leftJustified(9)
                << e.value(QLatin1String("result")).toString().leftJustified(12)
                << e.value(QLatin1String("from_version")).toString() << " -> " << e.value(QLatin1String("to_version")).toString()
                << "  " << e.value(QLatin1String("message")).toString() << "\n";
        }
        return 0;
    }
    if (cmd == QLatin1String("check")) {
        if (!callOk(iface, QStringLiteral("CheckForUpdates")))
            return 1;
        const QVariantMap s = follow(iface, {QStringLiteral("checking")}, json);
        if (json) {
            printJson(s);
            return resultOf(s);
        }
        if (resultOf(s) != 0)
            return 1;
        if (s.value(QStringLiteral("updateAvailable")).toBool())
            out << "CustomOS " << s.value(QStringLiteral("availableVersion")).toString() << " is available ("
                << formatBytes(s.value(QStringLiteral("downloadSize")).toDouble()) << "). Install it with: customos-update install\n";
        else
            out << "CustomOS " << s.value(QStringLiteral("currentVersion")).toString() << " is up to date.\n";
        if (!s.value(QStringLiteral("supported")).toBool())
            out << s.value(QStringLiteral("unsupportedMessage")).toString() << "\n";
        return 0;
    }
    if (cmd == QLatin1String("download") || cmd == QLatin1String("install")) {
        const bool doInstall = cmd == QLatin1String("install");
        QVariantMap s = status();
        if (doInstall && !yes) {
            if (!s.value(QStringLiteral("updateAvailable")).toBool()) {
                err << "No update is available. Run: customos-update check\n";
                return 1;
            }
            if (!confirm(QStringLiteral("Install CustomOS %1 into the inactive slot?")
                             .arg(s.value(QStringLiteral("availableVersion")).toString())))
                return 1;
        }
        if (!callOk(iface, doInstall ? QStringLiteral("Install") : QStringLiteral("Download")))
            return 1;
        s = follow(iface, {QStringLiteral("downloading"), QStringLiteral("verifying"), QStringLiteral("installing")}, json);
        if (json)
            printJson(s);
        if (resultOf(s) != 0)
            return 1;
        if (!json) {
            if (doInstall)
                out << "CustomOS " << s.value(QStringLiteral("pendingVersion")).toString()
                    << " is installed. Restart to start using it: customos-update reboot\n";
            else
                out << "The update is downloaded and verified. Install it with: customos-update install\n";
        }
        return 0;
    }
    if (cmd == QLatin1String("rollback")) {
        const QVariantMap s = status();
        if (!s.value(QStringLiteral("rollbackAvailable")).toBool()) {
            err << "There is no previous version to go back to.\n";
            return 1;
        }
        if (!yes && !confirm(QStringLiteral("Start CustomOS %1 (slot %2) on the next restart?")
                                 .arg(s.value(QStringLiteral("otherSlotVersion")).toString(),
                                      s.value(QStringLiteral("otherSlot")).toString())))
            return 1;
        if (!callOk(iface, QStringLiteral("Rollback")))
            return 1;
        const QVariantMap after = follow(iface, {}, true);
        if (resultOf(after) != 0)
            return 1;
        out << "The previous version will start after a restart: customos-update reboot\n";
        return 0;
    }
    if (cmd == QLatin1String("reboot"))
        return callOk(iface, QStringLiteral("Reboot")) ? 0 : 1;
    if (cmd == QLatin1String("cancel"))
        return callOk(iface, QStringLiteral("Cancel")) ? 0 : 1;
    if (cmd == QLatin1String("dismiss"))
        return callOk(iface, QStringLiteral("DismissError")) ? 0 : 1;

    err << "customos-update: unknown command \"" << cmd << "\"\n\n" << Usage;
    return 2;
}
