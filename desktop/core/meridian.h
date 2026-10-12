#pragma once

#include <QString>

class QGuiApplication;
class QQmlEngine;

namespace Meridian {

// Directory with wallpapers/, images/ (installed: /usr/share/meridian).
QString dataDir();

// Common start-up for every Meridian program: application identity (which
// becomes the Wayland app_id), the Meridian Qt Quick Controls style, the UI
// font, the icon image provider and QML import paths.
void setupApplication(QGuiApplication &app, const QString &appId, const QString &displayName);
void setupEngine(QQmlEngine &engine);

// True when running from the live ISO (the image ships /etc/customos-live).
bool isLiveSystem();

} // namespace Meridian

// Each executable includes this once (in main.cpp) to pull in the statically
// linked QML modules.
#define MERIDIAN_IMPORT_QML_MODULES()            \
    Q_IMPORT_QML_PLUGIN(Meridian_CorePlugin)     \
    Q_IMPORT_QML_PLUGIN(MeridianStylePlugin)
