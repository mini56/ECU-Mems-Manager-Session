# RAVEMEMS V2 — architecture d'entrée source générique

Date: 2026-09-03

Décision utilisateur confirmée:
- Le moteur ne doit dépendre ni du nom RAVE, ni du nom d'une archive, ni d'un dossier particulier.
- L'entrée doit être une source générique: PDF unique, dossier ou archive contenant des PDF.
- Le moteur découvre les PDF, puis détermine depuis leur contenu publication, langue, type documentaire et structure.
- Le nom et le chemin physiques ne servent qu'à localiser le fichier, jamais à décider comment l'analyser.
- Un PDF unique hors archive doit fonctionner exactement comme un PDF découvert dans un dossier ou une archive.

Action avant prochaine pousse technique:
- introduire un résolveur de source générique;
- supprimer le verrou d'entrée RCL0193ENG;
- conserver les profils historiques uniquement comme références de régression;
- tester RCL 0179ENG comme PDF unique sans profil spécifique;
- refaire les régressions RCL0193ENG et RCL0213ENG;
- MEMSX64 reste protégé au BUILD #103, SHA 1d6316bd1746d6f2b4cfb751cab88d18e27ef730.
