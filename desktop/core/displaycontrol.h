#pragma once

#include <QObject>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

class QQmlEngine;
class QJSEngine;

// Output modes and scale through the wlr-output-management protocol, using
// the `wlr-randr` tool. Changes apply immediately; the chosen mode/scale is
// stored in Settings (displayMode / displayScale) and re-applied by the
// shell at every login.
class DisplayControl : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Display)
    QML_SINGLETON
    Q_PROPERTY(bool available READ available NOTIFY changed)
    Q_PROPERTY(QVariantList outputs READ outputs NOTIFY changed)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)

public:
    static DisplayControl *instance();
    static DisplayControl *create(QQmlEngine *, QJSEngine *);

    bool available() const { return m_available; }
    QVariantList outputs() const { return m_outputs; }
    bool busy() const { return m_busy; }

    Q_INVOKABLE void refresh();
    // mode: "1920x1080@60.000000" (refresh optional) or "" for preferred
    Q_INVOKABLE void setMode(const QString &output, const QString &mode, bool remember = true);
    Q_INVOKABLE void setScale(const QString &output, double scale, bool remember = true);
    // Applies the remembered mode/scale (called by the shell at start-up).
    Q_INVOKABLE void applySaved();

    static QVariantList parse(const QString &text);

signals:
    void changed();
    void busyChanged();
    void applied(bool ok, const QString &message);

private:
    explicit DisplayControl(QObject *parent = nullptr);
    void run(const QStringList &args, const QString &okMessage);

    bool m_available = false;
    bool m_busy = false;
    QVariantList m_outputs;
};
