// SPDX-License-Identifier: MIT
#pragma once
#include <QColor>
#include <QJsonObject>
#include <QString>
#include <QStringList>
#include <QVector>

#include "Project.h"

// Règles de calcul EBIOS RM (reprises du classeur Excel d'origine).
namespace Calc {

// ---------- utilitaires ----------
QString str(const QJsonValue &v);                          // valeur JSON -> texte
QString s(const QJsonObject &o, const QString &key);       // o[key] en texte
int toInt(const QString &text);                            // 0 si vide / invalide
QStringList splitIds(const QString &text);                 // "A, B;C" -> [A,B,C]
QColor levelColor(const QString &text);                    // couleur d'un niveau / zone / acceptabilité

// ---------- Atelier 2 ----------
QString couplePertinence(const QString &motivation, const QString &ressources);

// ---------- Atelier 3 : parties prenantes ----------
// proj = true : utilise les valeurs de projection (p_*), avec repli sur les valeurs initiales si vides.
int ppValue(const QJsonObject &pp, const QString &key, bool proj);
double ppExposure(const QJsonObject &pp, bool proj);     // dépendance x pénétration (0 si incomplet)
double ppReliability(const QJsonObject &pp, bool proj);  // maturité x confiance   (0 si incomplet)
double ppScore(const QJsonObject &pp, bool proj, bool *ok = nullptr);  // exposition / fiabilité
QString posture(double score, const Project &p);

struct RadarItem {
    QString id, name, category, posture;
    double score = 0.0;
    double exposure = 0.0;
    bool relevant = true;
};
QVector<RadarItem> radarItems(const Project &p, bool projection);

// ---------- Atelier 3 / 4 / 5 ----------
int ssGravityFromER(const QJsonObject &ss, const Project &p);   // max des gravités des ER associés
int ssGravity(const QJsonObject &ss, const Project &p);         // gravité saisie, sinon max des ER
int soLikelihood(const QString &soId, const Project &p);        // min des actions, sinon valeur manuelle
int soActionCount(const QString &soId, const Project &p);

struct RiskEval {
    int g = 0, v = 0;           // initiaux
    int gr = 0, vr = 0;         // résiduels (repli sur initiaux si non saisis)
    QString level, levelRes;    // Faible / Moyen / Élevé
    QString sources;            // noms des sources de risque
    QString ss;                 // id du scénario stratégique
};
RiskEval evalRisk(const QJsonObject &risk, const Project &p);

// Matrice 4x4 (index = (g-1)*4 + (v-1)) : niveaux de la matrice et identifiants des risques placés.
QStringList matrixLevels(const Project &p);
QStringList matrixTexts(const Project &p, bool residual);

// ---------- Socle ----------
double compliancePercent(const Project &p);   // -1 si aucune politique évaluable

} // namespace Calc
