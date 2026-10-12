#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QObject>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

class QTimer;

// System-wide resource usage, sampled once per second from /proc.
class SystemStats : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(double cpu READ cpu NOTIFY updated)                  // 0..100
    Q_PROPERTY(QVariantList cpuHistory READ cpuHistory NOTIFY updated)
    Q_PROPERTY(QVariantList perCore READ perCore NOTIFY updated)
    Q_PROPERTY(double memTotal READ memTotal NOTIFY updated)
    Q_PROPERTY(double memUsed READ memUsed NOTIFY updated)
    Q_PROPERTY(double memCached READ memCached NOTIFY updated)
    Q_PROPERTY(QVariantList memHistory READ memHistory NOTIFY updated)
    Q_PROPERTY(double swapTotal READ swapTotal NOTIFY updated)
    Q_PROPERTY(double swapUsed READ swapUsed NOTIFY updated)
    Q_PROPERTY(double netRx READ netRx NOTIFY updated)              // bytes/s
    Q_PROPERTY(double netTx READ netTx NOTIFY updated)
    Q_PROPERTY(QVariantList netHistory READ netHistory NOTIFY updated)
    Q_PROPERTY(QVariantList disks READ disks NOTIFY updated)
    Q_PROPERTY(QString loadAverage READ loadAverage NOTIFY updated)
    Q_PROPERTY(int processCount READ processCount NOTIFY updated)
    Q_PROPERTY(bool running READ running WRITE setRunning NOTIFY runningChanged)

public:
    explicit SystemStats(QObject *parent = nullptr);

    double cpu() const { return m_cpu; }
    QVariantList cpuHistory() const { return m_cpuHist; }
    QVariantList perCore() const { return m_perCore; }
    double memTotal() const { return m_memTotal; }
    double memUsed() const { return m_memUsed; }
    double memCached() const { return m_memCached; }
    QVariantList memHistory() const { return m_memHist; }
    double swapTotal() const { return m_swapTotal; }
    double swapUsed() const { return m_swapUsed; }
    double netRx() const { return m_rx; }
    double netTx() const { return m_tx; }
    QVariantList netHistory() const { return m_netHist; }
    QVariantList disks() const { return m_disks; }
    QString loadAverage() const { return m_load; }
    int processCount() const { return m_procs; }
    bool running() const;
    void setRunning(bool r);

public slots:
    void sample();

signals:
    void updated();
    void runningChanged();

private:
    struct CpuTimes { quint64 idle = 0, total = 0; };
    QList<CpuTimes> m_prevCpu;
    quint64 m_prevRx = 0, m_prevTx = 0;
    qint64 m_prevNetMs = 0;
    double m_cpu = 0, m_memTotal = 0, m_memUsed = 0, m_memCached = 0, m_swapTotal = 0, m_swapUsed = 0;
    double m_rx = 0, m_tx = 0;
    QVariantList m_cpuHist, m_memHist, m_netHist, m_perCore, m_disks;
    QString m_load;
    int m_procs = 0;
    QTimer *m_timer = nullptr;
};

// Running processes with CPU and memory use.
class ProcessModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString sortKey READ sortKey WRITE setSortKey NOTIFY sortChanged)    // cpu|memory|name|pid|user
    Q_PROPERTY(bool sortDescending READ sortDescending WRITE setSortDescending NOTIFY sortChanged)
    Q_PROPERTY(QString filter READ filter WRITE setFilter NOTIFY filterChanged)
    Q_PROPERTY(bool onlyMine READ onlyMine WRITE setOnlyMine NOTIFY filterChanged)
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(bool running READ running WRITE setRunning NOTIFY runningChanged)

public:
    enum Roles { PidRole = Qt::UserRole + 1, NameRole, CommandRole, UserRole, CpuRole, MemoryRole,
                 StateRole, ThreadsRole, MineRole };
    explicit ProcessModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString sortKey() const { return m_sortKey; }
    void setSortKey(const QString &k);
    bool sortDescending() const { return m_desc; }
    void setSortDescending(bool d);
    QString filter() const { return m_filter; }
    void setFilter(const QString &f);
    bool onlyMine() const { return m_onlyMine; }
    void setOnlyMine(bool m);
    bool running() const;
    void setRunning(bool r);

    // Sends SIGTERM (force=false) or SIGKILL; returns "" or an error message.
    Q_INVOKABLE QString endProcess(int pid, bool force);
    Q_INVOKABLE void refresh();

signals:
    void sortChanged();
    void filterChanged();
    void countChanged();
    void runningChanged();

private:
    struct Proc {
        int pid = 0;
        QString name, command, user, state;
        uint uid = 0;
        double cpu = 0;      // percent of one core x cores (0..100 of whole machine)
        double memory = 0;   // bytes (resident)
        int threads = 0;
    };
    void apply();
    QList<Proc> m_all, m_rows;
    QHash<int, quint64> m_prevTicks;
    quint64 m_prevTotal = 0;
    QString m_sortKey = QStringLiteral("cpu");
    QString m_filter;
    bool m_desc = true;
    bool m_onlyMine = false;
    QTimer *m_timer = nullptr;
};
