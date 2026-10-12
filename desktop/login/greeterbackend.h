#pragma once

#include <QObject>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

class QSocketNotifier;

// Talks to meridian-login over stdin/stdout (see meridian-login.cpp).
class GreeterBackend : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Greeter)
    QML_SINGLETON
    Q_PROPERTY(QVariantList users READ users CONSTANT)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString hostName READ hostName CONSTANT)
    Q_PROPERTY(bool live READ live CONSTANT)

public:
    explicit GreeterBackend(QObject *parent = nullptr);

    QVariantList users() const { return m_users; }
    bool busy() const { return m_busy; }
    QString hostName() const;
    bool live() const;

    Q_INVOKABLE void login(const QString &user, const QString &password);
    Q_INVOKABLE void powerOff();
    Q_INVOKABLE void reboot();

signals:
    void failed(const QString &message);
    void succeeded();
    void busyChanged();

private:
    void send(const QByteArray &line);
    QVariantList m_users;
    QSocketNotifier *m_notifier = nullptr;
    QByteArray m_buffer;
    bool m_busy = false;
};
