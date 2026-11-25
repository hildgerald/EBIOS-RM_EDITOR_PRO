# Atelier 3 – Scénarios stratégiques et radar de la menace

## Parties prenantes

Participants à l'écosystème (clients, partenaires, sous-traitants, fournisseurs, systèmes connectés…). Pour chacune, cotez de **1 à 4** :

| Critère | 1 | 4 |
|---|---|---|
| **Dépendance** | Relation non nécessaire | Relation indispensable et unique |
| **Pénétration** | Pas d'accès ou accès utilisateur | Accès administrateur à l'infrastructure ou physique aux salles serveurs |
| **Maturité cyber** | Hygiène ponctuelle, non formalisée | Politique de management du risque intégrée et proactive |
| **Confiance** | Intentions non évaluables | Intentions parfaitement connues et compatibles |

Le détail des niveaux 2 et 3 figure dans **Métriques > Référentiel des échelles**.

Colonnes calculées :

- **Exposition** = Dépendance × Pénétration ;
- **Fiabilité** = Maturité × Confiance ;
- **Score de menace** = Exposition ÷ Fiabilité ;
- **Posture** : *Zone de danger* (score > seuil danger), *Zone de contrôle* (≥ seuil contrôle), *Zone de veille* (> seuil veille), sinon *Hors périmètre*.

La case **Pertinente** indique si la partie prenante est retenue pour la suite (une partie non pertinente apparaît en pointillé et plus pâle sur le radar). Tant que les quatre cotations ne sont pas toutes renseignées, la partie prenante n'est pas placée sur le radar.

## Parties prenantes (projection)

Mêmes parties prenantes, avec une **mesure de sécurité complémentaire** et les nouvelles cotations attendues après sa mise en œuvre. **Une valeur laissée vide reprend la valeur actuelle.** Les scores et postures projetés sont recalculés.

## Radar de la menace

Cartographie de dangerosité :

- **Centre** = l'objet de l'étude ; **plus une bulle est proche du centre, plus la menace est élevée** ;
- **anneaux** = zones : veille (vert), contrôle (orange), danger (rouge), hors périmètre (gris) ; leurs seuils se règlent dans [Métriques](help:04_sommaire_metriques) ;
- **secteurs** = catégories de parties prenantes ;
- **taille de la bulle** = exposition ; **couleur** = posture ; **pointillé** = partie non pertinente.

Le sélecteur **Vue** bascule entre la *situation actuelle* et la *projection après mesures*. **Exporter en PNG…** enregistre l'image.

## Scénarios stratégiques

Chemins d'attaque de haut niveau :

| Colonne | Contenu |
|---|---|
| Sources de risque, Objectif visé, Parties prenantes | Éléments impliqués |
| Intitulé, Chemin d'attaque | Texte |
| Événements redoutés | Événements redoutés atteints |
| **Gravité saisie** | Facultative (1 à 4) |
| **Gravité retenue** (calculée) | La gravité saisie, à défaut la **plus élevée** des gravités des événements redoutés associés |

Suite : [Atelier 4](help:08_atelier4).
