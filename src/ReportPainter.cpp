// SPDX-License-Identifier: MIT
#include "ReportPainter.h"

#include <algorithm>
#include <QFontMetricsF>
#include <QPen>

namespace ReportPainter {
namespace {

const QColor kNavy("#1d3557");
const QColor kTeal("#009e8e");
const QColor kGray("#555555");

QFont font(double mm, double pxPerMm, bool bold = false, bool italic = false)
{
    QFont f;
    f.setFamilies({QStringLiteral("Segoe UI"), QStringLiteral("Helvetica"), QStringLiteral("Arial"), QStringLiteral("Sans Serif")});
    f.setPixelSize(std::max(4, int(mm * pxPerMm)));
    f.setBold(bold);
    f.setItalic(italic);
    return f;
}

void setupPainter(QPainter &p)
{
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);
}

} // namespace

void paintCover(QPainter &p, const QRectF &pg, const ReportAssembler::CoverInfo &c, double mm)
{
    p.save();
    setupPainter(p);
    p.fillRect(pg, Qt::white);

    // Bandeau vertical à gauche
    p.fillRect(QRectF(pg.left(), pg.top(), 14 * mm, pg.height()), kNavy);
    p.fillRect(QRectF(pg.left() + 14 * mm, pg.top(), 2 * mm, pg.height()), kTeal);

    const double x0 = pg.left() + 28 * mm;
    const double w = pg.width() - 28 * mm - 18 * mm;
    double y = pg.top() + 16 * mm;

    // Logo
    if (!c.logo.isNull()) {
        double lw = std::max(5.0, c.logoWidthMm) * mm;
        double lh = lw * c.logo.height() / double(c.logo.width());
        const double maxH = 45 * mm;
        if (lh > maxH) {
            lh = maxH;
            lw = lh * c.logo.width() / double(c.logo.height());
        }
        lw = std::min(lw, w);
        double lx = x0;
        if (c.logoAlign == 1) lx = x0 + (w - lw) / 2;
        else if (c.logoAlign == 2) lx = x0 + w - lw;
        p.drawImage(QRectF(lx, y, lw, lh), c.logo);
    }

    // Titre
    const double ty = pg.top() + pg.height() * 0.38;
    p.setFont(font(11, mm, true));
    p.setPen(kNavy);
    const QRectF tb = p.boundingRect(QRectF(x0, ty, w, pg.height() * 0.3), Qt::AlignLeft | Qt::TextWordWrap, c.title);
    p.drawText(QRectF(x0, ty, w, tb.height()), Qt::AlignLeft | Qt::TextWordWrap, c.title);
    double yy = ty + tb.height() + 5 * mm;

    p.fillRect(QRectF(x0, yy, 45 * mm, 1.6 * mm), kTeal);
    yy += 7 * mm;

    if (!c.subtitle.trimmed().isEmpty()) {
        p.setFont(font(6, mm));
        p.setPen(kGray);
        const QRectF sb = p.boundingRect(QRectF(x0, yy, w, pg.height() * 0.2), Qt::AlignLeft | Qt::TextWordWrap, c.subtitle);
        p.drawText(QRectF(x0, yy, w, sb.height()), Qt::AlignLeft | Qt::TextWordWrap, c.subtitle);
    }

    // Informations en bas à gauche
    QVector<QPair<QString, QString>> meta;
    auto add = [&](const QString &k, const QString &v) { if (!v.trimmed().isEmpty()) meta.append({k, v}); };
    add(QStringLiteral("Organisation"), c.organisation);
    add(QStringLiteral("Version"), c.version);
    add(QStringLiteral("Date"), c.date);
    add(QStringLiteral("Auteur"), c.author);
    add(QStringLiteral("Responsable métier"), c.owner);
    const double rowH = 6.6 * mm;
    double my = pg.bottom() - 16 * mm - rowH * meta.size();
    for (const auto &kv : meta) {
        p.setFont(font(4, mm, true));
        p.setPen(kNavy);
        p.drawText(QRectF(x0, my, 48 * mm, rowH), Qt::AlignLeft | Qt::AlignVCenter, kv.first);
        p.setFont(font(4, mm));
        p.setPen(Qt::black);
        p.drawText(QRectF(x0 + 50 * mm, my, w - 50 * mm, rowH), Qt::AlignLeft | Qt::AlignVCenter, kv.second);
        my += rowH;
    }

    // Classification (bas à droite, encadrée)
    if (!c.classification.trimmed().isEmpty()) {
        p.setFont(font(4.6, mm, true));
        const QFontMetricsF fm(p.font());
        const double bw = fm.horizontalAdvance(c.classification) + 10 * mm, bh = 10 * mm;
        const QRectF box(pg.right() - 18 * mm - bw, pg.bottom() - 16 * mm - bh, bw, bh);
        p.setPen(QPen(QColor("#b03a2e"), 0.5 * mm));
        p.setBrush(Qt::NoBrush);
        p.drawRect(box);
        p.setPen(QColor("#b03a2e"));
        p.drawText(box, Qt::AlignCenter, c.classification);
    }
    p.restore();
}

int tocCapacity(const QRectF &pg, double mm)
{
    const double usable = pg.height() - 36 * mm - 14 * mm;   // titre en haut, pied de page en bas
    return std::max(5, int(usable / (5.4 * mm)));
}

void paintToc(QPainter &p, const QRectF &pg, double mm, int pageIndex, int pageCount,
              const QVector<TocEntry> &toc, const QVector<TocEntry> &figs)
{
    p.save();
    setupPainter(p);
    p.fillRect(pg, Qt::white);

    const double margin = 20 * mm, gap = 14 * mm;
    const double colW = (pg.width() - 2 * margin - gap) / 2.0;
    const double xL = pg.left() + margin, xR = xL + colW + gap;
    const double top = pg.top() + 36 * mm, rowH = 5.4 * mm;
    const int cap = tocCapacity(pg, mm);

    auto title = [&](double x, const QString &t) {
        p.setFont(font(8, mm, true));
        p.setPen(kNavy);
        p.drawText(QRectF(x, pg.top() + 14 * mm, colW, 12 * mm), Qt::AlignLeft | Qt::AlignVCenter,
                   pageIndex > 0 ? t + QStringLiteral(" (suite)") : t);
        p.fillRect(QRectF(x, pg.top() + 27 * mm, colW, 0.5 * mm), kTeal);
    };

    auto column = [&](double x, const QVector<TocEntry> &entries) {
        const int from = pageIndex * cap;
        for (int i = from; i < std::min<int>(entries.size(), from + cap); ++i) {
            const TocEntry &e = entries[i];
            const double y = top + (i - from) * rowH;
            const double indent = e.level == 3 ? 6 * mm : 0;
            p.setFont(font(e.level == 3 ? 3.6 : 3.9, mm, e.level == 2));
            p.setPen(e.level == 3 ? QColor("#333333") : kNavy);
            const QFontMetricsF fm(p.font());
            const QString num = QString::number(e.page);
            const double numW = fm.horizontalAdvance(num) + 2 * mm;
            const QString txt = fm.elidedText(e.text, Qt::ElideRight, colW - indent - numW - 6 * mm);
            p.drawText(QRectF(x + indent, y, colW - indent, rowH), Qt::AlignLeft | Qt::AlignVCenter, txt);
            const double tw = fm.horizontalAdvance(txt);
            const double dx0 = x + indent + tw + 2 * mm, dx1 = x + colW - numW - 1 * mm;
            if (dx1 > dx0) {
                p.setPen(QPen(QColor("#999999"), 0.25 * mm, Qt::DotLine));
                p.drawLine(QPointF(dx0, y + rowH * 0.78), QPointF(dx1, y + rowH * 0.78));
            }
            p.setPen(e.level == 3 ? QColor("#333333") : kNavy);
            p.drawText(QRectF(x + colW - numW, y, numW, rowH), Qt::AlignRight | Qt::AlignVCenter, num);
        }
    };

    title(xL, QStringLiteral("Sommaire"));
    column(xL, toc);
    if (!figs.isEmpty()) {
        title(xR, QStringLiteral("Liste des figures"));
        column(xR, figs);
    }
    Q_UNUSED(pageCount)
    p.restore();
}

void paintFooter(QPainter &p, const QRectF &pg, double mm, double footerMm, const QString &left, int pageNo, int total)
{
    p.save();
    setupPainter(p);
    const double y = pg.bottom() - footerMm * mm;
    p.setPen(QPen(QColor("#bbbbbb"), 0.25 * mm));
    p.drawLine(QPointF(pg.left(), y + 1.5 * mm), QPointF(pg.right(), y + 1.5 * mm));
    p.setFont(font(3.2, mm));
    p.setPen(kGray);
    const QRectF r(pg.left(), y + 2 * mm, pg.width(), (footerMm - 2) * mm);
    const QFontMetricsF fm(p.font());
    p.drawText(r, Qt::AlignLeft | Qt::AlignVCenter, fm.elidedText(left, Qt::ElideRight, pg.width() * 0.7));
    p.drawText(r, Qt::AlignRight | Qt::AlignVCenter, QStringLiteral("Page %1 / %2").arg(pageNo).arg(total));
    p.restore();
}

QImage coverPreview(const ReportAssembler::CoverInfo &c, int widthPx)
{
    const int h = int(widthPx * 210.0 / 297.0);
    QImage img(widthPx, h, QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::white);
    QPainter p(&img);
    paintCover(p, QRectF(0, 0, widthPx, h), c, widthPx / 297.0);
    p.setPen(QPen(QColor("#cccccc"), 1));
    p.setBrush(Qt::NoBrush);
    p.drawRect(QRectF(0, 0, widthPx - 1, h - 1));
    p.end();
    return img;
}

} // namespace ReportPainter
