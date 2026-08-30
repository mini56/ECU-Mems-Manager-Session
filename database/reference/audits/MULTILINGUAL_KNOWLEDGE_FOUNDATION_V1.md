# MULTILINGUAL KNOWLEDGE FOUNDATION V1

Status: prototype additif hors production.

Base de reference: BUILD #103 `1d6316bd1746d6f2b4cfb751cab88d18e27ef730`.

## Objectif

Definir un socle documentaire generique, multilingue et extensible avant tout backfill massif RAVE/AKM7169/Mini MPi. Les pages de manuel restent des sources de tracabilite; les connaissances, valeurs, procedures, tableaux, vues et schemas sont des objets structures affichables nativement par MEMS Manager.

## Principes

- aucune colonne figee par langue (`text_fr`, `text_en`, etc.) dans le nouveau socle;
- traductions stockees par `locale` dans `mems_doc_text`;
- une seule ressource visuelle originale, intacte, reutilisable par toutes les langues;
- zones/labels visuels modelises separement avec coordonnees normalisees;
- tableaux et cellules structures, avec valeurs numeriques separees du texte traduit;
- valeurs/specifications structurees via `mems_doc_value`;
- procedures/etapes/avertissements representables comme entites hierarchiques;
- liaison possible avec les tables historiques via `legacy_table` + `legacy_key`;
- audit page/section via `mems_doc_unit` avec etats utile/deja couvert/hors perimetre/etc.;
- ajout d'une nouvelle langue par donnees de locale/traduction, sans migration de schema.

## 11 tables ajoutees

1. `mems_doc_locale`
2. `mems_doc_document`
3. `mems_doc_unit`
4. `mems_doc_entity`
5. `mems_doc_text`
6. `mems_doc_visual`
7. `mems_doc_visual_region`
8. `mems_doc_table`
9. `mems_doc_table_cell`
10. `mems_doc_value`
11. `mems_doc_relation`

Le lot prototype fixe `PRAGMA user_version=21`. Il n'est PAS reference dans `manifest.json` et reste sous `database/reference/prototypes/`; le runtime #103 ne peut donc pas le decouvrir/appliquer automatiquement.

## Locales initiales

`fr`, `en`, `es`, `it`, `pt`, `de`.

Le schema permet l'ajout ulterieur de `ja`, `zh-CN`, `hi` ou toute autre locale par simple insertion, sans ajout de colonne.

## Validation locale sur le SQLite reel de BUILD #103

Avant application:
- `integrity_check=ok`
- `user_version=20`
- tables=78
- `mems_knowledge_item`=1113
- `mems_rave_illustration`=126
- `mems_procedure`=410
- `mems_specification`=649

Apres application du prototype:
- `integrity_check=ok`
- `foreign_key_check=0`
- `user_version=21`
- tables=89
- `mems_knowledge_item`=1113
- `mems_rave_illustration`=126
- `mems_procedure`=410
- `mems_specification`=649

Donc 11 tables sont ajoutees sans modifier les compteurs historiques controles.

## Hashes

- SQL: 8295 octets
- SHA-256 SQL: `4a89004a556af7771a80592534c3b4e379733c7fe9c0f0dc6f79761dfe304546`
- QZ64: 2261 octets
- SHA-256 QZ64: `0df680bd40450ce57be927e5ee3ec0ef40c8a2962d12c939e2f4f00202301487`

## Validation distante attendue

Le workflow temporaire `temp-multilingual-foundation-validate.yml` telecharge l'artefact officiel BUILD #103, extrait son SQLite expert, verifie son etat de reference, applique ce prototype, controle integrite/FK/compteurs, puis insere uniquement dans une copie de test un petit jeu heterogene pour verifier la traduction generique par locale, l'ajout futur de locales sans changement de schema, le partage d'un meme visuel par plusieurs langues, les labels visuels traduisibles, un tableau structure, une procedure/etape, une valeur structuree et les relations entre entites.

Aucune donnee prototype de validation n'est destinee a la production.
