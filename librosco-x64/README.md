# `mems_manager_x64.dll` — moteur protocolaire x64 d’ECU MEMS Manager

Cette arborescence construit la **nouvelle DLL Windows x64 native d’ECU MEMS Manager**.

Nom du binaire : **`mems_manager_x64.dll`**.

L’ancienne bibliothèque historique reste distincte :

- `librosco.dll` = DLL historique x86 / 32 bits (celle de LeopoldG est la référence fonctionnelle avec **45 exports**) ;
- `mems_manager_x64.dll` = nouvelle DLL native x64 développée pour ECU MEMS Manager.

Le sous-dossier conserve pour l’instant le nom historique `librosco-x64` et le header public `rosco.h` afin de limiter les régressions pendant la migration. Cela ne change pas le nom du nouveau binaire.

## Reconstruction complète LeopoldG (branche isolée, 21/11/2026)

**Référence prioritaire :** `LeopoldG/sources-mems-rosco/librosco.dll`, binaire PE32 de 63 161 octets, **45 fonctions exportées**, comparées directement à sa table d'exports et à ses commandes machine. Cette version est distincte de l'ancienne DLL de référence Manager à 22 exports.

- **45/45 noms historiques recréés**, ordinals 1–45 conservés. La commande derrière `mems_reset_ECU` garde volontairement la valeur historique de Manager (FA), non celle de LeopoldG (AF). Les 20 actions précédemment regroupées utilisent une fonction individuelle qui appelle la même primitive de dialogue/écho/réponse.
- **Conflit binaire vérifié :** Manager-22 utilise `mems_reset_ECU=FA` et `mems_reset_adjustments=0F`. LeopoldG-45 utilise `mems_reset_ADJ=FA`, `mems_reset_ECU=AF`, `mems_reset_EMI=0F` et `mems_Save=81`. La nouvelle DLL conserve la signification de Manager pour les fonctions déjà utilisées, et propose `mems_reset_ECU_leopold=AF` comme fonction supplémentaire explicitement nommée. Le bouton de reset existant pour les familles ROSCO doit respecter ces valeurs ; les chemins MEMS 1.9 K-Line conservent leurs propres commandes historiques `0F/FA` dans Manager, sans substitution.
- **8 exports modernes préservés ou ajoutés :** `mems_reset_adjustments` (alias vers ADJ originale), `mems_save_adjustments` (alias vers Save original), 4 transports MEMS 1.9, et 2 fonctions de correction secondaire de richesse `7B/7C`. **État de la branche `dll-manager` : 91 exports (53 historiques/étendus, 1 reset LeopoldG distinct, 11 fonctions par famille, 6 fonctions d'analyse AANMP002, 20 fonctions de trames/mesures/défauts MEMS 2J, MEMS 3 et RC5).** La fonction de réinitialisation ECU LeopoldG originale est désormais `mems_reset_ECU_leopold`; cela empêche sa confusion avec la fonction `mems_reset_ECU` déjà utilisée par Manager.
- **Correction de carburant secondaire :** `mems_fuel_trim_2_plus` / `mems_fuel_trim_2_minus`. Cette adaptation n'est PAS une commande d'activation des injecteurs ; une valeur distincte n'est pas garantie sur toutes les variantes. Les commandes ne sont pas raccordées à une action automatique de l'interface. Sur MEMS 1.9, `7C` est également utilisé à l'initialisation : ne jamais utiliser la correction secondaire sur une famille/mode non confirmés.
- **Conversion température °F** de la DLL d'origine récupérée à partir du code x87 : décalage non signé sur 8 bits par 55, multiplication 1,8, ajout 32 et troncature modulo 256.
- **Aucun échange ECU** pendant les tests automatisés. Le workflow `.github/workflows/rosco-45-exports-x64-audit.yml` compile exclusivement la DLL Windows x64, vérifie la présence physique des 53 noms avec `GetProcAddress`, le code AMD64, les constantes et l'ABI, puis conserve le binaire en artefact **non publié dans Manager**.

**Attention :** compilation et tests sur PC sans matériel ne constituent pas une preuve de compatibilité physique avec tous les calculateurs. L'interface SESSION et ses Setup publics ne sont pas modifiés par ce workflow. Pour des fonctions permanentes ou de réinitialisation, confirmation utilisateur préalable et essai sur ECU réel requis.

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

`mems_manager_x64.dll` conserve les 22 noms exportés historiques nécessaires à la compatibilité, la structure Haro de 60 octets et les commandes directement confirmées par désassemblage :

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

Ces différences concernent uniquement les chemins d’erreur et ne doivent pas être confondues avec une différence de protocole.

## Architecture de migration

Le target CMake interne reste momentanément nommé `rosco` pour que le code existant puisse être lié sans réécriture massive. La propriété `OUTPUT_NAME` impose toutefois le nom physique **`mems_manager_x64.dll`** et le fichier d’exports est `mems_manager_x64.def`.

La future bibliothèque doit rester extensible et distinguer explicitement :

- MEMS 1.2 / 1.3 / 1.6 / 1.9 ;
- mode diagnostic courant ;
- Mode 4 / RAM / calibration / programmation ;
- commandes validées, expérimentales et ambiguës ;
- transport série et logique protocolaire.

La cartographie détaillée est conservée dans `ROSCO_COMMAND_CARTOGRAPHY.md`.

## Validation automatique

Le workflow dédié `rosco-45-exports-x64-audit.yml` vérifie :

- production exacte de `mems_manager_x64.dll` ;
- PE32+ / AMD64 ;
- tailles ABI `frame80=28`, `frame7d=32`, `mems_data=60` ;
- les 45 exports originaux de LeopoldG avec leurs ordinals + les 8 exports additifs ;
- test ABI natif ;
- publication du marqueur `MEMS_MANAGER_X64_DLL_VALIDATION.txt`.

Le workflow d'intégration de Manager devra ultérieurement vérifier que l’exécutable x64 d’ECU MEMS Manager importe **`mems_manager_x64.dll`** et n’importe plus l’ancienne `librosco.dll` 32 bits.


## Correction isolée de compatibilité — projet DLL Manager (22/11/2026)

- Branche unique de DLL : `dll-manager` ; seules les sources, les tests et la documentation dans `librosco-x64/` sont modifiées.
- `mems_reset_ECU` (ordinal historique 39) transmet à nouveau `FA`, comme la DLL à 22 exports de Manager, tandis que `mems_reset_adjustments` (ordinal 46) transmet `0F`.
- `mems_reset_ADJ=FA`, `mems_reset_EMI=0F`, `mems_Save=81` et toutes les autres commandes existantes restent présentes.
- `mems_reset_ECU_leopold=AF` est ajouté en ordinal 54, sans changer les 53 noms et ordinals antérieurs.
- Le test série simulé vérifie le byte transmis et les erreurs de réponse de chaque réinitialisation; le test d'ABI vérifie les exports réels après compilation Windows.
- Cette étape corrige les sources de la DLL candidate **sans remplacer la DLL de SESSION 196**, sans modification du programme ou de la détection de câbles.
- **Pas de confirmation matérielle** tant qu'un essai sur calculateur réel n'a pas été réalisé. Les anciens comptes rendus de 53 exports et workflows GitHub visent les versions d'audit antérieures.

## Extension MEMS 1.2 / 1.3 / 1.6 / 1.9 et injection transitoire

La DLL inclut maintenant les nouvelles unités suivantes (sans modification du programme) :

- `family.c` / `family.h` : API pour les quatre familles indiquées, avec identification, lecture des mesures, position/déplacement IAC, défauts, adaptations, commandes et sauvegarde. Les trois familles ROSCO réutilisent les primitives historiques. MEMS 1.9 réutilise le protocole K-Line déjà présent dans la DLL après le réveil ISO 9141 effectué par le programme. Les commandes restent déclenchées explicitement ; aucune réinitialisation n'est effectuée pendant la connexion.
- `transient_injection.c` / `transient_injection.h` : six fonctions de décodage **en lecture seule** fondées sur le test AANMP002 existant dans Manager : contrôle du mode `F0`, identification `D1`, trame `80`, réponse exacte Mode 4 `0x40` (et autres mots 0x37/0x64), lecture de la paire `0x0280/0x0281`, et comparaison des mots `0x026E/0x03C8` en ticks/temps.
- **Précision** : le calcul `03C8+S(026E)` est explicitement un **candidat de comparaison** et ne devient pas une correction d'injection validée par le seul calcul. Le traitement existant de la capture (entrée/sortie Mode 4, cadence rapide, confirmation de l'ECU, restauration de session) reste intact dans Manager. La DLL n'ouvre ni ne modifie une session Mode 4 pour ces six fonctions pures.
- `serial_protocol_selftest.c` inclut les tests simulés de toutes les familles et les décodeurs d'injection. `abi_selftest.c` vérifie 71 noms d'exports physiques quand le test Windows est exécuté. `validate_protocol_guards.py` valide l'ordre des 71 exports et leurs déclarations/sources.

**Statut de validation** : code ajouté et contrôles statiques à effectuer ; les tests compilés de la DLL Windows et les échanges réels sur ECU n'ont pas été effectués par cette modification. Les fonctions primitives d'origine déjà vérifiées par l'utilisateur ne sont pas réécrites. Le Manager installé, la détection des câbles, son interface et son Setup ne sont pas modifiés.

## Extension protocoles publiés — MEMS 2J / MEMS 3 / RC5 (22/11/2026)

La présence de fonctionnalités prouvées par les outils publics RoverMEMS et MEMS Mapper (Andrew Revill) motive la reprise de protocoles diagnostiques réels, plutôt qu'une liste d'exports sans traitement.

Les nouveaux fichiers `protocol_catalogue.c/.h` ajoutent **20 véritables fonctions techniques** :

- MEMS 2J : encoder/décoder les trames avec longueur et checksum additif ; construire requêtes de mesures `21 XX`, effacement `31 CB` (préparation de la trame uniquement), clé `27 02` ; calculer la clé de défi ; identifier la nature des réponses (positive, négative, en attente) ; décoder les mesures 2J ayant une échelle documentée et consulter les bits bruts de défauts.
- MEMS 3 : trames `B8 13 F7 [LEN] [PAYLOAD] [XOR]` séparées du 2J ; requêtes `21 XX` ; réponses ; lecture des températures, pression, papillon, régime, lambda, bobines et états de défaut.
- RC5 (module airbag/SRS, pas ECU moteur) : construction et contrôle de trames, extraction des codes DTC et statut historique.
- Les formats et équations sont repris de la **documentation de protocoles et des travaux déjà consignés dans le projet**. Les sources Go de RoverMEMS sont sous licence GPLv3. Les obligations de licence doivent être étudiées avant diffusion de composants dérivés.
- Les fonctions d'encodage et d'analyse ne remplacent pas automatiquement le transport série QSerialPort actuellement utilisé par Manager : **aucune intégration ni modification de Manager**.
- `protocol_catalogue_selftest.c` teste les séquences de référence synthétiques : checksum 2J, XOR 3, messages négatifs, clé, trame RC5 et lectures mesurées. Les vecteurs de test ne sont pas présentés comme des captures de calculateurs réels.
- Le projet `CMakeLists.txt` contient la compilation du module et son exécutable de test. Les 91 noms sont spécifiés dans `mems_manager_x64.def`, tout en conservant les ordinals 1 à 71.
- **À confirmer par compilation Windows** : création effective de la DLL et exécution des tests. Aucun test matériel n'a été lancé dans ce chantier. Les fonctions de programmation/antidémarrage d'Andrew Revill ne sont pas assimilées à des lectures sans conséquence.

Références : https://andrewrevill.co.uk/MEMS3FamiliesSupported.htm et https://github.com/james-portman/rover-mems-agent
