# Journal de développement de la DLL — lot Lucas 5AS, référence SESSION 205

**23 novembre 2026 — branche unique `dll-manager` — sans aucune modification du programme Manager.**

## 1. Contrat et objectif

- Le programme **Session 205 publié**, commit immuable [`b5cd9b61bbaed6251f17559885481b1bd03f4380`](https://github.com/mini56/ECU-Mems-Manager-Session/tree/b5cd9b61bbaed6251f17559885481b1bd03f4380), est désormais **l'unique cible actuelle de compatibilité de la DLL**.
- L'en-tête public `rosco.h` du 205 a **exactement le même SHA Git blob `dd8d88048ce4e879ef728b6bcd72837730bec654`** que celui de Session 200. Ce constat ne remplace pas les tests sur le véritable binaire 205.
- **Les 91 exports originaux gardent leur ordre, leur nom, leur ordinal et leurs signatures.** Le nouveau lot ajoute neuf fonctions à partir de l'ordinal 92 ; 100 exports sont attendus à sa validation.
- Les sources principales de Manager, ses workflows, son installateur publié, son site MAJ et les applications installées **ne sont pas modifiés**. Les essais compilent ou installent uniquement une copie temporaire sur un runner Windows éphémère.

## 2. Fonctions réellement développées dans cette livraison

| Ordinal | Fonction exportée | Rôle |
|---:|---|---|
| 92 | `mems_manager_lucas5as_slow_bit_high` | Lire un bit de l'adresse lente 5AS 0x1C, 10 cellules de 200 ms |
| 93 | `mems_manager_lucas5as_has_normal_wake` | Repérer la signature de réveil normal 55 83 76 |
| 94 | `mems_manager_lucas5as_decode_status` | Lire les champs documentés du statut normal 0x83 (2–127 octets, longueur totale) |
| 95 | `mems_manager_lucas5as_extract_status` | Extraire une trame complète après écho optionnel 0x00/0x83, sans rechercher des fausses longueurs dans le contenu |
| 96 | `mems_manager_lucas5as_output_test_commands` | Obtenir les octets START/STOP des trois sorties identifiées, **sans envoi ECU** |
| 97 | `mems_manager_lucas5as_open_normal` | Ouvrir une liaison Win32 9600 8N1, réveil à 5 bauds, vérifier la vraie réponse et lire un premier statut |
| 98 | `mems_manager_lucas5as_initial_status` | Récupérer la trame brute reçue lors de la connexion |
| 99 | `mems_manager_lucas5as_read_status` | Effectuer une nouvelle lecture 0x83 sur la session ouverte |
| 100 | `mems_manager_lucas5as_close` | Fermer la session isolée et libérer le port et la mémoire |

**Implémentation :** `lucas5as.h`, `lucas5as_codec.c`, `lucas5as_session.c`. Ce sont des modules C **sans Qt et sans interface**, ajoutés seulement à `librosco-x64/CMakeLists.txt` et `mems_manager_x64.def`.

**Source de comparaison :** `lucas5asprotocol.h`, `lucas5asreader.h`, `lucas5asprotocol_test.cpp` du **véritable Session 205**, pas des fichiers plus anciens de la branche DLL.

## 3. Limites techniques explicites

- **Port Win32 5AS séparé**, jamais ouvert par la fonction historique `mems_connect`. On ne change pas le parcours « Connecter » de Manager.
- Connexion reconnue uniquement si **réveil 55 83 76 suivi d'une trame complète de statut**, pas sur la simple présence USB/COM. Une adaptation matérielle 5AS correcte reste indispensable.
- Les données de statut sont conservées **brutes**. Le paquet 5AS n'a pas de checksum général démontré ; la DLL n'en invente pas. `possibleBoot` reste hypothétique ; aucun compteur de télécommandes n'est interprété comme compteur à vie.
- **Aucune procédure de télécommande/PLIP, EEPROM, jumelage, programmation, actionneurs ou effacement SRS n'a été ajoutée dans cette première étape de transport 5AS.** Le catalogue d'octets de sortie est de la **lecture de constantes**, non une commande exécutée.
- Ce pilote Lucas 5AS **n'a pas encore été validé avec un boîtier physique**. Les erreurs de timing électrique selon câble/Windows devront être confrontées à des captures réelles.
- Aucune fonction existante Manager n'est détournée vers le nouveau module pendant son développement.

## 4. Validation, journal des runs

- **Premier run [45239552407](https://github.com/mini56/ECU-Mems-Manager-Session/actions/runs/45239552407)** : **6 tests DLL sur 6 verts** ; la compilation du vrai Manager 205 en copie isolée a ensuite été bloquée par un **conflit de nom de cible CMake** entre les deux projets (`lucas5as_protocol_selftest`). Aucun binaire du programme officiel n'a été modifié.
- **Correction ciblée** dans le seul CMake de la DLL : cibles renommées `dll_lucas5as_protocol_selftest` et `dll_lucas5as_session_selftest`, sans modifier le code ECU ni les sources du Manager.
- **Second run de validation complète : [45240083756](https://github.com/mini56/ECU-Mems-Manager-Session/actions/runs/45240083756) — VERT**, figé sur le commit `85d483ebd9bd7d5fb58b966620a03d16e7f715aa`. Journaux récupérés : **6/6 CTest réussis** ; `SESSION205_BINARY_COMPAT_PASS` (**AMD64, 100 noms/ordinaux exacts, 15 imports Manager 205 résolus**) ; `SESSION205_LINK_AND_PE_IMPORTS_PASS`; démarrage du vrai Session 205 et `SESSION205_UI_SMOKE_WITH_NEW_DLL_PASS` (16 pages, `Checks PASS`, sortie Qt 0) ; téléchargement **du vrai Setup 205 publié et SHA256 vérifié** ; création d'un Setup Inno indépendant **avec la nouvelle DLL** ; installation effective, intégrité SHA256 de l'EXE original et de la DLL remplacée, `SESSION205_INSTALLED_EXE_AND_NEW_DLL_SHA256_ABI_PASS` ; lancement et test natif des **16 pages depuis le Setup de test installé**, `SESSION205_ISOLATED_SETUP_INSTALLED_16PAGE_UI_PASS` ; désinstallation `SESSION205_ISOLATED_SETUP_UNINSTALL_PASS` ; enfin `SESSION205_NEW_DLL_COMPLETE_SETUP_TEST_PASS`. **Aucun ECU réel, aucune écriture matérielle, aucune Release/MAJ créée.**
- Le workflow dédié `.github/workflows/rosco-45-exports-x64-audit.yml` teste une copie de Session 205, puis éventuellement un **Setup de test distinct et non publié**. Aucune Release/MAJ n'est déclenchée.

## 5. Reprise après validation

1. Contrôler physiquement **100/100 exports et ordinaux**, et tous les imports réels du binaire 205.
2. Vérifier les six CTests Windows : quatre historiques et deux nouveaux (codec 5AS, transport avec Win32/horloge simulés).
3. Compiler/démarrer une copie de Session 205, exécuter le smoke UI natif, puis fabriquer/installer/désinstaller un **Setup indépendant** sur le runner.
4. En cas de réussite logicielle, poursuivre les autres fonctions ECU de l'inventaire en gardant les exports 1–100 intacts et sans modifications Manager.
5. **Ne pas annoncer « 100 % des fonctions ECU confirmées »** sur la seule base d'un test logiciel : la validation ECU matériel et les fonctions encore absentes restent à faire.

[Inventaire des fonctions restantes](INVENTAIRE_EXHAUSTIF_ECU_DLL_SESSION200_20261123.md) · [Chiffrage des 84 propositions](CHIFFRAGE_84_FONCTIONS_DLL_SESSION200_20261123.md) · [Contrat initial 91 exports](MATRICE_91_EXPORTS_ET_CONTRAT_SESSION200_20261123.csv).
