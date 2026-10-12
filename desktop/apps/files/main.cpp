// meridian-files: the Meridian file manager.
//   meridian-files [FOLDER]

#include "meridian.h"

#include <QDir>
#include <QFileInfo>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QUrl>
#include <QtQml/qqmlextensionplugin.h>

MERIDIAN_IMPORT_QML_MODULES()

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    Meridian::setupApplication(app, QStringLiteral("meridian-files"), QObject::tr("Files"));

    QString start = QDir::homePath();
    const QStringList args = app.arguments().mid(1);
    if (!args.isEmpty()) {
        QString a = args.first();
        if (a.startsWith(QLatin1String("file://")))
            a = QUrl(a).toLocalFile();
        const QFileInfo fi(a);
        if (fi.isDir())
            start = fi.absoluteFilePath();
        else if (fi.exists())
            start = fi.absolutePath();
    }

    QQmlApplicationEngine engine;
    Meridian::setupEngine(engine);
    engine.setInitialProperties({{QStringLiteral("startPath"), start}});
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app,
                     [] { QCoreApplication::exit(1); }, Qt::QueuedConnection);
    engine.load(QUrl(QStringLiteral("qrc:/Meridian/Files/Main.qml")));
    if (engine.rootObjects().isEmpty())
        return 1;
    return app.exec();
}
