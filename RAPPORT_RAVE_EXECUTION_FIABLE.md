# ECU MEMS Manager — RAVE — procédure d’exécution fiable

## Objet
Cette procédure verrouille la reprise et l’exécution du backfill RAVE afin d’éviter les pertes de temps, les reprises à la main, les transports binaires fragiles et les chaînes de corrections successives.

## Source RAVE canonique
- Dépôt : `mini56/ECU-Mems-Manager-Session`
- Branche source : `main`
- Corpus complet : `rave/`
- Adresse canonique : `https://github.com/mini56/ECU-Mems-Manager-Session/tree/main/rave`
- Pour RCL0193ENG : `rave/xn/wmxn990e.pdf`
- Taille : `4 744 911` octets
- Pages : `372`
- SHA-256 attendu du PDF : `c050a3eebe50c5a85bf8a69b7722bd2052079944e09d58578a498984ecf06715`

Le corpus RAVE ne doit plus être redemandé ni rechargé dans ChatGPT. Toute génération doit partir directement de `main/rave/`.

## État de reprise verrouillé
- Branche technique : `tmp-rave-complete-multilingual-backfill`
- SAFE CHECKPOINT courant : `686ecee774da47baafc65de3b7d30ed0b3eb8bc2`
- Parent direct : `932b442fe0185368eff0a78b6caa552b427b6bd8`
- Dernier lot validé : RCL0193ENG pages 203–210 `DRIVE SHAFTS`
- Prochaine page : **211**, début `STEERING`
- Frontière déjà vérifiée : **STEERING p211–226** ; p212 et p214 blanches ; p227 ouvre `SUSPENSION`
- `MEMSX64` protégé : BUILD #103 `1d6316bd1746d6f2b4cfb751cab88d18e27ef730`

## Règle principale
Aucun transport QZ64/SQL/audit par fragments Base64 n’est autorisé pour les nouveaux lots RAVE lorsque la source est déjà disponible dans `main/rave/`.

La génération doit être faite directement dans GitHub Actions à partir de la source GitHub canonique.

## Chaîne obligatoire pour chaque lot
1. Vérifier que la branche technique pointe exactement sur le dernier SAFE CHECKPOINT attendu.
2. Récupérer la source directement depuis `main/rave/`.
3. Vérifier avant tout traitement la taille, le nombre de pages et le SHA-256 de la source.
4. Déterminer le périmètre exact du lot : première page, dernière page, pages blanches, section suivante.
5. Extraire exhaustivement texte, procédures, faits, valeurs, warnings/cautions/notes, outils, consommables et visuels utiles.
6. Respecter l’architecture N-langues : ancres techniques immuables, textes humains localisables, aucun raster traduit dupliqué.
7. Générer dans le runner les trois fichiers uniquement : SQL, QZ64, audit.
8. Vérifier QZ64 -> SQL byte-for-byte.
9. Appliquer la chaîne documentaire anglaise précédente puis le nouveau lot deux fois.
10. Exiger : `PRAGMA integrity_check=ok`, `foreign_key_check=0`, `user_version=21` et idempotence complète.
11. Vérifier les pages physiques exactes et l’état des pages blanches.
12. Vérifier que `manifest.json` et toutes les tables historiques restent inchangés.
13. Recontrôler `MEMSX64` : BUILD #103 et SHA attendu inchangés.
14. Nettoyer tous helpers/workflows/transports temporaires.
15. Refuser le commit si le diff final contient autre chose que les trois fichiers documentaires du lot.
16. Journaliser AVANT POUSSE, puis seulement committer.
17. Créer directement un commit propre ayant le SAFE CHECKPOINT précédent comme parent, sans conserver les commits de workflow temporaires.
18. Vérifier `ahead_by=1`, `behind_by=0`, exactement trois fichiers.
19. Écrire un SAFE CHECKPOINT avant de commencer le lot suivant.

## Règle anti-boucle / anti-corrections
À la première anomalie de génération, de SHA, de blob, de périmètre, de SQLite ou de diff :
- arrêter le lot ;
- ne pas empiler des correctifs ;
- ne pas réparer fragment par fragment ;
- revenir au dernier SAFE CHECKPOINT ;
- corriger la cause dans le générateur ou le workflow ;
- régénérer le lot complet en une seule chaîne.

Une seule régénération complète est préférable à une suite de patchs locaux.

## Règle de périmètre projet
Le backfill RAVE ne doit modifier ni protocole, ni ECU, ni UI, ni IA, ni ONNX, ni `MEMSX64`, ni les tables historiques. Toute modification hors documentation RAVE fait échouer le lot.

## 2026-08-31 — Validation de la méthode directe GitHub — p193–202

### Préflight direct
- workflow : `TEMP RCL0193ENG P193-202 DIRECT PREFLIGHT`
- run : `33418301172`
- job : `99574178225`
- conclusion : **SUCCESS**
- source lue directement depuis `main/rave/xn/wmxn990e.pdf`
- taille/SHA/pages : PASS
- p193 : `MANUAL GEARBOX`
- p194 : blanche
- p202 : blanche
- p203 : `DRIVE SHAFTS`

### Génération + validation + compaction directe
- workflow : `TEMP RCL0193ENG P193-202 DIRECT GENERATE`
- run : `33418771507`
- job : `99575713295`
- conclusion : **SUCCESS**
- SQLite/idempotence/invariants : **PASS**
- QZ64 -> SQL byte-for-byte : **PASS**
- périmètre final : **PASS**
- `manifest.json` : inchangé
- `MEMSX64` : toujours BUILD #103 `1d6316bd1746d6f2b4cfb751cab88d18e27ef730`
- workflows temporaires supprimés du tree final

### Fichiers finaux p193–202
1. `database/reference/prototypes/rcl0193eng_p193_202_multilingual_v1.sql`
   - taille : `181524` octets
   - SHA-256 : `e2e6ee98ea5fdf0d20cd25e2e1c6faeffe4b4f6a2c078bf05540da19051a86da`
   - Git blob : `d3cc9c876633fa596920207ae63cf1a954510756`
2. `database/reference/prototypes/rcl0193eng_p193_202_multilingual_v1.qz64`
   - taille : `12872` octets
   - SHA-256 : `3408d128b67370ef264f538a66c3c18b18f5af950ad5bb75d0fc4d7548cd2c54`
   - Git blob : `b569890be7f676e738b51566f1b7c5fe163b53f7`
3. `database/reference/audits/RCL0193ENG_P193_202_MULTILINGUAL_BACKFILL_V1.md`
   - taille : `1941` octets
   - SHA-256 : `80d6b55004383e69f5133f9b72b4f5de745956a8b710039570a6d470e47f3bbd`
   - Git blob : `f7af80e82e3ac1089255587b5cfef85d784276ce`

Ces valeurs **supersèdent** les valeurs du candidat intermédiaire p193–202 journalisé auparavant dans le rapport maître : le lot a été volontairement refait proprement depuis la source RAVE canonique à la demande de reprise sans chaîne de corrections.

### Contenu validé p193–202
- 10/10 unités physiques ; p194/p202 blanches ;
- 8/8 pages non blanches avec texte source anglais ;
- 8/8 candidats visuels ;
- 6 opérations constructeur : `37.20.10`, `37.16.25`, `37.20.02.99`, `37.25.01`, `37.25.05`, `37.27.01` ;
- 9 faits structurés ;
- 8 valeurs/couples/réglages ;
- 9 warnings/cautions/notes/exigences ;
- 6 outils/équipements ;
- 7 consommables/remplacements ;
- architecture N-langues conservée ; outil Rover `18G 1383` et ancres techniques immuables.

### SAFE CHECKPOINT APRÈS POUSSE p193–202
**SAFE CHECKPOINT = YES**

- commit propre : `932b442fe0185368eff0a78b6caa552b427b6bd8`
- tree : `a6e52a7211cb5065300ed13de16aa8128d7f05fa`
- parent direct : `f2ac8d5d7039d9d242b146882b4c29f25f2c468f`
- compare : `ahead_by=1`, `behind_by=0`, `total_commits=1`
- diff final : exactement les trois fichiers documentaires p193–202
- aucun helper/workflow/transport temporaire dans le tree final

## 2026-08-31 — RCL0193ENG p203–210 DRIVE SHAFTS — VALIDATION DIRECTE

### Frontière et inventaire
- workflow frontière : `TEMP RCL0193ENG P203 DRIVE SHAFTS BOUNDARY`
- run : `33419029541`
- job : `99576573124`
- conclusion : **SUCCESS**
- section exacte : **p203–210 DRIVE SHAFTS**
- p204 : blanche
- p211 : début `STEERING`
- scan déjà étendu : `STEERING` p211–226 ; p212/p214 blanches ; p227 ouvre `SUSPENSION`

- workflow inventaire : `TEMP RCL0193ENG P203-210 INVENTORY`
- run : `33419123418`
- job : `99576887448`
- conclusion : **SUCCESS**

### Génération + validation + compaction directe
- workflow : `TEMP RCL0193ENG P203-210 DIRECT GENERATE`
- run : `33419330403`
- job : `99577571398`
- conclusion : **SUCCESS**
- QZ64 -> SQL byte-for-byte : **PASS**
- double application SQLite / idempotence : **PASS**
- `PRAGMA integrity_check=ok`
- `foreign_key_check=0`
- `user_version=21`
- pages physiques exactes 203..210 ; p204 `out_of_scope/not_required`
- `manifest.json` inchangé
- `MEMSX64` toujours BUILD #103 `1d6316bd1746d6f2b4cfb751cab88d18e27ef730`
- tous workflows temporaires retirés du tree final

### Fichiers finaux p203–210
1. `database/reference/prototypes/rcl0193eng_p203_210_multilingual_v1.sql`
   - taille : `167061` octets
   - SHA-256 : `94c7a0e8a53149bd4b33ebf736a7b64b1f13847f05dfa1763753a609219bbd9d`
   - Git blob : `76d00e2a2543f949eb7ab1a3378a4ad045d3e713`
2. `database/reference/prototypes/rcl0193eng_p203_210_multilingual_v1.qz64`
   - taille : `11656` octets
   - SHA-256 : `f7cd536a8f2fa3df2bac78561e33c2b4887efbd909ee629cbc1d1297a35eb365`
   - Git blob : `86c0a4dd2a9073dd601c38ec6089b6782fecacd9`
3. `database/reference/audits/RCL0193ENG_P203_210_MULTILINGUAL_BACKFILL_V1.md`
   - taille : `1443` octets
   - SHA-256 : `95ed05cac1e2d7d5dd988cbca7e63d526533ac6114a17412774fce7fdc2a8d76`
   - Git blob : `ddfe418ba0a7f0c46f8ac1b28efe2c145fd4674c`

### Contenu validé p203–210
- 8/8 unités physiques ; p204 blanche ;
- 7/7 pages utiles avec texte source anglais et candidat visuel ;
- 3 opérations constructeur : `47.10.04`, `47.10.14`, `47.10.28` ;
- 9 faits structurés ;
- 5 valeurs/couples/dimensions ;
- 10 warnings/cautions/notes/exigences ;
- 10 outils/équipements ;
- 7 consommables/remplacements ;
- ancres Rover `18G 1240` et `18G 1584` immuables ; architecture N-langues conservée ;
- valeurs remarquables : `A ≈ 6 mm`, `52 N.m`, `30 N.m`, `52 N.m`, `260 N.m`.

### SAFE CHECKPOINT APRÈS POUSSE p203–210
**SAFE CHECKPOINT = YES**

- commit propre : `686ecee774da47baafc65de3b7d30ed0b3eb8bc2`
- tree : `99a34e597f919bd965adfd4427014567f0decb34`
- parent direct : `932b442fe0185368eff0a78b6caa552b427b6bd8`
- compare : `ahead_by=1`, `behind_by=0`, `total_commits=1`
- diff final : exactement les trois fichiers documentaires p203–210
- aucun helper/workflow/transport temporaire dans le tree final

## PROCHAINE ACTION EXACTE
Reprendre depuis le SAFE CHECKPOINT `686ecee774da47baafc65de3b7d30ed0b3eb8bc2`.

Traiter directement **RCL0193ENG p211–226 `STEERING`** depuis `main/rave/xn/wmxn990e.pdf`. La frontière est déjà vérifiée : p212 et p214 sont blanches ; p227 ouvre `SUSPENSION`. Ne pas refaire un scan de frontière inutile. Faire directement l’inventaire exhaustif p211–226 puis une génération/validation/compaction directe selon la procédure ci-dessus. À la première anomalie : arrêt + retour au SAFE CHECKPOINT, aucune chaîne de patchs. Ne toucher ni à `MEMSX64`, ni protocole/ECU/UI/IA/ONNX.

## But recherché
La fiabilité ne dépend plus du montage de fichiers de ChatGPT ni du transport manuel de gros binaires. GitHub devient à la fois la source documentaire, l’environnement de génération, le contrôleur d’intégrité et le point de reprise vérifiable.