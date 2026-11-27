# V1.36 — priorité commandes intégrables
27/11/2026. Analyse statique, aucun envoi ECU, aucun changement Manager/DLL. Comparaison ciblée rosco.h SESSION 215 (475243db18c3099595e1a5928b6916185380a808), pas audit de SESSION 220.
## E7/F8 — sélection de voie allumage
TFH7G029 E7→54DE charge F8=0102, F8→54E4 charge F8=0002 ; 54E8 charge durée depuis RAM 0146 (XF3 ign_max_charg_b, 0,001 ms), appelle 3A50. 3A88 refuse régime non nul ; 3A93 contrôle durée maximale RAM 0144 ; 3A9F–3ADD programme la voie et attend état matériel. 3AF9–3B51 sélectionne canal à partir de F8 haut (0/1), 3B95 table 01 02 02 01, 3B99 masques 02 04 04 02 ; 3B62–3B82 vérifie WSR1 et bits matériels. 54F0–5500 renvoie 0 si erreur interne, 1 si succès interne sans F6+1=1, 2 si succès interne avec F6+1=1. Ne pas attribuer OK/HS définitif à ces codes avant validation électrique. KBHBG009 5141/5147 puis 514B→372C, KLH4G018 5291/5297 puis 529B→3824 : même sélection 0102/0002. Deux voies logiques distinctes ; présence physique de deux bobines/sorties selon profil et câblage non démontrée. F8 déjà déclaré FireCoil, E7 absent rosco.h SESSION 215. Candidat : test seconde voie si matériel confirmé, moteur arrêté, résultat électrique interprété avec prudence. E7 n'est pas une nouvelle découverte universelle (V118 autres générations), ici qualification de son usage 1.9.
## 70 — référencement
KBHBG009 5443 pose 007E/40 (XF3 d_scan_force) et état 0D, 544A attend effacement puis clôt état sans code résultat visible. 65A7–65BE vérifie contact 007B/10 et m_eng_stopped D4/40, charge m_thr_shut_bits RAM 002A depuis constante m_thr_shut_init +2E, incrémente m_thr_jack_cnt 0032 avec saturation, positionne m_thr_fst_init D6/08, efface demande. KLH3G018 554F pose 007F/02, état 0D. AANMP002 3EF3 pose AD/80, état 0D ; 53E7–5411 et 545B→4970 lancent procédure selon contexte ; 3EFA–3F0E clôt état quand demande effacée et renvoie 0 seulement si RAM 015F=0. Ne pas assimiler effacement de demande à fin mécanique. V135/V130 : 114/123 NA. 70 présent dans les 16 profils supplémentaires ci-dessous. Pas déclaré dans rosco.h SESSION 215. Candidat fonction de référence/réinitialisation liée au papillon et au ralenti, pas balayage manuel arbitraire. Vérifier conditions, timeout, récupération, fin mécanique et impact apprentissage avant usage.
## 71 — configuration ventilateurs avec effet de bord
KBHBG009 5BC2 efface bit0 de 003E (XF3 fan_twin_speed), 5BC5 retourne constante fan_1_4_sng_2_4_twn +105. 608C–60AC compare à 1 (simple), 2 (double), autres selon condition matérielle et ancien état ; le bit effacé participe à la stratégie. KLH3G018 5BCA/5BCD même mécanisme, offset +10D. 8 valeurs HEX : KBHBG009 +105=0, KBHBG010 +10A=0, KLH3G018 +10D=0, KLH4G018 +117=0, TFH7G029 +13C=1, TAHHG019 +10F=0, KFACG002 +103=0, KJH6G012 +115=0 (base 9400). 0 n'est pas une absence de ventilateur. Ne pas ajouter de lecture périodique ou de test 71 sans maîtriser cet effet de bord.
## 88 — trois profils supplémentaires confirmés
KJH6G008 handler 51AA descripteurs 3C54–3C5C ; KJH6G009 5372 3C18–3C20 ; KJH6V000 514B 3C18–3C20. Même octets 30 00 02 00 20 00 30 00 et symbole i_cvt_load_adpt RAM 0030 offset −60 facteur 1 pas dans XF1/XF3. Six profils prouvés statiquement au total (V135 trois autres). 
## 16 profils supplémentaires (56 cumulés)
Profil;table;DA;DB;70;71;88
KBHHV002;546E;4EA3;4EA8;5222;5AC2;-
KFACG002;5692;50CA;50CF;541C;5CC2;-
KFACV000;5426;4EA3;4EA8;51F5;5BC2;-
KFACV001;5692;50CA;50CF;541C;5CC2;-
KFHBG007;5504;4F39;4F3E;52B8;5BC2;-
KFHBG009;56F4;511E;5123;5497;5BC2;-
KFHBV001;546E;4EA3;4EA8;5222;5BC2;-
KFHBV003;56A0;50CA;50CF;5443;5BC2;-
KFHBV004;56A0;50CA;50CF;5443;5CC2;-
KIH6G010;54C6;4F02;4F07;527B;5BC2;-
KIH6G012;56F4;511E;5123;5497;5BC2;-
KIHBG000;56A0;50CA;50CF;5443;5CC2;-
KJH6G008;54DE;4F0A;4F0F;528F;5BC7;51AA
KJH6G009;56B8;50D2;50D7;5457;5BC7;5372
KJH6V000;5480;4EAB;4EB0;5230;5BC7;514B
KLH3G018;5856;51D6;51DB;554F;5BCA;-
Extraction HEX signature D0/F0/F9, borne heuristique deuxième 0D/0D, contrôle ASM des handlers cités. 56/56 DA/DB présents (présence ≠ fonctionnement physique). Pas de nouvelle entrée hors inventaire connu dans ces 16. Les quatre ROM Turbo 1.6 n'ont pas cette signature ; aucune conclusion de compatibilité pour elles. Priorités : E7 et 70 à qualifier, 71 exclue en lecture périodique, 88 étendre aux 6 profils exacts. 
