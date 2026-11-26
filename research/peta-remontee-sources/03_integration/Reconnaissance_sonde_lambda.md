# Manager — état de reconnaissance de la sonde lambda
Nouveaux champs internes PETA, distincts de la tension, du défaut courant et de la régulation déjà présentés dans Manager (SESSION 216, 488dc8c, summarytab.cpp et mappedinjection_runtime_part4.inc).
| Symbole | AANMP002 | KBHBG009 | Signification |
|---|---|---|---|
| f_o2h_counter | C8 | AA | Compteur restant, 8 tours/unité |
| f_fbk_dis_count | FC/02 | DA/02 | Délai de reconnaissance expiré (pas un verdict de réussite) |
| f_o2s_fitted | FC/08 | DA/08 | Sonde reconnue par stratégie |
| f_o2s_fault_log | 52/80 | 3D/01 | Défaut de sonde mémorisé |
AANMP002 : 5DB5/5DDC chargent C8 depuis ROM 86FA; 5DD1–5DD6 testent zéro et positionnent FC/02; 5E03–5E21 évaluent tension et positionnent FC/08 si hors fenêtre. 5E16/5E19 mémorisent défaut et défaut courant si délai expiré sans reconnaissance. 20B2–20D4 décrémente C8 indirectement avec saturation zéro; 5E8F/5E92 autre chemin de défaut.
KBHBG009 : 6D71/6D87 chargent AA depuis ROM 9544; 6D7C–6D81 test zéro; 6DB1–6DDA détectent tension hors fenêtre et lancent délai AF; 6DDF–6DEA reconnaissent sonde à expiration de AF. 6DC4/6DC7 mémorisent défaut si délai expiré sans reconnaissance; 6E5B/6E5E autre chemin. 22DD–22FF décrément indirect saturé.
Les deux calibrations contiennent 250, soit 2000 tours nominaux selon XF. Fenêtre de reconnaissance des deux profils : valeurs 59 et 143, facteur PETA 0,01961 V, soit environ 1,157 et 2,804 V; ne pas confondre avec seuils usuels d'une sonde 0–1 V ni avec conversion de la trame. KBHBG009 délai additionnel AF chargé à 100 (200 tours nominaux, XF), distinct du compteur principal.
Proposition : afficher attente/reconnaissance/défaut courant et mémorisé séparément; ne pas déclarer sonde HS uniquement parce que non reconnue, ni chauffage physiquement opérationnel. Lecture RAM en mode 4 candidate avec lecteur Manager existant; pas de nouvelle commande active, pas de lecture ECU de ces adresses effectuée. Pas de validation de fréquence réelle des décréments au banc. Comparaison limitée aux sources Manager citées, pas de modification Manager/DLL ni compilation.
