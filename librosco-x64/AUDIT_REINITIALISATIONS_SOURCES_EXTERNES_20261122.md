# Commandes de réinitialisation MEMS — audit croisé des programmes

**Date :** 22 novembre 2026  
**Périmètre :** `mems_manager_x64.dll` sur la branche `dll-manager`. **Aucun fichier de MEMS Manager Session n'est à modifier dans ce chantier.**  
**Nature des preuves :** codes source publiés, code source Session 196, ancienne DLL de Manager auditée en binaire, DLL LeopoldG auditée en binaire et documentation des auteurs. **Aucune exécution sur calculateur réel n'a été effectuée lors de cet audit.**

## 1. Tableau de correspondance réel

| Source | Commande / appel | Désignation dans cette source | Ce qui est réellement établi | À ne pas déduire |
|---|---|---|---|---|
| Ancienne DLL utilisée par Manager (22 exports, PE32, SHA256 `a5ad466f8f1a198a6f8259a5ea5ab59775815c8cbc9637aa584db9e9c811d38f`) | `mems_reset_ECU()` -> **0xFA** | Reset ECU | Octet de commande vérifié par l'audit du binaire original ; même octet dans Session 196 | Cela ne démontre pas un redémarrage électrique ou logiciel de l'ECU |
| Ancienne DLL Manager-22 | `mems_reset_adjustments()` -> **0x0F** | Reset adjustments | Octet vérifié par l'audit du binaire original ; même octet dans Session 196 | Cela ne démontre pas un effacement des corrections apprises |
| DLL / logiciel **LeopoldG** (45 exports) | `mems_reset_ADJ()` -> **0xFA** | Reset adjustments | Constante dans `rosco.h`, bouton dans `memsinterface.cpp` et octet vérifié sur sa DLL PE32 | Ce n'est pas la fonction `mems_reset_ECU` de la DLL LeopoldG |
| DLL / logiciel **LeopoldG** | `mems_reset_ECU()` -> **0xAF** | Reset ECU | Constante, bouton et commande du binaire d'origine concordants | Son effet réel ne peut être transposé sans essai à toutes les familles/modes MEMS |
| DLL / logiciel **LeopoldG** | `mems_reset_EMI()` -> **0x0F** | Reset EMI | Constante, bouton et commande du binaire d'origine concordants | Ce n'est pas la preuve d'un « Reset adjustments » |
| DLL / logiciel **LeopoldG** | `mems_Save()` -> **0x81** | Save | Constante et octet d'émission documentés ; réponse attendue | Réception d'un octet != preuve de sauvegarde en mémoire non volatile |
| **RoverMEMS**, table des commandes 1.3/1.6/1.9 | **0xFA** | « Clear all adaptations » | Sens explicitement documenté : **effacement des adaptations** | Ne signifie pas « reboot ECU » |
| **RoverMEMS**, table des actionneurs 1.3/1.6/1.9 | **0x0F** | « Variable valve timing » dans la série des commandes d'arrêt/désactivation | Interprétation **contextuelle VVT OFF** de 0x0F pour certaines familles | Ne signifie pas « Reset adjustments » de façon universelle |
| **RoverMEMS**, protocole normal | **0xF6** | « Disconnect/Reset diagnostic session » | Réinitialisation de la **session diagnostic** | Ce n'est pas forcément un reboot ECU |
| **RoverMEMS**, code Go Windows i386 de 2023 | `0xCC` (1x), `31 CB` (2J), `14 00 00` (3) | Effacement des défauts | Ces commandes sont effectivement câblées comme `clearfaults` dans le programme Go | Le Go n'apporte **pas** de validation directe de `0xFA` ou `0x0F` comme boutons de reset sur ces familles |
| **Andrew Revill**, MEMS Mapper EU2 / MEMS3 | Service **0x11** dans **ISO 14230 / KWP2000** | « SVC_ECU_RESET » = reset ECU | Service documenté explicitement, distinct de « Clear Fuel Adaptations » | **0x11 n'est pas une commande brute ROSCO** et ne doit pas être envoyé sur une session ROSCO comme équivalent de FA/AF |
| **Andrew Revill**, MEMS Mapper EU3 | « Reset ECU » et « Clear Fuel Adaptations » | Deux actions distinctes | Le reset emprunte intentionnellement ISO14230 même si l'autre protocole est sélectionné ; il **n'enchaîne pas** par une reconnexion immédiate | Le numéro précis et les paramètres propres à chaque variante ne sont pas tous publics |
| **colinbourassa/librosco** (version source historique libre) | `mems_clear_faults` / 0xCC | Clear stored fault codes | Fonction réellement implémentée dans `src/protocol.c` | Pas de fonction `mems_reset_ECU` / `mems_reset_adjustments` exportée dans son API source, donc pas de corroboration de 0xFA ou 0x0F par ce projet |

**Conclusion :** les trois nombres `FA`, `0F` et `AF` existent et sont documentés, mais leurs **étiquettes diffèrent selon les projets**. Aucun nom seul ne valide la sémantique sur toutes les familles/modes. Le service KWP `11` relève d'un protocole différent.

## 2. État de la compatibilité Session 196 — source figée

Dans `memsinterface_dispatch.cpp` de Session 196 :
- `guardedResetAdjustments()` autorise puis émet **0x0F** (voie MEMS 1.9 K-Line ou `mems_reset_adjustments`).
- `guardedResetEcu()` autorise puis émet **0xFA** (voie MEMS 1.9 K-Line ou `mems_reset_ECU`).
- Ces appels ne constituent pas une preuve indépendante de l'effet ECU ; ils fixent la **compatibilité du programme installé**.

Dans `librosco-x64/protocol.c` de `dll-manager`, les anciens noms `mems_reset_ECU` et `mems_reset_adjustments` doivent **continuer à émettre respectivement FA et 0F**, à l'identique du Manager installé. Cela **n'autorise pas** à les présenter comme une preuve sémantique universelle.

La commande LeopoldG `AF` doit rester un export **distinct**, `mems_reset_ECU_leopold` (ordinal 54), jamais remappée silencieusement sur un nom historique de Manager.

## 3. Règles pour enrichir une DLL complète et compatible

1. Ne modifier **ni Manager, ni sa détection de câbles, ni ses actions existantes**. La DLL est l'unique chantier en cours.
2. Ne **jamais** remplacer une fonction d'ABI existante par une opération différente. Garder les exports et leurs ordinals.
3. Les nouvelles fonctions doivent séparer **la famille ECU, le protocole, le mode courant et l'action**. Exemples de noms lisibles : `clear_adaptations`, `reset_ecu`, `reset_diagnostic_session`, `vvt_off`, `clear_dtc`.
4. Un bouton/une fonction d'effacement des adaptations ne doit **pas** déclencher un reset/reboot ECU par supposition.
5. Le code de commandes disponible publiquement doit être intégré en distinguant : source documentée, simulation testée, essai sur calculateur réel.
6. Pas de changement implicite de mode, de reset, de calibration ou de programmation pendant une **connexion** ou une simple **lecture** ; les opérations modifiantes restent explicitement demandées.
7. Une commande de la famille **KWP2000** n'est pas compatible octet-à-octet avec les requêtes **ROSCO** ; une commande de « reset » ne s'identifie jamais seulement par un octet isolé.
8. Les tests de la DLL doivent comparer les constantes avec **le vrai `rosco.h` de Session 196** (ResetAdj=0F, ResetECU=FA), et **pas** les constantes différentes de LeopoldG (ResetAdj=FA, ResetECU=AF).
9. Ce document ne prétend pas résoudre la divergence physique `0x0F` avant preuve adaptée à la famille/variante ; il la **rend traçable** et évite d'ajouter de nouvelles actions mal étiquetées.

## 4. Sources contrôlées

- RoverMEMS, **Technical info**, tableau des commandes MEMS 1.3/1.6/1.9 : https://rovermems.com/diagnostics/technical/
- RoverMEMS (James Portman), code Go : https://github.com/james-portman/rover-mems-agent/tree/ed9ec4dd3181e60dcb7e4bcb340016fcb6bf20f3
- LeopoldG, constantes et association aux boutons : https://github.com/LeopoldG/sources-mems-rosco/blob/main/rosco.h et https://github.com/LeopoldG/sources-mems-rosco/blob/main/memsinterface.cpp
- Andrew Revill, service ECU Reset `11` dans MEMS Mapper EU2 : https://andrewrevill.co.uk/MEMSMapperEU2.htm
- Andrew Revill, **Reset ECU** distinct de **Clear Fuel Adaptations** : https://andrewrevill.co.uk/MEMSMapperEU3.htm
- Andrew Revill, sélection du protocole ISO14230 et reset : https://andrewrevill.co.uk/MEMS3SupportNNN100682.htm
- Colin Bourassa, `librosco` original, `src/protocol.c` : https://github.com/colinbourassa/librosco/blob/master/src/protocol.c
- Manager Session 196 figé, source dispatch : https://github.com/mini56/ECU-Mems-Manager-Session/blob/3f35477e876b9b8a6e2a2ce9e10021a66a7c60ab/memsinterface_dispatch.cpp
- Audit binaire historique du chantier DLL : `librosco-x64/audit_historical_dll_compat.py`.

**Statut final : vérification croisée documentaire et du code source. Ni commande de reset envoyée à un ECU ni modification du programme Session.**

## 5. Protocole de preuve de l'effet ECU — ne pas confondre ACK et action

**Principe : une trame acceptée ne suffit pas à établir l'effet attendu.** L'écho USB local, l'écho généré par l'ECU, un octet d'acquittement et la modification réelle d'un état sont quatre observations différentes.

| Objectif souhaité | Action candidate | Source et niveau de preuve | Observation permettant de confirmer le résultat sur ECU réel | Ce que la DLL ne peut pas présumer |
|---|---|---|---|---|
| Effacer les adaptations apprises (ROSCO 1.3/1.6/1.9) | `FA` | RoverMEMS : « Clear all adaptations », réponse indiquée `FA 00` ; ancien Manager le nomme « Reset ECU » ; code octet confirmé | Relever les valeurs d'adaptation disponibles avant, exécuter la commande en conditions prévues par le constructeur, puis relever les mêmes valeurs après et vérifier le retour vers les valeurs initiales/attendues | Que `FA` provoque un redémarrage matériel ou que chaque variante possède les mêmes adaptations |
| Redémarrer réellement un ECU ROSCO | `AF` | Commande nommée « Reset ECU » dans logiciel/DLL LeopoldG, mais **effet matériel indépendant non identifié** dans les sources examinées | Journal TX/RX avec code ECU/firmware identifié, éventuelle fin de session suivie de nouvelle initialisation, vérification du redémarrage et maintien des adaptations si distinctes | Qu'une fonction appelée reset fasse vraiment rebooter l'ECU, ou que `AF` fonctionne pour tous les modèles |
| Remettre les ajustements à leur valeur par défaut | `0F` historiquement dans Manager | **Non validé** comme effacement d'adaptations : les références attribuent aussi `0F` à « Reset EMI » ou VVT OFF | D'abord identifier exactement la variante, le mode de diagnostic et la documentation de la commande ; ensuite seulement observer les données d'ajustement avant/après dans une procédure de banc maîtrisée | Que `0F` soit un reset ou un simple no-op sur la variante |
| Désactiver VVT (si cet actionneur existe) | `0F` selon tableau RoverMEMS | Documenté dans la famille des commandes d'arrêt d'actionneur, sans preuve universelle ni réponse systématiquement publiée | Vérifier une réaction d'actionneur constatée sur une variante VVT qui prend en charge ce test ; rétablir le fonctionnement ensuite | Qu'un ECU sans VVT supporte la commande |
| Fermer la session diagnostic | `F6` | Description RoverMEMS et observation de Colin Bourassa sur MEMS 1.6 : après F6, l'ECU n'envoie plus les trames normales avant réinitialisation du dialogue | Enregistrer l'arrêt des réponses utiles puis la réussite de CA/75/D0 pour une nouvelle session | Que le microcontrôleur ait réellement redémarré |
| Reset ECU en MEMS KWP/ISO 14230 | **service `11` dans une trame valide**, pas octet ROSCO brut | Source d'Andrew Revill MEMS Mapper EU2/MEMS3 | Session KWP identifiée, requête complète autorisée, résultat positif ou rupture de session prévue, puis dialogue rétabli selon protocole | Que `11` soit utilisable à l'identique sur ROSCO/2J/3/TD5 |

**Classification au 22/11/2026 :**
- `FA` / **effacement adaptations** : **documentation fonctionnelle externe concordante**, mais *non revalidé sur ECU par le présent projet*.
- `AF` / **reboot ECU** : **nom d'API et émission confirmés**, *effet moteur/ECU non établi indépendamment*.
- `0F` / **effacement ajustements** : **non confirmé et conflit de sens majeur** ; ne pas le marquer « fonctionnel » sur la base du seul nom d'export.
- `F6` / **fin session** : **effet expérimental documenté chez Colin Bourassa** pour MEMS 1.6, distinct d'un reboot.
- `11` / **reset ISO 14230** : **service protocolaire documenté chez Andrew Revill**, propre à la session et à la famille ECU.

**Règle de compatibilité pendant l'interdiction de modifier Manager :** la DLL candidate conserve sans changement `mems_reset_ECU=FA`, `mems_reset_adjustments=0F`, `mems_reset_ECU_leopold=AF`. Cette conservation préserve les appels et octets du programme installé, **sans valider les libellés des boutons**. Aucun envoi de commande nouvelle n'est autorisé par le seul audit documentaire. La substitution de la DLL en production reste conditionnée à des tests complets de compatibilité.

## 6. Sources additionnelles relatives à l'effet constaté

- Colin Bourassa, **Protocol**, essai sur banc après `F6` : https://colinbourassa.github.io/car_stuff/mems_interface/
- RoverMEMS, tableau complet des commandes et mentions d'incertitude : https://rovermems.com/diagnostics/technical/
- Andrew Revill, service `11` ECU Reset dans l'implémentation ISO 14230 : https://andrewrevill.co.uk/MEMSMapperEU2.htm
- Andrew Revill, procédure de réinitialisation via ISO 14230 avant retour au protocole Rover/BMW : https://andrewrevill.co.uk/MEMS3SupportNNN100682.htm
- Andrew Revill, avertissement selon familles : commandes des ECU essence pouvant produire des effets nuisibles sur Td5 : https://andrewrevill.co.uk/MEMS3Td5Emulation.htm
