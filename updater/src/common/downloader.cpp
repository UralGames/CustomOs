#include "downloader.h"

#include <QDir>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QRegularExpression>

#include <unistd.h>

Downloader::Downloader(QNetworkAccessManager *nam, GitHubClient *github, QObject *parent)
    : QObject(parent), m_nam(nam), m_github(github)
{
}

void Downloader::start(const ReleaseAsset &asset, const QString &targetPath, qint64 expectedSize)
{
    if (m_reply)
        return;
    m_target = targetPath;
    m_expected = expectedSize;
    m_cancelled = false;
    m_restarted = false;
    m_abortError = UpdateError();
    m_rate = 0;

    QFileInfo done(m_target);
    if (done.exists() && done.size() == m_expected) {
        emit progress(m_expected, m_expected, 0);
        emit finished(UpdateError());
        return;
    }
    QFile::remove(m_target);
    QDir().mkpath(QFileInfo(m_target).absolutePath());

    m_file.setFileName(m_target + QStringLiteral(".part"));
    if (!m_file.open(QIODevice::ReadWrite)) {
        emit finished(UpdateError::make(UpdateError::DownloadFailed,
                                        QStringLiteral("cannot open %1: %2").arg(m_file.fileName(), m_file.errorString())));
        return;
    }
    m_offset = m_file.size();
    if (m_offset > m_expected) {   // stale partial file from another release
        m_file.resize(0);
        m_offset = 0;
    }
    m_written = m_offset;

    QNetworkRequest req = m_github->assetRequest(asset);
    if (m_offset > 0 && m_offset < m_expected)
        req.setRawHeader("Range", "bytes=" + QByteArray::number(m_offset) + '-');
    m_clock.start();
    m_rateMark = 0;
    m_rateBytes = m_written;
    request(req, 0);
}

void Downloader::request(const QNetworkRequest &req, int redirects)
{
    if (!m_github->urlAllowed(req.url())) {
        fail(UpdateError::make(UpdateError::DownloadFailed, QStringLiteral("refused non-HTTPS URL %1").arg(req.url().toString())));
        return;
    }
    m_request = req;
    m_redirects = redirects;
    m_accepting = false;
    m_reply = m_nam->get(req);
    connect(m_reply, &QNetworkReply::metaDataChanged, this, &Downloader::onMetaData);
    connect(m_reply, &QNetworkReply::readyRead, this, &Downloader::onReadyRead);
    connect(m_reply, &QNetworkReply::finished, this, &Downloader::onFinished);
}

void Downloader::onMetaData()
{
    const int status = m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (status == 200) {
        // Full body: the server ignored (or we did not send) a Range header.
        if (m_written != 0) {
            m_file.resize(0);
            m_file.seek(0);
            m_written = 0;
            m_offset = 0;
        }
        m_accepting = true;
    } else if (status == 206) {
        // "Content-Range: bytes <start>-<end>/<total>" must continue our file.
        static const QRegularExpression re(QStringLiteral("^bytes (\\d+)-(\\d+)/(\\d+|\\*)$"));
        const auto m = re.match(QString::fromLatin1(m_reply->rawHeader("Content-Range")).trimmed());
        if (!m.hasMatch() || m.captured(1).toLongLong() != m_written) {
            // Does not continue our file: drop it so the retry starts clean.
            m_file.resize(0);
            m_written = m_offset = 0;
            abortWith(UpdateError::make(UpdateError::DownloadFailed, QStringLiteral("unexpected Content-Range from server")));
            return;
        }
        m_file.seek(m_written);
        m_accepting = true;
    }
}

void Downloader::onReadyRead()
{
    if (!m_accepting) {
        m_reply->readAll();   // body of a redirect or error response
        return;
    }
    const QByteArray chunk = m_reply->readAll();
    if (m_written + chunk.size() > m_expected) {
        m_file.resize(0);
        m_written = m_offset = 0;
        abortWith(UpdateError::make(UpdateError::DownloadFailed,
                                    QStringLiteral("server sent more data than the published size (%1 bytes)").arg(m_expected)));
        return;
    }
    if (m_file.write(chunk) != chunk.size()) {
        abortWith(UpdateError::make(UpdateError::NoSpace, m_file.errorString()));
        return;
    }
    m_written += chunk.size();

    const qint64 now = m_clock.elapsed();
    if (now - m_rateMark >= 500) {
        const double inst = double(m_written - m_rateBytes) * 1000.0 / double(qMax<qint64>(1, now - m_rateMark));
        m_rate = m_rate <= 0 ? inst : 0.7 * m_rate + 0.3 * inst;
        m_rateMark = now;
        m_rateBytes = m_written;
        emit progress(m_written, m_expected, m_rate);
    }
}

void Downloader::onFinished()
{
    QNetworkReply *reply = m_reply;
    if (!reply)
        return;
    reply->deleteLater();
    m_reply = nullptr;
    if (!m_file.isOpen())
        return;   // already failed
    if (m_cancelled) {
        m_file.close();
        emit finished(UpdateError::make(UpdateError::Cancelled));
        return;
    }
    if (m_abortError.isError()) {
        const UpdateError e = m_abortError;
        m_abortError = UpdateError();
        fail(e);
        return;
    }
    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if ((status == 301 || status == 302 || status == 303 || status == 307 || status == 308) && m_redirects < 8) {
        const QUrl location = m_request.url().resolved(QUrl(QString::fromUtf8(reply->rawHeader("Location"))));
        request(m_github->redirectRequest(m_request, location), m_redirects + 1);
        return;
    }
    if (status == 416 && !m_restarted) {
        // Range not satisfiable: our partial file is unusable, start over.
        m_restarted = true;
        m_file.resize(0);
        m_written = m_offset = 0;
        QNetworkRequest req = m_request;
        req.setRawHeader("Range", QByteArray());
        request(req, m_redirects);
        return;
    }
    const UpdateError e = GitHubClient::classify(reply, QString(), true);
    if (e.isError()) {
        fail(e);
        return;
    }
    if (m_written != m_expected) {
        fail(UpdateError::make(UpdateError::DownloadFailed,
                               QStringLiteral("received %1 of %2 bytes").arg(m_written).arg(m_expected)));
        return;
    }
    finishOk();
}

void Downloader::finishOk()
{
    m_file.flush();
    ::fsync(m_file.handle());
    m_file.close();
    if (!QFile::rename(m_file.fileName(), m_target)) {
        emit finished(UpdateError::make(UpdateError::DownloadFailed, QStringLiteral("cannot rename the finished download")));
        return;
    }
    emit progress(m_expected, m_expected, m_rate);
    emit finished(UpdateError());
}

void Downloader::fail(const UpdateError &error)
{
    if (m_file.isOpen()) {
        m_file.flush();
        m_file.close();   // keep the .part file so the next attempt resumes
    }
    emit finished(error);
}

void Downloader::abortWith(const UpdateError &error)
{
    m_accepting = false;
    m_abortError = error;
    if (m_reply)
        m_reply->abort();   // onFinished() reports m_abortError
}

void Downloader::cancel()
{
    if (!m_reply)
        return;
    m_cancelled = true;
    m_reply->abort();
}
