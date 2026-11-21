# SESSION 187 — Compléments de recherche RC5 SRS, câblage, compatibilité et vérifications

**Date : 21 novembre 2026.**  
**Portée : documentation de recherche uniquement, sans changement de code, de traduction ou de workflow.**  
**Branche unique : `fix-documentary-additive-session-161-20261118`.**  
**Contexte :** le module RC5 est intégré à la première colonne de la page « Autres calculateurs » en SESSION 187 ; sa validation d'intégration Windows est distincte de sa validation avec calculateur et câble réels.

## 1. Vérification indépendante des capacités RC5

Plusieurs systèmes de diagnostic disposent réellement de la lecture des défauts RC5/EC5 :

- **RoverMEMS Agent**, logiciel avec code source disponible, prend en charge l'initialisation, les commandes de ping, la lecture des défauts et leur effacement sur le RC5 SRS.
- **Pscan** annonce le RC5/EC5 pour Mini MPI et JDM, MGF et certaines MG TF ; sa liste de compatibilité donne une limite pour les dernières TF.
- **Blackbox Solutions Faultmate SM040** documente la lecture et l'effacement de défauts, et **davantage** d'informations d'identification ECU. Sa liste de véhicules indique Rover 200, 400, Mini, MGF et certaines TF.

**Conclusion :** les fonctions diagnostic de base du RC5 ne sont pas hypothétiques. En revanche, la possibilité d'identifier la variante du calculateur avec des commandes non présentes dans le code RoverMEMS reste une recherche séparée. Ne pas inventer ces trames à partir du seul intitulé TestBook/Faultmate.

### 1.1 Contrôle du code source RoverMEMS

Fichier public actuel : https://github.com/james-portman/rover-mems-agent/blob/master/ecu-rc5.go  
Archive GitHub du projet : `research/mems/lecture-ecu-19-2j-3/rovermems-reference/i386-20230128/ecu-rc5.go`.

**Comparaison effectuée le 21/11/2026 : le fichier `ecu-rc5.go` de la branche `master` actuelle est textuellement identique à notre archive de 8 768 caractères.** Nous disposons donc de la même référence source RC5, sans correctif supplémentaire repéré dans le fichier publié.

| Opération | Requête connue | Réponse / critère |
|---|---|---|
| Initialisation | BREAK en série à 2 400 bauds, 8N1 | Réveil `55 06 3B` |
| Ping | `82 00 7D` | `C2 00 3D` |
| Lecture DTC | `82 33 4A` | Réponse dont second octet = `33`, DTC de 16 bits |
| Effacement DTC | `82 C3 BA` | `C2 C3 7A` |

La table archivée énumère **24 descriptions de codes présents** et **24 variantes historiques** (préfixes `15`/`16` et `25`/`26`). Le code spécial `0000` est étiqueté « inconnu » dans RoverMEMS : ne pas le traiter automatiquement comme « aucun défaut ».

**Point de provenance :** RoverMEMS n'effectue pas de validation de somme de contrôle explicite sur toutes les réponses de défauts, même si ses commandes fixes totalisent `FF` modulo 256. Le nouveau lecteur SESSION 187 **suppose** que toute réponse RC5 variable présente cette propriété additive et la vérifie. Cette exigence est **une hypothèse d'implémentation raisonnable, pas encore une propriété démontrée par une capture brute de réponse longue RC5**. Les tests synthétiques montrent l'auto-cohérence de cette règle, pas la conformité sur tous les boîtiers.

## 2. Nouvelle information essentielle : rythme du dialogue

Dans `ecu-rc5.go`, après réception d'un **pong**, RoverMEMS exécute un `time.Sleep(200 * time.Millisecond)` **avant** d'envoyer la requête de défauts. Le même délai de **200 ms** apparaît après la réception des défauts avant le ping suivant, ainsi qu'après confirmation de leur effacement avant la nouvelle lecture.

**Écart de la source SESSION 187 à examiner après le RUN :**
- Dans `Rc5Worker::connectCandidates`, le code déclenche immédiatement `readFaults()` après validation du ping, sans insérer les 200 ms observées dans RoverMEMS entre pong et lecture.
- Après réception des défauts, RoverMEMS repasse périodiquement par le ping ; dans SESSION 187 la lecture est déclenchée au clic/à la connexion, sans boucle de ping permanente. Cela peut être volontaire, mais la durée de maintien d'une session inactive par le RC5 n'est **pas démontrée** par la source archivées.
- Après effacement, SESSION 187 attend déjà **200 ms** avant de relire. Cette partie correspond au rythme observé dans RoverMEMS.

**Recommandation pour la prochaine correction de source propre, si nécessaire après compilation et contrôle :** comparer précisément le rythme reçu avec le cycle ping → 200 ms → lecture → 200 ms → ping de RoverMEMS et vérifier si une temporisation de 200 ms après le pong est nécessaire. Ne pas introduire de nouvelle boucle permanente ou un mécanisme générique de connexion sans preuve. Préserver les fonctions moteur.

## 3. Règles de longueur et décodage des défauts

Le code RoverMEMS déduit la taille d'une réponse DTC du premier octet par `len = header - 0xC0 + 1`. Le code Go d'origine passe ensuite **le tampon global** à `rc5ParseFaults`, avant de retirer la trame consommée. Un tampon contenant un autre paquet ou les octets de contrôle pourrait donc fausser le nombre de défauts.

**SESSION 187 a déjà fait une séparation utile :** extraction d'une seule trame complète, rejet des paquets mal formés, lecture de codes sur deux octets, distinction de l'état historique, conservation des codes inconnus. Les tests synthétiques n'excluent pas des variantes de paquets réels différentes.

**À conserver impérativement dans les vérifications matérielles :** trame hexadécimale brute complète (sans nom, VIN, codes confidentiels ni données personnelles), code DTC, état présent/historique, numéro du port série, moment du ping et de la réponse. Une réponse valide RC5 constitue la preuve de communication ; le seul fait de détecter le port USB n'est pas une preuve.

## 4. Câblage : pourquoi la reconnaissance du COM ne suffit pas

**Mini MPI, prise OBD 16 voies :**
- Ligne K **moteur MEMS 2J : broche OBD 7**.
- Ligne K **airbag RC5/EC5 : broche OBD 13**.

Ces deux lignes sont **électriquement distinctes**. La documentation de pscan confirme leur distinction. Un câble USB KKL/CH340 reconnu comme COM sous Windows peut être câblé uniquement vers la broche 7 ; Manager peut détecter son port COM, mais **aucun protocole ne peut accéder en logiciel à la broche 13 si le câble ne l'y raccorde pas**.

Sur certaines **Mini JDM**, pscan indique un **connecteur moteur à 3 broches** et un connecteur **airbag à 16 broches**, avec recours éventuel à un câble spécial. Il ne faut donc pas déduire l'accessibilité RC5 du type de moteur ou de la prise moteur.

**Distinguer deux numérotations :** Faultmate SM040 décrit la **broche 14 du connecteur du boîtier RC5** comme point de diagnostic ; cela **n'est pas** la broche 14 de la prise OBD du véhicule. Les schémas de faisceaux font la correspondance selon le véhicule.

Le site pscan explique aussi qu'un défaut de liaison K-Line peut venir du faisceau ou de l'ECU, pas seulement du logiciel. Un échec de réponse RC5 ne permet pas de conclure « calculateur absent » sans examen de la liaison.

**À valider lors d'un essai :** liaison réelle OBD 13, prise OBD et alimentation du véhicule ; aucun câblage improvisé sur circuits pyrotechniques. Ne jamais intervenir sur les connecteurs d'airbags ou de prétensionneurs pour essayer de diagnostiquer la liaison série.

## 5. Compatibilité : RC5 n'est pas tous les SRS Rover/MG

| Famille / véhicule | Éléments documentés | Conséquence SESSION |
|---|---|---|
| Rover Mini MPI (marché européen) | RC5/EC5, OBD 16 voies, SRS sur OBD 13 | Cible de validation prioritaire |
| Rover Mini JDM | RC5/EC5 documenté mais connecteurs distincts moteur/SRS | Câble adapté selon la variante |
| MG MGF | RC5/EC5 documenté par pscan et Faultmate | Diagnostic RC5 pertinent |
| MG TF **avant VIN 5D639631** | RC5/EC5 documenté par pscan | Diagnostic RC5 pertinent selon unité d'origine |
| MG TF **à partir de VIN 5D639631** | Nouveau boîtier SRS « universel » configuré via TestBook ; sources spécialisées citent **Autoliv** après bascule | **Ne pas déclarer RC5 compatible sans identification du boîtier** |
| Rover 200 / 400 | Faultmate inclut EC5/RC5, mais d'autres systèmes SRS sont montés selon variante | Vérification par équipement réel indispensable |

La source pscan `/features/mgf.html` confirme une limite explicite : **RC5EC5 sur MGF et MG TF antérieures à la référence VIN 5D639631**. Le spécialiste MGOC Spares confirme une bascule du boîtier et mentionne une configuration TestBook pour les TF ultérieures. **Il ne s'agit pas du même protocole RC5 à étendre arbitrairement**.

Sur MGF/TF antérieures, les fournisseurs spécialisés distinguent également **boîtier simple airbag conducteur** et **boîtier double airbag conducteur/passager**. Ne pas associer « absence d'une ligne de défaut passager » à une anomalie sur un modèle dépourvu de cet airbag ; le diagnostic de l'équipement installé reste nécessaire.

## 6. DTC : 24 descriptions source contre 28 types de panne annoncés

- RoverMEMS fournit 24 intitulés de défauts actuels et les 24 mêmes variantes historiques ; ce sont **24 types d'intitulés, pas forcément 48 défauts physiques différents**.
- Faultmate SM040 annonce jusqu'à **28 types de problèmes détectables**. Il ne donne pas dans la fiche publique les quatre codes et libellés supplémentaires.
- Le site de pscan annonce avoir élargi sa couverture des codes Airbag MGF et Mini MPI au-delà de listes antérieures ; il ne publie pas ici une table hexadécimale complète.

**Décision de recherche :** conserver les 24 descriptions attestées et afficher **les codes inconnus en hexadécimal**, plutôt que fabriquer quatre défauts. Rechercher un catalogue vérifiable d'autres DTC dans un manuel constructeur ou une capture authentique avant extension.

**Recoupement indépendant de code concret :** le DTC **150C** est rapporté sur une MGF dans un compte rendu pscan comme **résistance élevée du circuit d'airbag conducteur**. D'autres témoignages pscan citent **150B** comme court-circuit du circuit d'airbag conducteur à la masse. Cela conforte la table principale de RoverMEMS.

## 7. Préparation des deux autres colonnes : informations nouvelles

### 7.1 Lucas 5AS (antidémarrage/alarme)

Faultmate SM034 distingue **deux variantes** de Lucas 5AS qui n'exposent pas nécessairement toutes les mêmes fonctions. Il décrit en lecture :
- Historique des **cinq derniers événements de déclenchement d'alarme**, plutôt qu'une mémoire de DTC moteur ordinaire.
- États des entrées portes, coffre, capot, contact, commande de clé.
- Informations de configuration, numéro de série, nombre de télécommandes reconnues et informations de synchronisation.

Un **historique d'alarme** devra donc être séparé des « défauts » moteur et des défauts RC5. Éviter d'afficher « aucun défaut » lorsqu'il s'agit en fait de « aucun déclenchement mémorisé ».

### 7.2 Programmation des clés et télécommandes

- Blackbox SM034 confirme **jusqu'à quatre télécommandes PLIP** en mode d'apprentissage 5AS, contact mis.
- Andrew Revill précise que l'ouverture de la procédure d'apprentissage **supprime les télécommandes précédemment appairées**, ce qui implique de prévoir toutes les télécommandes légitimes pendant la même session.
- Les catalogues MGOC Spares distinguent selon marchés les fréquences **433 MHz (notamment Europe)** et **315 MHz (notamment Japon/Australie)** ; les télécommandes ne sont pas interchangeables par simple programmation.
- MG TF a également connu une évolution de boîtier d'alarme à **VIN 4D620549**. Cette limite **ne doit pas être confondue** avec le changement de boîtier airbag à **VIN 5D639631** : deux fonctions, deux équipements et deux numérotations différentes.

**Conséquence de conception :** au futur bloc « Clés », afficher le **type d'appairage** et la **famille de calculateur** ; ne pas confondre télécommande PLIP et transpondeur, ne pas présenter une procédure comme universelle. La programmation ne peut pas être déduite d'une simple connexion RC5.

## 8. Inventaire des vérifications utiles après RUN 187

| Priorité | Vérification | Critère |
|---|---|---|
| 1 | Build et test Windows SESSION 187 | Setup publié, autotests C++/Qt réussis ; sans assertion de fonctionnement sur véhicule |
| 2 | Compatibilité câble RC5 | Ligne K physiquement reliée à la **broche OBD 13** sur Mini MPI ; port COM et réponse ECU |
| 3 | Réveil RC5 | `55 06 3B` reçu après séquence BREAK, sans faux positif d'écho |
| 4 | Pong | `C2 00 3D` reçu en réponse à `82 00 7D` |
| 5 | Délai inter-commandes | Comparaison avec **200 ms** du RoverMEMS entre pong et requête DTC |
| 6 | Paquet de défauts réel | Taille, données, checksum, zéro/plusieurs codes 16 bits, état présent/historique |
| 7 | Commande d'effacement | Uniquement sur clic, acquittement réel `C2 C3 7A`, puis relecture |
| 8 | Familles non RC5 | Pas de tentative de commandes RC5 sur Autoliv/EC3/autres SRS, aucune régression moteur |
| 9 | 24 vs 28 types de défauts | Provenance explicite nécessaire avant toute extension du dictionnaire |

**Ne pas lancer de nouvelle compilation pour ce document de recherche.** Lorsque l'utilisateur aura contrôlé le résultat 187, les éventuels correctifs devront être effectués **dans la source, sur la branche existante, sans patch, sans recompiler pour un changement de documentation uniquement**.

## 9. Sources et provenance (consultées le 21 novembre 2026)

1. RoverMEMS Agent RC5 — code actuel, inchangé par rapport à notre archive :  
   https://github.com/james-portman/rover-mems-agent/blob/master/ecu-rc5.go
2. Documentation RC5 indépendante de RoverMEMS :  
   https://github.com/james-portman/rover-mems-documentation/blob/master/SRS/RC5/RC5-SRS-airbag-diagnostics.md
3. RoverMEMS — diagnostic RC5 annoncé, lecture et effacement :  
   https://rovermems.com/diagnostics/
4. Faultmate SM040 — RC5/EC5, liste véhicules, diagnostic 14 sur connecteur ECU, 28 types de problèmes, informations de boîtier :  
   https://www.blackbox-solutions.com/site/product?sku=SM040
5. Pscan Mini MPI/JDM — câble et connecteurs :  
   https://pscan.uk/features/mini.html
6. Pscan MGF/MG TF — limite de compatibilité RC5/EC5 :  
   https://pscan.uk/features/mgf.html
7. Pscan — liste des calculateurs :  
   https://www.pscan.co.uk/features/eculist.html
8. Forum pscan animé par son développeur — ligne OBD 7 MEMS2J / OBD 13 RC5, contrôle du signal K :  
   https://www.the-t-bar.com/forum/pscan-uk-diagnostic-tool-forum/96517-mini-mpi-k-line-fault
9. MGOC Spares — générations de boîtiers SRS, changement VIN 5D639631, airbags simples/doubles, fréquences PLIP et changement de boîtier alarme VIN 4D620549 :  
   https://www.mgocspares.co.uk/parts/mg/mgf-tf/electrics/airbag-alarm
10. MG F owner / pscan — DTC 150C, haute résistance airbag conducteur :  
    https://www.the-t-bar.com/forum/pscan-uk-diagnostic-tool-forum/99606-srs-light-on
11. Pscan — ancien défaut 150B (airbag conducteur à la masse) :  
    https://www.the-t-bar.com/forum/pscan-uk-diagnostic-tool-forum/97689-srs-warning-light
12. Faultmate SM034 — Lucas 5AS, mémoire d'alarme et PLIP :  
    https://www.blackbox-solutions.com/site/product?sku=SM034
13. Andrew Revill — 5AS diagnostics, incompatibilité câble KKL standard, effets de l'apprentissage :  
    https://andrewrevill.co.uk/MEMS3LucasImmobiliserSupported.htm
14. Rapport préparatoire précédent dans le même dépôt :  
    `research/mems/lecture-ecu-19-2j-3/16_ETUDE_PREPARATOIRE_RC5_5AS_ANTIDEMARRAGE_CLES_20261121.md`

**Niveau de confiance :** confirmé par code source (commandes RC5, temps du programme RoverMEMS, 24 codes) ; recoupé par documentation tierce (modèles, OBD 13, fonctions Faultmate et 5AS) ; **à vérifier matériellement** (timing obligatoire, checksum de trame variable, compatibilité exacte des câbles et des variantes).
