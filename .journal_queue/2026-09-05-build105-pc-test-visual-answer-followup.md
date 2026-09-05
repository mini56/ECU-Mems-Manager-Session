## 2026-09-05 — BUILD #105 PRODUCTION + TEST PC RÉEL — SUIVI VISUEL/RÉPONSE

### BUILD #105 production

- branche : `MEMSX64`
- commit : `1b106eed05e1fd665b857f73a719f02ee6b6b2ac`
- run : `33956856201`
- job : `101281627904`
- conclusion : **SUCCESS**
- artefact : `ECU-MEMS-Manager-x64-BUILD-105-v1.0.105-CLEAN-V2`
- artifact ID : `9979320548`
- taille : `490453458` octets
- digest : `sha256:4e9127fb76da6776f8ea367b9b69f5e4c941bbe724dcc9a699b75b009baf86f9`
- log package : `PACKAGE_OK build=105 version=1.0.105`

Le BUILD #105 intègre la DLL MEMSLibrary corrigée, conserve Pack001 inchangé et a passé compilation, self-tests, validation du paquet et smoke launch.

### Test réel utilisateur sur PC

Question testée :
`Quel est le jeu axial du pignon primaire et comment le contrôler ?`

Résultat documentaire : **correct**.
- preuve retenue : `DOC_RCL0193ENG p.53`
- révision : `REV_RCL0193ENG_SOURCE`
- opération : `12.21.28`
- valeur : `0,089 à 0,165 mm`
- méthode : contrôle avec des cales d'épaisseur entre bague d'appui et pignon primaire
- aucune contamination p342 / antenne / coaxial constatée

Deux défauts utilisateur restent visibles :
1. la fenêtre visuelle est bien routée sur `RCL0193ENG p.53`, mais affiche un petit fragment/icône recadré au lieu de l'illustration technique pertinente de l'opération ;
2. la réponse IA expose un très long contexte brut MEMSLibrary/RAVE au lieu d'une réponse finale concise et propre.

Conclusion : la recherche documentaire, la page, la valeur et la méthode sont validées sur PC ; la sélection/extraction du visuel dans la preuve retenue et la synthèse/présentation finale de la réponse restent à corriger.

### PROCHAINE ACTION EXACTE

Créer une branche temporaire strictement depuis BUILD #105 et corriger **uniquement** :
- la sélection/extraction du visuel pertinent à l'intérieur de la preuve/page déjà retenue ;
- la synthèse/présentation de la réponse IA afin que le contexte documentaire reste interne et ne soit pas déversé tel quel à l'utilisateur.

Ne pas modifier le classement/recherche documentaire validé p53. Ne pas toucher au protocole ECU, acquisition ou autres sous-systèmes. Le correctif visuel doit être générique et ne doit pas hardcoder RCL0193ENG p53 ni un VIS particulier.

Tester au minimum la question primaire réelle, puis des régressions documentaires/visuelles non liées. **Aucun BUILD #106 avant validation verte de la branche temporaire et autorisation explicite.**
