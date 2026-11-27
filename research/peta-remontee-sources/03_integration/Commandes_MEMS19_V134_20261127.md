# V1.34 — MEMS 1.9, 16 profils supplémentaires
Analyse HEX/XF3, aucun envoi ECU ni modification Manager/DLL. Base ROM 2000. Recherche par signature D0 D0/F0 F0/F9 F9 et borne heuristique deuxième 0D/0D. Les entrées relevées précèdent la borne ; pas de prétention à un inventaire exhaustif.
## 6F/67 : sortie auxiliaire
KLH3G015 6F→5510 /67→551A ; KLH3G017 56B1/56BB ; KLHCG000 56B1/56BB ; REGI1224 571B/5725. HEX KLH3G015 à 5510 : b3 01 0f 03 f7 91 02 f7 20 08 b3 01 0f 03 f7 71 fd f7 b3 01 09 03 f6 71 fd f6 c7 01 09 03 f6 c7 01 0f 03 f7 11 f8. Pose/efface bit1 de 030F puis efface bit1 masque 0309, retour 0. XF3 KLH3G015 et KLHCG000 flagbyte8/10 bit1 iq_cruise_ebf_fmsk/fdir. Même sortie logique que 22/32 dans d'autres profils, ne pas les essayer en cascade. Organe physique non établi selon véhicule. KLHCG000 même signature à 56B1.
## 2B/3B : climatisation
KLHCG000 2B→568A efface bit2 030E, 3B→5694 le pose ; efface bit2 0308. XF3 bit2 iq_aircon_fdir/fmsk, même bit que 03/13 déjà utilisé sur Manager. Alias de forçage, pas une nouvelle fonction ; ne pas dupliquer l'interface.
## 20/30 : non identifié
KLH3G015/KLH3G017/KLHCG000/REGI1224 possèdent cette paire en plus de 6F/67. XF3 KLH3G015/KLHCG000/KLH3G019 : bit6 de 030E, 0308, 001C et 0022 est NULL ; aucune attribution physique fiable. Ne pas intégrer pour l'instant.
## Autres pistes
8F rencontré KBHHG008, KFHHV002, SELFG007, TAH7V000, TBH7V000 ; 88 rencontré KJH6G012, KRHBG017, REGI1224. Fonction non établie : exclure des tests actifs. ASM non récupérables en texte (réponse vide), HEX disponibles. TAH7V000/TBH7V000 DA/DB handlers distincts dans un ordre d'adresses inversé, ne pas transposer mécaniquement les groupes des profils plus récents.
## Matrice
Profil;DA;DB;autres entrées repérées
FTST0152;5268;526D;22/32
KBHBV002;50CA;50CF;-
KBHHG008;4F39;4F3E;8F
KFHBG010;50CA;50CF;-
KFHHV002;4EA3;4EA8;8F
KJH6G012;50D2;50D7;88
KLH3G015;4FF1;4FF6;6B/63 6F/67 30/20
KLH3G017;5180;5185;6B/63 6F/67 30/20
KLH4V006;51C4;51C9;22/32
KLHCG000;5180;5185;6B/63 3B/2B 6F/67 30/20
KLQ4G007;51C4;51C9;22/32
KRHBG017;5126;512B;88
REGI1224;51DE;51E3;88 6B/63 6F/67 30/20
SELFG007;5140;5145;8F EC 0B/1B
TAH7V000;4BEE;4BD9;8F EC 21/31
TBH7V000;4BEE;4BD9;8F EC 21/31
Cumul 40 profils distincts, présence DA/DB 40/40, EC 8/40, 6B/63 11/40, 22/32 9/40, 21/31 3/40, 20/30 6/40, 6F/67 4/40. Pas de confirmation de comportement pour toutes les variantes.
## Précisions Mini
Information utilisateur : A-Series ordre 1-3-4-2, cylindre 1 côté pompe à eau, rotation antihoraire distributeur SPi (vue côté rotor). SPi injecteur unique. MPi sans distributeur, allumage 1/4 et 2/3, admission siamoise 1/2 et 3/4. Rover K semi-séquentiel groupes 1/4 et 2/3 ; DA/DB association aux cylindres à confirmer par faisceau et non ordre d'allumage seul. Notes GitHub 63fa5967ebdb5522137576ef3e433534b7226c50 et 0597ef62496a76b0719c42c99fb99e20eb13dce1.
