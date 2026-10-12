#pragma once

#include <QObject>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

// Loading and saving text files for the editor.
class DocumentIO : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(DocumentIO)
    QML_SINGLETON

public:
    explicit DocumentIO(QObject *parent = nullptr) : QObject(parent) {}

    // { ok, text, error, encoding, lineEnding, path }
    Q_INVOKABLE QVariantMap load(const QString &path) const;
    // Returns "" on success or an error message. lineEnding: "LF" or "CRLF".
    Q_INVOKABLE QString save(const QString &path, const QString &text, const QString &lineEnding = QStringLiteral("LF")) const;
    Q_INVOKABLE bool exists(const QString &path) const;
    Q_INVOKABLE QString absolutePath(const QString &path) const;
};
