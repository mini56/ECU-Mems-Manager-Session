# Bilan de recherche 1.9 / 2J avant intégration à ECU MEMS Manager SESSION

**19 novembre 2026 — phase documentaire (sans changement du programme).**
Branche de travail conservée : fix-documentary-additive-session-161-20261118.
Le développement et la compilation sont **en attente d'une décision utilisateur**, conformément au cadrage du chantier.

## 1. Documentation rassemblée et examinée

| Sujet | Référence vérifiée | Résultat |
| --- | --- | --- |
| MEMS 1.9 — réveil K-Line | ecu-19.go, RoverMEMS 1.9, memsinterface_dispatch.cpp Manager | 0x16 à 5 bauds, 55 + clés, complément 2e clé, E9, puis protocole ROSCO |
| MEMS 1.9 — lecture | RoverMEMS technical, ecu-1x-shared.go, sources librosco Manager | Réponses ROSCO 0x80/0x7D, champs et différences selon familles |
| MEMS 2J — dialogue | ecu-2j.go, ecu-auth.go, serialReadWrite.go, RoverMEMS | 10400 8N1, FAST INIT, initialisation spéciale, session et seed/key, requêtes 21 XX, réponses 61 XX |
| MEMS 2J — mesures/défauts | ecu-2j-parse.go et ecu-2j-faults.go, Blackbox SM072, pscan | Champ par champ inventorié, conversions calculées, anomalies et champs incertains explicités |
| Variantes | Blackbox SM072, MG F VVC, Andrew Revill, Mini Spares, pscan | Mini MPi, VVC EU2 et autres applications connues ; différences de capteurs/firmware |
| Câbles | serialadapterdetector.h, décision USB du 19/11 | CH340 KKL et FT232 ROSCO reconnus dans le PC de l'utilisateur ; compatibilité K-Line **pas prouvée par la seule détection** |

Les fichiers Go **enregistrés à l'identique** dans rovermems-reference/ sont : ecu-19.go, ecu-2j.go, ecu-2j-parse.go, ecu-2j-faults.go, ecu-auth.go, serialReadWrite.go.

Références supplémentaires **consultées mais non copiées** : 
- https://github.com/james-portman/rover-mems-agent/blob/b381af39b2031c80303c5262055aa0c0121dbdd4/ecu-1x-shared.go : boucle ecu1xLoop() appelée par ecu-19.go et décodeurs 0x80/0x7D ;
- https://github.com/james-portman/rover-mems-agent/blob/b381af39b2031c80303c5262055aa0c0121dbdd4/helpers.go : utilitaires slicesEqual/sleepUntil/timestampMs.
Ces dépendances rendent ecu-19.go **non autonome en Go** ; Manager possède déjà son propre transport ROSCO en C++/librosco, ce n'est pas un fichier obligatoire à compiler en Go pour Manager.

## 2. Vérification documentaire supplémentaire : calculs 1.9

Le Go historique partagé contient plusieurs conversions **incompatibles avec la documentation technique de James Portman lui-même**. Elles n'ont pas été validées sur le programme C++ actuel :

| Donnée | Documentation RoverMEMS (source technique) | Décodage ecu-1x-shared.go | Constat |
| --- | --- | --- | --- |
| Réponse 0x80, offset 0x09 : tension papillon | 0,02 V par unité (= brut/50) | brut/200 | divergence facteur **4**. Ex. brut 250 : doc 5,00 V, Go 1,25 V |
| Réponse 0x7D, offset 0x06 : tension lambda | 0,5 mV par unité | brut x 5 | divergence facteur **10**. Ex. brut 100 : doc 50 mV, Go 500 mV |
| Réponse 0x80, offset 0x0A : contact de ralenti | bit **4** (masque 0x10) | le Go teste masque **0x08** (bit 3) | divergence de bit |
| Réponse 0x80, offsets 0x03/0x05 | température brute - 55°C | brut - 55°C | concordance documentaire |
| Réponse 0x80, offset 0x08 | tension batterie brute/10 V | brut/10 | concordance documentaire |
| Réponse 0x80, offsets 0x01-02 | régime u16 big-endian | u16 big-endian | concordance documentaire |

**Conséquence :** ne pas importer le décodeur Go des 1.x « tel quel » pour modifier le lecteur 1.9 de Manager ; comparer aux conversions actuellement utilisées par librosco **avant toute future modification demandée**. La source technique indique elle-même que certains offsets de 0x7D sont incertains. Ce document ne prétend pas corriger ces écarts sans ECU réel.

Source : https://rovermems.com/diagnostics/technical/ ; 
https://github.com/james-portman/rover-mems-agent/blob/b381af39b2031c80303c5262055aa0c0121dbdd4/ecu-1x-shared.go

## 3. Bilan de la vérification 2J

**Établi sur documents :**
- Fast Init + 10400 bauds + echo K-Line, octets init, trames de longueur/payload/checksum ; exemples recalculés dans le dossier 11.
- Algorithme seed/key source identifié (ecu-auth.go) ; codes de session et opérations de lecture identifiés.
- **Toutes les requêtes du Go audité** recensées et formules reportées dans le dossier 10.
- Défauts 21 19 : carte Mini MPi détaillée par index/bit et statuts présents/historiques ; aucune équivalence Pxxxx universelle inventée.
- Cas d'échec connus du Go (contrôle checksum manquant, réponse 7F, longueurs et refus de commandes optionnelles).
- Erreur de signé sur 21 21 : correction mathématique identifiée mais pas appliquée au logiciel.
- Deux ambiguïtés non résolues honnêtement : [12].bit2 signifie deux défauts différents dans la source, et la métrique 21 25 n'est pas prouvée en unité réelle ; 21 10 peut être refusée par Mini.

**À ne pas affirmer sans ECU :**
- que tous les firmware 2J exposent toutes les commandes ou tous les capteurs ;
- que les champs « injector 1..4 » de 21 0C sont quatre injecteurs utilisables pour une Mini MPi ;
- que les valeurs brutes de carburant, bobine, injecteur, véhicule ou 21 25 possèdent les unités supposées ;
- que la cartographie des défauts Mini s'applique sans modification au 2J VVC ;
- que le câble CH340 KKL de l'utilisateur exécute précisément le BREAK, 10400 bauds et les échos.

## 4. Synthèse GO / NO-GO de la phase

| Jalon | État |
| --- | --- |
| Sources Go documentées, attribution/licence | **Fait** |
| Recherche sur fonctions, familles, variantes, commandes et sources externes | **Fait** pour les sources identifiées ; variantes inconnues non inventées |
| Recalcul checksums + exemples de trames/valeurs | **Fait**, avec vecteurs synthétiques (pas traces d'ECU) |
| Identification des bugs/ambiguïtés dans les sources de référence | **Fait**, consignés, aucune correction en production |
| Qualification électrique USB CH340 réel (BREAK et 10400), timings 1.9 | **À tester sur le matériel**, non réalisable par lecture GitHub |
| Réponses 1.9/2J et mesures/erreurs d'ECU réels, plusieurs firmwares | **À tester en situation**, non réalisable par documentation seule |
| Développement pilote Qt/C++ 2J et intégration UI | **NON COMMENCÉ sur demande de l'utilisateur** |
| Compilation Windows/Setup et essais de non-régression | **NON COMMENCÉS**, le programme actuel n'a pas été touché |

La **recherche documentaire est clôturable en tant que revue de l'existant** : elle a inventorié les formats, sources, unités connues et inconnues, variantes documentées, et erreurs potentielles. **Cela ne signifie PAS « tout validé matériellement »** : pour les valeurs indécidables sans ECU réel, l'état honnête est « à vérifier » ; aucune quantité de recherches générales ne peut garantir leur valeur sur un firmware spécifique.

**Avis de préparation : pas de feu vert à l'intégration sur la seule base de ce dossier** tant que la décision de démarrer le développement et le plan de validation avec les vrais câbles/calculateur n'ont pas été approuvés. Aucun verrou additionnel n'est proposé dans le programme.

## 5. Documents de travail à consulter

- [10 — MEMS 2J : variantes, unités, défauts](10_AUDIT_2J_VARIANTES_MESURES_DEFAUTS_20261119.md)
- [11 — Vérification des trames, checksums et cas limites 1.9/2J](11_TRAMES_CALCULS_ET_PREVALIDATION_19_2J_20261119.md)
- [Provenance des fichiers de référence RoverMEMS](rovermems-reference/SOURCE_ET_LICENCE.md)
- [Décision du 19/11 : reconnaissance des câbles et connexion automatique](https://github.com/mini56/ECU-Mems-Manager-Session/blob/research-mems2j-rovermems-protocol-20261117/research/mems/lecture-ecu-19-2j-3/09_DECISIONS_CABLES_USB_CONNEXION_19_2J_20261119.md)

## 6. Sources externes complémentaires

- https://rovermems.com/mems-2j/index.html
- https://rovermems.com/diagnostics/technical/
- https://tmsmini.com/old_content/cooper/BlackBox/LUCAS%20MEMS%202J%20(SM072).htm
- https://mgfcar.de/library/ENGINE_MANAGEMENT_SYSTEM_VVC_MEMS_2J.htm
- https://pscan.uk/features/mems2j.html
- https://pscan.uk/features/mems19.html
- https://andrewrevill.co.uk/MEMS3FamiliesSupported.htm
- https://www.minispares.com/blog/2026/04/03/mini-spi-and-mpi-fuel-injectionelectronic-management-systems-and-emissions/
- https://standards.iteh.ai/catalog/standards/iso/20c9a21d-a095-41d7-81fa-423494aa28e7/iso-14230-3-1999

**Ne pas assimiler « recherche écrite terminée » et « calculateurs testés ».**
