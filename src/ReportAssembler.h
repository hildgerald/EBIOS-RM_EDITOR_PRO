// SPDX-License-Identifier: MIT
#pragma once
#include <QImage>
#include <QString>
#include <QVector>

#include "MarkdownModel.h"
#include "Project.h"

// Assemble le rapport « riche » commun aux exports PDF et Word :
// page de garde + sommaire (dessinés par chaque exporteur) + document (chapitre Figures, corps, annexe des schémas).
namespace ReportAssembler {

struct ReportOptions {
    bool cover = true;            // page de garde
    bool toc = true;              // sommaire + liste des figures
    bool figures = true;          // chapitre « Figures » (radars, matrices, architecture)
    bool scenarioFigures = true;  // annexe : schémas des scénarios opérationnels
};

struct CoverInfo {
    QImage logo;                  // image nulle = pas de logo
    double logoWidthMm = 45.0;
    int logoAlign = 0;            // 0 gauche, 1 centre, 2 droite
    QString title, subtitle, organisation, classification;
    QString version, date, author, owner;
};

struct Assembled {
    ReportOptions opt;
    CoverInfo cover;
    QString title;
    Md::Document doc;             // chapitre Figures + corps du rapport + annexe
};

ReportOptions optionsFrom(const Project &p);
CoverInfo coverFrom(const Project &p);

// Écrit les images dans workDir/a et construit le document. Retourne false en cas d'erreur.
bool prepare(const Project &p, const QString &workDir, Assembled *out, QString *error = nullptr);

// Figures de toutes les planches, dans l'ordre du document.
QVector<Md::Figure> collectFigures(const Md::Document &doc);

} // namespace ReportAssembler
