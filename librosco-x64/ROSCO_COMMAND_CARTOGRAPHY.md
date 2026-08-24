# Cartographie ROSCO / MEMS — base de travail x64

Ce document n’est **pas** une simple liste des fonctions actuellement appelées par ECU MEMS Manager. Il conserve la cartographie des commandes retrouvées dans la DLL historique, les headers Haro/Leopold, les sources libres et la documentation Rover MEMS.

La règle fondamentale est : **un même octet peut changer de signification selon la famille ECU et surtout selon le mode diagnostic.** Une future API x64 ne doit donc jamais transformer cette table en liste universelle de commandes sans contexte.

## Statuts utilisés

- **BINAIRE CONFIRMÉ** : comportement observé directement dans la `librosco.dll` historique utilisée par ECU MEMS Manager.
- **CROISÉ** : présent dans plusieurs sources indépendantes cohérentes.
- **SOURCE EXTERNE** : documenté par une source technique mais non encore confirmé sur notre binaire/ECU.
- **CONFLIT** : sources ou modes attribuent des significations différentes au même octet.
- **INCONNU** : réponse observée mais rôle non établi.

## 1. Initialisation et identification en mode diagnostic normal

| Octet | Fonction / observation | Statut | Notes |
|---:|---|---|---|
| `CA` | premier octet d’initialisation normale | **BINAIRE CONFIRMÉ** | la DLL historique commence par `CA` |
| `75` | deuxième étape d’initialisation | **BINAIRE CONFIRMÉ** | suit `CA` |
| `F4` | étape init/heartbeat avec réponse supplémentaire `00` | **BINAIRE CONFIRMÉ / CONFLIT** | aussi utilisé comme changement de mode selon contexte |
| `D0` | identification ECU / software ID | **BINAIRE CONFIRMÉ / CROISÉ** | la DLL lit 4 octets après l’écho |
| `D1` | identifications supplémentaires, notamment chaîne ASCII | **CROISÉ / CONFLIT MODE 4** | ex. `ABNMP003`, mais `D1` a une autre fonction en Mode 4 |
| `D2` | statut sécurité | **SOURCE EXTERNE + observation projet** | réponses vues : `02 01`, `00 01`, `01 01` |
| `D3` | recodage ECU selon documentation normale | **SOURCE EXTERNE / CONFLIT MODE 4** | en Mode 4, `D3` sert à écrire une calibration RAM vers ROM |
| `F0` | interrogation du mode diagnostic courant | **SOURCE EXTERNE + observation projet** | exemples : `14` mode 3, `1E` mode 4, `50` mode 5/6 |
| `F6` | déconnexion / reset session diagnostic | **SOURCE EXTERNE** | ne pas confondre avec reset ECU |

Séquence historique de la DLL actuelle : **`CA -> 75 -> F4 -> D0`**.

## 2. Lecture de données normales

| Octet | Fonction | Statut | Notes |
|---:|---|---|---|
| `7D` | trame de données 7D | **BINAIRE CONFIRMÉ / CROISÉ** | 32 octets lus par la DLL historique ; MEMS 1.2 simple prise peut ne pas répondre |
| `80` | trame de données 80 | **BINAIRE CONFIRMÉ / CROISÉ** | 28 octets lus par la DLL historique |
| `FB` | position actuelle IAC | **BINAIRE CONFIRMÉ / CROISÉ** | réponse : position |
| `FD` | ouvrir IAC d’un pas | **CROISÉ** | retourne la position |
| `FE` | fermer IAC d’un pas | **CROISÉ** | retourne la position |
| `FF` | position IAC ? | **SOURCE EXTERNE / INCONNU** | à confirmer |

La structure Haro reconstruite par `mems_read()` fait **60 octets** et conserve les valeurs brutes ; ECU MEMS Manager applique ensuite ses propres décodages.

## 3. Actionneurs groupés `0x00–0x1F`

La documentation Rover MEMS montre une organisation importante : beaucoup de commandes OFF occupent `0x00–0x0F` et leur commande ON correspondante est à `+0x10`.

| OFF | ON | Fonction | Niveau de confiance |
|---:|---:|---|---|
| `00` | `10` | jauge température liquide | SOURCE EXTERNE |
| `01` | `11` | relais pompe à carburant | **CROISÉ** |
| `02` | `12` | relais PTC / chauffage collecteur | **CROISÉ** |
| `03` | `13` | relais embrayage climatisation | **CROISÉ** |
| `04` | `14` | solénoïde ralenti | SOURCE EXTERNE |
| `05` | `15` | solénoïde ORFCO | SOURCE EXTERNE |
| `06` | `16` | pulse air valve | SOURCE EXTERNE |
| `07` | `17` | vanne EGR | SOURCE EXTERNE |
| `08` | `18` | vanne purge canister | **CROISÉ** |
| `09` | `19` | chauffage sonde O2/lambda | **CROISÉ** |
| `0A` | `1A` | témoin défaut émissions | SOURCE EXTERNE |
| `0B` | `1B` | wastegate / boost valve | **CROISÉ** |
| `0C` | `1C` | fuel used | SOURCE EXTERNE |
| `0D` | `1D` | ventilateur 1 | **CROISÉ** |
| `0E` | `1E` | ventilateur 2 | **CROISÉ** |
| `0F` | `1F` | VVT selon documentation externe | **CONFLIT MAJEUR** : la DLL historique utilise `0F` comme reset adjustments |

**Important : `0x0F` est déjà un conflit réel.** La DLL ECU MEMS Manager l’envoie dans `mems_reset_adjustments()`, tandis que la documentation de commandes groupées lui attribue VVT OFF et Leopold 3.0 parle de reset émissions. La future API doit donc conserver le contexte/profil et ne jamais utiliser seulement un nom global pour `0x0F`.

## 4. Autres actionneurs et tests

| Octet | Fonction | Statut |
|---:|---|---|
| `20` | témoin température compartiment moteur OFF | SOURCE EXTERNE |
| `21` | relais désactivation cruise control | SOURCE EXTERNE |
| `30` | témoin température compartiment moteur ON | SOURCE EXTERNE |
| `31` | état opposé cruise control disable relay | SOURCE EXTERNE |
| `60` | arrêt test compte-tours / autre usage exhaust backpressure | SOURCE EXTERNE / CONFLIT |
| `61` | test admission variable | SOURCE EXTERNE |
| `63` | test compte-tours | SOURCE EXTERNE |
| `64` | test boost gauge | SOURCE EXTERNE |
| `65` | S/W throttle S.W | SOURCE EXTERNE / INCONNU |
| `67` | ventilateur 3 OFF | **CROISÉ** |
| `6B` | démarrage test compte-tours | **CROISÉ** (rover-mems-agent + Rover MEMS) |
| `6D` | fonction inconnue, réponse `6D 00` observée | INCONNU |
| `6F` | ventilateur 3 ON | **CROISÉ** |
| `DA` | test injecteur 1 MEMS 1.9 ? | SOURCE EXTERNE |
| `DB` | test injecteur 2 MEMS 1.9 ? | SOURCE EXTERNE |
| `EF` | action injecteurs MPI ? | **CROISÉ mais famille à confirmer** |
| `F7` | action injecteur SPi en mode normal | **CROISÉ / CONFLIT MODE 4** |
| `F8` | déclenchement bobine en mode normal | **CROISÉ / CONFLIT MODE 4** |

## 5. Réglages / adaptations

| Octet | Fonction | Statut |
|---:|---|---|
| `79` | fuel trim 1 + | **CROISÉ** |
| `7A` | fuel trim 1 - | **CROISÉ** |
| `7B` | fuel trim 2 + | **CROISÉ** |
| `7C` | fuel trim 2 - | **CROISÉ / CONFLIT MEMS 1.9 INIT** |
| `7E` | partie d’ajustement ralenti automatique ? | SOURCE EXTERNE / INCONNU |
| `7F` | partie d’ajustement avance automatique ? | SOURCE EXTERNE / INCONNU |
| `81` | fin/sauvegarde d’ajustements ? | SOURCE EXTERNE ; Leopold l’appelle `Save` |
| `82` | fonction inconnue, réponse structurée observée | INCONNU |
| `89` | idle decay + | **CROISÉ** |
| `8A` | idle decay - | **CROISÉ** |
| `91` | idle speed + | **CROISÉ** |
| `92` | idle speed - | **CROISÉ** |
| `93` | ignition advance offset + | **CROISÉ** |
| `94` | ignition advance offset - | **CROISÉ** |
| `F9` | ajustement map principale ? | SOURCE EXTERNE / INCONNU |
| `FA` | la DLL historique : reset ECU ; Rover MEMS : clear all adaptations ; Leopold 3.0 : reset adjustments | **CONFLIT MAJEUR** |
| `AF` | Leopold 3.0 : reset ECU | **SOURCE EXTERNE / CONFLIT** |

Pour la compatibilité actuelle, les wrappers x64 reproduisent **la DLL réellement utilisée par ECU MEMS Manager** : `FA = mems_reset_ECU` et `0F = mems_reset_adjustments`. Cette décision est une règle de compatibilité, pas une affirmation que ces octets ont la même fonction sur toutes les familles/modes.

## 6. Modes diagnostics

Documentation Rover MEMS actuellement recensée :

| Octet | Transition / fonction | Statut |
|---:|---|---|
| `9E` | premier octet init alternatif / sécurité ? | SOURCE EXTERNE |
| `C4` | passer en mode 4 depuis mode 3 | SOURCE EXTERNE |
| `CE` | init alternative / sécurité ? | SOURCE EXTERNE |
| `CF` | init alternative / sécurité ? | SOURCE EXTERNE |
| `DE` | init alternative / sécurité ? | SOURCE EXTERNE |
| `E0` | init alternative / sécurité ? | SOURCE EXTERNE |
| `E5` | init alternative / sécurité ? | SOURCE EXTERNE |
| `F0` | lire mode courant | SOURCE EXTERNE + observation projet |
| `F2` | mode 4 -> mode 6 | SOURCE EXTERNE |
| `F3` | mode 5/6 -> mode 4 | SOURCE EXTERNE |
| `F4` | mode 3 -> mode 5 | **CONFLIT avec heartbeat/init normal** |
| `F5` | mode 4/5/6 -> mode 3 | SOURCE EXTERNE |

Cela confirme que **`F4` ne peut pas être modélisé comme une fonction universelle unique**.

## 7. Mode diagnostic 4 — RAM / calibration / programmation

Ces commandes sont **séparées du polling `7D/80`**. Elles ne doivent pas être exposées comme des commandes ordinaires sans garde de sécurité et validation du mode courant.

| Commande | Fonction documentée | Risque |
|---|---|---|
| `00–7F` | lire byte/word RAM/ROM à l’offset du bloc sélectionné par `DC` | lecture mémoire |
| `80–9F` | augmenter de 1 une valeur de calibration sélectionnée | **écriture calibration** |
| `A0–BF` | diminuer de 1 une valeur de calibration sélectionnée | **écriture calibration** |
| `C1` | remettre à zéro le numéro de bloc pour écritures (blocs 0x20) | contrôle pointeur |
| `C2` | incrémenter le bloc d’écriture | contrôle pointeur |
| `C3` | décrémenter le bloc d’écriture | contrôle pointeur |
| `C5` | choisir RAM bank 1 pour chargements | sélection banque |
| `C6` | choisir RAM bank 2 | sélection banque |
| `C7` | pointeur calibration vers ROM normale | sélection banque |
| `C8` | pointeur calibration vers RAM bank 1 | sélection banque |
| `C9` | pointeur calibration vers RAM bank 2 | sélection banque |
| `D1` | écrire les deux banques RAM calibration vers ROM | **programmation ROM — dangereux** |
| `D3` | écrire une calibration RAM -> ROM | **programmation ROM — dangereux** |
| `D4` | copier une calibration ROM -> RAM (ou ROM -> ROM selon sélection) | écriture/copier calibration |
| `DC xx` | choisir bloc de lecture, blocs de 0x80 bytes | lecture mémoire |
| `F7` | streamer la calibration courante complète | **CONFLIT : injecteur en mode normal** |
| `F8` | écrire une calibration complète, ECU développement requis | **TRÈS DANGEREUX / CONFLIT : bobine en mode normal** |

**Règle d’architecture : aucune API x64 future ne doit autoriser `D1`, `D3`, `F8` Mode 4 par accident via une fonction portant le même nom que leur usage en mode normal.** Le mode courant et la famille ECU doivent faire partie du contexte de transaction.

## 8. Commandes encore non comprises ou insuffisamment vérifiées

Ont des réponses ou mentions techniques mais ne doivent pas devenir des fonctions « validées » :

- `82`
- `CB`
- `CD` (debug / lecture RAM ?)
- `E7`
- `E8`
- `E9`
- `EA`
- `EB`
- `EC`
- `ED`
- `EE`
- `F1`
- `FC`

Les réponses observées documentées incluent par exemple `CB 00`, `CD 01`, `E7 02`, `E8 05 26 01 00 01`, `ED 00`, `EE 00`, `FC 00`. Une réponse n’est pas une preuve de fonction.

## 9. MEMS 1.9 — réveil K-Line distinct

MEMS 1.9 doit rester séparé du simple chemin ROSCO 1.3/1.6 :

1. liaison 9600 8N1 ;
2. réveil ISO 9141 à **5 bauds**, adresse ECU `0x16` ;
3. réponse typique `55 76 83` ;
4. renvoi du complément du deuxième key byte (`~0x83 = 0x7C`) ;
5. confirmation comprenant `E9` (complément de l’adresse `0x16`) ;
6. ensuite seulement, reprise de la boucle de communication 1.x.

Le code actuel d’ECU MEMS Manager est meilleur que l’exemple historique sur un point : il ne fige pas `0x83`, il calcule le complément du key byte reçu et tolère les interfaces qui renvoient ou non l’écho.

**Attention : `0x7C` est donc également contextuel** : fuel trim 2 - dans la cartographie 1.x normale, mais byte de handshake spécifique dans la séquence d’initialisation MEMS 1.9.

## 10. Conséquence pour la future API librosco x64

L’API devra être organisée autour d’un contexte explicite comprenant au minimum :

- famille ECU : 1.2 / 1.3 / 1.6 / 1.9 ;
- transport / type d’interface ;
- état de session ;
- mode diagnostic courant ;
- niveau de confiance de la commande ;
- classe d’opération : lecture normale, actionneur, adaptation, RAM, calibration, programmation.

La primitive bas niveau `mems_send_command()` est volontairement conservée dans la DLL x64 de compatibilité, mais l’interface haut niveau future devra empêcher qu’une commande dangereuse change de sens silencieusement lorsqu’on change de mode diagnostic.

## Sources techniques utilisées pour cette cartographie

- DLL historique réelle `prebuilt-librosco/librosco.dll`, désassemblée et auditée dans la branche `RAPPORT` ;
- header Haro/MEMS-Scan identique au header actuel ECU MEMS Manager ;
- Colin Bourassa `librosco` 0.1.12 ;
- LeopoldG `Source-librosco` 3.0.0 et dépôts MEMS-ROSCO ;
- James Portman `rover-mems-agent`, notamment `ecu-1x-shared.go` et `ecu-19.go` ;
- Rover MEMS Technical Info : `https://rovermems.com/diagnostics/technical/`.

Les données de sources externes restent des références techniques à confirmer par ECU/famille ; elles ne remplacent pas les faits observés dans notre propre binaire ou sur une trace ECU réelle.
