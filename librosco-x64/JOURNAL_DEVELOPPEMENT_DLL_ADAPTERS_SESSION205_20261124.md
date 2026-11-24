# Journal DLL — détection native des adaptateurs — 24 novembre 2026

Branche unique dll-manager. Manager 205 figé b5cd9b61bbaed6251f17559885481b1bd03f4380. Lot RC5 précédent validé (run 45370170574). Aucun changement du programme, du Setup officiel ni du site MAJ.

Trois exports ajoutés, les 130 précédents sont conservés :
- 131 `mems_manager_list_serial_adapters`
- 132 `mems_manager_get_adapter_details`
- 133 `mems_manager_detect_adapter_family`

Source fonctionnelle : serialadapterdetector.h et serialdevenumerator.cpp de Manager 205. Implémentation Win32 native en lecture seule : classe Ports SetupAPI, attributs du périphérique, VID/PID, aliases COM Windows (QueryDosDevice), déduplication, ordre numérique, port préféré en tête même si configuré mais absent (indicateur explicite). FTDI, Prolific, WCH CH340/CH341, Silicon Labs CP210x, USB générique et port série. VID/PID 1A86:7523 = simple préférence K-Line, jamais preuve du câble ni verrou. Les autres protocoles restent possibles. Aucun port COM ouvert, aucune modification USB ou registre.

Tests : normalisation, classification, VID/PID FTDI, déduplication, ordre et port préféré, énumération native en lecture seule sur runner sans câble imposé. CI attendue 12 CTest, 133 exports exacts, 15 imports Manager, 16 pages et Setup de test indépendant. Résultat CI non encore constaté. Aucune détection physique de câble/ECU revendiquée.

Découpage : ce lot couvre T01–T03 de l'inventaire. T04 et suivants (auto-connexion, propriété session, annulation, trace et erreurs) restent à faire. Le prochain lot doit respecter l'ordre de tentatives de memsinterface_dispatch.cpp et ne jamais inventer un sous-type ECU à partir du câble.
