# Journal de développement DLL — réveil complet MEMS 1.9 / Session 205

**23 novembre 2026 — uniquement la branche `dll-manager`. Aucune modification de Manager 205.**

## Objet

Transférer **dans la DLL** le réveil ISO 9141 à 5 bauds de la famille MEMS 1.9,
en conservant l'identité de poignée Win32 COM pendant le dialogue, et préserver
les **100 exports déjà présents**. Le programme Manager 205 reste le **binaire
officiel immuable**, utilisé exclusivement en référence pour les tests en copie
isolée sur un runner GitHub Actions.

**Source technique d'autorité :**
- Manager officiel Session 205, commit `b5cd9b61bbaed6251f17559885481b1bd03f4380`.
- `mems19protocol.h` (source de réveil 0x16, complément des clés, détection 0xE9).
- `memsinterface_dispatch.cpp` lignes 180–300 (voie **MEMS_NATIVE_KLINE_19** à poignée persistante), 518–560 (réveil suivi de `mems_init_link_kline19`).
- `librosco-x64/protocol.c` : primitive historique d'initialisation `CA/75/F4/D0` déjà exportée et laissée inchangée.

## Cinq nouvelles fonctions exportées

| Ordinal | Nouvelle fonction | Rôle |
|---:|---|---|
| 101 | `mems_manager_mems19_slow_bit_high` | Bit ISO 9141 de l'adresse `0x16`, start + huit données LSB-first + stop |
| 102 | `mems_manager_mems19_key_complement` | Extraction de la deuxième clé ISO 9141 après `0x55`, inversion du byte ACK |
| 103 | `mems_manager_mems19_has_address_confirmation` | Recherche du complément adresse `0xE9` |
| 104 | `mems_manager_mems19_wakeup` | **Port COM déjà ouvert** : ralenti 2 s, 10 bits à 200 ms, lecture clés 2 s, ACK, confirmation 1,6 s, écho KKL |
| 105 | `mems_manager_mems19_connect_identify` | Ouvre COM + réveil + initialisation `CA/75/F4/D0` sur **le même HANDLE**, retourne l'identité de 4 octets |

**Cible ABI : 105 exports**, soit les 91 historiques, neuf Lucas 5AS et cinq MEMS 1.9, **strictement append-only**. Aucune des signatures, dispositions de structures ou valeurs `0x0F`/`0xFA` des contrats utilisés par Manager 205 n'a été modifiée.

**Sources DLL :** `mems19_wakeup.h`, `mems19_wakeup_codec.c`, `mems19_wakeup.c`.
**Tests DLL nouveaux :** `mems19_wakeup_codec_selftest.c` et `mems19_wakeup_selftest.c` (faux ports Win32 et horloge, aucun matériel).

## Précautions de fonctionnement

- **Aucune intégration dans le programme Manager** : les appels 101–105 restent disponibles pour une intégration future, mais **le bouton Connecter de Manager 205 reste inchangé**.
- Ne pas fermer ni rouvrir le COM entre le `0xE9` et `CA`. Les sources Manager expliquent le risque d'effacement de la session ECU.
- Le code initial doit accepter des clés ISO 9141 variables : `55 76 83` est un exemple fréquent, non une valeur imposée. Le complément de `0x83` est `0x7C`.
- Garder le diagnostic d'écho physique KKL indépendamment de l'écho ECU. La DLL retourne le booléen `local_echo`.
- La nouvelle fonction complète `connect_identify` ne doit retourner vrai qu'après réception réelle de l'identification `D0`, pas après ouverture d'un port USB.
- Les fonctions mutantes (réglages, effacement DTC, actionneurs, écriture ROM/EEPROM, apprentissage 5AS) **ne sont pas déclenchées** par ces nouvelles API.
- En cas d'échec de `connect_identify`, le port ouvert est fermé et l'erreur différenciée ; le réveil sur port déjà ouvert laisse la propriété du port à l'appelant.
- Annulation par rappel optionnel, vérifiée pendant les longues attentes.
- **Non validé matériel** : les contraintes des pilotes USB et de la K-Line sur véhicule ne sont pas déduites des tests fictifs.

## Validation prévue et en cours

[Run GitHub n° 45246024037](https://github.com/mini56/ECU-Mems-Manager-Session/actions/runs/45246024037), commit `8dcb88ec994a94ac001e4991b9d19a2ab29ff579`.

**État à compléter par les logs : tests compilés, nombre de CTests, 105 exports, imports Manager 205, UI 16 pages, véritable Setup 205 installé en copie isolée puis désinstallé.** Ne pas interpréter un statut en cours comme une réussite.

Le workflow de la DLL seul cible le commit officiel `b5cd9b61...`, avec `validate_session205_binaries.py` actualisé à **105 noms/ordinaux physiques**. Les 15 imports du binaire Manager seront recomptés directement sur le vrai PE 205. **Aucune Release ni publication MAJ**.

## Étape suivante après validation

Poursuivre le moteur ECU par les sessions MEMS 2J/3 et le transport commun,
sans modifier Manager. Les nouvelles fonctions doivent toujours passer les
tests ABI et les essais logiciels sur le véritable Session 205.

[Journal du lot Lucas 5AS précédent](JOURNAL_DEVELOPPEMENT_DLL_SESSION205_20261123.md) · [Inventaire de référence](INVENTAIRE_EXHAUSTIF_ECU_DLL_SESSION200_20261123.md).
