// SPDX-License-Identifier: MIT
#include "ZipWriter.h"

#include <QDate>
#include <QDateTime>
#include <QSaveFile>
#include <QTime>

namespace {

quint32 crc32(const QByteArray &data)
{
    static quint32 table[256];
    static bool init = false;
    if (!init) {
        for (quint32 i = 0; i < 256; ++i) {
            quint32 c = i;
            for (int k = 0; k < 8; ++k) c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
            table[i] = c;
        }
        init = true;
    }
    quint32 crc = 0xFFFFFFFFu;
    for (char ch : data) crc = table[(crc ^ static_cast<quint8>(ch)) & 0xFF] ^ (crc >> 8);
    return crc ^ 0xFFFFFFFFu;
}

void put16(QByteArray &b, quint16 v)
{
    b.append(char(v & 0xFF));
    b.append(char((v >> 8) & 0xFF));
}

void put32(QByteArray &b, quint32 v)
{
    for (int i = 0; i < 4; ++i) b.append(char((v >> (8 * i)) & 0xFF));
}

} // namespace

void ZipWriter::addFile(const QString &name, const QByteArray &data, bool compress)
{
    Entry e;
    e.name = name;
    e.crc = crc32(data);
    e.size = static_cast<quint32>(data.size());
    e.stored = data;
    e.method = 0;
    if (compress && data.size() > 64) {
        // qCompress = 4 octets de taille + flux zlib (2 octets d'en-tête, deflate, 4 octets Adler-32).
        // Le « deflate brut » attendu par ZIP s'obtient en retirant ces octets.
        const QByteArray z = qCompress(data, 9);
        if (z.size() > 10) {
            const QByteArray raw = z.mid(6, z.size() - 10);
            if (raw.size() < data.size()) {
                e.stored = raw;
                e.method = 8;
            }
        }
    }
    entries_.append(e);
}

QByteArray ZipWriter::toByteArray() const
{
    const QDateTime now = QDateTime::currentDateTime();
    const QDate d = now.date();
    const QTime t = now.time();
    const quint16 dosTime = quint16((t.hour() << 11) | (t.minute() << 5) | (t.second() / 2));
    const quint16 dosDate = quint16(((qMax(1980, d.year()) - 1980) << 9) | (d.month() << 5) | d.day());

    QByteArray out, central;
    for (const Entry &e : entries_) {
        const QByteArray name = e.name.toUtf8();
        const quint32 offset = static_cast<quint32>(out.size());

        put32(out, 0x04034b50u);
        put16(out, 20);          // version nécessaire
        put16(out, 0x0800);      // noms en UTF-8
        put16(out, e.method);
        put16(out, dosTime);
        put16(out, dosDate);
        put32(out, e.crc);
        put32(out, static_cast<quint32>(e.stored.size()));
        put32(out, e.size);
        put16(out, static_cast<quint16>(name.size()));
        put16(out, 0);
        out.append(name);
        out.append(e.stored);

        put32(central, 0x02014b50u);
        put16(central, 20);      // version créatrice
        put16(central, 20);
        put16(central, 0x0800);
        put16(central, e.method);
        put16(central, dosTime);
        put16(central, dosDate);
        put32(central, e.crc);
        put32(central, static_cast<quint32>(e.stored.size()));
        put32(central, e.size);
        put16(central, static_cast<quint16>(name.size()));
        put16(central, 0);       // extra
        put16(central, 0);       // commentaire
        put16(central, 0);       // disque
        put16(central, 0);       // attributs internes
        put32(central, 0);       // attributs externes
        put32(central, offset);
        central.append(name);
    }
    const quint32 cdOffset = static_cast<quint32>(out.size());
    out.append(central);

    put32(out, 0x06054b50u);
    put16(out, 0);
    put16(out, 0);
    put16(out, static_cast<quint16>(entries_.size()));
    put16(out, static_cast<quint16>(entries_.size()));
    put32(out, static_cast<quint32>(central.size()));
    put32(out, cdOffset);
    put16(out, 0);
    return out;
}

bool ZipWriter::writeTo(const QString &path, QString *error) const
{
    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly)) {
        if (error) *error = f.errorString();
        return false;
    }
    f.write(toByteArray());
    if (!f.commit()) {
        if (error) *error = f.errorString();
        return false;
    }
    return true;
}
