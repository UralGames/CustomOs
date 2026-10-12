#include "desktopentry.h"

#include <QFile>
#include <QHash>
#include <QLocale>
#include <QStandardPaths>
#include <QTextStream>
#include <QUrl>

static QString unescapeValue(const QString &v)
{
    QString out;
    out.reserve(v.size());
    for (int i = 0; i < v.size(); ++i) {
        const QChar c = v.at(i);
        if (c == QLatin1Char('\\') && i + 1 < v.size()) {
            const QChar n = v.at(++i);
            switch (n.unicode()) {
            case 's': out += QLatin1Char(' '); break;
            case 'n': out += QLatin1Char('\n'); break;
            case 't': out += QLatin1Char('\t'); break;
            case 'r': out += QLatin1Char('\r'); break;
            case '\\': out += QLatin1Char('\\'); break;
            default: out += QLatin1Char('\\'); out += n; break;
            }
        } else {
            out += c;
        }
    }
    return out;
}

static QStringList splitList(const QString &v)
{
    QStringList parts;
    QString cur;
    for (int i = 0; i < v.size(); ++i) {
        const QChar c = v.at(i);
        if (c == QLatin1Char('\\') && i + 1 < v.size() && v.at(i + 1) == QLatin1Char(';')) {
            cur += QLatin1Char(';');
            ++i;
        } else if (c == QLatin1Char(';')) {
            if (!cur.isEmpty()) parts << cur;
            cur.clear();
        } else {
            cur += c;
        }
    }
    if (!cur.isEmpty()) parts << cur;
    return parts;
}

DesktopEntry DesktopEntry::parse(const QString &filePath, const QString &id)
{
    DesktopEntry e;
    e.id = id;
    e.path = filePath;
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return e;

    // Locale matching: prefer Key[lang_COUNTRY], then Key[lang], then Key.
    const QString lc = QLocale().name();            // e.g. "tr_TR"
    const QString lang = lc.section(QLatin1Char('_'), 0, 0);
    QHash<QString, int> rank;                       // key -> best match rank so far

    bool inMain = false;
    QTextStream ts(&f);
    while (!ts.atEnd()) {
        const QString line = ts.readLine().trimmed();
        if (line.isEmpty() || line.startsWith(QLatin1Char('#')))
            continue;
        if (line.startsWith(QLatin1Char('['))) {
            inMain = (line == QLatin1String("[Desktop Entry]"));
            continue;
        }
        if (!inMain)
            continue;
        const int eq = line.indexOf(QLatin1Char('='));
        if (eq <= 0)
            continue;
        QString key = line.left(eq).trimmed();
        const QString raw = line.mid(eq + 1).trimmed();

        int r = 1;
        const int br = key.indexOf(QLatin1Char('['));
        if (br > 0) {
            const QString loc = key.mid(br + 1).chopped(1).section(QLatin1Char('.'), 0, 0).section(QLatin1Char('@'), 0, 0);
            key = key.left(br);
            if (loc == lc) r = 3;
            else if (loc == lang) r = 2;
            else continue;
        }
        if (rank.value(key, 0) > r)
            continue;
        rank[key] = r;

        const QString v = unescapeValue(raw);
        if (key == QLatin1String("Type")) e.isApplication = (v == QLatin1String("Application"));
        else if (key == QLatin1String("Name")) e.name = v;
        else if (key == QLatin1String("GenericName")) e.genericName = v;
        else if (key == QLatin1String("Comment")) e.comment = v;
        else if (key == QLatin1String("Icon")) e.icon = v;
        else if (key == QLatin1String("Exec")) e.exec = raw;   // field-code parsing needs raw value
        else if (key == QLatin1String("TryExec")) e.tryExec = v;
        else if (key == QLatin1String("Path")) e.path_ = v;
        else if (key == QLatin1String("StartupWMClass")) e.startupWMClass = v;
        else if (key == QLatin1String("Terminal")) e.terminal = (v == QLatin1String("true"));
        else if (key == QLatin1String("NoDisplay")) e.noDisplay = (v == QLatin1String("true"));
        else if (key == QLatin1String("Hidden")) e.hidden = (v == QLatin1String("true"));
        else if (key == QLatin1String("Categories")) e.categories = splitList(v);
        else if (key == QLatin1String("MimeType")) e.mimeTypes = splitList(v);
        else if (key == QLatin1String("Keywords")) e.keywords = splitList(v);
        else if (key == QLatin1String("OnlyShowIn")) e.onlyShowIn = splitList(v);
        else if (key == QLatin1String("NotShowIn")) e.notShowIn = splitList(v);
    }
    return e;
}

bool DesktopEntry::shouldShow(const QString &desktop) const
{
    if (!isApplication || hidden || noDisplay)
        return false;
    if (!onlyShowIn.isEmpty() && !onlyShowIn.contains(desktop))
        return false;
    if (notShowIn.contains(desktop))
        return false;
    if (!tryExec.isEmpty()) {
        if (tryExec.startsWith(QLatin1Char('/'))) {
            if (!QFile::exists(tryExec)) return false;
        } else if (QStandardPaths::findExecutable(tryExec).isEmpty()) {
            return false;
        }
    }
    return true;
}

QStringList DesktopEntry::splitExec(const QString &exec)
{
    // Exec values: arguments separated by spaces; double quotes group, and
    // inside quotes \" \` \$ \\ are escapes (the general \\ escaping of the
    // value has not been applied to `exec`, so handle "\\\\" too).
    QStringList args;
    QString cur;
    bool inQuote = false, have = false;
    for (int i = 0; i < exec.size(); ++i) {
        const QChar c = exec.at(i);
        if (inQuote) {
            if (c == QLatin1Char('\\') && i + 1 < exec.size()) {
                QChar n = exec.at(i + 1);
                if (n == QLatin1Char('\\') && i + 2 < exec.size() && exec.at(i + 2) == QLatin1Char('\\')) {
                    // "\\\\" in the file -> one backslash escape inside quotes
                    i += 2;
                    if (i + 1 < exec.size()) { cur += exec.at(++i); }
                    continue;
                }
                if (n == QLatin1Char('"') || n == QLatin1Char('`') || n == QLatin1Char('$') || n == QLatin1Char('\\')) {
                    cur += n;
                    ++i;
                    continue;
                }
                cur += c;
            } else if (c == QLatin1Char('"')) {
                inQuote = false;
            } else {
                cur += c;
            }
        } else if (c == QLatin1Char('"')) {
            inQuote = true;
            have = true;
        } else if (c == QLatin1Char(' ') || c == QLatin1Char('\t')) {
            if (have || !cur.isEmpty()) { args << cur; cur.clear(); have = false; }
        } else if (c == QLatin1Char('\\') && i + 1 < exec.size() && exec.at(i + 1) == QLatin1Char('s')) {
            cur += QLatin1Char(' ');
            ++i;
        } else {
            cur += c;
        }
    }
    if (have || !cur.isEmpty())
        args << cur;
    return args;
}

QStringList DesktopEntry::expandExec(const QStringList &files) const
{
    const QStringList tokens = splitExec(exec);
    QStringList out;
    auto asUrl = [](const QString &f) {
        return f.contains(QLatin1String("://")) ? f : QUrl::fromLocalFile(f).toString();
    };
    auto asFile = [](const QString &f) {
        return f.startsWith(QLatin1String("file://")) ? QUrl(f).toLocalFile() : f;
    };
    for (const QString &t : tokens) {
        if (t == QLatin1String("%f")) { if (!files.isEmpty()) out << asFile(files.first()); continue; }
        if (t == QLatin1String("%F")) { for (const auto &f : files) out << asFile(f); continue; }
        if (t == QLatin1String("%u")) { if (!files.isEmpty()) out << asUrl(files.first()); continue; }
        if (t == QLatin1String("%U")) { for (const auto &f : files) out << asUrl(f); continue; }
        if (t == QLatin1String("%i")) { if (!icon.isEmpty()) out << QStringLiteral("--icon") << icon; continue; }
        QString a;
        for (int i = 0; i < t.size(); ++i) {
            if (t.at(i) == QLatin1Char('%') && i + 1 < t.size()) {
                const QChar n = t.at(++i);
                switch (n.unicode()) {
                case '%': a += QLatin1Char('%'); break;
                case 'c': a += name; break;
                case 'k': a += path; break;
                case 'f': case 'u': if (!files.isEmpty()) a += (n == 'f' ? asFile(files.first()) : asUrl(files.first())); break;
                default: break; // deprecated codes (%d %D %n %N %v %m) are dropped
                }
            } else {
                a += t.at(i);
            }
        }
        if (!a.isEmpty())
            out << a;
    }
    return out;
}
