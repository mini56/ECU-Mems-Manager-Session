## 2026-09-03 — RAVEMEMS V2 RCL0193ENG — RESULTAT PASS 2 : SEQUENCES A ZERO, MAIS 76 CONTAMINATIONS DE FRONTIERE RESTENT

Branche technique : `tmp-ravemems-v2-foundation` uniquement. Production `MEMSX64` toujours strictement protegee sur BUILD #103, commit `1d6316bd1746d6f2b4cfb751cab88d18e27ef730`. Aucun BUILD #104. Source unique : `RCL0193ENG` / `rave/xn/wmxn990e.pdf`.

### Run exact PASS 2 exploitable
- run GitHub Actions : `33791572975` — SUCCESS ;
- job : `100769165126` — SUCCESS ;
- commit wrapper effectivement teste : `5f988b3e8e4621639182d8e70dfe911a1b7e6b29` ;
- commit selector workflow : `c353c48b504b19bf39a00bf1cf3cb640624c01cb` ;
- artefact : `RAVEMEMS-V2-RCL0193ENG-ZERO-DEFECT-PASS2` ;
- artifact ID : `9907532567` ;
- taille : `25177840` octets ;
- digest artefact : `sha256:fbbbd5676ca3045b84c0e0ae0ccf858c2ac8d362bdfb01e01e32d966ccfa65d3`.

### Integrite et compteurs
- PDF exact : 372 pages, SHA-256 `c050a3eebe50c5a85bf8a69b7722bd2052079944e09d58578a498984ecf06715` ;
- SQLite SHA-256 : `c2eb99e00508f2b7a0ee7b62402fb349340257368b0d840ec2277a0b8e57f251` ;
- `integrity_check=ok` ;
- FK cassees : 0 ;
- operations : 201 ;
- phases : 397 ;
- etapes : 3104 ;
- notices : 145 ;
- requirements : 23 ;
- visuels : 738 ;
- liens visuels : 401 ;
- review flags : 0 ;
- provenance : 3847 ;
- `numeric_phase_defect_count=0` ;
- diagnostics de sequence : `[]` ;
- audit total : 1139, uniquement `visual_fidelity_not_verified=738` et `visual_link_not_verified=401`, donc constats de verification visuelle encore ouverte et non defauts numeriques.

### Sequences ciblees explicitement validees
- `12.29.02 remove` : 1..36 ;
- `12.60.38 remove` : 1..34 ;
- `30.15.15 remove` : 1..29 ;
- `57.25.01 remove` : 1..26 ;
- `57.50.01 remove` : 1..21 ;
- `86.65.64 remove` : 1..11 ;
- `33.25.03` : 1..13, phase implicite recuperee ;
- fuite stale `12.65.28` apres page 98 : absente ;
- etape constructeur `0` : absente.

### Defaut semantique reel encore present
Le compteur numerique a atteint zero mais l'inspection du SQLite et des pages 131-136 montre que le parseur ajoute encore le titre de l'operation suivante a la fin de la derniere etape de l'operation precedente. La cause est la frontiere titre/code : le titre constructeur apparait avant le numero d'operation, et la logique de continuation de l'etape l'absorbe avant que `_new_operation()` ne ferme l'etape precedente.

Exemples exacts :
- `18.30.12` derniere etape : `Connect harness to CKP sensor multiplug. THROTTLE POSITION (TP) SENSOR` ;
- `18.30.17` derniere etape : `Fit air cleaner. See this section. 12 MANIFOLD ABSOLUTE PRESSURE (MAP) SENSOR` ;
- `18.30.71` derniere etape : `Secure relay module to mounting bracket. CAMSHAFT POSITION (CMP) SENSOR` ;
- `19.22.16` derniere etape : `Fit inlet and exhaust manifold gasket. See MANIFOLD & EXHAUST SYSTEMS, Repairs. 14 FUEL RAIL` ;
- `19.60.04` derniere etape : `Adjust throttle cable. See Adjustments. 16 FUEL INJECTORS`.

Audit automatique supplementaire du SQLite PASS 2 : **76 occurrences exactes** ou le `title_source` de l'operation suivante est present en suffixe dans une etape de l'operation precedente. Ce sont des contaminations de frontiere reelles, meme si `numeric_phase_defect_count=0`.

### VERDICT
**RCL0193ENG V2 RESTE SEMANTIQUEMENT NON VALIDE.** Le zero des sequences est acquis, mais le zero defaut reel n'est pas encore atteint.

### PROCHAINE ACTION EXACTE
Corriger generiquement la frontiere d'operation : avant de fermer l'etape courante lors de la creation de l'operation suivante, retirer uniquement le titre constructeur identifie de l'operation suivante et son eventuel compteur de page/colonne immediatement associe, sans toucher au texte d'instruction precedent. Ajouter un garde global qui exige qu'aucune etape d'une operation ne se termine par le titre exact de l'operation suivante. Relancer uniquement RCL0193ENG et recontroler le SQLite complet, les 76 cas, les sequences, les pages 131-136, l'integrite/FK et `MEMSX64` #103.
