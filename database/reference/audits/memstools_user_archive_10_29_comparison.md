# Comparaison MEMSTools utilisateur 10.29 vs archive Andrew courante

- Ancien ZIP SHA-256 : `f652c214641c0c5ffb3d379fe8062dae818e99e5a3c2e47f31165f7ea76d5f6d`
- Ancien ZIP : **1189 fichiers**
- Ancien MemsMapper : **10.29 Release**
- Archive Andrew courante SHA-256 : `12bb7dfc8c1f3bed1eed28b85b3bb2c344e4d7b5fcb0534e584092aaeb7ff1e8`

## Comparaison technique byte-par-byte

- Fichiers cœur comparés individuellement : **21**
- Assets techniques courants détectés : **374**
- Identiques : **23**
- Modifiés : **0**
- Absents du courant : **0**

- correlation: identical=16, changed=0, missing_current=0
- definition: identical=2, changed=0, missing_current=0
- disassembly: identical=4, changed=0, missing_current=0
- firmware_image: identical=1, changed=0, missing_current=0

## Volumes sémantiques

| Table | 10.29 | Courant | Identique |
|---|---:|---:|:---:|
| `mems_firmware_catalog` | 286 | 286 | oui |
| `mems_axis_definition` | 320 | 320 | oui |
| `mems_scalar_definition` | 18519 | 18519 | oui |
| `mems_scalar_binding` | 96240 | 96240 | oui |
| `mems_table_definition` | 1815 | 1815 | oui |
| `mems_table_binding` | 17458 | 17458 | oui |
| `mems_variable_correlation` | 64706 | 64706 | oui |

**Volumes sémantiques identiques : OUI.**
**Corpus corrélations + DEF/DIM byte-identique : OUI.**

## Fichiers techniques différents

Aucun.

## Connaissances historiques conservées

- MEMS 1.3 : chemins service 5 ABEMR002/KBE6R003/MGE7R002 et corrections candidates conservés avec provenance.
- Les corrélations C8/C6 ne sont pas promues en mapping universel.
- La conversion ABEM approximative 0.0015 ms/unité reste `non_verifie`.
- MEMS 1.2 : absence de corpus de corrélation explicite conservée comme lacune réelle.

Aucune donnée de cette archive historique n’écrase les faits `decoded_by_project` ni les mesures ECU réelles.
