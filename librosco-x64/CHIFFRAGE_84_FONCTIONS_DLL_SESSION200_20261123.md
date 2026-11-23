# CHIFFRAGE NOMINATIF DES FONCTIONS À AJOUTER À LA DLL — SESSION 200

**23/11/2026 — proposition d'API pour la branche `dll-manager` — DOCUMENT DE PLANIFICATION, NON DU CODE COMPILÉ.**

## Résultat demandé

- **91 exports existants**, vérifiés dans `mems_manager_x64.def` et au CI Windows.
- **84 nouvelles fonctions d'API proposées** ci-dessous, **toutes nommées individuellement** : 81 fondées sur les fonctions/algorithmes existants dans Session 200, **3 fonctions ROM encore seulement documentées** et donc non opérationnelles aujourd'hui.
- **175 exports ciblés** dans ce découpage hypothétique : 91 conservés + 84 nouveaux, **si toutes les 84 deviennent effectivement des exports publics**.
- Ce chiffre est **une décision de découpage proposée**, pas une quantité « découverte » automatiquement dans le code. Lors de la conception des signatures, plusieurs sous-opérations pourront être internes à la DLL, fusionnées ou ajoutées : le total final devra être ajusté en expliquant les changements, jamais présenté comme un décompte incontestable.
- **Hors de ces 84** : 11 services exploratoires MEMS 2J consignés dans le catalogue, fonctions Lucas 5AS EEPROM/jumelage, Lucas 10AS/8AS/Pektron, programmation ROM et autres commandes de sens incertain. On sait qu'ils existent comme *pistes* ; faute d'un contrat de réponse/effet suffisamment documenté, leur **nombre de fonctions réellement à ajouter reste à déterminer**. Ils ne doivent ni disparaître du rapport ni être faussement déclarés implémentés.
- Les fonctions **M** portent un comportement technique identifié dans les sources Manager 200, parfois combiné sous une fonction existante ; il peut être extrait / mutualisé. Les fonctions **D** ne possèdent pas encore d'implémentation source complète dans Manager 200.

## Répartition arithmétique

| Groupe | Fonctions nouvelles proposées | Référence principale |
|---|---:|---|
| Transport / auto-connexion / traçage | **15** | `serialadapterdetector.h; memsinterface_dispatch.cpp; memsinterface.cpp` |
| ROSCO / MEMS 1.2–1.6 | **9** | `memsinterface.cpp; memsinterface_dispatch.cpp; ROSCO_COMMAND_CARTOGRAPHY.md; research 16_inconnues` |
| MEMS 1.9 K-Line | **6** | `memsinterface_dispatch.cpp; mems19protocol.h; activation-protocole-1.9` |
| MEMS 2J | **12** | `mems2jprotocol.h; rovermems-reference/i386-20230128/ecu-2j*.go` |
| MEMS 3 | **10** | `mems3protocol.h; rovermems-reference/i386-20230128/ecu-3.go` |
| RC5 SRS Airbag | **7** | `rc5protocol.h; rovermems-reference/i386-20230128/ecu-rc5.go` |
| Lucas 5AS | **11** | `lucas5asprotocol.h; lucas5asreader.h; recherches 16–31` |
| RAM / injection / ROM / firmware | **14** | `injectionramtest.cpp; mappedinjection_profiles*.inc; MEMS_MANAGER_FUTURE_FUNCTIONS.md; ROSCO_COMMAND_CARTOGRAPHY.md` |
| **TOTAL** | **84** | 91 + 84 = **175** exports projetés |

## Liste nominative et justification (84 propositions, chacune distincte)

### T — Transport / auto-connexion / traçage (15 ajouts)

Sources à comparer : `serialadapterdetector.h; memsinterface_dispatch.cpp; memsinterface.cpp`. **État M :** code/algorithme déjà exploitable dans Manager, sans préjuger des tests ECU réels ; **état D :** fonction issue de la recherche, encore à implémenter et prouver.

| ID | Nom d'API **proposé** (pas un export actuel) | Rôle technique | État |
|---|---|---|---|
| T01 | `mems_manager_list_serial_adapters` | recenser ports COM candidats | M |
| T02 | `mems_manager_get_adapter_details` | obtenir attributs du port/câble | M |
| T03 | `mems_manager_detect_adapter_family` | reconnaître familles adaptateur | M |
| T04 | `mems_manager_auto_connect` | choisir protocole et confirmer par ECU | M |
| T05 | `mems_manager_get_session_identity` | obtenir identité ECU de la session | M |
| T06 | `mems_manager_get_session_family` | obtenir famille ECU reconnue | M |
| T07 | `mems_manager_get_session_capabilities` | obtenir fonctions disponibles du modèle | M |
| T08 | `mems_manager_disconnect_auto` | terminer session toutes familles | M |
| T09 | `mems_manager_cancel_current` | annuler opération/session en cours | M |
| T10 | `mems_manager_set_session_timeouts` | paramétrer délais communication | M |
| T11 | `mems_manager_get_session_timeouts` | lire délais de communication | M |
| T12 | `mems_manager_get_transport_error` | récupérer erreur technique détaillée | M |
| T13 | `mems_manager_get_next_trace_event` | lire événement TX/RX horodaté | M |
| T14 | `mems_manager_clear_trace` | vider trace de la session | M |
| T15 | `mems_manager_get_response_timing` | durée réelle ECU pour la transaction | M |

### L — ROSCO / MEMS 1.2–1.6 (9 ajouts)

Sources à comparer : `memsinterface.cpp; memsinterface_dispatch.cpp; ROSCO_COMMAND_CARTOGRAPHY.md; research 16_inconnues`. **État M :** code/algorithme déjà exploitable dans Manager, sans préjuger des tests ECU réels ; **état D :** fonction issue de la recherche, encore à implémenter et prouver.

| ID | Nom d'API **proposé** (pas un export actuel) | Rôle technique | État |
|---|---|---|---|
| L01 | `mems_manager_rosco_read_d1` | identification complémentaire D1 contexte normal | M |
| L02 | `mems_manager_rosco_read_d2` | lecture état sécurité D2 | M |
| L03 | `mems_manager_rosco_read_f0` | interrogation mode diagnostique F0 | M |
| L04 | `mems_manager_rosco_read_adjustments` | lecture des réglages/valeurs mesurées | M |
| L05 | `mems_manager_rosco_get_unknown_raw` | retour des 16 champs bruts uk* sans interprétation | M |
| L06 | `mems_manager_rosco_decode_variant_dtc` | décodage DTC selon variante documentée | M |
| L07 | `mems_manager_rosco_timed_actuator` | séquence START/STOP à durée bornée | M |
| L08 | `mems_manager_rosco_test_fan3` | commande ventilateur 3 propre à la variante | M |
| L09 | `mems_manager_rosco_test_rpm_gauge` | commande compte-tours de test spécifique | M |

### N — MEMS 1.9 K-Line (6 ajouts)

Sources à comparer : `memsinterface_dispatch.cpp; mems19protocol.h; activation-protocole-1.9`. **État M :** code/algorithme déjà exploitable dans Manager, sans préjuger des tests ECU réels ; **état D :** fonction issue de la recherche, encore à implémenter et prouver.

| ID | Nom d'API **proposé** (pas un export actuel) | Rôle technique | État |
|---|---|---|---|
| N01 | `mems_manager_mems19_slow_wakeup` | réveil physique à 5 bauds adresse 0x16 | M |
| N02 | `mems_manager_mems19_read_sync_keys` | recevoir synchronisation / octets clés | M |
| N03 | `mems_manager_mems19_ack_keys` | valider échange clé de synchronisation | M |
| N04 | `mems_manager_mems19_detect_local_echo` | gérer écho adaptateur KKL | M |
| N05 | `mems_manager_mems19_open_full_session` | enchaîner réveil et dialogue compatible ROSCO | M |
| N06 | `mems_manager_mems19_get_injection_profile` | profil injection selon firmware détecté | M |

### J — MEMS 2J (12 ajouts)

Sources à comparer : `mems2jprotocol.h; rovermems-reference/i386-20230128/ecu-2j*.go`. **État M :** code/algorithme déjà exploitable dans Manager, sans préjuger des tests ECU réels ; **état D :** fonction issue de la recherche, encore à implémenter et prouver.

| ID | Nom d'API **proposé** (pas un export actuel) | Rôle technique | État |
|---|---|---|---|
| J01 | `mems_manager_mems2j_open_transport` | ouvrir liaison physique 10400 8N1 | M |
| J02 | `mems_manager_mems2j_fast_wakeup` | réveil rapide BREAK 2J | M |
| J03 | `mems_manager_mems2j_start_diagnostic` | démarrer session de diagnostic | M |
| J04 | `mems_manager_mems2j_authenticate` | gérer échange seed/key | M |
| J05 | `mems_manager_mems2j_keepalive` | maintenir session 2J | M |
| J06 | `mems_manager_mems2j_read_measurement` | lecture individuelle 21xx | M |
| J07 | `mems_manager_mems2j_read_cycle` | cycle des mesures 21xx | M |
| J08 | `mems_manager_mems2j_read_faults` | obtenir défauts 21 19 | M |
| J09 | `mems_manager_mems2j_decode_fault_states` | décoder masques et états, variantes | M |
| J10 | `mems_manager_mems2j_clear_faults` | envoyer demande effacement 31 CB et vérifier réponse | M |
| J11 | `mems_manager_mems2j_transaction` | échange diagnostique contextualisé avec erreurs | M |
| J12 | `mems_manager_mems2j_close_session` | clore port/session 2J | M |

### K — MEMS 3 (10 ajouts)

Sources à comparer : `mems3protocol.h; rovermems-reference/i386-20230128/ecu-3.go`. **État M :** code/algorithme déjà exploitable dans Manager, sans préjuger des tests ECU réels ; **état D :** fonction issue de la recherche, encore à implémenter et prouver.

| ID | Nom d'API **proposé** (pas un export actuel) | Rôle technique | État |
|---|---|---|---|
| K01 | `mems_manager_mems3_open_transport` | liaison 9600 8E1 | M |
| K02 | `mems_manager_mems3_initialise` | réveil dialogue MEMS3 | M |
| K03 | `mems_manager_mems3_start_diagnostic` | démarrer session spécifique 3 | M |
| K04 | `mems_manager_mems3_authenticate` | seed/key MEMS3 | M |
| K05 | `mems_manager_mems3_keepalive` | maintien session 3 | M |
| K06 | `mems_manager_mems3_read_measurement` | mesure individuelle MEMS3 | M |
| K07 | `mems_manager_mems3_read_cycle` | cycle mesures MEMS3 | M |
| K08 | `mems_manager_mems3_read_faults` | lire défauts MEMS3 | M |
| K09 | `mems_manager_mems3_clear_faults` | effacer DTC protocole MEMS3 | M |
| K10 | `mems_manager_mems3_close_session` | terminer session MEMS3 | M |

### R — RC5 SRS Airbag (7 ajouts)

Sources à comparer : `rc5protocol.h; rovermems-reference/i386-20230128/ecu-rc5.go`. **État M :** code/algorithme déjà exploitable dans Manager, sans préjuger des tests ECU réels ; **état D :** fonction issue de la recherche, encore à implémenter et prouver.

| ID | Nom d'API **proposé** (pas un export actuel) | Rôle technique | État |
|---|---|---|---|
| R01 | `mems_manager_rc5_open_transport` | ouvrir liaison 2400 8N1 | M |
| R02 | `mems_manager_rc5_wakeup` | réveil BREAK airbag RC5 | M |
| R03 | `mems_manager_rc5_ping` | confirmation réponse RC5 | M |
| R04 | `mems_manager_rc5_read_faults` | lire défauts SRS 16 bits | M |
| R05 | `mems_manager_rc5_decode_fault_detail` | texte/état présent/historique selon table | M |
| R06 | `mems_manager_rc5_clear_faults` | effacement SRS avec acquittement | M |
| R07 | `mems_manager_rc5_close_session` | clore session RC5 | M |

### S — Lucas 5AS (11 ajouts)

Sources à comparer : `lucas5asprotocol.h; lucas5asreader.h; recherches 16–31`. **État M :** code/algorithme déjà exploitable dans Manager, sans préjuger des tests ECU réels ; **état D :** fonction issue de la recherche, encore à implémenter et prouver.

| ID | Nom d'API **proposé** (pas un export actuel) | Rôle technique | État |
|---|---|---|---|
| S01 | `mems_manager_lucas5as_open_transport` | ouvrir transport dédié et configuration | M |
| S02 | `mems_manager_lucas5as_slow_wakeup` | initialisation 0x1C et réponse 55 83 76 | M |
| S03 | `mems_manager_lucas5as_read_status` | lire trame de statut 0x83 | M |
| S04 | `mems_manager_lucas5as_decode_status` | déduire états normalisés de la réponse | M |
| S05 | `mems_manager_lucas5as_get_identity` | identification/série disponible dans statut | M |
| S06 | `mems_manager_lucas5as_get_fob_state` | état emplacements télécommandes selon réponse | M |
| S07 | `mems_manager_lucas5as_pulse_output` | test sortie borné, arrêt demandé | M |
| S08 | `mems_manager_lucas5as_start_fob_learning` | commande début apprentissage | M |
| S09 | `mems_manager_lucas5as_stop_fob_learning` | commande fin apprentissage | M |
| S10 | `mems_manager_lucas5as_get_learning_state` | état logiciel éventuellement incertain | M |
| S11 | `mems_manager_lucas5as_close_session` | fermeture et état session 5AS | M |

### Q — RAM / injection / ROM / firmware (14 ajouts)

Sources à comparer : `injectionramtest.cpp; mappedinjection_profiles*.inc; MEMS_MANAGER_FUTURE_FUNCTIONS.md; ROSCO_COMMAND_CARTOGRAPHY.md`. **État M :** code/algorithme déjà exploitable dans Manager, sans préjuger des tests ECU réels ; **état D :** fonction issue de la recherche, encore à implémenter et prouver.

| ID | Nom d'API **proposé** (pas un export actuel) | Rôle technique | État |
|---|---|---|---|
| Q01 | `mems_manager_mode4_get_mode` | interroger mode courant avant lecture mémoire | M |
| Q02 | `mems_manager_mode4_enter` | ouvrir mode lecture mémoire contextualisé | M |
| Q03 | `mems_manager_mode4_exit` | restaurer diagnostic normal | M |
| Q04 | `mems_manager_mode4_select_bank` | choisir banque lecture mémoire | M |
| Q05 | `mems_manager_mode4_select_block` | choisir bloc lecture mémoire | M |
| Q06 | `mems_manager_mode4_read_ram_byte` | lire octet RAM | M |
| Q07 | `mems_manager_mode4_read_ram_word` | lire mot RAM | M |
| Q08 | `mems_manager_mode4_read_rom_byte` | lire octet ROM | M |
| Q09 | `mems_manager_mode4_read_rom_block` | lire bloc ROM | D |
| Q10 | `mems_manager_mode4_read_rom_full` | lecture ROM complète (non opérationnelle aujourd'hui) | D |
| Q11 | `mems_manager_mode4_rom_checksum` | vérifier intégrité image ROM, algorithme à confirmer | D |
| Q12 | `mems_manager_injection_transient_acquire` | acquisition temporelle AANMP002 | M |
| Q13 | `mems_manager_injection_transient_stop` | arrêt/restauration après acquisition | M |
| Q14 | `mems_manager_injection_read_firmware_profile` | lire données techniques selon cartographie MPI | M |

## Compléments de recherche à conserver SANS les compter comme fonctions vérifiées

1. **MEMS 2J** : 11 services exploratoires `13`, `31 D5`, séries `33 ...`, `23 00 07 22 01` ; deux services commentés `31 D6 / 31 D7` et un lecteur ROM commenté. Source JSON `15_CATALOGUE_MACHINE_PROTOCOLS_ROVERMEMS_I386_20261120.json`. **Nombre d'exports à définir**, car un service peut nécessiter plusieurs procédures internes ou être non pris en charge suivant la variante.
2. **Lucas 5AS** : configuration EEPROM, jumelage ECU/antidémarrage, association de clés avec preuve matérielle, programmation selon matériel. La couche diagnostics et apprentissage issue de Session 200 est dans les 11 propositions S ; **pas** ces écritures encore non établies.
3. **Lucas 10AS, 8AS et Pektron SCU** : autres familles, archives et sources d'analyse mais pas de pilote complet démontré. Nécessitent étude distincte.
4. **ROSCO Mode 4 / ROM écriture** : opérations potentiellement destructives `D1/D3/F8`, calibration et remapping ; rester documentées sans fonctions de programmation activables automatiquement.
5. **Autres commandes** : actionneurs dépendant de la variante, `0x0F` conflictuel, `7E/7F/82/FF/6D/F9` et 16 champs inconnus. Leurs octets ne justifient pas **16 nouveaux exports** : les mesures brutes sont déjà lues.

## Règles avant implémentation

- **Pas de duplication aveugle** des 91 exports déjà présents. Conserver chaque nom, signature, ordinal et le fonctionnement original de Manager 200 ; les nouveaux exports commencent après 91.
- L'objet de cet inventaire est **tout le moteur ECU**. Pas de fenêtres, IA, documentation ni graphismes dans la DLL.
- Toute fonction qui modifie un ECU doit avoir une famille/un mode explicite, un statut de résultat fidèle, et ne pas être lancée pendant une simple connexion. « Code compilé » ou « octets envoyés » ≠ validation matérielle.
- Un même code peut changer de sens selon mode/famille (notamment `0x0F`, `0xFA`, `0xAF`, `0x81`, `0xD1`, `0xD3`, `0xF7`, `0xF8`). Ne jamais requalifier les fonctions historiques de Session 200.
- **Aucune modification de Session 200**, pas de publication ni compilation pendant ce chiffrage documentaire.

## Dossiers sources contrôlés

- [Rapport d'inventaire technique complet](INVENTAIRE_EXHAUSTIF_ECU_DLL_SESSION200_20261123.md)
- [Contrat vérifié des 91 exports](mems_manager_x64.def)
- [Matrice CSV des 91 exports](MATRICE_91_EXPORTS_ET_CONTRAT_SESSION200_20261123.csv)
- [Sources de recherche 1.9, 2J, 3, RC5, Lucas 5AS](../research/mems/lecture-ecu-19-2j-3/)
- [Source officielle Session 200](https://github.com/mini56/ECU-Mems-Manager-Session/tree/3737dffa3e30e0b9167bda34fa661958b757cd28)
