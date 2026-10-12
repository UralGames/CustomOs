#pragma once

#include <QDateTime>
#include <QJsonArray>
#include <QString>

// An installed update (or rollback) waiting for the reboot that activates it.
struct PendingChange
{
    QString kind;          // "update" or "rollback"
    QString fromVersion;
    QString toVersion;
    QString fromSlot;      // slot that was running when the change was made
    QString targetSlot;    // slot that should be running after the reboot
    QString bootId;        // kernel boot id when the change was made (same boot = not rebooted yet)
    QDateTime when;

    bool isValid() const { return !kind.isEmpty() && !targetSlot.isEmpty(); }
};

// The verdict of customos-health-check about one boot, written to
// <state>/last-boot.env as KEY=VALUE lines (see boot/customos-health-check).
struct BootReport
{
    QString slot;
    QString version;
    QString result;        // "good" or "bad"
    QString reason;
    QDateTime when;
    bool isValid() const { return !slot.isEmpty() && !result.isEmpty(); }
};

// Persistent updater state in the state directory (on the data partition):
//   state.json    pending change, last successful update, last check
//   history.json  the update history shown in Settings (newest last)
// Files are replaced atomically, so a power cut never leaves them half written.
class StateStore
{
public:
    explicit StateStore(const QString &directory);

    bool load();
    bool save();
    QString directory() const { return m_dir; }
    QString errorString() const { return m_error; }

    PendingChange pending() const { return m_pending; }
    void setPending(const PendingChange &p) { m_pending = p; }
    void clearPending() { m_pending = PendingChange(); }

    QString lastSuccessVersion() const { return m_lastSuccessVersion; }
    QDateTime lastSuccessTime() const { return m_lastSuccessTime; }
    void setLastSuccess(const QString &version, const QDateTime &when);

    QDateTime lastCheck() const { return m_lastCheck; }
    void setLastCheck(const QDateTime &when) { m_lastCheck = when; }

    // event: "update", "rollback", "install", "download", "error"
    // result: "success", "failed", "pending", "rolled-back"
    void addHistory(const QString &event, const QString &result, const QString &fromVersion,
                    const QString &toVersion, const QString &message);
    // Turns the newest "pending" entry for toVersion into the final outcome
    // (one history line per update attempt); appends if there is none.
    void resolveHistory(const QString &event, const QString &result, const QString &fromVersion,
                        const QString &toVersion, const QString &message);
    QJsonArray history() const { return m_history; }

    BootReport lastBootReport() const;

    static constexpr int MaxHistory = 200;

private:
    bool writeJson(const QString &name, const QByteArray &data);

    QString m_dir;
    QString m_error;
    PendingChange m_pending;
    QString m_lastSuccessVersion;
    QDateTime m_lastSuccessTime;
    QDateTime m_lastCheck;
    QJsonArray m_history;
};
