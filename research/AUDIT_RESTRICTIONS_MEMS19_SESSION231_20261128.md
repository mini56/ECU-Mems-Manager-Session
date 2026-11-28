# Audit des limitations MEMS 1.9 — SESSION 231
Source programme : 4b465406cca3a3a619b0396bc8ca85c3af4c9dc0, branche fix-documentary-additive-session-161-20261118.
Inventaire de comparaison : commit recherche 5360f133fdec94a7bdadaf13cf83ee7a1da6b75a, 215 fichiers ROM/HEX/BIN et 215 noms uniques sous Rover MEMS 1.9. Audit ciblé des mécanismes de qualification déjà identifiés, non désassemblage DLL ni validation exhaustive des fonctions générales.
Les nombres indiquent les références de l'archive éligibles dans le code, pas les essais réels et pas la présence physique universelle des équipements. Les restrictions actives ne prouvent pas que les profils exclus possèdent la commande.

| Fonction | Éligibles/215 | Source et condition |
|---|---:|---|
| Observation lambda | 1 | mems217_passive.h:23,31-52 KBHBG009 |
| Observation température moteur | 1 | idem |
| Test de réchauffement sonde moteur | 1 | mems218_tests.h:106-121 réutilise passiveProfile |
| Observation température air | 1 | mems222_sensor_observation.h:15-29 réutilise profile19 |
| Observation TPS | 1 | idem |
| Observation contact de ralenti | 1 | idem |
| Observation MAP | 1 | idem |
| Voyant validité lambda Réglages | 1 | mainwindow.cpp:1207-1208 |
| Correction injection en µs | 2 | mems217_passive.h:6-8, mems219_units.h:17-18, mainwindow.cpp:1515,1531,1573 : KBHBG009, KBHBV001 |
| Correction lambda en % | 2 | mems219_units.h:13-16, mainwindow.cpp:1513,1575 |
| Indicateur IAC minimum Réglages | 2 | mainwindow.cpp:1205-1206 |
| Position ralenti chaud nouvelle interprétation | 2 | mainwindow.cpp:1517,1651 et summarytab.cpp:392 ; version étendue Mems220 limitée à 1 |
| Correction ralenti signée | 2 au niveau du prédicat injectionProfile (conditions supplémentaires selon données) | mainwindow.cpp:1519 |
| Erreur ralenti décodage étendu | 1 | mems220_measurements.h:7-9,19-44 |
| Tension apprentissage ralenti | 1 | idem |
| Référence papillon apprise | 1 | idem |
| Compteur apprentissage papillon | 1 | idem |
| État D/N | 1 | idem |
| Demande climatisation | 1 | idem |
| État coupure allumage 80 | 1 | idem |
| État coupure allumage 7D | 1 | idem |
| Demande ventilateurs | 1 | idem |
| Apparition horodatée du défaut sonde moteur | 0 | mems16na_session_evidence.h:16-18 exige profile_ NA ; diagnosticpanel.cpp:637-640 dans rebuildNa |
| Test injection rapide | 0 | injectionramtest.cpp:297-298 exige Rosco13_16 et profil NA |
| Calcul injection mode 4 standard | 162 | 164 noms uniques dans les cinq tables mappedinjection_profiles_mems19*.inc, 162 correspondent à l'archive, deux hors archive |
| Compte-tours | 41 | mems218_tests.h:30-75 et memsinterface.cpp:523-527 |
| Jauge température | 0 | mems218_tests.h:30-32, memsinterface.cpp:523-527 |
| DTC2 interprétation spécifique | 1 | memsextendeddtc.h:32,56 KBHBG009, pas absence de toute lecture brute sur autres |
| DTC3 synchronisation | 5 | memsextendeddtc.h:23,57 |
| DTC4 indicateur DA | 4 | memsextendeddtc.h:25,60 |
| DTC5 ventilateurs | 9 | memsextendeddtc.h:20,62 |
| DTC5 descente HDC | 5 | memsextendeddtc.h:21,63 |
| DTC5 régulateur vitesse | 3 | memsextendeddtc.h:22,64 |
| DTC5 bits 02/04 spécifiques | 2 | memsextendeddtc.h:35,61-62 KBHBG009 et REGI1224 |
| DTC0 interprétation circuit pression | 11 | memsextendeddtc.h:27-30 |
| DTC4 constant 00 | 1 | memsextendeddtc.h:49 |
| DTC3 constant FF | 2 | memsextendeddtc.h:50 TBP7F015, SELFG007 |
| DTC4/5 constants FF | 1 | memsextendeddtc.h:50 TBP7F015 |

Mems220 : exact nom KBHBG009 et identité 900009. Le test tools/mems220_selftest.cpp:16-35 vérifie ce profil et le rejet d'une identité discordante ; ne démontre pas la couverture 215. Application dans diagnosticpanel.cpp:366-381,495,816 ; mainwindow.cpp:1671-1713 ; summarytab.cpp:313,535,685-718. Les autres références passent dans des traitements historiques ou affichent des valeurs indisponibles. Ne pas confondre absence de décodage spécifique avec absence totale de mesure.

Injection standard : liste des 53 références de l'archive absentes des cinq tables MEMS 1.9 : FTST0152, REGI0872, REGI1224, REGI1297, REGI1330, REGI1341, REGI1342, REGI1343, REGI1344, REGI1345, REGI1346, REGI1348, REGI1349, REGI1350, REGI1351, REGI1352, REGI1353, REGI1354, REGI1355, REGI1356, REGI1359, REGI1360, REGI1361, REGI1362, REGI1363, REGI1364, REGI1365, REGI1366, REGI1367, REGI1368, REGI1369, REGI1371, REGI1374, REGI1376, REGI1377, REGI1378, REGI1379, REGI1380, REGI1381, REGI1382, REGI1383, REGI1384, REGI1386, REGI1387, REGI1406, REGI1409, REGI1412, REGI1415, REGI1416, REGI1417, SELFG004, SELFG007, SELFG008. Deux noms hors archive KLH3V004, TFH7G031, leur provenance reste à vérifier. Absence des tables 1.9 constatée, pas preuve d'un support technique de Mode4 sur tous les exclus.
Aucune modification du programme, de la DLL ou lancement de compilation.