#include "layershell.h"

#include <QQmlEngine>
#include <QRectF>
#include <QRegion>

#ifdef MERIDIAN_HAVE_LAYERSHELLQT
#include <LayerShellQt/Window>
#endif

LayerShell *LayerShell::create(QQmlEngine *, QJSEngine *)
{
    static LayerShell *l = new LayerShell();
    QQmlEngine::setObjectOwnership(l, QQmlEngine::CppOwnership);
    return l;
}

bool LayerShell::available() const
{
#ifdef MERIDIAN_HAVE_LAYERSHELLQT
    return true;
#else
    return false;
#endif
}

void LayerShell::setup(QQuickWindow *window, const QVariantMap &o)
{
    if (!window)
        return;
#ifdef MERIDIAN_HAVE_LAYERSHELLQT
    auto *lw = LayerShellQt::Window::get(window);
    lw->setLayer(LayerShellQt::Window::Layer(o.value(QStringLiteral("layer"), Top).toInt()));

    const int a = o.value(QStringLiteral("anchors"), 0).toInt();
    LayerShellQt::Window::Anchors anchors;
    if (a & AnchorTop) anchors |= LayerShellQt::Window::AnchorTop;
    if (a & AnchorBottom) anchors |= LayerShellQt::Window::AnchorBottom;
    if (a & AnchorLeft) anchors |= LayerShellQt::Window::AnchorLeft;
    if (a & AnchorRight) anchors |= LayerShellQt::Window::AnchorRight;
    lw->setAnchors(anchors);

    lw->setExclusiveZone(o.value(QStringLiteral("exclusiveZone"), 0).toInt());
    lw->setKeyboardInteractivity(LayerShellQt::Window::KeyboardInteractivity(
        o.value(QStringLiteral("keyboard"), KeyboardNone).toInt()));
    lw->setScope(o.value(QStringLiteral("scope"), QStringLiteral("meridian")).toString());
    lw->setMargins(QMargins(o.value(QStringLiteral("marginLeft"), 0).toInt(),
                            o.value(QStringLiteral("marginTop"), 0).toInt(),
                            o.value(QStringLiteral("marginRight"), 0).toInt(),
                            o.value(QStringLiteral("marginBottom"), 0).toInt()));
#else
    Q_UNUSED(o)
    // Development fallback without layer-shell: ordinary frameless windows.
    window->setFlag(Qt::FramelessWindowHint);
#endif
}

void LayerShell::setMargins(QQuickWindow *window, int left, int top, int right, int bottom)
{
#ifdef MERIDIAN_HAVE_LAYERSHELLQT
    if (window)
        LayerShellQt::Window::get(window)->setMargins(QMargins(left, top, right, bottom));
#else
    Q_UNUSED(window) Q_UNUSED(left) Q_UNUSED(top) Q_UNUSED(right) Q_UNUSED(bottom)
#endif
}

void LayerShell::setKeyboard(QQuickWindow *window, int keyboard)
{
#ifdef MERIDIAN_HAVE_LAYERSHELLQT
    if (window)
        LayerShellQt::Window::get(window)->setKeyboardInteractivity(
            LayerShellQt::Window::KeyboardInteractivity(keyboard));
#else
    Q_UNUSED(window) Q_UNUSED(keyboard)
#endif
}

void LayerShell::setInputRegion(QQuickWindow *window, const QVariantList &rects)
{
    if (!window)
        return;
    QRegion r;
    for (const QVariant &v : rects)
        r += v.toRectF().toAlignedRect();
    // An empty QRegion would mean "everything accepts input", so use a 1 px
    // corner instead when nothing should.
    if (r.isEmpty())
        r = QRegion(0, 0, 1, 1);
    window->setMask(r);
}

void LayerShell::clearInputRegion(QQuickWindow *window)
{
    if (window)
        window->setMask(QRegion());
}
