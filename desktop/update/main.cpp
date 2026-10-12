// meridian-update: the Meridian Update application.
//   meridian-update             open the Updates window
//   meridian-update --notify    background agent of the desktop session that
//                               shows notifications for update events
//   meridian-update --screenshot FILE [--dark]
//                               render the window to FILE and quit (development)

#include "meridian.h"
#include "updatenotifier.h"

#include <QCoreApplication>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QTimer>
#include <QtQml/qqmlextensionplugin.h>

MERIDIAN_IMPORT_QML_MODULES()
Q_IMPORT_QML_PLUGIN(Meridian_UpdatePlugin)

int main(int argc, char *argv[])
{
    for (int i = 1; i < argc; ++i) {
        if (qstrcmp(argv[i], "--notify") == 0) {
            QCoreApplication app(argc, argv);
            QCoreApplication::setOrganizationName(QStringLiteral("Meridian"));
            QCoreApplication::setApplicationName(QStringLiteral("meridian-update"));
            UpdateNotifier notifier;
            return app.exec();
        }
    }

    QGuiApplication app(argc, argv);
    Meridian::setupApplication(app, QStringLiteral("meridian-update"), QObject::tr("Meridian Update"));

    const QStringList args = app.arguments();
    const int shot = args.indexOf(QStringLiteral("--screenshot"));
    const QString shotFile = shot > 0 && shot + 1 < args.size() ? args.at(shot + 1) : QString();

    QQmlApplicationEngine engine;
    Meridian::setupEngine(engine);
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app,
                     [] { QCoreApplication::exit(1); }, Qt::QueuedConnection);
    engine.load(QUrl(QStringLiteral("qrc:/Meridian/UpdateApp/Main.qml")));
    if (engine.rootObjects().isEmpty())
        return 1;

    if (!shotFile.isEmpty()) {
        auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().constFirst());
        QTimer::singleShot(2500, &app, [window, shotFile] {
            window->grabWindow().save(shotFile);
            QCoreApplication::quit();
        });
    }
    return app.exec();
}
