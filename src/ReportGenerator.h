// SPDX-License-Identifier: MIT
#pragma once
#include <QSize>
#include <QString>

#include "Project.h"

namespace ReportGenerator {

// Variante de sortie :
//  File    : fichier .md destiné à un lecteur Markdown (balises <br> dans les tableaux, espaces des URL d'images encodés)
//  Preview : aperçu Qt (QTextBrowser) : pas de balises HTML
//  Rich    : source pour les exports PDF / DOCX (balises <br> conservées, URL d'images telles quelles)
enum class Mode { File, Preview, Rich };

// Génère le rapport Markdown complet.
//  assetsDir : dossier (ou URL « file:///… » en modes Preview / Rich) contenant radar_actuel.png, radar_projection.png,
//              architecture.png ; vide = sans images.
//  inlineImages : false = les figures ne sont pas insérées dans le texte (elles figurent dans le chapitre « Figures »)
QString generate(const Project &p, const QString &assetsDir, Mode mode = Mode::File, bool inlineImages = true);

// Écrit les images du rapport (radars, architecture) dans dir (créé si besoin).
//  matrices : écrit aussi matrice_initiale.png / matrice_residuelle.png
//  scenarios : écrit aussi so_<ID>.png (schéma Connaître / Entrer / Explorer / Exploiter de chaque scénario opérationnel)
bool writeAssets(const Project &p, const QString &dir, const QSize &radarSize = QSize(1200, 900),
                 bool matrices = false, bool scenarios = false);

// URL « file:/// » encodée d'un dossier local (utilisable dans les modes Preview et Rich).
QString dirUrl(const QString &localDir);

// Brouillon de conclusion calculé à partir des données du projet (point de départ à éditer).
QString draftConclusion(const Project &p);

// Écrit <mdPath> + le dossier <nom>_assets (radars, architecture). Retourne false en cas d'erreur.
bool exportToFile(const Project &p, const QString &mdPath, QString *error = nullptr);

} // namespace ReportGenerator
