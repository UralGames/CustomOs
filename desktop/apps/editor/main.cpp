// meridian-editor: plain-text editor.
//   meridian-editor [FILE]

#include "meridian.h"

#include <QFileInfo>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QUrl>
#include <QtQml/qqmlextensionplugin.h>

MERIDIAN_IMPORT_QML_MODULES()

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    Meridian::setupApplication(app, QStringLiteral("meridian-editor"), QObject::tr("Text Editor"));

    QString file;
    const QStringList args = app.arguments().mid(1);
    if (!args.isEmpty()) {
        file = args.first();
        if (file.startsWith(QLatin1String("file://")))
            file = QUrl(file).toLocalFile();
        file = QFileInfo(file).absoluteFilePath();
    }

    QQmlApplicationEngine engine;
    Meridian::setupEngine(engine);
    engine.setInitialProperties({{QStringLiteral("startFile"), file}});
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app,
                     [] { QCoreApplication::exit(1); }, Qt::QueuedConnection);
    engine.load(QUrl(QStringLiteral("qrc:/Meridian/Editor/Main.qml")));
    if (engine.rootObjects().isEmpty())
        return 1;
    return app.exec();
}
