#pragma once

#include "version.h"

#include <QByteArray>
#include <QDateTime>
#include <QHash>
#include <QString>

// CustomOS-vX.Y.Z-manifest.json: the release description published next to
// the RAUC bundle. It is informational (size, checksum, changelog) and is
// cross-checked against SHA256SUMS and against the *signed* manifest inside
// the bundle before anything is installed; authenticity comes from the RAUC
// signature, never from this file alone.
//
//   {
//     "schema": 1,
//     "product": "CustomOS",
//     "version": "0.2.0",
//     "channel": "stable",
//     "compatible": "customos-x86_64-ab",
//     "released": "2026-10-12T09:00:00Z",
//     "min_version": "0.2.0",              (optional: oldest version that may update to this)
//     "reboot_required": true,
//     "bundle": { "name": "CustomOS-v0.2.0.raucb", "size": 123, "sha256": "<64 hex>" },
//     "changelog": "Markdown text",
//     "build": { "id": "...", "git_commit": "...", "buildroot": "2026.02.3" }
//   }
struct ReleaseManifest
{
    int schema = 0;
    QString product;
    Version version;
    QString channel;
    QString compatible;
    QDateTime released;
    Version minVersion;          // invalid if not given
    bool rebootRequired = true;
    QString bundleName;
    qint64 bundleSize = 0;
    QByteArray bundleSha256;     // lower-case hex
    QString changelog;
    QString buildId;
    QString gitCommit;

    // Parses and validates the JSON. On failure returns false and sets *error
    // to a short English explanation.
    static bool parse(const QByteArray &json, ReleaseManifest *out, QString *error);
};

// A SHA256SUMS file as written by sha256sum(1): "<hex>  <name>" or "<hex> *<name>".
class Sha256Sums
{
public:
    static Sha256Sums parse(const QByteArray &text);
    bool contains(const QString &name) const { return m_sums.contains(name); }
    QByteArray value(const QString &name) const { return m_sums.value(name); }
    int size() const { return int(m_sums.size()); }

private:
    QHash<QString, QByteArray> m_sums;
};

bool isSha256Hex(const QByteArray &hex);
