// meridian-installer: graphical installer for the live image.
// Runs as the live user; the privileged work is done by customos-install
// through sudo.

#include "meridian.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QtQml/qqmlextensionplugin.h>

MERIDIAN_IMPORT_QML_MODULES()

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    Meridian::setupApplication(app, QStringLiteral("meridian-installer"), QObject::tr("Installer"));
    QQmlApplicationEngine engine;
    Meridian::setupEngine(engine);
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app,
                     [] { QCoreApplication::exit(1); }, Qt::QueuedConnection);
    engine.load(QUrl(QStringLiteral("qrc:/Meridian/Installer/Main.qml")));
    if (engine.rootObjects().isEmpty())
        return 1;
    return app.exec();
}
