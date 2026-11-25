// SPDX-License-Identifier: MIT
#include "Widgets.h"

#include <algorithm>
#include <cmath>
#include <QFontMetrics>
#include <QMap>
#include <QPaintEvent>

namespace {

const double kPi = 3.14159265358979323846;

QColor postureColor(const QString &posture)
{
    if (posture == "Zone de danger") return QColor("#e63946");
    if (posture == "Zone de contrôle") return QColor("#f4a52a");
    if (posture == "Zone de veille") return QColor("#009e8e");
    return QColor("#b8b8b8");
}

QColor levelFill(const QString &level)
{
    if (level == "Faible") return QColor("#a9d18e");
    if (level == "Moyen") return QColor("#ffd966");
    if (level == "Élevé") return QColor("#ff7c80");
    return QColor("#eeeeee");
}

QColor likelihoodColor(int v)
{
    switch (v) {
    case 1: return QColor("#dbe9f7");
    case 2: return QColor("#a9c9ec");
    case 3: return QColor("#4d90d8");
    case 4: return QColor("#0e4a63");
    default: return QColor("#dddddd");
    }
}

} // namespace

// =============================================================================== Radar
RadarThresholds thresholdsFrom(const Project &p)
{
    RadarThresholds th;
    th.hp = p.zone("z_hp");
    th.watch = p.zone("z_watch");
    th.control = p.zone("z_control");
    th.danger = p.zone("z_danger");
    return th;
}

void paintRadar(QPainter &p, const QRectF &area, const QVector<Calc::RadarItem> &items,
                const RadarThresholds &thIn, const QString &title)
{
    p.save();
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);
    p.fillRect(area, Qt::white);

    // Seuils triés pour garantir une échelle monotone
    double a[3] = {thIn.watch, thIn.control, thIn.danger};
    std::sort(a, a + 3);
    RadarThresholds th = thIn;
    th.watch = a[0]; th.control = a[1]; th.danger = a[2];
    const double maxScore = th.danger * 1.25 + 0.5;

    // score -> fraction du rayon (0 = centre = menace maximale, 1 = bord = menace nulle)
    const QVector<QPointF> pts = {{0.0, 1.0}, {th.watch, 0.78}, {th.control, 0.58},
                                  {th.danger, 0.28}, {maxScore, 0.04}};
    auto frac = [&](double s) -> double {
        if (s <= 0) return 1.0;
        if (s >= maxScore) return 0.04;
        for (int i = 1; i < pts.size(); ++i) {
            if (s <= pts[i].x()) {
                const double dx = pts[i].x() - pts[i - 1].x();
                if (dx <= 1e-9) return pts[i].y();
                const double t = (s - pts[i - 1].x()) / dx;
                return pts[i - 1].y() + t * (pts[i].y() - pts[i - 1].y());
            }
        }
        return 0.04;
    };

    const qreal titleH = title.isEmpty() ? 0 : 30;
    const qreal legendH = 64;
    const QRectF plot(area.left(), area.top() + titleH, area.width(), area.height() - titleH - legendH);
    const QPointF c = plot.center();
    qreal R = std::min((plot.width() - 2 * 190) / 2.0, plot.height() / 2.0 - 22);
    if (R < 70) R = 70;

    QFont base = p.font();
    base.setPointSizeF(std::max(8.0, std::min(11.0, area.width() / 90.0)));
    p.setFont(base);

    if (!title.isEmpty()) {
        QFont tf = base; tf.setBold(true); tf.setPointSizeF(base.pointSizeF() + 3);
        p.setFont(tf);
        p.setPen(QColor("#1d3557"));
        p.drawText(QRectF(area.left() + 10, area.top() + 4, area.width() - 20, titleH), Qt::AlignLeft | Qt::AlignVCenter, title);
        p.setFont(base);
    }

    // Axes
    p.setPen(QPen(QColor("#d0d0d0"), 1));
    p.drawLine(QPointF(c.x() - R, c.y()), QPointF(c.x() + R, c.y()));
    p.drawLine(QPointF(c.x(), c.y() - R), QPointF(c.x(), c.y() + R));

    // Anneaux de zones
    struct Ring { double thr; QColor col; QString name; };
    const QVector<Ring> rings = {{th.hp, QColor("#b8b8b8"), "Hors périmètre"},
                                 {th.watch, QColor("#009e8e"), "Zone de veille"},
                                 {th.control, QColor("#f4a52a"), "Zone de contrôle"},
                                 {th.danger, QColor("#e63946"), "Zone de danger"}};
    p.setBrush(Qt::NoBrush);
    for (const Ring &r : rings) {
        p.setPen(QPen(r.col, r.name == "Hors périmètre" ? 5 : 3));
        const double rr = frac(r.thr) * R;
        p.drawEllipse(c, rr, rr);
    }

    // Secteurs (catégories de parties prenantes)
    QStringList cats;
    for (const auto &it : items) if (!cats.contains(it.category)) cats << it.category;
    const int n = std::max<int>(1, cats.size());
    const double span = 2 * kPi / n;
    const double start = -kPi / 2;

    p.setPen(QPen(QColor("#808080"), 1.5, Qt::DashLine));
    if (n > 1) {
        for (int i = 0; i < n; ++i) {
            const double ang = start + i * span;
            p.drawLine(c + QPointF(std::cos(ang), std::sin(ang)) * (0.08 * R),
                       c + QPointF(std::cos(ang), std::sin(ang)) * (1.02 * R));
        }
    }
    QFont cf = base; cf.setBold(true); cf.setItalic(true);
    p.setFont(cf);
    p.setPen(QColor("#707070"));
    QFontMetrics cfm(cf);
    for (int i = 0; i < cats.size(); ++i) {
        const double mid = start + (i + 0.5) * span;
        const QPointF q = c + QPointF(std::cos(mid), std::sin(mid)) * (1.07 * R);
        const QString t = cfm.elidedText(cats[i], Qt::ElideRight, 170);
        const QRectF br(q.x() - 85, q.y() - 10, 170, 20);
        p.drawText(br, Qt::AlignCenter, t);
    }
    p.setFont(base);

    // Placement des bulles
    struct Bubble { QPointF pos; qreal rad; QColor col; Calc::RadarItem item; };
    QVector<Bubble> bubbles;
    for (int ci = 0; ci < cats.size(); ++ci) {
        QVector<Calc::RadarItem> inCat;
        for (const auto &it : items) if (it.category == cats[ci]) inCat.append(it);
        const int m = inCat.size();
        for (int j = 0; j < m; ++j) {
            const double ang = start + ci * span + span * (j + 1) / (m + 1);
            const double r = frac(inCat[j].score) * R;
            Bubble b;
            b.pos = c + QPointF(std::cos(ang), std::sin(ang)) * r;
            b.rad = std::max(9.0, std::min(26.0, 8.0 + inCat[j].exposure * 0.9));
            b.col = postureColor(inCat[j].posture);
            b.item = inCat[j];
            bubbles.append(b);
        }
    }

    // Étiquettes avec lignes de rappel (gauche / droite)
    QVector<int> left, right;
    for (int i = 0; i < bubbles.size(); ++i)
        (bubbles[i].pos.x() < c.x() ? left : right).append(i);
    auto byY = [&](int x, int y) { return bubbles[x].pos.y() < bubbles[y].pos.y(); };
    std::sort(left.begin(), left.end(), byY);
    std::sort(right.begin(), right.end(), byY);

    QFont lf = base; lf.setBold(true);
    QFontMetrics lfm(lf);
    auto drawLabels = [&](const QVector<int> &side, bool isRight) {
        qreal lastY = -1e9;
        const qreal anchorX = isRight ? c.x() + R + 45 : c.x() - R - 45;
        for (int idx : side) {
            const Bubble &b = bubbles[idx];
            qreal ly = std::max(b.pos.y(), lastY + 22);
            lastY = ly;
            p.setPen(QPen(Qt::black, 1));
            p.drawLine(b.pos, QPointF(anchorX, ly));
            p.setFont(lf);
            const QString t = lfm.elidedText(b.item.name, Qt::ElideRight, 175);
            if (isRight)
                p.drawText(QRectF(anchorX + 4, ly - 11, 180, 22), Qt::AlignLeft | Qt::AlignVCenter, t);
            else
                p.drawText(QRectF(anchorX - 184, ly - 11, 180, 22), Qt::AlignRight | Qt::AlignVCenter, t);
        }
    };
    drawLabels(left, false);
    drawLabels(right, true);

    // Bulles (au-dessus des lignes)
    for (const Bubble &b : bubbles) {
        QColor fill = b.col;
        if (!b.item.relevant) fill.setAlpha(110);
        p.setBrush(fill);
        p.setPen(QPen(Qt::black, b.item.relevant ? 1.6 : 1.0, b.item.relevant ? Qt::SolidLine : Qt::DashLine));
        p.drawEllipse(b.pos, b.rad, b.rad);
    }

    // Centre : objet de l'étude
    p.setBrush(QColor("#1d5c8a"));
    p.setPen(Qt::NoPen);
    p.drawEllipse(c, 11, 11);
    QFont of = base; of.setBold(true);
    p.setFont(of);
    p.setPen(QColor("#1d5c8a"));
    p.drawText(QRectF(c.x() + 14, c.y() - 4, 150, 18), Qt::AlignLeft | Qt::AlignVCenter, QStringLiteral("Objet de l'étude"));
    p.setFont(base);

    // Légende
    const qreal ly = area.bottom() - legendH + 8;
    const qreal colW = area.width() / 4.0;
    const QVector<Ring> legend = {rings[1], rings[2], rings[3], rings[0]};
    for (int i = 0; i < legend.size(); ++i) {
        const qreal x = area.left() + i * colW + 20;
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(legend[i].col, 4));
        p.drawEllipse(QPointF(x + 14, ly + 20), 14, 14);
        QFont bf = base; bf.setBold(true);
        p.setFont(bf);
        p.setPen(Qt::black);
        p.drawText(QRectF(x + 36, ly + 2, colW - 50, 20), Qt::AlignLeft | Qt::AlignVCenter, legend[i].name);
        p.setFont(base);
        p.setPen(QColor("#555555"));
        p.drawText(QRectF(x + 36, ly + 22, colW - 50, 20), Qt::AlignLeft | Qt::AlignVCenter,
                   QStringLiteral("Seuil : %1").arg(legend[i].thr));
    }
    p.setPen(QColor("#555555"));
    p.drawText(QRectF(area.left() + 10, area.bottom() - 20, area.width() - 20, 18), Qt::AlignLeft | Qt::AlignVCenter,
               QStringLiteral("Plus une partie prenante est proche du centre, plus sa menace est élevée. "
                              "Taille de la bulle = exposition ; contour pointillé = partie prenante non retenue comme pertinente."));
    p.restore();
}

QImage renderRadarImage(const QVector<Calc::RadarItem> &items, const RadarThresholds &th,
                        const QString &title, const QSize &size)
{
    QImage img(size, QImage::Format_ARGB32);
    img.fill(Qt::white);
    QPainter p(&img);
    paintRadar(p, QRectF(QPointF(0, 0), QSizeF(size)), items, th, title);
    p.end();
    return img;
}

RadarWidget::RadarWidget(QWidget *parent) : QWidget(parent)
{
    setMinimumSize(640, 520);
}

void RadarWidget::setData(const QVector<Calc::RadarItem> &items, const RadarThresholds &th, const QString &title)
{
    items_ = items; th_ = th; title_ = title;
    update();
}

QImage RadarWidget::toImage(const QSize &size) const
{
    return renderRadarImage(items_, th_, title_, size);
}

void RadarWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    paintRadar(p, QRectF(rect()), items_, th_, title_);
}

// =============================================================================== Matrice
void paintRiskMatrix(QPainter &p, const QRectF &area, const QString &title,
                     const QStringList &levels, const QStringList &texts)
{
    p.save();
    p.setRenderHint(QPainter::Antialiasing, true);
    p.fillRect(area, Qt::white);

    QFont base = p.font();
    base.setPointSizeF(9.5);
    p.setFont(base);

    const qreal titleH = 28, left = 70, bottom = 46;
    p.setPen(QColor("#1d3557"));
    QFont tf = base; tf.setBold(true); tf.setPointSizeF(11.5);
    p.setFont(tf);
    p.drawText(QRectF(area.left() + 6, area.top(), area.width() - 12, titleH), Qt::AlignLeft | Qt::AlignVCenter, title);
    p.setFont(base);

    const QRectF grid(area.left() + left, area.top() + titleH + 4,
                      area.width() - left - 10, area.height() - titleH - bottom - 8);
    const qreal cw = grid.width() / 4.0, ch = grid.height() / 4.0;

    for (int g = 4; g >= 1; --g) {
        for (int v = 1; v <= 4; ++v) {
            const int idx = (g - 1) * 4 + (v - 1);
            const QRectF r(grid.left() + (v - 1) * cw, grid.top() + (4 - g) * ch, cw, ch);
            p.setBrush(levelFill(idx < levels.size() ? levels[idx] : QString()));
            p.setPen(QPen(Qt::white, 2));
            p.drawRect(r);
            const QString t = idx < texts.size() ? texts[idx] : QString();
            if (!t.isEmpty()) {
                p.setPen(Qt::black);
                QFont bf = base; bf.setBold(true);
                p.setFont(bf);
                p.drawText(r.adjusted(3, 3, -3, -3), Qt::AlignCenter | Qt::TextWordWrap, t);
                p.setFont(base);
            }
        }
    }
    p.setPen(Qt::black);
    for (int g = 1; g <= 4; ++g)
        p.drawText(QRectF(area.left() + 40, grid.top() + (4 - g) * ch, left - 46, ch), Qt::AlignRight | Qt::AlignVCenter, QString::number(g));
    for (int v = 1; v <= 4; ++v)
        p.drawText(QRectF(grid.left() + (v - 1) * cw, grid.bottom() + 2, cw, 18), Qt::AlignCenter, QString::number(v));
    QFont af = base; af.setItalic(true);
    p.setFont(af);
    p.drawText(QRectF(grid.left(), grid.bottom() + 22, grid.width(), 20), Qt::AlignCenter, QStringLiteral("Vraisemblance"));
    p.save();
    p.translate(area.left() + 14, grid.center().y());
    p.rotate(-90);
    p.drawText(QRectF(-grid.height() / 2, -10, grid.height(), 20), Qt::AlignCenter, QStringLiteral("Gravité"));
    p.restore();
    p.restore();
}

RiskMatrixWidget::RiskMatrixWidget(QWidget *parent) : QWidget(parent)
{
    setMinimumSize(340, 320);
}

void RiskMatrixWidget::setData(const QString &title, const QStringList &levels, const QStringList &texts)
{
    title_ = title; levels_ = levels; texts_ = texts;
    update();
}

void RiskMatrixWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    paintRiskMatrix(p, QRectF(rect()), title_, levels_, texts_);
}

// =============================================================================== Kill chain
KillChainWidget::KillChainWidget(QWidget *parent) : QWidget(parent)
{
    setMinimumHeight(200);
}

void KillChainWidget::setData(const QString &title, int globalLikelihood, const QVector<KillChainAction> &actions)
{
    title_ = title; global_ = globalLikelihood; actions_ = actions;
    static const QStringList phases = {"Connaître", "Entrer", "Explorer", "Exploiter"};
    int maxStack = 1;
    for (const QString &ph : phases) {
        int c = 0;
        for (const auto &a : actions_) if (a.phase == ph) ++c;
        maxStack = std::max(maxStack, c);
    }
    setMinimumHeight(90 + maxStack * 74);
    updateGeometry();
    update();
}

void KillChainWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.fillRect(rect(), Qt::white);

    static const QStringList phases = {"Connaître", "Entrer", "Explorer", "Exploiter"};
    QFont base = p.font(); base.setPointSizeF(9);
    p.setFont(base);

    // Titre + vraisemblance globale
    QFont tf = base; tf.setBold(true); tf.setPointSizeF(11);
    p.setFont(tf);
    p.setPen(Qt::black);
    p.drawText(QRectF(8, 4, width() - 140, 30), Qt::AlignLeft | Qt::AlignVCenter, title_);
    p.setFont(base);
    const QRectF badge(width() - 120, 4, 112, 30);
    p.setBrush(likelihoodColor(global_));
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(badge, 6, 6);
    p.setPen(global_ >= 3 ? Qt::white : Qt::black);
    QFont bf = base; bf.setBold(true);
    p.setFont(bf);
    p.drawText(badge, Qt::AlignCenter, QStringLiteral("Vraisemblance : %1").arg(global_ > 0 ? QString::number(global_) : QStringLiteral("—")));
    p.setFont(base);

    const qreal top = 42, gap = 8;
    const qreal colW = (width() - gap * 5) / 4.0;
    const qreal boxH = 66;
    for (int i = 0; i < 4; ++i) {
        const QRectF col(gap + i * (colW + gap), top, colW, height() - top - gap);
        p.setBrush(QColor("#cfcce0"));
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(col, 8, 8);
        p.setPen(QColor("#ffffff"));
        QFont hf = base; hf.setBold(true); hf.setPointSizeF(10);
        p.setFont(hf);
        p.drawText(QRectF(col.left(), col.top() + 2, col.width(), 24), Qt::AlignCenter, phases[i].toUpper());
        p.setFont(base);

        int k = 0;
        for (const auto &a : actions_) {
            if (a.phase != phases[i]) continue;
            const QRectF box(col.left() + 8, col.top() + 30 + k * (boxH + 8), col.width() - 16, boxH);
            ++k;
            p.setBrush(Qt::white);
            p.setPen(QPen(QColor("#10253f"), 1.2));
            p.drawRoundedRect(box, 4, 4);
            const QRectF bdg(box.left(), box.top(), 26, box.height());
            p.setBrush(likelihoodColor(a.likelihood));
            p.setPen(Qt::NoPen);
            p.drawRoundedRect(bdg, 4, 4);
            p.setPen(a.likelihood >= 3 ? Qt::white : Qt::black);
            p.drawText(bdg, Qt::AlignCenter, a.likelihood > 0 ? QString::number(a.likelihood) : QStringLiteral("?"));
            p.setPen(Qt::black);
            QFont sf = base; sf.setPointSizeF(8.5);
            p.setFont(sf);
            p.drawText(box.adjusted(32, 3, -4, -3), Qt::AlignLeft | Qt::AlignVCenter | Qt::TextWordWrap, a.text);
            p.setFont(base);
        }
    }
}

QImage renderMatrixImage(const QString &title, const QStringList &levels, const QStringList &texts, const QSize &size)
{
    QImage img(size, QImage::Format_ARGB32);
    img.fill(Qt::white);
    QPainter p(&img);
    paintRiskMatrix(p, QRectF(QPointF(0, 0), QSizeF(size)), title, levels, texts);
    p.end();
    return img;
}

QImage renderKillChainImage(const QString &title, int globalLikelihood, const QVector<KillChainAction> &actions, int width)
{
    KillChainWidget w;
    w.setData(title, globalLikelihood, actions);
    w.resize(width, std::max(w.minimumHeight(), 240));
    return w.grab().toImage();
}
