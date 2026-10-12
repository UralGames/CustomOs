// meridian-settings: the Meridian settings app.
//   meridian-settings [PAGE]   PAGE: system display sound network appearance taskbar
//                                    multitasking input notifications time user

#include "meridian.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QtQml/qqmlextensionplugin.h>

MERIDIAN_IMPORT_QML_MODULES()

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    Meridian::setupApplication(app, QStringLiteral("meridian-settings"), QObject::tr("Settings"));

    const QStringList args = app.arguments().mid(1);
    QQmlApplicationEngine engine;
    Meridian::setupEngine(engine);
    engine.setInitialProperties({{QStringLiteral("startPage"), args.isEmpty() ? QStringLiteral("system") : args.first()}});
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app,
                     [] { QCoreApplication::exit(1); }, Qt::QueuedConnection);
    engine.load(QUrl(QStringLiteral("qrc:/Meridian/Settings/Main.qml")));
    if (engine.rootObjects().isEmpty())
        return 1;
    return app.exec();
}
