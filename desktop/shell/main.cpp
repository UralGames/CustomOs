// meridian-shell: the Meridian desktop shell. Runs as a client of the labwc
// compositor and draws the wallpaper, taskbar, start menu, quick settings,
// notification centre, task view, toasts and OSD as wlr-layer-shell surfaces.
//
//   meridian-shell                 start the shell (normally from labwc's autostart)
//   meridian-shell --action NAME   ask the running shell to do something
//                                  (used by labwc key bindings), e.g. launcher,
//                                  quick-settings, notifications, task-view,
//                                  show-desktop, lock, screenshot, volume-up ...

#include "meridian.h"
#include "shellipc.h"

#include <QCoreApplication>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QtQml/qqmlextensionplugin.h>

#include <cstdio>
#include <cstring>

MERIDIAN_IMPORT_QML_MODULES()

int main(int argc, char *argv[])
{
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--action") == 0 && i + 1 < argc) {
            QCoreApplication app(argc, argv);
            if (!ShellIpc::send(QString::fromLocal8Bit(argv[i + 1]))) {
                std::fprintf(stderr, "meridian-shell: no running shell to send \"%s\" to\n", argv[i + 1]);
                return 1;
            }
            return 0;
        }
        if (std::strcmp(argv[i], "--help") == 0) {
            std::printf("usage: meridian-shell [--action NAME]\n");
            return 0;
        }
    }

    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM"))
        qputenv("QT_QPA_PLATFORM", "wayland");
#ifdef MERIDIAN_HAVE_LAYERSHELLQT
    // Every window of this process is a layer-shell surface.
    qputenv("QT_WAYLAND_SHELL_INTEGRATION", "layer-shell");
#endif

    QGuiApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);
    Meridian::setupApplication(app, QStringLiteral("meridian-shell"), QStringLiteral("Meridian"));

    if (ShellIpc::send(QStringLiteral("ping"))) {
        std::fprintf(stderr, "meridian-shell: already running on this display\n");
        return 0;
    }
    ShellIpc::instance()->listen();

    QQmlApplicationEngine engine;
    Meridian::setupEngine(engine);
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app,
                     [] { QCoreApplication::exit(1); }, Qt::QueuedConnection);
    engine.load(QUrl(QStringLiteral("qrc:/Meridian/Shell/Main.qml")));
    if (engine.rootObjects().isEmpty())
        return 1;

    // Applications started from the shell are ordinary windows.
    qunsetenv("QT_WAYLAND_SHELL_INTEGRATION");
    return app.exec();
}
