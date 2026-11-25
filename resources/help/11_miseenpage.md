# Mise en page : page de garde, sommaire, figures

La page **Rapport > Mise en page** règle l'habillage des exports **PDF** et **Word**. Le fichier Markdown n'est pas concerné. Les réglages sont enregistrés dans le projet.

## Page de garde

Cochez **Page de garde** pour l'inclure. Paramètres :

| Réglage | Effet |
|---|---|
| **Logo** : *Choisir une image…* / *Retirer* | PNG, JPEG, BMP, GIF ou WebP ; la transparence d'un PNG est conservée ; l'image est réduite à 1200 px de large au maximum pour alléger le projet |
| **Largeur du logo (mm)** | De 10 à 150 mm ; la hauteur est limitée à 45 mm, proportions respectées |
| **Position du logo** | Gauche, Centre, Droite |
| **Sous-titre** | Ligne sous le titre |
| **Organisation** | Affichée dans le bloc d'informations |
| **Classification** | Mention encadrée en bas à droite (Non protégé, Diffusion restreinte, Confidentiel, Secret, ou texte libre) |

Le **titre**, la **version**, la **date**, l'**auteur** et le **responsable métier** proviennent de la page [Sommaire](help:04_sommaire_metriques). Un **aperçu** de la page de garde se met à jour à chaque modification.

La page de garde remplace le titre et le tableau d'identification qui ouvrent le rapport Markdown.

## Sommaire et liste des figures

- **PDF** : une page présente le **sommaire** (chapitres et sections avec numéros de page) à gauche et la **liste des figures** à droite. Si les entrées sont nombreuses, la page se prolonge.
- **Word** : le sommaire et la liste des figures sont des **champs Word** natifs, modifiables et mis à jour à l'ouverture (voir [Rapport](help:10_rapport)).

## Chapitre « Figures »

Cochez **Chapitre « Figures »** pour regrouper les figures principales, **une planche par page** :

1. radar de la menace actuel **et** radar projeté ;
2. matrice des risques initiaux **et** matrice des risques résiduels ;
3. schéma d'architecture (si une image a été chargée dans l'atelier 1).

Chaque figure reçoit un numéro (« Figure 1 – … ») repris dans la liste des figures. Quand le chapitre est activé, les images ne sont **plus répétées** dans le corps du texte, qui y renvoie. Désactivé, les radars et l'architecture reviennent dans les ateliers.

L'option **Ajouter en annexe les schémas des scénarios opérationnels** insère, en fin de rapport, un schéma Connaître / Entrer / Explorer / Exploiter par scénario ayant des actions élémentaires (un par page).

## Numérotation des pages

La page de garde compte pour la page 1 mais n'affiche pas de numéro. Les pages suivantes portent « Page n / total ».
