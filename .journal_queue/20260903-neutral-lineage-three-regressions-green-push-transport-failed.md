# Neutralisation générique — trois régressions vertes, push de transport refusé

Date: 2026-09-03

Run: 33805767194
Commit déclencheur: 2c539cb27f9b1b63a26ec9fca221abbd4283695e
Artifact: RAVEMEMS-V2-NEUTRAL-LINEAGE-THREE-DOCUMENT-REGRESSION, ID 9912881009, ZIP SHA256 97021b7758be311ab7ae193a1f0657d7d7dfb910a8b1a289a0274d33c1bfbc69

Résultat technique avant push:
- génération de la lignée générique neutre: PASS;
- compilation: PASS;
- aucun token RCL0193/rcl0193 dans le chemin d'exécution générique: PASS;
- RCL0179ENG 94 pages: 0 opérations / 0 phases / 0 étapes, 94 sections, 68/68 visuels exacts, audit 0, SQLite ok, FK 0;
- RCL0213ENG 212 pages: 0 opérations / 0 phases / 0 étapes, 212 sections, 304/304 visuels exacts, audit 0, SQLite ok, FK 0;
- RCL0193ENG 372 pages: 201 opérations, 397 phases, 3104 étapes, 738/738 visuels exacts, 401/401 liens, audit 0, SQLite ok, FK 0;
- garde globale: NEUTRAL_GENERIC_LINEAGE_THREE_DOCUMENT_GLOBAL_ZERO_PASS.

Le seul échec du run intervient après les tests au push du commit généré. GitHub refuse la mise à jour car le commit automatique incluait `.github/workflows/tmp-ravemems-v2-generic-source-rcl0179.yml` et le GITHUB_TOKEN du workflow n'a pas la permission `workflows`:
`refusing to allow a GitHub App to create or update workflow ... without workflows permission`.

Il ne s'agit pas d'un défaut du moteur ni d'une régression. Correction de transport prévue avant nouvelle pousse: ne pas modifier/embarquer de fichier workflow dans le commit automatique; refaire exactement les mêmes trois régressions, puis pousser uniquement les modules génériques neutralisés si elles restent vertes.

MEMSX64 reste protégé au BUILD #103 SHA 1d6316bd1746d6f2b4cfb751cab88d18e27ef730.
