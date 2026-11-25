// SPDX-License-Identifier: MIT
#include "PdfExporter.h"

#include "MarkdownModel.h"
#include "ReportAssembler.h"
#include "ReportPainter.h"

#include <algorithm>
#include <QAbstractTextDocumentLayout>
#include <QFileInfo>
#include <QMarginsF>
#include <QPageLayout>
#include <QPageSize>
#include <QPainter>
#include <QPdfWriter>
#include <QTemporaryDir>
#include <QTextBlock>
#include <QTextBlockFormat>
#include <QTextDocument>

namespace PdfExporter {

bool exportReport(const Project &p, const QString &pdfPath, QString *error)
{
    QTemporaryDir tmp;
    if (!tmp.isValid()) {
        if (error) *error = QObject::tr("Impossible de créer un dossier temporaire.");
        return false;
    }
    ReportAssembler::Assembled a;
    if (!ReportAssembler::prepare(p, tmp.path(), &a, error)) return false;

    // --- Corps du rapport : texte courant mis en page par QTextDocument -----------------------
    QTextDocument doc;
    doc.setDocumentMargin(0);
    doc.setHtml(Md::toHtml(a.doc));

    {
        QPdfWriter writer(pdfPath);
        writer.setPageSize(QPageSize(QPageSize::A4));
        writer.setPageOrientation(QPageLayout::Landscape);
        writer.setPageMargins(QMarginsF(12, 12, 12, 12), QPageLayout::Millimeter);
        writer.setTitle(a.title);
        writer.setCreator(QStringLiteral("EBIOS RM Studio"));

        const double mm = writer.resolution() / 25.4;
        const QRect pr = writer.pageLayout().paintRectPixels(writer.resolution());
        const QRectF page(0, 0, pr.width(), pr.height());
        const double footerMm = 9.0;
        const QSizeF bodySize(page.width(), page.height() - footerMm * mm);

        // La mise en page se fait dans l'unité du support PDF (comme QTextDocument::print)
        doc.documentLayout()->setPaintDevice(&writer);
        doc.setPageSize(bodySize);
        const int bodyPages = std::max(1, doc.pageCount());

        // --- Repérage des titres et des planches pour le sommaire ----------------------------
        QVector<ReportPainter::TocEntry> toc, figs;
        QVector<int> rowPages;
        for (QTextBlock b = doc.begin(); b.isValid(); b = b.next()) {
            const int lvl = b.blockFormat().headingLevel();
            if (lvl == 0) continue;
            const double top = doc.documentLayout()->blockBoundingRect(b).top();
            const int pg = std::max(0, int((top + 1.0) / bodySize.height()));
            if (lvl == 2 || lvl == 3) {
                ReportPainter::TocEntry e;
                e.level = lvl;
                e.text = b.text().trimmed();
                e.page = pg;
                toc.append(e);
            } else if (lvl == 5) {
                rowPages.append(pg);
            }
        }

        int row = 0;
        for (const Md::Block &b : a.doc.blocks) {
            if (b.type != Md::Block::FigureRow || b.figures.isEmpty()) continue;
            for (const Md::Figure &f : b.figures) {
                ReportPainter::TocEntry e;
                e.level = 0;
                e.text = f.caption();
                e.page = row < rowPages.size() ? rowPages[row] : 0;
                figs.append(e);
            }
            ++row;
        }

        const int cap = ReportPainter::tocCapacity(page, mm);
        const int tocPages = a.opt.toc ? std::max(1, int((std::max(toc.size(), figs.size()) + cap - 1) / cap)) : 0;
        const int frontPages = (a.opt.cover ? 1 : 0) + tocPages;
        const int total = frontPages + bodyPages;
        for (auto &e : toc) e.page += frontPages + 1;
        for (auto &e : figs) e.page += frontPages + 1;

        // --- Peinture, page par page ----------------------------------------------------------
        QPainter painter(&writer);
        bool first = true;
        int phys = 0;
        auto nextPage = [&]() {
            if (!first) writer.newPage();
            first = false;
            ++phys;
        };

        if (a.opt.cover) {
            nextPage();
            ReportPainter::paintCover(painter, page, a.cover, mm);
        }
        for (int k = 0; k < tocPages; ++k) {
            nextPage();
            ReportPainter::paintToc(painter, page, mm, k, tocPages, toc, figs);
            ReportPainter::paintFooter(painter, page, mm, footerMm, a.title, phys, total);
        }
        for (int pg = 0; pg < bodyPages; ++pg) {
            nextPage();
            painter.save();
            painter.translate(0, -pg * bodySize.height());
            doc.drawContents(&painter, QRectF(0, pg * bodySize.height(), bodySize.width(), bodySize.height()));
            painter.restore();
            ReportPainter::paintFooter(painter, page, mm, footerMm, a.title, phys, total);
        }
        painter.end();
    }   // le fichier est finalisé à la destruction de QPdfWriter

    const QFileInfo fi(pdfPath);
    if (!fi.exists() || fi.size() == 0) {
        if (error) *error = QObject::tr("Le fichier PDF n'a pas pu être écrit (droits d'accès ou fichier ouvert ailleurs ?).");
        return false;
    }
    return true;
}

} // namespace PdfExporter
