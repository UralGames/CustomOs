#pragma once

#include <QString>
#include <QStringList>

// A Semantic Versioning 2.0.0 version (MAJOR.MINOR.PATCH[-PRERELEASE][+BUILD]).
// A leading "v" (as in Git tags such as "v0.2.0") is accepted and ignored.
// Ordering follows the SemVer precedence rules: build metadata is ignored and
// a pre-release sorts before the corresponding release (0.2.0-rc.1 < 0.2.0).
class Version
{
public:
    Version() = default;

    static Version parse(const QString &text);

    bool isValid() const { return m_valid; }
    bool isPrerelease() const { return !m_prerelease.isEmpty(); }
    int majorVersion() const { return m_major; }
    int minorVersion() const { return m_minor; }
    int patchVersion() const { return m_patch; }
    // Normalised form without a leading "v": "0.2.0-rc.1+build.5".
    QString toString() const;

    // <0, 0, >0 like strcmp.
    static int compare(const Version &a, const Version &b);

    friend bool operator<(const Version &a, const Version &b) { return compare(a, b) < 0; }
    friend bool operator>(const Version &a, const Version &b) { return compare(a, b) > 0; }
    friend bool operator==(const Version &a, const Version &b) { return compare(a, b) == 0; }
    friend bool operator!=(const Version &a, const Version &b) { return compare(a, b) != 0; }
    friend bool operator<=(const Version &a, const Version &b) { return compare(a, b) <= 0; }
    friend bool operator>=(const Version &a, const Version &b) { return compare(a, b) >= 0; }

private:
    bool m_valid = false;
    int m_major = 0;
    int m_minor = 0;
    int m_patch = 0;
    QStringList m_prerelease;
    QString m_build;
};
