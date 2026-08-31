# ECU MEMS Manager — RAVE — procédure d’exécution fiable

## Objet
Cette procédure verrouille la reprise et l’exécution du backfill RAVE afin d’éviter les pertes de temps, les reprises à la main, les transports binaires fragiles et les chaînes de corrections successives.

## Source RAVE canonique
- Dépôt : `mini56/ECU-Mems-Manager-Session`
- Branche source : `main`
- Corpus complet : `rave/`
- Adresse canonique : `https://github.com/mini56/ECU-Mems-Manager-Session/tree/main/rave`
- Pour RCL0193ENG : `rave/xn/wmxn990e.pdf`
- SHA-256 attendu du PDF : `c050a3eebe50c5a85bf8a69b7722bd2052079944e09d58578a498984ecf06715`

Le corpus RAVE ne doit plus être redemandé ni rechargé dans ChatGPT. Toute génération doit partir directement de `main/rave/`.

## État de reprise verrouillé
- Branche technique : `tmp-rave-complete-multilingual-backfill`
- SAFE CHECKPOINT courant : `f2ac8d5d7039d9d242b146882b4c29f25f2c468f`
- Dernier lot validé : RCL0193ENG pages 177–192 `CLUTCH`
- Lot à refaire proprement : RCL0193ENG pages 193–202 `MANUAL GEARBOX`
- Page 203 : début `DRIVE SHAFTS`
- `MEMSX64` protégé : BUILD #103 `1d6316bd1746d6f2b4cfb751cab88d18e27ef730`

## Règle principale
Aucun transport QZ64/SQL/audit par fragments Base64 n’est autorisé pour les nouveaux lots RAVE lorsque la source est déjà disponible dans `main/rave/`.

La génération doit être faite directement dans GitHub Actions à partir de la source GitHub canonique.

## Chaîne obligatoire pour chaque lot
1. Vérifier que la branche technique pointe exactement sur le dernier SAFE CHECKPOINT attendu.
2. Récupérer la source directement depuis `main/rave/`.
3. Vérifier avant tout traitement le SHA-256 de la source.
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
17. Après validation, compacter l’historique temporaire sans modifier le tree validé.
18. Écrire un SAFE CHECKPOINT avant de commencer le lot suivant.

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

## Lot courant 193–202
Pour la reprise actuelle :
- source : RCL0193ENG `wmxn990e.pdf` ;
- pages physiques 193–202 : `MANUAL GEARBOX` ;
- p194 et p202 : blanches ;
- p203 : `DRIVE SHAFTS` ;
- traitement à refaire depuis le SAFE CHECKPOINT `f2ac8d5d7039d9d242b146882b4c29f25f2c468f` ;
- aucun ancien fragment p193–202 ne doit être réutilisé comme base de reprise.

## But recherché
La fiabilité ne dépend plus du montage de fichiers de ChatGPT ni du transport manuel de gros binaires. GitHub devient à la fois la source documentaire, l’environnement de génération, le contrôleur d’intégrité et le point de reprise vérifiable.
