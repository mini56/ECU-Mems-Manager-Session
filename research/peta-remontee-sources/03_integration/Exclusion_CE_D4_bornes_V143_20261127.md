# V1.43 — CE exclue sur décision utilisateur, D4 lecture non destructive et bornes exactes
27/11/2026. CE : rejet définitif pour intégration et essais ECU, décision utilisateur. Conserver preuves historiques uniquement. E9 non retenue pour tests courants. Aucun changement Manager/DLL, aucun envoi ECU.
## D4 : extension du chapitre 157
8 profils supplémentaires distincts des quatre V100. Les handlers vérifient le bit de validité, sinon branche retour 0 ; retour 2 si bit d'échec posé, sinon 1. Lecture d'indicateurs existants, aucun lancement de test ni écriture persistante par le handler.
Profil;D4;validité;échec;ASM SHA
REGI1369;58C7;0083/01;0082/80;584cb4ae934c354d7c88652d56dc9f8f778876b5
KJH6V004;55CC;0082/80;0082/40;44b6c8ce703808ffd4ae29bdff76bb22218bf247
KLH4G018;572F;0083/04;0083/02;33ba91239ca3b1c9ee9818a18c89a56579e05b00
KLH4V010;56D9;0082/80;0082/40;73c01002a9ae9d024f1406aa7a628109ddffbc15
KLH3G020;56F5;0083/04;0083/02;4f03c1fddfc9fcb0b7e5e016c82607caebb81fbe
REGI1417;572F;0083/04;0083/02;e009355a0f516d926880608b4862f9054f367177
FTST0152;577D;0083/08;0083/04;97754771fdb3c788b32dd00fe9169fec8a1614ae
TFH7G029;5A24;0083/08;0083/04;b148bac468cf7aef334a24097fe132dc43057628
REGI1369 : 87A4–87BF CRC par table 403A ; 87CD compare ROM_9FFE ; 87D4/87D9 efface/pose 0082/80 ; 87DC pose 0083/01 ; 58C7–58D6 lit ces flags ; 4720 efface F8 et saute au transmetteur. XF3 7ca22a37c67a7ce47f010e953b179c5609dcd476 confirme flagbyte27/28 dg_crc_test_status et dg_crc_result_state. Les 7 autres handlers vérifiés, producteurs non réaudités dans ce lot. 12 profils cumulés. D4 non déclaré dans rosco.h SESSION 215 ; état actuel non audité. Affichage 0 indisponible, 1 intégrité ROM réussie, 2 défaut intégrité ROM. Pas de conclusion sur EEPROM ou ECU complet, ni mesure terrain.
## Correction des limites
Recherche binaire des deux LD immédiats consécutifs A1 portant début/fin, puis extraction des entrées 4 octets ; signature D0/F0/F9 seulement repère. 16 profils avec une paire candidate unique. Sur REGI1369 le code ASM confirme 50B5 ; sur les autres paires binaires, vérification ASM intégrale non faite.
Profil;PC;début;fin exclue;D4
AANMP002;3A95;40B0;4154;-
TAP7F017;3CA4;4322;43CA;-
KBP8F066;3956;3EC0;3F58;-
REGI1369;50B5;59B8;5A94;58C7
KJH6V004;4E56;56B0;5784;55CC
KFHBV002;4C27;5466;5536;-
KLH4G018;4F86;581C;58F4;572F
TFH7G029;51D3;5B30;5C14;5A24
KLHBG000;4C72;54C2;558E;-
KLH4V010;4F48;57C6;589E;56D9
KLH3G020;4F42;584E;5936;56F5
REGI1417;4F86;581C;58F4;572F
SELFG004;4CB2;5574;5654;-
TBP7F015;3C81;431C;43C4;-
FTST0152;4FD4;586A;5942;577D
TAH8V000;4D24;55D0;56A4;-
0E après fin table normale KBP8F066 est un état interne, pas commande active ; ne pas extrapoler les ventilateurs 1.9 au 1.6. D4 absent des trois tables NA et du TBP7F015 (architecture particulière). Sur 1.3 non démontré, aucune extrapolation. Aucune nouvelle commande indépendante découverte ; compteurs de couverture V142 inchangés car profils réexaminés. 
