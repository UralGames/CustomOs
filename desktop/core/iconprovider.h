#pragma once

#include <QHash>
#include <QMutex>
#include <QQuickImageProvider>

// Serves icons to QML through image://icon/...
//
//   image://icon/sym/<name>?c=<rrggbb|aarrggbb>   Meridian symbolic icon, recoloured
//   image://icon/app/<icon-name-or-path>          application icon: Meridian's own
//                                                 set first, then the XDG icon themes
//                                                 (hicolor, Adwaita), then /usr/share/pixmaps
class IconProvider : public QQuickImageProvider
{
public:
    IconProvider();
    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;

    // Resolves an XDG icon name to a file path ("" if not found).
    static QString findThemeIcon(const QString &name);

private:
    static QImage renderSvg(const QByteArray &svg, const QSize &size);
};
