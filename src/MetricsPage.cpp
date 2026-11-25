// SPDX-License-Identifier: MIT
#include "MetricsPage.h"

#include "Calc.h"
#include "UiUtil.h"
#include "ui_MetricsPage.h"

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QJsonArray>

namespace {

// Référentiel (feuille « Métriques » du classeur d'origine)
const char *kReferenceHtml = R"HTML(
<h2>Référentiel de métriques EBIOS RM</h2>
<h3>Niveaux de risque</h3>
<table border="1" cellpadding="4" cellspacing="0">
<tr><td><b>Faible</b></td><td>Aucune action n'est à entreprendre.</td></tr>
<tr><td><b>Moyen</b></td><td>Un suivi en termes de gestion du risque est à mener et des actions sont à mettre en place dans le cadre d'une amélioration continue sur le moyen et long terme.</td></tr>
<tr><td><b>Élevé</b></td><td>Des mesures de réduction du risque doivent impérativement être prises à court terme. Dans le cas contraire, la totalité ou une partie de l'activité sera refusée.</td></tr>
</table>
<h3>Critères de sécurité</h3>
<ul>
<li><b>Disponibilité (D)</b> : aptitude d'un système d'information à pouvoir être employé par les utilisateurs habilités dans les conditions d'accès et d'usage normalement prévues.</li>
<li><b>Intégrité (I)</b> : propriété assurant qu'une information ou son traitement n'a pas été modifié ou détruit de façon accidentelle ou non autorisée.</li>
<li><b>Confidentialité (C)</b> : propriété selon laquelle une information ne peut être divulguée ou rendue accessible à des entités n'ayant pas à la connaître.</li>
<li><b>Traçabilité (T)</b> : fait de créer, d'enregistrer et de préserver les données relatives aux évènements générés ou exécutés par le SI ou par un utilisateur.</li>
</ul>
<h3>Échelle de gravité (axes d'impact)</h3>
<table border="1" cellpadding="4" cellspacing="0">
<tr><th>Axe</th><th>1 – Négligeable</th><th>2 – Significatif</th><th>3 – Important</th><th>4 – Catastrophique</th></tr>
<tr><td><b>Opérationnel</b></td>
<td>Perturbation faible d'un processus ne remettant pas en cause le respect d'un calendrier ou la réalisation d'un service</td>
<td>Perturbation sensible de plusieurs processus, sans remettre en cause leur viabilité ou un calendrier</td>
<td>Désorganisation très importante d'un ou plusieurs processus, susceptible de remettre en cause leur viabilité ou un calendrier</td>
<td>Désorganisation durable et non maîtrisée d'un ou plusieurs processus, engageant gravement la responsabilité de l'établissement</td></tr>
<tr><td><b>Juridique</b></td>
<td>Plainte(s) d'une partie prenante signalant un dysfonctionnement</td>
<td>Plainte(s) signalant un dysfonctionnement grave, ou débouchant sur un recours</td>
<td>Plainte(s) débouchant sur une sanction disciplinaire à l'encontre d'un responsable</td>
<td>Plainte(s) débouchant sur la condamnation civile ou pénale d'un responsable</td></tr>
<tr><td><b>Image</b></td>
<td>Impact faible en termes d'atteinte à l'image (1 ou 2 collaborateurs ou candidats)</td>
<td>Altération sensible de l'image de la société (plusieurs collaborateurs ou candidats, ou 1 client mineur)</td>
<td>Altération critique de l'image du groupe (plusieurs clients mineurs ou 1 client majeur)</td>
<td>Altération définitive de l'image du groupe</td></tr>
<tr><td><b>Financier</b></td>
<td>Pertes non significatives sur le plan financier</td>
<td>Pertes &lt; 1 % du budget global</td>
<td>Pertes de l'ordre de 1 % du budget global</td>
<td>Pertes &gt; 1 % du budget global du mois</td></tr>
</table>
<h3>Échelle de vraisemblance</h3>
<table border="1" cellpadding="4" cellspacing="0">
<tr><td>1 – Invraisemblable</td><td>La source de risque a très peu de chances d'atteindre son objectif en empruntant l'un des modes opératoires envisagés.</td></tr>
<tr><td>2 – Moyennement vraisemblable</td><td>La vraisemblance du scénario de risque est très faible.</td></tr>
<tr><td>3 – Vraisemblable</td><td>La source de risque est susceptible d'atteindre son objectif en empruntant l'un des modes opératoires envisagés.</td></tr>
<tr><td>4 – Très vraisemblable</td><td>La vraisemblance du scénario de risque est significative.</td></tr>
</table>
<h3>Échelles D / I / C / T</h3>
<table border="1" cellpadding="4" cellspacing="0">
<tr><th>Niveau</th><th>Disponibilité</th><th>Intégrité</th><th>Confidentialité</th><th>Traçabilité</th></tr>
<tr><td>1</td><td>Faible : indisponibilité entre 1 semaine et 1 mois</td><td>Altérable</td><td>Public</td><td>Pas de besoin</td></tr>
<tr><td>2</td><td>Standard : entre 3 jours et 1 semaine</td><td>Détectable</td><td>Restreint</td><td>Informelle</td></tr>
<tr><td>3</td><td>Élevé : entre 72 h et 4 h</td><td>Maîtrisé</td><td>Confidentiel</td><td>Formelle</td></tr>
<tr><td>4</td><td>Forte : ne doit pas dépasser 4 h continues</td><td>Intègre (aucune altération acceptée)</td><td>Secret (accès nominatif)</td><td>Preuve opposable</td></tr>
</table>
<h3>Couples SR/OV : motivation, ressources, pertinence</h3>
<p>Motivation et ressources : Faible [+], Moyenne [++], Élevée [+++]. Pertinence calculée = motivation + ressources
(somme ≤ 3 : Faible ; = 4 : Moyenne ; ≥ 5 : Élevée), règle déduite du classeur d'origine.</p>
<h3>Parties prenantes : échelles de cotation (1 à 4)</h3>
<table border="1" cellpadding="4" cellspacing="0">
<tr><th>Niv.</th><th>Dépendance</th><th>Pénétration</th><th>Maturité cyber</th><th>Confiance</th></tr>
<tr><td>1</td><td>Relation non nécessaire aux fonctions stratégiques</td><td>Pas d'accès ou accès de type utilisateur à des terminaux utilisateurs</td><td>Règles d'hygiène appliquées ponctuellement, non formalisées</td><td>Intentions ne pouvant être évaluées</td></tr>
<tr><td>2</td><td>Relation utile aux fonctions stratégiques</td><td>Accès administrateur à des terminaux utilisateurs, ou accès physique aux sites</td><td>Hygiène et réglementation prises en compte, sans politique globale (mode réactif)</td><td>Intentions considérées comme neutres</td></tr>
<tr><td>3</td><td>Relation indispensable mais non exclusive</td><td>Accès administrateur à des serveurs « métier »</td><td>Politique globale appliquée, mode réactif avec recherche d'anticipation</td><td>Intentions connues et probablement positives</td></tr>
<tr><td>4</td><td>Relation indispensable et unique</td><td>Accès administrateur à des équipements d'infrastructure, ou accès physique aux salles serveurs</td><td>Politique de management du risque intégrée et proactive</td><td>Intentions parfaitement connues et compatibles</td></tr>
</table>
<p><b>Niveau de menace</b> = (dépendance × pénétration) / (maturité cyber × confiance).<br/>
Zones : danger &gt; seuil danger ; contrôle ≥ seuil contrôle ; veille &gt; seuil veille ; sinon hors périmètre.</p>
)HTML";

} // namespace

MetricsPage::MetricsPage(Project *p, QWidget *parent) : QWidget(parent), ui(new Ui::MetricsPage), project_(p)
{
    ui->setupUi(this);

    QLineEdit *g[4] = {ui->gravityEdit1, ui->gravityEdit2, ui->gravityEdit3, ui->gravityEdit4};
    QLineEdit *v[4] = {ui->likelihoodEdit1, ui->likelihoodEdit2, ui->likelihoodEdit3, ui->likelihoodEdit4};
    QDoubleSpinBox *z[4] = {ui->spinHp, ui->spinWatch, ui->spinControl, ui->spinDanger};
    for (int i = 0; i < 4; ++i) {
        g_[i] = g[i];
        v_[i] = v[i];
        z_[i] = z[i];
        connect(g_[i], &QLineEdit::textChanged, this, &MetricsPage::commit);
        connect(v_[i], &QLineEdit::textChanged, this, &MetricsPage::commit);
        connect(z_[i], QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MetricsPage::commit);
    }
    for (int gr = 1; gr <= 4; ++gr) {
        for (int vr = 1; vr <= 4; ++vr) {
            auto *cb = findChild<QComboBox *>(QStringLiteral("matrixCombo_%1_%2").arg(gr).arg(vr));
            Q_ASSERT(cb);
            cb->addItems(QStringList{QStringLiteral("Faible"), QStringLiteral("Moyen"), QStringLiteral("Élevé")});
            mat_[(gr - 1) * 4 + (vr - 1)] = cb;
            connect(cb, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MetricsPage::commit);
        }
    }
    ui->referenceBrowser->setHtml(QString::fromUtf8(kReferenceHtml));

    connect(project_, &Project::changed, this, &MetricsPage::load);
    load();
}

MetricsPage::~MetricsPage()
{
    delete ui;
}

void MetricsPage::load()
{
    if (inEdit_) return;
    loading_ = true;
    const QJsonObject m = project_->section("metrics");
    const QJsonArray ga = m.value("g").toArray(), va = m.value("v").toArray(), ma = m.value("matrix").toArray();
    for (int i = 0; i < 4; ++i) {
        setIfDifferent(g_[i], i < ga.size() ? ga.at(i).toString() : QString());
        setIfDifferent(v_[i], i < va.size() ? va.at(i).toString() : QString());
    }
    const char *keys[4] = {"z_hp", "z_watch", "z_control", "z_danger"};
    for (int i = 0; i < 4; ++i) z_[i]->setValue(m.value(keys[i]).toDouble());
    for (int i = 0; i < 16; ++i) {
        const int idx = mat_[i]->findText(i < ma.size() ? ma.at(i).toString() : QString());
        mat_[i]->setCurrentIndex(idx < 0 ? 0 : idx);
    }
    loading_ = false;
}

void MetricsPage::commit()
{
    if (loading_) return;
    QJsonObject m = project_->section("metrics");
    QJsonArray ga, va, ma;
    for (int i = 0; i < 4; ++i) { ga.append(g_[i]->text()); va.append(v_[i]->text()); }
    for (int i = 0; i < 16; ++i) ma.append(mat_[i]->currentText());
    m["g"] = ga;
    m["v"] = va;
    m["matrix"] = ma;
    const char *keys[4] = {"z_hp", "z_watch", "z_control", "z_danger"};
    for (int i = 0; i < 4; ++i) m[keys[i]] = z_[i]->value();
    inEdit_ = true;
    project_->setSection("metrics", m);
    inEdit_ = false;
}
