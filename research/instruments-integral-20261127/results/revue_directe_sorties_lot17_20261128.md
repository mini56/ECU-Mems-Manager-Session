# Lot 17 — vérifications directes et qualification des 44 pistes
28/11/2026. Périmètre exclusif MEMS 1.6 NA / 1.9. Suite du run 46135234985 (342 ASM, 338 ROM, 56 témoins, 44 profils hors catalogue).
## Cinq pistes supplémentaires 1.9 : preuve directe ASM et XF
| Firmware | ASM blob | XF blob | XOR / sortie 1FFE | Chargement période | Échéance | Identité XF |
|---|---|---|---|---|---|---|
| KLH3G018 | fa75ced10aac49bf53c7a54bcc8d3fc3a9c7f413 | 2588a72949fe5a7368b958e63c1e5ae92e729bbf | 4678 / 467B | 4680 | 4685/468A | 900018 |
| KLHCG000 | 02f831a1ea63c0fec84679daae8e202758df77c3 | 84f5045d34d10ebeb9dc0004eea6fd7da8dccb73 | 463A / 463D | 4642 | 4647/464C | 900000 |
| KUH3G001 | 1eb7102a8ce908feac4f92d10d61c240c0abf377 | fbde6118fa02b887ac39a0ac8616aad0a686bff5 | 449B / 449E | 44A3 | 44A8/44AD | 900001 |
| REGI0872 | e4092b0825cd3d3c7ab82eafbfac724a5e9519e0 | 40a5edea7a38534ef5c7736ce4fb0f1f1a2291ab | 4493 / 4496 | 449B | 44A0/44A5 | 900872 |
| REGI1345 | d98962c7ee47943ab311e117b805ceb39929187c | 6b4689b7b4d306e74abb48ec89798bece6fa2653 | 463A / 463D | 4642 | 4647/464C | 901345 |

Chacune de ces routines vérifie 0308/20 et 030E/20, bascule 22/20, écrit 1FFE, charge 14h[40] et ajoute la durée à 0340 avant de sauvegarder la prochaine échéance. Les cinq XF nomment tacho_sw_time (offset 0014 en XF3, 20 décimal en XF1) et tacho_next_rti (0340). Les identités ci-dessus sont nominales (trois octets), pas des identifiants uniques complets. Le binaire et les commandes/rétablissement ne sont pas encore vérifiés dans ce lot manuel. Cinq nouveaux profils à générateur de compte-tours confirmé statiquement, sans autorisation d'intégration.
## Qualification automatisée ciblée
Script qualify_physical_lot17.py : reprend exactement les 44 pistes du registre lot 16, conserve les ASM et XF pertinents, vérifie les tables normales, les écritures réelles de drapeaux de forçage et d'annulation, les chargements calibrés et les collisions d'identité nominale. Les quatre ASM orphelins restent distincts. Pas de présomption de sécurité ni de validation physique. Les ROM binaires ne sont pas lisibles directement par le connecteur texte, d'où la vérification sur le runner. Aucun changement Manager ni DLL.