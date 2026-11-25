# Atelier 1 – Cadrage et socle de sécurité

Objectif : définir le périmètre étudié, ce qu'on cherche à protéger et ce qu'on craint.

## Valeurs métiers

Processus ou informations dont l'organisation ne peut se passer.

| Colonne | Contenu |
|---|---|
| ID | Identifiant (`VM-01`…) |
| Intitulé, Famille de processus, Description | Texte |
| **D / I / C / T** | Besoins de sécurité : Disponibilité, Intégrité, Confidentialité, Traçabilité |
| **Biens supports associés** | Biens supports qui portent cette valeur métier (sélection multiple) |

## Biens supports

Ressources (matériels, logiciels, réseaux, personnes, locaux) qui font fonctionner les valeurs métiers : nom, type, description, D / I / C / T. La colonne calculée **Valeurs métiers** indique à quelles valeurs métiers le bien est rattaché (selon la page précédente).

## Événements redoutés

Ce que l'organisation craint (perte, altération, divulgation, indisponibilité…) :

- **Valeur métier** concernée (liste), **intitulé**, **impacts** ;
- **Niveau** de 1 à 4 : la colonne calculée *Gravité* affiche le libellé de l'échelle (voir [Métriques](help:04_sommaire_metriques)).

## Socle de sécurité

Référentiels, règles et politiques applicables (réglementation, normes, politiques internes) avec : nom, règle, caractère obligatoire, contenu, porteur, priorité, documents liés, **conformité**, cible, échéance, preuves, commentaire.

Valeurs de conformité : *Conforme*, *Partiellement conforme*, *Conformité prévue*, *Non conforme*, *Non applicable*.
Le **taux de conformité** affiché sous le tableau compte 100 % par politique *Conforme*, 50 % par politique *Partiellement conforme*, 0 % pour les autres, et ignore les politiques vides ou *Non applicable*.

## Vulnérabilités

Liste facultative des faiblesses connues, rattachables à des biens supports ou à des parties prenantes (sélection multiple).

## Architecture

Description textuelle et **schéma** du système. **Charger une image…** accepte PNG, JPEG, BMP ou GIF ; l'image est intégrée au projet. Elle apparaît dans le rapport (chapitre *Figures*).

---

Suite : [Atelier 2](help:06_atelier2). Voir aussi [Règles de calcul](help:12_calculs).
