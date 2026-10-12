#include "appmodel.h"

#include <QCollator>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QMimeDatabase>
#include <QProcess>
#include <QQmlEngine>
#include <QSaveFile>
#include <QSet>
#include <QStandardPaths>
#include <QTextStream>
#include <QTimer>

namespace {
AppModel *s_instance = nullptr;

QString terminalProgram()
{
    const QString env = qEnvironmentVariable("MERIDIAN_TERMINAL");
    if (!env.isEmpty())
        return env;
    return QStringLiteral("foot");
}
} // namespace

AppModel *AppModel::instance()
{
    if (!s_instance)
        s_instance = new AppModel();
    return s_instance;
}

AppModel *AppModel::create(QQmlEngine *, QJSEngine *)
{
    AppModel *m = instance();
    QQmlEngine::setObjectOwnership(m, QQmlEngine::CppOwnership);
    return m;
}

AppModel::AppModel(QObject *parent)
    : QAbstractListModel(parent)
{
    m_reloadTimer = new QTimer(this);
    m_reloadTimer->setSingleShot(true);
    m_reloadTimer->setInterval(600);
    connect(m_reloadTimer, &QTimer::timeout, this, &AppModel::reload);

    m_watcher = new QFileSystemWatcher(this);
    connect(m_watcher, &QFileSystemWatcher::directoryChanged, m_reloadTimer, qOverload<>(&QTimer::start));
    reload();
}

QStringList AppModel::applicationDirs()
{
    QStringList dirs;
    QString dataHome = qEnvironmentVariable("XDG_DATA_HOME");
    if (dataHome.isEmpty())
        dataHome = QDir::homePath() + QStringLiteral("/.local/share");
    dirs << dataHome + QStringLiteral("/applications");
    QString dataDirs = qEnvironmentVariable("XDG_DATA_DIRS");
    if (dataDirs.isEmpty())
        dataDirs = QStringLiteral("/usr/local/share:/usr/share");
    for (const QString &d : dataDirs.split(QLatin1Char(':'), Qt::SkipEmptyParts))
        dirs << d + QStringLiteral("/applications");
    dirs.removeDuplicates();
    return dirs;
}

void AppModel::reload()
{
    QHash<QString, DesktopEntry> byId;
    const QStringList dirs = applicationDirs();
    for (const QString &dir : dirs) {
        QDirIterator it(dir, {QStringLiteral("*.desktop")}, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            const QString path = it.next();
            QString id = path.mid(dir.size() + 1);
            id.replace(QLatin1Char('/'), QLatin1Char('-'));
            if (byId.contains(id))
                continue;   // earlier directories take precedence
            DesktopEntry e = DesktopEntry::parse(path, id);
            byId.insert(id, e);
        }
    }

    QList<DesktopEntry> visible;
    for (const DesktopEntry &e : std::as_const(byId)) {
        if (e.isValid() && e.shouldShow())
            visible << e;
    }
    QCollator coll;
    coll.setCaseSensitivity(Qt::CaseInsensitive);
    std::sort(visible.begin(), visible.end(), [&coll](const DesktopEntry &a, const DesktopEntry &b) {
        return coll.compare(a.name, b.name) < 0;
    });

    beginResetModel();
    m_byId = byId;
    m_entries = visible;
    endResetModel();
    emit countChanged();

    if (!m_watcher->directories().isEmpty())
        m_watcher->removePaths(m_watcher->directories());
    for (const QString &d : dirs) {
        if (QFileInfo::exists(d))
            m_watcher->addPath(d);
    }
}

int AppModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_entries.size();
}

QVariant AppModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_entries.size())
        return {};
    const DesktopEntry &e = m_entries.at(index.row());
    switch (role) {
    case IdRole: return e.id;
    case Qt::DisplayRole:
    case NameRole: return e.name;
    case GenericNameRole: return e.genericName;
    case CommentRole: return e.comment;
    case IconRole: return e.icon.isEmpty() ? QStringLiteral("meridian-app") : e.icon;
    case CategoriesRole: return e.categories;
    case TerminalRole: return e.terminal;
    }
    return {};
}

QHash<int, QByteArray> AppModel::roleNames() const
{
    return {{IdRole, "appId"}, {NameRole, "name"}, {GenericNameRole, "genericName"},
            {CommentRole, "comment"}, {IconRole, "iconName"}, {CategoriesRole, "categories"},
            {TerminalRole, "terminal"}};
}

QVariantMap AppModel::toMap(const DesktopEntry &e) const
{
    return {{QStringLiteral("appId"), e.id},
            {QStringLiteral("name"), e.name},
            {QStringLiteral("genericName"), e.genericName},
            {QStringLiteral("comment"), e.comment},
            {QStringLiteral("iconName"), e.icon.isEmpty() ? QStringLiteral("meridian-app") : e.icon},
            {QStringLiteral("terminal"), e.terminal}};
}

QVariantMap AppModel::get(const QString &id) const
{
    auto it = m_byId.constFind(id);
    if (it == m_byId.cend())
        return {};
    return toMap(*it);
}

QVariantList AppModel::all() const
{
    QVariantList out;
    for (const DesktopEntry &e : m_entries)
        out << toMap(e);
    return out;
}

bool AppModel::startEntry(const DesktopEntry &e, const QStringList &files)
{
    QStringList argv = e.expandExec(files);
    if (argv.isEmpty()) {
        emit launchFailed(e.name, tr("The application has no command to run."));
        return false;
    }
    if (e.terminal)
        argv.prepend(terminalProgram());
    const QString program = argv.takeFirst();
    QString wd = e.path_;
    if (wd.isEmpty() || !QFileInfo(wd).isDir())
        wd = QDir::homePath();
    qint64 pid = 0;
    const bool ok = QProcess::startDetached(program, argv, wd, &pid);
    if (!ok)
        emit launchFailed(e.name, tr("Could not start \"%1\".").arg(program));
    else
        emit launched(e.id);
    return ok;
}

bool AppModel::launch(const QString &id, const QStringList &files)
{
    auto it = m_byId.constFind(id);
    if (it == m_byId.cend()) {
        emit launchFailed(id, tr("Application not found."));
        return false;
    }
    return startEntry(*it, files);
}

bool AppModel::runCommand(const QString &program, const QStringList &args, const QString &workingDir)
{
    const QString wd = workingDir.isEmpty() ? QDir::homePath() : workingDir;
    const bool ok = QProcess::startDetached(program, args, wd);
    if (!ok)
        emit launchFailed(program, tr("Could not start \"%1\".").arg(program));
    return ok;
}

bool AppModel::openTerminal(const QString &workingDir)
{
    const QString wd = workingDir.isEmpty() ? QDir::homePath() : workingDir;
    if (m_byId.contains(QStringLiteral("meridian-terminal.desktop")) && workingDir.isEmpty())
        return launch(QStringLiteral("meridian-terminal.desktop"));
    return runCommand(terminalProgram(), {QStringLiteral("--working-directory=") + wd}, wd);
}

// ---- MIME -------------------------------------------------------------------

QString AppModel::mimeTypeFor(const QString &path) const
{
    static QMimeDatabase db;
    return db.mimeTypeForFile(path).name();
}

QString AppModel::mimeDescription(const QString &path) const
{
    static QMimeDatabase db;
    return db.mimeTypeForFile(path).comment();
}

static QStringList mimeChain(const QString &path)
{
    static QMimeDatabase db;
    const QMimeType mt = db.mimeTypeForFile(path);
    QStringList chain{mt.name()};
    chain << mt.aliases();
    chain << mt.allAncestors();
    chain.removeDuplicates();
    return chain;
}

static QStringList mimeappsFiles()
{
    QString cfgHome = qEnvironmentVariable("XDG_CONFIG_HOME");
    if (cfgHome.isEmpty())
        cfgHome = QDir::homePath() + QStringLiteral("/.config");
    QStringList files{cfgHome + QStringLiteral("/mimeapps.list"),
                      QStringLiteral("/etc/xdg/mimeapps.list")};
    for (const QString &d : AppModel::applicationDirs())
        files << d + QStringLiteral("/mimeapps.list");
    return files;
}

QStringList AppModel::defaultsFor(const QString &mime) const
{
    QStringList result;
    for (const QString &file : mimeappsFiles()) {
        QFile f(file);
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
            continue;
        bool inDefaults = false;
        QTextStream ts(&f);
        while (!ts.atEnd()) {
            const QString line = ts.readLine().trimmed();
            if (line.startsWith(QLatin1Char('['))) {
                inDefaults = (line == QLatin1String("[Default Applications]"));
                continue;
            }
            if (!inDefaults || !line.startsWith(mime + QLatin1Char('=')))
                continue;
            for (const QString &id : line.mid(mime.size() + 1).split(QLatin1Char(';'), Qt::SkipEmptyParts)) {
                if (m_byId.contains(id) && !result.contains(id))
                    result << id;
            }
        }
    }
    return result;
}

QVariantList AppModel::appsForFile(const QString &path) const
{
    const QStringList chain = mimeChain(path);
    QStringList ids;
    for (const QString &m : chain)
        for (const QString &id : defaultsFor(m))
            if (!ids.contains(id)) ids << id;
    for (const QString &m : chain) {
        for (const DesktopEntry &e : m_entries) {
            if (e.mimeTypes.contains(m) && !ids.contains(e.id))
                ids << e.id;
        }
    }
    QVariantList out;
    for (const QString &id : ids)
        out << toMap(m_byId.value(id));
    return out;
}

QString AppModel::defaultAppForFile(const QString &path) const
{
    const QVariantList apps = appsForFile(path);
    return apps.isEmpty() ? QString() : apps.first().toMap().value(QStringLiteral("appId")).toString();
}

void AppModel::setDefaultApp(const QString &mimeType, const QString &id)
{
    const QString file = mimeappsFiles().first();
    QDir().mkpath(QFileInfo(file).absolutePath());
    QStringList lines;
    {
        QFile f(file);
        if (f.open(QIODevice::ReadOnly | QIODevice::Text))
            lines = QString::fromUtf8(f.readAll()).split(QLatin1Char('\n'));
    }
    int section = lines.indexOf(QStringLiteral("[Default Applications]"));
    if (section < 0) {
        if (!lines.isEmpty() && !lines.last().isEmpty()) lines << QString();
        lines << QStringLiteral("[Default Applications]");
        section = lines.size() - 1;
    }
    int end = section + 1;
    bool replaced = false;
    for (; end < lines.size() && !lines.at(end).startsWith(QLatin1Char('[')); ++end) {
        if (lines.at(end).startsWith(mimeType + QLatin1Char('='))) {
            lines[end] = mimeType + QLatin1Char('=') + id + QLatin1Char(';');
            replaced = true;
        }
    }
    if (!replaced)
        lines.insert(section + 1, mimeType + QLatin1Char('=') + id + QLatin1Char(';'));
    QSaveFile out(file);
    if (out.open(QIODevice::WriteOnly | QIODevice::Text)) {
        out.write(lines.join(QLatin1Char('\n')).toUtf8());
        if (!lines.last().isEmpty()) out.write("\n");
        out.commit();
    }
}

bool AppModel::openFile(const QString &path)
{
    const QFileInfo fi(path);
    if (fi.isDir()) {
        if (m_byId.contains(QStringLiteral("meridian-files.desktop")))
            return launch(QStringLiteral("meridian-files.desktop"), {fi.absoluteFilePath()});
    }
    const QString id = defaultAppForFile(path);
    if (id.isEmpty()) {
        emit launchFailed(fi.fileName(), tr("No installed application can open this kind of file (%1).")
                                              .arg(mimeTypeFor(path)));
        return false;
    }
    return launch(id, {fi.absoluteFilePath()});
}

bool AppModel::openFileWith(const QString &id, const QString &path)
{
    return launch(id, {QFileInfo(path).absoluteFilePath()});
}

// ---- lookup -----------------------------------------------------------------

QString AppModel::idForAppId(const QString &appId) const
{
    if (appId.isEmpty())
        return {};
    const QString direct = appId + QStringLiteral(".desktop");
    if (m_byId.contains(direct))
        return direct;
    const QString lower = appId.toLower();
    if (m_byId.contains(lower + QStringLiteral(".desktop")))
        return lower + QStringLiteral(".desktop");
    for (const DesktopEntry &e : m_byId) {
        if (e.startupWMClass.compare(appId, Qt::CaseInsensitive) == 0)
            return e.id;
    }
    for (const DesktopEntry &e : m_byId) {
        const QString base = e.id.chopped(8);    // strip ".desktop"
        if (base.section(QLatin1Char('.'), -1).compare(appId, Qt::CaseInsensitive) == 0)
            return e.id;
    }
    for (const DesktopEntry &e : m_byId) {
        const QStringList argv = DesktopEntry::splitExec(e.exec);
        if (!argv.isEmpty() && QFileInfo(argv.first()).fileName().compare(appId, Qt::CaseInsensitive) == 0
            && !e.noDisplay)
            return e.id;
    }
    return {};
}

QVariantList AppModel::search(const QString &text, int limit) const
{
    const QString q = text.trimmed();
    if (q.isEmpty())
        return {};
    struct Hit { int score; const DesktopEntry *e; };
    QList<Hit> hits;
    for (const DesktopEntry &e : m_entries) {
        int score = 0;
        if (e.name.startsWith(q, Qt::CaseInsensitive))
            score = 100;
        else {
            const QStringList words = e.name.split(QLatin1Char(' '), Qt::SkipEmptyParts);
            for (const QString &w : words)
                if (w.startsWith(q, Qt::CaseInsensitive)) { score = 80; break; }
            if (!score && e.name.contains(q, Qt::CaseInsensitive)) score = 60;
        }
        if (!score && e.genericName.contains(q, Qt::CaseInsensitive)) score = 40;
        if (!score) {
            for (const QString &k : e.keywords)
                if (k.startsWith(q, Qt::CaseInsensitive)) { score = 35; break; }
        }
        if (!score && e.comment.contains(q, Qt::CaseInsensitive)) score = 20;
        if (!score) {
            const QStringList argv = DesktopEntry::splitExec(e.exec);
            if (!argv.isEmpty() && QFileInfo(argv.first()).fileName().startsWith(q, Qt::CaseInsensitive))
                score = 15;
        }
        if (score)
            hits << Hit{score, &e};
    }
    std::stable_sort(hits.begin(), hits.end(), [](const Hit &a, const Hit &b) { return a.score > b.score; });
    QVariantList out;
    for (int i = 0; i < hits.size() && i < limit; ++i)
        out << toMap(*hits.at(i).e);
    return out;
}

void AppModel::runAutostart()
{
    QString cfgHome = qEnvironmentVariable("XDG_CONFIG_HOME");
    if (cfgHome.isEmpty())
        cfgHome = QDir::homePath() + QStringLiteral("/.config");
    QStringList dirs{cfgHome + QStringLiteral("/autostart")};
    QString cfgDirs = qEnvironmentVariable("XDG_CONFIG_DIRS");
    if (cfgDirs.isEmpty())
        cfgDirs = QStringLiteral("/etc/xdg");
    for (const QString &d : cfgDirs.split(QLatin1Char(':'), Qt::SkipEmptyParts))
        dirs << d + QStringLiteral("/autostart");

    QSet<QString> seen;
    for (const QString &dir : dirs) {
        const QStringList files = QDir(dir).entryList({QStringLiteral("*.desktop")}, QDir::Files);
        for (const QString &file : files) {
            if (seen.contains(file))
                continue;
            seen.insert(file);
            DesktopEntry e = DesktopEntry::parse(dir + QLatin1Char('/') + file, file);
            // NoDisplay does not apply to autostart entries.
            e.noDisplay = false;
            if (!e.isValid() || !e.shouldShow())
                continue;
            startEntry(e, {});
        }
    }
}
