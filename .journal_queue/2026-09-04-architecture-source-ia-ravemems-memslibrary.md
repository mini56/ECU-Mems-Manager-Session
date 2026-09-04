## 2026-09-04 - ARCHITECTURE FONDAMENTALE DE LA SOURCE IA

### BUT GENERAL
La chaine RAVEMEMS V2 + MEMSLibrary n'est pas une fonction annexe. Elle devient la source documentaire fondamentale destinee a alimenter l'IA de ECU MEMS Manager avec des donnees constructeur verifiables, structurees et extensibles.

L'objectif est d'eviter une IA qui repond a partir de connaissances generales ou inventees. Les reponses techniques doivent etre construites a partir des donnees documentaires retrouvees dans la bibliotheque, puis reformulees par Qwen.

### CHAINE DE DONNEES DE REFERENCE
PDF RAVE constructeur
-> RAVEMEMS V2
-> MEMSLibrary_Pack_001
-> MEMSLibrary.dll ABI 2 x64
-> recherche documentaire MEMS Manager
-> contexte/faits fournis a Qwen
-> reponse utilisateur sourcee
-> reference document/page/illustration disponible si pertinente

### ROLE DE RAVEMEMS V2
RAVEMEMS V2 extrait et conserve la structure utile des vrais PDF constructeur, et ne se limite pas a une copie de texte.

Le corpus valide couvre 47 PDF et 1359 pages.

RAVEMEMS V2 conserve notamment :
- document source ;
- numero de page ;
- texte PDF natif ;
- lignes et positions ;
- blocs de contenu ;
- illustrations/images extraites ;
- occurrences de chaque visuel sur les pages ;
- coordonnees des visuels ;
- OCR uniquement quand necessaire pour les pages raster ;
- identification correcte des pages reellement blanches ;
- relations SQLite controlees.

Etat final valide du corpus :
- 47/47 PDF ;
- 1359/1359 pages ;
- 1291 pages avec texte PDF natif ;
- 66 pages blanches correctement reconnues ;
- 45 zones OCR / 490 mots sur le cas raster CDXN p.7 ;
- 1794 occurrences visuelles ;
- 1070 assets visuels ;
- 54732 lignes natives ;
- 19039 blocs de contenu ;
- SQLite integrity_check=ok ;
- aucune FK cassee ;
- aucune erreur de traitement ;
- pass=true.

Run de reference RAVEMEMS complet : 33484362718
Commit : 72dcaf04e84181669aa25c9103ea60bf47d9e1a7

### ROLE DE MEMSLibrary_Pack_001
MEMSLibrary_Pack_001 est la bibliotheque normalisee construite a partir du corpus RAVEMEMS V2 valide.

Le but est que MEMS Manager ne lise pas directement 47 PDF et ne depende pas de leur organisation brute. Les donnees extraites sont assemblees dans un pack controle, interrogeable et reproductible.

Validation Pack001 :
- run 33847448261 SUCCESS ;
- commit 9cf3d00840cec1749ef9358f708d3e68a2c8bc4d ;
- Pack001 construit deux fois et reproductibilite validee ;
- integrite SQLite et FK validees ;
- recherche dans Pack001 validee ;
- Pack002 volontairement corrompu rejete sans casser Pack001.

Artefact de reference : MEMSLibrary-Pack001-33847448261
Artifact ID : 9927059742

### ROLE DE LA NOUVELLE MEMSLibrary.dll
MEMSLibrary.dll ABI 2 x64 est la couche d'acces entre MEMS Manager et les packs documentaires.

Architecture attendue :
MEMS Manager
-> MEMSLibrary.dll
-> MEMSLibrary_Pack_001 et futurs packs
-> donnees issues de RAVEMEMS V2

Cette separation est fondamentale :
- l'application ne doit pas contenir toute la logique documentaire RAVE ;
- la DLL fournit une interface stable de recherche ;
- les packs peuvent evoluer ou etre ajoutes sans reconstruire toute l'architecture documentaire du programme ;
- un pack corrompu doit pouvoir etre rejete sans rendre les packs valides inutilisables.

La DLL ABI 2 x64 a ete compilee et chargee reellement pendant la validation Pack001.

### ROLE DE QWEN
Qwen n'est pas la source des donnees techniques.

Le flux voulu est :
1. la question utilisateur est analysee ;
2. MEMSLibrary recherche les donnees pertinentes ;
3. les faits/references documentaires trouves sont fournis a Qwen ;
4. Qwen formule une reponse lisible ;
5. MEMS Manager conserve la source documentaire, la page et le visuel associe quand ils existent et sont pertinents.

Le paquet de reference qui valide l'integration IA + MEMSLibrary + RAVEMEMS + Qwen est :
- commit 9da599e697aa96d099982a30f5f5c8a562a7c788 ;
- run 33889068203 SUCCESS ;
- artefact TMP-ECU-MEMS-Manager-x64-IA-MEMSLibrary-33889068203 ;
- artifact ID 9943343980 ;
- digest sha256:4cf818b64670a8fb4dc5e6f77b7b30c35a3ab95251046ca24c529194a622f7f5.

### REGLE FONDAMENTALE POUR LES ILLUSTRATIONS
Les illustrations extraites par RAVEMEMS V2 font partie de la preuve documentaire.

Si une recherche documentaire retrouve une information technique et qu'une illustration constructeur reellement pertinente est liee a cette information/page/occurrence, MEMS Manager doit pouvoir proposer Voir le schema meme si la question utilisateur ne contient pas le mot schema.

Inversement, une image ne doit jamais etre proposee simplement parce qu'un mot-cle de la question ressemble a un mot-cle du catalogue visuel.

La decision doit etre basee sur la preuve documentaire effectivement retrouvee et son lien avec le visuel, afin d'eviter les images parasites.

### BUT A LONG TERME
Cette architecture doit devenir la source principale d'alimentation de l'IA technique de MEMS Manager.

Elle doit permettre :
- des reponses fondees sur les documents constructeur ;
- une tracabilite vers document/page ;
- l'affichage d'illustrations pertinentes ;
- l'ajout futur d'autres packs documentaires ;
- l'independance entre le moteur documentaire, l'interface et le modele Qwen.

### INVARIANTS ACTUELS
- MEMSX64 reste strictement sur BUILD #103 : 1d6316bd1746d6f2b4cfb751cab88d18e27ef730.
- Aucun BUILD #104.
- Ne pas reconstruire le corpus RAVEMEMS complet pour corriger un probleme UI.
- Ne pas modifier MEMSLibrary.dll, Pack001 ou Qwen pour corriger le seul routage du bouton Voir le schema.
- La derniere base de paquet completement verte reste 33889068203 / 9da599e697aa96d099982a30f5f5c8a562a7c788.
- La branche propre de reprise IA est tmp-ai-memslibrary-reprise-clean.
- Son commit de base propre actuel est 06dfb885478cb4ccd1e6a2ceffa49a1da8ec5054, parent direct de 9da599e697aa96d099982a30f5f5c8a562a7c788 avec uniquement les fichiers applicatifs utiles remis au-dessus de la base verte.

### PROCHAINE ACTION APRES JOURNALISATION
Ne reprendre le code qu'apres confirmation que cette architecture fondamentale est bien inscrite dans RAPPORT_SUIVI_ECU_MEMS_MANAGER.md.

Ensuite, travailler uniquement sur la branche tmp-ai-memslibrary-reprise-clean pour corriger le routage de Voir le schema a partir de la preuve documentaire reelle, sans toucher MEMSX64 #103, RAVEMEMS V2, MEMSLibrary.dll, Pack001 ou Qwen.