# Questions fréquentes et dépannage

## Les tableaux et les calculs

**Une liste déroulante est vide.** Elle se remplit à partir d'un autre tableau : créez d'abord les éléments à référencer (par exemple des valeurs métiers avant les événements redoutés).

**Une partie prenante n'apparaît pas sur le radar.** Les quatre cotations (dépendance, pénétration, maturité, confiance) doivent être renseignées de 1 à 4 ; vérifiez-le dans *Atelier 3 > Parties prenantes*.

**La vraisemblance d'un scénario opérationnel est vide ou « ? ».** Aucune action élémentaire n'a de vraisemblance : renseignez-les dans *Atelier 4 > Détail*, ou saisissez la *vraisemblance manuelle*.

**Le niveau d'un risque est vide.** La gravité ou la vraisemblance manque : vérifiez le lien scénario de risque → scénario opérationnel → scénario stratégique → événements redoutés (et leur niveau de gravité).

**Une valeur affiche « (?) ».** L'élément référencé a été supprimé. Choisissez un autre élément ou recréez-le avec le même identifiant.

**Le taux de conformité affiche n/a.** Aucune politique n'a de conformité renseignée (hors *Non applicable*).

## Exports

**Le PDF est généré mais des images manquent.** Vérifiez que le dossier temporaire est accessible en écriture. Les images d'architecture ne figurent que si une image a été chargée dans l'atelier 1.

**Word propose de mettre à jour les champs à l'ouverture.** C'est voulu : répondez *Oui* pour calculer les numéros de page du sommaire et de la liste des figures.

**Le sommaire Word n'affiche pas les numéros de page.** Faites un clic droit sur le sommaire puis *Mettre à jour les champs*, ou Ctrl+A puis F9. Dans LibreOffice : *Outils > Mettre à jour > Tous les index et tables*.

**Un tableau est difficile à lire dans le PDF.** Les tableaux très larges (socle de sécurité, mesures) sont réduits pour tenir en largeur. Pour un meilleur confort de lecture ou de modification, utilisez l'export Word.

**Le logo n'apparaît pas sur la page de garde.** Vérifiez que la case *Page de garde* est cochée et que l'aperçu affiche bien le logo.

**Le rapport Markdown n'a pas de page de garde.** Normal : l'habillage ne concerne que le PDF et Word.

## Fichiers

**Le fichier ne s'ouvre pas.** Il doit s'agir d'un fichier `.ebios` (JSON) valide ; s'il a été modifié à la main, une virgule manquante suffit à le rendre illisible.

**J'ai fermé sans enregistrer.** Le logiciel demande confirmation avant de fermer ou d'ouvrir un autre projet ; il n'y a pas de sauvegarde automatique, enregistrez régulièrement.

## Installation

**Sous Windows, des DLL manquent au lancement.** Lancez l'utilitaire `windeployqt` de votre installation Qt sur `EbiosRMStudio.exe` (il est exécuté automatiquement à la compilation avec CMake).

**Sous Linux, « could not load the Qt platform plugin xcb ».** Installez la bibliothèque `libxcb-cursor0`.

## Besoin d'aide supplémentaire

Parcourez les autres rubriques via la liste de gauche ou le champ de recherche.
