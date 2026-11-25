// SPDX-License-Identifier: MIT
#pragma once
#include <QString>

#include "Project.h"
#include "ReportAssembler.h"

// Export du rapport au format Word (.docx, Office Open XML), sans dépendance externe.
// Contenu : page de garde (logo), sommaire (champ TOC natif), liste des figures, chapitre Figures, corps, annexe.
namespace DocxExporter {

bool exportReport(const Project &p, const QString &docxPath, QString *error = nullptr);

// Écrit un rapport déjà assemblé (utile pour les tests).
bool writeDocx(const ReportAssembler::Assembled &report, const QString &docxPath, QString *error = nullptr);

} // namespace DocxExporter
