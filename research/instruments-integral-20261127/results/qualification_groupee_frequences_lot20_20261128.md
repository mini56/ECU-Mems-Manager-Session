# Lot 20 — qualification groupée des 40 candidats compte-tours à période fixe
28/11/2026. Recalcul à partir des preuves individuelles du lot17 (`qualification_sorties_lot17.json`), recoupement avec le bilan des identités du lot18. Aucun nouveau scan, aucun run, aucun changement Manager/DLL.

## Résultat
- 40 profils, 40 avec contrôles de hash ROM, accord offset XF, activation, désactivation et restauration présents dans le registre.
- 40 hashes ROM distincts.
- 39 profils : 5000 us / 100 Hz (fréquence théorique, basculement par demi-période). KLH3G015, KLH3G017, KLH3G018, KLH3G019, KLH3G020, KLH3V005, KLH3V006, KLHCG000, KLHCG001, KLHCG002, KLHCV000, KLHCV001, KUH3G000, KUH3G001, KUH3G002, KUH3G003, REGI0872, REGI1224, REGI1345, REGI1346, REGI1350, REGI1355, REGI1356, REGI1359, REGI1360, REGI1361, REGI1362, REGI1363, REGI1364, REGI1365, REGI1366, REGI1367, REGI1368, REGI1374, REGI1376, REGI1383, REGI1384, REGI1386, REGI1387.
- 1 profils : 3751 us / 133.297787 Hz (fréquence théorique, basculement par demi-période). SELFG008.
- 25 identités nominales à trois octets sans collision recensée ; 15 avec collision. D1 (nom huit caractères et cohérence D0 reçu) nécessaire pour tous ; absence de collision nominale ne valide pas la réponse réelle.
- Proposition technique : une seule architecture de test fréquence fixe avec fréquence attendue par profil, distincte du mode simulation de régime. Ne pas afficher un régime attendu sans rapport impulsions/tour démontré. Pas de validation physique ni de preuve nouvelle du câblage ; autorisations supplémentaires : 0. Les quatre ASM seuls et 242 autres restent hors de cette consolidation.

| Profil | Identité nominale | Collision | Demi-période µs | Hz | Entrées 6B / 63 / F4 | Contrôles du lot17 |
|---|---|---|---:|---:|---|---|
| KLH3G015 | 900015 | KLH3G015, KRHBG015, KTHBG015, TBP7F015 | 5000 | 100 | 54DF / 54E9 / 4DDE | Présents |
| KLH3G017 | 900017 | KLH3G017, KLH4G017, KRHBG017, TBH7G017 | 5000 | 100 | 566E / 5678 / 4F8B | Présents |
| KLH3G018 | 900018 | KLH3G018, KLH4G018, KRHBG018, TAH7G018 | 5000 | 100 | 56C4 / 56CE / 4FC3 | Présents |
| KLH3G019 | 900019 | KLH3G019, KLH4G019, TAHHG019, TBH7G019 | 5000 | 100 | 56C4 / 56CE / 4FC3 | Présents |
| KLH3G020 | 900020 | KLH3G020, KLH4G020 | 5000 | 100 | 56C4 / 56CE / 4FC3 | Présents |
| KLH3V005 | C70005 | Non recensée | 5000 | 100 | 5447 / 5451 / 4D64 | Présents |
| KLH3V006 | C70006 | Non recensée | 5000 | 100 | 566E / 5678 / 4F8B | Présents |
| KLHCG000 | 900000 | KIHBG000, KLHBG000, KLHCG000, KLQ4G000, KTHHG000, KUH3G000, KUHHG000 | 5000 | 100 | 5647 / 5651 / 4F8B | Présents |
| KLHCG001 | 900001 | KFACG001, KLHBG001, KLHCG001, KLQ4G001, KUH3G001, KUHHG001 | 5000 | 100 | 564A / 5654 / 4F8B | Présents |
| KLHCG002 | 900002 | KFACG002, KLHBG002, KLHCG002, KLQ4G002, KUH3G002 | 5000 | 100 | 56A0 / 56AA / 4FC3 | Présents |
| KLHCV000 | D10000 | KLHBV000, KLHCV000 | 5000 | 100 | 5647 / 5651 / 4F8B | Présents |
| KLHCV001 | D10001 | KLHBV001, KLHCV001 | 5000 | 100 | 564A / 5654 / 4F8B | Présents |
| KUH3G000 | 900000 | KIHBG000, KLHBG000, KLHCG000, KLQ4G000, KTHHG000, KUH3G000, KUHHG000 | 5000 | 100 | 54F3 / 54FD / 4DE6 | Présents |
| KUH3G001 | 900001 | KFACG001, KLHBG001, KLHCG001, KLQ4G001, KUH3G001, KUHHG001 | 5000 | 100 | 54F3 / 54FD / 4DE6 | Présents |
| KUH3G002 | 900002 | KFACG002, KLHBG002, KLHCG002, KLQ4G002, KUH3G002 | 5000 | 100 | 5466 / 5470 / 4D77 | Présents |
| KUH3G003 | 900003 | KLQ4G003, KUH3G003 | 5000 | 100 | 54C0 / 54CA / 4DB3 | Présents |
| REGI0872 | 900872 | Non recensée | 5000 | 100 | 54DF / 54E9 / 4DDE | Présents |
| REGI1224 | 901224 | Non recensée | 5000 | 100 | 56D8 / 56E2 / 4FCB | Présents |
| REGI1345 | 901345 | Non recensée | 5000 | 100 | 5647 / 5651 / 4F8B | Présents |
| REGI1346 | 901346 | Non recensée | 5000 | 100 | 5647 / 5651 / 4F8B | Présents |
| REGI1350 | 901350 | Non recensée | 5000 | 100 | 569D / 56A7 / 4FC3 | Présents |
| REGI1355 | 901355 | Non recensée | 5000 | 100 | 569D / 56A7 / 4FC3 | Présents |
| REGI1356 | 901356 | Non recensée | 5000 | 100 | 569D / 56A7 / 4FC3 | Présents |
| REGI1359 | 901359 | Non recensée | 5000 | 100 | 569D / 56A7 / 4FC3 | Présents |
| REGI1360 | 901360 | Non recensée | 5000 | 100 | 569D / 56A7 / 4FC3 | Présents |
| REGI1361 | 901361 | Non recensée | 5000 | 100 | 56A0 / 56AA / 4FC3 | Présents |
| REGI1362 | 901362 | Non recensée | 5000 | 100 | 56A0 / 56AA / 4FC3 | Présents |
| REGI1363 | 901363 | Non recensée | 5000 | 100 | 56A0 / 56AA / 4FC3 | Présents |
| REGI1364 | 901364 | Non recensée | 5000 | 100 | 56A0 / 56AA / 4FC3 | Présents |
| REGI1365 | 901365 | Non recensée | 5000 | 100 | 56A0 / 56AA / 4FC3 | Présents |
| REGI1366 | 901366 | Non recensée | 5000 | 100 | 56A0 / 56AA / 4FC3 | Présents |
| REGI1367 | 901367 | Non recensée | 5000 | 100 | 56A0 / 56AA / 4FC3 | Présents |
| REGI1368 | 901368 | Non recensée | 5000 | 100 | 564A / 5654 / 4F8B | Présents |
| REGI1374 | 901374 | Non recensée | 5000 | 100 | 56C4 / 56CE / 4FC3 | Présents |
| REGI1376 | 901376 | Non recensée | 5000 | 100 | 56C4 / 56CE / 4FC3 | Présents |
| REGI1383 | 901383 | Non recensée | 5000 | 100 | 56A0 / 56AA / 4FC3 | Présents |
| REGI1384 | 901384 | Non recensée | 5000 | 100 | 56A0 / 56AA / 4FC3 | Présents |
| REGI1386 | 901386 | Non recensée | 5000 | 100 | 5647 / 5651 / 4F8B | Présents |
| REGI1387 | 901387 | Non recensée | 5000 | 100 | 564A / 5654 / 4F8B | Présents |
| SELFG008 | 900008 | KBHHG008, KFHBG008, KJH6G008, KTH6G008, SELFG008 | 3751 | 133.297787 | 57E3 / 57ED / 5048 | Présents |

Les valeurs sont recalculées sur les données enregistrées et ne constituent pas une mesure au banc. La fréquence 133,297787 Hz de SELFG008 doit rester distincte de 100 Hz. Ne pas annoncer 40 nouvelles découvertes : les 40 étaient déjà recensés au lot17. Ce lot consolide leurs différences et les bloqueurs d'intégration.
