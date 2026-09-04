## 2026-09-04 — MEMSLibrary.dll — démarrage du socle isolé

Décision utilisateur validée : retenir `MEMSLibrary.dll` comme nom du moteur générique de bibliothèque de connaissances pour ECU MEMS Manager.

Vérification préalable effectuée dans le dépôt : aucune occurrence existante de `MEMSLibrary`, `MEMSLibrary.dll`, `MEMS Library` ou `Library.dll` n'a été trouvée avant création.

### Périmètre autorisé
- créer un socle DLL générique nommé exactement `MEMSLibrary.dll` ;
- séparer ce moteur des packs de données `knowledge.sqlite` et de Qwen ;
- conserver une architecture indépendante de RAVE, de tout nom d'archive/dossier et de tout corpus particulier ;
- travailler sur une branche temporaire isolée ;
- compiler et tester uniquement via GitHub Actions ;
- ne brancher encore aucune modification d'interface utilisateur, aucun protocole ECU et aucune modification de production.

### Protection production
`MEMSX64` reste strictement protégé sur BUILD #103 / v1.0.103, SHA `1d6316bd1746d6f2b4cfb751cab88d18e27ef730`. Aucun BUILD #104 n'est autorisé par cette étape.

### Prochaine action exacte
Inspecter le système de build et les conventions existantes au SHA production protégé, créer une branche `tmp-memslibrary-foundation`, ajouter le minimum nécessaire pour produire `MEMSLibrary.dll`, puis lancer un test GitHub Actions dédié avec garde explicite de `MEMSX64`.
