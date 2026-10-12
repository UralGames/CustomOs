#pragma once

#include <QString>
#include <QStringList>

// A parsed freedesktop.org .desktop file (Desktop Entry Specification 1.5,
// the subset a desktop shell needs).
struct DesktopEntry
{
    QString id;          // file name, e.g. "org.gnome.Foo.desktop"
    QString path;        // absolute path of the file
    QString name;
    QString genericName;
    QString comment;
    QString icon;
    QString exec;
    QString tryExec;
    QString path_;       // Path= (working directory)
    QString startupWMClass;
    QStringList categories;
    QStringList mimeTypes;
    QStringList keywords;
    QStringList onlyShowIn;
    QStringList notShowIn;
    bool terminal = false;
    bool noDisplay = false;
    bool hidden = false;
    bool isApplication = true;

    bool isValid() const { return !name.isEmpty() && (!exec.isEmpty() || !isApplication); }
    bool shouldShow(const QString &desktop = QStringLiteral("Meridian")) const;

    static DesktopEntry parse(const QString &filePath, const QString &id);

    // Splits Exec= into argv and expands field codes (%f %F %u %U %i %c %k).
    QStringList expandExec(const QStringList &files) const;
    static QStringList splitExec(const QString &exec);
};
