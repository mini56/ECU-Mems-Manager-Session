# Colin Bourassa — comparaison des protocoles K-Line et ROSCO

**Relevé :** 23 novembre 2026. **Source primaire :** Colin Bourassa, [Automotive diagnostic protocols](https://colinbourassa.github.io/car_stuff/diagnostic_protocols/), **publié le 23 février 2024**.  
**Source complémentaire du même auteur :** [Rover MEMS 1.6 diagnostic protocol](https://colinbourassa.github.io/car_stuff/mems_interface/), publié en 2014, mis à jour en 2015 ; déjà analysé dans [COLIN_BOURASSA_MEMS16_PROTOCOL_COMPARE_20261123.md](COLIN_BOURASSA_MEMS16_PROTOCOL_COMPARE_20261123.md).

**Nature : documentation comparative uniquement.** Les protocoles KWP-71, FIAT-9141 et Marelli 1AF de la page ne sont **pas présentés par Bourassa comme étant le protocole des calculateurs Rover MEMS 1.9, MEMS 2J ou MEMS 3**. Ne pas leur attribuer les caractéristiques de ces protocoles sans preuve distincte.

## 1. Informations explicitement données par l'article du 23/02/2024

| Observation de Bourassa | Conséquence pour notre recherche | Classement |
| --- | --- | --- |
| Les protocoles K-Line décrits utilisent **une ligne bidirectionnelle, à demi-duplex, avec niveaux 0/12 V**. | Un simple câble USB/série TTL n'est pas équivalent à un adaptateur K-Line ; conserver les transports distincts. | `PRECISION / DEJA_PRESENT` |
| **MEMS ROSCO et Lucas 14CUX** sont cités comme contre-exemples : lignes RX/TX séparées, niveaux logiques **0/5 V TTL**, pas de K-Line ni de « block exchange » dans les exemples décrits. | Ne jamais transférer au ROSCO la forme électrique K-Line et son réveil à 5 bauds. Ne pas en déduire que le protocole Lucas 5AS est celui du 14CUX. | `PRECISION` |
| Certains protocoles K-Line commencent par une **adresse envoyée à 5 bauds** suivie d'une **séquence d'octets-clés** qui précise la variante ou le protocole. | Documenter séparément : adresse de réveil, caractéristiques de la trame lente, synchronisation, clés et acquittement. | `DEJA_PRESENT / PRECISION` |
| **KWP-71, KW-82, KW-1281, FIAT-9141 et Marelli 1AF** appartiennent à une famille de *block exchange* étudiée par l'auteur. L'échange doit se poursuivre pour maintenir la session. | Pour de futurs ECU, ne pas interpréter un silence comme une simple réponse de lecture manquante ; étudier les exigences propres de maintien de session. | `NOUVEAU (PISTE DE RECHERCHE)` |
| Une phase de réveil similaire peut déboucher sur des **formats de blocs, échos, numérotations, sommes de contrôle et cadences distincts**. | Le réveil ne démontre ni l'interchangeabilité des commandes, ni celle des parseurs et des fonctions de modification. | `PRECISION / A_VERIFIER PAR FAMILLE` |
| Dans les exemples FIAT-9141 et Marelli 1AF, la phase d'initialisation lente peut employer **7 bits + parité impaire** ; selon les notes, les communications après initialisation emploient généralement **8N1**. | Ne pas coder « initialisation 5 bauds = toujours 8N1 » comme règle universelle. Cela ne change pas le 0x16 actuellement documenté pour MEMS 1.9. | `PRECISION` |
| Le protocole **Marelli P8**, cité séparément, peut envoyer les mots-clés spontanément au démarrage et fonctionner ensuite par requête/réponse, sans *block exchange*. | Un préambule de clés ISO n'implique pas automatiquement un échange cyclique de blocs. | `PRECISION` |

### Exemples de la table de Bourassa — comparaison, **pas paramètres Rover MEMS**

| Champ de l'article | KWP-71 | FIAT-9141 | Marelli 1AF |
| --- | --- | --- | --- |
| Débit nominal | 9 600 bauds | 4 800 bauds | 4 800 bauds |
| Initialisation lente | 8 bits, sans parité | 7 bits, parité impaire | 7 bits, parité impaire |
| Nombre d'octets-clés | 3 : `55 00 81` | 6 | 6 |
| Réponse à l'octet-clé | complément de l'octet à l'indice 2 | complément de l'octet à l'indice 2 | échange `SELECT` plutôt qu'écho |
| Temporisation avant reconnexion (exemple) | 250 ms | 2 000 ms | 500 ms |
| Intervalle entre blocs (exemple) | 5–250 ms | 2–500 ms | 2–200 ms |
| Charge utile maximale (table de l'auteur) | 252 octets | 29 octets | 16 octets |
| Fin de bloc | `03` | somme 8 bits | somme 16 bits |
| Compteur de séquence | oui | non | non |

Ces chiffres s'appliquent **aux exemples exacts de l'article**. Aucun de ces délais, mots-clés, contrôles de somme ou limites de taille ne doit être transplanté dans ROSCO, MEMS 1.9, MEMS 2J, MEMS 3 ou Lucas 5AS.

## 2. Comparaison au code actuel d'ECU MEMS Manager SESSION 205

| Famille dans Manager | Constat vérifié en source | Rapport à l'article |
| --- | --- | --- |
| ROSCO 1.3/1.6 | `librosco-x64/setup.c` initialise le port en **9 600 bauds, 8N1** ; `librosco-x64/protocol.c` garde `CA/75/F4/D0` et les lectures `80/7D`. | Distinct des exemples K-Line ; ne pas ajouter un réveil à 5 bauds. |
| MEMS 1.9 | `mems19protocol.h` prépare l'adresse **`0x16` à 5 bauds**, 10 bits de 200 ms ; `memsinterface_dispatch.cpp` lit **`55` + deux clés**, émet le complément de la seconde et recherche la confirmation **`E9`**. Puis `librosco-x64/protocol.c` utilise le dialogue adapté et traite les échos locaux. | Le principe « *slow init* puis clés » est cohérent ; **la table KWP-71 / FIAT / Marelli n'est pas une spécification MEMS 1.9**. |
| MEMS 2J | `mems2jprotocol.h` met en œuvre une initialisation K-Line **rapide** à 10 400 bauds et ses propres trames/contrôles. | Ne pas substituer le réveil 5 bauds de KWP-71 au réveil 2J. |
| MEMS 3 | `mems3protocol.h` définit une session séparée **9 600 bauds, 8E1** avec ses propres échanges. | Ne pas assimiler les 7 bits + parité de la table de Bourassa au MEMS 3. |
| Lucas 5AS | Non décrit par cet article. **Lucas 14CUX ≠ Lucas 5AS**. | Aucun nouveau protocole 5AS établi ici. |

**Attention aux deux jeux d'octets souvent confondus :** `55 00 81` dans la table **KWP-71** de Bourassa ne remplace **pas** la réponse de réveil MEMS 1.9 typiquement documentée `55 76 83`. Pour MEMS 1.9, l'adresse `0x16` et la réponse `E9` proviennent de **références MEMS 1.9 distinctes**, pas de la table comparative du 23/02/2024.

## 3. Éléments à conserver pour les essais matériels futurs

1. **MEMS 1.9 :** enregistrer le réveil `TX-5BAUD 16`, les octets `RX` de synchronisation et clés, l'acquittement, `E9`, l'identification et les premiers échanges `80/7D`. Contrôler également les doublons d'échos propres au câble. Le journal existant permet de retrouver ces étapes ; la seule ligne « TX-5BAUD » ne prouve **pas** électriquement la forme du signal sur la K-Line.
2. **Câbles :** distinguer l'identifiant du circuit USB (FTDI, CH340…) de la **couche électrique** du câble raccordé à l'ECU. Le code détecte l'adaptateur USB, mais la réussite des octets d'initialisation doit confirmer le dialogue réel.
3. **Temporisations :** si un futur ECU échoue, relever la réponse réelle, la durée inter-octets et la phase exacte avant de retoucher un timeout ; ne pas adopter les chiffres de la table des autres calculateurs.
4. **Futurs calculateurs K-Line :** étudier individuellement besoin de *block exchange*, écho inversé, parité lente, octets-clés, maintien de session, contrôle de trame et conditions de reconnexion.
5. **ROSCO et 5AS :** protéger le fonctionnement existant ; ne pas inférer un nouveau câble, un nouveau brochage, une commande de programmation ou un protocole Lucas 5AS à partir de la page générale.

## 4. Provenance, limites, décision

- **Source directe consultée le 23/11/2026 :** https://colinbourassa.github.io/car_stuff/diagnostic_protocols/ — *Automotive diagnostic protocols*, publication **2024-02-23**, tableau comparatif et notes.
- **Source liée, déjà analysée séparément :** https://colinbourassa.github.io/car_stuff/mems_interface/ — protocole ROSCO **MEMS 1.6**, historique 2014–2015.
- **Références internes :** `memsinterface_dispatch.cpp`, `mems19protocol.h`, `mems2jprotocol.h`, `mems3protocol.h`, `librosco-x64/setup.c`, `librosco-x64/protocol.c`, `research/mems/lecture-ecu-19-2j-3/11_TRAMES_CALCULS_ET_PREVALIDATION_19_2J_20261119.md`, `research/mems/COLIN_BOURASSA_MEMS16_PROTOCOL_COMPARE_20261123.md`.
- **État :** étude statique/documentaire. **Aucune validation de nouveaux ECU réels** apportée par cette page. Les fonctionnalités MEMS 1.9, 2J, 3 et Lucas 5AS ne sont ni certifiées ni élargies par ce relevé.
- **Décision :** archiver les connaissances dans `research/mems`, **aucune modification des commandes, de la DLL, des bases, du câblage, du Setup ou du workflow CI ; aucune compilation**.
