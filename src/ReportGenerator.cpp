// SPDX-License-Identifier: MIT
#include "ReportGenerator.h"

#include "Calc.h"
#include "Widgets.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QJsonArray>
#include <QMap>
#include <QSaveFile>
#include <QSet>
#include <QUrl>

namespace ReportGenerator {
namespace {

using Calc::s;

bool gPreview = false;
bool gEncodeUrls = false;

QString imageLine(const QString &alt, const QString &dir, const QString &file, bool inlineImages)
{
    if (!inlineImages) return "_Figure « " + alt + " » : voir le chapitre « Figures »._\n\n";
    QString u = dir + "/" + file;
    if (gEncodeUrls) u.replace(" ", "%20");
    return "![" + alt + "](" + u + ")\n\n";
}

QString img(const QString &dir, const QString &file)
{
    QString u = dir + "/" + file;
    if (gEncodeUrls) u.replace(" ", "%20");
    return u;
}

QString esc(QString t)
{
    t = t.trimmed();
    if (t.isEmpty()) return QStringLiteral("—");
    t.replace("\r\n", "\n");
    t.replace("|", "\\|");
    t.replace("\n", gPreview ? QStringLiteral(" ; ") : QStringLiteral("<br>"));
    return t;
}

QString mdTable(const QStringList &heads, const QVector<QStringList> &rows)
{
    if (rows.isEmpty()) return QStringLiteral("_Aucun élément._\n\n");
    QString out = "| " + heads.join(" | ") + " |\n|";
    for (int i = 0; i < heads.size(); ++i) out += " --- |";
    out += "\n";
    for (const QStringList &r : rows) {
        QStringList cells;
        for (const QString &c : r) cells << esc(c);
        out += "| " + cells.join(" | ") + " |\n";
    }
    return out + "\n";
}

QString yesNo(bool b) { return b ? QStringLiteral("Oui") : QStringLiteral("Non"); }

QString dict(const QJsonObject &o)
{
    QString t;
    if (o.value("d").toBool()) t += "D";
    if (o.value("i").toBool()) t += "I";
    if (o.value("c").toBool()) t += "C";
    if (o.value("t").toBool()) t += "T";
    return t.isEmpty() ? QStringLiteral("non renseigné") : t;
}

QString heading(int level, const QString &t) { return QString(level, QLatin1Char('#')) + " " + t + "\n\n"; }

// "ID – libellé; ID – libellé"
QString labels(const Project &p, const QString &table, const QString &ids)
{
    QStringList out;
    for (const QString &id : Calc::splitIds(ids)) {
        const QString n = p.name(table, id).simplified();
        out << (n.isEmpty() ? id : id + QStringLiteral(" – ") + n);
    }
    return out.join("; ");
}

QString oneLabel(const Project &p, const QString &table, const QString &id)
{
    return id.isEmpty() ? QString() : labels(p, table, id);
}

QString gLabel(const Project &p, int g)
{
    return g > 0 ? QStringLiteral("%1 – %2").arg(g).arg(p.gravityLabel(g)) : QString();
}

QString vLabel(const Project &p, int v)
{
    return v > 0 ? QStringLiteral("%1 – %2").arg(v).arg(p.likelihoodLabel(v)) : QString();
}

QString num(double d) { return QString::number(d, 'g', 3); }

QString emoji(const QString &level)
{
    if (level == "Faible") return QStringLiteral("🟩");
    if (level == "Moyen") return QStringLiteral("🟨");
    if (level == "Élevé") return QStringLiteral("🟥");
    return QStringLiteral("⬜");
}

// mode 0 = niveaux seuls, 1 = risques initiaux, 2 = risques résiduels
QString matrixMd(const Project &p, int mode)
{
    const QStringList levels = Calc::matrixLevels(p);
    const QStringList texts = mode == 0 ? QStringList() : Calc::matrixTexts(p, mode == 2);
    QString out = "| Gravité \\ Vraisemblance | 1 – " + p.likelihoodLabel(1) + " | 2 – " + p.likelihoodLabel(2) +
                  " | 3 – " + p.likelihoodLabel(3) + " | 4 – " + p.likelihoodLabel(4) + " |\n|---|---|---|---|---|\n";
    for (int g = 4; g >= 1; --g) {
        out += "| **" + QString::number(g) + " – " + p.gravityLabel(g) + "** |";
        for (int v = 1; v <= 4; ++v) {
            const int idx = (g - 1) * 4 + (v - 1);
            QString c = emoji(levels[idx]);
            if (mode == 0) c += " " + levels[idx];
            else if (idx < texts.size() && !texts[idx].isEmpty()) c += " **" + texts[idx] + "**";
            out += " " + c + " |";
        }
        out += "\n";
    }
    return out + "\n";
}

// ---- traçabilité ER -> SS -> SO -> risque
QStringList ssOfEr(const Project &p, const QStringList &erIds)
{
    QStringList out;
    for (const QJsonValue &jv : p.table("ss")) {
        const QJsonObject o = jv.toObject();
        for (const QString &e : Calc::splitIds(s(o, "er")))
            if (erIds.contains(e)) { out << s(o, "id"); break; }
    }
    return out;
}

QVector<QJsonObject> risksOfSs(const Project &p, const QStringList &ssIds)
{
    QVector<QJsonObject> out;
    QSet<QString> seen;
    for (const QJsonValue &sv : p.table("so")) {
        const QJsonObject so = sv.toObject();
        if (!ssIds.contains(s(so, "ss"))) continue;
        for (const QJsonValue &rv : p.table("risques")) {
            const QJsonObject r = rv.toObject();
            if (s(r, "so") != s(so, "id") || seen.contains(s(r, "id"))) continue;
            seen.insert(s(r, "id"));
            out.append(r);
        }
    }
    return out;
}

QStringList erOfVm(const Project &p, const QString &vmId)
{
    QStringList out;
    for (const QJsonValue &jv : p.table("er"))
        if (s(jv.toObject(), "vm") == vmId) out << s(jv.toObject(), "id");
    return out;
}

QStringList vmsOfRisk(const Project &p, const QJsonObject &risk)
{
    QStringList vms;
    const QJsonObject ss = p.find("ss", s(p.find("so", s(risk, "so")), "ss"));
    for (const QString &e : Calc::splitIds(s(ss, "er"))) {
        const QString vm = s(p.find("er", e), "vm");
        if (!vm.isEmpty() && !vms.contains(vm)) vms << vm;
    }
    return vms;
}

QVector<QStringList> riskRows(const Project &p, const QVector<QJsonObject> &risks)
{
    QVector<QStringList> rows;
    for (const QJsonObject &r : risks) {
        const Calc::RiskEval e = Calc::evalRisk(r, p);
        rows.append({s(r, "id"), s(r, "titre"), s(r, "so"), e.ss, e.level,
                     s(r, "niveau_retenu"), e.levelRes, s(r, "acceptabilite")});
    }
    return rows;
}

const QStringList kRiskHeads = {"Risque", "Intitulé", "SO", "SS", "Niveau initial", "Niveau retenu", "Niveau résiduel", "Acceptabilité"};

} // namespace

// ============================================================================ Brouillon de conclusion
QString draftConclusion(const Project &p)
{
    const QString title = s(p.section("sommaire"), "titre");
    int nSr = 0, nOv = 0;
    for (const QJsonValue &jv : p.table("sr")) if (jv.toObject().value("retenue").toBool()) ++nSr;
    for (const QJsonValue &jv : p.table("ov")) if (jv.toObject().value("retenue").toBool()) ++nOv;

    QString t;
    t += QStringLiteral("L'analyse de risques EBIOS RM « %1 » a porté sur %2 valeur(s) métier, %3 bien(s) support et %4 événement(s) redouté(s). "
                        "Elle a retenu %5 source(s) de risque et %6 objectif(s) visé(s), et a conduit à construire %7 scénario(s) stratégique(s), "
                        "%8 scénario(s) opérationnel(s) et %9 scénario(s) de risque.\n\n")
             .arg(title.isEmpty() ? QStringLiteral("(sans titre)") : title)
             .arg(p.table("vm").size()).arg(p.table("bs").size()).arg(p.table("er").size())
             .arg(nSr).arg(nOv).arg(p.table("ss").size()).arg(p.table("so").size()).arg(p.table("risques").size());

    QMap<QString, QStringList> ini, res;
    for (const QJsonValue &jv : p.table("risques")) {
        const QJsonObject r = jv.toObject();
        const Calc::RiskEval e = Calc::evalRisk(r, p);
        ini[e.level] << s(r, "id");
        res[e.levelRes] << s(r, "id");
    }
    auto part = [](const QMap<QString, QStringList> &m, const QString &lvl) {
        const QStringList l = m.value(lvl);
        return l.isEmpty() ? QStringLiteral("aucun") : QStringLiteral("%1 (%2)").arg(l.size()).arg(l.join(", "));
    };
    t += QStringLiteral("**Risques avant traitement** – niveau élevé : %1 ; niveau moyen : %2 ; niveau faible : %3.\n\n")
             .arg(part(ini, "Élevé"), part(ini, "Moyen"), part(ini, "Faible"));
    t += QStringLiteral("**Risques résiduels (après mesures complémentaires)** – niveau élevé : %1 ; niveau moyen : %2 ; niveau faible : %3.\n\n")
             .arg(part(res, "Élevé"), part(res, "Moyen"), part(res, "Faible"));

    QMap<QString, int> st;
    for (const QJsonValue &jv : p.table("mc")) {
        const QString x = s(jv.toObject(), "statut");
        st[x.isEmpty() ? QStringLiteral("statut non renseigné") : x]++;
    }
    if (!st.isEmpty()) {
        QStringList l;
        for (auto it = st.begin(); it != st.end(); ++it) l << QStringLiteral("%1 : %2").arg(it.key()).arg(it.value());
        t += QStringLiteral("**Plan d'action** – %1 mesure(s) de sécurité complémentaire(s) (%2).\n\n").arg(p.table("mc").size()).arg(l.join(" ; "));
    }

    QStringList danger;
    for (const Calc::RadarItem &it : Calc::radarItems(p, false))
        if (it.posture == "Zone de danger") danger << it.name;
    t += danger.isEmpty()
             ? QStringLiteral("**Parties prenantes** – aucune partie prenante n'est en zone de danger.\n\n")
             : QStringLiteral("**Parties prenantes** – en zone de danger : %1.\n\n").arg(danger.join(", "));

    const double c = Calc::compliancePercent(p);
    if (c >= 0) t += QStringLiteral("**Socle de sécurité** – taux de conformité : %1 %.\n\n").arg(c, 0, 'f', 0);

    t += QStringLiteral("**Recommandations** :\n\n"
                        "- mettre en œuvre en priorité les mesures complémentaires qui traitent les risques de niveau élevé ;\n"
                        "- faire valider formellement les risques résiduels par l'autorité d'homologation ;\n"
                        "- réviser cette analyse lors de toute évolution du produit, de son architecture ou du contexte de menace.\n");
    return t;
}

// ============================================================================ Rapport
QString generate(const Project &p, const QString &assetsDir, Mode mode, bool inlineImages)
{
    gPreview = (mode == Mode::Preview);
    gEncodeUrls = (mode == Mode::File);
    QString md;
    const QJsonObject somm = p.section("sommaire");
    const QString title = s(somm, "titre").isEmpty() ? QStringLiteral("Analyse de risques EBIOS RM") : s(somm, "titre");
    const QJsonArray risks = p.table("risques");

    md += "# " + title + "\n\n";
    md += mdTable({"Champ", "Valeur"},
                  {{"Version", s(somm, "version")}, {"Date", s(somm, "date")}, {"Auteur", s(somm, "auteur")},
                   {"Responsable métier", s(somm, "responsable")}, {"Méthode", "EBIOS Risk Manager (ANSSI)"}});
    if (!s(somm, "objectif").isEmpty()) md += heading(2, "Objectif") + s(somm, "objectif").trimmed() + "\n\n";
    if (!s(somm, "documents").isEmpty()) md += heading(2, "Documents associés") + s(somm, "documents").trimmed() + "\n\n";
    if (!s(somm, "historique").isEmpty()) md += heading(2, "Historique") + s(somm, "historique").trimmed() + "\n\n";

    // ------------------------------------------------------------------ Synthèse
    md += heading(2, "Synthèse");
    QMap<QString, int> ini, res;
    for (const QJsonValue &jv : risks) {
        const Calc::RiskEval e = Calc::evalRisk(jv.toObject(), p);
        ini[e.level]++;
        res[e.levelRes]++;
    }
    md += mdTable({"Niveau", "Risques initiaux", "Risques résiduels"},
                  {{"Élevé", QString::number(ini["Élevé"]), QString::number(res["Élevé"])},
                   {"Moyen", QString::number(ini["Moyen"]), QString::number(res["Moyen"])},
                   {"Faible", QString::number(ini["Faible"]), QString::number(res["Faible"])}});
    md += mdTable({"Élément", "Nombre"},
                  {{"Valeurs métiers", QString::number(p.table("vm").size())},
                   {"Biens supports", QString::number(p.table("bs").size())},
                   {"Événements redoutés", QString::number(p.table("er").size())},
                   {"Sources de risque", QString::number(p.table("sr").size())},
                   {"Objectifs visés", QString::number(p.table("ov").size())},
                   {"Couples SR/OV", QString::number(p.table("couples").size())},
                   {"Parties prenantes", QString::number(p.table("pp").size())},
                   {"Scénarios stratégiques", QString::number(p.table("ss").size())},
                   {"Scénarios opérationnels", QString::number(p.table("so").size())},
                   {"Actions élémentaires", QString::number(p.table("actions").size())},
                   {"Scénarios de risque", QString::number(risks.size())},
                   {"Mesures complémentaires", QString::number(p.table("mc").size())}});
    const double comp = Calc::compliancePercent(p);
    if (comp >= 0) md += "Conformité au socle de sécurité : **" + QString::number(comp, 'f', 0) + " %**.\n\n";

    // ------------------------------------------------------------------ Échelles
    md += heading(2, "Méthode et échelles");
    md += mdTable({"Niveau", "Gravité", "Vraisemblance"},
                  {{"1", p.gravityLabel(1), p.likelihoodLabel(1)}, {"2", p.gravityLabel(2), p.likelihoodLabel(2)},
                   {"3", p.gravityLabel(3), p.likelihoodLabel(3)}, {"4", p.gravityLabel(4), p.likelihoodLabel(4)}});
    md += "Matrice de niveau de risque (gravité × vraisemblance) :\n\n" + matrixMd(p, 0);
    md += "Seuils de la cartographie de menace : zone de veille > " + num(p.zone("z_watch")) + " ; zone de contrôle ≥ " + num(p.zone("z_control")) +
          " ; zone de danger > " + num(p.zone("z_danger")) + " ; hors périmètre ≤ " + num(p.zone("z_watch")) + ".\n\n";

    // ================================================================== Atelier 1
    md += heading(2, "Atelier 1 – Cadrage et socle de sécurité");

    md += heading(3, "1.1 Valeurs métiers");
    {
        QVector<QStringList> rows;
        for (const QJsonValue &jv : p.table("vm")) {
            const QJsonObject o = jv.toObject();
            rows.append({s(o, "id"), s(o, "titre"), s(o, "famille"), s(o, "description"), dict(o),
                         QString::number(Calc::splitIds(s(o, "bs")).size()), QString::number(erOfVm(p, s(o, "id")).size())});
        }
        md += mdTable({"ID", "Valeur métier", "Famille de processus", "Description", "DICT", "Nb biens supports", "Nb ER"}, rows);
    }
    md += "Fiche détaillée de chaque valeur métier :\n\n";
    for (const QJsonValue &jv : p.table("vm")) {
        const QJsonObject vm = jv.toObject();
        const QString id = s(vm, "id");
        md += heading(4, id + " – " + s(vm, "titre"));
        if (!s(vm, "famille").trimmed().isEmpty()) md += "- **Famille de processus** : " + s(vm, "famille").trimmed() + "\n";
        md += "- **Critères de sécurité (DICT)** : " + dict(vm) + "\n";
        md += "- **Description** : " + (s(vm, "description").trimmed().isEmpty() ? QStringLiteral("—") : s(vm, "description").trimmed().replace("\n", " ")) + "\n\n";

        md += "**Biens supports associés**\n\n";
        QVector<QStringList> brows;
        for (const QString &bid : Calc::splitIds(s(vm, "bs"))) {
            const QJsonObject b = p.find("bs", bid);
            brows.append({bid, b.isEmpty() ? QStringLiteral("(introuvable)") : s(b, "nom"), s(b, "type"), b.isEmpty() ? QString() : dict(b), s(b, "description")});
        }
        md += mdTable({"ID", "Bien support", "Type", "DICT", "Description"}, brows);

        md += "**Événements redoutés**\n\n";
        QVector<QStringList> erows;
        const QStringList ers = erOfVm(p, id);
        for (const QString &eid : ers) {
            const QJsonObject e = p.find("er", eid);
            erows.append({eid, s(e, "titre"), s(e, "impacts"), gLabel(p, Calc::toInt(s(e, "niveau")))});
        }
        md += mdTable({"ID", "Événement redouté", "Impacts", "Gravité"}, erows);

        md += "**Risques associés** (via événements redoutés → scénarios stratégiques → opérationnels → risques)\n\n";
        md += mdTable(kRiskHeads, riskRows(p, risksOfSs(p, ssOfEr(p, ers))));
    }

    md += heading(3, "1.2 Biens supports");
    {
        QVector<QStringList> rows;
        for (const QJsonValue &jv : p.table("bs")) {
            const QJsonObject o = jv.toObject();
            QStringList vms;
            for (const QJsonValue &vv : p.table("vm"))
                if (Calc::splitIds(s(vv.toObject(), "bs")).contains(s(o, "id"))) vms << s(vv.toObject(), "id");
            rows.append({s(o, "id"), s(o, "nom"), s(o, "type"), s(o, "description"), dict(o), vms.join(", ")});
        }
        md += mdTable({"ID", "Bien support", "Type", "Description", "DICT", "Valeurs métiers"}, rows);
    }

    md += heading(3, "1.3 Événements redoutés");
    {
        QVector<QStringList> rows;
        for (const QJsonValue &jv : p.table("er")) {
            const QJsonObject o = jv.toObject();
            rows.append({s(o, "id"), oneLabel(p, "vm", s(o, "vm")), s(o, "titre"), s(o, "impacts"), gLabel(p, Calc::toInt(s(o, "niveau")))});
        }
        md += mdTable({"ID", "Valeur métier", "Événement redouté", "Impacts", "Gravité"}, rows);
    }

    md += heading(3, "1.4 Socle de sécurité");
    {
        QVector<QStringList> rows;
        for (const QJsonValue &jv : p.table("socle")) {
            const QJsonObject o = jv.toObject();
            rows.append({s(o, "id"), s(o, "nom"), s(o, "regle"), yesNo(o.value("obligatoire").toBool()), s(o, "contenu"), s(o, "porteur"),
                         s(o, "priorite"), s(o, "docs"), s(o, "conformite"), s(o, "cible"), s(o, "echeance"), s(o, "preuves"), s(o, "commentaire")});
        }
        md += mdTable({"Politique", "Référentiel", "Règle", "Obligatoire", "Contenu", "Porteur", "Priorité", "Documents liés",
                       "Conformité", "Cible", "Échéance", "Preuves", "Commentaire"}, rows);
        if (comp >= 0) md += "**Taux de conformité : " + QString::number(comp, 'f', 0) + " %** (Conforme = 100 %, Partiellement conforme = 50 %, hors « Non applicable »).\n\n";
    }

    md += heading(3, "1.5 Vulnérabilités");
    {
        QVector<QStringList> rows;
        for (const QJsonValue &jv : p.table("vuln")) {
            const QJsonObject o = jv.toObject();
            QStringList cibles;
            for (const QString &id : Calc::splitIds(s(o, "cible")))
                cibles << (!p.find("bs", id).isEmpty() ? labels(p, "bs", id) : labels(p, "pp", id));
            rows.append({s(o, "id"), s(o, "nom"), cibles.join("; ")});
        }
        md += mdTable({"ID", "Vulnérabilité", "Bien support / partie prenante"}, rows);
    }

    const QJsonObject arch = p.section("architecture");
    md += heading(3, "1.6 Architecture");
    if (!s(arch, "description").isEmpty()) md += s(arch, "description").trimmed() + "\n\n";
    if (!assetsDir.isEmpty() && !s(arch, "image").isEmpty()) md += imageLine("Architecture", assetsDir, "architecture.png", inlineImages);

    // ================================================================== Atelier 2
    md += heading(2, "Atelier 2 – Sources de risque");
    md += heading(3, "2.1 Sources de risque");
    {
        QVector<QStringList> rows;
        for (const QJsonValue &jv : p.table("sr")) {
            const QJsonObject o = jv.toObject();
            rows.append({s(o, "id"), s(o, "nom"), s(o, "description"), yesNo(o.value("retenue").toBool()), s(o, "justification")});
        }
        md += mdTable({"ID", "Source de risque", "Description", "Retenue", "Justification"}, rows);
    }
    md += heading(3, "2.2 Objectifs visés");
    {
        QVector<QStringList> rows;
        for (const QJsonValue &jv : p.table("ov")) {
            const QJsonObject o = jv.toObject();
            rows.append({s(o, "id"), s(o, "nom"), s(o, "description"), yesNo(o.value("retenue").toBool()), s(o, "justification")});
        }
        md += mdTable({"ID", "Finalité", "Description", "Retenu", "Justification"}, rows);
    }
    md += heading(3, "2.3 Couples source de risque / objectif visé");
    {
        QVector<QStringList> rows;
        for (const QJsonValue &jv : p.table("couples")) {
            const QJsonObject o = jv.toObject();
            rows.append({s(o, "id"), oneLabel(p, "sr", s(o, "sr")), oneLabel(p, "ov", s(o, "ov")), s(o, "objectif"),
                         s(o, "motivation"), s(o, "ressources"), Calc::couplePertinence(s(o, "motivation"), s(o, "ressources")),
                         yesNo(o.value("retenu").toBool()), s(o, "justification")});
        }
        md += mdTable({"ID", "Source de risque", "Objectif visé", "Description de l'objectif", "Motivation", "Ressources", "Pertinence", "Retenu", "Justification"}, rows);
    }

    // ================================================================== Atelier 3
    md += heading(2, "Atelier 3 – Scénarios stratégiques");
    md += heading(3, "3.1 Parties prenantes et radar de la menace");
    md += "Niveau de menace = (dépendance × pénétration) / (maturité cyber × confiance).\n\n";
    {
        QVector<QStringList> rows;
        for (const QJsonValue &jv : p.table("pp")) {
            const QJsonObject o = jv.toObject();
            bool ok = false;
            const double sc = Calc::ppScore(o, false, &ok);
            rows.append({s(o, "id"), s(o, "categorie"), s(o, "nom"), s(o, "mesures"), s(o, "dependance"), s(o, "penetration"),
                         num(Calc::ppExposure(o, false)), s(o, "maturite"), s(o, "confiance"), num(Calc::ppReliability(o, false)),
                         ok ? num(sc) : QString(), ok ? Calc::posture(sc, p) : QString(), yesNo(o.value("pertinence").toBool()), s(o, "commentaire")});
        }
        md += mdTable({"ID", "Catégorie", "Partie prenante", "Mesures existantes", "Dép.", "Pén.", "Expo.", "Mat.", "Conf.", "Fiab.", "Score", "Posture", "Pertinente", "Commentaires"}, rows);
    }
    if (!assetsDir.isEmpty()) md += imageLine("Radar de la menace – situation actuelle", assetsDir, "radar_actuel.png", inlineImages);

    md += heading(3, "3.2 Projection après mesures de sécurité complémentaires");
    {
        QVector<QStringList> rows;
        for (const QJsonValue &jv : p.table("pp")) {
            const QJsonObject o = jv.toObject();
            bool ok0 = false, ok1 = false;
            const double s0 = Calc::ppScore(o, false, &ok0), s1 = Calc::ppScore(o, true, &ok1);
            rows.append({s(o, "id"), s(o, "nom"), s(o, "mesure_comp"),
                         s(o, "p_dependance"), s(o, "p_penetration"), s(o, "p_maturite"), s(o, "p_confiance"),
                         ok0 ? num(s0) + " – " + Calc::posture(s0, p) : QString(), ok1 ? num(s1) + " – " + Calc::posture(s1, p) : QString()});
        }
        md += mdTable({"ID", "Partie prenante", "Mesure complémentaire", "Dép.", "Pén.", "Mat.", "Conf.", "Menace actuelle", "Menace projetée"}, rows);
    }
    if (!assetsDir.isEmpty()) md += imageLine("Radar de la menace – après mesures complémentaires", assetsDir, "radar_projection.png", inlineImages);

    md += heading(3, "3.3 Scénarios stratégiques");
    {
        QVector<QStringList> rows;
        for (const QJsonValue &jv : p.table("ss")) {
            const QJsonObject o = jv.toObject();
            rows.append({s(o, "id"), labels(p, "sr", s(o, "sr")), oneLabel(p, "ov", s(o, "ov")), labels(p, "pp", s(o, "pp")),
                         s(o, "titre"), s(o, "chemin"), labels(p, "er", s(o, "er")), gLabel(p, Calc::ssGravity(o, p))});
        }
        md += mdTable({"ID", "Sources de risque", "Objectif visé", "Parties prenantes", "Intitulé", "Chemin d'attaque", "Événements redoutés", "Gravité"}, rows);
    }

    // ================================================================== Atelier 4
    md += heading(2, "Atelier 4 – Scénarios opérationnels");
    md += heading(3, "4.1 Synthèse des scénarios opérationnels");
    {
        QVector<QStringList> rows;
        for (const QJsonValue &jv : p.table("so")) {
            const QJsonObject o = jv.toObject();
            rows.append({s(o, "id"), oneLabel(p, "ss", s(o, "ss")), s(o, "titre"), s(o, "chemin"),
                         QString::number(Calc::soActionCount(s(o, "id"), p)), vLabel(p, Calc::soLikelihood(s(o, "id"), p))});
        }
        md += mdTable({"ID", "Scénario stratégique", "Intitulé", "Chemin d'attaque", "Nb actions", "Vraisemblance"}, rows);
    }
    md += "La vraisemblance globale d'un scénario opérationnel est la plus basse des vraisemblances de ses actions élémentaires.\n\n";
    md += heading(3, "4.2 Détail des actions élémentaires");
    for (const QJsonValue &jv : p.table("so")) {
        const QJsonObject o = jv.toObject();
        const QString id = s(o, "id");
        md += heading(4, id + " – " + s(o, "titre"));
        md += "Scénario stratégique : " + oneLabel(p, "ss", s(o, "ss")) + "  \nVraisemblance globale : **" +
              (vLabel(p, Calc::soLikelihood(id, p)).isEmpty() ? QStringLiteral("non évaluée") : vLabel(p, Calc::soLikelihood(id, p))) + "**\n\n";
        QVector<QStringList> rows;
        for (const QString &phase : {QStringLiteral("Connaître"), QStringLiteral("Entrer"), QStringLiteral("Explorer"), QStringLiteral("Exploiter")}) {
            for (const QJsonValue &av : p.table("actions")) {
                const QJsonObject a = av.toObject();
                if (s(a, "so") != id || s(a, "phase") != phase) continue;
                rows.append({phase, s(a, "action"), vLabel(p, Calc::toInt(s(a, "vraisemblance"))), labels(p, "bs", s(a, "bs"))});
            }
        }
        md += mdTable({"Phase", "Action élémentaire", "Vraisemblance", "Biens supports"}, rows);
    }

    // ================================================================== Atelier 5
    md += heading(2, "Atelier 5 – Traitement du risque");
    md += heading(3, "5.1 Scénarios de risque");
    {
        QVector<QStringList> rows;
        for (const QJsonValue &jv : risks) {
            const QJsonObject r = jv.toObject();
            const Calc::RiskEval e = Calc::evalRisk(r, p);
            rows.append({s(r, "id"), e.sources, oneLabel(p, "so", s(r, "so")), s(r, "titre"), gLabel(p, e.g), vLabel(p, e.v), e.level,
                         s(r, "niveau_retenu"), s(r, "acceptabilite"), s(r, "decision")});
        }
        md += mdTable({"ID", "Sources de risque", "Scénario opérationnel", "Risque", "Gravité", "Vraisemblance", "Niveau initial", "Niveau retenu", "Acceptabilité", "Décisions et actions"}, rows);
    }
    md += heading(3, "5.2 Matrice des risques initiaux");
    md += matrixMd(p, 1);

    md += heading(3, "5.3 Mesures de sécurité complémentaires");
    {
        QVector<QStringList> rows;
        for (const QJsonValue &jv : p.table("mc")) {
            const QJsonObject o = jv.toObject();
            QStringList rs;
            for (const QJsonValue &rv : risks)
                if (Calc::splitIds(s(rv.toObject(), "mesures")).contains(s(o, "id"))) rs << s(rv.toObject(), "id");
            rows.append({s(o, "id"), s(o, "nom"), s(o, "description"), dict(o), s(o, "type"), s(o, "perimetre"), s(o, "responsable"),
                         s(o, "limite"), s(o, "priorite"), s(o, "cout"), s(o, "statut"), rs.join(", "), s(o, "commentaire")});
        }
        md += mdTable({"ID", "Mesure", "Description", "DICT", "Type", "Périmètre", "Responsable", "Échéance", "Priorité", "Coût/complexité", "Statut", "Risques traités", "Commentaire"}, rows);
    }

    md += heading(3, "5.4 Risques résiduels");
    {
        QVector<QStringList> rows;
        for (const QJsonValue &jv : risks) {
            const QJsonObject r = jv.toObject();
            const Calc::RiskEval e = Calc::evalRisk(r, p);
            rows.append({s(r, "id"), s(r, "titre"), gLabel(p, e.gr), vLabel(p, e.vr), e.levelRes, labels(p, "mc", s(r, "mesures")), s(r, "decision_res")});
        }
        md += mdTable({"ID", "Risque", "Gravité résiduelle", "Vraisemblance résiduelle", "Niveau résiduel", "Mesures appliquées", "Décision"}, rows);
    }
    md += heading(3, "5.5 Matrice des risques résiduels");
    md += matrixMd(p, 2);

    // ================================================================== Conclusion (éditable)
    md += heading(2, "Conclusion");
    const QString concl = p.section("conclusion").value("texte").toString().trimmed();
    md += (concl.isEmpty() ? QStringLiteral("_Conclusion à rédiger (page « Conclusion » de l'application)._") : concl) + "\n\n";

    // ================================================================== Annexe
    md += heading(2, "Annexe – Traçabilité des risques");
    md += "Chaîne complète : événement redouté et valeur métier → scénario stratégique → scénario opérationnel → scénario de risque.\n\n";
    {
        QVector<QStringList> rows;
        for (const QJsonValue &jv : risks) {
            const QJsonObject r = jv.toObject();
            const Calc::RiskEval e = Calc::evalRisk(r, p);
            const QJsonObject ss = p.find("ss", e.ss);
            rows.append({s(r, "id"), oneLabel(p, "so", s(r, "so")), oneLabel(p, "ss", e.ss), e.sources, oneLabel(p, "ov", s(ss, "ov")),
                         labels(p, "er", s(ss, "er")), labels(p, "vm", vmsOfRisk(p, r).join(",")), e.level, e.levelRes});
        }
        md += mdTable({"Risque", "SO", "SS", "Sources de risque", "Objectif visé", "Événements redoutés", "Valeurs métiers", "Niveau initial", "Niveau résiduel"}, rows);
    }

    md += "\n---\n_Rapport généré par EBIOS RM Studio._\n";
    gPreview = false;
    gEncodeUrls = false;
    return md;
}

QString dirUrl(const QString &localDir)
{
    return QUrl::fromLocalFile(localDir).toString(QUrl::FullyEncoded);
}

bool writeAssets(const Project &p, const QString &dir, const QSize &radarSize, bool matrices, bool scenarios)
{
    if (!QDir().mkpath(dir)) return false;
    const RadarThresholds th = thresholdsFrom(p);
    bool ok = true;
    ok &= renderRadarImage(Calc::radarItems(p, false), th, QObject::tr("Radar de la menace – situation actuelle"), radarSize).save(dir + "/radar_actuel.png");
    ok &= renderRadarImage(Calc::radarItems(p, true), th, QObject::tr("Radar de la menace – après mesures complémentaires"), radarSize).save(dir + "/radar_projection.png");
    const QByteArray b64 = p.section("architecture").value("image").toString().toLatin1();
    if (!b64.isEmpty()) {
        QImage im;
        im.loadFromData(QByteArray::fromBase64(b64));
        if (!im.isNull()) ok &= im.save(dir + "/architecture.png");
    }
    if (matrices) {
        const QStringList levels = Calc::matrixLevels(p);
        ok &= renderMatrixImage(QObject::tr("Risques initiaux"), levels, Calc::matrixTexts(p, false)).save(dir + "/matrice_initiale.png");
        ok &= renderMatrixImage(QObject::tr("Risques résiduels"), levels, Calc::matrixTexts(p, true)).save(dir + "/matrice_residuelle.png");
    }
    if (scenarios) {
        for (const QJsonValue &sv : p.table("so")) {
            const QJsonObject so = sv.toObject();
            const QString id = s(so, "id");
            QVector<KillChainAction> acts;
            for (const QJsonValue &av : p.table("actions")) {
                const QJsonObject a = av.toObject();
                if (s(a, "so") == id) acts.append({s(a, "phase"), s(a, "action"), Calc::toInt(s(a, "vraisemblance"))});
            }
            if (acts.isEmpty()) continue;
            const QString t = id + QStringLiteral(" : ") + s(so, "titre").simplified();
            ok &= renderKillChainImage(t, Calc::soLikelihood(id, p), acts).save(dir + "/so_" + id + ".png");
        }
    }
    return ok;
}

bool exportToFile(const Project &p, const QString &mdPath, QString *error)
{
    const QFileInfo fi(mdPath);
    const QString assetsName = fi.completeBaseName() + "_assets";
    if (!writeAssets(p, fi.absoluteDir().filePath(assetsName))) {
        if (error) *error = QObject::tr("Impossible d'écrire les images dans le dossier %1").arg(assetsName);
        return false;
    }
    QSaveFile f(mdPath);
    if (!f.open(QIODevice::WriteOnly)) {
        if (error) *error = f.errorString();
        return false;
    }
    f.write(generate(p, assetsName, Mode::File).toUtf8());
    if (!f.commit()) {
        if (error) *error = f.errorString();
        return false;
    }
    return true;
}

} // namespace ReportGenerator
