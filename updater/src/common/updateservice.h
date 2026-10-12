#pragma once

#include "config.h"
#include "errors.h"
#include "githubclient.h"
#include "manifest.h"
#include "raucclient.h"
#include "statestore.h"
#include "systemstate.h"

#include <QDBusConnection>
#include <QObject>
#include <QTimer>
#include <QVariantMap>

class QNetworkAccessManager;
class Downloader;

// The update state machine behind org.customos.Update1.
//
//   idle -> checking -> up-to-date | update-available
//   update-available -> downloading -> verifying -> (ready) -> installing -> reboot-required
//   any step -> error (the previous release information is kept so Retry works)
//
// After the reboot, start() reconciles the pending change with the slot that
// actually booted and with the health check's verdict, and records the
// outcome (success, or failure with automatic rollback) in the history.
class UpdateService : public QObject
{
    Q_OBJECT
public:
    enum class State {
        Idle,
        Checking,
        UpToDate,
        UpdateAvailable,
        Downloading,
        Verifying,
        Installing,
        Confirming,      // first boot of a new version, waiting for the health check
        RebootRequired,
        Error
    };
    Q_ENUM(State)

    UpdateService(const UpdateConfig &config, const QDBusConnection &systemBus, QObject *parent = nullptr);
    ~UpdateService() override;

    void start();

    QVariantMap status() const;
    QString historyJson() const;
    State state() const { return m_state; }
    static QString stateName(State s);

    UpdateError checkForUpdates(bool userInitiated = true);
    UpdateError download();
    UpdateError install();
    UpdateError cancel();
    UpdateError reboot();
    UpdateError rollback();
    void dismissError();

    // Exposed for tests.
    SystemState systemState() const { return m_system; }
    void setSystemState(const SystemState &s) { m_system = s; }

signals:
    void statusChanged(const QVariantMap &status);
    void updateAvailable(const QString &version, const QString &name);

private:
    enum class Goal { None, Download, Install };

    void setState(State s);
    void setError(const UpdateError &e);
    void setProgress(int percent, const QString &text);
    void notifyChanged();
    UpdateError supportedForInstall() const;
    bool busy() const;

    void onReleaseFound(const ReleaseInfo &release, const UpdateError &error, bool userInitiated);
    void onManifest(const QByteArray &data, const UpdateError &error, bool userInitiated);
    void onChecksums(const QByteArray &data, const UpdateError &error, bool userInitiated);
    void finishCheck(const UpdateError &error, bool userInitiated);

    void startDownload();
    void onDownloadFinished(const UpdateError &error);
    void verifyBundle();
    void onHashed(const QByteArray &sha256);
    void onInspected(const RaucClient::BundleInfo &info, const QString &error);
    void startInstall();
    void onInstallCompleted(int result);

    void reconcileAfterBoot();
    void pollHealth();
    void refreshSlots();
    void cleanCache(const QString &keep);
    QString bundlePath() const;
    UpdateError classifyRaucError(const QString &message, UpdateError::Code fallback) const;
    void saveState();

    UpdateConfig m_config;
    QDBusConnection m_bus;
    SystemState m_system;
    StateStore m_store;
    QNetworkAccessManager *m_nam;
    GitHubClient *m_github;
    Downloader *m_downloader;
    RaucClient *m_rauc;
    QTimer m_checkTimer;
    QTimer m_notifyTimer;
    QTimer m_healthTimer;
    int m_healthPolls = 0;

    State m_state = State::Idle;
    State m_stateBeforeError = State::Idle;
    UpdateError m_error;
    UpdateError m_lastCheckError;
    Goal m_goal = Goal::None;

    ReleaseInfo m_release;
    ReleaseManifest m_manifest;
    bool m_haveUpdate = false;
    bool m_downloaded = false;     // bundle downloaded *and* verified
    QString m_notifiedVersion;

    int m_progress = -1;
    QString m_progressText;
    qint64 m_bytesDone = 0;
    double m_rate = 0;

    QString m_otherSlotVersion;
    bool m_otherSlotGood = false;
    QString m_otherSlotBootname;
    QString m_bootedSlotStatus;
    bool m_tlsAvailable = true;
};
