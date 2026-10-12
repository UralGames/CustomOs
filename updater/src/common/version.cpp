#include "version.h"

#include <QRegularExpression>

Version Version::parse(const QString &input)
{
    // Official SemVer 2.0.0 grammar (numeric identifiers without leading zeros).
    static const QRegularExpression re(QStringLiteral(
        "^v?(0|[1-9]\\d*)\\.(0|[1-9]\\d*)\\.(0|[1-9]\\d*)"
        "(?:-((?:0|[1-9]\\d*|\\d*[a-zA-Z-][0-9a-zA-Z-]*)(?:\\.(?:0|[1-9]\\d*|\\d*[a-zA-Z-][0-9a-zA-Z-]*))*))?"
        "(?:\\+([0-9a-zA-Z-]+(?:\\.[0-9a-zA-Z-]+)*))?$"));
    Version v;
    const QRegularExpressionMatch m = re.match(input.trimmed());
    if (!m.hasMatch())
        return v;
    bool ok1 = false, ok2 = false, ok3 = false;
    v.m_major = m.captured(1).toInt(&ok1);
    v.m_minor = m.captured(2).toInt(&ok2);
    v.m_patch = m.captured(3).toInt(&ok3);
    if (!ok1 || !ok2 || !ok3)
        return Version();   // out of int range
    if (!m.captured(4).isEmpty())
        v.m_prerelease = m.captured(4).split(QLatin1Char('.'));
    v.m_build = m.captured(5);
    v.m_valid = true;
    return v;
}

QString Version::toString() const
{
    if (!m_valid)
        return QString();
    QString s = QStringLiteral("%1.%2.%3").arg(m_major).arg(m_minor).arg(m_patch);
    if (!m_prerelease.isEmpty())
        s += QLatin1Char('-') + m_prerelease.join(QLatin1Char('.'));
    if (!m_build.isEmpty())
        s += QLatin1Char('+') + m_build;
    return s;
}

static bool isNumeric(const QString &s, qulonglong *value)
{
    bool ok = false;
    *value = s.toULongLong(&ok);
    return ok && !s.isEmpty();
}

int Version::compare(const Version &a, const Version &b)
{
    // Invalid versions sort before every valid one, and equal to each other.
    if (!a.m_valid || !b.m_valid)
        return int(a.m_valid) - int(b.m_valid);
    if (a.m_major != b.m_major) return a.m_major < b.m_major ? -1 : 1;
    if (a.m_minor != b.m_minor) return a.m_minor < b.m_minor ? -1 : 1;
    if (a.m_patch != b.m_patch) return a.m_patch < b.m_patch ? -1 : 1;
    // A release has higher precedence than any of its pre-releases.
    if (a.m_prerelease.isEmpty() || b.m_prerelease.isEmpty())
        return int(a.m_prerelease.isEmpty()) - int(b.m_prerelease.isEmpty());
    const int n = qMin(a.m_prerelease.size(), b.m_prerelease.size());
    for (int i = 0; i < n; ++i) {
        const QString &x = a.m_prerelease.at(i);
        const QString &y = b.m_prerelease.at(i);
        qulonglong nx = 0, ny = 0;
        const bool xn = isNumeric(x, &nx);
        const bool yn = isNumeric(y, &ny);
        if (xn && yn) {
            if (nx != ny) return nx < ny ? -1 : 1;
        } else if (xn != yn) {
            return xn ? -1 : 1;   // numeric identifiers sort before alphanumeric ones
        } else {
            const int c = QString::compare(x, y, Qt::CaseSensitive);   // ASCII order
            if (c != 0) return c < 0 ? -1 : 1;
        }
    }
    if (a.m_prerelease.size() != b.m_prerelease.size())
        return a.m_prerelease.size() < b.m_prerelease.size() ? -1 : 1;
    return 0;
}
