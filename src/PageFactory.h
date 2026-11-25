// SPDX-License-Identifier: MIT
#pragma once
#include <QWidget>

#include "Project.h"

// Fabriques des pages « tableau » : les colonnes (types, listes de valeurs, calculs) sont décrites en C++ ;
// la mise en page commune (titre, barre d'outils, tableau) est dans ui/TablePage.ui.
namespace Pages {
QWidget *valeursMetiers(Project *p);
QWidget *biensSupports(Project *p);
QWidget *evenementsRedoutes(Project *p);
QWidget *socle(Project *p);
QWidget *vulnerabilites(Project *p);
QWidget *sourcesRisque(Project *p);
QWidget *objectifsVises(Project *p);
QWidget *couplesSrOv(Project *p);
QWidget *partiesPrenantes(Project *p);
QWidget *partiesPrenantesProjection(Project *p);
QWidget *scenariosStrategiques(Project *p);
QWidget *scenariosOperationnels(Project *p);
QWidget *scenariosRisque(Project *p);
QWidget *mesuresComplementaires(Project *p);
QWidget *risquesResiduels(Project *p);
} // namespace Pages
