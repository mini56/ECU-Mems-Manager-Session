# RAPPORT DE CONTINUITÉ — ECU MEMS MANAGER

> **RÈGLE OBLIGATOIRE** : relire ce fichier avant toute modification. Branche rapport : `RAPPORT`. Branche x64 active : `MEMSX64`.
>
> **RÈGLE QUALITÉ — AUCUNE RUSTINE** : ne jamais corriger un symptôme par un patch provisoire, une suppression de capacité, un contournement matériel ou une sélection arbitraire destinée uniquement à faire passer le build. Rechercher la cause réelle, utiliser en priorité l’architecture officiellement supportée par le composant amont et produire une solution propre, générale et maintenable.
>
> **SUIVI IMMÉDIAT** : avant chaque nouvelle étape, inscrire ici l’étape et son objectif ; après chaque résultat, l’inscrire avant la suite.
>
> **NOMMAGE UTILISATEUR** : `ECU MEMS Manager x64 #NN — Commit xxxxxxx`. `#NN` est le numéro GitHub Actions, pas le BUILD logiciel.

## ÉTAT ACTUEL — 26 AOÛT 2026

- Dépôt : `mini56/ECU-Mems-Manager-Session`.
- Branche x64 active : **`MEMSX64`**.
- HEAD x64 courant : **`8671275bc77eb1fdbdefc0b0158254efdf86df5c`**.
- Commit : **`BUILD #30 optimize native ONNX response latency`**.
- BUILD logiciel actif : **#30 / v1.0.30**.
- Aucun BUILD #31 sans demande explicite.
- 32 bits : `lab-expert-engine` — **NE PAS TOUCHER**.
- Rollback x64 : `MEMSX64-BUILD26-BASE` — **NE PAS TOUCHER**.
- Aucun changement protocole ECU pendant la stabilisation IA.
- UI dark/responsive officielle à préserver.

## DERNIER BUILD COMPLET VALIDÉ CI

### ECU MEMS Manager x64 #81 — Commit `8671275` — VERT COMPLET

- Run GitHub Actions : **`33001002041`**.
- Run number : **#81**.
- Conclusion GitHub : **SUCCESS**.
- Branche : `MEMSX64`.
- Commit complet : `8671275bc77eb1fdbdefc0b0158254efdf86df5c`.
- Artifact : **`ECU-MEMS-Manager-x64-BUILD-30-v1.0.30`**.
- Artifact ID : **`9618786882`**.
- Taille artifact : **386 739 081 octets**.
- Digest artifact : **`sha256:17f8b0d3b81ff0a7d02b964ef265b38e5746fa70ff1cf754675d9d14ced3265e`**.
- Expiration GitHub actuelle : 9 septembre 2026.

Validations #81 :
- compilation MSVC x64 : VERT ;
- tests déterministes : VERT ;
- base experte r20 : VERT ;
- téléchargement + hashes Qwen3-0.6B ONNX INT4 CPU : VERT ;
- vrai self-test `LocalAiClient` avec ONNX/Qwen : VERT ;
- assemblage du package complet : VERT ;
- validation depuis le package final : VERT ;
- smoke launch réel de MEMS Manager : VERT ;
- manifeste/hashes : VERT ;
- upload artifact : VERT.

**Important** : #81 est le premier état courant à considérer comme package ONNX complet techniquement validé par la CI. Il reste à faire le **test PC réel utilisateur**, notamment ouverture IA, temps de réponse et qualité sur questions complexes.

## ARCHITECTURE IA COURANTE

`navigationorderpatch.cpp -> IaMemsTab (vue) -> IaMemsService -> ExpertEngine + ExpertKnowledgeReader(read-only) -> LocalAiClient -> ONNX Runtime GenAI natif -> Qwen3 ONNX`

Règles :
- moteur génératif **dans le processus MEMS Manager** ;
- **aucun `QProcess` pour le moteur IA** ;
- **aucun `llama-server.exe`** ;
- **aucun serveur HTTP localhost** ;
- **aucun port 18089** ;
- chargement/génération hors thread UI ;
- ExpertEngine et ExpertKnowledgeReader conservés ;
- base experte r20 préconstruite, lecture seule ;
- mesures ECU accessibles en lecture seule au contexte ;
- aucune commande/mutation ECU accessible au LLM ;
- modèle : Qwen3-0.6B ONNX INT4 CPU ;
- runtime officiel ONNX Runtime GenAI 0.14.0 + ONNX Runtime app-local ;
- ne pas réintroduire `iamemstab_clean.cpp`, `iamemsqualitypatch.cpp`, `iaresponsecontextpatch.cpp`.

## OPTIMISATION #81

Commit : **`8671275bc77eb1fdbdefc0b0158254efdf86df5c`**.

Objectif : réduire la latence Qwen sans modifier le protocole ECU ni l’UI.

État intégré :
- suppression du mode `/think` pour les réponses normales ;
- réponses normales limitées à **128 tokens** ;
- diagnostics génératifs plus complexes limités à **192 tokens** ;
- réponses déterministes/base experte continuent d’être servies sans appel génératif lorsque la connaissance est déjà connue ;
- aucun changement de BUILD logiciel : toujours #30 ;
- aucun changement 32 bits ;
- aucun changement protocole ECU.

Le self-test CI prouve une génération Qwen réelle mais courte. Il ne suffit pas à lui seul pour conclure sur la vitesse d’un diagnostic complexe sur le PC utilisateur.

## HISTORIQUE ONNX RÉCENT — À NE PAS REFAIRE

### #74 — probe ONNX

- modèle chargé et génération réelle obtenue ;
- run rouge uniquement à cause d’un `UnicodeEncodeError` console Windows CP1252 sur emoji ;
- pas un défaut ONNX/modèle.

### #75 — probe Windows x64 — VERT

- HEAD : `bf7c6dde6f74daaf59a80f4c121870d6d8ea995e` ;
- run : `32977752270` ;
- ONNX Runtime GenAI 0.14.0 officiel Windows x64 vérifié ;
- Qwen3-0.6B ONNX INT4 CPU snapshot/hashes vérifiés ;
- chargement + génération réels VERTS ;
- artifact `ONNX-Windows-x64-Probe-BUILD-30` de 659 octets = **rapport de probe uniquement, pas l’application**.

### #79 — intégration application — ROUGE AU SELF-TEST PRODUCTION

- HEAD : `9e105623c88776b67ef071bbb5b8c0f32c3eac79` ;
- run : `32990549804` ;
- job : `98246877610` ;
- compilation application x64 native ONNX : VERT ;
- tests déterministes : VERT ;
- base r20 : VERT ;
- téléchargement + hashes Qwen : VERT ;
- échec unique : **`Run production LocalAiClient native ONNX self-test`** ;
- packaging final et artifact non produits sur #79.

### #80 — correction runtime app-local — VERT

- HEAD : **`e460586c140d2f87cbcbbe45740ef666c7923395`** ;
- commit : **`BUILD #30 pin ONNX Runtime 1.24.4 app-local`** ;
- run : **`32997181985`** ;
- conclusion : **SUCCESS** ;
- cette étape a permis de sortir de l’échec #79 et d’obtenir une chaîne ONNX native complète fonctionnelle en CI.

### #81 — optimisation latence — VERT

- HEAD : `8671275bc77eb1fdbdefc0b0158254efdf86df5c` ;
- run : `33001002041` ;
- artifact complet produit ;
- toutes validations natives ONNX + package + smoke app VERTES.

## HISTORIQUE LLAMA — VOIE ABANDONNÉE

Ne pas revenir à llama.cpp sans décision explicite de l’utilisateur.

- #63 : package fonctionnel sur PC mais latence 30 s à 2 min sur certaines réponses Qwen ; base/IA prêtes ; UI 14 onglets correcte.
- Défauts #63 : MAP confondu avec mesure live, injecteur décrit à tort comme injection d’huile, SPI halluciné ; corrigés ensuite par réponses contrôlées.
- Commit `f860749313447e224f63b99801f7e7d6a1839a49` : réponses contrôlées MAP / injecteur / SPI.
- #65 : `GGML_BACKEND_DL requires BUILD_SHARED_LIBS`.
- #66 : crash chargement modèle `0xC0000409` avec runtime partagé multi-variantes.
- #67 : staging partagé incomplet, `0xC0000135 STATUS_DLL_NOT_FOUND`.
- Runtime officiel llama b10516 testé ; archive SHA-256 `fbbbc55e0eb2e1b07f9dcb9488616c98ed47d9003b90e15e7c8c7812c4307cd3`.
- #68 : CI verte, mais **PC réel : `QProcess 0 / FailedToStart`** à l’ouverture IA.
- #72 : dépendances VC++ app-local, CI verte, PC réel identique.
- #73 : retour source-build llama, nouveau `0xC0000409` au chargement Qwen.
- Décision utilisateur : remplacer le moteur ; cible ONNX native retenue.

## PRINCIPE IA / BASE EXPERTE

**Toute connaissance certaine déjà présente dans le logiciel, les libellés/aides, le décodage ou la base experte doit produire une réponse immédiate sans appel au modèle génératif.**

Qwen est réservé :
- au croisement de plusieurs faits ;
- au raisonnement diagnostic ;
- aux questions générales MEMS ;
- aux cas où aucune réponse déterministe fiable n’existe.

Réponses immédiates déjà présentes/à préserver : batterie, régime, température liquide, MAP, lambda, avance, dwell, ralenti/IAC, papillon, état moteur, diagnostic de cohérence, MAP/injecteur/SPI contrôlés.

Définitions contrôlées :
- MAP = **Manifold Absolute Pressure**, pression absolue du collecteur, information de charge ;
- injecteur = électrovanne **essence** commandée ECU, SPI/MPI, jamais injection d’huile ;
- SPI = **Single Point Injection**, injection monopoint Rover/Mini MEMS.

## AUDIT IA — CONNAISSANCES UI À EXPLOITER

Aperçu/cadrans :
- régime moteur ;
- température liquide ;
- MAP ;
- position papillon ;
- tension batterie ;
- correction carburant court terme ;
- tension lambda ;
- temps injecteur ;
- température air admission ;
- position IAC ;
- avance allumage ;
- état système ;
- ancien UI : contact ralenti, boucle fermée, enregistrement.

Repères déjà codés :
- lambda 0–200 mV = pauvre ; 700–900 mV = riche ;
- MAP moteur arrêté ~100 kPa ; ralenti ~25–40 kPa ;
- plages visuelles batterie/liquide/IAT/avance/RPM = guides UI, pas forcément spécifications constructeur.

Réglages présents : correction carburant, position ralenti chaud, vitesse ralenti, correction avance, remise à zéro réglages, reset ECU.

Actionneurs présents : chauffage collecteur/PTC, pompe carburant, chauffage lambda/O2, purge canister, embrayage clim, pression/boost, ventilateurs 1/2/3, injecteur, bobine, moteur IAC, reset actionneurs.

Codes affichés : 01 à 24. **Code 23 / antidémarrage reste non prouvé**.

## SOUS-AUDIT RAVE / MINI SPi / MPi

Règles :
- ne pas stocker les manuels complets ;
- extraire des faits techniques structurés ;
- conserver source/document/famille/page/niveau de preuve/conflits ;
- séparer strictement SPi, MPi et cas particuliers ;
- priorité aux preuves constructeur Rover/RAVE ;
- tout fait vérifié utile doit être intégré à la base de référence.

Faits déjà classés :
- ralenti SPi 1993–96 : **850 ±25 tr/min** ;
- SPi 1997+ et MPi : **900 ±50 tr/min** ;
- pression carburant SPi : ~1 bar ;
- pression carburant MPi : **3,0 ±0,2 bar** ;
- IACV, TPS, MAP, ECT, IAT, CKP, CMP, lambda, injecteurs, pompe, purge, ventilateurs et stratégies de secours documentés ;
- ventilateur MPi 97MY : **105 °C ON / 98 °C OFF** ;
- SPi Japon : **98 °C ON / 93 °C OFF** ;
- ne pas régler le ralenti par la vis de butée papillon ;
- conflits résistance bobine conservés sans arbitrage non prouvé.

Enrichissements déjà poussés :
- lot **1660** : faits RAVE pratiques/constructeur ;
- lot **1670** : brochages MPi 97MY RCL0194 ;
- base experte r20 régénérée sans changement de schéma/révision.

TestBook / codes / brochages :
- Rover distingue Mini MEMS 1.3 SPi, MEMS 1.6 SPi et MEMS 2J MPi ; ne pas appeler automatiquement Mini MPi « MEMS 1.9 » ;
- `0x80` : code 1 ECT, 2 IAT, 10 pompe carburant, 16 TPS fortement supportés Mini SPi ;
- `0x7D:0x05` : 20 chauffage lambda, 21 synchronisation, 22 ventilateur 1, 24 ventilateur 2 ;
- code 23 / bit6 = `preuve_insuffisante` ;
- RCL0194 MPi 97MY : MAP `C159-8`, retour capteurs `C159-13`, IAT `C159-14`, ECT `C159-36`, relais pompe `C159-30`, signal antidémarrage `C159-17` ;
- ne pas inventer les pins SPi ancien sans schéma constructeur lisible.

## SÉCURITÉ PROTOCOLE — À PRÉSERVER, NE PAS MODIFIER PENDANT BUILD #30 IA

- `MemsEcuFamily::{Unknown, Rosco13_16, Mems19}`.
- `MemsDiagnosticMode::{Unknown, Normal, Mode3, Mode4, Transition}`.
- D0/D1/D2 : normal seulement.
- D1 bloqué Mode4.
- D3/F3/F4/F5 bloqués interface générique.
- Mutations : Rosco13_16 prouvé + mode Normal uniquement.
- Unknown = fail-closed.
- MEMS1.9 mutations bloquées.
- F7/EF bloqués sans sous-type prouvé.
- Transaction RAM bloque commandes génériques.
- Conserver `void onProtocolCommandRequested(quint8 command);`.
- Traces : D0 `D0 98 00 02 02`, D1 `AANMP002`, F0 `F0 50`, D2 `D2 00 01`, F4 `F4 00`.
- Ralenti chaud : `raw - 32768 - correction`, correction réelle de Réglages ; jamais `-3` hardcodé.
- Dwell : repère ~1,9–3,1 ms vers 14 V.
- Aucune mutation ECU pendant BUILD #30.

## BLOQUEURS NO-GO

Ne pas débloquer sans preuve/documentation :
- MEMS1.9 F7/EF ;
- tailles 7D/80 ;
- W4 25–50 ms ;
- reconnexion 1.9 ;
- failsafe actionneurs ;
- ports série arbitraires ;
- profils RAM non validés ;
- reset/clear faults/trims/écritures pendant BUILD #30.

## UI OFFICIELLE À PRÉSERVER

Onglets : Aperçu, Injection, Réglages, Actionneurs, Erreurs, Diagnostic automatique, IA MEMS, Analyse, Toutes les mesures, ECU/ROSCO, Toutes les données, Base de données, Interactif, Test ECU 1.9.

- style dark inchangé ;
- responsive obligatoire ;
- ne pas retirer/modifier un onglet sans demande explicite ;
- Injection reste entre Aperçu et Réglages ;
- les valeurs de l’onglet Injection doivent partager le même mode de lecture/polling compatible pour ne pas ralentir le programme.

## EXIGENCES IA FUTURES — UNIQUEMENT APRÈS VALIDATION DU MOTEUR SUR PC

Fichiers CSV/TXT :
- glisser-déposer `.csv/.txt` ;
- bouton `+` ;
- filtre Windows ;
- fichier sélectionné amovible ;
- analyse locale read-only ;
- aucune commande ECU.

Zone de saisie :
- dark/responsive ;
- arrondie, multiligne ;
- bouton `+` à gauche ;
- bouton rond orange avec flèche vers le haut à droite ;
- aucun changement global de thème.

## DÉSINSTALLATION BUILD #30

- `ecu_mems_uninstaller.exe` + `install_manifest.txt` requis ;
- refuse si application active ;
- profil conservé par défaut ;
- données locales supprimées seulement sur choix explicite ;
- fichiers étrangers préservés.

## TEST PC RÉEL ATTENDU POUR #81

Ne pas modifier le code avant d’obtenir ces résultats si l’utilisateur teste #81 :
1. application démarre normalement ;
2. onglet IA MEMS s’ouvre sans crash ;
3. statut base experte prêt ;
4. statut IA locale prêt ;
5. question déterministe (ex. MAP/SPI/injecteur) répond immédiatement et correctement ;
6. question générative simple répond sans délai anormal ;
7. diagnostic plus complexe : relever temps réel et qualité ;
8. vérifier que navigation/UI et autres onglets n’ont pas régressé.

Le test CI #81 valide la technique, **pas encore le comportement réel sur le PC utilisateur**.

## PROCHAINE ACTION EXACTE

**État courant à reprendre après toute coupure de discussion : `MEMSX64`, BUILD logiciel #30/v1.0.30, HEAD `8671275bc77eb1fdbdefc0b0158254efdf86df5c`, GitHub Actions #81 VERT, artifact complet `ECU-MEMS-Manager-x64-BUILD-30-v1.0.30` ID `9618786882`, digest `sha256:17f8b0d3b81ff0a7d02b964ef265b38e5746fa70ff1cf754675d9d14ced3265e`. Prochaine étape : test PC réel de #81 et mesure de la latence/qualité IA. Ne faire aucun BUILD #31, aucun changement protocole ECU, aucun changement 32 bits, aucun changement UI non demandé. Après validation du moteur IA sur PC, reprendre l’enrichissement RAVE/base experte exactement au point déjà documenté ci-dessus.**
