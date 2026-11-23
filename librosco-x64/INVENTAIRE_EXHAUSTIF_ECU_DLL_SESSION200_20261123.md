# INVENTAIRE EXHAUSTIF DU CORPUS ECU — SESSION 200 / `mems_manager_x64.dll`
**Date : 23 novembre 2026 — rapport de référence pour l'intégration future, branche `dll-manager`.**  
**Nature : audit documentaire et comparatif de code. Aucune commande ECU émise, aucun correctif de Manager, aucune compilation déclenchée par le présent rapport.**

## 0. Périmètre, références immuables et règles de lecture

- **Application auditée : SESSION 200 publiée**, tag `mems-session-build-200`, commit exact [`3737dffa3e30e0b9167bda34fa661958b757cd28`](https://github.com/mini56/ECU-Mems-Manager-Session/tree/3737dffa3e30e0b9167bda34fa661958b757cd28). **Ne pas prendre la racine de `dll-manager` pour les sources du Manager de référence** : elle contient d'anciens fichiers Manager.
- **DLL auditée :** sources actuelles `librosco-x64/` de [`dll-manager`](https://github.com/mini56/ECU-Mems-Manager-Session/tree/dll-manager/librosco-x64), contrat d'exports [`mems_manager_x64.def`](mems_manager_x64.def). Le programme officiel 200 et ses workflows sont intacts.
- **Corpus interne réellement ouvert :** les **25 fichiers Markdown** et le **catalogue JSON** du dossier `research/mems/lecture-ecu-19-2j-3/` (le 27e élément du répertoire est un **dossier**, pas un 27e document), les sources Go archivées sous `rovermems-reference/`, les archives et recherches d'août, les deux audits DLL de novembre, les sources C/C++ Session 200 et la liste physique des exports.
- **État des preuves :** **P** = export présent dans la DLL, source implémentée / contrôle binaire réussi ; **C** = codec seul dans la DLL ; **M** = implémenté dans le programme 200 mais **pas** dans la DLL ; **D** = documentaire/recherche seulement ; **?** = sémantique, matériel ou effet non prouvé. Un export **P** ne veut pas dire « essayé sur calculateur réel ».
- **Périmètre DLL cible :** détection transport/câbles, initialisation et session, sérialisation/décodage, lectures, DTC, réglages, actionneurs, protocoles MEMS, RC5 et 5AS, opérations de mémoire justifiées, horodatage du dialogue et résultats structurés. **Exclus de la DLL :** fenêtres, boutons, widgets/cadrans/courbes, documentation utilisateur, traductions, recherche documentaire, IA MEMS et mise en page. La sauvegarde/affichage d'un journal peuvent rester gérés par Manager ; les événements TX/RX/temps doivent venir du moteur DLL.
- **Niveau de risque :** lecture/diagnostic, commande avec effet (actionneur/effacement/réglage), programmation/EEPROM/antidémarrage à impact durable. **Une opération non éprouvée peut être décrite ou implémentée comme expérimentale avec résultat « non vérifié / non pris en charge » ; elle ne doit pas être déclenchée implicitement par « Connecter ».** Ne jamais fabriquer une réponse ECU ou un succès de sauvegarde.

## 1. Résultat chiffré vérifié

| Mesure | Nombre | Ce que cela démontre |
|---|---:|---|
| Exports exactement déclarés dans `mems_manager_x64.def` et vérifiés par le dernier CI | **91** | Ordinals 1 à 91, tous distincts ; **ce ne sont pas 91 fonctions testées sur matériel** |
| Fonctions `mems_*` déclarées dans le vrai `rosco.h` de SESSION 200 | **39** | Contrat d'en-tête du programme, pas nombre de toutes ses opérations ECU |
| Déclarations Session 200 retrouvées parmi les 91 exports | **38** | Correspondances nominales ; les tests PE ont confirmé 15 **imports effectifs du binaire** |
| Déclaration Session 200 absente des exports | **1** | `mems_interactive_mode` : déclaration historique, pas d'export correspondant, non invoquée dans le binaire testé |
| Exports additionnels ne figurant pas dans le `rosco.h` public 200 | **53** | API additionnelle / codecs / helpers ; **non-utilisation actuelle ≠ inutilité** |
| Grands groupes d'exports | **7** | 45 ROSCO + 9 extensions + 11 dispatch famille + 6 injection + 9 2J + 7 MEMS3 + 4 RC5 |
| Familles/profils présents dans le catalogue RoverMEMS JSON | **5** | 1.x, 1.9, 2J, 3, RC5 ; **5AS étudié hors de ces cinq profils** |
| Entrées de mutations archivées dans le catalogue JSON | **24** | 20 pour 1.x, 2 pour 2J, 1 pour MEMS3, 1 pour RC5 ; **entrées de protocole, pas 24 exports nouveaux** |
| Services 2J expérimentaux consignés séparément dans le JSON | **11** | Non universellement identifiés ; ni automatisés ni implicitement considérés sûrs |
| Champs bruts `uk*` historiques encore sans interprétation certaine | **16** | Emplacements connus ; sens métier à rechercher |
| Fonctions annoncées « 194 » dans les échanges antérieurs | **NON JUSTIFIÉ COMME NOMBRE D'EXPORTS** | 194 mélangeait fonctions, modules, futures capacités, UI et documentation ; **aucune liste de 194 signatures ECU vérifiées ne figure dans le corpus lu** |

**Matrice machine exploitable** : [`MATRICE_91_EXPORTS_ET_CONTRAT_SESSION200_20261123.csv`](MATRICE_91_EXPORTS_ET_CONTRAT_SESSION200_20261123.csv) contient 91 lignes d'exports et 1 déclaration de Session 200 non exportée, avec groupe, fichier source, statut et contrôle CI. Elle sert de base pour suivre les nouveaux ordinaux sans perdre l'ABI.

La comparaison correcte consiste donc à **lister les opérations techniques une à une**, y compris celles restées dans Manager et celles présentes seulement dans la recherche. **Ni « 91 » ni « 194 » ne mesurent à eux seuls la couverture fonctionnelle.**

## 2. Matrice des écarts : l'essentiel à transférer ou à développer

| ID | Famille / fonction ECU | DLL aujourd'hui | SESSION 200 / recherche | État réel et travail |
|---|---|---|---|---|
| T01 | Énumérer les ports Windows COM et détecter FTDI/CH340/KKL | — | `serialadapterdetector.h`, `serialdevenumerator.cpp` | **M** : porter l'énumération et fournir candidats/indices, pas déduire la compatibilité du VID/PID |
| T02 | Choisir automatiquement câble + protocole via le seul bouton Connecter | — | `memsinterface_dispatch.cpp` | **M** : ordre d'essais et validation **par réponse ECU** ; un connecteur est une piste, pas une preuve |
| T03 | Gestion du port exclusif, concurrence, annulation, fermeture, reprise après échec | Win32 COM ROSCO partiel | `memsinterface.cpp`, sessions Qt série indépendantes | **P/M** : moteur unique de sessions avec état/cancellation et non-régression |
| T04 | Journal TX/RX horodaté : octets émis/reçus, écho, réponses, durées, erreurs, timeouts | — / primitives série | Observabilité répartie, logs tests / recherche | **M/D** : créer une sortie événements DLL conservable dans un fichier par connexion |
| T05 | Paramétrage des transports 9600 8N1, 10400 8N1, 9600 8E1, 2400 8N1 | 9600 8N1 ROSCO | `mems2jprotocol.h`, `mems3protocol.h`, `rc5protocol.h` | **M** pour autres formats : intégrer profils distincts, ne pas réutiliser un protocole voisin |
| L01 | Init ROSCO 1.2/1.3/1.6 `CA/75/F4/D0`, ID ECU | `mems_connect`, `mems_init_link` | `memsinterface.cpp` | **P** : préserver octets, timing, retours |
| L02 | Lecture/contrôle longueur des trames `80` et `7D` | `mems_read_raw`, `mems_read` | lectures de Session 200 | **P** : préserver conversions, gestion écho et variante 1.2 sans `7D` |
| L03 | Position IAC et déplacement à position absolue | `mems_read_iac_position`, `mems_move_iac` | réglages/Actionneurs | **P** : ne pas confondre position absolue et pas `FD/FE` |
| L04 | Actionneurs ROSCO déjà exposés : purge, lambda, boost, ventilateurs 1/2 | exports historiques + `mems_test_actuator` | commandes via `memsinterface.cpp` | **P/M** : primitive DLL présente, temporisations START/STOP et orchestration encore Manager |
| L05 | Autres actionneurs ROSCO : pompe, PTC, clim, bobine, injecteur, ventilo 3, compte-tours, IAC pas | `mems_test_actuator` générique, wrappers incomplets | `memsinterface.cpp`, `protocolcontext.h`, cartographie | **P/M/D** selon commande ; établir variantes et réponses ; aucune simulation de réussite |
| L06 | Adaptations richesse 1, idle decay, idle speed, avance | exports historiques | `settingspage.cpp`, `memsinterface.cpp` | **P** pour octets; interface seulement hors DLL |
| L07 | Richesse 2 `7B/7C` | `mems_fuel_trim_2_plus/minus` | recherches ROSCO / variantes | **P/?** : octets présents, indépendance du canal par firmware non certifiée |
| L08 | Sauvegarde ajustements `0x81` et réponse | `mems_Save`, `mems_save_adjustments` | commande dans Manager 200 | **P/?** : émission et un octet reçu, **pas preuve de persistance EEPROM** |
| L09 | Réinitialisations `0x0F`, `0xFA`, `0xAF` | wrappers distincts | audit 22/11, `rosco.h` de 200 | **P/?** : contrat **Session 200** 0F=réglages, FA=adaptations calculées/apprises ; AF=Leopold distinct ; effet variant non prouvé |
| L10 | Codes défauts, masques, DTC présents/historiques et correction des ambiguïtés | `mems_read`, `mems_clear_faults` | `RECHERCHE_MEMS_REPRISE`, traces | **P/D** : conserver conflit de masques, ne pas injecter une table non recoupée |
| L11 | D0/D1/D2, identification complémentaire, données brutes et mode courant F0 | raw `mems_send_command` | `memsinterface_dispatch.cpp`, cartographie | **P/M/D** : décodage/usage contextualisés ; D1 ambigu en Mode 4 |
| N01 | MEMS 1.9 : réveil K-Line 5 bauds, horloge, retour clés, écho | primitives de dialogue **après** réveil | `MEMSInterface::performMems19Wakeup` | **M** : manque majeur de transport dans la DLL |
| N02 | MEMS 1.9 : séquence CA/75/F4/D0 après réveil, lectures | `mems_*_kline19`, `mems_manager_family_*` | `memsinterface_dispatch.cpp` | **P/M** : transport préliminaire et sélection de câble à transférer |
| N03 | MEMS 1.9 : variantes et profils injection MPI (KLH4/KLQ4/TBP7F015, autres) | décodages transitoires **6 exports** | `mappedinjection_profiles*.inc`, rapport activation 1.9 | **M** : cartographies et routines métier ECU réparties hors DLL |
| J01 | MEMS 2J : ouverture 10400, BREAK, réveil, gestion écho | encode/decode frames | `Mems2JProtocol::Session::connectTo` | **C + M** : pas de véritable session 2J dans la DLL |
| J02 | MEMS 2J : start session, seed/key, réponses positives/négatives `7F`, ping | `mems2j_seed_key`, `mems2j_response_status` | `mems2jprotocol.h` | **C + M** : orchestrateur session/reprise/timeout à déplacer |
| J03 | MEMS 2J : lectures `21 XX`, unités, décodage et variantes Mini MPi/VVC | `mems2j_make_read_request`, `mems2j_decode_measurement` | `mems2jprotocol.h`, rapports 10–12/14 | **C + M** : lecteur complet et matrice champs/variantes restent Manager |
| J04 | MEMS 2J : défauts `21 19`, états et masques | `mems2j_fault_bit` | `mems2jprotocol.h`, `ecu-2j-faults.go` | **C + M/?** : décodage complet + ambiguïté masque Mini à préserver |
| J05 | MEMS 2J : effacer les DTC en session établie | `mems2j_make_clear_fault_request` (codec) | `Mems2JProtocol::Session::clearFaults` | **C + M** : vraie transaction et confirmation dans Manager |
| J06 | MEMS 2J : autres services mémoire / calibration / statut anti-démarrage | — | catalogue JSON services 13/31/33/23 ; certains commentés | **D/?** : inventaire seulement, ne jamais déclencher par découverte automatique |
| K01 | MEMS 3 : 9600 8E1, réveil, session, seed/key, ping | codec XOR / statut / trames | `Mems3Protocol::Session::connectTo` | **C + M** : session physique et dialoguage absents DLL |
| K02 | MEMS 3 : lectures données `21 XX`, conversions, défauts `18...` | codec `mems3_decode_measurement`, `fault_count/at` | `mems3protocol.h` | **C + M** : cycle d'acquisition, gestion DTC et variantes |
| K03 | MEMS 3 : effacement DTC propre au protocole | — (pas d'export distinct d'effacement 3) | `Mems3Protocol::Session::clearFaults` | **M** : transaction propre à 3, jamais réutiliser ROSCO/2J |
| R01 | RC5 SRS : réveil BREAK 2400, ping et session | `mems_rc5_make_request`, `validate_frame` | `Rc5Protocol::Session::connectTo` | **C + M** : DLL n'ouvre pas de session RC5 ; **ne pas inclure dans auto-identification moteur** |
| R02 | RC5 : lecture/décodage des défauts SRS | `mems_rc5_fault_count/at` | `Rc5Protocol::Session::readFaults` | **C + M** : table et lecture par port encore Manager |
| R03 | RC5 : effacement des défauts SRS, statut réponse | — (codec générique de requête seulement) | `Rc5Protocol::Session::clearFaults` | **M** : commande à effet, spécifique Airbag ; aucune autre action pyrotechnique inférée |
| S01 | Lucas 5AS : transport adapté, réveil diagnostic normal, lecture identifiant/status | **0 export 5AS** | `lucas5asprotocol.h`, `lucas5asreader.h` | **M** : session complète de lecture (non validée matériel), priorité forte |
| S02 | 5AS : lecture des entrées/états, série, type et compteurs PLIP | **0** | `Lucas5AsProtocol::StatusPacket`, `packetValues` | **M/?** : champs variables selon année/modèle, conserver brut |
| S03 | 5AS : sortie testée pendant durée bornée puis arrêt | **0** | `Lucas5AsProtocol::Session::pulseOutput` | **M/?** : tests matériels non validés ; distinguer de l'UI |
| S04 | 5AS : entrée/sortie du mode d'apprentissage télécommandes et état incertain | **0** | `FobLearningCycle`, `Lucas5AsProtocol::Session` | **M/?** : implémentation Manager existe, succès de transmission ≠ association effective |
| S05 | 5AS : journal des octets, interprétation des réponses, échos | **0** | `lucas5asreader.h`, rapport Revill 27 | **M/D** : événements transport dans DLL ; sauvegarde texte hors DLL |
| S06 | 5AS : gestion paramètres EEPROM, codage, jumelage ECU/antidémarrage | **0** | rapports 16/20/27/31 | **D/?** : capacités tierces documentées, aucune session de programmation validée dans Manager 200 |
| S07 | Lucas 10AS, 8AS et Pektron SCU : protocoles spécifiques | **0** | rapports 19/20/27/32/33 | **D/?** : références de variantes, **ne pas copier** l'API 5AS par analogie |
| Q01 | Lecture RAM/ROM diagnostics Mode 4, choix bloc et banque, restauration session | 6 décodeurs d'injection **sans transport Mode 4** | `injectionramtest.cpp`, `mappedinjection_*` | **M** : phase de mesure native mais transaction complète hors DLL |
| Q02 | Injections transitoires AANMP002, coordonnées mémoire, calculs et cadence | `mems_manager_transient_*` | `injectionramtest.cpp` | **P + M** : décodeurs prêts, orchestration de lectures/temporisation dans Manager |
| Q03 | Cartographies injection MPI par firmware, profiles/maps additionnelles | — | `mappedinjection_profiles*.inc` et données associées | **M/D** : logique/profils techniques à centraliser et vérifier par identifiant firmware |
| Q04 | Lire/sauvegarder ROM complète + contrôle checksum/identification | — | `MEMS_MANAGER_FUTURE_FUNCTIONS.md`, travaux Revill | **D/?** : mention d'un `readROM()` stub dans la note historique, **pas démonstration d'un lecteur complet opérationnel dans Session 200** |
| Q05 | Écriture calibration RAM/ROM et modes d'usine | — | cartographie Mode 4, autres sources | **D/?** : effets potentiellement irréversibles, séparer strictement du lecteur ROM et ne pas activer sans spécification/test |
| Q06 | Identification firmware, calibration, correspondances de génération | reconnaissance partielle dans les échanges normaux | `memsinterface`, `mappedinjection_profiles`, fonds de recherche | **M/D** : déplacer la détection *technique* vers DLL, laisser la navigation documentaire à Manager |
| Q07 | 16 octets `uk*` issus des trames 80/7D : valeurs brutes existantes | lecture trames | recherche `16_inconnues` | **P + D/?** : conserver brut, ne pas inventer le sens ; liste complète §5 |
| Q08 | Contrôle des timings et journal de connexion par calculateur | primitives rudimentaires | rapports 13/14, code session Qt, tests récents | **M** : enregistrer TX/RX, réponse, millisecondes, échos, codes d'erreur dans DLL |
| X01 | Helpers de calibration et tests hérités dont le sens reste conflictuel (`0x7E/7F/FF/6D/F9`, etc.) | parfois send raw possible | `ROSCO_COMMAND_CARTOGRAPHY.md` | **D/?** : garder l'entrée au catalogue, aucune « opération validée » inventée |

**Diagnostic principal :** 91 exports techniquement réels, mais **la gestion des sessions 2J, 3, RC5, 5AS, le réveil physique 1.9, la détection câble et la lecture RAM Mode 4** restent dans Manager 200. La première tâche est donc l'**intégration de fonctions déjà codées dans Manager**, pas l'ajout artificiel de fonctions sans utilité.

## 3. Relevé des commandes/lectures retrouvées dans les recherches — couverture par famille

### 3.1 MEMS 1.2, 1.3, 1.6 et 1.9

- **Lecture** : identité D0, complément D1/D2 en mode approprié, trames 80 et 7D, DTC et états, position IAC FB. Pour 1.9, réveil K-Line 5 bauds préalable, puis dialogue ROSCO avec gestion des échos.
- **Réglages** : `79/7A` richesse 1, `7B/7C` richesse 2 (validité indépendante à confirmer), `89/8A` idle decay, `91/92` ralenti cible, `93/94` avance, `FD/FE` IAC pas à pas, `0x81` demande Save (persistance non prouvée).
- **Actionneurs explicitement cartographiés** : pompe carburant, PTC, A/C, purge, lambda, wastegate/boost, ventilateurs 1/2/3, jauge température, VVT, EGR, solénoïdes, voyant, compte-tours, boost-gauge, bobine et injecteur (selon variantes/firmware). **Ne pas assimiler une commande cataloguée à une sortie physiquement présente sur le véhicule**.
- **Autres octets en recherche** : `20/21/30/31/60/61/63/64/65/67/6B/6D/6F/DA/DB/EF/F7/F8/7E/7F/82/F9/FF`. Les conflits sont conservés dans la cartographie : **F7 et F8 changent de sens selon le mode**, **0F** conflit historique **reset vs VVT**, et **7C** dans le réveil 1.9 ne représente pas la même action qu'en session de réglages.
- **Diagnostic 4 et mémoire** : interrogation F0, transitions F2/F3/F4/F5 selon mode, choix de banque/bloc, lecture RAM/ROM, calcul de l'injection. `D1/D3/F8` en Mode 4 peuvent avoir des effets de programmation ; **aucune écriture implicite**.

### 3.2 MEMS 2J : inventaire complet des lectures documentées dans le catalogue

| ID lecture `21 xx` | Groupe de données | Ce qui reste à prouver / préserver |
|---|---|---|
| `00`, `06` | Paquets présents mais non décodés | conserver brut |
| `01`, `02`, `03`, `05` | Températures eau/huile/air/carburant | huile/carburant éventuellement absents et unité carburant non prouvée |
| `07`, `08`, `09` | MAP, papillon, régime | calibrations et variantes |
| `0A`, `0B`, `0C` | Lambda/correction, bobines, injecteurs | unités/scales et nombre réel d'injecteurs par véhicule |
| `0D`, `0F`, `10`, `11` | Vitesse, contacts/clim, batterie, synchronisations | services refusés sur certaines variantes |
| `12`, `13` | IAC/ralenti et boucle fermée | précision/échelle |
| `19` | DTC `61 19` | masques **Mini MPi**, collisions de bits documentées, pas universels VVC |
| `21`, `25`, `3A` | Écart régime signé, came, corrections de ralenti/consigne | erreur de conversion Go sur valeur négative ; `25` parfois refusée ; échelles à confirmer |

**Total : 22 identifiants `21 xx` distincts dans cette liste**, dont `19` pour les défauts. La DLL contient les *helpers* `mems2j_*` ; le lecteur qui enchaîne ces demandes, négocie la session et gère la réponse réelle est encore le C++ de Manager.

**Autres capacités explicitement documentées** : réveil K-Line rapide, session diagnostic, authentification seed/key, ping, gestion écho, refus KWP2k et réponse « pending », effacement DTC sur demande, 11 services exploratoires/variables et trois pistes commentées (deux services de calibration et ROM incomplète). **Ne pas classer les pistes commentées comme commandes implantées.**

### 3.3 MEMS 3

- Protocole distinct : 9600 **8E1**, trames `B8 13 F7` + longueur + XOR ; réveil propre, session diagnostique, seed/key, ping et gestion des réponses.
- **Lectures** : `21 00` températures, `21 06` MAP/papillon/RPM, `21 0A` carburant/lambda, `21 0B` bobines, `21 21` écart régime ; DTC par `18 00 00 00` et réponse `58`.
- **Effet** : effacement DTC propre au MEMS 3. Pas de preuve que les actionneurs ROSCO 1.x s'appliquent au MEMS 3.
- **Écart** : le dialogue complet est dans `mems3protocol.h` du Manager, tandis que la DLL ne contient que framing / décodages.

### 3.4 RC5 / airbag SRS

- **Lecture** : réveil spécifique 2400 8N1, ping, lecture des défauts SRS, DTC 16 bits et état historique/présent, contrôle trames/réponses.
- **Effet** : effacement DTC SRS uniquement sur demande explicite.
- **Écart** : exports 88–91 = encode/validate/count/at ; toute la session `Rc5Protocol::Session` et la logique de `Rc5Worker` sont encore dans Manager. **Le RC5 n'est pas un ECU moteur.**

### 3.5 Lucas 5AS, 10AS, 8AS et Pektron

- **Lucas 5AS** : code Manager 200 pour réveil diagnostic normal, réception réelle, lecture du paquet de statut, identification/états/compteurs, sorties temporisées et apprentissage PLIP (avec états incertains et commande de fin). **0 export 5AS** dans notre DLL.
- **Lucas 5AS — non acquis** : lecture/écriture EEPROM, codage, jumelage antidémarrage↔ECU moteur, vérification physique de l'apprentissage, compatibilité toutes générations et capacités additionnelles de Lucas 5AS Tools. Ce sont des **références de recherche** ; aucune écriture/association EEPROM n'est prouvée dans le Manager 200.
- **Lucas 10AS / 8AS** : archives de mémoire et constat de variantes ; **pas** de protocole complet confirmé dans notre DLL. Le 10AS ne reprend pas automatiquement câbles/commandes 5AS ; le 8AS reste distinct.
- **Pektron SCU** : autre famille, binaire et protocole à étudier séparément ; **aucune compatibilité par déduction**.
- **Câbles** : documenter avec famille/variante et caractéristiques physiques ; reconnaissance USB `COM/FTDI/CH340` ne garantit **ni l'inversion de ligne ni le câblage**.

## 4. Mémoire, injection et ROM : ce qui est techniquement acquis

| Fonction technique | État documenté |
|---|---|
| Décodeurs d'injection transitoire, 6 exports C | Dans DLL ; tests hors ECU |
| Commandes et cycles RAM AANMP002 (phases, échantillonnage/cadence) | Dans `injectionramtest.cpp` du **vrai Manager 200**, pas totalement dans DLL |
| Cartographie profils injection, calibration dépendant des firmwares | `mappedinjection_profiles*.inc` / `mappedinjection_runtime_part*.inc` dans Manager |
| Lectures RAM/ROM en Mode 4, banque/bloc et restauration | Code et recherches à rapprocher, **pas d'API complète DLL** |
| Lecture/sauvegarde ROM entière + SHA/checksum | **Recherchée**, pas une fonction complète de DLL démontrée ; ne pas déclarer « ROM entière lisible » avant code et résultats |
| Identification ROM/firmware/calibration par ID, révision, signature | Données et idées de recherche ; API de détection à définir |
| Écriture/remapping ROM / EEPROM | Fonction possible selon boîtier, **non validée, potentiellement destructive** ; ne jamais confondre « trame connue » et « programmation sûre » |

Distinguer dans la DLL future : **lecture brute**, décodage avec preuve, opérations susceptibles de modifier un état volatile, et **écriture non volatile**. En particulier, `0x81` ou la présence d'un ACK ne démontre pas la mémorisation d'un réglage.

## 5. Les 16 champs bruts restant sans signification confirmée

D'après [`research/16_inconnues/ETAT_2026-08-21.md`](../research/16_inconnues/ETAT_2026-08-21.md) :

- **Trame `0x80` :** `uk1@0x0B`, `uk2@0x11`, `uk3@0x19`, `uk4@0x1A`, `uk5@0x1B`.
- **Trame `0x7D` :** `uk6@0x03`, `uk7@0x10`, `uk11@0x17`, `uk12@0x18`, `uk13@0x19`, `uk14@0x1A`, `uk15@0x1B`, `uk16@0x1C`, `uk1A@0x1D`, `uk1B@0x1E`, `uk1C@0x1F`.

Les valeurs existent dans les trames brutes ; **le sens est inconnu**. Ne **pas** reclasser comme inconnus : `uk8` (DTC4), `uk9` (correction ralenti) et `uk10` (brut `7D14–15` et correction ralenti chaud). La suite de la recherche doit relier chaque champ à la ROM AANMP002 et à son adresse RAM avant de lui donner un libellé.

## 6. Compatibilité actuelle — ce qu'ont effectivement prouvé les tests

- [Run vert DLL + Session 200](https://github.com/mini56/ECU-Mems-Manager-Session/actions/runs/45072564428) : DLL Windows x64, **4 CTest / 4**, **91 exports**, **15 imports** Manager résolus, compilation et test UI 16 pages.
- [Run vert Setup séparé de Session 200 + nouvelle DLL](https://github.com/mini56/ECU-Mems-Manager-Session/actions/runs/45134425955) : **vrai Setup officiel 200** vérifié et extrait sur runner Windows temporaire, **seule la DLL remplacée** dans la copie ; **Setup de test indépendant** généré, installé, EXE original 200 / DLL recompilée vérifiés par SHA256, UI 16 pages réussie puis désinstallation.
- **Ces runs ne prouvent PAS** les échanges réels 1.9/2J/3/RC5/5AS, les sorties physiques, la persistance des réglages, une sauvegarde ROM ou les fonctions de programmation. Les opérations testées logiciel doivent rester marquées comme telles.

## 7. Recommandations d'architecture pour intégrer toutes les fonctions ECU

1. **Ne pas remplacer les 91 exports ni renuméroter les ordinaux.** API existante stable ; ajouter des fonctions à la suite avec signatures C stables et versionnage explicite ; ne pas introduire de fausse `mems_interactive_mode` pour masquer son absence.
2. **Une DLL de moteur ECU complète, sans Qt ni interface** : gestionnaire COM Windows unique, profils de transport, identification par réponse ECU, sessions distinctes par famille, opérations explicites, récupération des réponses, erreurs et temps. Manager conserve l'UI, l'IA, l'Aide et la présentation.
3. **Un seul bouton Connecter** dans Manager. La DLL renvoie famille/variante, identifiants, statut de session, prise en charge des capacités ; **pas de nouveau choix de câble imposé à l'utilisateur**.
4. **Réutiliser les comportements éprouvés** : transférer, sans modifier leur sémantique, les implémentations existantes 1.9, 2J, 3, RC5 et 5AS de Session 200 ; éviter de copier les bugs du Go externe ou d'ajouter des verrous artificiels.
5. **Registre de capacités et état de preuve** par fonction/famille/firmware : `SOURCE_RECONSTITUEE`, `CODE_INTEGRE`, `TEST_SYNTHETIQUE_OK`, `ECU_REEL_OK`, `NON_CONFIRME`, `NON_SUPPORTÉ`. Un succès d'écriture série n'est pas un succès de l'opération ECU.
6. **Fonctions issues de recherches non testées** : les conserver dans la matrice et implémenter celles dont la trame, le contexte et le résultat peuvent être définis honnêtement. Les commandes non qualifiées, EEPROM, calibration et antidémarrage à effets durables restent des fonctions expérimentales non déclenchables automatiquement tant que leur comportement n'est pas établi.
7. **Journal de communication centralisé** : chaque connexion produit l'ECU, le protocole, les octets TX/RX, l'écho, les réponses/négatifs, délais réels, erreurs et opérations demandées ; fichier/export et vue du journal appartiennent à Manager.
8. **Séparation licence et provenance** : sources Go RoverMEMS archivées sous **GPLv3** (vérifier obligations avant réemploi), analyse statique du binaire Revill à utiliser comme **constat technique**, jamais copier un binaire tiers ou du code propriétaire dans notre DLL.

## 8. Feuille de route après inventaire, sans altérer SESSION 200

| Lot | Travail dans `dll-manager` | Critère de sortie |
|---|---|---|
| 0 | Fixer cet inventaire, les ABI et la source Session 200 | 91 exports et 39 déclarations suivis ; aucun changement Manager |
| 1 | Transport COM, détection des ports/câbles, retour de connexion, horodatage TX/RX | Tests simulateur (faux ports/réponses), timeout, annulation, CI DLL uniquement |
| 2 | Transport ROSCO 1.x et réveil intégral 1.9 + modes de lecture RAM diagnostic | Rejeu des traces et tests de non-régression contre code 200 |
| 3 | Sessions MEMS 2J et 3 complètes, décodage et DTC | Connexions simulées (positives, erreurs, échos), variables et variantes |
| 4 | RC5 SRS et Lucas 5AS (diagnostic normal, lectures, sorties / PLIP expérimentaux) | Protocoles isolés, aucun effet implicite, tests de réponses partielles |
| 5 | ROM/firmware, cartographies et 16 champs inconnus ; revue fonctions expérimentales | Chaque opération étiquetée selon preuves et support firmware ; pas de succès fictif |
| 6 | Construire une copie de **Session 200** avec la DLL complète, puis Setup séparé | Windows x64, 91 exports conservés, tests complets, Setup isolé, aucun MAJ officiel |
| 7 | **Après** validation DLL : raccordement de Manager au moteur unique ECU | Lot dédié avec autorisation préalable ; aucune modification pendant l'audit |

**Règle de projet :** correction **dans les sources**, sans empiler les patchs ni créer une branche par correction ; conserver `dll-manager`, ne pas modifier ni publier SESSION 200 ou les dépôts MAJ durant cet audit. Pour les tests réels, ne pas confondre lecture, mutation et authentification.

## 9. Annexes — les 91 exports exactement listés (source `.def`)

#### Exports 01-45 — ROSCO historique (exports LeopoldG, avec sémantiques Manager protégées)

| Ordinal | Nom réel exporté |
|---:|---|
| 1 | `mems_Boost_Valve_Off` |
| 2 | `mems_Boost_Valve_On` |
| 3 | `mems_CloseIAC` |
| 4 | `mems_Fan1_Off` |
| 5 | `mems_Fan1_On` |
| 6 | `mems_Fan2_Off` |
| 7 | `mems_Fan2_On` |
| 8 | `mems_O2Heater_Off` |
| 9 | `mems_O2Heater_On` |
| 10 | `mems_OpenIAC` |
| 11 | `mems_Purge_Valve_Off` |
| 12 | `mems_Purge_Valve_On` |
| 13 | `mems_Save` |
| 14 | `mems_cleanup` |
| 15 | `mems_clear_faults` |
| 16 | `mems_connect` |
| 17 | `mems_disconnect` |
| 18 | `mems_fuel_trim_minus` |
| 19 | `mems_fuel_trim_plus` |
| 20 | `mems_get_lib_version` |
| 21 | `mems_heartbeat` |
| 22 | `mems_idle_decay_minus` |
| 23 | `mems_idle_decay_plus` |
| 24 | `mems_idle_speed_minus` |
| 25 | `mems_idle_speed_plus` |
| 26 | `mems_ignition_advance_minus` |
| 27 | `mems_ignition_advance_plus` |
| 28 | `mems_init` |
| 29 | `mems_init_link` |
| 30 | `mems_is_connected` |
| 31 | `mems_lock` |
| 32 | `mems_move_iac` |
| 33 | `mems_openserial` |
| 34 | `mems_read` |
| 35 | `mems_read_iac_position` |
| 36 | `mems_read_raw` |
| 37 | `mems_read_serial` |
| 38 | `mems_reset_ADJ` |
| 39 | `mems_reset_ECU` |
| 40 | `mems_reset_EMI` |
| 41 | `mems_send_command` |
| 42 | `mems_test_actuator` |
| 43 | `mems_unlock` |
| 44 | `mems_write_serial` |
| 45 | `temperature_value_to_degrees_f` |

#### Exports 46-54 — extensions ROSCO / MEMS 1.9 / réglages

| Ordinal | Nom réel exporté |
|---:|---|
| 46 | `mems_reset_adjustments` |
| 47 | `mems_save_adjustments` |
| 48 | `mems_init_link_kline19` |
| 49 | `mems_read_kline19` |
| 50 | `mems_command_kline19` |
| 51 | `mems_move_iac_kline19` |
| 52 | `mems_fuel_trim_2_plus` |
| 53 | `mems_fuel_trim_2_minus` |
| 54 | `mems_reset_ECU_leopold` |

#### Exports 55-65 — dispatch par famille 1.2/1.3/1.6/1.9

| Ordinal | Nom réel exporté |
|---:|---|
| 55 | `mems_manager_family_supported` |
| 56 | `mems_manager_family_uses_kline19` |
| 57 | `mems_manager_family_init_link` |
| 58 | `mems_manager_family_read` |
| 59 | `mems_manager_family_read_iac` |
| 60 | `mems_manager_family_move_iac` |
| 61 | `mems_manager_family_clear_faults` |
| 62 | `mems_manager_family_reset_adjustments` |
| 63 | `mems_manager_family_reset_ecu` |
| 64 | `mems_manager_family_save_adjustments` |
| 65 | `mems_manager_family_actuator` |

#### Exports 66-71 — décodage injection transitoire

| Ordinal | Nom réel exporté |
|---:|---|
| 66 | `mems_manager_transient_parse_mode_reply` |
| 67 | `mems_manager_transient_is_aanmp002` |
| 68 | `mems_manager_transient_parse_data80` |
| 69 | `mems_manager_transient_parse_word_reply` |
| 70 | `mems_manager_transient_decode_pair` |
| 71 | `mems_manager_transient_compare_ticks` |

#### Exports 72-80 — codec MEMS 2J, sans dialogue série complet

| Ordinal | Nom réel exporté |
|---:|---|
| 72 | `mems2j_frame_encode` |
| 73 | `mems2j_frame_decode` |
| 74 | `mems2j_make_read_request` |
| 75 | `mems2j_make_clear_fault_request` |
| 76 | `mems2j_make_key_request` |
| 77 | `mems2j_seed_key` |
| 78 | `mems2j_response_status` |
| 79 | `mems2j_decode_measurement` |
| 80 | `mems2j_fault_bit` |

#### Exports 81-87 — codec MEMS 3, sans dialogue série complet

| Ordinal | Nom réel exporté |
|---:|---|
| 81 | `mems3_frame_encode` |
| 82 | `mems3_frame_decode` |
| 83 | `mems3_make_read_request` |
| 84 | `mems3_response_status` |
| 85 | `mems3_decode_measurement` |
| 86 | `mems3_fault_count` |
| 87 | `mems3_fault_at` |

#### Exports 88-91 — codec RC5 SRS, sans dialogue série complet

| Ordinal | Nom réel exporté |
|---:|---|
| 88 | `mems_rc5_make_request` |
| 89 | `mems_rc5_validate_frame` |
| 90 | `mems_rc5_fault_count` |
| 91 | `mems_rc5_fault_at` |

**Déclaration du header SESSION 200 non exportée :** `mems_interactive_mode`. L'audit des exports ne doit pas la compter parmi les 91.

## 10. Index vérifiable des recherches conservées dans le dépôt

Le répertoire [`research/mems/lecture-ecu-19-2j-3`](../research/mems/lecture-ecu-19-2j-3/) a été parcouru fichier par fichier. Il contient **25 Markdown + 1 JSON + 1 dossier d'archive** :

- [`10_AUDIT_2J_VARIANTES_MESURES_DEFAUTS_20261119.md`](../research/mems/lecture-ecu-19-2j-3/10_AUDIT_2J_VARIANTES_MESURES_DEFAUTS_20261119.md)
- [`11_TRAMES_CALCULS_ET_PREVALIDATION_19_2J_20261119.md`](../research/mems/lecture-ecu-19-2j-3/11_TRAMES_CALCULS_ET_PREVALIDATION_19_2J_20261119.md)
- [`12_BILAN_RECHERCHE_ET_DECODAGE_19_2J_AVANT_INTEGRATION_20261119.md`](../research/mems/lecture-ecu-19-2j-3/12_BILAN_RECHERCHE_ET_DECODAGE_19_2J_AVANT_INTEGRATION_20261119.md)
- [`13_JOURNAL_DEVELOPPEMENT_MEMS19_PUIS_2J_20261120.md`](../research/mems/lecture-ecu-19-2j-3/13_JOURNAL_DEVELOPPEMENT_MEMS19_PUIS_2J_20261120.md)
- [`14_RAPPORT_DETAILLE_ROVERMEMS_I386_TOUTES_SEQUENCES_20261120.md`](../research/mems/lecture-ecu-19-2j-3/14_RAPPORT_DETAILLE_ROVERMEMS_I386_TOUTES_SEQUENCES_20261120.md)
- [`16_ETUDE_PREPARATOIRE_RC5_5AS_ANTIDEMARRAGE_CLES_20261121.md`](../research/mems/lecture-ecu-19-2j-3/16_ETUDE_PREPARATOIRE_RC5_5AS_ANTIDEMARRAGE_CLES_20261121.md)
- [`17_COMPLEMENTS_RC5_CABLAGE_VEHICULES_ET_VERIFICATIONS_SESSION187_20261121.md`](../research/mems/lecture-ecu-19-2j-3/17_COMPLEMENTS_RC5_CABLAGE_VEHICULES_ET_VERIFICATIONS_SESSION187_20261121.md)
- [`18_ETUDE_PLIP_433MHZ_COPIE_ET_APPRENTISSAGE_5AS_20261121.md`](../research/mems/lecture-ecu-19-2j-3/18_ETUDE_PLIP_433MHZ_COPIE_ET_APPRENTISSAGE_5AS_20261121.md)
- [`19_ETUDE_LUCAS_5AS_PEKTRON_10AS_COMPATIBILITE_ET_LECTURE_20261121.md`](../research/mems/lecture-ecu-19-2j-3/19_ETUDE_LUCAS_5AS_PEKTRON_10AS_COMPATIBILITE_ET_LECTURE_20261121.md)
- [`20_DIAGNOSTIC_5AS_MODE_NORMAL_CABLE_ET_VARIANTES_10AS_20261121.md`](../research/mems/lecture-ecu-19-2j-3/20_DIAGNOSTIC_5AS_MODE_NORMAL_CABLE_ET_VARIANTES_10AS_20261121.md)
- [`21_AUDIT_CABLES_LUCAS_5AS_KKL_FTDI_CH340_PLIP_PEKTRON_20261121.md`](../research/mems/lecture-ecu-19-2j-3/21_AUDIT_CABLES_LUCAS_5AS_KKL_FTDI_CH340_PLIP_PEKTRON_20261121.md)
- [`22_GUIDE_FABRICATION_CABLES_RC5_5AS_PLIP_20261121.md`](../research/mems/lecture-ecu-19-2j-3/22_GUIDE_FABRICATION_CABLES_RC5_5AS_PLIP_20261121.md)
- [`23_LUCAS_5AS_PREPARATION_LECTURE_WIN32_SERIE_ET_TESTS_20261121.md`](../research/mems/lecture-ecu-19-2j-3/23_LUCAS_5AS_PREPARATION_LECTURE_WIN32_SERIE_ET_TESTS_20261121.md)
- [`24_ORIENTATION_RECHERCHE_DIALOGUE_COMPLET_LUCAS_5AS_20261121.md`](../research/mems/lecture-ecu-19-2j-3/24_ORIENTATION_RECHERCHE_DIALOGUE_COMPLET_LUCAS_5AS_20261121.md)
- [`25_JOURNAL_ANDREW_REVILL_5AS_AUDIT_ET_TRACE_A_RECHERCHER_20261121.md`](../research/mems/lecture-ecu-19-2j-3/25_JOURNAL_ANDREW_REVILL_5AS_AUDIT_ET_TRACE_A_RECHERCHER_20261121.md)
- [`26_REVILL_LOG_5AS_ROSCO_MODIFIE_VERIFICATION_20261121.md`](../research/mems/lecture-ecu-19-2j-3/26_REVILL_LOG_5AS_ROSCO_MODIFIE_VERIFICATION_20261121.md)
- [`27_BINAIRE_LUCAS5ASOUTILS_ARCHIVE_20261121.md`](../research/mems/lecture-ecu-19-2j-3/27_BINAIRE_LUCAS5ASOUTILS_ARCHIVE_20261121.md)
- [`28_SESSION_189_IMPLEMENTATION_LECTURE_LUCAS_5AS_20261121.md`](../research/mems/lecture-ecu-19-2j-3/28_SESSION_189_IMPLEMENTATION_LECTURE_LUCAS_5AS_20261121.md)
- [`29_ARCHIVE_FIRMWARE_ANDREW_REVILL_REFERENCE_20261121.md`](../research/mems/lecture-ecu-19-2j-3/29_ARCHIVE_FIRMWARE_ANDREW_REVILL_REFERENCE_20261121.md)
- [`30_LUCAS5AS_PLIP_APPRENTISSAGE_SESSION192_20261121.md`](../research/mems/lecture-ecu-19-2j-3/30_LUCAS5AS_PLIP_APPRENTISSAGE_SESSION192_20261121.md)
- [`31_ARCHIVE_LUCAS5AS_EEPROM_ANDREW_REVILL_20261121.md`](../research/mems/lecture-ecu-19-2j-3/31_ARCHIVE_LUCAS5AS_EEPROM_ANDREW_REVILL_20261121.md)
- [`32_ARCHIVE_LUCAS10AS_EEPROM_ANDREW_REVILL_20261121.md`](../research/mems/lecture-ecu-19-2j-3/32_ARCHIVE_LUCAS10AS_EEPROM_ANDREW_REVILL_20261121.md)
- [`33_ARCHIVE_LUCAS8AS_ANDREW_REVILL_20261121.md`](../research/mems/lecture-ecu-19-2j-3/33_ARCHIVE_LUCAS8AS_ANDREW_REVILL_20261121.md)
- [`34_AUDIT_REGLAGES_ROSCO_ET_MEMS_20261121.md`](../research/mems/lecture-ecu-19-2j-3/34_AUDIT_REGLAGES_ROSCO_ET_MEMS_20261121.md)
- [`README.md`](../research/mems/lecture-ecu-19-2j-3/README.md)

- [Catalogue JSON machine des 5 profils et des commandes](../research/mems/lecture-ecu-19-2j-3/15_CATALOGUE_MACHINE_PROTOCOLS_ROVERMEMS_I386_20261120.json) : profils, 24 entrées modifiantes, 11 services exploratoires 2J, 3 éléments commentés/non implémentés, preuves attendues.
- [Sources originales RoverMEMS conservées pour comparaison](../research/mems/lecture-ecu-19-2j-3/rovermems-reference/i386-20230128/) : sources Go 1.x/1.9/2J/3/RC5, corrections de réponse, transport et licence GPLv3 ; jamais assimilées à des essais matériels.
- [Cartographie complète ROSCO et Mode 4](ROSCO_COMMAND_CARTOGRAPHY.md).
- [Audit sémantique réinitialisations `0F/FA/AF/81`](AUDIT_REINITIALISATIONS_SOURCES_EXTERNES_20261122.md).
- [Pistes techniques historiques (ROM, identification, profils)](../MEMS_MANAGER_FUTURE_FUNCTIONS.md).
- [Recherche 16 champs inconnus](../research/16_inconnues/ETAT_2026-08-21.md).
- [Recherches août 2026 / conflits de masques DTC](../RECHERCHE_MEMS_REPRISE_2026-08-17.md).
- [Activation et injection MEMS 1.9](../activation-protocole-1.9/RAPPORT_ACTIVATION_PROTOCOLE_MEMS_1.9.md).
- [Règles permanentes de recoupement](../MEMS_RESEARCH_RULES.md).

## 11. À ne pas déclarer tant que ce n'est pas prouvé

- « 194 fonctions de la DLL » : faux / mélange de périmètres. **91 exports actuellement**, plusieurs opérations de Manager à rapatrier et fonctions de recherche à développer.
- « MEMS 2J/3/RC5/5AS complet dans la DLL » : faux, présence de **codecs** ou d'exports partiels seulement, les sessions restent dans les sources 200.
- « Sauvegarde des ajustements EEPROM validée » : faux, le simple ACK `0x81` ne prouve pas la persistance.
- « Reset 0x0F identique sur tous les firmwares » : non prouvé et parfois contradictoire selon source/mode.
- « Écriture ROM / EEPROM ou jumelage 5AS validés » : faux, recherches et fonctions tierces ne suffisent pas.
- « Les 16 champs inconnus sont des capteurs cassés » : faux, ils sont présents en brut ; leur sens reste inconnu.

**Conclusion définitive de l'audit au 23/11/2026 :** les oublis les plus importants sont **le transport ECU complet déjà présent dans Manager** (MEMS 1.9, 2J, 3, RC5, Lucas 5AS, détection COM, mode 4/injection) et **des familles d'opérations documentées mais non encore intégrées** (ROM, mémoire/calibration, services exploratoires, certaines variantes et commandes). **L'inventaire couvre exhaustivement le corpus cité et audité à cette date, pas une promesse que toutes les fonctions existant dans l'écosystème automobile sont connues.**