#pragma once

#include <QObject>
#include <QStringList>
#include <QUrl>
#include <QtQml/qqmlregistration.h>

class QQmlEngine;
class QJSEngine;

// Small utilities for QML: paths, formatting, wallpapers, environment.
class Sys : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Sys)
    QML_SINGLETON
    Q_PROPERTY(QString homePath READ homePath CONSTANT)
    Q_PROPERTY(QString userName READ userName CONSTANT)
    Q_PROPERTY(QString dataDir READ dataDir CONSTANT)
    Q_PROPERTY(bool isLive READ isLive CONSTANT)
    Q_PROPERTY(QString platformName READ platformName CONSTANT)

public:
    static Sys *create(QQmlEngine *, QJSEngine *);

    QString homePath() const;
    QString userName() const;
    QString dataDir() const;
    bool isLive() const;
    QString platformName() const;

    Q_INVOKABLE QString hostName() const;
    Q_INVOKABLE QString userFullName() const;
    Q_INVOKABLE QString env(const QString &name) const;
    Q_INVOKABLE bool fileExists(const QString &path) const;
    Q_INVOKABLE bool isDir(const QString &path) const;
    Q_INVOKABLE QString fileName(const QString &path) const;
    Q_INVOKABLE QString parentDir(const QString &path) const;
    Q_INVOKABLE QUrl fileUrl(const QString &path) const;
    Q_INVOKABLE QString urlToPath(const QUrl &url) const;
    Q_INVOKABLE QString standardDir(const QString &which) const;   // documents, downloads, ...
    Q_INVOKABLE QString formatBytes(double bytes) const;
    Q_INVOKABLE QString formatDuration(qint64 seconds) const;
    Q_INVOKABLE QStringList wallpapers() const;
    // Copies an image into ~/Pictures/Wallpapers and returns its new path.
    Q_INVOKABLE QString importWallpaper(const QString &path) const;
    Q_INVOKABLE bool hasExecutable(const QString &name) const;

private:
    explicit Sys(QObject *parent = nullptr) : QObject(parent) {}
};
