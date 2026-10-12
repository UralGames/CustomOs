#include "documentio.h"

#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QStringDecoder>

QVariantMap DocumentIO::load(const QString &path) const
{
    QVariantMap r{{QStringLiteral("ok"), false}, {QStringLiteral("path"), QFileInfo(path).absoluteFilePath()}};
    QFile f(path);
    if (!f.exists()) {
        // Opening a new, not yet existing file is fine: it is created on save.
        r[QStringLiteral("ok")] = true;
        r[QStringLiteral("text")] = QString();
        r[QStringLiteral("encoding")] = QStringLiteral("UTF-8");
        r[QStringLiteral("lineEnding")] = QStringLiteral("LF");
        r[QStringLiteral("isNew")] = true;
        return r;
    }
    if (QFileInfo(path).isDir()) {
        r[QStringLiteral("error")] = tr("\"%1\" is a folder.").arg(path);
        return r;
    }
    if (f.size() > 64LL * 1024 * 1024) {
        r[QStringLiteral("error")] = tr("The file is too large to open in the text editor (over 64 MB).");
        return r;
    }
    if (!f.open(QIODevice::ReadOnly)) {
        r[QStringLiteral("error")] = tr("Could not open the file: %1").arg(f.errorString());
        return r;
    }
    const QByteArray data = f.readAll();
    if (data.left(8192).contains('\0')) {
        r[QStringLiteral("error")] = tr("This looks like a binary file, not text, so it can't be edited here.");
        return r;
    }
    QString text;
    QString encoding = QStringLiteral("UTF-8");
    QStringDecoder utf8(QStringDecoder::Utf8, QStringDecoder::Flag::Stateless);
    text = utf8.decode(data);
    if (utf8.hasError()) {
        // Not valid UTF-8: fall back to Latin-1 so nothing is lost.
        text = QString::fromLatin1(data);
        encoding = QStringLiteral("ISO-8859-1");
    }
    if (text.startsWith(QChar(0xFEFF)))
        text.remove(0, 1);
    const bool crlf = text.contains(QLatin1String("\r\n"));
    if (crlf)
        text.replace(QLatin1String("\r\n"), QLatin1String("\n"));
    r[QStringLiteral("ok")] = true;
    r[QStringLiteral("text")] = text;
    r[QStringLiteral("encoding")] = encoding;
    r[QStringLiteral("lineEnding")] = crlf ? QStringLiteral("CRLF") : QStringLiteral("LF");
    r[QStringLiteral("readOnly")] = !QFileInfo(path).isWritable();
    return r;
}

QString DocumentIO::save(const QString &path, const QString &text, const QString &lineEnding) const
{
    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly))
        return tr("Could not save: %1").arg(f.errorString());
    QString out = text;
    if (lineEnding == QLatin1String("CRLF"))
        out.replace(QLatin1Char('\n'), QLatin1String("\r\n"));
    f.write(out.toUtf8());
    if (!f.commit())
        return tr("Could not save: %1").arg(f.errorString());
    return {};
}

bool DocumentIO::exists(const QString &path) const
{
    return QFileInfo::exists(path);
}

QString DocumentIO::absolutePath(const QString &path) const
{
    return QFileInfo(path).absoluteFilePath();
}
