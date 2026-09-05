## 2026-09-05 — CHANGEMENT DE STRATÉGIE — AUDIT DU BASELINE ET RECONSTRUCTION DOCUMENTAIRE RAVE

Constat utilisateur après test réel du BUILD #105 : la chaîne documentaire actuelle continue à révéler des défauts de fond (sélection d'images non pertinentes, heuristiques de recherche, synthèse IA fragile). L'utilisateur demande d'arrêter l'empilement de correctifs aval et de repartir plus en arrière afin de déterminer le bon baseline applicatif avant de reconstruire correctement la fondation documentaire.

Décision immédiate :
- figer le BUILD #105 ;
- ne pas intégrer le correctif temporaire `tmp-build105-visual-answer-fix` dans `MEMSX64` ;
- ne lancer aucun BUILD #106 avant validation de la nouvelle fondation ;
- conserver l'historique existant uniquement comme preuve et comparaison, sans le prendre automatiquement comme nouvelle base documentaire.

Objectif de l'audit de baseline : déterminer factuellement si la reprise applicative doit partir de #105, #104, #103 ou d'un état antérieur, en comparant les changements réellement introduits dans les builds et en séparant :
1. le socle applicatif/UI/protocole valide ;
2. les intégrations IA/MEMSLibrary/RAVEMEMS qui devront être rejetées ou remplacées.

Ordre de reconstruction demandé :
1. déterminer le baseline applicatif exact ;
2. corriger RAVEMEMS V2 avant toute nouvelle intégration afin que l'extraction et l'assemblage soient structurellement corrects ;
3. repartir des 47 PDF RAVE sources canoniques ;
4. refaire l'extraction texte/pages/opérations/publication/révision/langue sans mélange entre documents ;
5. corriger l'extraction des images afin de rejeter les pictogrammes, logos, icônes et éléments décoratifs de page qui ne constituent pas des preuves techniques ;
6. conserver pour chaque vrai visuel ses coordonnées, dimensions, type et lien exact vers page/section/opération ;
7. auditer manuellement et automatiquement les images et l'extraction avant assemblage ;
8. reconstruire l'assemblage SQLite avec relations explicites document -> révision -> langue -> page -> opération/section -> texte -> visuels ;
9. reconstruire ensuite MEMSLibrary.dll autour de ce schéma afin qu'elle retourne directement la provenance structurée et les identifiants visuels pertinents ;
10. valider base + DLL indépendamment de Qwen avant tout retour dans MEMS Manager.

Cas de référence obligatoire : question `Quel est le jeu axial du pignon primaire et comment le contrôler ?` -> RCL0193ENG p.53, opération 12.21.28, 0.089 à 0.165 mm, contrôle aux cales d'épaisseur, vrai schéma mécanique de contrôle ; aucun p342/coaxial et aucun pictogramme de page.

PROCHAINE ACTION EXACTE : auditer les builds #103, #104, #105 et, si nécessaire, les builds immédiatement antérieurs afin de fixer le baseline applicatif de reprise avant toute reconstruction RAVEMEMS V2.
