# RAPPORT DE CONTINUITÉ — ECU MEMS MANAGER

> **RÈGLE OBLIGATOIRE** : relire ce fichier avant toute modification. Branche rapport : `RAPPORT`. Branche x64 active : `MEMSX64`.
>
> **RÈGLE QUALITÉ — AUCUNE RUSTINE** : rechercher la cause réelle et produire une solution propre, générale et maintenable. Ne pas supprimer une capacité juste pour faire passer un test.
>
> **SUIVI IMMÉDIAT** : avant chaque nouvelle étape, inscrire ici l’étape et son objectif ; après chaque résultat, l’inscrire avant la suite.
>
> **NOMMAGE UTILISATEUR** : `ECU MEMS Manager x64 #NN — Commit xxxxxxx`. `#NN` = GitHub Actions.
>
> **VERSIONNAGE REPRIS À PARTIR DU PROCHAIN PUSH** : à partir du prochain push sur `MEMSX64`, le numéro de version du programme doit suivre le numéro du run/build GitHub Actions visible par l’utilisateur. Exemple obligatoire : **#93 => v1.0.93**, puis **#94 => v1.0.94**, etc. La fenêtre de démarrage/splash doit afficher ce même numéro ; About, aide et toute autre occurrence de version doivent rester synchronisés. **#92 reste historiquement v1.0.30** ; la reprise de numérotation commence au prochain push. Ne pas repartir en v1.0.31.

## ÉTAT COURANT — 27 AOÛT 2026

- Dépôt : `mini56/ECU-Mems-Manager-Session`.
- Branche active : **`MEMSX64`**.
- HEAD x64 courant : **`5ad520dc5ef4b5ec6eb7096233d7a7a1d7f916f3`**.
- Dernière version logicielle entièrement validée : **#92 = v1.0.30**, commit `16b99c3f`, run `33058810115`.
- Run **#93 = v1.0.93 — FAILURE**, run `33066459991`, commit `5ad520dc` : échec pendant l’étape de build parce que le nouveau `ia_mems_diagram_selftest.exe`, lié à `Qt5::Core`, est lancé en `POST_BUILD` avant que le runtime Qt soit disponible dans le `PATH`. Le programme principal est bien généré et le test protocole passe avant cet échec.
- Le prochain push sur `MEMSX64` doit produire **#94 = v1.0.94** selon la règle dynamique `github.run_number`.
- Run **#91 — FAILURE**, run `33047008070`, commit `897b51c8` : échec uniquement à `Validate packaged production LocalAiClient with Qwen` après assemblage ; moteur ONNX et réponses déterministes prêts, puis réponse générative rejetée par le contrôle de langue active.
- Run **#92 — SUCCESS**, run `33058810115`, commit `16b99c3f` : correction de déterminisme ONNX validée ; LocalAiClient natif + packagé passent.
- Artefact #90 : ID `9635406628`, taille `386 768 840`, SHA-256 `125e27658bba577efdf5d22a7cb3fa26670cddadd8383e8f4e7d907d765f6d6d`.
- 32 bits `lab-expert-engine` et rollback `MEMSX64-BUILD26-BASE` : **NE PAS TOUCHER**.

## RAVE 1700 — VALIDÉ DIRECTEMENT DANS #90

Source Rover `RCL0194ENG`, SPi Japan 97MY, page 20.4. 8 faits couleur : injecteur SU/YN, purge NK/BW, IAC phases OS/OU/OG/KU.

Artefact #90 inspecté directement : **75 faits RAVE / 87 faits experts**, 8 faits 1700 + 8 miroirs, tous `verifie_constructeur`, variante unique SPi Japon 97MY, `PRAGMA integrity_check = ok`, `user_version = 20`.

## RAVE 1710 — POUSSÉ SUR MEMSX64

Source primaire : PDF Rover complet `RCL0194ENG`, page **20.3**, SPi Japan 97MY.

11 faits constructeur :
1. C159-4 MAIN RELAY CONTROL : WK = blanc/rose.
2. C159-35 DUAL PRESSURE SWITCH : GW = vert/blanc.
3. C159-36 OXYGEN SENSOR RELAY : BG = noir/vert.
4. C159-14 AUTOMATIC INHIBITOR : WLG = blanc/vert clair.
5. C159-29 SCREEN GROUND sonde oxygène : B = noir.
6. C159-7 OXYGEN SENSOR +VE : S = gris.
7. C159-18 OXYGEN SENSOR -VE : LGS = vert clair/gris.
8. C159-32 CKP -VE, boîte manuelle : WU = blanc/bleu.
9. C159-31 CKP +VE, boîte manuelle : UP = bleu/violet.
10. C159-20 FUEL PUMP RELAY : BP = noir/violet.
11. C159-11 IGNITION SENSE : W = blanc.

Validation locale : +11 RAVE +11 experts, integrity `ok`, user_version20, tous `verifie_constructeur`. Après 1700 + 1710 attendu : **86 RAVE / 98 experts**.

- SQL 1710 : 12903 octets, SHA-256 `a1d1283c0f38a1b3f65994abd7f8d2ce5c34d9e1f0798c447c5862114316c6de`.
- qz64 1710 : 1945 octets, SHA-256 `fa9ec2dbdf2178c30f47a1f6cc356c97035c46b780d1e5adced765335af412dc`.
- Préparé sur `tmp-rave-1710`, exactement trois fichiers documentaires/base.
- `MEMSX64` avancée en fast-forward sur **`897b51c8382513e15f236c18446d1cffc2352c31`** avant correction #91.
- Aucun code IA, protocole, UI, packaging, 32 bits ou BUILD modifié par RAVE 1710.

## INCIDENT #91 — VALIDATION IA PACKAGÉE

### Résultat observé
Le journal complet fourni et le run GitHub montrent :
- `STATE=IA locale en démarrage` puis `STATE=IA locale prête` ;
- les étapes déterministes 0 à 3 répondent correctement, y compris date et faits de brochage de test ;
- le passage génératif entre bien en `STATE=IA locale en réponse` puis revient prêt ;
- l'échec final est : **`Le modèle local n'a pas produit de réponse exploitable dans la langue active.`** ;
- le processus sort avec code 1 uniquement dans la validation packagée.

La comparaison **#90 `fab2e4cf` → #91 `897b51c8`** ne contient aucun changement de `LocalAiClient`, du self-test, d'ONNX ou du packaging IA : le lot 1710 est documentaire/base. Le défaut à traiter est donc la robustesse/déterminisme du chemin génératif + validation de langue, pas un crash ONNX ni une régression de code IA introduite par RAVE 1710.

### OBJECTIF AVANT MODIFICATION
Identifier exactement le prompt génératif, les paramètres de génération et le contrôle `réponse exploitable dans la langue active`. Corriger la cause générale qui peut rejeter de manière non déterministe une génération valide, **sans supprimer ni affaiblir le garde de langue de production** et sans masquer l'échec par une simple boucle de relance. Valider ensuite le self-test natif ET packagé sur GitHub Actions en restant BUILD #30 / v1.0.30 pour cette correction #91.

Aucun changement protocole, ECU, UI, 32 bits, Qwen/ONNX de version ou BUILD logiciel n'était autorisé dans cette correction.

## NOUVELLE ACTION AUTORISÉE — SCHÉMAS PROPOSÉS PAR IA MEMS

### Objectif avant toute modification
Ajouter à IA MEMS une capacité **séparée du moteur IA** : lorsqu’une question correspond à un schéma local connu (ex. brochage MEMS 1.3, connecteur MEMS 1.9, ROSCO 3 broches), l’onglet IA peut proposer un bouton explicite **« Ouvrir le schéma … »** pour visualiser le fichier local correspondant.

### Contraintes obligatoires
- **NE PAS MODIFIER** Qwen, ONNX, `LocalAiClient`, le prompt, les budgets de tokens ou la génération qui fonctionne.
- La détection de schéma doit être déterministe et locale, indépendante du LLM.
- Réutiliser le mécanisme d’affichage de schéma existant s’il existe ; sinon créer le plus petit composant de visualisation local possible.
- Aucun accès réseau nécessaire pour ouvrir un schéma.
- Ne jamais ouvrir automatiquement : seulement proposer, puis ouvrir sur clic utilisateur.
- Ne jamais inventer de schéma ; n’utiliser que des fichiers réellement présents dans le package / manifeste.
- Préparer sur branche temporaire, ajouter un self-test spécifique, comparer le diff, puis seulement envisager `MEMSX64` après validation.
- Aucun changement protocole, ECU, RAVE ou 32 bits. Le seul changement de version autorisé/obligatoire au prochain push est la synchronisation de version définie plus haut : **run #93 => v1.0.93**.

## RAVE 1720 — EN ATTENTE PENDANT LA MODIFICATION SCHÉMAS

Candidats déjà identifiés à confirmer dans RCL0194 avant intégration : signal jauge température C159-5 GU, commande bobine C159-25 WB et liaison C161-18 WS, ligne A/C C159-19 RW. Reprendre ce lot séparément après stabilisation de la fonction schémas.

## SÉCURITÉ À PRÉSERVER

Ne pas modifier protections protocole BUILD #30, MEMS1.9 F7/EF, 7D/80, W4, reconnexion1.9, RAM non validée, reset/clear/writes, `onProtocolCommandRequested(quint8)`, formule `raw-32768-correction`. UI dark/responsive inchangée. ONNX natif inchangé. 32 bits inchangé.

## SUIVI INCIDENT #91 — AUDIT AVANT CORRECTION

Audit effectué après relecture obligatoire du présent rapport, sans modification préalable du code :
- `expert/LocalAiOnnxSelfTest.cpp` : les étapes 0 à 3 sont déterministes ; l'étape 4 est le vrai passage Qwen avec `Réponds uniquement par OK.` et sans grounding de secours.
- `expert/LocalAiClient.cpp` : la génération active `do_sample=true`, `temperature=0.7` en mode rapide, `top_p=0.8`, `top_k=20`. Aucun `random_seed` n'est fixé ; deux exécutions identiques peuvent donc diverger.
- Le garde français `likelyWrongLanguage()` est conservé : rejet uniquement si au moins trois marqueurs anglais sont détectés et au plus un marqueur français. `containsInternalInstructionLeak()` reste également obligatoire.
- L'ancien probe ONNX Python antérieur à l'intégration C++ utilisait `do_sample=False`, donc était déterministe ; l'intégration C++ a introduit l'échantillonnage aléatoire.
- La documentation officielle ONNX Runtime GenAI expose `random_seed` comme option numérique de recherche ; `-1` signifie graine aléatoire.

### ÉTAPE AUTORISÉE AVANT MODIFICATION DU CODE

Correction ciblée retenue : **conserver `do_sample=true`, les températures, `top_p`, `top_k`, le prompt, les budgets de tokens et tous les gardes de production, mais fixer `random_seed=42` dans les paramètres ONNX GenAI**. Cela rend une même requête reproductible sans transformer la génération en greedy, sans affaiblir le contrôle de langue et sans ajouter de boucle de relance. Modification limitée à `expert/LocalAiClient.cpp`, puis contrôle du diff et push sur `MEMSX64` en restant BUILD #30 / v1.0.30 pour cette correction.

### RÉSULTAT DE LA CORRECTION

- Commit `MEMSX64` : **`16b99c3f40985b7ef5439ee8834eb5e8dd8126bf`** (`BUILD #30 make ONNX sampling reproducible`).
- Diff contrôlé après push : **exactement une ligne ajoutée** dans `expert/LocalAiClient.cpp`, `OgaGeneratorParamsSetSearchNumber(params, "random_seed", 42.0)` ; aucun autre fichier ni aucune autre ligne de code modifiée.
- Le garde `likelyWrongLanguage()`, `containsInternalInstructionLeak()`, le prompt, `do_sample=true`, `temperature`, `top_p`, `top_k`, les budgets de tokens et les fallbacks restent inchangés.
- GitHub Actions **#92 — SUCCESS**, run `33058810115`, commit `16b99c3f`. Les validations LocalAiClient native et packagée passent ; l'incident #91 est clos.

## REPRISE DU VERSIONNAGE — RÈGLE ACTIVE

- État historique actuel : **ECU MEMS Manager x64 #92 = v1.0.30**.
- **Prochain push : #93 doit produire v1.0.93.**
- Ensuite le numéro reste synchronisé : #94 = v1.0.94, #95 = v1.0.95, etc.
- La version affichée dans la fenêtre de démarrage/splash est la référence visible par l’utilisateur et doit correspondre au run/build livré.
- About, aide, métadonnées et toute autre occurrence du numéro de version doivent utiliser la même valeur ; éviter plusieurs numéros incohérents dans le même package.
- Cette règle remplace pour les futurs pushes la conservation en v1.0.30. Elle ne modifie pas rétroactivement #92.

## ORDRE DE TRAVAIL VALIDÉ APRÈS #92

L’ordre demandé est désormais strictement :
1. **Consigner #92 vert** et conserver la validation LocalAiClient native + packagée — fait.
2. **Auditer `IaMemsTab` / `IaMemsService`** avant modification pour comprendre l’intégration actuelle et éviter toute régression.
3. **Ajouter la fonction de proposition de schéma local** : lorsqu’un schéma réellement présent correspond à la demande, proposer explicitement un bouton « Ouvrir le schéma … », sans ouverture automatique et sans dépendre du LLM.
4. **Respecter toutes les contraintes de séparation et de sécurité** : ne pas modifier Qwen, ONNX, `LocalAiClient`, le protocole, l’ECU, RAVE ni le 32 bits pendant cette fonction ; préparer et tester proprement avant intégration.

**L’ancien point 5 “rester BUILD #30 / v1.0.30” est supprimé pour les futurs pushes.** Il est remplacé par la règle active de synchronisation : **#93 = v1.0.93**, puis #94 = v1.0.94, etc.

**Après validation complète des points 1 à 4 et du build correspondant, revenir à RAVE.** Le lot en attente est **RAVE 1720**, à reprendre séparément après stabilisation de la fonction schémas.

## POINT 2 — AUDIT IA MEMS / SCHÉMAS TERMINÉ AVANT CODE

Audit effectué sur le HEAD `MEMSX64` `16b99c3f`, après relecture du présent rapport et **avant toute modification du code** :
- `IaMemsTab` est une vue simple : `sendQuestion()` transmet la question à `IaMemsService::ask()`, puis `onServiceResponse()` affiche la réponse. Aucun mécanisme de schéma n’est actuellement couplé au service IA.
- `IaMemsService` gère le contexte, le grounding, le moteur expert et `LocalAiClient`. Il n’a pas besoin d’être modifié pour proposer un schéma ; il doit rester inchangé afin de préserver Qwen/ONNX et le chemin IA validé dans #92.
- `database/reference/manifest.json` contient déjà exactement six schémas locaux déclarés : MEMS 1.2 ECU, MEMS 1.3 ECU, MEMS 1.6 ECU, MEMS 1.9 ECU, ROSCO 3 broches et MEMS 1.9 OBD 16 broches. La future proposition devra être limitée à ces entrées réellement déclarées.
- `database/MemsDatabaseDiagramSearchPatch.cpp` possède déjà une méthode d’affichage SVG local dans un `QTextBrowser`, avec contrôle d’existence du fichier. `MemsDatabaseBrowser` n’expose cependant pas de méthode publique permettant à `IaMemsTab` d’ouvrir directement un schéma sans couplage artificiel.
- Le choix retenu est donc de laisser le navigateur de base intact et d’ajouter un **petit composant déterministe local** dédié à la résolution et à l’affichage des schémas pour l’onglet IA, en réutilisant la même technique Qt/SVG déjà validée.
- Le bouton sera caché par défaut, calculé localement à partir de la question, affiché uniquement si un schéma manifeste correspondant existe, et ne s’ouvrira que sur clic utilisateur. Il sera placé dans `IaMemsTab` entre la zone de réponse et la zone de saisie afin de ne pas perturber le dialogue.
- Aucun schéma ne pourra être inventé : résolution par clé/chemin du manifeste + contrôle d’existence du fichier local. Aucun accès réseau.
- Le `CMakeLists.txt` copie déjà tout `database/reference` dans le dossier du programme ; aucun nouveau mécanisme de packaging des SVG n’est nécessaire.
- Le workflow `.github/workflows/memsx64.yml` ne se déclenche sur push que pour `MEMSX64`. Une branche temporaire peut donc être utilisée sans consommer le prochain numéro GitHub Actions #93.
- Le calcul de version CMake utilise déjà `MEMS_BUILD_NUMBER` : avec la valeur `93`, `APP_VERSION` devient correctement `1.0.93`. `About` et l’aide utilisent déjà `APP_VERSION`; le splash utilise `APP_BUILD_NUMBER`. Le push final devra donc mettre le workflow à `93` / `1.0.93` pour respecter la nouvelle règle.
- Un self-test spécifique sera ajouté pour vérifier au minimum : MEMS 1.3 ECU, MEMS 1.9 OBD, ROSCO 3 broches, absence de suggestion sur une question non liée à un schéma, et refus d’un chemin absent/non déclaré.

### PLAN AUTORISÉ POUR LE POINT 3

1. Créer une branche temporaire depuis `16b99c3f`, sans déclencher le workflow x64.
2. Ajouter le résolveur de schémas local déterministe + le petit visualiseur local ; ne pas modifier `IaMemsService` ni `LocalAiClient`.
3. Intégrer dans `IaMemsTab` uniquement le bouton de proposition et son clic d’ouverture.
4. Ajouter le self-test spécifique et son intégration CMake, puis contrôler le diff complet de la branche temporaire.
5. Après validation du diff et des tests, préparer **un seul passage vers `MEMSX64`** avec synchronisation workflow/version **#93 = v1.0.93** afin d’éviter tout build intermédiaire désynchronisé.

## POINT 3 — CANDIDAT TEMPORAIRE ET CONTRÔLE SVG

- Branche temporaire créée depuis exactement `16b99c3f` : **`tmp-ia-schema-proposal`**. Aucun push sur `MEMSX64`, donc aucun run #93 consommé.
- Candidat actuel limité à six fichiers : `CMakeLists.txt`, `iamemstab.cpp`, `iamemstab.h`, plus les nouveaux `expert/IaMemsDiagramCatalog.cpp`, `expert/IaMemsDiagramCatalog.h` et `expert/IaMemsDiagramSelfTest.cpp`. Aucun fichier `IaMemsService`, `LocalAiClient`, Qwen/ONNX, protocole, ECU, RAVE ou 32 bits modifié.
- Le résolveur est déterministe et local : intention de schéma + famille explicite, lecture exclusive de `database/reference/manifest.json`, rejet des chemins absolus/traversants, contrôle du fichier réel et du chemin canonique.
- Les six entrées du manifeste sont couvertes par le self-test : MEMS 1.2 ECU, MEMS 1.3 ECU, MEMS 1.6 ECU, MEMS 1.9 ECU, ROSCO 3 broches et MEMS 1.9 OBD 16 broches. Le test couvre également une question ordinaire, une demande OBD ambiguë, une prise diagnostic ambiguë, un SVG absent et un SVG local non déclaré dans le manifeste.
- Les six fichiers SVG déclarés ont été vérifiés physiquement présents dans `database/reference/images`.
- Contrôle direct de l’artefact **#92**, ID **`9640959766`**, nom `ECU-MEMS-Manager-x64-BUILD-30-v1.0.30` : le package contient bien les six SVG ainsi que **`imageformats/qsvg.dll`**, **`iconengines/qsvgicon.dll`** et **`Qt5Svg.dll`**. Le rendu SVG local par Qt est donc déjà une capacité réellement packagée et validée ; aucune nouvelle dépendance SVG ni modification de packaging n’est nécessaire.
- Revue avant intégration : deux durcissements minimes sont retenus avant le diff final : (1) ajouter l’inclusion explicite de `QByteArray` dans le self-test ; (2) au clic, revalider la suggestion via le manifeste et le fichier local au lieu de se contenter du chemin mémorisé, afin qu’un manifeste/fichier modifié après la question ne puisse jamais ouvrir une proposition devenue invalide.

### ÉTAPE AUTORISÉE AVANT LA PROCHAINE MODIFICATION DU CODE

Sur **`tmp-ia-schema-proposal` uniquement**, appliquer ces deux durcissements sans modifier aucun autre comportement ni fichier interdit. Ensuite contrôler à nouveau le diff complet contre `16b99c3f`. Ne pas pousser sur `MEMSX64` à cette étape.

## POINT 3 — CONTRÔLE FINAL AVANT BUILD #93

- Les deux durcissements ont été appliqués sur `tmp-ia-schema-proposal` : inclusion explicite de `QByteArray` dans le self-test et revalidation complète manifeste/fichier lors du clic.
- HEAD candidat temporaire : **`5ad520dc5ef4b5ec6eb7096233d7a7a1d7f916f3`**.
- Comparaison finale contre `16b99c3f` : exactement **7 fichiers** modifiés/ajoutés. Les six fichiers fonctionnels schéma sont `CMakeLists.txt`, `iamemstab.cpp`, `iamemstab.h`, `expert/IaMemsDiagramCatalog.cpp`, `expert/IaMemsDiagramCatalog.h`, `expert/IaMemsDiagramSelfTest.cpp`; le septième est uniquement `.github/workflows/memsx64.yml` pour la validation et le versionnement.
- Aucun fichier `IaMemsService`, `LocalAiClient`, Qwen/ONNX, protocole, ECU, RAVE ou 32 bits n'est modifié. `iamemstab.cpp` et `iamemstab.h` n'ont aucune suppression par rapport au HEAD #92.
- Le workflow historique utilisait provisoirement `BUILD 30` / `1.0.30` en dur. Retour à la méthode générale : le numéro de build et la version sont maintenant dérivés de **`github.run_number`**. Le run #92 a été contrôlé via l'API GitHub et possède bien `run_number: 92`; le prochain run de ce workflow doit donc produire **#93 / v1.0.93**, puis #94 / v1.0.94, etc.
- Le workflow ne contient plus d'occurrence active de `1.0.30`. `MEMS_BUILD`, `MEMS_BUILD_NUMBER`, `MEMS_VERSION`, le README, les noms de fichiers de hash et le nom d'artefact utilisent la valeur dynamique.
- Le nouveau `ia_mems_diagram_selftest` est explicitement compilé puis exécuté dans GitHub Actions.
- La validation du package exige désormais le manifeste de référence, les six SVG, `Qt5Svg.dll` et `imageformats/qsvg.dll`, en plus des validations ONNX/expert déjà présentes.
- Le workflow reste déclenché par push uniquement sur **`MEMSX64`** ; aucun run n'a été consommé pendant la préparation temporaire.

## INCIDENT #93 — SELF-TEST SCHÉMA LANCÉ AVANT LE RUNTIME QT

- `MEMSX64` a été avancée sans force sur le candidat contrôlé `5ad520dc5ef4b5ec6eb7096233d7a7a1d7f916f3`.
- GitHub Actions a créé correctement **#93 / v1.0.93**, run `33066459991` : la nouvelle règle de version dynamique fonctionne.
- Le log fourni montre `ecu_mems_manager.exe` généré puis `PASS protocol context safety policy`.
- L’échec survient immédiatement lorsque le `POST_BUILD` de `ecu_mems_manager` tente d’exécuter `Release\ia_mems_diagram_selftest.exe`; MSBuild remonte **`-1073741515`** (`0xC0000135`, runtime DLL introuvable).
- `ia_mems_diagram_selftest` est lié à `Qt5::Core`. Au moment du `POST_BUILD`, le workflow n’a pas encore ajouté `C:\Qt\5.15.2\msvc2019_64\bin` au `PATH` et n’a pas encore déployé Qt dans `Release`.
- Le workflow possède déjà une étape séparée `Run deterministic self-tests` qui ajoute explicitement le répertoire Qt au `PATH`, puis exécute `ia_mems_diagram_selftest.exe`. Le test est donc déjà prévu au bon endroit.
- Cause réelle : **ordre d’exécution du self-test**, pas erreur de compilation du catalogue, pas protocole, pas Qwen/ONNX, pas packaging.

### ÉTAPE AUTORISÉE AVANT MODIFICATION DU CODE — CORRECTION #93

1. Créer une nouvelle branche temporaire depuis `5ad520dc` ; ne pas modifier directement `MEMSX64`.
2. Dans `CMakeLists.txt`, supprimer uniquement la commande `POST_BUILD` qui lance `ia_mems_diagram_selftest` trop tôt.
3. Conserver intégralement la cible `ia_mems_diagram_selftest`, sa compilation, `Qt5::Core`, `add_dependencies(${PNAME} ia_mems_diagram_selftest)` et son exécution explicite dans `Run deterministic self-tests` du workflow.
4. Ne modifier aucun fichier IA moteur, protocole, ECU, RAVE, 32 bits ou interface.
5. Contrôler le diff complet contre `5ad520dc`. Si et seulement si le diff est cette suppression CMake ciblée, intégrer sans force sur `MEMSX64` ; le push suivant doit produire **#94 / v1.0.94**.

## PROCHAINE ACTION EXACTE

**Préparer la correction #93 sur branche temporaire depuis `5ad520dc`, avec uniquement la suppression du lancement `POST_BUILD` prématuré de `ia_mems_diagram_selftest`. Contrôler le diff avant toute intégration `MEMSX64`. RAVE 1720 reste en attente.**