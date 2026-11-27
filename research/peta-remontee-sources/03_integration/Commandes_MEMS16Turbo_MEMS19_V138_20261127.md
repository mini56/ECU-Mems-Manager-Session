# V1.38 — MEMS 1.6 Turbo : répartiteur et commande 87 ; MEMS 1.9 : 8 profils
27/11/2026. Analyse statique, sans envoi ECU et sans modification Manager/DLL.

## MEMS 1.6 Turbo
ROM base 2000h, 49152 octets chacune. Les 4 ROM ne contiennent pas la signature contiguë D0 D0/F0 F0/F9 F9 du 1.9. Le mode 50 est explicitement comparé dans TCF7X005/TCF8X005 à 7726 (99 50 46 DF 01 F0), puis parcourt des pointeurs de routines 775C–7784 (20 mots). TFF8X000 à 7A74, TFFSX000 à 7A78, pointeurs 8946–8970 (21 mots). Fin exclue. Ce sont des pointeurs de code, non des quadruplets d'opcodes.
Nouvelle piste 87 : TCF entrée 777C→572E, comparaison 5732, appels 5741→3112 et 5750→3160, après sélection 04. Les quatre octets de résultat sont stockés à 0150–0153 (C7 01 50 01 1C ... C7 01 53 01 1F), puis transmission via 5778→3BA3 avec pointeurs 0150/0153 et longueur 05 ; état 59. TFF entrée 8966→572A, comparaison 572E, appels 573D→3114 et 574C→3162, transmission 5774→3BA5. Même structure 4/4. Sous-routines 3112/3160 utilisent sélection 04, registre 94, table adressée par 0156, seconde routine accepte valeur 87 et appelle indirectement. Nature physique du périphérique et définition des quatre octets non prouvées. Ne pas assimiler à un test actionneur ni à une lecture ordinaire ; aucune intégration ni envoi recommandé.
E9 : dans cette table mode 50, TCF 777E→7468 (comparaison 746B, pose BF/08), TFF 8968→7792 (comparaison 7795, pose BF/08, et EF/01 en TFFSX000). L'effacement des défauts n'est pas démontré par cette seule demande. DA : comparaison TCF 70A9, TFF 73E9 ; pas de preuve d'activation d'injecteur comme dans le 1.9. DC : comparaison TCF 7093, TFF 73D3 ; sélection d'état 31. Ne pas transposer les commandes entre modes/générations.
Sources Git blobs: TCF7X005 e530ebd65a4b4204d4f6710a48745132c58b9321; TCF8X005 5c425b2b67ebd7d9b303e08897ddb7eab91ac876; TFF8X000 d57b2a9ccfc704a86ff0d01127bc1da9b41450d5; TFFSX000 c3d4c298bb974060b96c21632a01d7da32b81e4d.

## MEMS 1.9
8 profils supplémentaires (80 cumulés sur 215 selon série V137). Signature unique D0/F0/F9, base 2000h, limite heuristique deuxième 0D/0D (les entrées suivantes de machine d'état ne doivent pas être comptées comme commandes).
Profil;table;DA;DB;E7;70;6B;63;EC;88
REGI1368;57F6;5180;5185;51F7;54D5;564A;5654;-;-
REGI1369;59C0;5331;5336;53A8;56D5;588C;589B;56AA;-
REGI1376;5856;51D6;51DB;524D;554F;56C4;56CE;-;-
REGI1377;56B8;50D2;50D7;5149;5457;-;-;-;5372
REGI1379;56B8;50D2;50D7;5149;5457;-;-;-;5372
REGI1380;570C;5126;512B;519D;54AB;-;-;-;53C6
REGI1381;56B8;50D2;50D7;5149;5457;-;-;-;5372
REGI1382;570C;5126;512B;519D;54AB;-;-;-;53C6

5 profils CVT supplémentaires: REGI1377/1379/1381 : handler 5372, A1 18 3C E4 A1 20 3C E2 B1 09 67 F0, descripteurs 3C18–3C20 ; REGI1380/1382 : handler 53C6, A1 54 3C E4 A1 5C 3C E2 B1 09 65 F0, descripteurs 3C54–3C5C. Tous contiennent 30 00 02 00 20 00 30 00 ; XF3 de chacun : i_cvt_load_adpt,0030,B,-60,1,steps. 11 profils exacts cumulés, commande de lecture déjà identifiée V135, pas nouvelle commande active. Aucune validation physique ni audit de la version actuelle de Manager.
HEX blobs : REGI1368 c9c1b81afed5e9f75da516f00a824d4bb4f27861; REGI1369 b19f36d2dd3ace19237e98e7d769652025bc2826; REGI1376 f10df30c576abd9f29702ceb370c2acacbe60f47; REGI1377 1c5283544540e24a05e272af9d34caeaae6beffd; REGI1379 eb32ffe4e7df0e22088600d9d727175071f0bc79; REGI1380 cdee5c245a6d67e6701b0a9e56bec0ccf6af0d64; REGI1381 2a0543253e0bdd95b46a3444815a8ce10b52662c; REGI1382 3ce214bbd8603eeb0e799c64a91f77b0a41c1ee9.
XF3 blobs : REGI1377 673cdb12efc332fa8314dee88f9741c858a20e79; REGI1379 9a87e9e613fa09ed1b142adcb0ff7414c653d0cb; REGI1380 cf2da456790068f8f7585de40d33334b8449af9b; REGI1381 fffd0c5e52f3ccf2668e878943b2584f02c14a42; REGI1382 a5bad4aab6c9a2ff94c76ee91f6b2d9ff8125435.
Priorité inchangée : découverte de commandes actives absentes de Manager, 1.6 NA/Turbo et 1.9 uniquement. Aucun nouveau test actif indépendant validé dans ce lot.
