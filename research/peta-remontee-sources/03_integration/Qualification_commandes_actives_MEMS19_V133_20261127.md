# MEMS 1.9 — qualification DA/DB, variantes 6B/63, sorties auxiliaires
27/11/2026 — V1.33. Analyse statique du dépôt, aucun envoi ECU, aucune modification Manager/DLL.
## 1. Injection : contrôle électrique par groupe
TFH7G029 ASM 5462–54DB : DA sélectionne 4BC5 seul (canal 3), DB 4BE6 seul (canal 4), EF/F7 les deux ; XF3 inj1_pw_pending 004A, inj1_pw_fired 03C2, inj2_pw_pending 004C, inj2_pw_fired 03C4 ; 4BC5 sauvegarde 004A→03C2, 4BE6 004C→03C4. Donc groupe Rover injecteur 1/2 prouvé, association aux cylindres non établie. 5474→4842 retourne 0 si régime non nul à l'entrée. 5503–5536 sauvegarde puis efface temporairement les bits de défaut AA/AB de 003A (XF3 flagbyte12), sauvegarde aussi io_inj_1or2 ; 549D–54B9 contrôle matériel WSR1 08/10 et éventuelle protection 4EE0, 4F99 ; 54BB–54C1 retourne 1 si bits matériels inactifs ; 54C4–54DB retourne 2 si défaut AA/AB détecté ou voie résiduelle ; 5537–554B restaure les anciens défauts mémorisés. Ne pas assimiler 1 à un injecteur mécaniquement sain, ni 2 à une cause physique unique. Aucun essai réel.
inj_pw_rosco à offset 000E, XF3 facteur 0,001 ms, mot 2000 pour TFH7G029/TAHHG019, 1999 pour KLH4G018/KBHBG009/KLH3G019/KLHCG002. Valeur de calibration de largeur d'impulsion, pas durée totale du test.
SESSION 215 memsinterface.cpp onFuelInjectorTest appelle guardedMemsTestActuator(F7,NULL) sans exploiter le résultat dans cette fonction. Intégration candidate : diagnostic électrique groupes 1/2, préserver historique défauts, interdire moteur tournant ; ne pas afficher OK mécanique. Vérifier décodage de retour et protection anti répétition avant essai.
## 2. Compte-tours : deux variantes non interchangeables
KLH3G019 6B/63→56C4/56CE et KLHCG002→56A0/56AA : signature b3 01 0e 03 f8 91 20 f8 20 08 (mise bit 5 de 030E) / b3 01 0e 03 f8 71 df f8 (effacement) ; suite commune écrit 030E, efface bit 5 de 0308 (XF3 iq_tacho_fmsk) puis efface bit 5 de 0022 et écrit 1FFE. Pas de chargement de consigne de régime ; les offsets 18/1A sont des constantes de stepper dans leurs XF3. Ne PAS utiliser un libellé 3000 tr/min. Effet électrique exact et câblage à qualifier.
TAHHG019, TBH7G019, TFH7G028 : signature b3 01 0e 03 f8 91 20 f8 a3 41 1a 00 f6 20 0d, retour symétrique +18, écriture 0156 (XF3 tacho_display_speed). TFH7G029 écrit 0158 ; TAH7G018 possède la paire, calibration vérifiée V132, handler complet à confirmer. Consignes 3000/0 prouvées dans les cinq calibrations V132 et TAHHG019 ici ; pas de lecture position aiguille. 
## 3. Sortie auxiliaire supplémentaire 20/30
KLH3G019 30→5730 pose bit6 de 030E, 20→573A efface ; KLHCG002 30→5733 et 20→573D même code. Efface bit6 de masque 0308, écrit 0308/030E. Le XF3 de ces profils marque le bit6 de flagbyte9 (030E) NULL ; les labels cruise/EBF existent ailleurs, non sur ce bit. Ne pas attribuer à HDC, ventilateur ou régulateur sans suivre le registre jusqu'à sa sortie et vérifier le schéma. 21/31 déjà documentées V130 ch166 (régulateur selon profil), pas nouvelle fonction. 22/32 supplémentaires KLH4G020/REGI1341 ; libellé de sortie partagé ne prouve pas l'organe.
## 4. Matrice supplémentaire (16 HEX)
Repérage par D0 D0/F0 F0/F9 F9 ; limite à la deuxième occurrence exacte 0D/0D, qui sépare ici la table directe de la suite d'états internes dans les 16 profils. Attention : borne heuristique, pas vérification complète du sélecteur. Les entrées présentées précèdent la borne. Un motif unique par fichier.
| Profil | DA | DB | EC | 6B | 63 | 22/32 | 21/31 | 20/30 |
|---|---|---|---|---|---|---|---|---|
|KBHBV001|4EA3|4EA8|-|-|-|-|-|-|
|KFACG001|4EF7|4EFC|-|-|-|-|-|-|
|KFHBG008|50CA|50CF|-|-|-|-|-|-|
|KIH6G011|50CA|50CF|-|-|-|-|-|-|
|KJH6G011|5126|512B|-|-|-|-|-|-|
|KLH3G019|51D6|51DB|-|56C4|56CE|-|-|573A/5730|
|KLH4G020|51C4|51C9|-|-|-|56BC/56B2|-|-|
|KLHBG002|511E|5123|-|-|-|-|-|-|
|KLHCG002|51D6|51DB|-|56A0|56AA|-|-|573D/5733|
|KTHBG015|511E|5123|-|-|-|-|-|-|
|KUHHG001|5126|512B|-|-|-|-|-|-|
|REGI1341|521A|521F|-|-|-|5712/5708|-|-|
|REGI1378|5126|512B|-|-|-|-|-|-|
|TAHHG019|531F|5324|5698|587A|5889|-|58D1/58C7|-|
|TBH7G019|5331|5336|56AA|588C|589B|-|-|-|
|TFH7G028|53CF|53D4|5748|5951|5960|-|-|-|
Cumul V132+V133 : 24 profils, DA/DB 24, EC 5, 6B/63 7 (2 logiques, 4 simulations avec handler vérifié dont TFH7G029, 1 TAH7G018 à confirmer), 22/32 6, 21/31 1, 20/30 2. Pas de généralisation à tous les 1.9.
Sources : ASM TFH7G029, HEX/XF3 profils listés, rapport V1.32 ch166/193/194, code SESSION 215 commit 475243db18c3099595e1a5928b6916185380a808. Candidat nouveau : interprétation du contrôle électrique des injecteurs ; séparation stricte des variantes de compte-tours ; sortie 20/30 à identifier. Pas de validation sur véhicule.