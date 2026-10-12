#pragma once

#include <QObject>
#include <QStringList>
#include <QtQml/qqmlregistration.h>

class QQmlEngine;
class QJSEngine;

// Static facts about the machine, for Settings > About and System Monitor.
class SystemInfo : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(SystemInfo)
    QML_SINGLETON
    Q_PROPERTY(QString osName READ osName CONSTANT)
    Q_PROPERTY(QString osVersion READ osVersion CONSTANT)
    Q_PROPERTY(QString kernel READ kernel CONSTANT)
    Q_PROPERTY(QString architecture READ architecture CONSTANT)
    Q_PROPERTY(QString cpuModel READ cpuModel CONSTANT)
    Q_PROPERTY(int cpuCount READ cpuCount CONSTANT)
    Q_PROPERTY(double memoryTotal READ memoryTotal CONSTANT)
    Q_PROPERTY(QString graphicsDriver READ graphicsDriver CONSTANT)
    Q_PROPERTY(bool softwareRendering READ softwareRendering CONSTANT)
    Q_PROPERTY(QString machine READ machine CONSTANT)
    Q_PROPERTY(QString qtVersion READ qtVersion CONSTANT)
    Q_PROPERTY(QString desktopVersion READ desktopVersion CONSTANT)

public:
    static SystemInfo *create(QQmlEngine *, QJSEngine *);

    QString osName() const { return m_osName; }
    QString osVersion() const { return m_osVersion; }
    QString kernel() const { return m_kernel; }
    QString architecture() const { return m_arch; }
    QString cpuModel() const { return m_cpuModel; }
    int cpuCount() const { return m_cpuCount; }
    double memoryTotal() const { return m_memTotal; }
    QString graphicsDriver() const { return m_gpuDriver; }
    bool softwareRendering() const;
    QString machine() const { return m_machine; }
    QString qtVersion() const;
    QString desktopVersion() const;

    Q_INVOKABLE qint64 uptimeSeconds() const;
    Q_INVOKABLE QVariantMap diskUsage(const QString &path = QStringLiteral("/")) const;
    Q_INVOKABLE QStringList userGroups() const;

private:
    explicit SystemInfo(QObject *parent = nullptr);
    QString m_osName, m_osVersion, m_kernel, m_arch, m_cpuModel, m_gpuDriver, m_machine;
    int m_cpuCount = 1;
    double m_memTotal = 0;
};
