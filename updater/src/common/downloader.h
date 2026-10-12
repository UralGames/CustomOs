#pragma once

#include "errors.h"
#include "githubclient.h"

#include <QElapsedTimer>
#include <QFile>
#include <QNetworkReply>
#include <QObject>
#include <QPointer>

class QNetworkAccessManager;

// Downloads one release asset to a file, resuming an interrupted transfer
// (HTTP Range) from "<target>.part". The finished file is renamed to
// <target> only after it reached the expected size and was synced to disk;
// verifying its contents is the caller's job.
class Downloader : public QObject
{
    Q_OBJECT
public:
    Downloader(QNetworkAccessManager *nam, GitHubClient *github, QObject *parent = nullptr);

    void start(const ReleaseAsset &asset, const QString &targetPath, qint64 expectedSize);
    void cancel();
    bool isRunning() const { return m_reply != nullptr; }

signals:
    void progress(qint64 received, qint64 total, double bytesPerSecond);
    void finished(const UpdateError &error);

private:
    void request(const QNetworkRequest &req, int redirects);
    void onMetaData();
    void onReadyRead();
    void onFinished();
    void fail(const UpdateError &error);
    void abortWith(const UpdateError &error);
    void finishOk();

    QNetworkAccessManager *m_nam;
    GitHubClient *m_github;
    QPointer<QNetworkReply> m_reply;
    QFile m_file;
    QString m_target;
    qint64 m_expected = 0;
    qint64 m_offset = 0;       // bytes already in the .part file when the request started
    qint64 m_written = 0;      // total bytes in the .part file
    bool m_accepting = false;  // the current response carries the body we want
    bool m_cancelled = false;
    bool m_restarted = false;
    int m_redirects = 0;
    QNetworkRequest m_request;
    UpdateError m_abortError;  // reason for an abort() we started ourselves
    QElapsedTimer m_clock;
    qint64 m_rateMark = 0;
    qint64 m_rateBytes = 0;
    double m_rate = 0;
};
