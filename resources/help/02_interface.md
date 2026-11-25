# Interface et édition des tableaux

## Fenêtre principale

- **Arbre de navigation** (à gauche) : les pages sont groupées par atelier. Un clic ouvre la page.
- **Zone de page** (à droite) : tableau, formulaire ou graphique selon la page.
- **Barre de titre** : nom du fichier ; `*` = modifications non enregistrées.
- **Barre de menu** : **Fichier** (projet, sous-menu *Exporter*), **Affichage** (déplier / replier l'arbre, plein écran, barre d'outils), **Ateliers** (accès direct à chaque groupe de pages), **Aide**.
- **Barre d'outils** (sous les menus) : Nouveau, Ouvrir, Enregistrer, export PDF, export Word, Aide. Elle se masque par *Affichage > Barre d'outils*.

## Les pages « tableau »

Presque tous les éléments des ateliers sont des tableaux identiques dans leur fonctionnement :

| Élément | Rôle |
|---|---|
| **Filtrer…** | Ne garde que les lignes contenant le texte saisi (toutes colonnes confondues) |
| **Ajouter** | Ajoute une ligne en fin de tableau avec un identifiant automatique (par exemple `VM-04`) |
| **Supprimer** | Supprime les lignes sélectionnées après confirmation |
| **▲ / ▼** | Monte / descend la ligne sélectionnée |
| Compteur | Nombre d'éléments affichés ; certaines pages ajoutent un pied de tableau (taux de conformité…) |

Sélection multiple : **Ctrl + clic** ou **Maj + clic** sur les numéros de ligne.

## Types de colonnes

| Type | Comment l'éditer |
|---|---|
| **Texte court** | Double-clic ou touche **F2**, saisissez, validez par Entrée |
| **Texte long** | Double-clic : une boîte de dialogue s'ouvre, valider par *OK* |
| **Liste de valeurs** | Cliquez la cellule et choisissez ; la molette de la souris fait défiler le tableau sans modifier la valeur tant que la liste n'a pas le focus |
| **Case à cocher** | Un clic (D, I, C, T, « Retenu », « Pertinente »…) |
| **Référence** | Liste déroulante des éléments d'une autre page (ex. valeur métier d'un événement redouté) |
| **Références multiples** | Bouton affichant la sélection ; cliquez pour cocher plusieurs éléments dans une boîte de dialogue |
| **Calculée** (fond gris) | Lecture seule, recalculée à chaque modification |

## Couleurs

Les niveaux calculés sont colorés : **Faible / Acceptable / Zone de veille** en vert, **Moyen / Tolérable / Zone de contrôle** en jaune,
**Élevé / Inacceptable / Zone de danger** en rouge, **Hors périmètre** en gris.

## Bon à savoir

- Supprimer un élément **ne modifie pas** les références existantes vers lui : la liste affiche alors la valeur suivie de `(?)`. Corrigez-la ou rétablissez l'élément.
- Les identifiants sont numérotés automatiquement (`prefixe` + numéro suivant). Vous pouvez les modifier (colonnes de texte) mais gardez-les **uniques**.
- Les pages « Parties prenantes (projection) » et « Risques résiduels » affichent les mêmes éléments que « Parties prenantes » et « Scénarios de risque » mais avec d'autres colonnes ; on n'y ajoute pas de lignes.
