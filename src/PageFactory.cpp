// SPDX-License-Identifier: MIT
#include "PageFactory.h"

#include "Calc.h"
#include "TablePage.h"

#include <QJsonArray>
#include <QObject>

namespace {
const QStringList kL14 = {"1", "2", "3", "4"};
const QStringList kLevels = {"Faible", "Moyen", "Élevé"};
} // namespace

// ====================================================================== Fabriques de pages
namespace Pages {
namespace {

Column dictCols(const QString &k, const QString &t) { return Column::check(k, t, 36); }

Column grav(const QString &key, const QString &title, std::function<int(const QJsonObject &, const Project &)> fn)
{
    return Column::computed(key, title, [fn](const QJsonObject &o, const Project &p) {
        const int g = fn(o, p);
        return g > 0 ? QStringLiteral("%1 – %2").arg(g).arg(p.gravityLabel(g)) : QString();
    }, 150);
}

Column ppComputed(const QString &key, const QString &title, bool proj, int what)
{
    return Column::computed(key, title, [proj, what](const QJsonObject &o, const Project &p) -> QString {
        bool ok = false;
        const double sc = Calc::ppScore(o, proj, &ok);
        switch (what) {
        case 0: { const double e = Calc::ppExposure(o, proj); return e > 0 ? QString::number(e) : QString(); }
        case 1: { const double f = Calc::ppReliability(o, proj); return f > 0 ? QString::number(f) : QString(); }
        case 2: return ok ? QString::number(sc, 'f', 3) : QString();
        default: return ok ? Calc::posture(sc, p) : QString();
        }
    }, what == 3 ? 130 : 80);
}

} // namespace

QWidget *valeursMetiers(Project *p)
{
    return new TablePage(p, "vm", QObject::tr("Valeurs métiers"),
        QObject::tr("Atelier 1 – Identifier les processus opérationnels pris en charge par la solution, avec leurs besoins de sécurité (D, I, C, T) et les biens supports associés."),
        {Column::text("id", QObject::tr("ID VM"), 70), Column::text("titre", QObject::tr("Intitulé"), 260), Column::text("famille", QObject::tr("Famille de processus"), 130),
         Column::multi("description", QObject::tr("Description"), 300), dictCols("d", "D"), dictCols("i", "I"), dictCols("c", "C"), dictCols("t", "T"),
         Column::multiref("bs", QObject::tr("Biens supports associés"), "bs", 320)}, "VM-");
}

QWidget *biensSupports(Project *p)
{
    return new TablePage(p, "bs", QObject::tr("Biens supports"),
        QObject::tr("Atelier 1 – Ressources matérielles et immatérielles (techniques, réseau, physiques, humaines…) permettant d'exploiter les valeurs métiers."),
        {Column::text("id", QObject::tr("ID"), 70), Column::text("nom", QObject::tr("Bien support"), 200), Column::text("type", QObject::tr("Type"), 170),
         Column::multi("description", QObject::tr("Description"), 320), dictCols("d", "D"), dictCols("i", "I"), dictCols("c", "C"), dictCols("t", "T"),
         Column::computed("vm", QObject::tr("Valeurs métiers"), [](const QJsonObject &o, const Project &pr) {
             QStringList l;
             for (const QJsonValue &v : pr.table("vm"))
                 if (Calc::splitIds(Calc::s(v.toObject(), "bs")).contains(Calc::s(o, "id"))) l << Calc::s(v.toObject(), "id");
             return l.join(", ");
         }, 160)}, "BS-");
}

QWidget *evenementsRedoutes(Project *p)
{
    return new TablePage(p, "er", QObject::tr("Événements redoutés"),
        QObject::tr("Atelier 1 – Craintes de l'organisation concernant les valeurs métiers, avec leurs impacts et leur gravité."),
        {Column::text("id", QObject::tr("ID ER"), 70), Column::ref("vm", QObject::tr("Valeur métier"), "vm", 260), Column::multi("titre", QObject::tr("Événement redouté"), 320),
         Column::multi("impacts", QObject::tr("Impacts"), 320), Column::combo("niveau", QObject::tr("Niveau"), kL14, 70),
         grav("gravite", QObject::tr("Gravité"), [](const QJsonObject &o, const Project &) { return Calc::toInt(Calc::s(o, "niveau")); })}, "ER-");
}

QWidget *socle(Project *p)
{
    auto *t = new TablePage(p, "socle", QObject::tr("Socle de sécurité"),
        QObject::tr("Atelier 1 – Exigences légales, réglementaires et normatives applicables, état de conformité et plan d'action."),
        {Column::text("id", QObject::tr("Politique"), 70), Column::multi("nom", QObject::tr("Nom de la politique"), 300), Column::text("regle", QObject::tr("Règle"), 100),
         Column::check("obligatoire", QObject::tr("Oblig."), 56), Column::multi("contenu", QObject::tr("Contenu"), 200), Column::text("porteur", QObject::tr("Porteur"), 110),
         Column::text("priorite", QObject::tr("Priorité"), 70), Column::multi("docs", QObject::tr("Documents liés"), 200),
         Column::combo("conformite", QObject::tr("Conformité"), {"Conforme", "Partiellement conforme", "Conformité prévue", "Non conforme", "Non applicable"}, 170),
         Column::text("cible", QObject::tr("Cible"), 80), Column::text("echeance", QObject::tr("Échéance"), 90),
         Column::multi("preuves", QObject::tr("Éléments de preuve"), 200), Column::multi("commentaire", QObject::tr("Commentaire"), 300)}, "P-");
    t->setFooter([](const Project &pr) {
        const double c = Calc::compliancePercent(pr);
        return c < 0 ? QObject::tr("Taux de conformité : n/a") : QObject::tr("Taux de conformité : %1 %  (Conforme = 100 %, Partiellement = 50 %)").arg(c, 0, 'f', 0);
    });
    return t;
}

QWidget *vulnerabilites(Project *p)
{
    Column c = Column::multiref("cible", QObject::tr("Bien support / partie prenante"), "bs", 320);
    c.refTables = QStringList{"bs", "pp"};
    return new TablePage(p, "vuln", QObject::tr("Vulnérabilités"),
        QObject::tr("Vulnérabilités du système ou de l'organisation (feuille masquée dans le classeur d'origine)."),
        {Column::text("id", QObject::tr("ID"), 70), Column::multi("nom", QObject::tr("Vulnérabilité"), 420), c}, "V-");
}

QWidget *sourcesRisque(Project *p)
{
    return new TablePage(p, "sr", QObject::tr("Sources de risque"),
        QObject::tr("Atelier 2 – Types d'attaquants envisagés : qui chercherait à attaquer le système, et pourquoi ce type plutôt qu'un autre ?"),
        {Column::text("id", QObject::tr("ID SR"), 70), Column::text("nom", QObject::tr("Source de risque"), 200), Column::multi("description", QObject::tr("Description"), 380),
         Column::check("retenue", QObject::tr("Retenue"), 60), Column::multi("justification", QObject::tr("Justification"), 380)}, "SR-");
}

QWidget *objectifsVises(Project *p)
{
    return new TablePage(p, "ov", QObject::tr("Objectifs visés"),
        QObject::tr("Atelier 2 – Finalités poursuivies par les attaquants : pourquoi attaqueraient-ils le système ?"),
        {Column::text("id", QObject::tr("ID OV"), 70), Column::text("nom", QObject::tr("Finalité"), 160), Column::multi("description", QObject::tr("Description"), 380),
         Column::check("retenue", QObject::tr("Retenu"), 60), Column::multi("justification", QObject::tr("Justification"), 380)}, "OV-");
}

QWidget *couplesSrOv(Project *p)
{
    const QStringList lv = {"Faible", "Moyenne", "Élevée"};
    return new TablePage(p, "couples", QObject::tr("Couples source de risque / objectif visé"),
        QObject::tr("Atelier 2 – Valider les couples SR/OV pertinents. La pertinence est calculée à partir de la motivation et des ressources."),
        {Column::text("id", QObject::tr("ID"), 80), Column::ref("sr", QObject::tr("Source de risque"), "sr", 200), Column::ref("ov", QObject::tr("Objectif visé"), "ov", 160),
         Column::multi("objectif", QObject::tr("Description de l'objectif"), 260), Column::combo("motivation", QObject::tr("Motivation"), lv, 90),
         Column::combo("ressources", QObject::tr("Ressources"), lv, 90),
         Column::computed("pertinence", QObject::tr("Pertinence"), [](const QJsonObject &o, const Project &) {
             return Calc::couplePertinence(Calc::s(o, "motivation"), Calc::s(o, "ressources"));
         }, 90),
         Column::check("retenu", QObject::tr("Retenu"), 56), Column::multi("justification", QObject::tr("Justification"), 340)}, "SR.OV.");
}

QWidget *partiesPrenantes(Project *p)
{
    return new TablePage(p, "pp", QObject::tr("Parties prenantes"),
        QObject::tr("Atelier 3 – Participants directs et indirects à l'activité. Niveau de menace = (dépendance × pénétration) / (maturité cyber × confiance), cotés de 1 à 4."),
        {Column::text("categorie", QObject::tr("Catégorie"), 190), Column::text("id", QObject::tr("ID"), 64), Column::multi("nom", QObject::tr("Partie prenante"), 230),
         Column::multi("mesures", QObject::tr("Mesures existantes"), 240),
         Column::combo("dependance", QObject::tr("Dépend."), kL14, 70), Column::combo("penetration", QObject::tr("Pénétr."), kL14, 70), ppComputed("expo", QObject::tr("Expo."), false, 0),
         Column::combo("maturite", QObject::tr("Maturité"), kL14, 74), Column::combo("confiance", QObject::tr("Confiance"), kL14, 74), ppComputed("fiab", QObject::tr("Fiabilité"), false, 1),
         ppComputed("score", QObject::tr("Score"), false, 2), ppComputed("posture", QObject::tr("Posture"), false, 3),
         Column::check("pertinence", QObject::tr("Pertin."), 56), Column::multi("commentaire", QObject::tr("Commentaires"), 300)}, "PP-");
}

QWidget *partiesPrenantesProjection(Project *p)
{
    auto *t = new TablePage(p, "pp", QObject::tr("Parties prenantes – projection"),
        QObject::tr("Atelier 3 – Effet des mesures de sécurité complémentaires sur l'exposition et la fiabilité cyber. Une valeur vide reprend la valeur actuelle."),
        {Column::computed("id", QObject::tr("ID"), [](const QJsonObject &o, const Project &) { return Calc::s(o, "id"); }, 64),
         Column::computed("nom", QObject::tr("Partie prenante"), [](const QJsonObject &o, const Project &) { return Calc::s(o, "nom").simplified(); }, 240),
         Column::multi("mesure_comp", QObject::tr("Mesure de sécurité complémentaire"), 280),
         Column::combo("p_dependance", QObject::tr("Dépend."), kL14, 70), Column::combo("p_penetration", QObject::tr("Pénétr."), kL14, 70), ppComputed("expo", QObject::tr("Expo."), true, 0),
         Column::combo("p_maturite", QObject::tr("Maturité"), kL14, 74), Column::combo("p_confiance", QObject::tr("Confiance"), kL14, 74), ppComputed("fiab", QObject::tr("Fiabilité"), true, 1),
         ppComputed("score", QObject::tr("Score"), true, 2), ppComputed("posture", QObject::tr("Posture"), true, 3)}, "PP-");
    t->setStructureEditable(false);
    return t;
}

QWidget *scenariosStrategiques(Project *p)
{
    return new TablePage(p, "ss", QObject::tr("Scénarios stratégiques"),
        QObject::tr("Atelier 3 – Chemins d'attaque de haut niveau d'une source de risque vers son objectif. La gravité est celle saisie, à défaut la plus élevée des ER associés."),
        {Column::text("id", QObject::tr("ID"), 64), Column::multiref("sr", QObject::tr("Sources de risque"), "sr", 200), Column::ref("ov", QObject::tr("Objectif visé"), "ov", 150),
         Column::multiref("pp", QObject::tr("Parties prenantes"), "pp", 150), Column::multi("titre", QObject::tr("Intitulé"), 260), Column::multi("chemin", QObject::tr("Chemin d'attaque"), 320),
         Column::multiref("er", QObject::tr("Événements redoutés"), "er", 180), Column::combo("gravite", QObject::tr("Gravité saisie"), kL14, 90),
         grav("gretenue", QObject::tr("Gravité retenue"), [](const QJsonObject &o, const Project &pr) { return Calc::ssGravity(o, pr); })}, "SS-");
}

QWidget *scenariosOperationnels(Project *p)
{
    return new TablePage(p, "so", QObject::tr("Scénarios opérationnels"),
        QObject::tr("Atelier 4 – Méthodes d'attaque de bas niveau. La vraisemblance provient des actions élémentaires (vue détaillée) ; sinon la valeur manuelle est utilisée."),
        {Column::text("id", QObject::tr("ID"), 64), Column::ref("ss", QObject::tr("Scénario stratégique"), "ss", 260), Column::multi("titre", QObject::tr("Intitulé"), 300),
         Column::multi("chemin", QObject::tr("Chemin d'attaque"), 360), Column::combo("vraisemblance_manuelle", QObject::tr("Vraisemb. manuelle"), kL14, 110),
         Column::computed("nb", QObject::tr("Actions"), [](const QJsonObject &o, const Project &pr) { return QString::number(Calc::soActionCount(Calc::s(o, "id"), pr)); }, 60),
         Column::computed("v", QObject::tr("Vraisemblance"), [](const QJsonObject &o, const Project &pr) {
             const int v = Calc::soLikelihood(Calc::s(o, "id"), pr);
             return v > 0 ? QStringLiteral("%1 – %2").arg(v).arg(pr.likelihoodLabel(v)) : QString();
         }, 170)}, "SO");
}

QWidget *scenariosRisque(Project *p)
{
    auto ev = [](const QJsonObject &o, const Project &pr) { return Calc::evalRisk(o, pr); };
    return new TablePage(p, "risques", QObject::tr("Scénarios de risque"),
        QObject::tr("Atelier 5 – Croisement de la gravité (scénario stratégique) et de la vraisemblance (scénario opérationnel). Le niveau initial est calculé par la matrice ; le niveau retenu peut être ajusté."),
        {Column::text("id", QObject::tr("ID"), 56), Column::ref("so", QObject::tr("Scénario opérationnel"), "so", 260), Column::multi("titre", QObject::tr("Intitulé du risque"), 280),
         Column::computed("src", QObject::tr("Sources de risque"), [ev](const QJsonObject &o, const Project &pr) { return ev(o, pr).sources; }, 180),
         grav("g", QObject::tr("Gravité initiale"), [ev](const QJsonObject &o, const Project &pr) { return ev(o, pr).g; }),
         Column::computed("v", QObject::tr("Vraisemblance initiale"), [ev](const QJsonObject &o, const Project &pr) {
             const int v = ev(o, pr).v; return v > 0 ? QStringLiteral("%1 – %2").arg(v).arg(pr.likelihoodLabel(v)) : QString(); }, 170),
         Column::computed("niv", QObject::tr("Niveau initial"), [ev](const QJsonObject &o, const Project &pr) { return ev(o, pr).level; }, 90),
         Column::combo("niveau_retenu", QObject::tr("Niveau retenu"), kLevels, 90),
         Column::combo("acceptabilite", QObject::tr("Acceptabilité"), {"Acceptable", "Tolérable sous contrôle", "Inacceptable"}, 160),
         Column::multi("decision", QObject::tr("Décisions et actions"), 320)}, "R");
}

QWidget *mesuresComplementaires(Project *p)
{
    return new TablePage(p, "mc", QObject::tr("Mesures de sécurité complémentaires"),
        QObject::tr("Atelier 5 – Plan de gestion des risques : mesures qui réduisent la gravité ou la vraisemblance, avec responsable, échéance et statut."),
        {Column::text("id", QObject::tr("ID"), 70), Column::text("nom", QObject::tr("Mesure"), 220), Column::multi("description", QObject::tr("Description"), 320),
         dictCols("d", "D"), dictCols("i", "I"), dictCols("c", "C"), dictCols("t", "T"),
         Column::combo("type", QObject::tr("Type"), {"Technique", "Organisationnelle", "Physique"}, 120), Column::multi("perimetre", QObject::tr("Périmètre"), 160),
         Column::text("responsable", QObject::tr("Responsable"), 90), Column::text("limite", QObject::tr("Limite temporelle"), 100),
         Column::combo("priorite", QObject::tr("Priorité"), {"P1", "P2", "P3"}, 64), Column::text("cout", QObject::tr("Coût/complexité"), 90),
         Column::combo("statut", QObject::tr("Statut"), {"À débuter", "À vérifier", "En cours", "Fait", "Annulé"}, 100),
         Column::multi("commentaire", QObject::tr("Commentaire"), 240)}, "MC-");
}

QWidget *risquesResiduels(Project *p)
{
    auto ev = [](const QJsonObject &o, const Project &pr) { return Calc::evalRisk(o, pr); };
    auto *t = new TablePage(p, "risques", QObject::tr("Risques résiduels"),
        QObject::tr("Atelier 5 – Gravité et vraisemblance résiduelles après application des mesures complémentaires. Une valeur vide reprend la valeur initiale."),
        {Column::computed("id", QObject::tr("ID"), [](const QJsonObject &o, const Project &) { return Calc::s(o, "id"); }, 56),
         Column::computed("src", QObject::tr("Sources de risque"), [ev](const QJsonObject &o, const Project &pr) { return ev(o, pr).sources; }, 180),
         Column::computed("titre", QObject::tr("Scénario de risque"), [](const QJsonObject &o, const Project &) { return Calc::s(o, "titre").simplified(); }, 300),
         Column::combo("g_res", QObject::tr("Gravité résiduelle"), kL14, 100), Column::combo("v_res", QObject::tr("Vraisemb. résiduelle"), kL14, 100),
         Column::computed("niv", QObject::tr("Niveau résiduel"), [ev](const QJsonObject &o, const Project &pr) { return ev(o, pr).levelRes; }, 100),
         Column::multiref("mesures", QObject::tr("Mesures appliquées"), "mc", 240), Column::multi("decision_res", QObject::tr("Décision"), 300)}, "R");
    t->setStructureEditable(false);
    return t;
}

} // namespace Pages
