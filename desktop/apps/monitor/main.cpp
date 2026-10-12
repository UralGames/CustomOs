// meridian-monitor: CPU, memory, network and disk use, and running processes.

#include "meridian.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QtQml/qqmlextensionplugin.h>

MERIDIAN_IMPORT_QML_MODULES()

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    Meridian::setupApplication(app, QStringLiteral("meridian-monitor"), QObject::tr("System Monitor"));
    QQmlApplicationEngine engine;
    Meridian::setupEngine(engine);
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app,
                     [] { QCoreApplication::exit(1); }, Qt::QueuedConnection);
    engine.load(QUrl(QStringLiteral("qrc:/Meridian/Monitor/Main.qml")));
    if (engine.rootObjects().isEmpty())
        return 1;
    return app.exec();
}
