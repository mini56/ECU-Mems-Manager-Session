# Réglages MEMS 1.6 NA et 1.9 — complément V1.29 (27/11/2026)
Référence Manager SESSION 216 : 488dc8cd7dc6a8ec6e25bf8905597fb36e644006. Recherche statique uniquement. Extension de l'action capture ralenti chaud (registre V128) : ne pas compter CB comme un cinquième paramètre ajustable. 
## CB : preuve de copie
| Firmware | RAM source i_step_pos | RAM destination i_hotdbpos | Routine ROM | Table dispatcher (base, décalage décimal) |
|---|---|---|---|---|
| AANMP002 (1.6 NA) | 008E | 0042 | 3DE9 | entrée 4118 (V111) |
| KBHBG009 (1.9) | 0060 | 0030 | 5EEF | 5678 + 236 |
| KBHBG010 (1.9) | 005E | 0030 | 5EEF | 56CC + 236 |
| KBHBV001 (1.9) | 0060 | 002D | 5112 | 5446 + 176 |
| KBHBV002 (1.9) | 0060 | 0030 | 5EEF | 5678 + 236 |
ASM KBHBG009/002V : ldb INTMEM_30, INTMEM_60 ; ljmp ROM_44EB (efface F8, saut 3A46). KBHBG010 : ldb INTMEM_30, INTMEM_5E ; ljmp ROM_4527. KBHBV001 : ldb INTMEM_2C+1, INTMEM_60 ; ljmp ROM_42C4. XF3 confirme nom, adresse et unité (pas). La copie directe ne vérifie ni température, ni stabilité, ni limites de réglage. Aucune persistance démontrée.
## Quatre réglages indépendants
| Réglage | Commandes +/- | AANMP002 | KBHBG009/010, KBHBV002 | KBHBV001 | Bornes brutes |
|---|---|---|---|---|---|
| Correction injection | 79/7A (alias 7B/7C) | 0043 | 002D | 002E | 00–FE |
| Position ralenti chaud | 89/8A | 0042 | 0030 | 002D | 0A–3C |
| Correction régime | 91/92 | 0045 | 002F | 0030 | 78–88 |
| Correction avance | 93/94 | 0044 | 002E | 002F | 74–8C |
Tables 1.9 à 55CA (009, V002), 561E (010), 5398 (V001) : cinq lignes, deux alias pour carburant. 52FB–5338 (009) itère la table, compare bornes, ajuste d'une unité, écrit RAM et transmet réponse. XF3 des quatre profils : f_adpt_offset = -1024+8*x µs ; i_setpt_rosco = -3200+25*x tr/min ; i_hotdbpos = x pas ; ign_offs_rosco = 64-0,5*x degrés. Conversions RAM, non application aveugle aux octets de trame transformés. 7F et 7E (009, routines 5339 et 5341) imposent 86h dans 002E et 82h dans 002F : pas de cinquième réglage. Ne pas envoyer ces commandes arbitrairement.
## Manager 216
rosco.h 81–88 : quatre paires existantes ; memsinterface.cpp 655 et suivantes les appelle. mainwindow.cpp 1432 affiche correction carburant brute moins 128 sans facteur 8 ; 1433 affiche position chaude moins 35 (décalage par rapport à 35, pas position absolue). Correction avance 1198 déjà convertie depuis la trame. Aucun bouton ou appel CB identifié dans rosco.h, memsinterface.cpp, memsinterface_dispatch.cpp, mainwindow.cpp, settingspage.cpp, mems19protocol.h, memslogic.cpp (recherche ciblée). Affichage et contrôle de lecture à vérifier profil par profil, notamment 80/10 et 80/12 (source V111 et rapprochements des trames 1.9) ; pas d'envoi CB tant que transport, confirmation et relecture ne sont pas vérifiés. Moteur chaud au ralenti stabilisé, position fraîche et comprise dans la plage manuelle 10–60, accord explicite ; vérifier résultat sans confondre ACK et persistance ; sauvegarde séparée sur demande. 
Périmètre : 10 firmwares 1.6 NA (V111, preuves archivées), 4 firmwares 1.9 nouvellement recoupés ASM/XF3 ; pas exhaustif, pas 1.6 Turbo. Aucun nouveau paramètre indépendant trouvé dans ces tables ; extension CB et unités confirmées statiquement. Aucune modification du Manager/DLL, compilation ou essai réel.
