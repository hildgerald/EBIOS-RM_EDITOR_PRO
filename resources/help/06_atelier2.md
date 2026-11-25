# Atelier 2 – Sources de risque

Objectif : identifier qui pourrait vouloir nuire (sources de risque), pourquoi (objectifs visés), et retenir les couples les plus pertinents.

## Sources de risque

Types d'attaquants (organisation étatique, cybercriminel, concurrent, vengeur, terroriste…) : nom, description, **Retenue** (oui / non) et **justification** de ce choix.

## Objectifs visés

Finalités poursuivies (espionnage, déstabilisation, gain financier, sabotage…) : nom, description, **Retenu**, justification.

## Couples SR / OV

Un couple associe une source de risque à un objectif visé :

| Colonne | Contenu |
|---|---|
| Source de risque, Objectif visé | Listes alimentées par les deux tableaux précédents |
| Description de l'objectif | Texte |
| **Motivation**, **Ressources** | *Faible*, *Moyenne* ou *Élevée* |
| **Pertinence** (calculée) | Faible, Moyenne ou Élevée à partir des deux colonnes précédentes |
| **Retenu**, justification | Décision de garder ou non le couple pour la suite |

**Règle de pertinence** : on additionne motivation et ressources (Faible = 1, Moyenne = 2, Élevée = 3). Somme ≤ 3 : *Faible* ; somme = 4 : *Moyenne* ; somme ≥ 5 : *Élevée*.

Les couples **retenus** servent de base aux scénarios stratégiques de l'[atelier 3](help:07_atelier3).
