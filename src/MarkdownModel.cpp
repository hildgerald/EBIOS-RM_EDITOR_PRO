// SPDX-License-Identifier: MIT
#include "MarkdownModel.h"

#include "Calc.h"

#include <algorithm>
#include <QImage>
#include <QRegularExpression>
#include <QStringList>
#include <QUrl>

namespace Md {
namespace {

bool hasClosing(const QString &s, int from, QChar ch)
{
    for (int j = from; j < s.size(); ++j) {
        if (s[j] == ch && j > 0 && !s[j - 1].isSpace() && (j + 1 >= s.size() || !s[j + 1].isLetterOrNumber()))
            return true;
    }
    return false;
}

Runs parseInline(const QString &src)
{
    Runs out;
    QString cur;
    bool bold = false, italic = false;
    QChar italicChar;
    auto flush = [&]() {
        if (cur.isEmpty()) return;
        Run r;
        r.text = cur;
        r.bold = bold;
        r.italic = italic;
        out.append(r);
        cur.clear();
    };
    const int n = src.size();
    for (int i = 0; i < n; ++i) {
        const QChar c = src[i];
        if (c == '\\' && i + 1 < n && QStringLiteral("\\|*_`#[]()<>!-+.").contains(src[i + 1])) {
            cur += src[i + 1];
            ++i;
            continue;
        }
        if (c == '<') {
            const QString t = src.mid(i, 6).toLower();
            int len = 0;
            if (t.startsWith("<br>")) len = 4;
            else if (t.startsWith("<br/>")) len = 5;
            else if (t.startsWith("<br />")) len = 6;
            if (len) {
                flush();
                Run r;
                r.lineBreak = true;
                out.append(r);
                i += len - 1;
                continue;
            }
        }
        if (c == '*' && i + 1 < n && src[i + 1] == '*') {
            flush();
            bold = !bold;
            ++i;
            continue;
        }
        if (c == '`') continue;
        if (c == '_' || c == '*') {
            if (italic && c == italicChar && i > 0 && !src[i - 1].isSpace() && (i + 1 >= n || !src[i + 1].isLetterOrNumber())) {
                flush();
                italic = false;
                continue;
            }
            const bool prevOk = (i == 0) || !src[i - 1].isLetterOrNumber();
            const bool nextOk = (i + 1 < n) && !src[i + 1].isSpace();
            if (!italic && prevOk && nextOk && hasClosing(src, i + 1, c)) {
                flush();
                italic = true;
                italicChar = c;
                continue;
            }
        }
        cur += c;
    }
    flush();
    return out;
}

QStringList splitRow(QString row)
{
    row = row.trimmed();
    if (row.startsWith('|')) row.remove(0, 1);
    if (row.endsWith('|') && !row.endsWith("\\|")) row.chop(1);
    QStringList cells;
    QString cur;
    for (int i = 0; i < row.size(); ++i) {
        if (row[i] == '\\' && i + 1 < row.size() && row[i + 1] == '|') {
            cur += '|';
            ++i;
        } else if (row[i] == '|') {
            cells << cur.trimmed();
            cur.clear();
        } else {
            cur += row[i];
        }
    }
    cells << cur.trimmed();
    return cells;
}

Cell makeCell(QString text, bool header)
{
    Cell c;
    if (!header) {
        const QVector<QPair<QString, QString>> ems = {
            {QStringLiteral("🟩"), "#a9d18e"}, {QStringLiteral("🟨"), "#ffd966"},
            {QStringLiteral("🟥"), "#ff7c80"}, {QStringLiteral("⬜"), "#eeeeee"}};
        for (const auto &e : ems) {
            if (text.startsWith(e.first)) {
                c.fill = QColor(e.second);
                text = text.mid(e.first.size()).trimmed();
                break;
            }
        }
    }
    c.runs = parseInline(text);
    if (!header && !c.fill.isValid()) {
        const QColor lc = Calc::levelColor(plain(c.runs).trimmed());
        if (lc.isValid()) c.fill = lc;
    }
    return c;
}

bool isSeparator(const QString &line)
{
    static const QRegularExpression re(QStringLiteral("^\\s*\\|?\\s*:?-{2,}:?\\s*(\\|\\s*:?-{2,}:?\\s*)*\\|?\\s*$"));
    return re.match(line).hasMatch();
}

} // namespace

QString plain(const Runs &runs)
{
    QString s;
    for (const Run &r : runs) s += r.lineBreak ? QStringLiteral(" ") : r.text;
    return s;
}

QString localPath(const QString &src)
{
    if (src.startsWith("file:", Qt::CaseInsensitive)) return QUrl(src).toLocalFile();
    return QUrl::fromPercentEncoding(src.toUtf8());
}

bool startsNewPage(const QString &t)
{
    return t.startsWith("Atelier ") || t.startsWith("Conclusion") || t.startsWith("Annexe") ||
           t.startsWith("Synthèse") || t.startsWith("Méthode") || t.startsWith("Figures");
}

QVector<double> columnWeights(const Block &t)
{
    const int nc = t.head.size();
    QVector<double> w(nc, 5.0);
    for (int c = 0; c < nc; ++c) {
        int hdr = 0;
        for (const QString &word : plain(t.head[c].runs).split(' ', Qt::SkipEmptyParts)) hdr = std::max<int>(hdr, word.size());
        hdr = std::min(hdr, 14);
        int mx = 0;
        double sum = 0;
        for (const auto &row : t.rows) {
            const int len = c < row.size() ? plain(row[c].runs).size() : 0;
            mx = std::max(mx, len);
            sum += len;
        }
        const double avg = t.rows.isEmpty() ? 0.0 : sum / t.rows.size();
        w[c] = std::min(60.0, std::max<double>({5.0, double(hdr), 0.6 * mx + 0.4 * avg}));
    }
    double total = 0;
    for (double x : w) total += x;
    if (total <= 0) return QVector<double>(nc, nc ? 1.0 / nc : 1.0);
    for (double &x : w) x = std::max(0.03, x / total);
    total = 0;
    for (double x : w) total += x;
    for (double &x : w) x /= total;
    return w;
}

Document parse(const QString &markdown)
{
    Document d;
    QString src = markdown;
    src.replace("\r\n", "\n");
    const QStringList lines = src.split('\n');
    QStringList para;

    auto flushPara = [&]() {
        if (para.isEmpty()) return;
        QString joined;
        for (int k = 0; k < para.size(); ++k) {
            const bool hard = para[k].endsWith("  ");
            joined += para[k].trimmed();
            if (k + 1 < para.size()) joined += hard ? QStringLiteral("<br>") : QStringLiteral(" ");
        }
        Block b;
        b.type = Block::Paragraph;
        b.runs = parseInline(joined);
        d.blocks.append(b);
        para.clear();
    };

    static const QRegularExpression reHead(QStringLiteral("^(#{1,6})\\s+(.*)$"));
    static const QRegularExpression reImg(QStringLiteral("^!\\[(.*?)\\]\\((.*)\\)\\s*$"));
    static const QRegularExpression reRule(QStringLiteral("^(-{3,}|\\*{3,}|_{3,})$"));
    static const QRegularExpression reBullet(QStringLiteral("^(\\s*)([-*+])\\s+(.*)$"));
    static const QRegularExpression reNum(QStringLiteral("^(\\s*)(\\d+[.)])\\s+(.*)$"));

    for (int i = 0; i < lines.size(); ++i) {
        const QString line = lines[i];
        const QString t = line.trimmed();
        if (t.isEmpty()) { flushPara(); continue; }

        QRegularExpressionMatch m = reHead.match(t);
        if (m.hasMatch()) {
            flushPara();
            Block b;
            b.type = Block::Heading;
            b.level = std::min<int>(4, m.captured(1).size());
            b.runs = parseInline(m.captured(2));
            d.blocks.append(b);
            continue;
        }
        if (t.startsWith('|') && i + 1 < lines.size() && isSeparator(lines[i + 1])) {
            flushPara();
            Block b;
            b.type = Block::Table;
            for (const QString &c : splitRow(t)) b.head.append(makeCell(c, true));
            i += 2;
            while (i < lines.size() && lines[i].trimmed().startsWith('|')) {
                QVector<Cell> row;
                for (const QString &c : splitRow(lines[i])) row.append(makeCell(c, false));
                while (row.size() < b.head.size()) row.append(Cell());
                while (row.size() > b.head.size()) row.removeLast();
                b.rows.append(row);
                ++i;
            }
            --i;
            d.blocks.append(b);
            continue;
        }
        m = reImg.match(t);
        if (m.hasMatch()) {
            flushPara();
            Block b;
            b.type = Block::Image;
            b.alt = m.captured(1);
            b.imageSrc = m.captured(2).trimmed();
            d.blocks.append(b);
            continue;
        }
        if (reRule.match(t).hasMatch()) {
            flushPara();
            Block b;
            b.type = Block::Rule;
            d.blocks.append(b);
            continue;
        }
        m = reBullet.match(line);
        const bool num = !m.hasMatch();
        if (num) m = reNum.match(line);
        if (m.hasMatch()) {
            flushPara();
            Block b;
            b.type = Block::Bullet;
            b.level = m.captured(1).size() / 2;
            b.marker = num ? m.captured(2) : QStringLiteral("•");
            b.runs = parseInline(m.captured(3));
            d.blocks.append(b);
            continue;
        }
        para << line;
    }
    flushPara();
    return d;
}

// ------------------------------------------------------------------ HTML (QTextDocument)
namespace {

QString runsHtml(const Runs &runs)
{
    QString s;
    for (const Run &r : runs) {
        if (r.lineBreak) { s += QStringLiteral("<br/>"); continue; }
        QString t = r.text.toHtmlEscaped();
        if (r.bold) t = "<b>" + t + "</b>";
        if (r.italic) t = "<i>" + t + "</i>";
        s += t;
    }
    return s;
}

} // namespace

QString toHtml(const Document &doc)
{
    QString h = QStringLiteral(
        "<html><head><meta charset=\"utf-8\"/><style>"
        "body{font-family:'Segoe UI','Helvetica','Arial',sans-serif;font-size:9pt;}"
        "h1{color:#1d3557;font-size:22pt;} h2{color:#1d3557;font-size:16pt;}"
        "h3{color:#2a5a8a;font-size:12pt;} h4{color:#444444;font-size:10pt;} h5{color:#2a5a8a;font-size:12pt;}"
        "</style></head><body>");
    bool first = true;
    for (const Block &b : doc.blocks) {
        switch (b.type) {
        case Block::Heading: {
            const QString tag = "h" + QString::number(b.level);
            const bool brk = !first && b.level == 2 && startsNewPage(plain(b.runs));
            h += "<" + tag + (brk ? QStringLiteral(" style=\"page-break-before:always;\"") : QString()) + ">" + runsHtml(b.runs) + "</" + tag + ">";
            break;
        }
        case Block::Paragraph:
            h += "<p>" + runsHtml(b.runs) + "</p>";
            break;
        case Block::Bullet:
            h += QStringLiteral("<p style=\"margin-left:%1px;\">%2&nbsp;%3</p>").arg(18 + 18 * b.level).arg(b.marker.toHtmlEscaped(), runsHtml(b.runs));
            break;
        case Block::Rule:
            h += QStringLiteral("<hr/>");
            break;
        case Block::Image: {
            const QString path = localPath(b.imageSrc);
            const QImage im(path);
            if (im.isNull()) {
                h += "<p><i>[image introuvable : " + b.alt.toHtmlEscaped() + "]</i></p>";
            } else {
                const int w = std::min(im.width(), 820);
                h += QStringLiteral("<p><img src=\"%1\" width=\"%2\"/></p>")
                         .arg(QUrl::fromLocalFile(path).toString(QUrl::FullyEncoded), QString::number(w));
            }
            break;
        }
        case Block::FigureRow: {
            const int n = b.figures.size();
            if (n == 0) break;
            const int maxW = n == 1 ? 940 : 480, maxH = 540;
            // <h5> sert de repère de pagination (voir PdfExporter) : une planche = une page
            h += "<h5 style=\"page-break-before:always;\">" + b.rowTitle.toHtmlEscaped() + "</h5>";
            h += QStringLiteral("<table width=\"100%\" border=\"0\" cellspacing=\"0\" cellpadding=\"6\"><tr>");
            for (const Figure &f : b.figures) {
                const QString path = localPath(f.src);
                const QImage im(path);
                QString cell;
                if (im.isNull()) {
                    cell = QStringLiteral("<i>[image introuvable]</i>");
                } else {
                    const double sc = std::min(1.0, std::min(double(maxW) / im.width(), double(maxH) / im.height()));
                    cell = QStringLiteral("<img src=\"%1\" width=\"%2\" height=\"%3\"/>")
                               .arg(QUrl::fromLocalFile(path).toString(QUrl::FullyEncoded), QString::number(int(im.width() * sc)), QString::number(int(im.height() * sc)));
                }
                h += QStringLiteral("<td align=\"center\" valign=\"top\" width=\"%1%\">%2<br/><i>%3</i></td>")
                         .arg(QString::number(100 / n), cell, f.caption().toHtmlEscaped());
            }
            h += "</tr></table>";
            break;
        }
        case Block::Table: {
            const int nc = b.head.size();
            const int fs = nc >= 12 ? 6 : (nc >= 9 ? 7 : (nc >= 6 ? 8 : 9));
            const QVector<double> w = columnWeights(b);
            const QString span = QStringLiteral("<span style=\"font-size:%1pt;\">").arg(fs);
            h += QStringLiteral("<table border=\"1\" cellspacing=\"0\" cellpadding=\"3\" width=\"100%\" style=\"border-color:#7f7f7f;\"><thead><tr>");
            for (int c = 0; c < nc; ++c)
                h += QStringLiteral("<th width=\"%1%\" bgcolor=\"#d9e2f3\" align=\"left\">%2<b>%3</b></span></th>")
                         .arg(w[c] * 100.0, 0, 'f', 1).arg(span, runsHtml(b.head[c].runs));
            h += "</tr></thead><tbody>";
            for (const auto &row : b.rows) {
                h += "<tr>";
                for (const Cell &c : row)
                    h += QStringLiteral("<td valign=\"top\"%1>%2%3</span></td>")
                             .arg(c.fill.isValid() ? QStringLiteral(" bgcolor=\"%1\"").arg(c.fill.name()) : QString(), span, runsHtml(c.runs));
                h += "</tr>";
            }
            h += "</tbody></table><p></p>";
            break;
        }
        }
        first = false;
    }
    return h + "</body></html>";
}

} // namespace Md
