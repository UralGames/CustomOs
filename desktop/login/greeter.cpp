// meridian-greeter: the sign-in screen, run by meridian-login (as root, on
// the console). Uses Qt's linuxfb platform with the software renderer by
// default, so it works on every graphics adapter without 3D drivers.

#include "meridian.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QtQml/qqmlextensionplugin.h>

MERIDIAN_IMPORT_QML_MODULES()

int main(int argc, char *argv[])
{
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM"))
        qputenv("QT_QPA_PLATFORM", "linuxfb");
    if (qEnvironmentVariableIsEmpty("QT_QUICK_BACKEND"))
        qputenv("QT_QUICK_BACKEND", "software");

    QGuiApplication app(argc, argv);
    Meridian::setupApplication(app, QStringLiteral("meridian-greeter"), QObject::tr("Sign in"));

    QQmlApplicationEngine engine;
    Meridian::setupEngine(engine);
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app,
                     [] { QCoreApplication::exit(1); }, Qt::QueuedConnection);
    engine.load(QUrl(QStringLiteral("qrc:/Meridian/Greeter/Main.qml")));
    if (engine.rootObjects().isEmpty())
        return 1;
    return app.exec();
}
