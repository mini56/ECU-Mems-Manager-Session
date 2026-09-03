## 2026-09-03 — RAVEMEMS V2 RCL0193ENG — AVANT POUSSE PASS 2 ZERO DEFAUT

Branche technique autorisee : `tmp-ravemems-v2-foundation` uniquement. Production `MEMSX64` strictement protegee sur BUILD #103, commit `1d6316bd1746d6f2b4cfb751cab88d18e27ef730`. Aucun BUILD #104. Aucun autre PDF que `RCL0193ENG` / `rave/xn/wmxn990e.pdf`.

### Resultat PASS 1 deja constate
- run exact : `33788182483` SUCCESS technique ;
- artefact : `RAVEMEMS-V2-RCL0193ENG-ZERO-DEFECT-PASS1`, ID `9906260679`, digest `sha256:24ec897b1cee53784108c8b8a1a3b52031b74eb8a1a44e99a66254db34034d30` ;
- defauts numeriques : 22, contre 242 avant correction ;
- SQLite semantique modifiee, donc correction reelle et non simple diagnostic ;
- verdict : NON VALIDE tant que les 22 cas ne sont pas classes/corriges.

### Diagnostic structurel des 22 cas
1. Plusieurs pages restent lues en ordre global Y alors qu'elles contiennent une procedure constructeur en deux colonnes. Exemples observes : pages 60, 91, 172, 216, 224 et 353. La geometrie des marqueurs montre que l'ordre correct est colonne gauche puis colonne droite.
2. Des mots de continuation de ligne tels que `assembly.`, `assembly` ou `remove.` sont pris a tort pour des titres de phase, car le parseur accepte actuellement un libelle de phase sur le texte seul sans verifier son alignement de titre.
3. Certaines vraies sous-phases constructeur continuent la numerotation precedente au lieu de repartir a 1. Exemples : `Check 1..11` puis `Adjust 12..13`, ou `Remove 1..6` puis `Inspection 7..9`. Ce sont des continuations valides et ne doivent pas etre signalees comme sequence incomplete.
4. L'etat operation/phase peut survivre a plusieurs pages non procedurales et absorber ensuite des listes numerotees descriptives. Exemples constates apres les pages 96, 104, 145, 160, 255 et 324.
5. Une valeur decimale `0.05 mm` est encore interpretee comme etape constructeur 0 ; une etape constructeur 0 doit etre rejetee.
6. Certaines operations commencent par des etapes numerotees avant tout libelle de phase explicite ; une phase implicite de procedure doit alors etre creee au lieu de perdre les premieres etapes.

### Correction PASS 2 autorisee
Correction generique uniquement, sans regle composant/page specifique dans le parseur :
- renforcer la reconnaissance deux-colonnes par la geometrie des marqueurs numerotes et repousser la zone footer pour ne pas sortir les etapes basses du corps ;
- n'accepter un libelle de phase que s'il a la geometrie/typographie d'un vrai titre de colonne, pas une continuation indentee ;
- creer une phase implicite lorsqu'une operation reconnue rencontre ses premieres etapes avant un titre de phase explicite ;
- casser la continuite operation/phase apres deux pages consecutives sans activite procedurale ;
- rejeter toute etape constructeur `0` et les faux marqueurs decimaux ;
- reconnaitre comme valide une sous-phase dont la numerotation est contigue a la sous-phase precedente de la meme operation ;
- conserver comme bloquants les vrais trous, doublons ou inversions ; ne jamais supprimer un defaut uniquement pour faire baisser le compteur.

Validation exigee apres relance exacte RCL0193ENG : SQLite differente si la semantique change, `integrity_check=ok`, aucune FK cassee, controle des pages 131-136 toujours propre, et examen de tout defaut restant. Objectif final : `0` defaut structurel reel ; si un compteur reste non nul, continuer la correction au lieu de declarer la validation.
