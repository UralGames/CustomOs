#pragma once

#include <QObject>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

class QQmlEngine;
class QJSEngine;

// System-level settings that need privileges or system tools. Each action
// runs asynchronously and reports through `finished(action, ok, message)`.
//  * hostname / time zone / network time: systemd's hostnamectl and
//    timedatectl (authorised for administrators by polkit)
//  * password: `sudo -S chpasswd`, authorised by the current password
//  * keyboard layouts: the XKB rules database
class SystemActions : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(SystemActions)
    QML_SINGLETON
    Q_PROPERTY(QString timezone READ timezone NOTIFY timeChanged)
    Q_PROPERTY(bool ntp READ ntp NOTIFY timeChanged)

public:
    static SystemActions *create(QQmlEngine *, QJSEngine *);

    QString timezone() const { return m_timezone; }
    bool ntp() const { return m_ntp; }

    Q_INVOKABLE void refreshTime();
    Q_INVOKABLE QStringList timezones() const;
    Q_INVOKABLE void setTimezone(const QString &tz);
    Q_INVOKABLE void setNtp(bool on);
    Q_INVOKABLE void setHostname(const QString &name);
    Q_INVOKABLE void changePassword(const QString &current, const QString &next);

    // [{ code, name }] from /usr/share/X11/xkb/rules/base.lst
    Q_INVOKABLE QVariantList keyboardLayouts() const;
    Q_INVOKABLE QVariantList keyboardVariants(const QString &layout) const;

signals:
    void finished(const QString &action, bool ok, const QString &message);
    void timeChanged();

private:
    explicit SystemActions(QObject *parent = nullptr);
    void run(const QString &action, const QString &program, const QStringList &args,
             const QByteArray &input = {}, const QString &okMessage = {});
    QString m_timezone;
    bool m_ntp = false;
};
