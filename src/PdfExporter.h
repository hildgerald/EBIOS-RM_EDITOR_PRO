// SPDX-License-Identifier: MIT
#pragma once
#include <QString>

#include "Project.h"

// Export du rapport au format PDF (A4 paysage) : Markdown -> modèle -> HTML -> QTextDocument -> QPdfWriter.
namespace PdfExporter {

bool exportReport(const Project &p, const QString &pdfPath, QString *error = nullptr);

} // namespace PdfExporter
