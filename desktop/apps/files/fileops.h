#pragma once

#include <QObject>
#include <QStringList>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

class QQmlEngine;
class QJSEngine;

// File operations for the Files app. Long operations (copy, move, delete,
// trash) run on a worker thread and report progress; they never overwrite:
// name clashes get " (2)", " (3)", ... appended.
class FileOps : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(FileOps)
    QML_SINGLETON
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString status READ status NOTIFY progressChanged)
    Q_PROPERTY(double progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(QStringList clipboardPaths READ clipboardPaths NOTIFY clipboardChanged)
    Q_PROPERTY(bool clipboardCut READ clipboardCut NOTIFY clipboardChanged)
    Q_PROPERTY(QString trashPath READ trashPath CONSTANT)

public:
    static FileOps *create(QQmlEngine *, QJSEngine *);

    bool busy() const { return m_busy; }
    QString status() const { return m_status; }
    double progress() const { return m_progress; }
    QStringList clipboardPaths() const;
    bool clipboardCut() const { return m_cut; }
    QString trashPath() const;

    Q_INVOKABLE void copy(const QStringList &sources, const QString &destDir);
    Q_INVOKABLE void move(const QStringList &sources, const QString &destDir);
    Q_INVOKABLE void trash(const QStringList &paths);
    Q_INVOKABLE void deletePermanently(const QStringList &paths);
    Q_INVOKABLE void restore(const QStringList &trashedPaths);
    Q_INVOKABLE void emptyTrash();
    Q_INVOKABLE void ensureTrash();

    // Quick operations; return "" on success or an error message.
    Q_INVOKABLE QString rename(const QString &path, const QString &newName);
    Q_INVOKABLE QString newFolder(const QString &dir, const QString &name);
    Q_INVOKABLE QString newFile(const QString &dir, const QString &name);
    Q_INVOKABLE QString uniqueName(const QString &dir, const QString &name) const;

    Q_INVOKABLE QVariantMap properties(const QString &path) const;
    Q_INVOKABLE void computeSize(const QString &path);   // -> sizeComputed

    // Clipboard (also published as text/uri-list for other applications).
    Q_INVOKABLE void setClipboard(const QStringList &paths, bool cut);
    Q_INVOKABLE void paste(const QString &destDir);
    Q_INVOKABLE bool canPaste() const;

signals:
    void busyChanged();
    void progressChanged();
    void clipboardChanged();
    void finished(bool ok, const QString &message);
    void sizeComputed(const QString &path, double bytes, int files, int folders);

private:
    explicit FileOps(QObject *parent = nullptr);
    enum class Kind { Copy, Move, Trash, Delete, Restore, EmptyTrash };
    void runJob(Kind kind, const QStringList &paths, const QString &dest);

    bool m_busy = false;
    QString m_status;
    double m_progress = 0;
    QStringList m_clip;
    bool m_cut = false;
};
