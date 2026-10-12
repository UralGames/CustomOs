#pragma once

#include <QObject>
#include <QQuickWindow>
#include <QtQml/qqmlregistration.h>

class QQmlEngine;
class QJSEngine;

// Turns QML Windows into wlr-layer-shell surfaces (wallpaper, taskbar,
// panels, toasts) through LayerShellQt. Must be called before the window is
// first shown.
class LayerShell : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(LayerShell)
    QML_SINGLETON
    Q_PROPERTY(bool available READ available CONSTANT)

public:
    enum Layer { Background = 0, Bottom = 1, Top = 2, Overlay = 3 };
    Q_ENUM(Layer)
    enum Anchor { AnchorTop = 1, AnchorBottom = 2, AnchorLeft = 4, AnchorRight = 8 };
    Q_ENUM(Anchor)
    enum Keyboard { KeyboardNone = 0, KeyboardExclusive = 1, KeyboardOnDemand = 2 };
    Q_ENUM(Keyboard)

    static LayerShell *create(QQmlEngine *, QJSEngine *);
    bool available() const;

    // options: { layer, anchors, exclusiveZone, keyboard, scope,
    //            marginLeft, marginTop, marginRight, marginBottom }
    Q_INVOKABLE void setup(QQuickWindow *window, const QVariantMap &options);
    Q_INVOKABLE void setMargins(QQuickWindow *window, int left, int top, int right, int bottom);
    Q_INVOKABLE void setKeyboard(QQuickWindow *window, int keyboard);
    // Input region: only the given rectangles receive pointer input; the rest
    // of the surface lets clicks through to the windows below. An empty list
    // makes the whole surface click-through.
    Q_INVOKABLE void setInputRegion(QQuickWindow *window, const QVariantList &rects);
    Q_INVOKABLE void clearInputRegion(QQuickWindow *window);

private:
    explicit LayerShell(QObject *parent = nullptr) : QObject(parent) {}
};
