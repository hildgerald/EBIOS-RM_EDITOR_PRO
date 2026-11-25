# Mentions légales des composants tiers

EBIOS RM Studio est distribué sous **licence MIT** (fichier `LICENSE`). Il s'appuie sur les composants suivants,
qui restent soumis à leurs propres licences.

## Qt 6 (bibliothèque utilisée à l'exécution)

| | |
|---|---|
| Modules utilisés | **Qt Core, Qt Gui, Qt Widgets** (aucun autre module : ni Charts, ni WebEngine, ni PrintSupport) |
| Licence retenue | **GNU LGPL version 3** (Qt est aussi disponible en GPL v2/v3 et en licence commerciale) |
| Site, sources | <https://www.qt.io> – <https://download.qt.io> – <https://doc.qt.io/qt-6/licensing.html> |
| Texte de la licence | <https://www.gnu.org/licenses/lgpl-3.0.txt> (et GPL v3 : <https://www.gnu.org/licenses/gpl-3.0.txt>) |

Qt est **lié dynamiquement** (DLL / bibliothèques partagées) et n'est pas modifié par ce projet.
Le code de ce dépôt reste donc sous licence MIT ; seule la bibliothèque Qt est sous LGPL v3.

### Si vous distribuez l'application compilée (installeur, archive, paquet)

La LGPL v3 demande, pour la partie Qt :

1. **Mentionner l'utilisation de Qt** et sa licence (c'est le rôle de ce fichier et de la fenêtre *À propos*).
2. **Joindre le texte de la licence LGPL v3** (et de la GPL v3 qu'elle référence) avec les binaires.
3. **Indiquer où obtenir les sources de Qt** correspondant à la version distribuée (<https://download.qt.io>).
4. **Permettre le remplacement de Qt** par une version modifiée : garder Qt en liaison dynamique (comportement par défaut ;
   ne pas lier Qt statiquement sauf à respecter les obligations supplémentaires de la LGPL).
5. **Ne pas interdire** la rétro-ingénierie nécessaire au débogage de ces modifications.

Sous Windows, `windeployqt` copie les DLL de Qt à côté de l'exécutable : placez dans le même dossier ce fichier, `LICENSE`
et le texte de la LGPL v3. Qt embarque lui-même quelques composants tiers (par exemple des polices, la bibliothèque
libpng, zlib) dont les mentions figurent dans la documentation de Qt : <https://doc.qt.io/qt-6/licenses-used-in-qt.html>.

### Outils de compilation

`moc`, `uic`, `rcc`, `windeployqt` et Qt Designer sont des outils de développement fournis avec Qt ; ils ne sont pas
distribués avec l'application. Le code qu'ils génèrent (`moc_*.cpp`, `ui_*.h`, `qrc_*.cpp`) n'est pas soumis à la
licence de l'outil (exception Qt GPL 1.0).

## Script de conversion Excel (outil facultatif)

`tools/excel_to_json.py` utilise **openpyxl** (licence MIT, <https://openpyxl.readthedocs.io>), installé séparément par
l'utilisateur ; il n'est pas distribué avec le logiciel.

## Méthode EBIOS Risk Manager

EBIOS Risk Manager est une méthode publiée par l'**ANSSI**. Ce projet n'est ni édité ni approuvé par l'ANSSI. Les textes
d'aide et de glossaire sont des rédactions originales ; les échelles du référentiel intégré (page *Métriques*) sont des
échelles génériques usuelles de la méthode.

## Marques

« Qt » et son logo sont des marques de The Qt Company Ltd. Les autres noms de produits appartiennent à leurs propriétaires.

---
*Ce document informe ; il ne constitue pas un avis juridique. En cas de doute sur une distribution commerciale, consultez un juriste.*
