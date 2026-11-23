# Journal de développement DLL — Session MEMS 2J complète — référence Manager 205

**Date : 23 novembre 2026 — branche unique `dll-manager`. Aucun changement du programme Manager 205, de son Setup officiel ou des mises à jour.**

## Référence figée et périmètre

- **Source de vérité** : [Manager Session 205, commit `b5cd9b61bbaed6251f17559885481b1bd03f4380`](https://github.com/mini56/ECU-Mems-Manager-Session/tree/b5cd9b61bbaed6251f17559885481b1bd03f4380), fichier `mems2jprotocol.h` (transport, dialogue, cycle et effacement) et `mems2jprotocol_test.cpp` (vecteurs synthétiques).
- **Code DLL existant réutilisé, inchangé** : `protocol_catalogue.c/.h` (encadrement de trame `LEN PAYLOAD CHECKSUM`, clé 2J, définitions de mesures, masques bruts DTC).
- **Code DLL nouveau** : `mems2j_session.c`, `mems2j_session.h`, test déterministe `mems2j_session_selftest.c`.
- **Compatibilité absolue de l'API historique** : exports **1 à 105 conservés à l'identique**, 9 nouvelles entrées ajoutées aux ordinaux 106–114. La compilation du vrai EXE 205 et un Setup indépendant avec notre DLL constituent la validation ABI, sans aucune intégration officielle.

## Liste des neuf nouveaux exports

| Ordinal | Export réel ajouté | Fonction technique |
|---:|---|---|
| 106 | `mems_manager_mems2j_connect` | Ouverture COM 10400 8N1, réveil BREAK 25 ms, dialogue session `10 A0`, seed `27 01`, clé `27 02`, identification par réponses réelles |
| 107 | `mems_manager_mems2j_read_measurement` | Lecture diagnostique `21 XX`, réponse `61 XX` brute intégrale, métriques via codec existant |
| 108 | `mems_manager_mems2j_read_faults` | Lecture `21 19`, retour brut DTC sans imposer les libellés Mini MPi à toutes les variantes |
| 109 | `mems_manager_mems2j_clear_faults` | **Commande explicite seulement** `31 CB` + 15 octets 0, validation de la réponse `71 CB` |
| 110 | `mems_manager_mems2j_keepalive` | `3E/7E` à la demande pour conserver le dialogue |
| 111 | `mems_manager_mems2j_read_cycle` | Rotation de lectures conforme à la Session 205 : 09,19, quatre primaires et une supplémentaire |
| 112 | `mems_manager_mems2j_last_transaction` | Retour du résultat, code de refus `7F` et durée réelle mesurée |
| 113 | `mems_manager_mems2j_is_open` | État d'une session authentifiée et de son port |
| 114 | `mems_manager_mems2j_close` | Fermeture et libération du port et des ressources |

### Caractéristiques du dialogue

- **Séquence** : `81 13 F7 81 0C` **non encadrée**, réponse `C1 D5 8F`, `10 A0 / 50`, `27 01 / 67 01 SEED`, clé calculée `27 02 / 67 02`, puis `3E/7E` et `21 09 / 61 09` comme **sondes optionnelles**. Une seule mesure refusée ne doit pas annuler une vraie session établie.
- **Trames** : `LEN (1–96) PAYLOAD CHECKSUM`, somme additive, décodage contre réponses tronquées/mauvais checksum, écho KKL local éventuellement fragmenté, `7F service 78` *response pending*, négatifs `7F service code`, et réponses obsolètes d'un autre service ignorées.
- **Lecture du cycle** : `09` + `19` + 4 IDs parmi `01,03,07,08,0A,0F,10,12,13` + 1 parmi `00,02,05,06,0B,0C,0D,11,21,25,3A` ; refus ponctuel d'un capteur ne bloque pas les suivants.
- **Données** : les paquets sans conversion certaine, en particulier `21 00` et `21 06`, sont rendus **bruts**, jamais transformés en pseudo-capteurs. Les libellés DTC Mini MPi ne sont pas supposés universels.
- **Communication** : C natif Win32, **aucune dépendance Qt**, aucun accès à l'interface, à l'Aide ou à l'IA. Horodatage **de chaque TX/RX réel**, avec un callback synchrone. Le fichier journal et son affichage sont du ressort futur de Manager, non de la DLL.
- **Commandes modifiantes** : seulement `clear_faults`, explicitement invoquée ; pas d'effacement automatique lors de `connect`. Un `WriteFile` réussi ne constitue **jamais** une confirmation ECU.
- **Licences et sources** : réimplémentation d'après les sources de Session 205 déjà utilisées par le projet, sans copier les fichiers Go RoverMEMS externes dans ce lot. Vérifier les obligations des matériaux GPLv3 en cas de distribution ultérieure.

## Tests et preuves exigés

- **Test Win32 simulé** : toutes les fonctions série externes de `mems2j_session.c` sont remplacées par des simulations **dans l'exécutable de test uniquement**. Simulation de 10400 8N1, BREAK, trames entrantes fragmentées, écho présent/absent, seed/key `0x1234 → 0x8247`, réponse pending, refus, timeout, invalidité, annulation, lecture `21 19`, commande `31 CB`, rotations et traces TX/RX. **Aucun port COM réel ni calculateur réel n'est ouvert** par ce test.
- **Conservation du protocole existant** : le codec `protocol_catalogue.c` et ses tests restent ceux du lot précédent.
- **Run de validation dédiée** : [GitHub Actions n° 45251694520](https://github.com/mini56/ECU-Mems-Manager-Session/actions/runs/45251694520), commit `80ee676bff463276c20291cd62af5b1b87b4014c`.
- **Conditions de validation finale** : compilation x64, succès de tous les CTests, exactement **114 exports et leurs ordinaux**, résolution des imports du **véritable Manager 205**, puis installation/démarrage/16 pages/désinstallation d'un **Setup de test indépendant fondé sur la Release officielle 205**. Le statut doit être renseigné d'après les journaux, pas supposé.

## Prochaines étapes

Une fois ce lot logiciel validé, le prochain bloc sera **MEMS 3** avec son transport 9600 **8E1**, son protocole propre `B8 13 F7 LEN PAYLOAD XOR`, le seed/key et les défauts, sans réutiliser par erreur les trames MEMS 2J. La DLL ne sera intégrée au programme **qu'avec une autorisation distincte après finalisation**.

[Lot MEMS 1.9 précédent](JOURNAL_DEVELOPPEMENT_DLL_MEMS19_SESSION205_20261123.md) · [Inventaire global](INVENTAIRE_EXHAUSTIF_ECU_DLL_SESSION200_20261123.md) · [Chiffrage prévisionnel](CHIFFRAGE_84_FONCTIONS_DLL_SESSION200_20261123.md).
