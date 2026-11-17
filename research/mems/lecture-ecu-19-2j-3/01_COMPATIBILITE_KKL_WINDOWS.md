# Recherche 01 — Interfaces KKL, ports COM Windows et protocoles MEMS

**Recherche ouverte le 17/11/2026** — analyse documentaire effectuée ; **aucun câble branché et aucun ECU testé dans cette recherche**.

## Question à résoudre

Un même câble de diagnostic K-Line USB, de type **VAG-COM KKL 409.1**, peut-il être utilisé sous Windows avec ECU MEMS Manager pour les MEMS **1.9, 2J et 3**, en respectant leurs initialisations et paramètres de ligne ?

**Réponse documentaire : oui en principe, avec un câble KKL doté d'une véritable interface K-Line et d'un pilote qui fonctionne ; la capacité de CE câble précis à transmettre correctement les signaux n'est pas encore vérifiée.**

### Compatibilité par famille

| Famille | Paramètres de communication à implémenter | Initialisation | Type de câble | Niveau de preuve |
|---|---|---|---|---|
| MEMS **1.9** | **9600, 8 bits, aucune parité, 1 stop (8N1)** | Adresse `0x16` en **5 bauds**, puis synchronisation et ROSCO | Câble KKL K-Line (prise 16 broches sur véhicule adapté) | Documenté chez RoverMEMS ; **réveil déjà codé dans notre C++**, essai ECU absent |
| MEMS **2J** | **10400, 8N1** | **Fast Init : BREAK 25 ms + relâchement 25 ms**, puis trame `81 13 F7 81 0C` | KKL K-Line ; puce FTDI recommandable pour BREAK | Documenté chez RoverMEMS, exemple Go fonctionnel chez l'auteur ; **aucun pilote C++ chez nous** |
| MEMS **3** | **9600, 8E1 (parité paire)** pour le **mode Rover/BMW** | Envoi `1A 9A`, **pas de réveil lent** dans ce mode | KKL K-Line | Documenté chez RoverMEMS et code Go ; **aucun pilote C++ chez nous** |

**Attention MEMS 3 :** Andrew Revill décrit aussi une **autre voie ISO 14230/KWP2000 à 10400 bauds avec Fast Init**. Ce sont **deux modes distincts** du MEMS 3. La première implémentation envisagée est le chemin **Rover/BMW à 9600 bauds**, tel que documenté et utilisé dans `ecu-3.go` ; ne pas fusionner son format de trame avec celui du 2J.

**Attention MEMS 1.2/1.3/1.6 :** le câble **3 broches ROSCO** historique n'est pas remplacé par la prise KKL 16 broches. La prise OBDII *physique* ne rend pas le protocole automatiquement OBD-II standard.

## Compatibilité des interfaces USB-série

| Élément | Recherche effectuée | État / prudence |
|---|---|---|
| **FTDI FT232R** | Fabricant : prises en charge de vitesses standards et non standards ; Windows VCP expose un port COM. | Très bon candidat pour `10400` bauds et BREAK, sans garantir le comportement d'une copie de puce ou d'un câble donné |
| **CH340 / CH340G** | Andrew Revill indique qu'un câble **FT232 ou CH340** convient à MEMS 1.9/2J/3 dans sa suite d'outils ; propose le pilote CH340 sous Windows. | Compatible **en principe** ; disponibilité et précision du BREAK **25 ms** à valider sur chaque modèle/pilote |
| **Pilote Windows** | Le logiciel doit détecter le port COM et vérifier chaque configuration de ligne. | Un port COM visible n'atteste **ni** la présence d'un transceiver K-Line **ni** l'aptitude au réveil de l'ECU |
| **Adaptateur ELM327** | Andrew Revill indique explicitement que l'ELM327 n'est **pas** un câble KKL équivalent. | **Ne pas utiliser comme interface transparente K-Line** pour ces lecteurs |
| **Câble reprogrammé Lucas 5AS** | Le Lucas 5AS peut nécessiter un câble RXD inversé et un adaptateur distinct. | **Ne pas le confondre** avec le KKL ordinaire des calculateurs MEMS ; ne pas modifier électriquement le câble pour ce chantier |

La compatibilité **Windows 7**, comme la capacité du matériel de l'utilisateur à générer précisément les temporisations demandées, **n'est pas démontrée** par ces seules sources. Les sources documentent les fonctions Qt/Windows, pas chaque pilote et matériel existant.

## Vérification côté ECU MEMS Manager (code de la branche source)

### MEMS 1.9 — présent

Dans `memsinterface_dispatch.cpp` :
- `performMems19Wakeup()` construit un port `QSerialPort` en **9600 8N1**, commande le signal `setBreakEnabled()`, et transmet les bits de `0x16` avec une période de **200 ms** ;
- `connectToECU()` essaie en premier le chemin ROSCO historique, puis le réveil 1.9 et le retour vers les échanges ROSCO.
- Le code signale `break-control-unavailable` si BREAK n'est pas disponible.

**Conclusion code :** l'initialisation 1.9 existe ; ni la qualité du timing réel ni la lecture sur véhicule n'ont été prouvées dans ce dossier.

### MEMS 2J et 3 — non présents

- La branche source courante ne possède **aucun pilote C++ propre** au 2J ou au MEMS 3.
- Le 2J doit demander `setBaudRate(10400)` explicitement, et vérifier `setBreakEnabled` pour la Fast Init.
- Le MEMS 3 Rover/BMW doit sélectionner **`QSerialPort::EvenParity`** à 9600 et gérer ses trames en-tête + XOR.
- Ces paramètres ne doivent pas être appliqués indistinctement à `connectToECU()` sans sélection de génération.

### Capacité Qt 5.15

La documentation Qt 5 indique que `QSerialPort::setBaudRate(qint32)` accepte **toute valeur entière positive** (dont **10400**), que `QSerialPort::setParity()` gère la parité paire et que `setBreakEnabled(bool)` permet de piloter BREAK.

**Limite essentielle :** la réussite de ces appels prouve l'acceptation par l'API/pilote, **pas** une impulsion de 25 ms réellement conforme sur la K-Line. Il faudra contrôler la communication avec un ECU et journaliser les délais.

## Points pratiques à vérifier sur le port et le véhicule

1. **Windows** : vérifier dans le Gestionnaire de périphériques le nom du port (COMx), le fabricant de la puce et la version de son pilote.
2. **Ouverture** : ouvrir le port avec `QSerialPort` sans modifier les autres ports déjà utilisés ; vérifier 9600 8N1, 10400 8N1 et 9600 8E1 **séparément**.
3. **BREAK** : vérifier si l'API accepte l'activation puis sa désactivation. Sans réponse matérielle, noter **test de signal non concluant**.
4. **Prise K-Line** : s'assurer que l'interface est **électriquement adaptée à la K-Line**, et non un simple convertisseur USB/TTL direct.
5. **Brochages / masse** : certaines prises MEMS 1.9 n'ont pas le contact de masse de signal en broche 5 ; contrôler la documentation du véhicule et le retour de masse du câble. **Ne pas shunter arbitrairement des broches**.
6. **Écho** : la K-Line renvoie généralement les octets transmis. Le logiciel doit distinguer l'écho et la réponse réelle de l'ECU.
7. **Échec de requête** : noter chaque refus `0x7F`, timeout et numéro de trame, sans répéter indéfiniment le réveil.
8. **Compatibilité ancienne** : un essai KKL ne doit jamais remplacer ni dégrader la liaison 3 broches ROSCO pour 1.2/1.3/1.6.

## Décision technique à l'issue de l'étude documentaire

**À retenir :**
- Pas d'achat de câble imposé ; on peut examiner l'interface déjà disponible au moment des essais.
- Priorité d'essai : un câble KKL à **puce FT232 authentique**, si un tel câble est déjà disponible, compte tenu des impératifs BREAK du 2J ; **CH340 à tester**, non à rejeter.
- Un même périphérique USB peut *en principe* supporter les trois familles **en changeant les paramètres par connexion**, jamais en leur envoyant toutes les initialisations à la suite.
- **Aucun branchement ni aucune compatibilité validés sur le matériel de l'utilisateur** à ce stade.

## Sources consultées (avec leur portée)

1. **James Portman, RoverMEMS, MEMS 1.9** : [paramètres, réveil, écho](https://rovermems.com/mems-1.9/index.html).
2. **James Portman, RoverMEMS, MEMS 2J** : [10400 8N1, Fast Init, comportement des câbles](https://rovermems.com/mems-2j/index.html).
3. **James Portman, RoverMEMS, MEMS 3** : [9600 8E1, absence de réveil, en-têtes](https://rovermems.com/mems-3/index.html).
4. **James Portman, code source** : [ecu-2j.go](https://github.com/james-portman/rover-mems-agent/blob/b381af39b2031c80303c5262055aa0c0121dbdd4/ecu-2j.go), [ecu-3.go](https://github.com/james-portman/rover-mems-agent/blob/b381af39b2031c80303c5262055aa0c0121dbdd4/ecu-3.go) ; vérification des réglages de port.
5. **Andrew Revill** : [familles et connecteurs](https://andrewrevill.co.uk/MEMS3FamiliesSupported.htm), [câbles FT232/CH340 adaptés](https://andrewrevill.co.uk/shop.htm), [guide des pilotes et ports COM Windows](https://andrewrevill.co.uk/CableInstructions.htm), [MEMS3 : modes Rover/BMW et ISO14230](https://andrewrevill.co.uk/MEMSMapperEU3.htm).
6. **Qt 5.15 QSerialPort** : [vitesse personnalisée, parité et BREAK](https://qthub.com/static/doc/qt5/qtserialport/qserialport.html).
7. **FTDI** : [FT232R et vitesses non standards](https://www.digikey.com/en/resources/datasheets/ftdi/ft232r) ; voir aussi [FTDI — FAQ vitesse personnalisée](https://ftdichip.com/faq/how-can-you-configure-usb-to-uart-chip-to-allow-this-to-work-in-a-special-baud-rate-2/).
8. **Exemple de limitation des prises MEMS 1.9** : [MEMSDiag — absence de broche 5 sur certains véhicules](https://memsdiag.blogspot.com/2015/01/memsdiag-what-is-memsdiag-memsdiag-is.html) (à confirmer selon véhicule).
9. **Notre propre source** : [memsinterface_dispatch.cpp](https://github.com/mini56/ECU-Mems-Manager-Session/blob/fix-original-mems-xml-fiches-20261117/memsinterface_dispatch.cpp).

**Bilan : documentation de principe = établie ; qualification de l'interface physique et validation Windows sur ce matériel = à faire.**
