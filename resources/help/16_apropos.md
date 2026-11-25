# À propos

**EBIOS RM Studio** – application de bureau pour mener une analyse de risques selon la méthode **EBIOS Risk Manager**.

- Interface : Qt 6 (Widgets), C++17 ; écrans modifiables avec Qt Designer (dossier `ui/`).
- Plateformes : Linux et Windows.
- Format de projet : `.ebios` (JSON).
- Exports : Markdown, PDF, Word (.docx) – sans bibliothèque externe.

EBIOS Risk Manager est une méthode publiée par l'**ANSSI**. Ce logiciel est un outil d'aide à la saisie, au calcul et à la présentation : il ne remplace ni l'expertise de l'analyste ni la décision de l'autorité d'homologation.

Les calculs reprennent ceux d'un classeur Excel EBIOS RM classique (voir [Règles de calcul](help:12_calculs)).

## Licence

EBIOS RM Studio est un **logiciel libre sous licence MIT** : vous pouvez l'utiliser, le copier, le modifier et le redistribuer, y compris dans un cadre commercial, à condition de conserver la mention de copyright et le texte de la licence (fichier `LICENSE`). Il est fourni **sans garantie**.

## Composants tiers

Le logiciel utilise **Qt 6** (modules Core, Gui et Widgets) sous licence **GNU LGPL v3**, lié dynamiquement : Qt peut être remplacé par une version modifiée compatible. Le texte complet et les obligations de redistribution figurent dans le fichier `THIRD_PARTY_NOTICES.md`. Qt et son logo sont des marques de The Qt Company Ltd.

EBIOS Risk Manager est une méthode de l'ANSSI ; ce logiciel n'est ni édité ni approuvé par l'ANSSI.
