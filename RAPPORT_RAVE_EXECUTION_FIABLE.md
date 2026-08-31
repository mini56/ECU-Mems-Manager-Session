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
- SAFE CHECKPOINT courant : `932b442fe0185368eff0a78b6caa552b427b6bd8`
- Parent direct : `f2ac8d5d7039d9d242b146882b4c29f25f2c468f`
- Dernier lot validé : RCL0193ENG pages 193–202 `MANUAL GEARBOX`
- Prochaine page : **203**, début `DRIVE SHAFTS`
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

## 2026-08-31 — Validation de la méthode directe GitHub

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

### SAFE CHECKPOINT APRÈS POUSSE
**SAFE CHECKPOINT = YES**

- commit propre : `932b442fe0185368eff0a78b6caa552b427b6bd8`
- tree : `a6e52a7211cb5065300ed13de16aa8128d7f05fa`
- parent direct : `f2ac8d5d7039d9d242b146882b4c29f25f2c468f`
- compare : `ahead_by=1`, `behind_by=0`, `total_commits=1`
- diff final : exactement les trois fichiers documentaires p193–202
- aucun helper/workflow/transport temporaire dans le tree final

## PROCHAINE ACTION EXACTE
Reprendre depuis le SAFE CHECKPOINT `932b442fe0185368eff0a78b6caa552b427b6bd8`.

Ouvrir RCL0193ENG page physique **203**, section `DRIVE SHAFTS`, depuis la source canonique `main/rave/xn/wmxn990e.pdf`. Déterminer automatiquement et visuellement la fin exacte de la section et les éventuelles pages blanches, comparer au bloc français correspondant uniquement pour la déduplication, extraire exhaustivement les données utiles et les visuels avec l’architecture N-langues, puis exécuter la même chaîne GitHub directe. À la première anomalie : arrêt + retour au SAFE CHECKPOINT, aucune chaîne de patchs. Ne toucher ni à `MEMSX64`, ni protocole/ECU/UI/IA/ONNX.

## But recherché
La fiabilité ne dépend plus du montage de fichiers de ChatGPT ni du transport manuel de gros binaires. GitHub devient à la fois la source documentaire, l’environnement de génération, le contrôleur d’intégrité et le point de reprise vérifiable.