# MEMS 1.9 — générateur périodique : cinq candidats supplémentaires
Sources du corpus 9c6c443eabcf5c23dd1235b7e9c892de50802dd7, ASM, XF1/XF3 et ROM de chaque profil. Complément au lot KLH3G015.
Pour chacun : 6B positionne 030E/20h, 63 l'efface ; les deux effacent 0308/20h et initialisent la sortie 1FFE à zéro. Le générateur teste les deux masques, bascule 22/20h par XOR puis écrit 1FFE ; délai calibré ajouté à (33C+4)=0340. F4 appelle la routine qui repositionne 0308/20h (retour stratégie). XF1/XF3 identifie tacho_sw_time et tacho_next_rti ; les offsets varient (SELFG008 = 16h contre 14h).
| Firmware | 6B | 63 | F4 | XOR | Écriture sortie | Restauration masque | Adresse calibration | Octets | Délai ms | Hz théorique |
|---|---|---|---|---|---|---|---|---|---:|---:|
| KLH3G017 | 566E | 5678 | 4F8B | 463A | 463D | 57AE | 9414 | 88 13 | 5 | 100.000 |
| KLH3V005 | 5447 | 5451 | 4D64 | 4413 | 4416 | 5574 | 9514 | 88 13 | 5 | 100.000 |
| REGI1361 | 56A0 | 56AA | 4FC3 | 4678 | 467B | 57EC | 9414 | 88 13 | 5 | 100.000 |
| KUH3G000 | 54F3 | 54FD | 4DE6 | 449B | 449E | 5624 | 9514 | 88 13 | 5 | 100.000 |
| SELFG008 | 57E3 | 57ED | 5048 | 46FE | 4701 | 58D8 | 9516 | a7 0e | 3.751 | 133.298 |
Statut : cinq nouveaux candidats forts, en plus de KLH3G015, avec chemin de pilotage documenté, mais fréquence physique, correspondance régime du combiné et protections non validées sur banc. Aucune modification Manager/DLL. Ne pas confondre 63 (arrêt du signal forcé) et F4 (retour à la stratégie).
