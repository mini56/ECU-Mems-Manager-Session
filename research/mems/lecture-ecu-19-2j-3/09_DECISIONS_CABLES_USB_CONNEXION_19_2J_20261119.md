# Décisions et constats du 19/11/2026 — câbles USB, connexions MEMS 1.9 et 2J

**Dossier existant :** `research/mems/lecture-ecu-19-2j-3/` — branche `research-mems2j-rovermems-protocol-20261117`.
**Référence de l'application examinée :** ECU MEMS Manager SESSION **1.1.71 / version 171**, release `mems-session-build-171`, commit `f8782eb97db98e80c1bcfc19dfdf013a94b1ed26`.
**Portée de cette note :** consignation de décisions et de résultats observés ; **aucun code source modifié, aucune compilation, aucun Setup et aucun essai sur un ECU réel**.

## 1. Preuves apportées par les captures de l'utilisateur

L'utilisateur a branché deux adaptateurs USB **au PC uniquement** et montré, sur la page existante **« Test câble USB »** de SESSION 1.1.71, leurs propriétés lues par le programme :

| Capture de l'écran « Test câble USB » | Port constaté | Famille affichée | VID:PID | Description Windows | Usage indiqué par l'utilisateur |
| --- | --- | --- | --- | --- | --- |
| Capture n° 1 (19/11/2026) | **COM6** | **CH340** | **1A86:7523** | `USB-SERIAL CH340` | **Câble KKL à connecteur OBD 16 broches**, destiné aux communications K-Line 1.9 / 2J |
| Capture n° 2 (19/11/2026) | **COM4** | **FTDI FT232** | **0403:6001** | `USB Serial Port` | **Câble ROSCO trois fils / trois broches** pour les MEMS 1.2 / 1.3 / 1.6 |

**Constat vérifié sur les captures :** SESSION détecte déjà les ports, la famille de puce, le VID/PID et la description ; les messages indiquent respectivement « Nouveau port détecté : COM6 — CH340 » et « Nouveau port détecté : COM4 — FTDI FT232 ».

**Attribution des usages :** l'association des deux adaptateurs physiques aux câbles ROSCO et KKL provient des explications de l'utilisateur, **pas** d'une identification du connecteur par USB. Les captures ne démontrent ni le schéma interne ni une communication réussie avec un ECU. Les numéros COM sont ceux du jour, **pas des constantes à coder en dur**.

**Existant à réutiliser :** `serialadapterdetector.h` identifie déjà les familles `FTDI FT232` et `WCH CH340/CH341` avec les identifiants `VID/PID` et les descriptions. L'onglet de test USB existe ; **ne pas recréer de détecteur ou une nouvelle page**.

## 2. Règle de reconnaissance des câbles retenue pour ces deux équipements

- Pour **ces câbles connus**, VID:PID **0403:6001 / FTDI FT232** est associé par l'utilisateur au câble **ROSCO trois fils**, et VID:PID **1A86:7523 / CH340** au câble **KKL K-Line OBD 16 broches**.
- À la connexion, réutiliser les données détectées par SESSION pour choisir la **famille de communication** correspondante ; ne pas demander à l'utilisateur de connaître à l'avance le modèle de son calculateur.
- **Ne pas confondre puce et câblage** : un câble KKL peut aussi contenir une puce FTDI. Ces associations sont valables pour **les deux câbles de l'utilisateur après confirmation**, **pas une règle universelle** pour tout périphérique FTDI/CH340. Des périphériques partageant le même VID/PID ne sont pas toujours distinguables sans numéro de série propre ou confirmation préalable.
- Si un autre câble inconnu est branché, ne pas lui attribuer automatiquement un circuit ROSCO ou KKL sur le seul fabricant de sa puce. Ne pas rendre cette vérification inutilement répétitive pour les deux câbles déjà reconnus.
- Ne jamais utiliser `COM4` ou `COM6` comme identité permanente : Windows peut changer le port.
- Conserver le **fonctionnement ROSCO existant** pour les 1.2/1.3/1.6. N'ajouter ni nouveau blocage ni régression au chemin déjà fonctionnel.

## 3. Les séquences de communication ont déjà été retrouvées

| Élément | MEMS 1.9 | MEMS 2J |
| --- | --- | --- |
| Interface physique | K-Line avec câble KKL adapté | K-Line avec câble KKL adapté |
| Réglage série | **9600, 8N1** | **10400, 8N1** |
| Réveil | Adresse **0x16 à 5 bauds** (200 ms/bit) | **Fast init** : BREAK 25 ms, relâchement 25 ms |
| Début d'échange | Synchronisation `55`, octets clés ; envoi du complément du second octet ; confirmation `E9`, puis initialisation ROSCO | Initialisation `81 13 F7 81 0C` ; réponse attendue `03 C1 D5 8F 28` (charge utile `C1 D5 8F`) |
| Suite | Trames ROSCO : identification et données `D0/D1/D2/F0`, `7D`, `80` selon mode/variante | KWP2000 : requêtes `21 xx` (ex. `21 09` régime, `21 19` défauts), réponses `61 xx`, sommes de contrôle, écho et refus `7F` |
| État dans SESSION 171 | `performMems19Wakeup()` et `connectToECU()` **présents** dans `memsinterface_dispatch.cpp` ; page « Test ECU 1.9 » présente | **Aucun pilote C++ dédié 2J confirmé** dans la version 171 |
| Validation de dialogue ECU réel | **Non démontrée dans ce dossier** | **Non démontrée dans ce dossier** |

Sources protocolaires : [RoverMEMS 1.9](https://rovermems.com/mems-1.9/index.html), [RoverMEMS 2J](https://rovermems.com/mems-2j/index.html), [James Portman / ecu-19.go](https://github.com/james-portman/rover-mems-agent/blob/master/ecu-19.go), [ecu-2j.go](https://github.com/james-portman/rover-mems-agent/blob/master/ecu-2j.go), [ecu-2j-parse.go](https://github.com/james-portman/rover-mems-agent/blob/master/ecu-2j-parse.go). Sources externes utilisées comme **références techniques**, non comme code à copier sans examen de licence.

### Point précis constaté dans le code 171

`MEMSInterface::connectToECU()` essaie le chemin ROSCO historique sur les adaptateurs détectés ; faute de succès, il essaie le réveil MEMS 1.9, puis rouvre le port pour la suite ROSCO. **Il ne choisit pas encore systématiquement le protocole selon le câble physique confirmé.** La fermeture/réouverture du port après le réveil 1.9 et la tenue des temporisations doivent être vérifiées, **sans modifier maintenant ce qui fonctionne**.

Le code Go 2J de référence comporte encore des `TODO` sur le contrôle des checksums, des tailles de paquets, les réponses négatives et certains refus : ne pas les importer tels quels dans SESSION. Le pilote Windows/CH340 doit encore être validé pour `10400` bauds et BREAK **25/25 ms réels** ; l'apparition du port COM ne le prouve pas.

## 4. Fonctionnement voulu, sans imposer l'identification préalable de l'ECU

1. L'utilisateur branche son câble au PC ; **SESSION possède déjà la détection USB dans « Test câble USB »**.
2. Pour un câble **reconnu et associé à son usage**, SESSION utilise la famille de protocole correspondante : ROSCO pour le câble 3 fils, K-Line pour le KKL.
3. L'utilisateur clique sur **Connecter** ; il **n'a pas besoin de connaître à l'avance** « 1.2 / 1.3 / 1.6 / 1.9 / 2J ».
4. **Seule une réponse de communication réellement réussie** permet de conclure qu'une session de diagnostic est établie. Ensuite récupérer l'identification de l'ECU si elle est disponible ; ne pas afficher un modèle inventé.
5. Dans la famille K-Line, déterminer un chemin de découverte limité **1.9 puis éventuellement 2J** après échec propre, en respectant les initialisations distinctes et les délais ; **ordre et stratégie à tester sur ECU réel avant activation automatique**, jamais un balayage agressif indiscriminé.
6. En cas d'échec de connexion, afficher simplement l'échec et son contexte sans déclarer l'ECU défectueux. Une fois connecté, utiliser les mesures réellement disponibles, et non des valeurs supposées.

**Décision de cadrage :** l'identification du calculateur ne doit pas devenir un nouveau verrou préalable à la connexion ; inversement, l'identité USB d'une puce ne constitue pas une preuve qu'un câble peut physiquement dialoguer avec un ECU.

## 5. Intégration envisagée et non-régression

- **Conserver exactement les 15 pages actuelles** ; pas de nouvelle page de diagnostic ou de détection USB.
- Développer les lecteurs **MEMS 1.9** (à finaliser à partir du chemin existant) et **MEMS 2J** (lecteur distinct), réutilisant les pages actuelles pour les mesures, défauts, courbes, analyse et IA MEMS.
- Adapter les **informations affichées au calculateur et à l'équipement réels**. Exemple rappelé : Mini SPi/MEMS 1.6 avec un injecteur ; Mini MPi d'origine généralement MEMS 2J avec deux injecteurs. **Ne pas dupliquer artificiellement une mesure individuelle d'injecteur lorsque le protocole ne l'expose pas.**
- **Ne toucher ni à la connexion ROSCO historique ni aux fonctions qui marchent** sans justification et contrôle de non-régression. Aucune accumulation de patchs : toute future évolution devra être une modification de source propre et limitée.
- Une fois une connexion réelle établie, la lecture doit rester utile sans conditions nouvelles qui bloqueraient artificiellement l'accès aux données connues du calculateur.
- Pour l'étape de développement initiale : identification, mesures et défauts en **lecture seule** ; les opérations d'effacement, réglage et antidémarrage sont distinctes.
- Les essais sur ECU réel, les logs des requêtes/réponses, et la validation des mesures seront documentés séparément. Une compilation réussie ne prouve pas une connexion ECU.

## 6. Travail restant avant de modifier le programme

- Confirmer, si nécessaire, l'identité physique des deux câbles de l'utilisateur et la possibilité de les distinguer durablement par USB (un VID/PID générique ne suffit pas universellement).
- Vérifier la qualité du réveil 1.9 actuel et la transition entre réveil K-Line et session ROSCO.
- Vérifier sur le CH340 réel le support de `10400` bauds et les impulsions BREAK 25/25 ms, puis les réponses K-Line.
- Définir et tester le traitement des échos, erreurs `7F`, délais, checksums et longueurs pour le 2J ; construire son lecteur C++ indépendant.
- Valider le protocole 1.9 et 2J sur calculateurs réels ; vérifier explicitement que le ROSCO 1.2/1.3/1.6 reste intact.

**Décision utilisateur du 19/11/2026 : consigner ces éléments maintenant sur GitHub, mais NE PAS modifier le programme pour l'instant.**
