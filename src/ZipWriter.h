// SPDX-License-Identifier: MIT
#pragma once
#include <QByteArray>
#include <QString>
#include <QVector>

// Écrivain d'archives ZIP minimal (suffisant pour les conteneurs OOXML / .docx).
// Compression « deflate » via zlib de Qt (qCompress), ou stockage brut si la compression n'apporte rien.
class ZipWriter
{
public:
    void addFile(const QString &name, const QByteArray &data, bool compress = true);
    bool writeTo(const QString &path, QString *error = nullptr) const;
    QByteArray toByteArray() const;

private:
    struct Entry {
        QString name;
        QByteArray stored;      // données écrites (compressées ou non)
        quint32 crc = 0;
        quint32 size = 0;       // taille non compressée
        quint16 method = 0;     // 0 = stocké, 8 = deflate
    };
    QVector<Entry> entries_;
};
