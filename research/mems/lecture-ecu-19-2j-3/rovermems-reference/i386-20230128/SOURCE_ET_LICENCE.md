# Source RoverMEMS correspondant au binaire Windows i386 — archive intégrale

**Nature : sources Go et fichiers de compilation historiques archivés à titre de référence, non utilisés dans la compilation de SESSION.**

- Dépôt auteur : https://github.com/james-portman/rover-mems-agent
- Révision Git **exacte** inscrite dans l'exécutable de l'utilisateur : `ed9ec4dd3181e60dcb7e4bcb340016fcb6bf20f3`.
- Exécutable inspecté : `rover-mems-windows-i386.exe`, Go `go1.19.5`, `GOOS=windows`, `GOARCH=386`, `vcs.modified=false`, construit le **28 janvier 2023**.
- Archive du binaire : `rover-mems-windows-i386.zip`.
- SHA-256 ZIP : `4283c2b10064246310314f0ab5cf4600b0977c932ffc64fbdba4c022f487824d`.
- SHA-256 EXE : `0b7ac753299ec7feae6b1e2793ce7c994313ee8ffa58371476c8f9db08e6fb87`.
- **Licence : GNU GPLv3** : le fichier original `LICENSE` est archivé dans ce même dossier ; respecter attribution, source et conditions de redistribution.

**Tous les fichiers Go originaux de la révision étudiée sont conservés tels quels**, plus `go.mod`, `go.sum`, scripts historiques, `README` et `LICENSE`. Les fichiers `.go` ne sont **ni appelés ni liés par CMake**.

| Fichiers principaux | Fonction |
| --- | --- |
| `ecu-1x.go`, `ecu-1x-shared.go` | MEMS 1.2/1.3/1.6, commandes et décodeurs |
| `ecu-19.go` | MEMS 1.9, réveil lent |
| `ecu-2j.go`, `ecu-2j-parse.go`, `ecu-2j-faults.go` | MEMS 2J, dialogue, mesures et défauts |
| `ecu-3.go` | MEMS 3, trames 8E1/XOR, diagnostic et données |
| `ecu-rc5.go` | RC5 airbag/SRS, dialogue et défauts |
| `ecu-auth.go`, `helpers.go` | Authentification et fonctions de construction des trames |
| `main.go`, `serialReadWrite.go`, `webserver.go`, `getSerialPorts*.go` | Sélection protocole, dialogue avec interface web, ports série et lecture |
| `build_packed.cmd`, `run-32.cmd`, `go.mod`, `go.sum` | Environnement de compilation de référence |

**Ne pas confondre** cette révision binaire du 28/01/2023 avec les six fichiers Go d'une **révision antérieure différente**, déjà présents sous `../`. Les sources existantes n'ont pas été écrasées.

Reprise : **[Rapport détaillé](../../14_RAPPORT_DETAILLE_ROVERMEMS_I386_TOUTES_SEQUENCES_20261120.md)** et **[catalogue JSON](../../15_CATALOGUE_MACHINE_PROTOCOLS_ROVERMEMS_I386_20261120.json)**.

**Aucune intervention sur le moteur SESSION 182, ses bases ou ses workflows.**
