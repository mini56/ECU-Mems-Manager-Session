from pathlib import Path

report = Path('RAPPORT_SUIVI_ECU_MEMS_MANAGER.md')
current = report.read_text(encoding='utf-8')
marker = '## 2026-08-30 - CORRECTION CRITIQUE CHECKPOINT 1860'
if marker in current:
    print('correction already present')
    raise SystemExit(0)
text = r'''

## 2026-08-30 - CORRECTION CRITIQUE CHECKPOINT 1860

Correction explicite des valeurs erronees inscrites dans le checkpoint precedent apres reprise depuis un resume de session. Les controles ont ete refaits directement sur le PDF utilisateur et contre les entrees historiques 1860 du rapport maitre.

- **Source RCL0193FRE correcte et verifiee directement** : `Manuel Rover MPI(2).pdf`, **67009217 octets**, **371 pages**, SHA-256 **`0c7fef28d0d0f0673ba321d6625a019c005823103caa98afb3258114e1fec713`**.
- Cette identite correspond exactement a la source historique des lots RCL0193FRE deja enregistree dans le rapport. Les valeurs `27320889` octets et `0c7fef287294...` inscrites dans le checkpoint precedent sont **A IGNORER / INVALIDES**.
- **TIFF final 1860 correct** : SHA-256 **`d25fd347ccf9fd795c635fffff5a0dcc2800bbe33e185416c9a188d034b8f2c4`**.
- **XZ final 1860 correct** : SHA-256 **`fbd982995304a41f978fc61280b83fbead120591e898bb66716ad70685da9a08`**.
- Le SHA TIFF `9e1a984e...` mentionne dans le checkpoint precedent est **A IGNORER / INVALIDE pour le candidat final 1860**.
- SQL 1860 attendu : SHA-256 **`1cf7cd58e71d436fc701401d841f0a4b521c9f386ff23a5720d8fb7a5b86fd54`**.
- QZ64 1860 attendu : SHA-256 **`18e859c236a7e865b3416e0083f024e872bebda540c2dbf15ce233c0900c18be`**.

### PROCHAINE ACTION EXACTE CORRIGEE

Retrouver/reproduire le rendu historique des 45 pages 324-371 sauf 325, 327 et 359 a partir de la source exacte ci-dessus, et exiger d abord le TIFF SHA `d25fd347...` puis le XZ SHA `fbd98299...`. Aucun transport 1860 ne doit etre pousse avant creation et relecture distante du SAFE CHECKPOINT complet dans ce rapport maitre.
'''
report.write_text(current + text, encoding='utf-8', newline='\n')
print('critical 1860 correction appended')
