## 2026-09-04 — MEMSLibrary Pack 001 : intention avant pousse

Décision utilisateur : poursuivre l'intégration des 47 PDF RAVEMEMS V2 validés dans le futur système de connaissances de MEMS Manager.

### État protégé
- `MEMSX64` reste strictement BUILD #103 / v1.0.103.
- SHA production protégé : `1d6316bd1746d6f2b4cfb751cab88d18e27ef730`.
- Aucun BUILD #104 n'est autorisé à cette étape.

### Base validée
- Corpus RAVEMEMS V2 : 47/47 PDF, run `33810202288`, zéro défaut global.
- Moteur générique validé : `5232215f5853241572f580cf1696c9f881b254f4`.
- Fondation `MEMSLibrary.dll` validée sur branche isolée, commit `acdfac19321e49d414d3dbe0f47717dcd36c71a5`, run `33840842794` vert.

### Prochaine action exacte
Construire et tester, hors production, le premier paquet canonique de connaissances issu des 47 PDF validés :
1. produire un `knowledge.sqlite` autonome et déterministe avec manifeste/version/checksums ;
2. conserver les imports futurs indépendants afin qu'un nouveau paquet défectueux ne puisse pas casser un paquet déjà validé ;
3. étendre `MEMSLibrary.dll` pour vérifier et ouvrir les paquets en lecture seule et effectuer des recherches génériques ;
4. conserver la provenance document/page en interne sans l'afficher par défaut dans les réponses IA ;
5. tester sur GitHub Actions la découverte du Pack 001, l'intégrité SQLite, les 47 documents, des recherches techniques réelles et l'isolation face à un faux paquet corrompu ;
6. ne connecter ni Qwen ni l'interface MEMS Manager tant que ce bloc autonome n'est pas complètement vert.

Le Pack 001 doit servir de format canonique pour les futurs imports PDF. Le pipeline d'extraction sera ensuite adapté pour reproduire automatiquement exactement ce même format.
