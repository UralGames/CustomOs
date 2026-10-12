#include "settings.h"
#include "meridian.h"

#include <QDir>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QQmlEngine>
#include <QSettings>
#include <QStandardPaths>
#include <QTimer>

namespace {
Settings *s_instance = nullptr;
}

Settings *Settings::instance()
{
    if (!s_instance)
        s_instance = new Settings();
    return s_instance;
}

Settings *Settings::create(QQmlEngine *, QJSEngine *)
{
    Settings *s = instance();
    QQmlEngine::setObjectOwnership(s, QQmlEngine::CppOwnership);
    return s;
}

Settings::Settings(QObject *parent)
    : QQmlPropertyMap(this, parent)
{
    const QString dataDir = Meridian::dataDir();
    m_defaults = {
        // Appearance
        {"darkMode", false},
        {"accentColor", QStringLiteral("#1F8A7A")},
        {"wallpaper", dataDir + QStringLiteral("/wallpapers/ridge.jpg")},
        {"wallpaperFill", QStringLiteral("fill")},
        {"transparency", true},
        {"animations", true},
        {"roundedCorners", true},
        // Taskbar
        {"taskbarAlignment", QStringLiteral("center")},
        {"pinnedApps", QStringList{QStringLiteral("meridian-files.desktop"),
                                   QStringLiteral("meridian-terminal.desktop"),
                                   QStringLiteral("meridian-editor.desktop"),
                                   QStringLiteral("meridian-settings.desktop")}},
        {"launcherPinned", QStringList{QStringLiteral("meridian-files.desktop"),
                                       QStringLiteral("meridian-terminal.desktop"),
                                       QStringLiteral("meridian-editor.desktop"),
                                       QStringLiteral("meridian-monitor.desktop"),
                                       QStringLiteral("meridian-settings.desktop"),
                                       QStringLiteral("meridian-installer.desktop")}},
        // Clock
        {"clock24h", true},
        {"showSeconds", false},
        // Notifications
        {"doNotDisturb", false},
        // Input
        {"keyboardLayout", QStringLiteral("us")},
        {"keyboardVariant", QString()},
        {"repeatDelay", 400},
        {"repeatRate", 30},
        {"naturalScroll", false},
        {"leftHanded", false},
        {"pointerSpeed", 0.0},          // libinput accel speed, -1.0 .. 1.0
        {"tapToClick", true},
        // Window management
        {"desktopCount", 4},
        // Display ("OUTPUT WxH@Hz"; "" = the monitor's preferred mode)
        {"displayMode", QString()},
        {"displayScale", 1.0},
        // User
        {"userDisplayName", QString()},
    };

    const QString cfgDir = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)
                           + QStringLiteral("/meridian");
    QDir().mkpath(cfgDir);
    m_path = cfgDir + QStringLiteral("/desktop.conf");

    for (auto it = m_defaults.cbegin(); it != m_defaults.cend(); ++it)
        insert(it.key(), it.value());
    reload();

    m_watcher = new QFileSystemWatcher(this);
    m_watcher->addPath(cfgDir);
    connect(m_watcher, &QFileSystemWatcher::fileChanged, this, [this] {
        if (!m_saving) reload();
        watch();
    });
    connect(m_watcher, &QFileSystemWatcher::directoryChanged, this, [this] {
        if (!m_saving) reload();
        watch();
    });
    watch();
}

void Settings::watch()
{
    if (QFileInfo::exists(m_path) && !m_watcher->files().contains(m_path))
        m_watcher->addPath(m_path);
}

QVariant Settings::coerce(const QString &key, const QVariant &raw) const
{
    const QVariant def = m_defaults.value(key);
    if (!def.isValid())
        return raw;
    QVariant v = raw;
    if (def.typeId() == QMetaType::QStringList) {
        // QSettings returns a QString for single-element lists.
        if (v.typeId() == QMetaType::QString)
            return QStringList{v.toString()};
        return v.toStringList();
    }
    if (def.typeId() == QMetaType::Bool) {
        if (v.typeId() == QMetaType::QString)
            return v.toString() == QLatin1String("true") || v.toString() == QLatin1String("1");
        return v.toBool();
    }
    if (v.convert(def.metaType()))
        return v;
    return def;
}

void Settings::reload()
{
    QSettings ini(m_path, QSettings::IniFormat);
    for (auto it = m_defaults.cbegin(); it != m_defaults.cend(); ++it) {
        const QVariant v = ini.contains(it.key()) ? coerce(it.key(), ini.value(it.key())) : it.value();
        if (value(it.key()) != v) {
            insert(it.key(), v);
            emit settingChanged(it.key());
        }
    }
}

void Settings::save()
{
    m_saving = true;
    {
        QSettings ini(m_path, QSettings::IniFormat);
        for (const QString &k : keys()) {
            if (value(k) == m_defaults.value(k))
                ini.remove(k);
            else
                ini.setValue(k, value(k));
        }
        ini.sync();
    }
    // Let the watcher's notification for our own write pass before
    // reacting to external edits again.
    QTimer::singleShot(250, this, [this] { m_saving = false; watch(); });
}

QVariant Settings::updateValue(const QString &key, const QVariant &input)
{
    const QVariant v = coerce(key, input);
    // QQmlPropertyMap stores the returned value after this function returns,
    // so persist on the next turn of the event loop.
    QTimer::singleShot(0, this, [this, key] {
        save();
        emit settingChanged(key);
    });
    return v;
}

void Settings::set(const QString &key, const QVariant &v)
{
    const QVariant c = coerce(key, v);
    if (value(key) == c)
        return;
    insert(key, c);
    save();
    emit settingChanged(key);
}

void Settings::resetToDefaults()
{
    for (auto it = m_defaults.cbegin(); it != m_defaults.cend(); ++it)
        insert(it.key(), it.value());
    save();
    for (auto it = m_defaults.cbegin(); it != m_defaults.cend(); ++it)
        emit settingChanged(it.key());
}
