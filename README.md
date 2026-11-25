# EBIOS RM Studio (Qt 6 / C++17)

Application de bureau pour mener une analyse de risques **EBIOS Risk Manager** : 5 ateliers, radar de la menace,
matrices de risque, conclusion éditable, **aide intégrée** (F1) et rapport exportable en **Markdown, PDF et Word (.docx)** avec
**page de garde (logo paramétrable), sommaire, liste des figures et chapitre de figures**.
Elle reprend toutes les feuilles d'un classeur Excel EBIOS RM classique (22 feuilles).

> **Licence : MIT** (voir `LICENSE`). Qt 6 est utilisé sous licence LGPL v3 (voir `THIRD_PARTY_NOTICES.md`).

---------------------------------------------------------------------------------------------------
## 1. Compiler

Prérequis : **Qt >= 6.2** (module Widgets), **CMake >= 3.16**, un compilateur C++17 (GCC/Clang, MSVC 2019+, MinGW).
Aucune autre dépendance (le PDF utilise `QPdfWriter`, le .docx est écrit par du code maison, sans bibliothèque externe).

### Linux (Debian / Ubuntu)
    sudo apt install build-essential cmake ninja-build qt6-base-dev qt6-base-dev-tools libgl1-mesa-dev
    cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
    cmake --build build
    ./build/EbiosRMStudio examples/demo_commandes.ebios
    # optionnel : sudo cmake --install build   (installe aussi l'entrée de menu .desktop)

Fedora : `sudo dnf install cmake ninja-build qt6-qtbase-devel mesa-libGL-devel`
Avec un Qt installé ailleurs (installeur officiel) : ajouter `-DCMAKE_PREFIX_PATH=~/Qt/6.x.y/gcc_64`.

### Windows
1. Installer Qt 6 avec l'installeur officiel (kit **MSVC 2019/2022 64-bit** ou **MinGW 64-bit**) et CMake.
2. Ouvrir un terminal « x64 Native Tools Command Prompt » (MSVC) ou le terminal Qt (MinGW), puis :

        set QT_ROOT_DIR=C:\Qt\6.5.3\msvc2019_64
        cmake --preset windows-msvc
        cmake --build --preset windows-msvc

   ou, sans preset :

        cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH=C:\Qt\6.5.3\msvc2019_64
        cmake --build build --config Release

   MinGW : `cmake --preset windows-mingw` puis `cmake --build --preset windows-mingw`.
3. `windeployqt` est lancé automatiquement après la compilation : le dossier contenant `EbiosRMStudio.exe`
   (par ex. `build\Release`) est directement exécutable et copiable sur un autre poste.
   Désactivation : `-DEBIOS_WINDEPLOYQT=OFF`.

### Qt Creator (Linux ou Windows)
Ouvrir `CMakeLists.txt` **ou** `EbiosRMStudio.pro` (qmake), choisir un kit Qt 6, compiler.

### Intégration continue
`.github/workflows/build.yml` compile le projet sur Ubuntu (Qt de la distribution et Qt 6.5) et Windows (MSVC) à
chaque `push` et publie les binaires en artefacts.

---------------------------------------------------------------------------------------------------
## 2. Modifier l'interface avec Qt Designer

Toute la mise en page est dans le dossier **`ui/`** : double-cliquer un `.ui` dans Qt Creator, ou lancer `designer ui/SummaryPage.ui`.

| Fichier `.ui` | Contenu |
|---|---|
| `MainWindow.ui` | fenêtre, menus (Fichier, Exporter, Aide), raccourcis, arbre de navigation, zone de pages |
| `TablePage.ui` | modèle commun de toutes les pages « tableau » : titre, aide, filtre, boutons, tableau, pied |
| `SummaryPage.ui`, `MetricsPage.ui`, `ArchitecturePage.ui`, `ConclusionPage.ui` | formulaires |
| `RadarPage.ui`, `TreatmentPage.ui`, `OperationalDetailPage.ui` | pages graphiques |
| `ReportPage.ui` | aperçu et boutons d'export Markdown / PDF / Word |
| `ReportLayoutPage.ui` | réglages de la page de garde (logo, sous-titre, classification), du sommaire et des figures, avec aperçu |
| `HelpDialog.ui` | fenêtre d'aide : recherche, liste des rubriques, lecteur |

Règles à respecter pour que le code continue de fonctionner :
* **Ne pas renommer les widgets** (propriété `objectName`) : le code y accède par leur nom (`ui->titleEdit`, …).
  Vous pouvez en revanche les déplacer, les redimensionner, changer textes, polices, couleurs, tailles, styles, infobulles
  et ajouter des widgets décoratifs. Pour qu'un nouveau widget soit utilisé, ajoutez la ligne correspondante dans le `.cpp` de la page.
* `RadarWidget`, `RiskMatrixWidget` et `KillChainWidget` sont des **widgets promus** (en-tête `Widgets.h`) : ils apparaissent
  comme de simples `QWidget` dans Designer et se dessinent à l'exécution.
* Les **colonnes** des pages tableau (types, listes de valeurs, colonnes calculées) sont décrites en C++ dans
  `src/PageFactory.cpp` ; ce choix permet de les garder cohérentes avec le modèle de données.
* **Contenu de l'aide** : fichiers Markdown dans `resources/help/` (un fichier par rubrique, ordre et titres dans `index.txt`),
  compilés dans l'exécutable via `resources/resources.qrc`. Pour ajouter une rubrique : créer le `.md`, l'ajouter à `index.txt`
  et à `resources.qrc`, puis recompiler. Les liens entre rubriques s'écrivent `[texte](help:nom_du_fichier)`.
* L'arbre de navigation (ordre des pages) est construit dans `MainWindow.cpp`.
* La **barre de menu** (Fichier, Affichage, Ateliers, Aide) et la **barre d'outils** sont définies dans `MainWindow.ui` (Éditeur d'actions) ; le menu « Ateliers » est rempli dans le code à partir de l'arbre de navigation. Les actions de menu sont définies dans `MainWindow.ui` ; leur câblage est dans le constructeur de `MainWindow`.

---------------------------------------------------------------------------------------------------
## 3. Utilisation

Arbre de navigation : Général (Sommaire, Métriques) → Atelier 1 (Valeurs métiers, Biens supports, Événements redoutés,
Socle de sécurité, Vulnérabilités, Architecture) → Atelier 2 (Sources de risque, Objectifs visés, Couples SR/OV) →
Atelier 3 (Parties prenantes, Projection, **Radar de la menace**, Scénarios stratégiques) → Atelier 4 (Scénarios opérationnels,
Détail des actions élémentaires) → Atelier 5 (Scénarios de risque, Mesures complémentaires, Risques résiduels, Matrices) →
Rapport (Conclusion, aperçu et exports).

Les tableaux sont éditables (Ajouter / Supprimer / Monter / Descendre / Filtrer) ; les liens entre éléments se choisissent par
listes déroulantes. Les projets sont enregistrés en `.ebios` (JSON lisible).

### Règles de calcul (reprises du classeur)
* Menace d'une partie prenante = (Dépendance × Pénétration) / (Maturité × Confiance) ; zones : danger > 3, contrôle ≥ 1,
  veille > 0,49, sinon hors périmètre (seuils modifiables dans Métriques).
* Vraisemblance d'un scénario opérationnel = minimum des vraisemblances de ses actions élémentaires.
* Gravité d'un scénario stratégique = valeur saisie, sinon maximum des gravités des événements redoutés associés.
* Niveau de risque = matrice gravité × vraisemblance (modifiable) ; pertinence d'un couple SR/OV : motivation + ressources.

### Exports du rapport (menu Fichier > Exporter, ou page « Rapport »)
| Format | Détails |
|---|---|
| **Markdown** | `rapport.md` + dossier `rapport_assets/` (radars, architecture) ; sans page de garde |
| **PDF** | A4 paysage ; **page de garde** (logo), **sommaire numéroté** et liste des figures sur une page, chapitre **Figures** (une planche par page), tableaux à en-têtes répétés, pieds de page « Page n / total » |
| **Word (.docx)** | mêmes éléments ; sommaire et liste des figures = champs Word natifs (Word propose de les mettre à jour à l'ouverture : répondre « Oui »), styles Titre 1–4, légendes numérotées, tableaux modifiables aux niveaux colorés, numéros de page |

Réglages (page « Rapport > Mise en page ») : activer / désactiver la page de garde, le sommaire, le chapitre Figures et l'annexe des
schémas de scénarios ; logo (PNG/JPEG…, largeur en mm, gauche / centre / droite), sous-titre, organisation, classification.

Contenu : synthèse, échelles, [figures], ateliers 1 à 5 (avec fiche détaillée par valeur métier), conclusion, annexe de traçabilité,
[annexe des schémas des scénarios opérationnels].

### Aide
Menu **Aide** (ou touche **F1**, qui ouvre la rubrique de la page affichée) : 17 rubriques, recherche plein texte, glossaire EBIOS RM,
raccourcis clavier, dépannage.

### Importer un classeur Excel
    python3 tools/excel_to_json.py classeur.xlsx sortie.ebios [annexe.json]     # nécessite openpyxl

Le script convertit les 22 feuilles du gabarit. Les liaisons « scénario stratégique → événements redoutés / objectif visé » et les actions
élémentaires des scénarios opérationnels (souvent des schémas dans le classeur) peuvent être fournies dans un `annexe.json` facultatif
(format décrit en tête du script) ; sinon elles restent vides et se saisissent dans l'application.

`examples/demo_commandes.ebios` est un **exemple fictif** (application de gestion de commandes) pour découvrir le logiciel.

---------------------------------------------------------------------------------------------------
## 4. Licence et contributions

* Code source : **licence MIT** (`LICENSE`). Remplacez la mention `[NOM DU TITULAIRE DU COPYRIGHT]` par votre nom ou celui de votre organisation.
  Chaque fichier source porte l'identifiant `SPDX-License-Identifier: MIT`.
* **Qt 6** (modules Core, Gui, Widgets) est utilisé sous **LGPL v3**, en liaison dynamique : l'application reste libre de licence MIT.
  En distribuant des binaires, joignez les textes de licence et indiquez où obtenir les sources de Qt : détails dans `THIRD_PARTY_NOTICES.md`.
  N'ajoutez pas de module Qt distribué en GPL seule (Charts, Data Visualization…) sans changer de licence.
* **Données sensibles** : ne publiez jamais d'analyse réelle (fichiers `.ebios`, classeurs Excel, annexes JSON) dans le dépôt. Le dossier
  `private/` est ignoré par Git. Seul l'exemple fictif `examples/demo_commandes.ebios` est destiné à être publié.
* Contributions : toute contribution acceptée est publiée sous la même licence MIT.
