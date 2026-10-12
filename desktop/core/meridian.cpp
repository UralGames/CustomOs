#include "meridian.h"
#include "iconprovider.h"

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QFont>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QQmlEngine>
#include <QQuickStyle>

namespace Meridian {

QString dataDir()
{
    static QString dir = [] {
        const QString env = qEnvironmentVariable("MERIDIAN_DATA_DIR");
        if (!env.isEmpty() && QFileInfo::exists(env))
            return env;
        const QString installed = QStringLiteral(MERIDIAN_INSTALL_DATADIR);
        if (QFileInfo::exists(installed + QStringLiteral("/wallpapers")))
            return installed;
        return QStringLiteral(MERIDIAN_SOURCE_ASSETDIR);
    }();
    return dir;
}

bool isLiveSystem()
{
    return QFileInfo::exists(QStringLiteral("/etc/customos-live"));
}

static void ensureFonts()
{
    // On the OS the fonts are installed system-wide; during development we
    // register the vendored copies if fontconfig does not know them.
    if (QFontDatabase::families().contains(QStringLiteral("Inter")))
        return;
    QStringList dirs{dataDir() + QStringLiteral("/fonts"),
                     QStringLiteral(MERIDIAN_SOURCE_ASSETDIR) + QStringLiteral("/fonts")};
    for (const QString &d : dirs) {
        QDirIterator it(d, {QStringLiteral("*.otf"), QStringLiteral("*.ttf")}, QDir::Files);
        while (it.hasNext())
            QFontDatabase::addApplicationFont(it.next());
    }
}

void setupApplication(QGuiApplication &app, const QString &appId, const QString &displayName)
{
    QCoreApplication::setOrganizationName(QStringLiteral("Meridian"));
    QCoreApplication::setApplicationName(appId);
    QCoreApplication::setApplicationVersion(QStringLiteral(MERIDIAN_VERSION));
    QGuiApplication::setApplicationDisplayName(displayName);
    // Becomes the Wayland xdg_toplevel app_id, which the taskbar matches
    // against <appId>.desktop.
    QGuiApplication::setDesktopFileName(appId);

    ensureFonts();
    QFont f(QStringLiteral("Inter"));
    f.setPixelSize(14);
    f.setHintingPreference(QFont::PreferVerticalHinting);
    app.setFont(f);

    QQuickStyle::setStyle(QStringLiteral("MeridianStyle"));
}

void setupEngine(QQmlEngine &engine)
{
    // Static QML modules are compiled into the binary under qrc:/<URI path>.
    engine.addImportPath(QStringLiteral("qrc:/"));
    engine.addImageProvider(QStringLiteral("icon"), new IconProvider());
}

} // namespace Meridian
