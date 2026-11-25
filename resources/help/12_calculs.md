# Règles de calcul

Toutes les valeurs calculées se mettent à jour dès qu'une donnée change. Elles reprennent celles du classeur Excel d'origine.

## Parties prenantes (atelier 3)

| Grandeur | Formule |
|---|---|
| Exposition | Dépendance × Pénétration |
| Fiabilité cyber | Maturité cyber × Confiance |
| **Score de menace** | Exposition ÷ Fiabilité |

Les quatre cotations (1 à 4) doivent être renseignées ; sinon le score reste vide et la partie prenante n'est pas placée sur le radar.

**Posture** (seuils réglables dans [Métriques](help:04_sommaire_metriques) ; valeurs par défaut entre parenthèses) :

| Condition | Posture |
|---|---|
| score > seuil danger (3) | Zone de danger |
| score ≥ seuil contrôle (1) | Zone de contrôle |
| score > seuil veille (0,49) | Zone de veille |
| sinon | Hors périmètre |

Dans la **projection**, les mêmes formules s'appliquent aux valeurs projetées ; une valeur projetée vide reprend la valeur actuelle.

## Couples SR / OV (atelier 2)

Motivation et ressources valent 1 (Faible), 2 (Moyenne), 3 (Élevée). Somme ≤ 3 : pertinence **Faible** ; = 4 : **Moyenne** ; ≥ 5 : **Élevée**.

## Gravité (ateliers 1 et 3)

- Gravité d'un événement redouté = son niveau (1 à 4), libellé selon l'échelle des Métriques.
- Gravité d'un scénario stratégique = la gravité saisie ; **à défaut, le maximum** des gravités des événements redoutés associés.

## Vraisemblance (atelier 4)

- Vraisemblance d'un scénario opérationnel = **minimum** des vraisemblances de ses actions élémentaires ; à défaut d'action, la *vraisemblance manuelle*.

## Niveau de risque (atelier 5)

- **Niveau initial** = case (gravité, vraisemblance) de la matrice de risque.
- **Niveau résiduel** = même matrice avec la gravité et la vraisemblance résiduelles ; une valeur vide reprend la valeur initiale.

## Socle de sécurité (atelier 1)

Taux de conformité = moyenne des politiques évaluées : *Conforme* = 1, *Partiellement conforme* = 0,5, autres = 0 ; les politiques sans conformité renseignée ou *Non applicable* sont ignorées.

## Mise en page du rapport

Aucun calcul : voir [Mise en page](help:11_miseenpage).
