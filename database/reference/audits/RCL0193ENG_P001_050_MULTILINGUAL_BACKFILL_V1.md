# RCL0193ENG — pages physiques 1–50 — backfill multilingue V1

Source exacte : `rave/xn/wmxn990e.pdf` — **RCL0193ENG Mini Workshop Manual, 5th Edition**.

- PDF : **4 744 911 octets**, **372 pages physiques**.
- SHA-256 PDF : `c050a3eebe50c5a85bf8a69b7722bd2052079944e09d58578a498984ecf06715`.
- Lot audité : pages physiques **1–50**, sans déduction depuis les anciens tokens de noms d'images.
- Branche de travail : `tmp-rave-complete-multilingual-backfill`.
- Production protégée : `MEMSX64` reste BUILD #103 `1d6316bd1746d6f2b4cfb751cab88d18e27ef730`.
- Aucune modification protocole/ECU/UI/IA/ONNX ; aucune modification de table historique ; aucun changement de `manifest.json`.

## Disposition complète 1–50

- **10 pages blanches déjà prouvées visuellement** : `2, 8, 10, 14, 16, 18, 30, 34, 42, 48` → `out_of_scope / not_required`, raison `visually_confirmed_blank`.
- **9 pages de contexte/navigation** : `1, 3, 4, 5, 6, 7, 9, 15, 29` → `useful / integrated` par métadonnées de document/unité.
- **17 pages à contenu utile nouvellement structuré** : `11, 12, 13, 17, 19–28, 31–33` → `useful / integrated`.
- **14 pages déjà couvertes structurellement** : `35–41, 43–47, 49–50` → `already_covered / integrated` : les valeurs et tableaux correspondent exactement aux pages françaises RCL0193FRE 34–40, 42–46, 48–49 déjà intégrées par le lot 1790 ; le présent lot ajoute la traçabilité exacte vers la source anglaise sans recréer les lignes numériques historiques.

Total : **50 / 50 pages disposées explicitement**.

## Contrôle anglais ↔ français pour les pages 35–50

La copie historique exacte `Manuel Rover MPI.pdf` (RCL0193FRE, 371 pages) a été relue sur les pages physiques françaises 34–49. Le décalage est constant sur ce segment : **page anglaise = page française + 1**. Les valeurs correspondent, notamment :

- capacité réservoir **34,0 L** ;
- remplissage moteur/boîte + filtre **4,8 L** ;
- boîte automatique **5,1 L** ;
- circuit de refroidissement **4,0 L** ;
- lave-glace **2,3 L** ;
- couples de serrage, données générales, SPi manuel, SPi automatique et MPi concordants.

Conséquence : aucune duplication des ~350 spécifications du lot RCL0193FRE 1790. Les différences de langue sont traitées par le nouveau socle documentaire, pas par des copies de valeurs.

## Contenu nouveau structuré

Le lot ajoute au prototype multilingue la sémantique atelier utile absente du backfill numérique 1790 :

- niveaux WARNING/CAUTION/NOTE et validation après réparation ;
- politique de pièces de remplacement et dispositifs de blocage ;
- **glossaire complet des abréviations/symboles de la page 13** sous forme de table structurée ;
- identification VIN/peinture/garnissage et emplacements moteur/boîte/carrosserie ;
- sécurité allumage, substances dangereuses, huiles usagées et environnement ;
- levage général, hydraulique de frein, bouchons de refroidissement ;
- pratiques de filetage/couple, dispositifs de blocage, vis auto-freinées/encapsulées ;
- joints d'huile et risque fluoroélastomère ;
- sécurité essence, vidange/réservoir, réparation avec chaleur ;
- précautions électriques, déconnexion batterie, hydrogène de charge, ESD/connecteurs et graisse `BAU 5811` ;
- **10 points de levage/support/remorquage** structurés ;
- procédures de cric/pont, remorquage général, boîte manuelle et boîte automatique ;
- valeurs nouvelles normalisées : 0,4 mm, 15–60 min, ×150, 120 min, 15 min, 50 km/h, 50 km.

## Visuels

- Pages physiques **39, 40, 41** : réutilisation stricte des assets BUILD #103 déjà remappés : `RCL0193ENG_PDF_038.png`, `_039.png`, `_040.png`. Aucun binaire dupliqué.
- Pages physiques **17** et **31** : les vues constructeur VIN et points de levage sont enregistrées comme `visual_candidate`. Leur transport binaire est volontairement différé ; aucun dessin généré ou redessiné n'est substitué à la source originale.

## Fichiers du lot

- `database/reference/prototypes/rcl0193eng_p001_050_multilingual_v1.sql`
- `database/reference/prototypes/rcl0193eng_p001_050_multilingual_v1.qz64`
- `database/reference/audits/RCL0193ENG_P001_050_MULTILINGUAL_BACKFILL_V1.md`

## Validation locale

- application du socle exact `multilingual_knowledge_foundation_v1.sql` : SHA-256 `4a89004a556af7771a80592534c3b4e379733c7fe9c0f0dc6f79761dfe304546` ;
- application du lot puis **réapplication complète** du même lot : succès, donc idempotence vérifiée ;
- `PRAGMA integrity_check = ok` ;
- `PRAGMA foreign_key_check = 0` ;
- `PRAGMA user_version = 21` ;
- document : 1 ; unités : 50 ; entités : 244 ; textes source anglais : 285 ; tables : 2 ; cellules : 196 ; valeurs : 8 ; relations : 3 ;
- statuts unités : 10 blanches prouvées, 14 `already_covered`, 26 `useful` ;
- séquence physique : **1–50 exacte, sans trou ni doublon** ;
- glossaire : 88 lignes ; points de levage/support : 10 lignes ;
- QZ64 décodé byte-for-byte identique au SQL source.

Hashes du lot :

- SQL : **488301 octets**, SHA-256 `a1d5d4d286d4c425ed83d4df667ffec2eaf0014f0d62f4463faba3c6024914dd` ;
- QZ64 : **22757 octets**, SHA-256 `b8720f2c8f6d3718d829db757b00539de80da657a461f2dc407b69b4f9302f2e`.


## Règle de non-duplication

Les pages 35–50 ne recréent pas les spécifications historiques. Leur statut `already_covered` conserve la provenance anglaise exacte et permet le rattachement multilingue ultérieur aux entités canoniques. Les pages 39–41 réutilisent les images anglaises existantes et ne déduisent jamais la page physique depuis le token historique du nom de fichier.
