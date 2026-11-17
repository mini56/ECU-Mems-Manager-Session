# Étude technique MEMS 1.9, 2J et 3 — RoverMEMS

Date : 17 novembre 2026. Statut : étude du protocole, **pas un pilote intégré**, pas de confirmation de fonctionnement sur les trois ECU réels.

## Sources et licence
Application source : [james-portman/rover-mems-agent](https://github.com/james-portman/rover-mems-agent), révision fixée `b381af39b2031c80303c5262055aa0c0121dbdd4`.
Fichiers analysés : [ecu-19.go](https://github.com/james-portman/rover-mems-agent/blob/b381af39b2031c80303c5262055aa0c0121dbdd4/ecu-19.go),
[ecu-2j.go](https://github.com/james-portman/rover-mems-agent/blob/b381af39b2031c80303c5262055aa0c0121dbdd4/ecu-2j.go),
[ecu-2j-parse.go](https://github.com/james-portman/rover-mems-agent/blob/b381af39b2031c80303c5262055aa0c0121dbdd4/ecu-2j-parse.go),
[ecu-2j-faults.go](https://github.com/james-portman/rover-mems-agent/blob/b381af39b2031c80303c5262055aa0c0121dbdd4/ecu-2j-faults.go),
[ecu-3.go](https://github.com/james-portman/rover-mems-agent/blob/b381af39b2031c80303c5262055aa0c0121dbdd4/ecu-3.go), [ecu-auth.go](https://github.com/james-portman/rover-mems-agent/blob/b381af39b2031c80303c5262055aa0c0121dbdd4/ecu-auth.go),
[serialReadWrite.go](https://github.com/james-portman/rover-mems-agent/blob/b381af39b2031c80303c5262055aa0c0121dbdd4/serialReadWrite.go), [main.go](https://github.com/james-portman/rover-mems-agent/blob/b381af39b2031c80303c5262055aa0c0121dbdd4/main.go) et [LICENSE](https://github.com/james-portman/rover-mems-agent/blob/b381af39b2031c80303c5262055aa0c0121dbdd4/LICENSE).

La source Go est publiée sous **GPL-3.0**. Cette note reprend des faits techniques et non du code source copié. Ne pas incorporer les fichiers Go directement dans ECU MEMS Manager sans vérifier les obligations de licence.

## Comparaison des trois calculateurs

| ECU | Liaison | Réveil et initialisation | Encadrement | Lectures identifiées | État dans notre logiciel |
|---|---|---|---|---|---|
| MEMS 1.9 | K-Line, 9600 bauds 8N1 | Adresse 0x16 à 5 bauds, puis séquence de synchronisation 0x55 / octets-clés / complément | ROSCO ; commandes D0, D1, D2, F0, F4... | données moteur, identifiants et défauts du chemin existant | code présent, test sur véhicule nécessaire |
| MEMS 2J | K-Line, **10400 bauds 8N1** | Fast Init : BREAK 25 ms et relâchement 25 ms, puis `81 13 F7 81 0C` | longueur + payload + somme octets modulo 256 | `21 xx` / réponse `61 xx` ; défauts `21 19` | **non intégré** |
| MEMS 3 | K-Line, **9600 bauds 8E1 (parité paire)** | Commande `1A 9A` ; réponse `5A 9A` | en-tête `B8 13 F7`, longueur et contrôle **XOR** | données `21 00, 21 06, 21 0A, 21 0B, 21 21`, défauts `18 00 00 00` | **non intégré** |

Le programme source fonctionne comme service API Go pour un site web ; il ne constitue pas un composant C++ Qt intégrable tel quel.

## MEMS 2J : échanges et mesures

- Initialisation envoyée : `81 13 F7 81 0C` ; réponse recherchée : `C1 D5 8F` (dans le paquet).
- Session diagnostic : `10 A0` -> `50`; interrogation sécurité : `27 01`, réponse `67 01 ...`, puis `27 02 ...` et acceptation `67 02`.
- Maintien de session : `3E` -> `7E`.
- Lecture régime : payload `21 09`, trame complète `02 21 09 2C`, réponse commençant par `61 09`.
- Lecture défauts : payload `21 19`, trame complète `02 21 19 3C`, réponse commençant par `61 19`.
- Autres mesures : `21 01` température eau, `21 02` huile, `21 03` air, `21 07` pression MAP, `21 08` papillon, `21 0A` lambda/correction, `21 0C` injecteurs, `21 10` batterie, `21 12` ralenti. Certains index sont refusés par une Mini MPi, notamment `21 25` et selon un commentaire `21 10`.
- Conversion du régime : deux octets de données big-endian, en tr/min. Températures eau et air : (valeur 16 bits - 2732) / 10. MAP : valeur / 100. Pour les quatre injecteurs, la source fournit des valeurs **brutes** sans établir l'unité finale.
- Défauts : cartographie de **bits** par positions de réponse ; plusieurs défauts actuels, historiques, haute/basse tension. Un masque est lui-même noté ambigu dans le programme source : ne pas afficher de certitude injustifiée.

Risques source identifiés : absence de vérification systématique du checksum de réception (TODO explicite), indexation non toujours protégée sur données tronquées, réponses négatives `7F` insuffisamment prises en charge, interruptions du polling lors d'un refus.

## MEMS 3 : échanges et mesures

- Port série : `9600, 8 bits, parité paire, 1 stop` ; aucune confusion possible avec 2J (10400, sans parité).
- Initialisation `1A 9A` et reconnaissance `5A 9A`.
- Envoi des requêtes avec en-tête `B8 13 F7`, longueur, payload, contrôle XOR des octets de la trame.
- Session diagnostic `10 A0`, défi sécurité `27 01` / `27 02` ; `3E` pour garder la session.
- Requêtes de lecture : `21 00` (températures liquide, huile et air), `21 06` (MAP, papillon en mV et régime), `21 0A` (correction carburant/lambda), `21 0B` (charge des bobines), `21 21` (écart de régime).
- Défauts : requête `18 00 00 00`, réponse commençant par `58`, triplets code défaut 16 bits + statut. Le fichier comporte une table de codes et statuts mais reste à vérifier sur ECU réel.
- Conversions observées : régime deux octets big-endian ; température (valeur - 2730)/10 ; pression MAP valeur /100.
- Commande d'effacement `14 00 00` existe chez RoverMEMS, **mais ne doit pas être activée** dans notre premier module de lecture.

Risques source identifiés : absence de vérification de checksum des réponses (TODO), risques de lectures hors limites sur paquets incomplets, décodages et statuts à éprouver en conditions réelles.

## Intégration propre envisagée dans ECU MEMS Manager

Le code actuel, en C++/Qt, possède déjà un dispatcher ROSCO/MEMS 1.9 (`memsinterface_dispatch.cpp`). Ne pas le modifier indistinctement pour envoyer des commandes 2J ou 3 à toutes les familles.

1. **Sélection explicite du protocole** 1.9, 2J ou 3, ou identification sécurisée avant chaque tentative ; ne jamais envoyer une séquence non destinée au calculateur.
2. **Transport K-Line spécifique par famille** : vitesses, parité, fast-init/slow-init et contrôle de trames indépendants.
3. **Lecture seule** : identification, défauts et mesures, sans effacement, écriture mémoire, adaptation, immobiliseur ou reprogrammation.
4. **Décodage séparé** du transport : fonctions pures sur trames enregistrées, validant longueur, checksum, en-tête, champs et unités.
5. **Réponses négatives** : journaliser, marquer indisponible et continuer avec les autres champs, sans boucle de reconnexion.
6. **Tests de non-régression** des ECU déjà pris en charge, et essais sur captures réelles MEMS 2J/3 avant de déclarer le matériel compatible.
7. L'IA MEMS et la Base de données peuvent consulter les références et les résultats validés ; elles ne doivent pas inventer des mesures absentes.

**État de cette branche : documentation de recherche seulement, aucune compilation ni Setup et aucune commande transmise à un ECU.**
