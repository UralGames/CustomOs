#pragma once

#include <QObject>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

class QProcess;

// Front-end side of the installer: lists disks and runs the privileged
// `customos-install` script (through sudo) while reporting its progress.
class InstallerBackend : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Installer)
    QML_SINGLETON
    Q_PROPERTY(QVariantList disks READ disks NOTIFY disksChanged)
    Q_PROPERTY(bool uefi READ uefi CONSTANT)
    Q_PROPERTY(bool running READ running NOTIFY runningChanged)
    Q_PROPERTY(int progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(QString status READ status NOTIFY progressChanged)
    Q_PROPERTY(QString log READ log NOTIFY logChanged)

public:
    explicit InstallerBackend(QObject *parent = nullptr);

    QVariantList disks() const { return m_disks; }
    bool uefi() const;
    bool running() const { return m_running; }
    int progress() const { return m_progress; }
    QString status() const { return m_status; }
    QString log() const { return m_log; }

    Q_INVOKABLE void refreshDisks();
    Q_INVOKABLE void install(const QString &disk, const QString &confirmDisk, const QString &user,
                             const QString &fullName, const QString &hostName, const QString &password);
    Q_INVOKABLE void reboot();
    Q_INVOKABLE QString suggestUserName(const QString &fullName) const;

signals:
    void disksChanged();
    void runningChanged();
    void progressChanged();
    void logChanged();
    void finished(bool ok, const QString &message);

private:
    QVariantList m_disks;
    bool m_running = false;
    int m_progress = 0;
    QString m_status;
    QString m_log;
    QString m_error;
    QProcess *m_proc = nullptr;
};
