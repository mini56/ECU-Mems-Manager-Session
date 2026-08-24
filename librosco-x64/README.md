# librosco x64 — baseline de compatibilité ECU MEMS Manager

Cette arborescence reconstruit une **première DLL Windows x64 de compatibilité** à partir des éléments vérifiés dans la DLL 32 bits réellement utilisée par ECU MEMS Manager.

Elle n’est pas encore branchée sur le programme principal. Le but de cette étape est de prouver par GitHub Actions que le contrat binaire peut être reproduit nativement en x64 sans perdre l’API existante.

## Référence binaire contrôlée

Ancienne DLL : `prebuilt-librosco/librosco.dll`

- taille : 49 672 octets ;
- SHA-256 : `a5ad466f8f1a198a6f8259a5ea5ab59775815c8cbc9637aa584db9e9c811d38f` ;
- architecture : PE32 / x86 ;
- version retournée : 0.1.12 ;
- structure `mems_data` : 60 octets ;
- 22 exports nommés.

Le header utilisé par ECU MEMS Manager est le header Haro/MEMS-Scan étendu, pas le header Colin 0.1.12 d’origine.

## Compatibilité conservée

La DLL x64 de cette étape conserve les 22 noms exportés historiques, la structure Haro de 60 octets et les commandes directement confirmées par désassemblage :

- initialisation `CA 75 F4 D0` ;
- polling `80` (28 octets) puis `7D` (32 octets) ;
- position IAC `FB` ;
- effacement défauts `CC` ;
- heartbeat `F4` ;
- reset ECU `FA` ;
- reset adjustments `0F` ;
- primitive générique `mems_test_actuator()` ;
- primitives basses lecture/écriture/envoi de commande.

## Différences volontaires de sûreté

Le chemin de succès normal reproduit le comportement utile du binaire historique. Deux comportements erronés de l’ancien code ne sont pas reproduits :

- une trame `0x7D` tronquée est refusée au lieu d’être acceptée à cause de l’ancien test de longueur mal parenthésé ;
- `clear/reset` renvoient réellement l’état de l’échange série au lieu de forcer un succès après acquisition du mutex.

Ces différences concernent uniquement les chemins d’erreur et seront conservées dans le rapport de validation afin qu’elles ne soient jamais confondues avec une différence de protocole.

## Ce qui n’est pas encore fait

- la DLL x64 n’est pas encore utilisée par l’exécutable ECU MEMS Manager ;
- les commandes supplémentaires Leopold/ROSCO ne sont pas encore promues au rang de commandes validées ;
- les familles MEMS 1.2 / 1.3 / 1.6 / 1.9 ne sont pas encore implémentées comme profils dans cette bibliothèque ;
- Mode 4/RAM et programmation restent séparés du polling normal.

La prochaine validation doit être entièrement automatisée : compilation AMD64, tests de tailles/offsets, contrôle PE32+, contrôle des 22 exports et publication d’un artefact de test.
