// SPDX-License-Identifier: MIT
#include "Calc.h"

#include <algorithm>
#include <QJsonArray>
#include <QRegularExpression>

namespace Calc {

QString str(const QJsonValue &v)
{
    if (v.isString()) return v.toString();
    if (v.isDouble()) return QString::number(v.toDouble());
    if (v.isBool()) return v.toBool() ? QStringLiteral("true") : QStringLiteral("false");
    return QString();
}

QString s(const QJsonObject &o, const QString &key)
{
    return str(o.value(key));
}

int toInt(const QString &text)
{
    bool ok = false;
    const int v = text.trimmed().toInt(&ok);
    return ok ? v : 0;
}

QStringList splitIds(const QString &text)
{
    return text.split(QRegularExpression(QStringLiteral("[,;\\s]+")), Qt::SkipEmptyParts);
}

QColor levelColor(const QString &t)
{
    if (t == "Faible" || t == "Acceptable" || t == "Zone de veille") return QColor("#c6efce");
    if (t == "Moyen" || t == "Tolérable sous contrôle" || t == "Zone de contrôle") return QColor("#ffeb9c");
    if (t == "Élevé" || t == "Inacceptable" || t == "Zone de danger") return QColor("#ffc7ce");
    if (t == "Hors périmètre") return QColor("#e0e0e0");
    return QColor();
}

// ---------------------------------------------------------------- Atelier 2
static int levelOf(const QString &t)
{
    const QString x = t.trimmed().toLower();
    if (x == "faible") return 1;
    if (x == "moyenne" || x == "moyen") return 2;
    if (x == "élevée" || x == "elevée" || x == "élevé" || x == "elevé") return 3;
    return 0;
}

// Règle déduite des 17 couples du classeur : motivation + ressources
//  <= 3 -> Faible ; 4 -> Moyenne ; >= 5 -> Élevée
QString couplePertinence(const QString &motivation, const QString &ressources)
{
    const int a = levelOf(motivation), b = levelOf(ressources);
    if (!a || !b) return QString();
    const int sum = a + b;
    if (sum <= 3) return QStringLiteral("Faible");
    if (sum == 4) return QStringLiteral("Moyenne");
    return QStringLiteral("Élevée");
}

// ---------------------------------------------------------------- Atelier 3
int ppValue(const QJsonObject &pp, const QString &key, bool proj)
{
    if (proj) {
        const int pv = toInt(s(pp, "p_" + key));
        if (pv > 0) return pv;
    }
    return toInt(s(pp, key));
}

double ppExposure(const QJsonObject &pp, bool proj)
{
    return double(ppValue(pp, "dependance", proj)) * double(ppValue(pp, "penetration", proj));
}

double ppReliability(const QJsonObject &pp, bool proj)
{
    return double(ppValue(pp, "maturite", proj)) * double(ppValue(pp, "confiance", proj));
}

double ppScore(const QJsonObject &pp, bool proj, bool *ok)
{
    const double e = ppExposure(pp, proj), f = ppReliability(pp, proj);
    if (ok) *ok = (e > 0 && f > 0);
    return (e > 0 && f > 0) ? e / f : 0.0;
}

// Même logique que la formule Excel de la colonne « Posture ».
QString posture(double score, const Project &p)
{
    if (score > p.zone("z_danger")) return QStringLiteral("Zone de danger");
    if (score >= p.zone("z_control")) return QStringLiteral("Zone de contrôle");
    if (score > p.zone("z_watch")) return QStringLiteral("Zone de veille");
    return QStringLiteral("Hors périmètre");
}

QVector<RadarItem> radarItems(const Project &p, bool projection)
{
    QVector<RadarItem> out;
    const QJsonArray rows = p.table("pp");
    for (const QJsonValue &v : rows) {
        const QJsonObject o = v.toObject();
        bool ok = false;
        const double sc = ppScore(o, projection, &ok);
        if (!ok) continue;
        RadarItem it;
        it.id = s(o, "id");
        QString n = s(o, "nom");
        const int par = n.indexOf('(');
        if (par > 0) n = n.left(par);
        it.name = n.simplified();
        it.category = s(o, "categorie").simplified();
        if (it.category.isEmpty()) it.category = QStringLiteral("Autres");
        it.score = sc;
        it.exposure = ppExposure(o, projection);
        it.posture = posture(sc, p);
        it.relevant = o.value("pertinence").toBool();
        out.append(it);
    }
    return out;
}

// ---------------------------------------------------------------- Ateliers 3-5
int ssGravityFromER(const QJsonObject &ss, const Project &p)
{
    int mx = 0;
    for (const QString &id : splitIds(s(ss, "er")))
        mx = std::max(mx, toInt(s(p.find("er", id), "niveau")));
    return mx;
}

int ssGravity(const QJsonObject &ss, const Project &p)
{
    const int manual = toInt(s(ss, "gravite"));
    return manual > 0 ? manual : ssGravityFromER(ss, p);
}

int soActionCount(const QString &soId, const Project &p)
{
    int n = 0;
    for (const QJsonValue &v : p.table("actions"))
        if (s(v.toObject(), "so") == soId) ++n;
    return n;
}

int soLikelihood(const QString &soId, const Project &p)
{
    int mn = 0;
    for (const QJsonValue &v : p.table("actions")) {
        const QJsonObject a = v.toObject();
        if (s(a, "so") != soId) continue;
        const int lv = toInt(s(a, "vraisemblance"));
        if (lv > 0) mn = (mn == 0) ? lv : std::min(mn, lv);
    }
    if (mn > 0) return mn;
    return toInt(s(p.find("so", soId), "vraisemblance_manuelle"));
}

RiskEval evalRisk(const QJsonObject &risk, const Project &p)
{
    RiskEval e;
    const QJsonObject so = p.find("so", s(risk, "so"));
    const QJsonObject ss = p.find("ss", s(so, "ss"));
    e.ss = s(ss, "id");
    e.g = ssGravity(ss, p);
    e.v = soLikelihood(s(so, "id"), p);
    e.gr = toInt(s(risk, "g_res"));
    e.vr = toInt(s(risk, "v_res"));
    if (e.gr == 0) e.gr = e.g;
    if (e.vr == 0) e.vr = e.v;
    e.level = p.matrixCell(e.g, e.v);
    e.levelRes = p.matrixCell(e.gr, e.vr);
    QStringList names;
    for (const QString &id : splitIds(s(ss, "sr"))) names << p.name("sr", id);
    e.sources = names.join(", ");
    return e;
}

QStringList matrixLevels(const Project &p)
{
    QStringList out;
    for (int g = 1; g <= 4; ++g)
        for (int v = 1; v <= 4; ++v) out << p.matrixCell(g, v);
    return out;
}

QStringList matrixTexts(const Project &p, bool residual)
{
    QVector<QStringList> cells(16);
    for (const QJsonValue &jv : p.table("risques")) {
        const QJsonObject r = jv.toObject();
        const RiskEval e = evalRisk(r, p);
        const int g = residual ? e.gr : e.g;
        const int v = residual ? e.vr : e.v;
        if (g < 1 || g > 4 || v < 1 || v > 4) continue;
        cells[(g - 1) * 4 + (v - 1)] << s(r, "id");
    }
    QStringList out;
    for (const QStringList &c : cells) out << c.join(", ");
    return out;
}

// ---------------------------------------------------------------- Socle
double compliancePercent(const Project &p)
{
    double total = 0; int n = 0;
    for (const QJsonValue &v : p.table("socle")) {
        const QString c = s(v.toObject(), "conformite");
        if (c.isEmpty() || c == "Non applicable") continue;
        ++n;
        if (c == "Conforme") total += 1.0;
        else if (c == "Partiellement conforme") total += 0.5;
    }
    return n == 0 ? -1.0 : 100.0 * total / n;
}

} // namespace Calc
