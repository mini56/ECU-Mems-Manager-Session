# MEMS 1.6 NA — lot 15 : qualification des onze sélections HSO2 restantes
28/11/2026. Suite du lot 14 (8abcf8fa4e0f41b0946e4595ded3e6333b48ca12). Lecture des onze ASM complets et des onze XF1 originaux via leurs blobs GitHub, comparaison avec EKDMF006/007/009. Les octets des tables sont issus du bilan ROM lot 13, pas d'une nouvelle lecture binaire dans ce lot. Aucun run ni accès ECU.
## Preuves individuelles (adresses hexadécimales)
| Firmware | Entrée routine | Table | Écriture HSO | Écriture date | OR/AND ign_flags | ign_coil_no | Huit appels directs |
|---|---|---|---|---|---|---|---|
| TAEDP000 | 3162 | 31F7 | 317D | 31A4 | 31B0/31B7 (97) | 82 | 2295 2320 2378 2774 2790 310B 3117 312C |
| TAEDP001 | 3167 | 31FC | 3182 | 31A9 | 31B5/31BC (95) | 82 | 2295 2320 2378 2779 2795 3110 311C 3131 |
| TAPDF008 | 3183 | 3218 | 319E | 31C5 | 31D1/31D8 (95) | 82 | 2295 2320 2378 2787 27A3 312C 3138 314D |
| TAPDF010 | 3361 | 33F6 | 337C | 33A3 | 33AF/33B6 (91) | 80 | 2381 240C 2464 2928 2944 330A 3316 332B |
| TAPDF011 | 3356 | 33EB | 3371 | 3398 | 33A4/33AB (93) | 80 | 238D 2418 2470 292B 2947 32FF 330B 3320 |
| TAPDF012 | 3356 | 33EB | 3371 | 3398 | 33A4/33AB (93) | 80 | 238D 2418 2470 292B 2947 32FF 330B 3320 |
| TAPDF013 | 3372 | 3407 | 338D | 33B4 | 33C0/33C7 (91) | 80 | 238D 2418 2470 2939 2955 331B 3327 333C |
| TAPDP000 | 3167 | 31FC | 3182 | 31A9 | 31B5/31BC (95) | 82 | 2295 2320 2378 2779 2795 3110 311C 3131 |
| TAPDP001 | 3167 | 31FC | 3182 | 31A9 | 31B5/31BC (95) | 82 | 2295 2320 2378 2779 2795 3110 311C 3131 |
| TAPDP002 | 3167 | 31FC | 3182 | 31A9 | 31B5/31BC (95) | 82 | 2295 2320 2378 2779 2795 3110 311C 3131 |
| TAPDP003 | 3356 | 33EB | 3371 | 3398 | 33A4/33AB (93) | 80 | 238D 2418 2470 292B 2947 32FF 330B 3320 |

Chaque routine compare l'index à 4, charge la table 01 02 02 01, programme HSO_COMMAND puis HSO_TIME, et modifie le registre identifié ign_flags par le XF1 correspondant. Dans chaque profil le troisième appel est suivi d'un accès 3BCh[ign_coil_no]. Les premiers appels passent les valeurs internes 1 et 2 et les autres 0, 4 et 3 ; ce ne sont pas des opcodes diagnostiques. Les adresses des variables output_int (5C) et output_time (80 pour TAEDP000/001, TAPDF008, TAPDP000/001/002 ; 7A pour les autres) concordent avec l'ordonnancement. Les onze XF1 portent les libellés d_tpg_force/onoff mais aucune des variables nommées m_tmpgauge_dr, m_tmp_gauge_dr, m_tempg_pwm_cnt ou tacho (recherche de ces libellés seulement). Le témoin EKDMF009 possède une routine d'allumage analogue en plus d'une chaîne de jauge distincte. Ces sélections indirectes HSO2 ne constituent donc pas une preuve de test d'instrument.
## Blobs ASM / XF1
- TAEDP000 : 2ce80d8ea2343c8853b92aeee2a399f2abf73fd6 / d97b2c597ef6e625f137acdd85e80801e024e7a8
- TAEDP001 : 562d347bef55273c54cbacec065b3a22e3b4c9a9 / 72053d7c70f600a1a1d8aba38c56b1f907d731e6
- TAPDF008 : b81c35f363c176fc89f881161a31c1ea3a90ce28 / 2f2d6a31d9b79195b2f845f526e92a9262012bb8
- TAPDF010 : 4ac9c144ffcf39cf202186a02c4c8ce373aa0269 / 8d1274528ed598551d13d38e8977b455ff1ce283
- TAPDF011 : 07c2b58b0c691ab6e2bd99cffe06fc3d7b7b2cb2 / 28026971586d408d37434b450c893025f5cfec8a
- TAPDF012 : 660bbaedbeccb0fb2d6391e50d5545ae7e39a375 / 4709be472383887918f2d269d6f4fd3273ba6f16
- TAPDF013 : 1004503f933d4a4f9c177b4d984c7ebd31a7ea1c / fd68c81495ae31992277226c07c00b7de767dfa7
- TAPDP000 : fb5f5bc7e55dff4ea5b5222c29e7790d301a526d / 0a47535046a68d34d6e0e0940cbd95e8a553c12e
- TAPDP001 : 09374329e625e133a777b4c8bb1a867fce6dba58 / dc687e3600da6840e9d4d4536bc04741697f8879
- TAPDP002 : 4b6f37f509057efdc0d466736384f3f0768bd385 / 8c8de93a74ce14a2122ffb344d5137eaeb9d817f
- TAPDP003 : 743b17a7d59847f6a848ac5889a31421591c1f08 / 1492bf55aa80dd9fa86f80cd35817bf8c9df9c78
## Bilan
11/11 nouvelles pistes de la famille HSO2 qualifiées comme allumage, 13/13 avec le lot 14. Aucun nouveau candidat de jauge ou compte-tours confirmé. Les 104 pistes V2 et quatre références sans piste initiale ne sont pas exclues globalement : des producteurs autres que cette routine, des écritures port, d'autres ordonnanceurs et le routage matériel restent à étudier. Ne jamais utiliser une commande d'allumage comme test d'instrument. Les deux candidats ASM seuls du lot 12 restent à valider. Aucun changement Manager/DLL.