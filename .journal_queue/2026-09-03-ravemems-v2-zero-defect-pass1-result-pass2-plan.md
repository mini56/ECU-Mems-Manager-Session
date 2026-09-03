## 2026-09-03 — RAVEMEMS V2 RCL0193ENG — OBJECTIF ZERO DEFAUT — PASS 1 RESULTAT ET PASS 2

### RESULTAT IMMEDIAT PASS 1

- Perimetre strict : `RCL0193ENG` uniquement, branche technique `tmp-ravemems-v2-foundation`.
- `MEMSX64` reste strictement sur BUILD #103 / `1d6316bd1746d6f2b4cfb751cab88d18e27ef730`. Aucun BUILD #104.
- Correction PASS 1 : ordre de lecture local aux colonnes sur les vraies pages deux-colonnes ; les lignes pleine largeur ne doivent plus provoquer de bascule gauche/droite au milieu d'une procedure.
- Commit technique de correction : `70b99554a667156bbe66946759699debacd01204` (`Use column-local reading order for RCL0193ENG`).
- Run GitHub Actions : `33788182483` — SUCCESS technique.
- Artefact : `RAVEMEMS-V2-RCL0193ENG-ZERO-DEFECT-PASS1`, ID `9906260679`, taille `25218473` octets, digest `sha256:24ec897b1cee53784108c8b8a1a3b52031b74eb8a1a44e99a66254db34034d30`.
- Source constructeur conservee : `rave/xn/wmxn990e.pdf`, 372 pages, SHA-256 `c050a3eebe50c5a85bf8a69b7722bd2052079944e09d58578a498984ecf06715`.
- SQLite PASS 1 : taille `2768896` octets, SHA-256 `f0baa3227220067ab929992753beb40fa8a928c587cbca8192f65a295032be2c` ; elle n'est donc PAS identique a l'ancienne base `6ec3c21dda3f57e74626ff5a85e1a0d5a152269e40301c171c4edd1568c8ffdf`.
- Comptes PASS 1 : 201 operations, 402 phases, 3245 etapes, 180 notices, 23 requirements, 738 visuels, 449 liens visuels, 22 review flags, 4028 provenances.
- `numeric_phase_defect_count=22`.
- `rejected_numeric_candidate_count=1`.
- Audit : `1251` constats : 738 `visual_fidelity_not_verified`, 449 `visual_link_not_verified`, 22 `phase_not_complete`, 22 `open_review_flag`, 20 `operation_not_complete`. 22 constats bloquants.
- Verdict : PASS 1 a bien modifie l'extraction semantique et a preserve l'integrite SQLite/FK, mais OBJECTIF ZERO NON ATTEINT : 22 defauts numeriques reels/signales restent a traiter.

### DIAGNOSTIC DES 22 CAS RESTANTS

Les diagnostics montrent plusieurs causes generiques distinctes :

1. Faux numero d'etape issu d'une valeur decimale : `OP_0001_12_21_28`, page 53, la ligne `05 mm.` est interpretee comme etape `0` entre les etapes 7 et 8.
2. Ordre geometrique local encore incorrect sur certaines pages complexes : exemples `12.29.02` page 60 (`1,2,3,4,10,5,6,11,...`), `12.60.38` pages 88-91 (`...28,32,33,34,29,30,31`), `30.15.15` pages 172-174, `57.25.01` pages 215-218, `57.50.01` pages 222-224, `86.65.64` pages 352-353.
3. Continuation d'operation/phase trop permissive : des listes ou procedures ulterieures sont rattachees a l'operation precedente, par exemple `84.15.36` qui reprend page 327 une liste de relais/fusibles apres sa vraie phase de refit page 324.
4. Phases creees ou rattachees avec un debut de sequence manquant (`4` seul, `12-13`, `14-24`, `17-20`, etc.), ce qui indique des frontieres de phase/operation ou des continuations de page encore incorrectes, pas un motif particulier a coder en dur.
5. Des sequences repetant `1..N` sur des pages eloignees montrent egalement une fuite de contexte vers les operations suivantes (`12.65.28`, `17.50.05`, `19.55.25`, `26.50.01`, `70.35.14`).

### PROCHAINE ACTION EXACTE — PASS 2

Sur `tmp-ravemems-v2-foundation`, corriger GENERIQUEMENT et a la racine :

- la reconnaissance d'un vrai marqueur numerique d'etape afin qu'une mesure/decimale telle que `0.05 mm` ne puisse pas devenir une etape `0` ;
- l'ordre geometrique des blocs dans les pages complexes, en s'appuyant sur BBOX/colonnes/alignement des marqueurs d'etapes et non sur une simple permutation globale ;
- les frontieres et la continuation operation/phase entre pages afin qu'une operation terminee ne puisse absorber une liste, un titre ou une procedure suivante ;
- la detection des phases dont la premiere etape visible est >1 afin de distinguer une vraie continuation d'une mauvaise association ;
- les tests/diagnostics de non-contamination sur les cas representatifs, dont pages 133-135, sans aucune regle speciale injecteur/page/operation dans le parseur.

Relancer ensuite UNIQUEMENT RCL0193ENG et comparer les 22 diagnostics un par un. Objectif exige : `numeric_phase_defect_count=0` reel, sans suppression artificielle des controles. Aucun changement `MEMSX64`, aucun autre PDF.
