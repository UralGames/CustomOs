#include "updateservice.h"

#include "downloader.h"

#include <QCryptographicHash>
#include <QDBusMessage>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QPointer>
#include <QProcess>
#include <QRandomGenerator>
#include <QStorageInfo>
#include <QThread>
#if QT_CONFIG(ssl)
#include <QSslSocket>
#endif

#include <memory>

static QString sysroot()
{
    return qEnvironmentVariable("CUSTOMOS_UPDATE_SYSROOT");   // tests only
}

static QString readFirstLine(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return QString();
    return QString::fromUtf8(f.readLine()).trimmed();
}

static QString bootId()
{
    return readFirstLine(sysroot() + QStringLiteral("/proc/sys/kernel/random/boot_id"));
}

static QDateTime bootTime()
{
    const double up = readFirstLine(sysroot() + QStringLiteral("/proc/uptime")).section(QLatin1Char(' '), 0, 0).toDouble();
    return QDateTime::currentDateTimeUtc().addSecs(-qint64(up));
}

static QString formatBytes(qint64 b)
{
    const char *units[] = {"B", "KB", "MB", "GB"};
    double v = double(b);
    int i = 0;
    while (v >= 1000 && i < 3) { v /= 1000; ++i; }
    return QStringLiteral("%1 %2").arg(v, 0, 'f', i == 0 ? 0 : 1).arg(QLatin1String(units[i]));
}

UpdateService::UpdateService(const UpdateConfig &config, const QDBusConnection &systemBus, QObject *parent)
    : QObject(parent),
      m_config(config),
      m_bus(systemBus),
      m_system(SystemState::probe()),
      m_store(config.stateDirectory),
      m_nam(new QNetworkAccessManager(this)),
      m_github(new GitHubClient(m_nam, config, this)),
      m_downloader(new Downloader(m_nam, m_github, this)),
      m_rauc(new RaucClient(systemBus, this))
{
    m_nam->setAutoDeleteReplies(false);
    m_notifyTimer.setSingleShot(true);
    m_notifyTimer.setInterval(150);   // coalesce bursts of changes into one signal
    connect(&m_notifyTimer, &QTimer::timeout, this, [this] { emit statusChanged(status()); });

    connect(m_downloader, &Downloader::progress, this, [this](qint64 received, qint64 total, double rate) {
        m_bytesDone = received;
        m_rate = rate;
        const int pct = total > 0 ? int(received * 100 / total) : -1;
        setProgress(pct, tr("Downloading %1 of %2").arg(formatBytes(received), formatBytes(total)));
    });
    connect(m_downloader, &Downloader::finished, this, &UpdateService::onDownloadFinished);
    connect(m_rauc, &RaucClient::progressChanged, this, [this](int percent, const QString &message) {
        if (m_state == State::Installing)
            setProgress(percent, message);
    });
    connect(m_rauc, &RaucClient::completed, this, &UpdateService::onInstallCompleted);

    m_checkTimer.setSingleShot(true);
    connect(&m_checkTimer, &QTimer::timeout, this, [this] {
        if (!busy())
            checkForUpdates(false);
        m_checkTimer.start(m_config.checkIntervalHours * 3600 * 1000);
    });
    m_healthTimer.setInterval(5000);
    connect(&m_healthTimer, &QTimer::timeout, this, &UpdateService::pollHealth);

#if QT_CONFIG(ssl)
    m_tlsAvailable = QSslSocket::supportsSsl();
#else
    m_tlsAvailable = false;
#endif
}

UpdateService::~UpdateService() = default;

void UpdateService::start()
{
    m_store.load();
    reconcileAfterBoot();
    refreshSlots();
    if (m_config.automaticCheck) {
        // First check a little after boot (with jitter so many machines do
        // not hit GitHub at once), then every CheckIntervalHours.
        const int delay = 90 + int(QRandomGenerator::global()->bounded(120));
        m_checkTimer.start(delay * 1000);
    }
    notifyChanged();
}

QString UpdateService::stateName(State s)
{
    switch (s) {
    case State::Idle: return QStringLiteral("idle");
    case State::Checking: return QStringLiteral("checking");
    case State::UpToDate: return QStringLiteral("up-to-date");
    case State::UpdateAvailable: return QStringLiteral("update-available");
    case State::Downloading: return QStringLiteral("downloading");
    case State::Verifying: return QStringLiteral("verifying");
    case State::Installing: return QStringLiteral("installing");
    case State::Confirming: return QStringLiteral("confirming");
    case State::RebootRequired: return QStringLiteral("reboot-required");
    case State::Error: return QStringLiteral("error");
    }
    return QStringLiteral("idle");
}

bool UpdateService::busy() const
{
    return m_state == State::Checking || m_state == State::Downloading || m_state == State::Verifying
        || m_state == State::Installing;
}

UpdateError UpdateService::supportedForInstall() const
{
    switch (m_system.mode) {
    case SystemState::Mode::AB: return UpdateError();
    case SystemState::Mode::Live: return UpdateError::make(UpdateError::UnsupportedLive);
    case SystemState::Mode::Legacy:
    case SystemState::Mode::Unknown: break;
    }
    return UpdateError::make(UpdateError::UnsupportedLegacy);
}

QVariantMap UpdateService::status() const
{
    const PendingChange pending = m_store.pending();
    const UpdateError unsupported = supportedForInstall();
    QVariantMap s;
    s.insert(QStringLiteral("state"), stateName(m_state));
    s.insert(QStringLiteral("busy"), busy());
    s.insert(QStringLiteral("mode"), SystemState::modeName(m_system.mode));
    s.insert(QStringLiteral("supported"), !unsupported.isError());
    s.insert(QStringLiteral("unsupportedMessage"), unsupported.message);
    s.insert(QStringLiteral("osName"), m_system.osName);
    s.insert(QStringLiteral("prettyName"), m_system.prettyName);
    s.insert(QStringLiteral("currentVersion"), m_system.versionText);
    s.insert(QStringLiteral("buildId"), m_system.buildId);
    s.insert(QStringLiteral("bootedSlot"), m_system.bootedSlot);
    s.insert(QStringLiteral("otherSlot"), m_otherSlotBootname);
    s.insert(QStringLiteral("otherSlotVersion"), m_otherSlotVersion);
    s.insert(QStringLiteral("rollbackAvailable"),
             m_system.mode == SystemState::Mode::AB && m_otherSlotGood && !pending.isValid() && !busy());
    s.insert(QStringLiteral("repository"), m_config.repository);
    s.insert(QStringLiteral("channel"), m_config.channel);
    s.insert(QStringLiteral("automaticCheck"), m_config.automaticCheck);
    s.insert(QStringLiteral("updaterVersion"), QStringLiteral(CUSTOMOS_UPDATER_VERSION));

    s.insert(QStringLiteral("updateAvailable"), m_haveUpdate);
    s.insert(QStringLiteral("availableVersion"), m_haveUpdate ? m_manifest.version.toString() : QString());
    s.insert(QStringLiteral("releaseName"), m_haveUpdate ? m_release.name : QString());
    s.insert(QStringLiteral("releaseNotes"),
             m_haveUpdate ? (m_manifest.changelog.isEmpty() ? m_release.body : m_manifest.changelog) : QString());
    s.insert(QStringLiteral("releaseUrl"), m_haveUpdate ? m_release.htmlUrl.toString() : QString());
    s.insert(QStringLiteral("releaseDate"),
             m_haveUpdate && m_release.published.isValid() ? m_release.published.toSecsSinceEpoch() : qint64(0));
    s.insert(QStringLiteral("downloadSize"), m_haveUpdate ? qulonglong(m_manifest.bundleSize) : qulonglong(0));
    s.insert(QStringLiteral("rebootNeeded"), m_haveUpdate ? m_manifest.rebootRequired : true);
    s.insert(QStringLiteral("downloaded"), m_downloaded);
    s.insert(QStringLiteral("canInstall"), m_haveUpdate && !unsupported.isError() && !busy()
                                               && m_state != State::RebootRequired);

    s.insert(QStringLiteral("progress"), m_progress);
    s.insert(QStringLiteral("progressText"), m_progressText);
    s.insert(QStringLiteral("bytesDownloaded"), qulonglong(m_bytesDone));
    s.insert(QStringLiteral("downloadRate"), m_rate);

    s.insert(QStringLiteral("errorCode"), UpdateError::codeName(m_error.code));
    s.insert(QStringLiteral("errorMessage"), m_error.message);
    s.insert(QStringLiteral("errorDetail"), m_error.detail);
    s.insert(QStringLiteral("lastCheck"), m_store.lastCheck().isValid() ? m_store.lastCheck().toSecsSinceEpoch() : qint64(0));
    s.insert(QStringLiteral("lastCheckError"), m_lastCheckError.message);
    s.insert(QStringLiteral("lastSuccessVersion"), m_store.lastSuccessVersion());
    s.insert(QStringLiteral("lastSuccessTime"),
             m_store.lastSuccessTime().isValid() ? m_store.lastSuccessTime().toSecsSinceEpoch() : qint64(0));

    s.insert(QStringLiteral("rebootRequired"), m_state == State::RebootRequired);
    s.insert(QStringLiteral("pendingKind"), pending.kind);
    s.insert(QStringLiteral("pendingVersion"), pending.toVersion);
    return s;
}

QString UpdateService::historyJson() const
{
    return QString::fromUtf8(QJsonDocument(m_store.history()).toJson(QJsonDocument::Compact));
}

void UpdateService::notifyChanged()
{
    if (!m_notifyTimer.isActive())
        m_notifyTimer.start();
}

void UpdateService::setState(State s)
{
    if (s != State::Error)
        m_error = UpdateError();
    if (s != State::Downloading && s != State::Verifying && s != State::Installing && s != State::Confirming) {
        m_progress = -1;
        m_progressText.clear();
        m_rate = 0;
    }
    m_state = s;
    notifyChanged();
}

void UpdateService::setError(const UpdateError &e)
{
    if (m_state != State::Error)
        m_stateBeforeError = m_state;
    m_goal = Goal::None;
    m_error = e;
    m_state = State::Error;
    m_progress = -1;
    m_progressText.clear();
    qWarning("update error: %s: %s (%s)", qPrintable(UpdateError::codeName(e.code)), qPrintable(e.message),
             qPrintable(e.detail));
    notifyChanged();
}

void UpdateService::setProgress(int percent, const QString &text)
{
    m_progress = percent;
    m_progressText = text;
    notifyChanged();
}

void UpdateService::dismissError()
{
    if (m_state != State::Error)
        return;
    m_error = UpdateError();
    // Return to the most useful resting state.
    if (m_store.pending().isValid() && m_store.pending().bootId == bootId())
        setState(State::RebootRequired);
    else if (m_haveUpdate)
        setState(State::UpdateAvailable);
    else if (m_store.lastCheck().isValid())
        setState(State::UpToDate);
    else
        setState(State::Idle);
}

void UpdateService::saveState()
{
    if (!m_store.save())
        qWarning("cannot save updater state: %s", qPrintable(m_store.errorString()));
}

// ---- checking ---------------------------------------------------------------------

UpdateError UpdateService::checkForUpdates(bool userInitiated)
{
    if (busy())
        return UpdateError::make(UpdateError::Busy);
    if (m_state == State::RebootRequired || m_state == State::Confirming)
        return UpdateError();   // nothing to do until the pending change is settled
    if (!m_tlsAvailable && m_config.apiUrl.scheme() == QLatin1String("https")) {
        m_lastCheckError = UpdateError::make(UpdateError::TlsUnavailable);
        if (userInitiated)
            setError(m_lastCheckError);
        return UpdateError();
    }
    m_github->setConfig(m_config);
    setState(State::Checking);
    setProgress(-1, tr("Checking for updates"));
    m_github->findLatestRelease([this, userInitiated](const ReleaseInfo &r, const UpdateError &e) {
        onReleaseFound(r, e, userInitiated);
    });
    return UpdateError();
}

void UpdateService::finishCheck(const UpdateError &error, bool userInitiated)
{
    m_store.setLastCheck(QDateTime::currentDateTimeUtc());
    saveState();
    if (error.isError()) {
        m_lastCheckError = error;
        if (userInitiated) {
            setError(error);
        } else {
            // Background checks fail quietly; the page shows the last result.
            setState(m_haveUpdate ? State::UpdateAvailable : (m_store.lastCheck().isValid() ? State::UpToDate : State::Idle));
        }
        return;
    }
    m_lastCheckError = UpdateError();
    if (m_haveUpdate) {
        setState(State::UpdateAvailable);
        const QString v = m_manifest.version.toString();
        if (m_notifiedVersion != v) {
            m_notifiedVersion = v;
            emit updateAvailable(v, m_release.name);
        }
    } else {
        setState(State::UpToDate);
    }
}

void UpdateService::onReleaseFound(const ReleaseInfo &release, const UpdateError &error, bool userInitiated)
{
    if (error.isError()) {
        finishCheck(error, userInitiated);
        return;
    }
    const Version current = m_system.version.isValid() ? m_system.version : Version::parse(QStringLiteral("0.0.0"));
    if (!release.version.isValid() || (release.version <= current && !m_config.allowDowngrade)
        || release.version == current) {
        // No release yet, or nothing newer than what is running.
        m_haveUpdate = false;
        m_downloaded = false;
        finishCheck(UpdateError(), userInitiated);
        return;
    }
    // Same release as before: keep the download state.
    if (!(m_haveUpdate && m_release.tag == release.tag))
        m_downloaded = false;
    m_release = release;
    const QString v = release.version.toString();
    const ReleaseAsset *manifest = release.asset(QStringLiteral("CustomOS-v%1-manifest.json").arg(v));
    if (!manifest)
        manifest = release.assetWithSuffix(QStringLiteral("-manifest.json"));
    if (!manifest) {
        m_haveUpdate = false;
        finishCheck(UpdateError::make(UpdateError::InvalidRelease, QStringLiteral("no *-manifest.json asset"), release.tag),
                    userInitiated);
        return;
    }
    m_github->fetchSmallAsset(*manifest, 1024 * 1024, [this, userInitiated](const QByteArray &data, const UpdateError &e) {
        onManifest(data, e, userInitiated);
    });
}

void UpdateService::onManifest(const QByteArray &data, const UpdateError &error, bool userInitiated)
{
    m_haveUpdate = false;
    if (error.isError()) {
        finishCheck(error, userInitiated);
        return;
    }
    ReleaseManifest m;
    QString why;
    if (!ReleaseManifest::parse(data, &m, &why)) {
        finishCheck(UpdateError::make(UpdateError::InvalidManifest, why, m_release.tag), userInitiated);
        return;
    }
    if (m.version != m_release.version) {
        finishCheck(UpdateError::make(UpdateError::InvalidManifest,
                                      QStringLiteral("manifest version %1 differs from tag %2").arg(m.version.toString(), m_release.tag),
                                      m_release.tag), userInitiated);
        return;
    }
    if (m.compatible != m_config.compatible) {
        finishCheck(UpdateError::make(UpdateError::Incompatible,
                                      QStringLiteral("release is for \"%1\", this system is \"%2\"").arg(m.compatible, m_config.compatible),
                                      m.version.toString()), userInitiated);
        return;
    }
    if (m.minVersion.isValid() && m_system.version.isValid() && m_system.version < m.minVersion) {
        finishCheck(UpdateError::make(UpdateError::TooOld,
                                      QStringLiteral("requires at least %1").arg(m.minVersion.toString()), m.version.toString()),
                    userInitiated);
        return;
    }
    const ReleaseAsset *bundle = m_release.asset(m.bundleName);
    if (!bundle || bundle->size != m.bundleSize) {
        finishCheck(UpdateError::make(UpdateError::InvalidRelease,
                                      bundle ? QStringLiteral("bundle size %1 differs from manifest %2").arg(bundle->size).arg(m.bundleSize)
                                             : QStringLiteral("bundle %1 not attached").arg(m.bundleName),
                                      m_release.tag), userInitiated);
        return;
    }
    if (!bundle->sha256.isEmpty() && bundle->sha256 != m.bundleSha256) {
        finishCheck(UpdateError::make(UpdateError::InvalidRelease,
                                      QStringLiteral("GitHub asset digest differs from the manifest"), m_release.tag),
                    userInitiated);
        return;
    }
    m_manifest = m;
    const ReleaseAsset *sums = m_release.asset(QStringLiteral("SHA256SUMS"));
    if (!sums) {
        finishCheck(UpdateError::make(UpdateError::InvalidRelease, QStringLiteral("no SHA256SUMS asset"), m_release.tag),
                    userInitiated);
        return;
    }
    m_github->fetchSmallAsset(*sums, 256 * 1024, [this, userInitiated](const QByteArray &d, const UpdateError &e) {
        onChecksums(d, e, userInitiated);
    });
}

void UpdateService::onChecksums(const QByteArray &data, const UpdateError &error, bool userInitiated)
{
    if (error.isError()) {
        finishCheck(error, userInitiated);
        return;
    }
    const Sha256Sums sums = Sha256Sums::parse(data);
    if (sums.value(m_manifest.bundleName) != m_manifest.bundleSha256) {
        finishCheck(UpdateError::make(UpdateError::InvalidManifest,
                                      QStringLiteral("SHA256SUMS does not list %1 with the manifest's checksum").arg(m_manifest.bundleName),
                                      m_release.tag), userInitiated);
        return;
    }
    m_haveUpdate = true;
    cleanCache(m_manifest.bundleName);
    if (QFileInfo(bundlePath()).size() != m_manifest.bundleSize)
        m_downloaded = false;
    finishCheck(UpdateError(), userInitiated);
}

// ---- download and verification ---------------------------------------------------

QString UpdateService::bundlePath() const
{
    return m_config.cacheDirectory + QLatin1Char('/') + m_manifest.bundleName;
}

void UpdateService::cleanCache(const QString &keep)
{
    QDir dir(m_config.cacheDirectory);
    const QStringList files = dir.entryList({QStringLiteral("*.raucb"), QStringLiteral("*.raucb.part")}, QDir::Files);
    for (const QString &f : files) {
        if (!keep.isEmpty() && (f == keep || f == keep + QStringLiteral(".part")))
            continue;
        dir.remove(f);
    }
}

UpdateError UpdateService::download()
{
    if (busy())
        return UpdateError::make(UpdateError::Busy);
    const UpdateError unsupported = supportedForInstall();
    if (unsupported.isError())
        return unsupported;
    if (!m_haveUpdate)
        return UpdateError::make(UpdateError::NothingToInstall);
    m_goal = Goal::Download;
    startDownload();
    return UpdateError();
}

UpdateError UpdateService::install()
{
    if (busy())
        return UpdateError::make(UpdateError::Busy);
    if (m_state == State::RebootRequired)
        return UpdateError();
    const UpdateError unsupported = supportedForInstall();
    if (unsupported.isError())
        return unsupported;
    if (!m_haveUpdate)
        return UpdateError::make(UpdateError::NothingToInstall);
    m_goal = Goal::Install;
    if (m_downloaded)
        startInstall();
    else
        startDownload();
    return UpdateError();
}

void UpdateService::startDownload()
{
    QDir().mkpath(m_config.cacheDirectory);
    const qint64 have = qMax(QFileInfo(bundlePath()).size(), QFileInfo(bundlePath() + QStringLiteral(".part")).size());
    const qint64 needed = m_manifest.bundleSize - qMin(have, m_manifest.bundleSize) + 32 * 1024 * 1024;
    QStorageInfo storage(m_config.cacheDirectory);
    if (storage.isValid() && storage.bytesAvailable() < needed) {
        setError(UpdateError::make(UpdateError::NoSpace,
                                   QStringLiteral("%1 bytes available in %2").arg(storage.bytesAvailable()).arg(m_config.cacheDirectory),
                                   formatBytes(needed)));
        return;
    }
    const ReleaseAsset *asset = m_release.asset(m_manifest.bundleName);
    if (!asset) {
        setError(UpdateError::make(UpdateError::InvalidRelease, QString(), m_release.tag));
        return;
    }
    m_bytesDone = have;
    setState(State::Downloading);
    setProgress(m_manifest.bundleSize > 0 ? int(have * 100 / m_manifest.bundleSize) : 0, tr("Starting download"));
    m_github->setConfig(m_config);
    m_downloader->start(*asset, bundlePath(), m_manifest.bundleSize);
}

void UpdateService::onDownloadFinished(const UpdateError &error)
{
    if (error.code == UpdateError::Cancelled) {
        m_goal = Goal::None;
        setState(State::UpdateAvailable);
        return;
    }
    if (error.isError()) {
        setError(error);
        return;
    }
    verifyBundle();
}

void UpdateService::verifyBundle()
{
    setState(State::Verifying);
    setProgress(-1, tr("Checking the download"));
    const QString path = bundlePath();
    QPointer<UpdateService> self(this);
    QThread *t = QThread::create([self, path] {
        QByteArray result;
        QFile f(path);
        if (f.open(QIODevice::ReadOnly)) {
            QCryptographicHash h(QCryptographicHash::Sha256);
            if (h.addData(&f))
                result = h.result().toHex();
        }
        QMetaObject::invokeMethod(self, [self, result] {
            if (self)
                self->onHashed(result);
        }, Qt::QueuedConnection);
    });
    connect(t, &QThread::finished, t, &QObject::deleteLater);
    t->start(QThread::LowPriority);
}

void UpdateService::onHashed(const QByteArray &sha256)
{
    if (sha256 != m_manifest.bundleSha256) {
        QFile::remove(bundlePath());
        m_downloaded = false;
        setError(UpdateError::make(UpdateError::ChecksumMismatch,
                                   QStringLiteral("expected %1, got %2").arg(QString::fromLatin1(m_manifest.bundleSha256),
                                                                             QString::fromLatin1(sha256))));
        return;
    }
    if (!m_rauc->isAvailable()) {
        setError(UpdateError::make(UpdateError::RaucUnavailable));
        return;
    }
    setProgress(-1, tr("Checking the signature"));
    m_rauc->inspect(bundlePath(), [this](const RaucClient::BundleInfo &info, const QString &error) {
        onInspected(info, error);
    });
}

UpdateError UpdateService::classifyRaucError(const QString &message, UpdateError::Code fallback) const
{
    const QString m = message.toLower();
    if (m.contains(QLatin1String("signature")) || m.contains(QLatin1String("certificate"))
        || m.contains(QLatin1String("verif")) || m.contains(QLatin1String("keyring")) || m.contains(QLatin1String("cms")))
        return UpdateError::make(UpdateError::SignatureInvalid, message);
    if (m.contains(QLatin1String("compatible")))
        return UpdateError::make(UpdateError::Incompatible, message, m_manifest.version.toString());
    if (m.contains(QLatin1String("no space")) || m.contains(QLatin1String("too large")))
        return UpdateError::make(UpdateError::InstallFailed, message);
    return UpdateError::make(fallback, message);
}

void UpdateService::onInspected(const RaucClient::BundleInfo &info, const QString &error)
{
    if (!error.isEmpty()) {
        const UpdateError e = classifyRaucError(error, UpdateError::SignatureInvalid);
        if (e.code == UpdateError::SignatureInvalid) {
            // An untrusted bundle is never kept around.
            QFile::remove(bundlePath());
            m_downloaded = false;
        }
        setError(e);
        return;
    }
    // The values below come from the signed manifest inside the bundle.
    const QString systemCompatible = m_rauc->compatible().isEmpty() ? m_config.compatible : m_rauc->compatible();
    if (info.compatible != systemCompatible) {
        setError(UpdateError::make(UpdateError::Incompatible,
                                   QStringLiteral("bundle compatible \"%1\", system \"%2\"").arg(info.compatible, systemCompatible),
                                   info.version));
        return;
    }
    const Version bundleVersion = Version::parse(info.version);
    if (!bundleVersion.isValid() || bundleVersion != m_manifest.version) {
        setError(UpdateError::make(UpdateError::VersionMismatch,
                                   QStringLiteral("signed version \"%1\", release %2").arg(info.version, m_manifest.version.toString())));
        return;
    }
    if (m_system.version.isValid() && bundleVersion <= m_system.version && !m_config.allowDowngrade) {
        setError(UpdateError::make(UpdateError::Downgrade,
                                   QStringLiteral("bundle %1, running %2").arg(info.version, m_system.versionText), info.version));
        return;
    }
    m_downloaded = true;
    if (m_goal == Goal::Install) {
        startInstall();
    } else {
        m_goal = Goal::None;
        setState(State::UpdateAvailable);
    }
}

// ---- installation ---------------------------------------------------------------------

void UpdateService::startInstall()
{
    if (!QFileInfo::exists(bundlePath())) {
        m_downloaded = false;
        startDownload();
        return;
    }
    if (!m_rauc->isAvailable()) {
        setError(UpdateError::make(UpdateError::RaucUnavailable));
        return;
    }
    setState(State::Installing);
    setProgress(0, tr("Preparing installation"));
    m_rauc->install(bundlePath(), [this](const QString &error) {
        if (!error.isEmpty())
            setError(classifyRaucError(error, UpdateError::InstallFailed));
    });
}

void UpdateService::onInstallCompleted(int result)
{
    if (m_state != State::Installing)
        return;
    m_goal = Goal::None;
    if (result != 0) {
        const QString detail = m_rauc->lastError();
        m_store.addHistory(QStringLiteral("install"), QStringLiteral("failed"), m_system.versionText,
                           m_manifest.version.toString(), detail);
        saveState();
        setError(classifyRaucError(detail, UpdateError::InstallFailed));
        return;
    }
    PendingChange p;
    p.kind = QStringLiteral("update");
    p.fromVersion = m_system.versionText;
    p.toVersion = m_manifest.version.toString();
    p.fromSlot = m_system.bootedSlot;
    p.targetSlot = m_system.bootedSlot == QLatin1String("A") ? QStringLiteral("B") : QStringLiteral("A");
    p.bootId = bootId();
    p.when = QDateTime::currentDateTimeUtc();
    m_store.setPending(p);
    m_store.addHistory(QStringLiteral("install"), QStringLiteral("pending"), p.fromVersion, p.toVersion,
                       tr("Installed into slot %1; restart to finish").arg(p.targetSlot));
    saveState();
    QFile::remove(bundlePath());   // the image now lives in the inactive slot
    m_downloaded = false;
    setState(State::RebootRequired);
    refreshSlots();
}

UpdateError UpdateService::cancel()
{
    if (m_state == State::Downloading) {
        m_downloader->cancel();
        return UpdateError();
    }
    if (m_state == State::Installing)
        return UpdateError::make(UpdateError::Busy, QStringLiteral("installation can't be interrupted"));
    return UpdateError();
}

UpdateError UpdateService::reboot()
{
    if (busy())
        return UpdateError::make(UpdateError::Busy);
    // logind is the normal path; systemctl is the fallback.
    QDBusMessage m = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.login1"),
                                                    QStringLiteral("/org/freedesktop/login1"),
                                                    QStringLiteral("org.freedesktop.login1.Manager"),
                                                    QStringLiteral("Reboot"));
    m << false;
    const QDBusMessage reply = m_bus.call(m, QDBus::Block, 10000);
    if (reply.type() == QDBusMessage::ReplyMessage)
        return UpdateError();
    if (QProcess::startDetached(QStringLiteral("systemctl"), {QStringLiteral("reboot")}))
        return UpdateError();
    return UpdateError::make(UpdateError::RebootFailed, reply.errorMessage());
}

UpdateError UpdateService::rollback()
{
    if (busy())
        return UpdateError::make(UpdateError::Busy);
    if (m_system.mode != SystemState::Mode::AB)
        return supportedForInstall();
    if (!m_otherSlotGood || m_store.pending().isValid())
        return UpdateError::make(UpdateError::RollbackUnavailable);
    if (!m_rauc->isAvailable())
        return UpdateError::make(UpdateError::RaucUnavailable);
    m_rauc->mark(QStringLiteral("active"), QStringLiteral("other"),
                 [this](const QString &slotName, const QString &message, const QString &error) {
        if (!error.isEmpty()) {
            setError(UpdateError::make(UpdateError::InstallFailed, error));
            return;
        }
        PendingChange p;
        p.kind = QStringLiteral("rollback");
        p.fromVersion = m_system.versionText;
        p.toVersion = m_otherSlotVersion;
        p.fromSlot = m_system.bootedSlot;
        p.targetSlot = m_otherSlotBootname;
        p.bootId = bootId();
        p.when = QDateTime::currentDateTimeUtc();
        m_store.setPending(p);
        m_store.addHistory(QStringLiteral("rollback"), QStringLiteral("pending"), p.fromVersion, p.toVersion,
                           tr("Previous version selected (%1: %2); restart to finish").arg(slotName, message));
        saveState();
        setState(State::RebootRequired);
        refreshSlots();
    });
    return UpdateError();
}

// ---- after a reboot ------------------------------------------------------------------

void UpdateService::reconcileAfterBoot()
{
    const PendingChange p = m_store.pending();
    if (!p.isValid())
        return;
    if (!p.bootId.isEmpty() && p.bootId == bootId()) {
        setState(State::RebootRequired);   // installed during this boot; not restarted yet
        return;
    }
    const QString booted = m_system.bootedSlot;
    if (booted == p.targetSlot && (p.kind != QLatin1String("update") || p.toVersion.isEmpty()
                                   || Version::parse(p.toVersion) == m_system.version)) {
        // Running the new slot: success once the health check confirms it.
        m_healthPolls = 0;
        setState(State::Confirming);
        setProgress(-1, tr("Making sure CustomOS %1 started correctly").arg(m_system.versionText));
        pollHealth();
        if (m_state == State::Confirming)
            m_healthTimer.start();
        return;
    }
    // Back on the old slot: the new one did not boot or failed its health check.
    // The trial verdict is kept apart from last-boot.env, which this boot's
    // health check may already have replaced. A verdict about an earlier
    // installation into the same slot doesn't count.
    BootReport r = m_store.lastTrialReport();
    if (!r.isValid())
        r = m_store.lastBootReport();
    // (The report's version may carry "+<build id>".)
    const QString reportVersion = r.version.section(QLatin1Char('+'), 0, 0);
    const bool aboutThisChange = r.isValid() && r.slot == p.targetSlot && r.result == QLatin1String("bad")
                                 && (!p.when.isValid() || !r.when.isValid() || r.when >= p.when)
                                 && (p.toVersion.isEmpty() || reportVersion.isEmpty()
                                     || reportVersion == p.toVersion.section(QLatin1Char('+'), 0, 0));
    const QString reason = aboutThisChange ? r.reason : tr("slot %1 did not finish starting").arg(p.targetSlot);
    m_store.resolveHistory(p.kind, QStringLiteral("rolled-back"), p.fromVersion, p.toVersion, reason);
    m_store.clearPending();
    saveState();
    setError(UpdateError::make(UpdateError::RolledBack, reason, p.toVersion));
}

void UpdateService::pollHealth()
{
    const PendingChange p = m_store.pending();
    const BootReport r = m_store.lastBootReport();
    const bool thisBoot = r.isValid() && r.slot == m_system.bootedSlot && r.when.isValid()
                          && r.when >= bootTime().addSecs(-120);
    if (thisBoot && r.result == QLatin1String("good")) {
        m_healthTimer.stop();
        m_store.resolveHistory(p.kind, QStringLiteral("success"), p.fromVersion, p.toVersion.isEmpty() ? m_system.versionText : p.toVersion,
                           p.kind == QLatin1String("rollback") ? tr("Returned to the previous version")
                                                               : tr("Updated and started successfully"));
        if (p.kind == QLatin1String("update"))
            m_store.setLastSuccess(m_system.versionText, QDateTime::currentDateTimeUtc());
        m_store.clearPending();
        saveState();
        m_haveUpdate = false;
        setState(State::UpToDate);
        refreshSlots();
        return;
    }
    if (thisBoot && r.result == QLatin1String("bad")) {
        // customos-health-check restarts into the previous slot by itself;
        // the outcome is recorded on that boot.
        m_healthTimer.stop();
        setProgress(-1, tr("CustomOS %1 failed its start-up checks and is going back to the previous version").arg(m_system.versionText));
        return;
    }
    if (++m_healthPolls > 360) {   // 30 minutes without a verdict
        m_healthTimer.stop();
        setError(UpdateError::make(UpdateError::Internal, QStringLiteral("the health check did not report a result")));
    }
}

void UpdateService::refreshSlots()
{
    if (m_system.mode != SystemState::Mode::AB || !m_rauc->isAvailable())
        return;
    m_rauc->slotStatus([this](const QList<RaucClient::Slot> &slotList, const QString &error) {
        if (!error.isEmpty())
            return;
        m_otherSlotGood = false;
        m_otherSlotVersion.clear();
        m_otherSlotBootname.clear();
        for (const RaucClient::Slot &s : slotList) {
            if (s.bootname.isEmpty() || s.bootname == m_system.bootedSlot)
                continue;
            m_otherSlotBootname = s.bootname;
            m_otherSlotVersion = s.bundleVersion;
            m_otherSlotGood = s.bootStatus == QLatin1String("good");
        }
        notifyChanged();
    });
}
