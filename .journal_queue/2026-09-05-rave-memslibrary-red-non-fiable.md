## 2026-09-05 - ALERTE ROUGE RAVE / RAVEMEMS V2 / MEMSLibrary

### STATUT

La chaine documentaire RAVE -> RAVEMEMS V2 -> Pack001 -> MEMSLibrary.dll -> IA MEMS Manager doit etre consideree ROUGE et NON VALIDEE.

Ne pas utiliser les anciennes affirmations de validation multilingue comme preuve de fiabilite.

Aucune correction, reconstruction, nouvelle DLL ou nouveau BUILD n'est validee par cette entree. Cette entree documente uniquement l'etat reel constate.

### ECHEC REEL DANS BUILD #104

Question utilisateur testee dans l'IA :

`Quel est le jeu axial du pignon primaire et comment le controler ?`

Resultat documentaire observe :
- MEMSLibrary remonte `DOC_RCL0193ENG` page 342 puis page 343 ;
- ces pages concernent l'antenne / cable coaxial et sont hors sujet ;
- le bouton de schema propose egalement RCL0193ENG page 342 ;
- le test IA reel est donc ROUGE.

La preuve correcte existe pourtant dans RAVEMEMS V2 :
- operation constructeur `12.21.28` ;
- titre `ENGINE CRANKSHAFT PRIMARY GEAR END-FLOAT` ;
- page physique 53 de RCL0193ENG ;
- jeu axial `0.089 to 0.165 mm` ;
- controle avec cales d'epaisseur entre backing ring et primary gear ;
- visuels exacts lies a cette operation : `VIS_P0053_001` et `VIS_P0053_002`.

Conclusion : pour ce cas, la source V2 contient la bonne procedure et les bonnes images, mais la recherche/routage aval choisit la mauvaise preuve.

### PROBLEME PLUS GRAVE : LANGUES DU CORPUS

La validation precedente annoncant un corpus RAVEMEMS V2 complet et multilingue n'est plus recevable.

Un audit direct avait remonte une repartition de metadonnees `source_language` de 15 documents `en` et 32 documents `und`. Une reponse ulterieure a donne une autre classification 9/38 basee sur les noms/publications. Ces deux comptages ne mesurent pas la meme chose et ont ete presentes a tort comme s'ils etaient equivalentes.

Par consequent :
- ne pas considerer 15/32 ou 9/38 comme un comptage final des langues reelles ;
- `und` signifie langue non determinee dans les metadonnees, pas une langue ;
- un document `und` ne peut pas etre traduit ou route de facon fiable tant que sa langue reelle n'est pas identifiee ;
- aucun corpus francais n'a ete prouve dans le RAVEMEMS V2 final inspecte ;
- l'affirmation precedente selon laquelle toutes les langues du ZIP RAVE avaient ete conservees dans la chaine finale est invalidee.

Il faut distinguer obligatoirement trois informations :
1. langue declaree dans `source_language` ;
2. suffixe/nom de publication comme ENG/FRE/etc. ;
3. langue reelle detectee dans le contenu du PDF.

Aucune de ces trois mesures ne doit plus etre substituee a une autre.

### ETAT STRUCTUREL RAVEMEMS V2

Le schema RAVEMEMS V2 possede bien les champs permettant une isolation stricte :
- `ravemems_document.source_language` ;
- `ravemems_operation.source_language` ;
- `ravemems_visual.source_language` ;
- `ravemems_visual.revision_key` ;
- `ravemems_visual.page_key` ;
- liens exacts `ravemems_visual_link` vers operation/phase/step.

Sur les 47 bases inspectees precedemment :
- 1667 visuels ;
- 1667/1667 fichiers visuels presents dans leur dossier source ;
- aucun chemin relatif casse constate ;
- aucun lien visuel inter-document/revision constate.

Cela prouve seulement que le schema peut isoler correctement documents/revisions/images. Cela ne prouve pas que les langues attendues du ZIP source ont toutes ete ingerees ni que le runtime les isole.

### DEFAUT MEMSLibrary PACK001 / ABI2

Pack001 conserve un champ `source_language` dans la table de recherche interne, mais l'API publique ABI2 ne permet pas de demander une langue.

`MEMSLibrary_SearchPack(...)` ne recoit aucun parametre langue, document, revision ou publication.

La requete SQL de recherche filtre les termes via `search_text LIKE ?`, mais n'impose pas de filtre :
- `source_language` ;
- document ;
- revision ;
- publication.

Le resultat ABI2 ne retourne pas non plus `source_language` ni `revision_key`.

Conclusion : meme si plusieurs langues etaient presentes dans un Pack, la DLL actuelle ne peut pas garantir une isolation stricte des langues.

### DEFAUT DANS LE BRIDGE IA DE BUILD #104

`expert/IaMemsLibraryBridge.cpp` appelle la recherche ABI2 avec des variantes de la question et des mots-cles.

Il n'envoie aucun contexte obligatoire de :
- langue source ;
- document ;
- revision ;
- publication.

Le contrat de preuve retourne document/page/titre/corps, mais pas la langue ni le lien V2 exact vers un visuel.

Le routage visuel peut donc suivre une mauvaise preuve documentaire, comme observe sur le test pignon primaire page 342.

### CONSEQUENCE POUR LE PROJET

La DLL actuelle ne doit pas etre consideree comme une fondation documentaire fiable de l'IA MEMS Manager tant que la chaine source n'a pas ete re-etablie et prouvee.

Cela ne remet pas en cause les autres blocs du programme qui doivent rester preserves :
- fonctions ECU ;
- protocole et analyse MEMS Manager ;
- ROSCO ;
- donnees LeopoldG ;
- donnees Haro/MemsFCR ;
- autres donnees techniques independantes de RAVE.

BUILD #104 reste un build CI vert pour compilation/package/interface, mais la partie IA documentaire RAVE/MEMSLibrary est ROUGE en test utilisateur reel.

### PROCHAINE ACTION AUTORISEE A CE STADE

Aucune reconstruction n'est autorisee par cette entree.

La prochaine verification necessaire, avant toute correction, est un audit forensique en lecture seule de la chaine exacte :

`rave.zip source reel -> inventaire PDF -> fichiers fournis au generateur -> RAVEMEMS V2 produit -> Pack001 -> DLL`

Pour chaque PDF, relever sans interpretation :
- nom/publication ;
- langue declaree ;
- langue reelle ;
- pages ;
- presence dans le corpus V2 ;
- presence dans Pack001.

Objectif : identifier exactement ou les langues attendues ont ete perdues et pourquoi les controles precedents ont laisse passer ce defaut.

### REGLE DE VALIDATION

Ne plus declarer RAVE, RAVEMEMS V2, Pack001 ou MEMSLibrary VERT sur la base d'un simple succes de workflow ou d'un self-test anglais.

Une validation future devra prouver la correspondance exacte entre le ZIP source et le corpus produit, puis tester la recherche reelle dans la langue utilisateur avec la bonne procedure et le bon visuel.
