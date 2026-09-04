## 2026-09-04 - IA DATA - NETTOYAGE ORDONNE DE LA COUCHE VISUELLE PRE-V2

Decision utilisateur verrouillee :
- ne supprimer aucune donnee utile aux fonctions ECU ;
- ne supprimer aucune donnee utile a la fonction d analyse de MEMS Manager ;
- conserver les donnees ROSCO ;
- conserver les donnees LeopoldG/mems-rosco ;
- conserver les donnees Haro/MemsFCR et autres sources techniques independantes ;
- supprimer de la base active toutes les images documentaires RAVE anterieures a RAVEMEMS V2 ;
- ne plus reutiliser comme source active les anciens runtimes/bundles visuels pre-V2 ;
- conserver les connaissances structurees historiques lorsqu elles servent l analyse, meme si leurs anciennes images physiques sont retirees.

Constat du rapport historique utilise pour le nettoyage : l ancien BUILD #103 contenait 427 images physiques sous `database/reference/images/rave/` et 427 entrees correspondantes dans le manifeste. Les faits structures historiques avaient deja ete classes a conserver car ils apportent une connaissance curee utile a l analyse ; ils ne doivent pas etre confondus avec les anciens fichiers image.

Branche de nettoyage isolee : `tmp-ai-memslibrary-clean-v2-data`, creee exactement depuis le code direct propre `5b899f62662270d7eb65a793d45394548db2b2a2`. Commit workflow : `b7d550a54c13622cc1311f082e155419ad8b3a7e`.

Run GitHub Actions : `33919306584`, job `101173691392`, conclusion SUCCESS.

Nettoyage execute uniquement sur une copie du dernier package IA vert `33889068203` :
- 427/427 anciennes images pre-V2 sous `database/reference/images/rave/` supprimees de la copie active ;
- 427/427 entrees visuelles correspondantes retirees uniquement de la section `diagrams` du manifeste ;
- ancien `runtime_visual_catalog.json` retire s il etait present ;
- aucun autre champ du manifeste modifie ;
- 121 fichiers non cibles verifies byte-for-byte identiques avant/apres ;
- 77 lots QZ64 proteges et inchanges ;
- 6 images non-RAVE protegees et inchangees ;
- aucun fichier non cible supprime ou modifie.

Cette garde byte-for-byte protege la couche ECU/analyse et les donnees techniques independantes presentes dans la base historique, notamment ROSCO, Leopold et Haro. Le nettoyage ne supprime pas les connaissances structurees historiques ; il cible uniquement les anciens fichiers image RAVE et leurs entrees visuelles de menu.

Nouvelle source visuelle active unique : artefact final RAVEMEMS V2 run `33810202288`, artifact ID `9914590689`, digest `sha256:0dab01f5a5e58229245f1eb96eedaca234839030e9cffe630b1e103f56130ac5`.
- 47/47 bases V2 source verifiees ;
- 1 667 visuels V2 reconstruits depuis leurs `ravemems_v2.sqlite` et les fichiers exacts declares ;
- chaque fichier copie est verifie par SHA256 ;
- `source_lineage=RAVEMEMS_V2_FINAL_33810202288` ;
- `legacy_entries=0` ;
- `pre_v2_images_included=0` ;
- les anciens runs visuels `33510951021` et `33524210118` sont interdits comme source active du nouveau catalogue.

Artefact propre produit : `CLEAN-REFERENCE-V2-33919306584`, artifact ID `9954393075`, taille `123807774` octets, digest `sha256:b056ada5b925648824274f6b04cfa95b81fb6ba8bd07c101f3210757f5ef4df8`.

Production : `MEMSX64` reste strictement BUILD #103, SHA `1d6316bd1746d6f2b4cfb751cab88d18e27ef730`. Aucun BUILD #104. Aucun patch de source. Aucun changement ECU/protocole/32 bits.

PROCHAINE ACTION EXACTE : assembler hors production un package PC depuis le code direct propre `5b899f626...` et l artefact `CLEAN-REFERENCE-V2-33919306584`, sans aucun ancien bundle/runtime visuel ; verifier ensuite sur PC l IA, le scroll, le choix du schema et son rendu. Ne pas toucher a MEMSX64 et ne pas creer BUILD #104 sans autorisation explicite.
