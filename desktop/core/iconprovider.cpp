#include "iconprovider.h"

#include <QColor>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QPainter>
#include <QStandardPaths>
#include <QSvgRenderer>
#include <QUrlQuery>

IconProvider::IconProvider()
    : QQuickImageProvider(QQuickImageProvider::Image)
{
}

QImage IconProvider::renderSvg(const QByteArray &svg, const QSize &size)
{
    QSvgRenderer r(svg);
    if (!r.isValid())
        return {};
    QImage img(size, QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::transparent);
    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing);
    r.render(&p, QRectF(QPointF(0, 0), size));
    return img;
}

static QStringList iconSearchRoots()
{
    QStringList roots;
    const QString home = QDir::homePath();
    roots << home + QStringLiteral("/.local/share/icons") << home + QStringLiteral("/.icons");
    for (const QString &d : QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation))
        roots << d + QStringLiteral("/icons");
    roots << QStringLiteral("/usr/share/icons");
    roots.removeDuplicates();
    return roots;
}

QString IconProvider::findThemeIcon(const QString &name)
{
    if (name.isEmpty())
        return {};
    if (name.startsWith(QLatin1Char('/')))
        return QFileInfo::exists(name) ? name : QString();

    static QMutex mutex;
    static QHash<QString, QString> cache;
    QMutexLocker lock(&mutex);
    if (auto it = cache.constFind(name); it != cache.cend())
        return *it;

    static const QStringList themes{QStringLiteral("hicolor"), QStringLiteral("Adwaita"),
                                    QStringLiteral("AdwaitaLegacy")};
    static const QStringList sizes{QStringLiteral("scalable"), QStringLiteral("256x256"),
                                   QStringLiteral("512x512"), QStringLiteral("128x128"),
                                   QStringLiteral("96x96"), QStringLiteral("64x64"),
                                   QStringLiteral("48x48"), QStringLiteral("32x32"),
                                   QStringLiteral("24x24"), QStringLiteral("16x16")};
    static const QStringList contexts{QStringLiteral("apps"), QStringLiteral("mimetypes"),
                                      QStringLiteral("places"), QStringLiteral("devices"),
                                      QStringLiteral("categories"), QStringLiteral("actions"),
                                      QStringLiteral("status"), QStringLiteral("legacy")};
    static const QStringList exts{QStringLiteral(".svg"), QStringLiteral(".png")};

    QString found;
    const QStringList roots = iconSearchRoots();
    for (const QString &root : roots) {
        for (const QString &theme : themes) {
            const QString base = root + QLatin1Char('/') + theme + QLatin1Char('/');
            if (!QFileInfo::exists(base))
                continue;
            for (const QString &sz : sizes) {
                for (const QString &ctx : contexts) {
                    for (const QString &ext : exts) {
                        const QString p = base + sz + QLatin1Char('/') + ctx + QLatin1Char('/') + name + ext;
                        if (QFileInfo::exists(p)) { found = p; goto done; }
                    }
                }
            }
        }
    }
    for (const QString &ext : {QStringLiteral(".svg"), QStringLiteral(".png"), QStringLiteral(".xpm")}) {
        const QString p = QStringLiteral("/usr/share/pixmaps/") + name + ext;
        if (QFileInfo::exists(p)) { found = p; break; }
    }
done:
    cache.insert(name, found);
    return found;
}

QImage IconProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    const int qpos = id.indexOf(QLatin1Char('?'));
    const QString path = qpos >= 0 ? id.left(qpos) : id;
    const QUrlQuery query(qpos >= 0 ? id.mid(qpos + 1) : QString());
    const QSize target = requestedSize.isValid() && !requestedSize.isEmpty()
                             ? requestedSize : QSize(48, 48);
    QImage img;

    if (path.startsWith(QLatin1String("sym/"))) {
        const QString name = path.mid(4);
        QFile f(QStringLiteral(":/icons/symbolic/") + name + QStringLiteral(".svg"));
        if (!f.open(QIODevice::ReadOnly))
            f.setFileName(QStringLiteral(":/icons/symbolic/apps.svg")), f.open(QIODevice::ReadOnly);
        QByteArray svg = f.readAll();
        QString c = query.queryItemValue(QStringLiteral("c"));
        QColor color(c.isEmpty() ? QStringLiteral("#000000") : QLatin1Char('#') + c);
        if (!color.isValid())
            color = Qt::black;
        // SVG has no alpha-in-hex, so carry alpha separately as opacity.
        svg.replace("stroke=\"currentColor\"",
                    QStringLiteral("stroke=\"%1\" stroke-opacity=\"%2\"")
                        .arg(color.name(QColor::HexRgb)).arg(color.alphaF()).toUtf8());
        svg.replace("currentColor", color.name(QColor::HexRgb).toUtf8());
        img = renderSvg(svg, target);
    } else if (path.startsWith(QLatin1String("app/"))) {
        QString name = path.mid(4);
        if (name.endsWith(QLatin1String(".desktop")))
            name.chop(8);
        QString file;
        if (QFile::exists(QStringLiteral(":/icons/apps/") + name + QStringLiteral(".svg")))
            file = QStringLiteral(":/icons/apps/") + name + QStringLiteral(".svg");
        else
            file = findThemeIcon(name);
        if (file.isEmpty())
            file = QStringLiteral(":/icons/apps/meridian-app.svg");
        if (file.endsWith(QLatin1String(".svg"))) {
            QFile f(file);
            if (f.open(QIODevice::ReadOnly))
                img = renderSvg(f.readAll(), target);
        } else {
            QImageReader reader(file);
            img = reader.read().scaled(target, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }
    }

    if (img.isNull()) {
        img = QImage(target, QImage::Format_ARGB32_Premultiplied);
        img.fill(Qt::transparent);
    }
    if (size)
        *size = img.size();
    return img;
}
