# Projets : nouveau, ouvrir, enregistrer, importer

## Fichier projet

Une analyse est enregistrée dans un fichier **`.ebios`**, au format JSON lisible (UTF-8). Il contient tous les tableaux, les métriques, la mise en page du rapport (y compris le logo) et l'image d'architecture. Une seule pièce à sauvegarder ou à partager.

## Commandes du menu Fichier

| Commande | Raccourci | Effet |
|---|---|---|
| Nouveau | Ctrl+N | Projet vide (propose d'enregistrer si besoin) |
| Ouvrir… | Ctrl+O | Charge un fichier `.ebios` (ou `.json`) |
| Enregistrer | Ctrl+S | Écrit le fichier courant (demande un nom la première fois) |
| Enregistrer sous… | Ctrl+Maj+S | Écrit sous un nouveau nom |
| Exporter | voir [Rapport](help:10_rapport) | Markdown, PDF, Word, radar PNG |
| Quitter | Ctrl+Q | Ferme le logiciel (propose d'enregistrer) |

L'enregistrement est **atomique** : le fichier d'origine n'est remplacé que lorsque l'écriture a réussi.

## Ouvrir un fichier depuis la ligne de commande

    EbiosRMStudio mon_analyse.ebios

Sous Windows, vous pouvez aussi glisser le fichier sur l'exécutable.

## Importer un classeur Excel EBIOS RM

Le script `tools/excel_to_json.py` convertit un classeur Excel EBIOS RM au gabarit à 22 feuilles :

    python3 tools/excel_to_json.py classeur.xlsx sortie.ebios [annexe.json]

(nécessite Python 3 et le module `openpyxl`). Ouvrez ensuite `sortie.ebios` dans le logiciel. Les informations qui sont de simples schémas
dans le classeur (actions élémentaires des scénarios opérationnels, liaisons des scénarios stratégiques) peuvent être fournies dans un fichier
`annexe.json` facultatif décrit en tête du script ; sinon saisissez-les dans **Atelier 3** et **Atelier 4 > Détail**.

> **Confidentialité** : un fichier `.ebios` ou un classeur contient votre analyse de risques réelle. Ne le diffusez pas hors du cadre prévu.

## Bonnes pratiques

- Enregistrez des **versions successives** (`analyse_v1.ebios`, `analyse_v2.ebios`) à chaque jalon, ou versionnez le fichier avec un outil comme Git (le JSON se compare bien).
- Faites une copie du fichier avant une grosse restructuration.
