# Atelier 4 – Scénarios opérationnels

Objectif : décrire **comment** une attaque pourrait se dérouler, étape par étape, et estimer sa **vraisemblance**.

## Scénarios opérationnels

| Colonne | Contenu |
|---|---|
| ID, **Scénario stratégique** | Scénario de l'atelier 3 que l'on détaille |
| Intitulé, Chemin d'attaque | Texte |
| **Vraisemblance manuelle** | Utilisée **uniquement** si le scénario n'a aucune action élémentaire |
| **Actions** (calculée) | Nombre d'actions élémentaires |
| **Vraisemblance** (calculée) | Vraisemblance retenue pour le scénario |

## Détail (actions élémentaires)

Sélectionnez un scénario dans la liste de gauche. Deux représentations s'affichent :

1. Le **schéma** en quatre colonnes de la chaîne d'attaque : **Connaître → Entrer → Explorer → Exploiter**. Chaque case porte la vraisemblance (1 à 4) de l'action.
2. Le **tableau des actions** : phase, action élémentaire (texte), vraisemblance (1 à 4), biens supports ciblés.

Le schéma se met à jour au fil de la saisie. Utilisez **Ajouter** pour créer une action pour le scénario sélectionné.

## Vraisemblance globale

**Vraisemblance du scénario = la plus basse des vraisemblances de ses actions** : une attaque n'est aussi probable que son maillon le plus difficile. Exemple : actions à 4, 2, 3 → vraisemblance 2.

Cette valeur alimente la colonne *Vraisemblance* des [scénarios de risque](help:09_atelier5).
