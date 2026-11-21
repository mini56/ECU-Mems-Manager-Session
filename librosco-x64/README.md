# `mems_manager_x64.dll` — moteur protocolaire x64 d’ECU MEMS Manager

Cette arborescence construit la **nouvelle DLL Windows x64 native d’ECU MEMS Manager**.

Nom du binaire : **`mems_manager_x64.dll`**.

L’ancienne bibliothèque historique reste distincte :

- `librosco.dll` = DLL historique x86 / 32 bits (celle de LeopoldG est la référence fonctionnelle avec **45 exports**) ;
- `mems_manager_x64.dll` = nouvelle DLL native x64 développée pour ECU MEMS Manager.

Le sous-dossier conserve pour l’instant le nom historique `librosco-x64` et le header public `rosco.h` afin de limiter les régressions pendant la migration. Cela ne change pas le nom du nouveau binaire.

## Reconstruction complète LeopoldG (branche isolée, 21/11/2026)

**Référence prioritaire :** `LeopoldG/sources-mems-rosco/librosco.dll`, binaire PE32 de 63 161 octets, **45 fonctions exportées**, comparées directement à sa table d'exports et à ses commandes machine. Cette version est distincte de l'ancienne DLL de référence Manager à 22 exports.

- **45/45 exports historiques recréés**, avec leurs noms exacts et ordinals 1–45. Les 20 actions précédemment regroupées utilisent une fonction individuelle qui appelle la même primitive de dialogue/écho/réponse.
- **Valeurs originales vérifiées :** `mems_reset_ADJ = FA`, `mems_reset_ECU = AF`, `mems_reset_EMI = 0F`, `mems_Save = 81`. Le bouton de reset existant pour les familles ROSCO doit respecter ces valeurs ; les chemins MEMS 1.9 K-Line conservent leurs propres commandes historiques `0F/FA` dans Manager, sans substitution.
- **8 exports modernes préservés ou ajoutés :** `mems_reset_adjustments` (alias vers ADJ originale), `mems_save_adjustments` (alias vers Save original), 4 transports MEMS 1.9, et 2 fonctions de correction secondaire de richesse `7B/7C`. **Total attendu : 53 exports.**
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
