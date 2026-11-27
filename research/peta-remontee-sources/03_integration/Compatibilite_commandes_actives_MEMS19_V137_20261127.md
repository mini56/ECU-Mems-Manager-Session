# V1.37 — compatibilité commandes actives, extension à 72 fichiers
27/11/2026. Analyse statique exclusivement. Aucun envoi ECU, aucune modification Manager/DLL.
## Exception TBP7F015
Ce fichier est rangé dans le répertoire MEMS 1.9, mais présente une architecture de dialogue différente. Ne pas inférer sa génération effective du répertoire. ASM 3C63–3C8F : tables 42FC–431C mode 14, 431C–43C4 normal, 43C4–43F8 états internes, 43F8–445C mode 1E. Bornes fin exclue. Dans table normale (42 entrées) absence DA/DB ; EF/F7→3EA4, E7→3EBC, F8→3EC2, 70→4102, EC→40CD. DA trouvé dans le mode 1E uniquement (entrée 4450→8A49 ; non test injecteur normal, à exclure). XF3 IDENT TBP7F015,90,00,15 ; base calibration 9500 ; inj_pw_rosco facteur 0,001333 ms, différent de nombreux 1.9. 0/10→3F28/3F23 : effacement/pose 008B/80, pose 008B/40. XF3 flagbyte9 d_tpg_force/d_tpg_onoff. Annulation 42DE–42FA efface bit6 ; recherche textuelle des références à 008B ne retrouve pas de consommateur direct de ces deux bits dans le code de stratégie. Ne pas déclarer une jauge physique fonctionnelle pour ce profil sur cette seule preuve ; pas d'essai. 65/6D→3F1A/3F15 efface/pose 008C/02 et pose 008C/01 ; XF3 d_thr_sw_force/onoff ; 5D18–5D38 impose état logique papillon et dérivés 008C/10, 008E/80 et D7/80 puis 42F7 annule forçage. C'est une simulation d'entrée, pas un test de l'état réel du contacteur ; ne pas la proposer comme test capteur. E7/F8 sélectionnent 0102/0002, appel commun 31F1, retour 0/1/2 par 3ECE–3EDE ; matériel à vérifier.
## Compte-tours : extension des profils vérifiés
6 nouveaux profils de simulation : REGI1371 588C/589B (RAM 0156), TAHHV006 587A/5889 (0156), TFH7G025 57C6/57D5 (0158), TFHSG011 57C6/57D5 (0158), TFHSV000 5720/572F (0156), TFHSV006 5951/5960 (0156). HEX signature b3 01 0e 03 f8 91 20 f8 a3 41 1a 00 f6 20 0d, off charge +18. XF3/XF1 offsets 24/26 décimaux, facteur 1 rpm. Bases 9400 pour REGI1371, TAHHV006, TFHSV006 ; 9500 pour TFH7G025, TFHSG011, TFHSV000 (XF1). Six calibrations 0/3000 tr/min confirmées ; pas retour de position ni essai. 3 nouveaux profils uniquement forçage logique, REGI0872 54DF/54E9, REGI1359 569D/56A7, SELFG008 57E3/57ED : signature b3 01 0e 03 f8 91 20 f8 20 08 b3 01 0e 03 f8 71 df f8 ; pas lecture +18/+1A et écriture directe de bit de port 0022/20 vers 1FFE. Ne pas proposer 3000 tr/min sur ces profils. 
## Matrice du lot
Profil;table normale;DA;DB;E7;70;6B;63;EC
REGI0872;565E;4FF1;4FF6;5068;536A;54DF;54E9;-
REGI1297;57CE;51C4;51C9;523B;553D;-;-;-
REGI1342;56F4;511E;5123;5195;5497;-;-;-
REGI1359;584A;51D6;51DB;524D;5528;569D;56A7;-
REGI1371;59C0;5331;5336;53A8;56D5;588C;589B;56AA
REGI1417;5824;521A;521F;5291;5593;-;-;-
SELFG004;557C;4F46;4F4B;4FBD;52C5;-;-;-
SELFG008;5914;525B;5260;52D2;5605;57E3;57ED;55DA
TAH8V000;55D8;4FA0;4FA5;5017;534A;-;-;531F
TAHHV006;59E0;531F;5324;5396;56C3;587A;5889;5698
TBP7F015;431C;-;-;3EBC;4102;-;-;40CD
TFH7G025;5904;5244;5249;52BB;55E8;57C6;57D5;55BD
TFH7V000;525E;4C24;4C0F;4C72;4F50;-;-;4F25
TFHSG011;5904;5244;5249;52BB;55E8;57C6;57D5;55BD
TFHSV000;585E;5198;519D;520F;5542;5720;572F;5517
TFHSV006;5AA0;53CF;53D4;5446;5773;5951;5960;5748
Cumul 72 fichiers distincts, DA/DB 71/72, E7/70 16/16 dans ce lot ; EC 10/16, 6B/63 9/16. Pour 15 fichiers : repérage signature D0/F0/F9 et borne heuristique deuxième 0D/0D ; TBP7F015 : bornes exactes ASM. Pas de nouveau code actif indépendant identifié dans ce lot. Les commandes 02/07/0A/04/0C/06/05 repérées après 43C4 sont états internes, pas commandes nouvelles. Exclure les modes de programmation. Résultat pratique : élargir profils compte-tours vérifiés ; liste blanche DA/DB, ne pas activer sur TBP7F015. Sources HEX/ASM/XF1/XF3 du dépôt. Comparaison antérieure SESSION 215 seulement.