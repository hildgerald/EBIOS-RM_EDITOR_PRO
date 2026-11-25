// SPDX-License-Identifier: MIT
#include "DocxExporter.h"

#include "MarkdownModel.h"
#include "ReportAssembler.h"
#include "ZipWriter.h"

#include <algorithm>
#include <QBuffer>
#include <QDir>
#include <QImage>
#include <QTemporaryDir>

namespace DocxExporter {
namespace {

const int kPageW = 16838, kPageH = 11906, kMargin = 851;     // A4 paysage, en twips
const int kTextW = kPageW - 2 * kMargin;

QString xesc(const QString &in)
{
    QString s;
    s.reserve(in.size());
    for (QChar c : in) {
        const ushort u = c.unicode();
        if (u < 0x20 && u != 0x09 && u != 0x0A && u != 0x0D) continue;   // caractères interdits en XML 1.0
        if (u == '&') s += QStringLiteral("&amp;");
        else if (u == '<') s += QStringLiteral("&lt;");
        else if (u == '>') s += QStringLiteral("&gt;");
        else if (u == '"') s += QStringLiteral("&quot;");
        else s += c;
    }
    return s;
}

QString runXml(const Md::Run &r, int halfPt, bool forceBold)
{
    if (r.lineBreak) return QStringLiteral("<w:r><w:br/></w:r>");
    QString rpr;
    if (r.bold || forceBold) rpr += QStringLiteral("<w:b/>");
    if (r.italic) rpr += QStringLiteral("<w:i/>");
    if (halfPt > 0) rpr += QStringLiteral("<w:sz w:val=\"%1\"/><w:szCs w:val=\"%1\"/>").arg(halfPt);
    return QStringLiteral("<w:r>%1<w:t xml:space=\"preserve\">%2</w:t></w:r>")
        .arg(rpr.isEmpty() ? QString() : "<w:rPr>" + rpr + "</w:rPr>", xesc(r.text));
}

QString runsXml(const Md::Runs &runs, int halfPt = 0, bool forceBold = false)
{
    QString s;
    for (const Md::Run &r : runs) s += runXml(r, halfPt, forceBold);
    return s;
}

QString paragraph(const QString &pPr, const QString &content)
{
    return "<w:p>" + (pPr.isEmpty() ? QString() : "<w:pPr>" + pPr + "</w:pPr>") + content + "</w:p>";
}

QString cellXml(const Md::Cell &c, int width, int halfPt, bool header)
{
    QString tcPr = QStringLiteral("<w:tcW w:w=\"%1\" w:type=\"dxa\"/>").arg(width);
    if (header) tcPr += QStringLiteral("<w:shd w:val=\"clear\" w:color=\"auto\" w:fill=\"D9E2F3\"/>");
    else if (c.fill.isValid())
        tcPr += QStringLiteral("<w:shd w:val=\"clear\" w:color=\"auto\" w:fill=\"%1\"/>").arg(c.fill.name().mid(1).toUpper());
    return "<w:tc><w:tcPr>" + tcPr + "</w:tcPr>" +
           paragraph(QStringLiteral("<w:spacing w:before=\"20\" w:after=\"20\"/>"), runsXml(c.runs, halfPt, header)) + "</w:tc>";
}

QString tableXml(const Md::Block &b)
{
    const int nc = b.head.size();
    if (nc == 0) return QString();
    const int halfPt = nc >= 12 ? 12 : (nc >= 9 ? 14 : (nc >= 6 ? 16 : 18));
    const QVector<double> w = Md::columnWeights(b);
    QVector<int> widths(nc);
    int sum = 0;
    for (int c = 0; c < nc; ++c) {
        widths[c] = std::max(450, int(w[c] * kTextW));
        sum += widths[c];
    }
    widths[nc - 1] = std::max(450, widths[nc - 1] + (kTextW - sum));

    QString x = QStringLiteral("<w:tbl><w:tblPr><w:tblStyle w:val=\"TableGrid\"/><w:tblW w:w=\"%1\" w:type=\"dxa\"/>"
                               "<w:tblLayout w:type=\"fixed\"/><w:tblCellMar><w:left w:w=\"60\" w:type=\"dxa\"/>"
                               "<w:right w:w=\"60\" w:type=\"dxa\"/></w:tblCellMar></w:tblPr><w:tblGrid>").arg(kTextW);
    for (int c = 0; c < nc; ++c) x += QStringLiteral("<w:gridCol w:w=\"%1\"/>").arg(widths[c]);
    x += "</w:tblGrid>";

    x += "<w:tr><w:trPr><w:cantSplit/><w:tblHeader/></w:trPr>";
    for (int c = 0; c < nc; ++c) x += cellXml(b.head[c], widths[c], halfPt, true);
    x += "</w:tr>";
    for (const auto &row : b.rows) {
        x += "<w:tr><w:trPr><w:cantSplit/></w:trPr>";
        for (int c = 0; c < nc; ++c) x += cellXml(c < row.size() ? row[c] : Md::Cell(), widths[c], halfPt, false);
        x += "</w:tr>";
    }
    x += "</w:tbl>";
    x += paragraph(QStringLiteral("<w:spacing w:after=\"0\"/>"), QString());   // Word exige un paragraphe après un tableau
    return x;
}

const char *kNs =
    "xmlns:w=\"http://schemas.openxmlformats.org/wordprocessingml/2006/main\" "
    "xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\" "
    "xmlns:wp=\"http://schemas.openxmlformats.org/drawingml/2006/wordprocessingDrawing\" "
    "xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\" "
    "xmlns:pic=\"http://schemas.openxmlformats.org/drawingml/2006/picture\"";

QString heading(int level, int size, const char *color)
{
    return QStringLiteral(
        "<w:style w:type=\"paragraph\" w:styleId=\"Heading%1\"><w:name w:val=\"heading %1\"/><w:basedOn w:val=\"Normal\"/>"
        "<w:next w:val=\"Normal\"/><w:qFormat/><w:pPr><w:keepNext/><w:spacing w:before=\"200\" w:after=\"100\"/>"
        "<w:outlineLvl w:val=\"%2\"/></w:pPr><w:rPr><w:b/><w:color w:val=\"%3\"/><w:sz w:val=\"%4\"/><w:szCs w:val=\"%4\"/></w:rPr></w:style>")
        .arg(level).arg(level - 1).arg(QString::fromLatin1(color)).arg(size);
}

QString extraStyles()
{
    const QString tabs = QStringLiteral("<w:tabs><w:tab w:val=\"right\" w:leader=\"dot\" w:pos=\"%1\"/></w:tabs>").arg(kTextW);
    return QStringLiteral(
        "<w:style w:type=\"paragraph\" w:styleId=\"Title\"><w:name w:val=\"Title\"/><w:basedOn w:val=\"Normal\"/><w:next w:val=\"Normal\"/><w:qFormat/>"
        "<w:pPr><w:spacing w:after=\"200\"/></w:pPr><w:rPr><w:b/><w:color w:val=\"1D3557\"/><w:sz w:val=\"72\"/><w:szCs w:val=\"72\"/></w:rPr></w:style>"
        "<w:style w:type=\"paragraph\" w:styleId=\"Subtitle\"><w:name w:val=\"Subtitle\"/><w:basedOn w:val=\"Normal\"/><w:next w:val=\"Normal\"/><w:qFormat/>"
        "<w:rPr><w:color w:val=\"555555\"/><w:sz w:val=\"32\"/><w:szCs w:val=\"32\"/></w:rPr></w:style>"
        "<w:style w:type=\"paragraph\" w:styleId=\"TOCHeading\"><w:name w:val=\"TOC Heading\"/><w:basedOn w:val=\"Normal\"/><w:next w:val=\"Normal\"/><w:qFormat/>"
        "<w:pPr><w:keepNext/><w:spacing w:before=\"0\" w:after=\"200\"/></w:pPr><w:rPr><w:b/><w:color w:val=\"1D3557\"/><w:sz w:val=\"36\"/><w:szCs w:val=\"36\"/></w:rPr></w:style>"
        "<w:style w:type=\"paragraph\" w:styleId=\"TOC2\"><w:name w:val=\"toc 2\"/><w:basedOn w:val=\"Normal\"/><w:next w:val=\"Normal\"/><w:uiPriority w:val=\"39\"/>"
        "<w:pPr>") + tabs + QStringLiteral("<w:spacing w:before=\"60\" w:after=\"20\"/></w:pPr><w:rPr><w:b/><w:color w:val=\"1D3557\"/></w:rPr></w:style>"
        "<w:style w:type=\"paragraph\" w:styleId=\"TOC3\"><w:name w:val=\"toc 3\"/><w:basedOn w:val=\"Normal\"/><w:next w:val=\"Normal\"/><w:uiPriority w:val=\"39\"/>"
        "<w:pPr>") + tabs + QStringLiteral("<w:spacing w:after=\"20\"/><w:ind w:left=\"284\"/></w:pPr></w:style>"
        "<w:style w:type=\"paragraph\" w:styleId=\"TableofFigures\"><w:name w:val=\"table of figures\"/><w:basedOn w:val=\"Normal\"/><w:next w:val=\"Normal\"/><w:uiPriority w:val=\"99\"/>"
        "<w:pPr>") + tabs + QStringLiteral("<w:spacing w:after=\"20\"/></w:pPr></w:style>"
        "<w:style w:type=\"paragraph\" w:styleId=\"Caption\"><w:name w:val=\"caption\"/><w:basedOn w:val=\"Normal\"/><w:next w:val=\"Normal\"/><w:qFormat/>"
        "<w:pPr><w:spacing w:before=\"60\" w:after=\"120\"/><w:jc w:val=\"center\"/></w:pPr><w:rPr><w:i/><w:color w:val=\"595959\"/><w:sz w:val=\"18\"/><w:szCs w:val=\"18\"/></w:rPr></w:style>"
        "<w:style w:type=\"paragraph\" w:styleId=\"FigureTitle\"><w:name w:val=\"Figure Title\"/><w:basedOn w:val=\"Normal\"/><w:next w:val=\"Normal\"/><w:qFormat/>"
        "<w:pPr><w:keepNext/><w:spacing w:after=\"120\"/></w:pPr><w:rPr><w:b/><w:color w:val=\"2A5A8A\"/><w:sz w:val=\"26\"/><w:szCs w:val=\"26\"/></w:rPr></w:style>");
}

QString stylesXml()
{
    return QStringLiteral("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<w:styles xmlns:w=\"http://schemas.openxmlformats.org/wordprocessingml/2006/main\">"
        "<w:docDefaults><w:rPrDefault><w:rPr><w:rFonts w:ascii=\"Calibri\" w:hAnsi=\"Calibri\" w:eastAsia=\"Calibri\" w:cs=\"Calibri\"/>"
        "<w:sz w:val=\"20\"/><w:szCs w:val=\"20\"/><w:lang w:val=\"fr-FR\"/></w:rPr></w:rPrDefault>"
        "<w:pPrDefault><w:pPr><w:spacing w:after=\"80\" w:line=\"259\" w:lineRule=\"auto\"/></w:pPr></w:pPrDefault></w:docDefaults>"
        "<w:style w:type=\"paragraph\" w:default=\"1\" w:styleId=\"Normal\"><w:name w:val=\"Normal\"/><w:qFormat/></w:style>")
        + heading(1, 44, "1D3557") + heading(2, 32, "1D3557") + heading(3, 26, "2A5A8A") + heading(4, 22, "444444") +
        QStringLiteral(
        "<w:style w:type=\"table\" w:default=\"1\" w:styleId=\"TableNormal\"><w:name w:val=\"Normal Table\"/><w:uiPriority w:val=\"99\"/>"
        "<w:semiHidden/><w:tblPr><w:tblInd w:w=\"0\" w:type=\"dxa\"/><w:tblCellMar><w:top w:w=\"0\" w:type=\"dxa\"/>"
        "<w:left w:w=\"108\" w:type=\"dxa\"/><w:bottom w:w=\"0\" w:type=\"dxa\"/><w:right w:w=\"108\" w:type=\"dxa\"/></w:tblCellMar></w:tblPr></w:style>"
        "<w:style w:type=\"table\" w:styleId=\"TableGrid\"><w:name w:val=\"Table Grid\"/><w:basedOn w:val=\"TableNormal\"/><w:uiPriority w:val=\"59\"/>"
        "<w:tblPr><w:tblBorders>"
        "<w:top w:val=\"single\" w:sz=\"4\" w:space=\"0\" w:color=\"808080\"/><w:left w:val=\"single\" w:sz=\"4\" w:space=\"0\" w:color=\"808080\"/>"
        "<w:bottom w:val=\"single\" w:sz=\"4\" w:space=\"0\" w:color=\"808080\"/><w:right w:val=\"single\" w:sz=\"4\" w:space=\"0\" w:color=\"808080\"/>"
        "<w:insideH w:val=\"single\" w:sz=\"4\" w:space=\"0\" w:color=\"808080\"/><w:insideV w:val=\"single\" w:sz=\"4\" w:space=\"0\" w:color=\"808080\"/>"
        "</w:tblBorders></w:tblPr></w:style>")
        + extraStyles() + QStringLiteral("</w:styles>");
}

QString footerXml(const QString &title)
{
    return QStringLiteral("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?><w:ftr ") + kNs + ">" +
           paragraph(QStringLiteral("<w:jc w:val=\"right\"/>"),
                     QStringLiteral("<w:r><w:rPr><w:sz w:val=\"16\"/></w:rPr><w:t xml:space=\"preserve\">%1 – Page </w:t></w:r>"
                                    "<w:r><w:fldChar w:fldCharType=\"begin\"/></w:r>"
                                    "<w:r><w:instrText xml:space=\"preserve\"> PAGE </w:instrText></w:r>"
                                    "<w:r><w:fldChar w:fldCharType=\"separate\"/></w:r>"
                                    "<w:r><w:t>1</w:t></w:r><w:r><w:fldChar w:fldCharType=\"end\"/></w:r>").arg(xesc(title))) +
           "</w:ftr>";
}

QString drawingXml(int id, qint64 cx, qint64 cy, const QString &rid, const QString &alt)
{
    QString x = QStringLiteral(
        "<w:r><w:drawing><wp:inline distT=\"0\" distB=\"0\" distL=\"0\" distR=\"0\"><wp:extent cx=\"@CX@\" cy=\"@CY@\"/>"
        "<wp:docPr id=\"@ID@\" name=\"Image @ID@\" descr=\"@ALT@\"/><wp:cNvGraphicFramePr><a:graphicFrameLocks noChangeAspect=\"1\"/></wp:cNvGraphicFramePr>"
        "<a:graphic><a:graphicData uri=\"http://schemas.openxmlformats.org/drawingml/2006/picture\"><pic:pic>"
        "<pic:nvPicPr><pic:cNvPr id=\"@ID@\" name=\"image@ID@.png\"/><pic:cNvPicPr/></pic:nvPicPr>"
        "<pic:blipFill><a:blip r:embed=\"@RID@\"/><a:stretch><a:fillRect/></a:stretch></pic:blipFill>"
        "<pic:spPr><a:xfrm><a:off x=\"0\" y=\"0\"/><a:ext cx=\"@CX@\" cy=\"@CY@\"/></a:xfrm><a:prstGeom prst=\"rect\"><a:avLst/></a:prstGeom></pic:spPr>"
        "</pic:pic></a:graphicData></a:graphic></wp:inline></w:drawing></w:r>");
    x.replace("@CX@", QString::number(cx));
    x.replace("@CY@", QString::number(cy));
    x.replace("@ID@", QString::number(id));
    x.replace("@RID@", rid);
    x.replace("@ALT@", xesc(alt));
    return x;
}

// ---------------------------------------------------------------------------- assemblage
struct Ctx {
    QVector<QPair<QString, QByteArray>> media;
    QString rels;
    int imageCount = 0;
    int drawingId = 0;
};

QString imageRun(Ctx &cx, const QImage &im, qint64 w, qint64 h, const QString &alt)
{
    QByteArray png;
    QBuffer buf(&png);
    buf.open(QIODevice::WriteOnly);
    im.save(&buf, "PNG");
    const int n = ++cx.imageCount;
    cx.media.append({QStringLiteral("word/media/image%1.png").arg(n), png});
    const QString rid = QStringLiteral("rId%1").arg(10 + n);
    cx.rels += QStringLiteral("<Relationship Id=\"%1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/image\" "
                              "Target=\"media/image%2.png\"/>").arg(rid).arg(n);
    return drawingXml(++cx.drawingId, w, h, rid, alt);
}

// Redimensionne (en EMU) pour tenir dans maxCx x maxCy, sans agrandir au-delà de 1 px = 9525 EMU.
void fitEmu(const QImage &im, double maxCx, double maxCy, qint64 *cx, qint64 *cy)
{
    double w = im.width() * 9525.0, h = im.height() * 9525.0;
    const double sc = std::min(1.0, std::min(maxCx / w, maxCy / h));
    *cx = qint64(w * sc);
    *cy = qint64(h * sc);
}

QString plainRun(const QString &text, const QString &rpr = QString())
{
    return QStringLiteral("<w:r>%1<w:t xml:space=\"preserve\">%2</w:t></w:r>")
        .arg(rpr.isEmpty() ? QString() : "<w:rPr>" + rpr + "</w:rPr>", xesc(text));
}

QString pageBreak()
{
    return QStringLiteral("<w:p><w:r><w:br w:type=\"page\"/></w:r></w:p>");
}

// Bloc de champ (TOC) : le champ commence dans le 1er paragraphe et se termine dans le dernier.
QString fieldBlock(const QString &instr, const QVector<QPair<QString, QString>> &entries /* style, texte */)
{
    QString x;
    const int n = entries.size();
    for (int i = 0; i < n; ++i) {
        QString runs;
        if (i == 0)
            runs += QStringLiteral("<w:r><w:fldChar w:fldCharType=\"begin\"/></w:r><w:r><w:instrText xml:space=\"preserve\"> %1 </w:instrText></w:r>"
                                   "<w:r><w:fldChar w:fldCharType=\"separate\"/></w:r>").arg(xesc(instr));
        runs += plainRun(entries[i].second);
        if (i == n - 1) runs += QStringLiteral("<w:r><w:fldChar w:fldCharType=\"end\"/></w:r>");
        x += paragraph(QStringLiteral("<w:pStyle w:val=\"%1\"/>").arg(entries[i].first), runs);
    }
    return x;
}

QString coverXml(Ctx &cx, const ReportAssembler::CoverInfo &c)
{
    QString x;
    if (!c.logo.isNull()) {
        double w = std::max(5.0, c.logoWidthMm) * 36000.0;
        double h = w * c.logo.height() / double(c.logo.width());
        const double maxH = 45.0 * 36000.0;
        if (h > maxH) { h = maxH; w = h * c.logo.width() / double(c.logo.height()); }
        const char *jc = c.logoAlign == 1 ? "center" : (c.logoAlign == 2 ? "right" : "left");
        x += paragraph(QStringLiteral("<w:jc w:val=\"%1\"/>").arg(QString::fromLatin1(jc)),
                       imageRun(cx, c.logo, qint64(w), qint64(h), QStringLiteral("Logo")));
    }
    x += paragraph(QStringLiteral("<w:pStyle w:val=\"Title\"/><w:spacing w:before=\"2600\" w:after=\"200\"/>"), plainRun(c.title));
    if (!c.subtitle.trimmed().isEmpty())
        x += paragraph(QStringLiteral("<w:pStyle w:val=\"Subtitle\"/>"), plainRun(c.subtitle));

    QVector<QPair<QString, QString>> meta;
    auto add = [&](const QString &k, const QString &v) { if (!v.trimmed().isEmpty()) meta.append({k, v}); };
    add(QStringLiteral("Organisation"), c.organisation);
    add(QStringLiteral("Version"), c.version);
    add(QStringLiteral("Date"), c.date);
    add(QStringLiteral("Auteur"), c.author);
    add(QStringLiteral("Responsable métier"), c.owner);
    if (!meta.isEmpty()) {
        x += paragraph(QStringLiteral("<w:spacing w:before=\"1800\" w:after=\"0\"/>"), QString());
        x += QStringLiteral("<w:tbl><w:tblPr><w:tblW w:w=\"9000\" w:type=\"dxa\"/><w:tblLayout w:type=\"fixed\"/></w:tblPr>"
                            "<w:tblGrid><w:gridCol w:w=\"3000\"/><w:gridCol w:w=\"6000\"/></w:tblGrid>");
        for (const auto &kv : meta) {
            x += QStringLiteral("<w:tr><w:tc><w:tcPr><w:tcW w:w=\"3000\" w:type=\"dxa\"/></w:tcPr>") +
                 paragraph(QStringLiteral("<w:spacing w:before=\"40\" w:after=\"40\"/>"), plainRun(kv.first, QStringLiteral("<w:b/><w:color w:val=\"1D3557\"/>"))) +
                 QStringLiteral("</w:tc><w:tc><w:tcPr><w:tcW w:w=\"6000\" w:type=\"dxa\"/></w:tcPr>") +
                 paragraph(QStringLiteral("<w:spacing w:before=\"40\" w:after=\"40\"/>"), plainRun(kv.second)) + "</w:tc></w:tr>";
        }
        x += "</w:tbl>";
    }
    if (!c.classification.trimmed().isEmpty()) {
        const QString side = QStringLiteral("w:val=\"single\" w:sz=\"8\" w:space=\"4\" w:color=\"B03A2E\"");
        x += paragraph(QStringLiteral("<w:pBdr><w:top %1/><w:left %1/><w:bottom %1/><w:right %1/></w:pBdr><w:spacing w:before=\"600\"/><w:jc w:val=\"right\"/>").arg(side),
                       plainRun(c.classification, QStringLiteral("<w:b/><w:color w:val=\"B03A2E\"/>")));
    }
    x += pageBreak();
    return x;
}

QString tocXml(const ReportAssembler::Assembled &a)
{
    QVector<QPair<QString, QString>> heads;
    for (const Md::Block &b : a.doc.blocks)
        if (b.type == Md::Block::Heading && (b.level == 2 || b.level == 3))
            heads.append({b.level == 2 ? QStringLiteral("TOC2") : QStringLiteral("TOC3"), Md::plain(b.runs)});
    QVector<QPair<QString, QString>> figs;
    for (const Md::Figure &f : ReportAssembler::collectFigures(a.doc)) figs.append({QStringLiteral("TableofFigures"), f.caption()});

    QString x = paragraph(QStringLiteral("<w:pStyle w:val=\"TOCHeading\"/>"), plainRun(QStringLiteral("Sommaire")));
    x += fieldBlock(QStringLiteral("TOC \\o \"2-3\" \\h \\z \\u"), heads);
    if (!figs.isEmpty()) {
        x += paragraph(QStringLiteral("<w:pStyle w:val=\"TOCHeading\"/><w:spacing w:before=\"360\"/>"), plainRun(QStringLiteral("Liste des figures")));
        x += fieldBlock(QStringLiteral("TOC \\h \\z \\c \"Figure\""), figs);
    }
    x += pageBreak();
    return x;
}

QString figureRowXml(Ctx &cx, const Md::Block &b)
{
    const int n = b.figures.size();
    if (n == 0) return QString();
    QString x = paragraph(QStringLiteral("<w:pStyle w:val=\"FigureTitle\"/><w:pageBreakBefore/>"), plainRun(b.rowTitle));
    const int cellW = kTextW / n;
    x += QStringLiteral("<w:tbl><w:tblPr><w:tblW w:w=\"%1\" w:type=\"dxa\"/><w:tblLayout w:type=\"fixed\"/></w:tblPr><w:tblGrid>").arg(kTextW);
    for (int i = 0; i < n; ++i) x += QStringLiteral("<w:gridCol w:w=\"%1\"/>").arg(cellW);
    x += "</w:tblGrid><w:tr>";
    for (const Md::Figure &f : b.figures) {
        x += QStringLiteral("<w:tc><w:tcPr><w:tcW w:w=\"%1\" w:type=\"dxa\"/></w:tcPr>").arg(cellW);
        const QImage im(Md::localPath(f.src));
        if (im.isNull()) {
            x += paragraph(QString(), plainRun(QStringLiteral("[image introuvable]"), QStringLiteral("<w:i/>")));
        } else {
            qint64 cx64 = 0, cy64 = 0;
            fitEmu(im, n == 1 ? 8600000.0 : 4300000.0, 4800000.0, &cx64, &cy64);
            x += paragraph(QStringLiteral("<w:jc w:val=\"center\"/>"), imageRun(cx, im, cx64, cy64, f.title));
        }
        x += paragraph(QStringLiteral("<w:pStyle w:val=\"Caption\"/>"),
                       plainRun(QStringLiteral("Figure ")) +
                       QStringLiteral("<w:fldSimple w:instr=\" SEQ Figure \\* ARABIC \"><w:r><w:t>%1</w:t></w:r></w:fldSimple>").arg(f.number) +
                       plainRun(QStringLiteral(" – ") + f.title));
        x += "</w:tc>";
    }
    x += "</w:tr></w:tbl>";
    x += paragraph(QStringLiteral("<w:spacing w:after=\"0\"/>"), QString());
    return x;
}

} // namespace

bool writeDocx(const ReportAssembler::Assembled &rep, const QString &docxPath, QString *error)
{
    Ctx cx;
    QString body;
    if (rep.opt.cover) body += coverXml(cx, rep.cover);
    if (rep.opt.toc) body += tocXml(rep);

    bool first = true;
    for (const Md::Block &b : rep.doc.blocks) {
        switch (b.type) {
        case Md::Block::Heading: {
            const QString text = Md::plain(b.runs);
            QString ppr = QStringLiteral("<w:pStyle w:val=\"Heading%1\"/>").arg(b.level);
            if (b.level == 2 && Md::startsNewPage(text) && !(first && !rep.opt.cover && !rep.opt.toc)) ppr += QStringLiteral("<w:pageBreakBefore/>");
            body += paragraph(ppr, runsXml(b.runs));
            break;
        }
        case Md::Block::Paragraph:
            body += paragraph(QString(), runsXml(b.runs));
            break;
        case Md::Block::Bullet: {
            const int left = 425 + 425 * b.level;
            Md::Run mk;
            mk.text = b.marker + QStringLiteral(" ");
            body += paragraph(QStringLiteral("<w:ind w:left=\"%1\" w:hanging=\"283\"/>").arg(left), runXml(mk, 0, false) + runsXml(b.runs));
            break;
        }
        case Md::Block::Rule:
            body += paragraph(QStringLiteral("<w:pBdr><w:bottom w:val=\"single\" w:sz=\"6\" w:space=\"1\" w:color=\"999999\"/></w:pBdr>"), QString());
            break;
        case Md::Block::Table:
            body += tableXml(b);
            break;
        case Md::Block::FigureRow:
            body += figureRowXml(cx, b);
            break;
        case Md::Block::Image: {
            const QImage im(Md::localPath(b.imageSrc));
            if (im.isNull()) {
                Md::Run r;
                r.text = QStringLiteral("[image introuvable : %1]").arg(b.alt);
                r.italic = true;
                body += paragraph(QString(), runXml(r, 0, false));
                break;
            }
            qint64 w = 0, h = 0;
            fitEmu(im, 8600000.0, 5300000.0, &w, &h);
            body += paragraph(QStringLiteral("<w:jc w:val=\"center\"/>"), imageRun(cx, im, w, h, b.alt));
            break;
        }
        }
        first = false;
    }

    const QString document =
        QStringLiteral("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?><w:document ") + kNs + "><w:body>" + body +
        QStringLiteral("<w:sectPr><w:footerReference w:type=\"default\" r:id=\"rId2\"/>"
                       "<w:pgSz w:w=\"%1\" w:h=\"%2\" w:orient=\"landscape\"/>"
                       "<w:pgMar w:top=\"%3\" w:right=\"%3\" w:bottom=\"%3\" w:left=\"%3\" w:header=\"425\" w:footer=\"425\" w:gutter=\"0\"/>"
                       "%4</w:sectPr></w:body></w:document>").arg(kPageW).arg(kPageH).arg(kMargin).arg(rep.opt.cover ? QStringLiteral("<w:titlePg/>") : QString());

    const QString contentTypes = QStringLiteral(
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">"
        "<Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>"
        "<Default Extension=\"xml\" ContentType=\"application/xml\"/>"
        "<Default Extension=\"png\" ContentType=\"image/png\"/>"
        "<Override PartName=\"/word/document.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml\"/>"
        "<Override PartName=\"/word/styles.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.wordprocessingml.styles+xml\"/>"
        "<Override PartName=\"/word/settings.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.wordprocessingml.settings+xml\"/>"
        "<Override PartName=\"/word/footer1.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.wordprocessingml.footer+xml\"/>"
        "</Types>");
    const QString rootRels = QStringLiteral(
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
        "<Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument\" Target=\"word/document.xml\"/>"
        "</Relationships>");
    const QString docRels = QStringLiteral(
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
        "<Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles\" Target=\"styles.xml\"/>"
        "<Relationship Id=\"rId2\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/footer\" Target=\"footer1.xml\"/>"
        "<Relationship Id=\"rId3\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/settings\" Target=\"settings.xml\"/>") +
        cx.rels + "</Relationships>";
    // Les champs (sommaire, liste des figures) sont recalculés par Word à l'ouverture (numéros de page).
    const QString settings = QStringLiteral(
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<w:settings xmlns:w=\"http://schemas.openxmlformats.org/wordprocessingml/2006/main\">%1</w:settings>")
        .arg(rep.opt.toc ? QStringLiteral("<w:updateFields w:val=\"true\"/>") : QString());

    ZipWriter out;   // [Content_Types].xml doit être la première entrée
    out.addFile(QStringLiteral("[Content_Types].xml"), contentTypes.toUtf8());
    out.addFile(QStringLiteral("_rels/.rels"), rootRels.toUtf8());
    out.addFile(QStringLiteral("word/document.xml"), document.toUtf8());
    out.addFile(QStringLiteral("word/styles.xml"), stylesXml().toUtf8());
    out.addFile(QStringLiteral("word/settings.xml"), settings.toUtf8());
    out.addFile(QStringLiteral("word/footer1.xml"), footerXml(rep.title).toUtf8());
    out.addFile(QStringLiteral("word/_rels/document.xml.rels"), docRels.toUtf8());
    for (const auto &m : cx.media) out.addFile(m.first, m.second, false);
    return out.writeTo(docxPath, error);
}

bool exportReport(const Project &p, const QString &docxPath, QString *error)
{
    QTemporaryDir tmp;
    if (!tmp.isValid()) {
        if (error) *error = QObject::tr("Impossible de créer un dossier temporaire.");
        return false;
    }
    ReportAssembler::Assembled a;
    if (!ReportAssembler::prepare(p, tmp.path(), &a, error)) return false;
    return writeDocx(a, docxPath, error);
}

} // namespace DocxExporter
