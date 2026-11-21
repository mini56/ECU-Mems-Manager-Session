# Rapport 26 — Lucas 5AS : piste Log Andrew Revill, preuve « ROSCO modifié » et comparaison avec Manager

**Date : 21 novembre 2026**  
**Dépôt :** `mini56/ECU-Mems-Manager-Session` ; branche conservée `fix-documentary-additive-session-161-20261118`.  
**Objet :** continuer spécifiquement la recherche **Andrew Revill / Lucas 5AS Tools / Log** pour comprendre les octets échangés entre le réveil `1C` → `55 83 76` et la première lecture normale `83`.  
**Nature :** recherche/documentation seulement ; aucun changement du moteur, de l'antidémarrage ou du RC5, aucun run lancé pour cette étude.

## 1. NOUVEAU FAIT IMPORTANT : Andrew Revill classe explicitement le 5AS comme « modified ROSCO »

Une autre publication **primaire** d'Andrew Revill, distincte de la documentation du logiciel, traite d'un **OBDII Pairing Pod** qu'il a réalisé autour d'un Arduino Pro Micro / ATmega32U4.

**Publication du développeur :** https://andrewrevill.co.uk/MEMS&5ASOBDIIPod.htm  
**Discussion originale :** https://www.caterhamlotus7.club/forums/topic/279789-obdii-pods-ecu-immobiliser-key-pairing-and-much-more/

Andrew explique **expressément** que sa bibliothèque interne distingue :
- **MEMS 1.9** : protocole **ROSCO** ;
- **Lucas 5AS** : **« modified ROSCO »**, avec voie de réponse **L-Line inversée** et adaptation de cette inversion par l'électronique du boîtier ;
- **MEMS 2J et MEMS 3** : messages **ISO 14230 / KWP2000** formatés et validés.

L'appareil contient des circuits capables d'émettre sur K et de recevoir sur K ou sur la L inversée ; ce n'est **pas** le simple équivalent d'une interface FT232R USB–TTL à connecter au 5AS.

### Conséquence pour nos recherches

Ce qualificatif oriente le raisonnement vers la **famille historique ROSCO**, déjà étudiée et intégrée à Manager pour certains calculateurs. Il ne prouve **pas** que la chronologie de réveil, les commandes ou leurs réponses soient identiques :
- aucune transition `55 83 76 → ACK → 83` n'est explicitée dans cet article ;
- il n'indique pas non plus la nécessité d'envoyer `89` ou `E3` (compléments ISO 9141 hypothétiques) ;
- le firmware ou la bibliothèque spécifique du Pod n'est pas publié dans la page examinée ;
- la transmission USB-C du Pod ne représente pas automatiquement un pilote COM compatible avec Manager.

**Nouvelle priorité de comparaison :** analyser le transport ROSCO comme point de départ, mais **ne réutiliser aucun octet par analogie sans trace 5AS**.

## 2. Vérification contre le vrai ROSCO déjà conservé dans Manager

Sources du projet rouvertes sur la même branche :
- `librosco-x64/protocol.c`, fonction `mems_init_link()` ;
- `librosco-x64/ROSCO_COMMAND_CARTOGRAPHY.md` ;
- `librosco-x64/setup.c` ;
- `lucas5asprotocol.h` (décodeur 5AS indépendant, préparé pour la future lecture).

**Exemple de démarrage du ROSCO moteur historique dans `mems_init_link()` :**  
`CA` → `75` → `F4` → `D0` (la réponse D0 est identifiante).

**Exemple de lecture moteur ROSCO :** `80` / `7D` ; chaque famille dispose de réponses différentes.

**Démarrage Lucas 5AS selon James Portman :** `1C` émis à **5 bauds** → `55 83 76`, puis en mode normal une commande de lecture `83` répond par un paquet commençant par sa longueur.

**Interdiction de confusion :**
- Les séquences **`CA 75 F4 D0`** du moteur ROSCO **ne doivent pas** être envoyées au 5AS sans preuve dédiée.
- La commande **`83`** du 5AS **ne doit pas** devenir un alias de lecture `80` ou `7D` du moteur.
- Le `1C` à 5 bauds du réveil 5AS **n'est pas** la commande `1C` en mode normal du moteur ROSCO (dont la signification varie suivant famille/mode dans la cartographie).
- Les lignes K/L électriques et la gestion d'écho d'un KKL 5AS sont **distinctes** d'un ROSCO moteur.
- Une même famille protocolaire ne suffit pas à garantir la compatibilité d'une bibliothèque : **séparer les sessions et les parseurs**.

Cette comparaison élimine la fausse bonne idée d'importer toute la séquence `CA 75 F4 D0` dans le bloc Antidémarrage. La réutilisation éventuelle de procédures techniques doit rester au niveau du **transport générique**, et seulement après validation.

## 3. Ce que le vrai Log Lucas 5AS Tools montre et ne montre toujours pas

**Documentation d'Andrew Revill :** https://andrewrevill.co.uk/MEMS3LucasImmobiliserSupported.htm  
**Image du journal :** https://andrewrevill.co.uk/MEMS3LucasImmobiliserSupported_files/image012.png

Vérification recoupée avec le **rapport 25** :
- version **Lucas 5AS Tools 7.91**, interface **COM9** dans l'exemple ;
- colonnes d'horodatage, direction **Write / Read**, vitesse **9600**, description **Eeprom Data**, octets hexadécimaux ;
- les opérations photographiées appartiennent **au mode EEPROM**, pas au réveil diagnostique normal ;
- aucun extrait publiquement identifié dans les pages examinées n'expose la séquence **`1C` → `55 83 76` → [échange intermédiaire ?] → `83`**.

**Important :** l'onglet **Log de MEMS Mapper EU3** peut être copié, vidé, sauvegardé en texte et indique des millisecondes selon sa documentation spécifique : https://andrewrevill.co.uk/MEMSMapperEU3.htm. Lucas **5AS Tools est une application distincte** ; ne pas affirmer que les fonctions d'export sont identiques sans avoir ouvert son interface réelle.

### État des archives logicielles au 21/11/2026

| Adresse officielle | Vérification | Conséquence |
|---|---|---|
| `https://andrewrevill.co.uk/Downloads/MEMSTools.zip` | Serveur renvoie une archive avec type `application/x-zip-compressed`; le lecteur web refuse ce format binaire | Le lien du logiciel **existe** ; son contenu n'a **pas** été inspecté |
| `https://andrewrevill.co.uk/Downloads/MEMS3Tools.zip` | **404 Not Found** | L'ancien lien cité dans des annonces de 2024 est périmé |
| Téléchargement via conteneur de recherche | Échec réseau/DNS ou téléchargement indisponible dans cet environnement | **Aucune extraction ou exécution du logiciel n'a eu lieu** |

Ne pas inventer des chaînes, fonctions, boutons d'export ou captures de binaire à partir de cette archive non ouverte.

## 4. AUTRE DÉCOUVERTE CONCRÈTE : contact de l'auteur publié sur son site

La page index officielle **MEMS Tools** indique une adresse de contact publique :

- **Site** : https://andrewrevill.co.uk/MEMSToolsIndex.htm
- **Contact développeur** : `andrew.d.revill@googlemail.com`

L'utilisateur n'a pas demandé d'envoyer un message : **aucun courriel n'a été expédié**.

**Question très précise et limitée au diagnostic normal, à lui soumettre en cas de besoin :**

> Subject: Lucas 5AS Tools — normal-mode startup log and handshake  
> Hello Andrew, I am researching read-only diagnostics for Lucas 5AS with a modified KKL cable. Your Lucas 5AS Tools Log screenshot clearly shows timestamped byte traffic, and your OBDII Pod article identifies 5AS as a modified ROSCO protocol. Would you be willing to share only the first lines of the Log from an **About 5AS** identification: the 5-baud `0x1C` wake, the response `55 83 76`, any subsequent host acknowledgment (if required), and the first normal `0x83` data request/reply? I am interested in the read-only handshake, byte directions and timings, **not** in EEPROM contents, EKA/EMS codes, key data or programming commands. Does Lucas 5AS Tools offer Copy/Save of the Log? Thank you.

**Cette question est préparée, pas envoyée.** Cela permettrait d'obtenir la seule pièce manquante directement du concepteur, sans spéculer.

## 5. Critères de preuve pour l'intégration future dans Manager

N'enregistrer comme **validé** que ce qui sera démontré par une capture réelle ou un extrait de code client 5AS :

1. Émission effective du réveil `1C` à 5 bauds ; identité du câble et des lignes K/L.
2. Réception `55 83 76`, avec indication direction/timing (le brouillon `james-portman/5as` évoque 309 ms + 7 ms + 7 ms mais n'est pas un outil opérationnel complet).
3. Présence ou absence d'un ACK PC après les octets de réveil ; **octet exact si présent**, puis réponse attendue.
4. Passage à **9600 bauds**, parité et éventuel écho.
5. Requête de lecture normale `83` ; **premier octet de longueur** et paquet complet effectivement reçus.
6. Séparation matérielle et protocolaire du **RC5**, des familles moteur et des fonctions modifiantes du 5AS.

Ne jamais marquer « 5AS connecté » sur la seule présence de port COM, pas plus que déclarer une lecture réussie sur une simple réponse partielle.

## 6. Bilan factuel

**Nouvelle preuve établie :** Andrew Revill utilise la dénomination **ROSCO modifié** pour le 5AS, ce qui rapproche conceptuellement cette recherche des travaux ROSCO existants **mais ne résout pas le handshake**.

**Déjà confirmé :** existence de l'onglet Log 5AS, accès à « About 5AS », visualisation des opérations à 9600 bauds dans les captures et logiciel distribué en ZIP.

**Toujours absent :** journal de connexion **normale** montrant l'intervalle `55 83 76` → `83`. Les images publiées à ce jour montrent des échanges EEPROM et non cette phase.

**Prochaine source la plus productive :** journal lu **dès le début** d'une identification via About 5AS sur un véritable boîtier, ou réponse technique d'Andrew Revill concernant son **ROSCO modifié**.

**Contrainte respectée :** une seule branche GitHub ; rapport enregistré ; aucun autre programme, build, Setup, câble utilisateur ou ECU modifié.
