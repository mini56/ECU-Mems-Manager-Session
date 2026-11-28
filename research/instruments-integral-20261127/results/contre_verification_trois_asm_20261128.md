# Contre-vérification directe — 28/11/2026
Sources figées : branche recherche commit 24147ee4c8953f8f230a6e75685490318a9ce840 ; Manager 228 31f96516ff67c21e7cc11d545c6aba6af5ccef59. Lecture directe des trois blobs ASM, sans CI, sans modification Manager/DLL.

| Référence | Blob ASM | Entrées table normale vérifiées | Forçage / consommateur / sortie / restauration |
|---|---|---|---|
| ABNMP000 | d91ba44d5f84c5300161d70afc941fcf64984ddb | table 3DEA : 00→3B82, 10→3B7D, F4→394D, D0→3924, D1→3AF1 | AC bits 6/7 (3B7D–3B85), 48F2–48FC produit 00/FF dans 01AC, 2695–26A2 HSO2 02/22, 37B3 efface AC6 |
| ABNMP003 | b443ffa66d75f2e5756eb3282bc90453516d02c9 | table 407C : 00→3D4E, 10→3D49, F4→3B19, D0→3AF0, D1→3CBD | AC bits 6/7, 4FA3–4FAD produit 00/FF dans 01AB, 2790–27A2 HSO2 02/22, 3979 efface AC6 |
| TFH7G031 | a1ea25db38634d2758db98a1aa4aab8e3b5098a5 | table 5A78 : 63→5960, 6B→5951, F4→51DA, D0→51AE, D1→53BD | 5959/5968 charge calibration, 596D écrit 0156, 5972 écrit 030E, 597F écrit 0308 ; 8AFD–8B2A calcule intervalle depuis 0156 ; 4878–48A1 bascule 22/20 et écrit port 1FFE, 51F9 appelle 59B2, 5A62–5A65 rétablit 0308/20 |

TFH7G031 : 333A charge INTMEM_40=9400h. Reconstruction octet par octet du bloc DB étiqueté 93F5 (lignes ASM 18019 et suivantes). Mot LE à 9418=0000 (63), à 941A=0BB8=3000 (6B). Les consignes ne sont pas supposées d'après un firmware voisin. Le nom statique et l'identité sont visibles dans le bloc 3E8E (lignes 5046–5049), D0 pointe sur 3EB4. ABNMP000/003 : DB extraits respectivement 345C+10h et 35A2+10h : noms ABNMP000/003 suivis de 99 00 00 14 / 99 00 03 14. Les valeurs statiques ne sont pas des réponses physiques mesurées.

Conclusion : trois candidats déjà recensés, confirmation indépendante de la chaîne statique et des consignes du compte-tours ; zéro nouveau profil découvert dans cette contre-vérification. Aucun des trois n'est dans la liste instruments 228. Les deux jauges nécessitent également une qualification du profil d'identification et des trames de sécurité. Les trois nécessitent qualification des trames de sécurité, réponse/restauration réelle et validation physique ; ne pas présenter comme intégration finalisée. Les 41 autres sorties périodiques ne sont pas validées par ce lot. Aucun run lancé.
