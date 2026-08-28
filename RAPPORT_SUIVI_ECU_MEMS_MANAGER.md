# RAPPORT DE CONTINUITÉ — ECU MEMS MANAGER

> **RÈGLE OBLIGATOIRE** : relire ce fichier avant toute modification. Branche rapport : `RAPPORT`. Branche x64 active : `MEMSX64`.
>
> **RÈGLE QUALITÉ — AUCUNE RUSTINE** : rechercher la cause réelle et produire une solution propre, générale et maintenable. Ne pas supprimer une capacité juste pour faire passer un test.
>
> **SUIVI IMMÉDIAT** : avant chaque nouvelle étape, inscrire ici l’étape et son objectif ; après chaque résultat, l’inscrire avant la suite. **Toute modification, validation, échec, test réel et décision doit être ajouté au rapport au fur et à mesure afin de conserver l’historique complet entre les discussions.**
>
> **NOMMAGE UTILISATEUR** : `ECU MEMS Manager x64 #NN — Commit xxxxxxx`. `#NN` = GitHub Actions.
>
> **VERSIONNAGE ACTIF** : le numéro de version du programme suit désormais le numéro du run/build GitHub Actions visible par l’utilisateur : **#94 => v1.0.94**, #95 => v1.0.95, #96 => v1.0.96, #97 => v1.0.97, etc. La fenêtre de démarrage/splash, About, l’aide et toute autre occurrence de version doivent rester synchronisés. **#92 reste historiquement v1.0.30** ; #93 a inauguré la reprise dynamique avec v1.0.93.

## ÉTAT COURANT — 27 AOÛT 2026 — APRÈS TEST RÉEL BUILD #97

- Dépôt : `mini56/ECU-Mems-Manager-Session`.
- Branche active : **`MEMSX64`**.
- HEAD x64 courant : **`2b211554abdbb127fd4d472f9ce687394b2d4608`**.
- Dernier run GitHub entièrement vert : **#97 = v1.0.97 — SUCCESS**, run **`33107904830`**, commit **`2b211554abdbb127fd4d472f9ce687394b2d4608`**.
- Artefact #97 : ID **`9661453251`**, nom **`ECU-MEMS-Manager-x64-BUILD-97-v1.0.97`**, taille **386 847 391 octets**, SHA-256 **`7bf9ea11fb5ca7c8f7d3a500efe5bf04fa94e16d38a8a25d5c0aa972f41b14c7`**.
- #97 a été testé sur le PC réel utilisateur : **aucune régression visuelle constatée sur l’environnement et les onglets**, clarification SPi/MPi et boutons XML fonctionnels, mais plusieurs défauts IA restent ouverts et sont détaillés en fin de rapport. **#97 est vert côté build, mais la qualité conversationnelle IA n’est pas encore validée comme terminée.**
- Run **#96 = v1.0.96 — SUCCESS**, run **`33097323011`**, commit **`fea27058773fc3535c3ea40f7b9a36151b792ac1`**.
- Artefact #96 : ID **`9657131155`**, nom **`ECU-MEMS-Manager-x64-BUILD-96-v1.0.96`**, taille **386 836 238 octets**, SHA-256 **`a9677cdcead198e36f8fbcf9efccc6d55e493bede88c545683abaeffd8ca4124`**.
- #96 a été validé sur véhicule réel pour la connexion ECU x64, acquisition 7D/80, identification ECU/firmware/COM, RAM injection Mode 4, retour diagnostic et enregistrement TXT. Les défauts IA documentaires observés dans #96 ont conduit au lot #97.
- Run **#95 = v1.0.95 — SUCCESS**, run `33076089248`, commit `f2e97b3e` : lot RAVE 1720 Classic SPi / AKM7169.
- Run **#94 = v1.0.94 — SUCCESS**, run `33068860732`, commit `39543d69` : fonction schémas IA MEMS stabilisée et validée sur PC réel.
- Run **#93 = v1.0.93 — FAILURE**, run `33066459991`, commit `5ad520dc` : échec de lancement prématuré du nouveau self-test schéma Qt en `POST_BUILD`; incident corrigé et validé par #94.
- Run **#92 = v1.0.30 — SUCCESS**, run `33058810115`, commit `16b99c3f` : LocalAiClient natif + packagé validés.
- Run **#91 — FAILURE**, run `33047008070`, commit `897b51c8` : ancien incident de réponse générative rejetée, clos par #92.
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
- Aucun changement protocole, ECU, RAVE ou 32 bits.

## RAVE 1720 — ANCIENS CANDIDATS JAPON REPORTÉS

Candidats déjà identifiés dans RCL0194 : signal jauge température C159-5 GU, commande bobine C159-25 WB et liaison C161-18 WS, ligne A/C C159-19 RW. Ils restent conservés mais sont reportés dans un lot Japon séparé après rééquilibrage de la couverture SPi classique.

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

- État historique : **ECU MEMS Manager x64 #92 = v1.0.30**.
- Reprise dynamique appliquée : **#93 = v1.0.93**, puis **#94 = v1.0.94**.
- À partir de maintenant : #95 = v1.0.95, #96 = v1.0.96, #97 = v1.0.97, etc., via `github.run_number`.
- La version affichée dans la fenêtre de démarrage/splash est la référence visible par l’utilisateur et doit correspondre au run/build livré.
- About, aide, métadonnées et toute autre occurrence du numéro de version doivent utiliser la même valeur ; éviter plusieurs numéros incohérents dans le même package.

## ORDRE DE TRAVAIL VALIDÉ APRÈS #92

L’ordre demandé était strictement :
1. **Consigner #92 vert** et conserver la validation LocalAiClient native + packagée — fait.
2. **Auditer `IaMemsTab` / `IaMemsService`** avant modification — fait.
3. **Ajouter la fonction de proposition de schéma local** — fait et validé dans #94.
4. **Respecter toutes les contraintes de séparation et de sécurité** — fait : aucun changement Qwen, ONNX, `LocalAiClient`, protocole, ECU, RAVE ou 32 bits pour cette fonction.

**La fonction schémas est maintenant stabilisée par le build #94. Le travail est revenu à RAVE.**

## POINT 2 — AUDIT IA MEMS / SCHÉMAS TERMINÉ AVANT CODE

Audit effectué sur le HEAD `MEMSX64` `16b99c3f`, après relecture du présent rapport et **avant toute modification du code** :
- `IaMemsTab` est une vue simple : `sendQuestion()` transmet la question à `IaMemsService::ask()`, puis `onServiceResponse()` affiche la réponse. Aucun mécanisme de schéma n’était alors couplé au service IA.
- `IaMemsService` gère le contexte, le grounding, le moteur expert et `LocalAiClient`. Il n’a pas été modifié pour la fonction schémas.
- `database/reference/manifest.json` contient exactement six schémas locaux déclarés : MEMS 1.2 ECU, MEMS 1.3 ECU, MEMS 1.6 ECU, MEMS 1.9 ECU, ROSCO 3 broches et MEMS 1.9 OBD 16 broches.
- `database/MemsDatabaseDiagramSearchPatch.cpp` possède déjà une méthode d’affichage SVG local dans un `QTextBrowser`, avec contrôle d’existence du fichier.
- Le choix retenu a été un petit composant déterministe local dédié à la résolution et à l’affichage des schémas pour l’onglet IA, sans couplage au moteur IA.
- Le bouton est caché par défaut, calculé localement à partir de la question, affiché uniquement si un schéma manifeste correspondant existe, et ne s’ouvre que sur clic utilisateur.
- Aucun schéma ne peut être inventé : résolution par manifeste + contrôle du fichier réel et du chemin canonique. Aucun accès réseau.

## POINT 3 — CANDIDAT TEMPORAIRE ET CONTRÔLE SVG

- Branche temporaire créée depuis exactement `16b99c3f` : **`tmp-ia-schema-proposal`**.
- Candidat fonctionnel limité à six fichiers : `CMakeLists.txt`, `iamemstab.cpp`, `iamemstab.h`, `expert/IaMemsDiagramCatalog.cpp`, `expert/IaMemsDiagramCatalog.h` et `expert/IaMemsDiagramSelfTest.cpp`.
- Le résolveur est déterministe et local : intention de schéma + famille explicite, lecture exclusive de `database/reference/manifest.json`, rejet des chemins absolus/traversants, contrôle du fichier réel et du chemin canonique.
- Les six entrées du manifeste sont couvertes par le self-test : MEMS 1.2 ECU, MEMS 1.3 ECU, MEMS 1.6 ECU, MEMS 1.9 ECU, ROSCO 3 broches et MEMS 1.9 OBD 16 broches. Le test couvre aussi les refus d’ambiguïté, d’absence et de fichier non déclaré.
- Les six fichiers SVG déclarés ont été vérifiés physiquement présents dans `database/reference/images`.
- Contrôle direct de l’artefact #92 : package contenant les six SVG, `imageformats/qsvg.dll`, `iconengines/qsvgicon.dll` et `Qt5Svg.dll`.
- Deux durcissements ajoutés avant intégration : inclusion explicite de `QByteArray` dans le self-test et revalidation manifeste/fichier au clic.

## POINT 3 — CONTRÔLE FINAL AVANT BUILD #93

- HEAD candidat temporaire : **`5ad520dc5ef4b5ec6eb7096233d7a7a1d7f916f3`**.
- Comparaison finale contre `16b99c3f` : exactement **7 fichiers** modifiés/ajoutés : les six fichiers fonctionnels schéma plus `.github/workflows/memsx64.yml` pour validation/versionnement.
- Aucun fichier `IaMemsService`, `LocalAiClient`, Qwen/ONNX, protocole, ECU, RAVE ou 32 bits modifié.
- Retour à la méthode générale : build/version dérivés de `github.run_number`.
- Le nouveau `ia_mems_diagram_selftest` est explicitement compilé puis exécuté dans GitHub Actions.
- La validation du package exige le manifeste de référence, les six SVG, `Qt5Svg.dll` et `imageformats/qsvg.dll`, en plus des validations ONNX/expert.

## INCIDENT #93 — SELF-TEST SCHÉMA LANCÉ AVANT LE RUNTIME QT

- `MEMSX64` avancée sans force sur `5ad520dc5ef4b5ec6eb7096233d7a7a1d7f916f3`.
- GitHub Actions a créé correctement **#93 / v1.0.93**, run `33066459991` : la nouvelle règle de version dynamique fonctionne.
- Le log fourni montre `ecu_mems_manager.exe` généré puis `PASS protocol context safety policy`.
- L’échec survient lorsque le `POST_BUILD` tente d’exécuter `Release\ia_mems_diagram_selftest.exe`; MSBuild remonte **`-1073741515`** (`0xC0000135`, runtime DLL introuvable).
- `ia_mems_diagram_selftest` est lié à `Qt5::Core`. Au moment du `POST_BUILD`, le workflow n’a pas encore ajouté `C:\Qt\5.15.2\msvc2019_64\bin` au `PATH`.
- Le workflow possède déjà `Run deterministic self-tests`, qui ajoute le répertoire Qt au `PATH` puis exécute ce test. Cause réelle : **ordre d’exécution**, pas compilation du catalogue, protocole, Qwen/ONNX ou packaging.

## CORRECTION #93 — DIFF TEMPORAIRE VALIDÉ

- Branche temporaire : **`tmp-fix-93-diagram-selftest`**, créée exactement depuis `5ad520dc`.
- Commit candidat : **`39543d694507d4c01c16d54cf2f6b01f3043fd6f`** (`Fix diagram selftest execution order`).
- Comparaison `5ad520dc` → `39543d69` : **1 seul fichier**, `CMakeLists.txt`, **0 ajout / 2 suppressions**.
- Le patch supprime exactement le `add_custom_command(TARGET ${PNAME} POST_BUILD ...)` du test schéma.
- La cible `ia_mems_diagram_selftest`, son lien `Qt5::Core`, sa dépendance de build et son exécution explicite dans `Run deterministic self-tests` restent inchangés.
- Aucun autre fichier ni comportement modifié.

## BUILD #94 — VALIDATION COMPLÈTE ET CLÔTURE DE L’INCIDENT #93

- `MEMSX64` avancée sans force sur **`39543d694507d4c01c16d54cf2f6b01f3043fd6f`**.
- GitHub Actions : **#94 / v1.0.94 — SUCCESS**, run **`33068860732`**.
- `Validate protocol guards before build` : **SUCCESS**.
- `Configure and build ECU MEMS Manager x64 with native ONNX IA` : **SUCCESS**. L’échec de compilation apparent #93 est donc définitivement identifié comme un défaut d’ordre de lancement, pas un défaut du code schéma.
- `Run deterministic self-tests` : **SUCCESS** ; le nouveau `ia_mems_diagram_selftest` s’exécute donc correctement avec Qt au bon moment.
- Base expert r20 : **SUCCESS**.
- Modèle Qwen piné + hashes : **SUCCESS**.
- `Run production LocalAiClient native ONNX self-test` : **SUCCESS**.
- Assemblage package : **SUCCESS**.
- Validation package complet, incluant manifeste/SVG/Qt SVG : **SUCCESS**.
- `Validate packaged production LocalAiClient with Qwen` : **SUCCESS**.
- Smoke launch : **SUCCESS**.
- Manifest/hash package : **SUCCESS**.
- Upload artefact : **SUCCESS**.
- Artefact : ID **`9645083399`**, nom **`ECU-MEMS-Manager-x64-BUILD-94-v1.0.94`**, taille **386 779 885 octets**, SHA-256 **`11503728fc75dbbff8104d68f6876cdca82cc8f41e234c939e65b7913fe698db`**.
- **Incident #93 clos. Fonction de proposition/ouverture des schémas IA MEMS stabilisée sur #94.**

### VALIDATION UTILISATEUR #94 — FONCTION SCHÉMAS

- Test effectué sur le PC réel utilisateur avec **ECU MEMS Manager v1.0.94**.
- Le bouton de proposition de schéma apparaît correctement pour une demande explicite de brochage, notamment **« COULEUR DES FILS DE LA BROCHE ECU 1.6 »** → **« Ouvrir le schéma MEMS 1.6 ECU »**.
- Le clic et l’ouverture du schéma fonctionnent : **fonction schémas validée sur le PC réel**.
- Les SVG actuels sont jugés graphiquement perfectibles ; amélioration visuelle reportée à plus tard, sans toucher au mécanisme validé.
- Des réponses IA restent parfois à côté de la question (ex. contrôle sonde lambda routé vers « aucune mesure ECU disponible », couleurs de fils parfois incomplètes ou mélangeant les variantes). Ces défauts sont consignés pour un lot ultérieur **qualité/routage IA** ; ne pas modifier maintenant `LocalAiClient`, Qwen/ONNX ou la fonction schémas pour les traiter.

## RAVE — AUDIT DE COUVERTURE MARCHÉ / ANNÉE APRÈS #94

- Base de l’artefact #94 inspectée directement : **86 faits RAVE / 98 faits experts**.
- Répartition RAVE par variante : **40** `SPi_Japan_97MY_from_VIN_SAXXNNAXKBD_134455`, **26** `Mini_1997_2000`, **15** `MPi_97MY_from_VIN_SAXXNNAZEBD_134455`, **3** `MPi_1997_plus`, **2** `SPi_1997_plus`.
- Conclusion : la Mini **SPi classique non-Japon est nettement sous-couverte** par rapport à la SPi Japon ; la fréquence des réponses IA mentionnant le Japon est cohérente avec cette répartition documentaire et ne doit pas être corrigée en masquant les faits Japan.
- Les 86 faits RAVE portent déjà un millésime, une année ou une plage d’années dans leur variante ; le déficit principal concerne la **portée marché explicite** pour les faits non-Japon et le manque de faits constructeur sur les SPi 1991–1996.
- Le sommaire constructeur `RCL0194ENG` 97MY sépare lui-même `Engine Management System: MPi 20.1` et `SPi (JAPAN) 20.3`, avec une portée VIN `Japan only — SAXXNNAXKBD 134455` et `All other vehicles — SAXXNNAZEBD 134455`. Continuer 20.3/20.4 augmenterait donc mécaniquement la surreprésentation SPi Japon.
- Publication Rover prioritaire pour la SPi classique : **AKM6799 — Mini Single Point Injection 1991 to 1996**. `AKM7169` couvre les Mini tous modèles 1993 et suivants et fournit des pages SPi exploitables.

## RAVE — RÈGLE DE PORTÉE VARIANTE / ANNÉE / MARCHÉ

- Chaque nouveau fait Mini doit conserver **exactement la portée disponible dans la source constructeur** : type d’injection (SPi/MPi), millésime/année ou plage VIN, marché (UK/Europe/Japon/export/etc.) et, lorsqu’ils sont distingués, transmission, Cooper/non-Cooper, climatisation ou autre équipement.
- **Ne jamais inférer un marché** et ne jamais généraliser un fait spécifique à une autre variante.
- Si la source constructeur ne précise pas un axe, noter **`non précisé`** plutôt que de supposer UK, Europe, Japon ou « toutes Mini ».
- Les faits Japon doivent rester explicitement Japon dans la variante et dans le contexte restitué par IA MEMS ; ils ne doivent pas être présentés comme une vérité universelle SPi.
- Les sources secondaires (MEMS FCR, catalogues de pièces, sites techniques, forums) peuvent servir à orienter la recherche ou à signaler une divergence, mais **ne deviennent pas `verifie_constructeur`** sans recoupement avec une documentation Rover primaire correspondante.

## RAVE 1720 CLASSIC SPi — CANDIDAT TEMPORAIRE VALIDÉ

- Source constructeur exploitée : **Rover Group Limited `AKM7169ENG`, Mini Repair Manual, publication AKM7169, 1993**.
- Portée générale explicitement donnée par le manuel : Mini fabriquées à partir du VIN **`049349`**.
- Pages utilisées : `Engine Tuning Data 3` (SPi boîte manuelle), `Engine Tuning Data 4` (SPi automatique), `Engine Tuning Data 5` (SPi moteur haute compression).
- Les pages utilisées ne donnent pas de marché : portée enregistrée **`market_non_precise`** ; aucune supposition UK/Europe/Japon.
- 7 faits constructeur retenus : portée document/VIN, ECU manuel `MNE101040`, ECU automatique `MNE101060`, ECU haute compression `MNE101070`, principe speed/density, TPS 0–1 V fermé / 4–5 V ouvert, pression carburant 1,0 bar ±4,0 % constante.
- `High compression engine` reste tel quel : **ne pas le transformer automatiquement en Cooper**.
- La tolérance de pression AKM7169 reste distincte du fait RCL0193 1997+ `1,0 ±0,2 bar` ; ne pas fusionner les portées tant qu’elles ne sont pas recoupées.
- Branche temporaire : **`tmp-rave-1720-classic-spi`**, créée depuis exactement `39543d69` (#94 vert).
- HEAD candidat temporaire : **`f2e97b3e3a432785e159d30bbeca7b7bef2fdcb4`**.
- Payload : `database/reference/research_enrichment_1720.qz64` ; SQL décompressé **10086 octets**, SHA-256 `2b68af4bd031908a5a46f4f9acae65e673edb2ee17772e6afdc2c9d4cfe43bb9` ; qz64 **2161 octets**, SHA-256 `e9d91ad14edf8340131050df933459b05b889927a4310db5235c657ba4862e77`.
- Contrôle du blob GitHub après correction : **`ea933e26eacea5bcacbd62428e9d67b424de347d`**, identique au Git blob calculé sur le fichier local validé.
- Validation sur copie exacte de la DB r20 de l’artefact #94 : **86→93 RAVE**, **98→105 experts**, exactement 7 + 7 faits `SRC-AKM7169`, aucun `RAVE-SPI93-*` préexistant, tous `verifie_constructeur`, `PRAGMA integrity_check = ok`, `user_version = 20`.
- Diff final #94 `39543d69` → candidat : **exactement 3 fichiers** : ajout du qz64, ajout de `database/reference/audits/rave_1720_audit.md`, mise à jour de `database/reference/manifest.json`. Aucun code, moteur IA, protocole, ECU, UI ou 32 bits modifié.
- Les anciens candidats RCL0194 SPi Japon ne sont pas supprimés ; ils deviennent un lot Japon séparé ultérieur.

## BUILD #95 — VALIDATION DU LOT RAVE 1720 CLASSIC SPi

- `MEMSX64` avancée sans force sur **`f2e97b3e3a432785e159d30bbeca7b7bef2fdcb4`**.
- GitHub Actions : **#95 / v1.0.95 — SUCCESS**, run **`33076089248`**.
- Le diff #94 → #95 contient uniquement les 3 fichiers RAVE/base du lot 1720 ; **aucun fichier UI, responsive, IA, protocole ou code applicatif** n’a changé.
- Génération et validation de la base expert r20 : **SUCCESS**.
- Self-tests déterministes : **SUCCESS**.
- Production LocalAiClient native ONNX : **SUCCESS**.
- Production LocalAiClient packagée Qwen : **SUCCESS**.
- Package complet, smoke launch et upload artefact : **SUCCESS**.
- Artefact réel du run #95 : ID **`9648135346`**, nom **`ECU-MEMS-Manager-x64-BUILD-95-v1.0.95`**, taille **386 785 918 octets**, SHA-256 **`6491cd545d4770476f13ce31929aa665fabe15a6068cd28e5c2619cc1db444af`**.
- Inspection directe de la SQLite du ZIP #95 : **93 faits RAVE / 105 faits experts**, `PRAGMA integrity_check = ok`, `user_version = 20`.

### OBSERVATIONS UTILISATEUR SUR IA MEMS #95

- L’enrichissement SPi classique produit un effet visible : à la question générale sur la Mini SPi, l’IA remonte désormais les nouveaux faits AKM7169 (pression carburant, portée VIN, TPS) au lieu de partir immédiatement vers la SPi Japon.
- Cas de pertinence à corriger ultérieurement : **`C'est quoi le CKP ?`** peut produire une définition erronée malgré des faits CKP corrects présents dans la base ; la sélection/grounding doit être auditée avant toute correction.
- **Couleurs de fils de sonde de température** : réponse parfois méta ou incomplète au lieu de restituer directement les couleurs/broches compatibles.
- **`FICHE MEMS 1.3 XML` / `MEMS 1.3 XML`** : réponses hors sujet et observation d’une phrase ressemblant à une consigne interne dans la sortie utilisateur ; à auditer précisément avant correction, sans supposer la cause.
- L’utilisateur a observé un déplacement/chamboulement visuel des onglets/responsive sur #95. Contrôle du diff : #94 → #95 ne touche aucun fichier UI ; **ne pas qualifier cela de régression introduite par #95 sans preuve**. Point UI différé.
- Priorité active demandée par l’utilisateur : **base de données + RAVE + amélioration de la pertinence IA**. UI et qualité graphique des SVG seront traitées plus tard.

## AUDIT STRUCTUREL — LIMITATION DU CLASSEMENT ACTUEL DES FAITS

- Le nombre de tables n’est pas le problème : la base possède déjà de nombreuses tables spécialisées et une table de compatibilité `ecu_fitment` avec des axes séparés tels que injection, modèle, variante, transmission, moteur, marché, années et VIN.
- La limitation critique est au niveau des faits RAVE : une partie de la portée est aujourd’hui compactée dans le texte libre `variant` (ex. `SPi_Japan_97MY_from_VIN_...`). Cela est lisible par un humain mais insuffisamment structuré pour un filtrage déterministe.
- Les faits recopiés vers `mems_expert_fact_external` portent principalement `family`, `firmware_code`, `topic`, `statement`, `verification_level`, etc. Le `ExpertContext` utilisé par le moteur expert ne contient actuellement que **famille MEMS + firmware**.
- Conséquence : l’IA peut disposer d’un fait correct mais recevoir en même temps des faits d’une autre injection, année ou marché parce que ces dimensions ne sont pas encore des critères de compatibilité de premier niveau.
- **Décision utilisateur et projet : corriger le socle maintenant avant d’ajouter massivement AKM6799**, afin de ne pas devoir reclasser des centaines de faits supplémentaires plus tard.

## ÉTAPE STRUCTURANTE AUTORISÉE — SOCLE RAVE COMPLET ET ÉVOLUTIF

### Objectif
Construire un socle de connaissances durable capable d’intégrer **tout ce qui est utile dans RAVE**, pas uniquement l’ECU : électricité, diagnostic, mécanique, couples de serrage, procédures de dépose/repose, réglages, tolérances, capacités, lubrifiants, outillage, précautions, contrôles et spécifications.

### Principes obligatoires

1. **Aucune destruction de l’existant.** Les tables/faits actuels restent compatibles ; migration additive et contrôlée uniquement. Ne pas casser les générateurs r20, les faits RAVE existants ni les fonctions IA déjà validées.
2. **Séparer le fait de sa portée.** Le contenu technique ne doit plus dépendre d’une chaîne `variant` qui mélange injection, année, marché et VIN.
3. **Portée véhicule/moteur commune à tous les domaines.** Elle doit pouvoir représenter, lorsque la source les précise : marque, modèle, moteur, cylindrée, variante moteur, alimentation/injection (carburateur/SPi/MPi/etc.), famille ECU/MEMS si pertinente, année début/fin, VIN début/fin, marché, transmission, niveau de compression/variante, catalyseur, climatisation et autres équipements discriminants.
4. **Valeur non précisée ≠ valeur universelle.** Si Rover ne donne pas un marché, une transmission ou une année, conserver explicitement `non précisé` / NULL selon le schéma ; ne jamais transformer l’absence d’information en « toutes variantes ».
5. **Une même connaissance peut viser plusieurs portées.** Prévoir une relation plusieurs-à-plusieurs plutôt que recopier artificiellement le même fait.
6. **Traçabilité constructeur permanente.** Toute donnée doit pouvoir conserver source, document/publication, section/page, niveau de vérification, notes et éventuellement image/schéma associé.
7. **Extensible sans refaire le schéma à chaque nouveau manuel.** Le noyau doit accepter de nouveaux types de connaissance et de nouvelles portées sans multiplier des colonnes spécifiques à chaque cas particulier.

### Domaines de connaissance à supporter explicitement

- ECU / protocole / commandes / mémoire / firmware.
- Câblage / connecteurs / broches / couleurs de fils / masses / alimentations.
- Capteurs / actionneurs / valeurs de contrôle / formes de signal.
- Diagnostic / DTC / symptômes / contrôles / causes / solutions.
- Spécifications moteur et mécanique.
- **Couples de serrage** et séquences de serrage.
- Réglages, jeux, tolérances et limites d’usure.
- Capacités, fluides, lubrifiants et consommables.
- **Procédures de dépose, repose, démontage, remontage et réglage.**
- Étapes ordonnées d’une procédure.
- Outillage spécial et outils requis.
- Avertissements, précautions et conditions préalables.
- Contrôles après remontage / vérifications finales.
- Illustrations, schémas et références de figures lorsque disponibles.

### Structure mécanique minimale à prévoir

Pour une question telle que **« quel couple de serrage pour la culasse ? »**, la base doit pouvoir distinguer au minimum : composant, fixation/élément concerné, opération, valeur, unité, étape de serrage, angle complémentaire éventuel, ordre/séquence, condition d’application et portée véhicule/moteur. Une valeur multi-étapes ne doit pas être écrasée en une seule phrase ou un seul nombre.

Pour **« quelle procédure pour déposer la culasse ? »**, prévoir une entité procédure reliée à des **étapes ordonnées**, avec possibilité d’associer à chaque étape un avertissement, un outil, une valeur/couple, une figure ou une condition. Dépose et repose doivent pouvoir être distinctes et reliées.

### Routage IA visé

Le fonctionnement cible doit devenir :
**question utilisateur → type de connaissance demandé → portée véhicule/moteur → élimination déterministe des incompatibilités → classement par source/niveau de preuve/pertinence → grounding → Qwen.**

Exemples :
- question Mini SPi 1995 → ne pas fournir automatiquement des faits MPi ou Japon-only incompatibles ;
- question sur couple de culasse → chercher d’abord une spécification mécanique compatible, pas des faits ECU ;
- question de dépose culasse → fournir la procédure Rover ordonnée compatible ;
- si aucune valeur/procédure constructeur compatible n’existe, l’IA doit le dire clairement **et ne pas inventer** un couple ou une procédure.

### Migration des données existantes

- Ne pas reclasser aveuglément les 93 faits RAVE.
- Extraire les dimensions de `variant` uniquement lorsqu’elles sont déjà prouvées par la source/audit ; sinon laisser le champ correspondant non précisé.
- Conserver `variant` pendant la transition pour rétrocompatibilité et audit.
- Réutiliser les données structurées déjà présentes (`ecu_fitment`, véhicules, ECU, sources, etc.) lorsqu’elles sont fiables, au lieu de créer des doublons.
- Ajouter des tests d’intégrité et de compatibilité avant tout passage sur `MEMSX64`.
- Toute évolution du `user_version` SQLite doit être volontaire, documentée et testée ; ne pas augmenter le numéro uniquement pour marquer une étape.

### Contraintes pendant cette étape

- Travail sur **branche temporaire créée depuis #95** ; aucun changement direct `MEMSX64`.
- **Pas de #96** avant audit du schéma, prototype/migration contrôlée, self-tests et comparaison complète du diff.
- Ne pas toucher au protocole ECU, protections MEMS1.9, acquisition, calculs RAM, UI, responsive, schémas SVG, Qwen/ONNX ou paramètres de génération pendant la construction du socle.
- La recherche AKM6799 peut continuer en **lecture seule**, mais ne pas injecter massivement de nouveaux faits dans l’ancien classement pendant cette étape.
- Les anciens candidats Japon RCL0194 restent séparés et en attente.

## AUDIT TECHNIQUE DU SOCLE — TERMINÉ AVANT MODIFICATION SQL

- Branche temporaire **`tmp-rave-knowledge-foundation`** créée exactement depuis #95 `f2e97b3e`; `MEMSX64` n’a pas bougé et aucun #96 n’a été déclenché.
- Le générateur r20 est additif : seeds historiques puis application ordonnée des `research_enrichment_*.qz64`; un nouveau lot peut donc créer le socle sans réécrire les anciens lots.
- Le workflow #95 vérifie déjà `PRAGMA integrity_check` et `user_version=20`; les nouveaux self-tests devront ajouter les invariants de portée/procédures sans affaiblir ces contrôles.
- SQLite #95 : 63 tables métier + table SQLite interne, 93 RAVE, 105 experts, intégrité `ok`, user_version 20.
- `mems_rave_fact` : `fact_key, source_key, document, variant, topic, statement, source_section, verification_level, image_ref, notes`.
- `mems_expert_fact_external` : `source_key, fact_key, family, firmware_code, topic, statement, verification_level, notes`.
- `ecu_fitment` possède déjà injection, modèle, transmission, moteur, marché, année et VIN, mais est attachée aux références ECU : elle sera réutilisée lorsque possible sans devenir le socle mécanique général.
- Aucune table dédiée aux procédures, couples, outils ou avertissements n’existe actuellement. Les valeurs/procédures mécaniques RAVE déjà présentes seront conservées comme preuves brutes puis structurées progressivement.

## SCHÉMA EXACT VALIDÉ AVANT MIGRATION

Le schéma SQL exact, les colonnes, relations, index, règles de compatibilité, stratégie de migration et self-tests sont figés dans le fichier **`RAPPORT_SCHEMA_SOCLE_RAVE_EXACT.md`** sur la branche `RAPPORT`, créé au commit **`9100c1c535924cba4cee84f987ca54d137e3e49e`**.

Ce fichier fait partie intégrante du rapport de continuité et doit être lu **immédiatement après le présent rapport** avant toute modification de la base.

Le socle retenu comporte 11 tables additives :
1. `mems_applicability_scope` — portée véhicule/moteur/système ;
2. `mems_scope_constraint` — exclusions, ANY explicite et dimensions futures ;
3. `mems_knowledge_item` — connaissance atomique avec provenance ;
4. `mems_knowledge_scope` — relation plusieurs-à-plusieurs fait ↔ portée ;
5. `mems_specification` — en-tête couple/réglage/pression/capacité/etc. ;
6. `mems_specification_value` — valeurs, plages, tolérances et séquences multi-étapes ;
7. `mems_procedure` — dépose/repose/réglage/inspection/etc. ;
8. `mems_procedure_step` — étapes ordonnées avec lien possible vers une spécification ;
9. `mems_procedure_requirement` — avertissement, outil, consommable, prérequis, post-contrôle ;
10. `mems_knowledge_relation` — relations/conflits/liaisons entre connaissances ;
11. `mems_term_alias` — synonymes et terminologie multilingue pour routage déterministe.

### Règle de compatibilité figée

Pour chaque dimension connue de la question, le futur filtre produit trois états :
- **EXACT** : portée explicitement compatible ;
- **UNKNOWN** : la source ne précise pas cette dimension, donc candidat secondaire seulement ;
- **INCOMPATIBLE** : portée explicitement opposée, donc élimination avant grounding/Qwen.

**NULL = UNKNOWN, jamais ANY.** Une universalité doit être explicitement documentée avec une contrainte `operator=any`.

### Migration figée

- Premier lot prévu : **`research_enrichment_1730.qz64`**, tables + index uniquement au départ.
- Aucun historique supprimé ou renommé.
- Les 93 `mems_rave_fact` et 105 `mems_expert_fact_external` doivent rester inchangés.
- Après validation du schéma seul : créer exactement un miroir `mems_knowledge_item` par fait RAVE historique avec `legacy_rave_fact_key`.
- Les portées ne sont remplies qu’avec des dimensions déjà prouvées par source/audit ; aucune déduction libre depuis le nom `variant`.
- Conversion mécanique pilote manuelle/explicite avant toute extraction de masse : quelques couples simples + au moins une vraie procédure dépose/repose.
- Aucun lecteur IA ne bascule sur le nouveau socle tant que couverture, compatibilité et self-tests ne sont pas validés.
- Après validation du socle, reprendre AKM6799 et injecter directement les nouveaux faits dans cette structure au lieu d’accumuler des variantes textuelles.

## PROCHAINE ACTION EXACTE — HISTORIQUE AVANT #96

Cette section est conservée comme **point de reprise historique** tel qu’il existait à la fin de #95. Elle a depuis été exécutée et est remplacée par les sections #96/#97 ci-dessous.

Ordre historique prévu :
1. lire `RAPPORT_SUIVI_ECU_MEMS_MANAGER.md` sur `RAPPORT` ;
2. lire immédiatement `RAPPORT_SCHEMA_SOCLE_RAVE_EXACT.md` sur `RAPPORT` ;
3. vérifier que `MEMSX64` est toujours #95 `f2e97b3e3a432785e159d30bbeca7b7bef2fdcb4` ;
4. reprendre exclusivement `tmp-rave-knowledge-foundation` ;
5. construire `research_enrichment_1730.qz64` avec les **11 tables + index uniquement**, sans suppression ni modification des tables historiques ;
6. reconstruire localement la base r20 et vérifier au minimum 93 RAVE / 105 experts / `integrity_check=ok` / `user_version=20` ;
7. ajouter le self-test de fondation ;
8. seulement ensuite migrer les 93 faits et leurs portées prouvées ;
9. ne pas avancer `MEMSX64` et ne pas lancer #96 avant validation complète du candidat temporaire.

AKM6799 restait alors en recherche lecture seule. Les anciens candidats Japon RCL0194 restaient séparés et en attente.

# HISTORIQUE AJOUTÉ — DU DERNIER POINT DE RAPPORT À BUILD #96

## EXÉCUTION DU SOCLE RAVE / BASE UNIFIÉE

Le point de reprise #95 a été exécuté sur branche temporaire, sans toucher au 32 bits ni au protocole ECU.

### `research_enrichment_1730.qz64`
- Mise en place additive du **socle de connaissances RAVE complet** défini dans `RAPPORT_SCHEMA_SOCLE_RAVE_EXACT.md`.
- Création des **11 tables** prévues et des index associés, sans suppression ni renommage des tables historiques.
- Conservation de `user_version=20`.
- Règle conservée : `NULL = UNKNOWN`, jamais ANY ; universalité uniquement si elle est explicitement prouvée.

### `research_enrichment_1740.qz64`
- Migration contrôlée des **93 faits RAVE historiques** dans le nouveau socle.
- Un miroir `mems_knowledge_item` par fait RAVE historique avec traçabilité vers le fait legacy.
- Portées remplies uniquement lorsque les dimensions sont prouvées par la source/audit ; aucune généralisation libre depuis le nom de variante.
- Aucun fait historique supprimé.

### Lecteurs / moteur expert adaptés au nouveau socle
Le diff #95 → #96 représente **16 commits** et exactement 10 fichiers finaux modifiés/ajoutés :
- `database/reference/manifest.json` ;
- `database/reference/research_enrichment_1730.qz64` ;
- `database/reference/research_enrichment_1740.qz64` ;
- `expert/ExpertKnowledgeReader.cpp` ;
- `expert/ExpertKnowledgeReader.h` ;
- `expert/ExpertRuntimeDatabase.cpp` ;
- `expert/IaMemsDiagramCatalog.cpp` ;
- `expert/IaMemsService.cpp` ;
- `tools/validate_rave_knowledge_foundation.py` ;
- `tools/validate_unified_ia_knowledge.py`.

Fonctions ajoutées/étendues :
- `ExpertKnowledgeReader` conserve la lecture historique et ajoute la lecture générique du nouveau socle, des spécifications/procédures et des portées explicites.
- `ExpertRuntimeDatabase` découvre automatiquement les futurs `research_enrichment_*.qz64` par suffixe numérique, tout en excluant les lots archive-only.
- `IaMemsDiagramCatalog` devient un résolveur générique basé sur le manifeste, avec protection contre l’ambiguïté et les chemins non sûrs.
- `IaMemsService` ajoute le filtrage déterministe des portées (SPi/MPi, Japon/UK/Europe, boîte auto/manuelle, compression, etc.) avant classement et Qwen, ainsi que des protections contre la réutilisation d’un fait explicitement incompatible.
- Les termes de brochage génériques ne suffisent plus à inventer une variante ; les faits absents doivent conduire à une réponse contrôlée.
- Le classement a été durci pour éviter des collisions de tokens, notamment ECT/injecteur.

### Validation fondation / unification
- `validate_rave_knowledge_foundation.py` vérifie l’intégrité du nouveau socle, sa migration et les règles de portée.
- `validate_unified_ia_knowledge.py` valide le chemin unifié vers les 93 faits, les assets et des recherches représentatives.
- Validation déterministe obtenue avant push : **93 knowledge items**, assets locaux détectés, ECT priorisé, MAP MPi priorisé, SPi MAP non inventé lorsqu’aucun pinout constructeur compatible n’est disponible.

### Correction de compilation pendant le candidat #96
- MSVC a signalé un `C2664` sur un appel à `joinedNonEmpty` utilisant `QLatin1Char('\n')` alors que la signature attendait un `QString`.
- Correction minimale : `QStringLiteral("\n")`.
- Commit de correction connu : **`e1d402477f35f42e4cc5341bd2e0a32535e0277f`**.
- Aucun changement fonctionnel protocole/ECU lié à cette correction.

## BUILD #96 — SOCLE RAVE + CONNAISSANCE UNIFIÉE

- `MEMSX64` avancée après validation du candidat.
- HEAD #96 : **`fea27058773fc3535c3ea40f7b9a36151b792ac1`** (`Remove temporary RAVE validation workflow`).
- GitHub Actions : **#96 / v1.0.96 — SUCCESS**, run **`33097323011`**, démarré le 27/08/2026 à 17:13:45Z et terminé à 17:20:07Z.
- Artefact : ID **`9657131155`**, **`ECU-MEMS-Manager-x64-BUILD-96-v1.0.96`**, 386 836 238 octets, SHA-256 **`a9677cdcead198e36f8fbcf9efccc6d55e493bede88c545683abaeffd8ca4124`**.
- Compilation x64 MSVC, protections protocole, self-tests déterministes, base expert r20, Qwen3-0.6B ONNX INT4, LocalAiClient production, package, validation package et smoke launch : **SUCCESS**.
- Le workflow de validation temporaire RAVE a été retiré avant le HEAD de production.

## VALIDATION UTILISATEUR RÉELLE BUILD #96

Une série de **28 captures** et un journal TXT réel ont été analysés. Résolution de toutes les captures : **1366 × 697** ; cette résolution est donc validée, pas encore l’ensemble des résolutions responsive.

### ECU / acquisition — validé sur véhicule réel
- Connexion réelle ECU : **VALIDÉE**.
- Identification affichée : **`AANMP002 — MNE101150`**.
- Port : **COM3 — FTDI FT232 — ROSCO 1.3/1.6**.
- D0 : `98 00 02 02` ; D1 : `AANMP002` répété en ASCII.
- Live 7D/80 : **VALIDÉ**.
- RAM Injection Mode 4 : **VALIDÉE**.
- Retour Mode 4 → diagnostic : cohérent.
- Enregistrement TXT : **VALIDÉ**.
- Valeurs live globalement cohérentes pendant le test réel : régime, liquide, MAP, batterie, TPS, lambda, ralenti, dwell.
- Valeurs brutes `255` de températures ambiante/carburant affichées comme `200 °C` : affichage sentinelle trompeur à traiter plus tard.
- Dwell observé autour de 3,23–3,30 ms : le warning actuel 1,9–3,1 ms à ~14 V doit être revu séparément avec source avant toute modification ; ne pas conclure à une panne ECU sur cette seule base.

### UI — #96
- Aucun problème nouveau majeur observé dans l’environnement/onglets sur les captures.
- Styles dark et structures principales cohérents.
- La validation multi-résolution reste à faire ultérieurement ; seule la résolution 1366 × 697 a été réellement observée dans cette série.

### IA — défauts réels découverts dans #96
Malgré les validateurs offline verts, la production UI présentait un défaut de routage important :
- `Broche MAP Mini MPi 1997` pouvait répondre **« Je n'ai encore aucune mesure ECU disponible. »** ;
- `Broche MAP Mini SPi Japan 1997` pouvait subir le même détournement ;
- `Couleur des fils sonde lambda ?` pouvait être interprété comme une demande de mesure lambda ;
- `schema sonde broche ecu 1.3` pouvait faire ressortir une phrase de type instruction interne ;
- le schéma MEMS 1.6 pouvait être trouvé lorsque l’utilisateur connaissait déjà le nom exact `mems_1_6_ecu_connector.svg`, mais l’IA ne guidait pas suffisamment l’utilisateur vers ce document ;
- les réponses ECT pouvaient contenir un fait IAT non demandé ou des doublons.

### Cause exacte du détournement MAP/lambda identifiée dans le code #96
Dans `IaMemsService::groundingFor(...)`, le chemin faisait d’abord :
`IaResponseLogic::classify(question)` → `metricAnswer(...)`, puis seulement plus tard `knowledgeAnswer(question)`.

`IaResponseLogic::classify()` classait la simple présence de `map`, `lambda`, etc. comme une intention de mesure live, sans exiger `valeur`, `mesure`, `actuel`, etc. Une question documentaire contenant MAP/lambda était donc interceptée **avant** la recherche documentaire.

C’est la cause concrète des réponses « aucune mesure ECU disponible » sur des demandes de broche/couleur.

## DÉCOUVERTE / VÉRIFICATION DES FICHES XML QZ64 AVANT #97

Les fiches documentation sont bien présentes dans le package/base :
- `database/reference/fiches/mems_1_2.xml.qz64`
- `database/reference/fiches/mems_1_3.xml.qz64`
- `database/reference/fiches/mems_1_6.xml.qz64`
- `database/reference/fiches/mems_1_9.xml.qz64`

Le `manifest.json` mappe explicitement :
- `MEMS 1.2` → `fiches/mems_1_2.xml.qz64`
- `MEMS 1.3` → `fiches/mems_1_3.xml.qz64`
- `MEMS 1.6` → `fiches/mems_1_6.xml.qz64`
- `MEMS 1.9` → `fiches/mems_1_9.xml.qz64`

Il existe également des variantes de langue `.de/.en/.es/.it/.pt.xml.qz64`.

L’onglet **Base de données** sait déjà retrouver ces fiches et proposer **« Fiche XML complète »**. Une capture utilisateur a confirmé `MEMS 1.3 XML — Génération 1.3 — Documentation`. Conclusion avant #97 : le document existe et le programme sait l’ouvrir, mais **IA MEMS ne savait pas encore exploiter/proposer proprement la documentation XML de manière naturelle**.

# HISTORIQUE BUILD #97 — ROUTAGE CONVERSATIONNEL / CLARIFICATION / XML

## OBJECTIF AUTORISÉ PAR L’UTILISATEUR

Corriger la couche d’orchestration IA sans toucher au protocole ECU ni au 32 bits :
1. donner priorité aux intentions documentaires (`broche`, `câblage`, `couleur`, `schéma`, `connecteur`, `XML`, `fiche`, `documentation`, `couple`, etc.) avant les mesures live ;
2. ajouter une clarification lorsque l’information manquante change réellement la réponse ;
3. conserver la question précédente pour comprendre une réponse courte telle que `MPi`, `SPi` ou `cherche` ;
4. si l’utilisateur répond `cherche`, exploiter le contexte ECU/firmware/base avant de demander à nouveau une information ;
5. proposer les fiches XML MEMS 1.2/1.3/1.6/1.9 comme ressources ouvrables depuis l’onglet IA ;
6. préserver `Valeur MAP ?` comme vraie demande de mesure ECU ;
7. ne jamais recycler un brochage MPi pour une SPi Japan lorsque la donnée constructeur manque.

## BRANCHE TEMPORAIRE #97

- Branche : **`tmp-ia-conversation-routing`**.
- Créée exactement depuis #96 **`fea27058773fc3535c3ea40f7b9a36151b792ac1`**.
- Aucun changement initial sur `MEMSX64`.
- Un workflow et un applicateur temporaires ont été utilisés uniquement pour la validation, puis supprimés avant le HEAD final.

### Diff final #96 → candidat #97
Exactement **6 fichiers fonctionnels** restent dans le diff final :
- `expert/IaMemsConversationRouting.h` — nouveau ;
- `expert/IaMemsService.cpp` ;
- `expert/IaResponseLogic.h` ;
- `expert/IaResponseLogicTest.cpp` ;
- `iamemstab.cpp` ;
- `iamemstab.h`.

Aucun fichier protocole, acquisition, RAM, 32 bits, base RAVE ou Qwen/ONNX n’a été modifié par ce lot.

### Fonctions introduites dans le candidat #97
- Reconnaissance d’une intention documentaire avant le détournement vers une métrique live.
- Clarification déterministe SPi/MPi pour une demande ambiguë comme `Broche MAP Mini`.
- Conservation d’une requête en attente pour pouvoir reprendre après `MPi`, `SPi` ou `cherche`.
- Traitement spécial de `cherche` visant à utiliser d’abord les informations déjà disponibles.
- Proposition d’un bouton **`Ouvrir la fiche XML MEMS X.X`** en réutilisant le mécanisme existant de `MemsReferenceDatabase` plutôt qu’un nouveau décodeur QZ64.
- Déduplication/filtrage supplémentaire de résultats documentaires.
- Tests ajoutés pour MAP/lambda/ECT et distinction documentaire/mesure.

### Validation temporaire avant production
- Configuration MSVC x64 : **VERTE**.
- Compilation complète `ecu_mems_manager` x64 : **VERTE**.
- Tests IA existants : **VERTS**.
- Tests de schémas : **VERTS**.
- Tests recherche base MEMS : **VERTS**.
- Nouveaux tests de routage MAP/lambda/ECT : **VERTS**.
- Clarification SPi/MPi : **VERTE**.
- Reconnaissance de `cherche` : **VERTE**.
- Identification documentation MEMS 1.9 : **VERTE**.
- Après validation, le workflow et le script d’application temporaires ont été supprimés.

## PUSH ET BUILD #97

- `MEMSX64` avancée en **fast-forward sans force** sur **`2b211554abdbb127fd4d472f9ce687394b2d4608`**.
- GitHub Actions : **#97 / v1.0.97 — SUCCESS**, run **`33107904830`**.
- Run démarré le 27/08/2026 à 19:19:43Z, terminé à 19:26:41Z.
- Artefact : ID **`9661453251`**, nom **`ECU-MEMS-Manager-x64-BUILD-97-v1.0.97`**, taille **386 847 391 octets**, SHA-256 **`7bf9ea11fb5ca7c8f7d3a500efe5bf04fa94e16d38a8a25d5c0aa972f41b14c7`**.
- Build vert = compilation/tests/package validés ; **cela ne vaut pas validation définitive de la qualité conversationnelle réelle**, qui doit être contrôlée sur le PC utilisateur.

# TEST RÉEL UTILISATEUR BUILD #97 — CAPTURES DU 27 AOÛT 2026

L’utilisateur a testé #97 sans pouvoir connecter le véhicule à cet instant et a fourni une série de captures de l’onglet IA MEMS.

## UI / ENVIRONNEMENT
- **Aucun changement constaté** au niveau de l’environnement et des onglets par rapport à l’état précédent.
- Version visible : **v1.0.97**.
- Aucun problème de responsive nouveau identifié dans cette série de captures.

## TEST 1 — `Broche MAP Mini` puis `cherche`

### Ce qui fonctionne
- `Broche MAP Mini` déclenche correctement une clarification : **« est-ce une SPi ou une MPi ? »**.
- Le message propose aussi de répondre `cherche` si l’utilisateur ne sait pas.
- C’est le comportement conversationnel demandé.

### Défaut critique découvert
Après `cherche`, IA MEMS répond :
**« J’ai identifié SPi à partir des informations disponibles. Je poursuis la recherche initiale. »**

Or, dans ce test :
- l’écran indique **ECU non connecté** ;
- aucune preuve visible ne justifie cette conclusion SPi ;
- l’utilisateur lui-même ne sait pas sur quelle information ce choix repose.

**Conclusion : le mécanisme `cherche` existe mais son choix SPi n’est pas acceptable sans justification vérifiable.** Il faut auditer exactement la source de cette conclusion. Si plusieurs possibilités subsistent, l’IA doit le dire et demander l’information discriminante au lieu de choisir.

## FUITE DE CONSIGNES INTERNES — DÉFAUT MAJEUR #97

Après certaines questions/réponses, des textes destinés au guidage interne apparaissent dans l’interface utilisateur, notamment :
- **« La réponse attendue est la suivante »** ;
- **« Réponse attendue : diagnostic bref, hypothèses les plus probables dans l’ordre, puis contrôles prioritaires. Ne montre aucun raisonnement interne. »** ;
- **« La réponse attendue est une critique bref »** ;
- des formulations méta du type **« la réponse est donnée par la mention des faits fournis par MEMS Manager… »**.

C’est **non acceptable**. Les consignes internes ne doivent jamais devenir la réponse utilisateur.

Ce défaut explique plusieurs échecs visibles de lambda et ECT malgré un routage documentaire amélioré. Les tests automatiques #97 n’ont pas suffisamment vérifié **le texte final réellement affiché après passage dans Qwen**.

## TEST 2 — `Broche MAP Mini MPi 1997`

### Amélioration validée
La requête n’est plus détournée vers « aucune mesure ECU ».

Le moteur remonte bien un fait constructeur MPi pertinent :
- capteur MAP MPi ;
- connecteur **C186** ;
- fils **RG/YP/KB** ;
- ECU **C159-36**, **C159-8** ;
- retour commun capteurs **C159-13** ;
- `C159-8` identifié comme voie MAP et `C159-13` comme SENSOR EARTH.

### Défaut restant
La même réponse ajoute ensuite un fait **SPi Japan** alors que la question demande explicitement **MPi**.

**Conclusion : la recherche documentaire MAP MPi fonctionne enfin, mais le filtrage de portée SPi/MPi n’est pas encore strict jusqu’au texte final.** Un fait explicitement incompatible ne doit pas entrer dans la réponse finale.

## TEST 3 — `Broche MAP Mini SPi Japan 1997`

### Point positif
- #97 ne recycle pas directement le brochage MAP MPi comme s’il était SPi Japan.

### Défaut
- La réponse finale est une phrase générique/méta et n’apporte pas une réponse technique utile.

### Règle à conserver
S’il n’existe pas de brochage MAP SPi Japan vérifié dans le socle, la bonne réponse doit être contrôlée et claire : **aucune donnée constructeur compatible trouvée**, sans utiliser les broches MPi.

## TEST 4 — `Couleur des fils sonde lambda`

### Amélioration
- La question n’est plus transformée en demande de valeur lambda live / « aucune mesure ECU ».

### Défaut
- Le texte final est remplacé par la consigne interne **« Réponse attendue : diagnostic bref… »** au lieu de restituer les faits de câblage/couleurs disponibles.

**Conclusion : routage documentaire amélioré, génération finale non fiable.**

## TEST 5 — `Couple de serrage sonde température ECT`

- La réponse attendue techniquement doit exploiter la donnée ECT constructeur compatible (le cas de test connu est **15 Nm**).
- #97 affiche **« La réponse attendue est une critique bref »**.
- Le défaut n’est donc plus seulement le classement ECT/IAT : **la réponse finale générée masque le fait exact**.

## TESTS 6/7/8 — DOCUMENTATION XML MEMS 1.6 ET 1.9

### Fonction réussie
- `Je cherche la documentation MEMS 1.6` fait apparaître le bouton **`Ouvrir la fiche XML MEMS 1.6`**.
- Le bouton ouvre effectivement la fiche sur le PC utilisateur.
- `Je cherche la documentation MEMS 1.9` fait apparaître le bouton **`Ouvrir la fiche XML MEMS 1.9`**.
- La fiche 1.9 s’ouvre également.

### Contenu/rendu à vérifier
L’utilisateur signale :
- pour la fiche 1.6, il s’attend à retrouver **toutes les broches ECU et les couleurs de fils** présentes dans la fiche d’origine, mais ce n’est pas ce qu’il observe dans la fiche ouverte ;
- pour la fiche 1.9, les **descriptions/codes de couleurs** semblent présentes, mais les couleurs ne paraissent pas réellement représentées visuellement comme dans la fiche d’origine.

**Ne pas corriger avant audit.** Il faut comparer :
1. contenu XML/QZ64 réellement embarqué ;
2. contenu de la fiche d’origine ;
3. décodage par `MemsReferenceDatabase` ;
4. rendu par le viewer.

Il faut déterminer si l’information manque dans le fichier QZ64 ou si elle est perdue/neutralisée uniquement lors de l’affichage.

## TEST 9 — `Valeur MAP ?`

Le véhicule n’était pas connectable lors du test. #97 répond :
**« Je n’ai encore aucune mesure ECU disponible. »**

C’est **le comportement correct ECU déconnecté** et cela confirme que la correction documentaire n’a pas supprimé le chemin mesure live.

# BILAN TECHNIQUE BUILD #97

## Réussites
- Build x64 complet vert.
- Pas de régression environnement/onglets constatée.
- `Broche MAP Mini` déclenche enfin une clarification SPi/MPi.
- La réponse courte `cherche` est reconnue et reprend la recherche précédente.
- Une question documentaire MAP n’est plus systématiquement détournée vers la mesure MAP live.
- Une question documentaire lambda n’est plus systématiquement détournée vers la mesure lambda live.
- Le fait MAP MPi constructeur est effectivement retrouvable et affichable.
- Le chemin mesure live (`Valeur MAP ?`) est conservé.
- Les fiches XML MEMS 1.6 et 1.9 peuvent être proposées et ouvertes depuis IA MEMS sans connaître le nom de fichier.

## Défauts ouverts — ordre de priorité
1. **Fuite de consignes internes dans la réponse utilisateur** : priorité critique.
2. **`cherche` choisit SPi sans preuve/journalisation suffisante** : auditer la provenance et empêcher toute conclusion non justifiée.
3. **Filtrage de portée final insuffisant** : une question MPi peut encore inclure un fait SPi Japan.
4. **Réponses factuelles exactes trop dépendantes de Qwen** : un fait déterministe disponible (broche, couleur, couple, référence) peut être remplacé par une phrase méta/instruction interne.
5. **ECT** : doit restituer directement le fait constructeur compatible et exclure IAT lorsque ECT est explicite.
6. **Lambda couleurs** : doit restituer les couleurs/broches disponibles, pas une instruction de diagnostic.
7. **SPi Japan MAP** : réponse contrôlée utile si aucune donnée vérifiée compatible n’existe.
8. **Fiches XML** : vérifier exhaustivité 1.6 et représentation visuelle des couleurs 1.9 par comparaison source → QZ64 → viewer.
9. **Tests automatiques** : ajouter des tests end-to-end sur le texte utilisateur final, pas seulement sur classification/routage.

# ORIENTATION DE CORRECTION RETENUE APRÈS TEST #97

Ne pas refaire l’architecture ni changer de modèle IA. Le socle contient déjà de nombreuses données utiles. La priorité est de fiabiliser la chaîne **question → sélection déterministe → réponse utilisateur**.

Pour les requêtes factuelles vérifiables telles que broche, couleur de fil, couple de serrage, référence, fiche ou schéma :
- si le moteur expert possède un fait exact compatible et vérifié, **produire d’abord une réponse déterministe sûre** ;
- Qwen peut servir à expliquer/reformuler ensuite si cela apporte quelque chose, mais **ne doit pas pouvoir remplacer ou masquer la donnée factuelle vérifiée** par une consigne interne ou une phrase générique ;
- les incompatibilités SPi/MPi/Japon/année/transmission doivent être éliminées avant le texte final ;
- si l’information manque réellement, répondre clairement qu’elle n’est pas vérifiée/disponible pour cette variante au lieu d’inventer.

# PROCHAINE ACTION EXACTE — APRÈS BUILD #97

**Ne rien pousser avant cet audit. Ne pas toucher au protocole ECU ni au 32 bits. Partir strictement de `MEMSX64` #97 `2b211554abdbb127fd4d472f9ce687394b2d4608`.**

Ordre obligatoire :
1. **Auditer le chemin `cherche`** dans `IaMemsConversationRouting` / `IaMemsTab` / contexte service et déterminer exactement pourquoi le test réel ECU déconnecté conclut SPi. Identifier la source utilisée et définir une règle de preuve avant toute correction.
2. **Auditer la fuite de consignes internes** : retrouver dans `IaMemsService` / `LocalAiClient` / prompt/grounding la chaîne exacte qui produit `La réponse attendue...`, `Diagnostic bref...`, `Ne montre aucun raisonnement interne`, etc. Ne pas supprimer aveuglément du texte ; corriger la séparation instruction/contenu et ajouter un garde testable.
3. **Auditer le filtrage final de portée** sur `Broche MAP Mini MPi 1997` afin de comprendre pourquoi un fait SPi Japan survit encore après un fait MPi exact.
4. **Auditer les réponses factuelles ECT et lambda** avec les faits exacts actuellement disponibles. Vérifier la sélection avant Qwen et le texte final après Qwen.
5. **Auditer les fiches XML 1.6 et 1.9** : décoder/inspecter le contenu QZ64 avec le mécanisme réel du programme, comparer aux fiches sources et au rendu. Vérifier précisément brochage complet et représentation des couleurs.
6. Sur branche temporaire créée depuis #97, préparer une correction minimale et générale : réponse déterministe pour faits exacts, filtre de portée strict, `cherche` fondé sur preuve, aucun leak interne, sans changement protocole/ECU/32 bits.
7. Ajouter des **tests end-to-end du texte final affiché** pour les scénarios réels :
   - `Broche MAP Mini` → clarification ;
   - `cherche` ECU déconnecté sans preuve → ne pas choisir SPi/MPi arbitrairement ;
   - `Broche MAP Mini MPi 1997` → MAP MPi, aucun fait SPi ;
   - `Broche MAP Mini SPi Japan 1997` → donnée compatible ou indisponibilité contrôlée, jamais MPi recyclé ;
   - `Couleur des fils sonde lambda` → faits de câblage, aucune consigne interne ;
   - `Couple de serrage sonde température ECT` → donnée ECT compatible, aucune IAT et aucune consigne interne ;
   - `Je cherche la documentation MEMS 1.6/1.9` → bouton XML correct ;
   - `Valeur MAP ?` → mesure live si disponible, message d’absence si ECU déconnecté.
8. Compiler/valider sur GitHub Actions en branche temporaire, comparer le diff, puis seulement proposer un nouveau push `MEMSX64`.

**État de référence à conserver : BUILD #97 est la base active. Le build est vert et apporte de vraies améliorations de routage/clarification/XML, mais le lot IA n’est pas considéré terminé tant que les défauts de réponse finale ci-dessus ne sont pas corrigés et retestés sur le PC réel.**

# AUDIT ET CORRECTION APRÈS TEST RÉEL #97 — CANDIDAT VALIDÉ AVANT PRODUCTION

## POINT DE DÉPART

- Base obligatoire : **`MEMSX64` BUILD #97**, HEAD **`2b211554abdbb127fd4d472f9ce687394b2d4608`**.
- Branche de travail créée depuis exactement #97 : **`tmp-ia-post97-audit-fix`**.
- Aucun changement protocole ECU, acquisition, RAM, 32 bits ou protections de connexion dans ce lot.
- Objectif : corriger uniquement les défauts observés dans les captures #97 : `cherche` concluant SPi sans preuve suffisante, fuite de consignes internes, mélange MPi/SPi, réponses factuelles ECT/lambda et rendu incomplet des fiches XML.

## CAUSE 1 — `cherche` POUVAIT CONCLURE SPi SANS PREUVE SUFFISANTE

L’audit du code a montré que le chemin de résolution SPi/MPi utilisait une recherche générique sur **`Mini`** lorsque le firmware n’était pas disponible. Si les résultats renvoyés à cet instant ne contenaient qu’un type d’injection, ce résultat pouvait être traité comme une preuve et produire **« J’ai identifié SPi »**, y compris ECU déconnecté.

Correction retenue :
- le mot générique `Mini` n’est plus une preuve SPi/MPi ;
- les sondes de preuve sont limitées à des éléments forts : **firmware ECU réellement connu lorsque l’ECU est connecté** et/ou **référence ECU explicite présente dans la question** (`MNE`, `MKC`, `NNN`, `AANMP...`) ;
- si aucune preuve forte ne permet de trancher, la réponse reste contrôlée et demande l’information discriminante au lieu de choisir arbitrairement.

## CAUSE 2 — FUITE `RÉPONSE ATTENDUE / DIAGNOSTIC BREF / NE MONTRE AUCUN RAISONNEMENT INTERNE`

La cause exacte a été retrouvée dans `LocalAiClient` : le grounding documentaire contient naturellement des mentions de **niveau de preuve**. Le code utilisait la présence du mot **`preuve`** dans le grounding comme un signal suffisant pour passer en génération de type diagnostic et ajouter au prompt des consignes telles que **`Réponse attendue : diagnostic bref...`**. Le petit modèle pouvait alors recopier cette consigne dans la réponse utilisateur.

Correction retenue :
- suppression de la règle `grounding contient preuve => diagnostic` ;
- ajout d’une décision testable `shouldUseDiagnosticGeneration(question, grounding)` basée sur une vraie intention de diagnostic/raisonnement, pas sur les métadonnées de vérification d’un fait documentaire ;
- une question factuelle telle que **couple ECT**, **couleur de fils lambda**, **broche**, **fiche** ou **documentation** ne doit plus entrer dans ce mode uniquement parce que le fait comporte `preuve : constructeur` ;
- les vrais diagnostics conservent le chemin génératif diagnostic.

## CAUSE 3 — UN FAIT SPi POUVAIT SURVIVRE À UNE QUESTION MPi

Le filtre de portée existant s’appuyait surtout sur les nouvelles notes structurées contenant `Portee`. Certains faits historiques/legacy portent pourtant leur variante explicitement dans le `topic`, le `statement`, la clé, les notes ou la famille sans avoir encore cette note structurée.

Correction retenue dans `IaMemsService` :
- détection d’un label explicite SPi/MPi dans l’ensemble des champs du fait ;
- une demande MPi rejette un fait explicitement SPi s’il n’est pas également explicitement MPi ;
- une demande SPi rejette symétriquement un fait explicitement MPi ;
- le même durcissement est appliqué aux incompatibilités explicites de marché et de transmission avant le classement/Qwen ;
- le filtre structuré `Portee` reste conservé pour les nouveaux faits du socle.

## CAUSE 4 — LES FICHES XML CONTENAIENT LES BROCHES/COULEURS MAIS LE VIEWER IA LES JETAIT

Audit direct des QZ64 réels :
- **MEMS 1.6 contient bien 25 broches côté ECU**, en plus des éléments ROSCO, et des représentations de couleurs intégrées ;
- les couleurs sont stockées avec de vrais **SVG intégrés** dans le XML ;
- **MEMS 1.9 contient également des SVG de couleurs intégrés**.

Le viewer ajouté dans #97 reconstruisait son propre HTML et ne traitait que quelques balises génériques (`titre`, `section`, `p`, `note`, `table`, `ligne`, `cellule`). Il ignorait les balises spécifiques **`broche`**, **`fonction`**, **`couleur`** de la fiche 1.6 et détruisait le SVG imbriqué lors du rendu des cellules génériques de la fiche 1.9.

Correction retenue :
- création d’un renderer commun **`database/MemsReferenceSheetRenderer.h`** ;
- prise en charge des deux structures XML réellement présentes ;
- conservation et rendu des SVG de couleur intégrés ;
- lecture des balises `broche`, `fonction`, `couleur`, `cellule` et de leur contenu imbriqué ;
- réutilisation du même renderer dans **Base de données** et **IA MEMS**, afin d’éviter deux implémentations divergentes du rendu XML.

## DIFF FINAL DU CANDIDAT APRÈS NETTOYAGE

Après suppression de tous les workflows/scripts temporaires, la comparaison #97 → candidat final contient **exactement 9 fichiers** :
1. `database/MemsDatabaseBrowser.cpp` ;
2. `database/MemsReferenceSheetRenderer.h` — nouveau renderer commun ;
3. `expert/IaMemsConversationRouting.h` ;
4. `expert/IaMemsService.cpp` ;
5. `expert/IaResponseLogicTest.cpp` ;
6. `expert/LocalAiClient.cpp` ;
7. `expert/LocalAiOnnxSelfTest.cpp` ;
8. `iamemstab.cpp` ;
9. `iamemstab.h`.

Aucun fichier temporaire, aucun workflow temporaire, aucun protocole ECU et aucun fichier 32 bits ne reste dans le diff final.

## VALIDATION GITHUB TEMPORAIRE — ÉCHEC INTERMÉDIAIRE CONSIGNÉ

Premier run Windows x64 de validation : **`33114280638` — FAILURE**.

L’échec ne venait pas du code applicatif corrigé mais d’un **nouveau self-test** : comparaison directe `QStringList == QStringList{...}` provoquant sous Qt 5.15.2 / runner MSVC 2026 une erreur dans `qlist.h` (`stdext::make_checked_array_iterator`).

Correction appliquée uniquement au test : vérification de la taille puis comparaison élément par élément. Aucun comportement applicatif modifié pour contourner cet échec.

## VALIDATION GITHUB TEMPORAIRE FINALE — VERTE

Run corrigé : **`33114572801` — SUCCESS**.

Toutes les étapes sont vertes :
- checkout candidat ;
- validation des invariants source et du contenu QZ64 réel ;
- installation Qt 5.15.2 MSVC x64 ;
- configuration x64 ;
- **compilation de l’application et des self-tests touchés : SUCCESS** ;
- **tests déterministes : SUCCESS** ;
- cleanup : SUCCESS.

Les tests couvrent notamment :
- `Broche MAP Mini` + ECU déconnecté sans firmware => aucune preuve SPi/MPi automatique ;
- firmware connecté `AANMP002` => preuve forte disponible ;
- référence ECU explicite => utilisable comme preuve ;
- `preuve : constructeur` dans un grounding documentaire => ne déclenche pas le mode diagnostic ;
- vrai diagnostic => conserve le mode diagnostic ;
- rendu XML spécifique `broche/fonction/couleur` avec couleurs SVG ;
- rendu XML générique `cellule` avec SVG imbriqué ;
- routage documentaire MAP/lambda/ECT conservé.

## ÉTAT AVANT PUSH PRODUCTION

- #97 reste à cet instant la production active.
- Candidat final nettoyé : **`tmp-ia-post97-audit-fix`**, arbre final au commit **`e1d3ed416c41a5dd71f01871592e73a107a81e07`**.
- Le candidat est strictement en avance sur #97 et ne contient plus de fichier temporaire dans son arbre final.
- Pour ne pas importer l’historique des workflows temporaires dans la branche de production, le push production doit utiliser **un commit propre basé sur l’arbre final validé avec #97 comme parent**, puis avancer `MEMSX64` sans force.

# PROCHAINE ACTION EXACTE — APRÈS VALIDATION POST-#97

1. Créer le **commit production propre** à partir de l’arbre final validé, parent direct #97 `2b211554...`.
2. Avancer `MEMSX64` **sans force** sur ce commit propre.
3. Attendre le vrai workflow `BUILD` et ne déclarer le nouveau build validé qu’après résultat GitHub réel.
4. Si le BUILD est vert, consigner run/commit/artefact dans le présent rapport.
5. Test utilisateur recommandé : **désinstallation locale complète + suppression des données/profil/cache MEMS Manager réellement utilisés sur le PC**, puis installation du nouvel artefact afin d’écarter tout reste d’une ancienne base/profil.
6. Rejouer les scénarios réels #97 après installation propre : `Broche MAP Mini` → `cherche`, MAP MPi, MAP SPi Japan, couleurs lambda, couple ECT, XML 1.6/1.9 et `Valeur MAP ?` lorsque le véhicule pourra être connecté.

**Ne pas toucher au protocole ECU, au 32 bits ou à l’acquisition dans cette suite.**


# PASSAGE EN PRODUCTION ET TEST RÉEL BUILD #98

## PROMOTION PRODUCTION ET BUILD OFFICIEL

- Commit production propre : **`d7836e86930e107d8a68563a6fce3643f27c5748`** — `Fix post97 IA routing and XML rendering`.
- Parent direct : BUILD #97 **`2b211554abdbb127fd4d472f9ce687394b2d4608`**.
- Arbre : **`3dca12167987495e265fcb232b66d63100fb4126`**, identique à l'arbre candidat validé.
- `MEMSX64` avancée **sans force** ; aucun fichier protocole ECU ni voie 32 bits modifié.
- Workflow officiel **BUILD** (`341566505`) : run **`33115741255`**, run number **98**, branche `MEMSX64`, HEAD **`d7836e86930e107d8a68563a6fce3643f27c5748`**.
- Début : `2026-08-27T20:56:15Z` ; fin : `2026-08-27T21:03:07Z` ; conclusion : **SUCCESS**.
- Artefact : **`ECU-MEMS-Manager-x64-BUILD-98-v1.0.98`**, ID **`9664609350`**, taille **386 848 692 octets**, digest **`sha256:dd230381c5afcd2a52d29f71d412ebdea8c21134b75f912ab8ecbeeeaf283b52`**.

## INCIDENTS DU MÉCANISME DE RAPPORT

- Après la validation post-#97, le premier workflow temporaire d'ajout au rapport a produit **`33115448894` — FAILURE sans job** à cause d'une indentation Python/YAML invalide. Aucun impact sur `MEMSX64`, l'application ou l'ECU.
- Correction au commit **`cea4596848a508d95f33ad17fa59f6a582f5fb53`**, puis run **`33115661734`**, job **`98669527587`** : **SUCCESS**.
- Workflow temporaire supprimé au commit **`22d05221ff7859c93965fba086758ab03adc664a`**.
- Le premier essai d'ajout du présent compte-rendu #98 a reproduit la même erreur d'indentation YAML : run **`33119375824` — FAILURE sans job**. Cette erreur concerne uniquement le mécanisme de rapport et doit rester consignée.

## TEST INSTALLATION PROPRE #98 SUR PC RÉEL

Avant installation, l'ancienne version et les données locales identifiées dans le code ont été supprimées : ancien dossier `database`, données locales de l'application et profil utilisateur. Le test #98 ne réutilise donc pas volontairement l'ancien cache/base/profil.

- Installation complète après décompression : **1 minute 10 secondes**.
- Premier lancement et base locale : fonctionnels.
- Interface : aucun défaut nouveau signalé pendant cette séquence.

## TESTS IA RÉELS #98 — ECU DÉCONNECTÉ

### `Broche MAP Mini` puis `cherche`
**CORRIGÉ** : après clarification SPi/MPi, `cherche` ne conclut plus arbitrairement SPi sans preuve forte et demande une référence ECU ou année/marché.

### `Broche MAP Mini MPi 1997`
**PARTIEL** : les faits MAP MPi utiles sont retrouvés (C186, C159-36, C159-8, C159-13), mais un fait **SPi Japon 97MY** apparaît encore malgré la demande explicitement MPi.

### `Broche MAP Mini SPi Japan 1997`
**DÉFAUT** : le fait SPi Japon est retrouvé, mais un fait MPi et des faits CKP/bobine sans rapport direct avec MAP apparaissent également.

### `Couleur des fils sonde lambda ?`
**FUITE INTERNE CORRIGÉE**, pertinence encore insuffisante : aucun prompt interne affiché ; un même fait HO2S apparaît deux fois ; des faits descriptifs passent avant le câblage ; le fait `BG` noir/vert concerne la commande du relais de sonde oxygène SPi Japon 97MY et ne doit pas être présenté comme preuve directe de toutes les couleurs de la sonde.

### `Couple de serrage sonde ECT ?`
**CORRECT SUR LE FOND** : **15 Nm**, procédure pertinente, aucune fuite interne. Défaut restant : le fait ECT est dupliqué et un fait secondaire est ajouté malgré la question ciblée.

## TESTS FICHES XML RÉELS #98

### MEMS 1.6
- Bouton `Ouvrir la fiche XML MEMS 1.6` fonctionnel.
- Fiche complète : **25 broches ECU**, fonctions, couleurs graphiques, sources en bas, défilement complet.
- Défaut restant : absence du petit **cadre/fond gris commun autour des carrés de couleur**, rendant le noir presque invisible sur fond sombre.

### MEMS 1.9
- Fiche complète : **36 broches**, fonctions, couleurs graphiques, sources documentaires en bas.
- Même défaut de cadre/fond gris autour des couleurs.

## TEST ECU CONNECTÉ

Le véhicule n'est pas disponible. **Aucune validation #98 avec ECU réellement connecté n'est déclarée.** Ce test reste en attente et ne doit pas être simulé.

# PROCHAINE ACTION EXACTE — APRÈS TEST RÉEL #98

Partir strictement de **BUILD #98 `d7836e86930e107d8a68563a6fce3643f27c5748`** sur une branche temporaire. Ne toucher ni au protocole ECU ni au 32 bits.

Corriger uniquement :
1. filtrage strict **MPi/SPi/Japan** ;
2. pertinence du sujet : **MAP** ne doit pas sortir CKP/bobine ; une demande de **couleur lambda** doit privilégier un vrai fait de câblage compatible ;
3. déduplication des faits identiques ;
4. petit **cadre/fond gris commun** autour des carrés de couleur dans le renderer XML 1.6/1.9.

Ajouter les tests déterministes correspondant aux défauts reproduits, valider x64 sur GitHub Actions en branche temporaire, contrôler le diff et avancer `MEMSX64` sans force uniquement si la validation est verte. Le test ECU connecté restera en attente de la voiture.

# CONTINUITÉ AJOUTÉE — BUILDS #99 ET #100 — 28 AOÛT 2026

Cette section **supplante comme état courant** la section historique `PROCHAINE ACTION EXACTE — APRÈS TEST RÉEL #98`. Les sections antérieures restent conservées pour l’historique.

## ÉTAT DE DÉPART VÉRIFIÉ AVANT #100

- Branche de production x64 : **`MEMSX64`**.
- HEAD vérifié avant l’action #100 : **`35336ce820c3ef09153878bd918f7710c89552a4`** — `Fix post98 IA filtering and XML wire visibility`.
- Ce HEAD correspond au **BUILD #99**, workflow `BUILD`, run **`33120718004`**, conclusion **SUCCESS**.
- #99 reste le parent direct de #100 ; aucun retour arrière ni force-push n’a été utilisé.
- Le protocole ECU, l’acquisition, la RAM, le 32 bits et les protections de connexion n’ont pas été modifiés dans l’action #100.

## ACTION EFFECTUÉE — PROMOTION IA BUILD #100

Objectif exécuté : transférer sur la production x64 **uniquement les trois fichiers IA déjà validés dans le candidat temporaire**, sans importer de workflow/helper temporaire et sans toucher au reste du programme.

Commit production créé :
- **`cd6e52c714ee35a3fffb405797f14a58c2da4fa5`** — `Build #100: fix IA documentary precision` ;
- parent direct : **#99 `35336ce820c3ef09153878bd918f7710c89552a4`** ;
- arbre : **`5e2468947254aa6ccb54d154ce4d2a0a62c35c2d`**.

Comparaison #99 → #100 contrôlée : **1 seul commit et exactement 3 fichiers modifiés** :
1. `expert/IaMemsConversationRouting.h` — **+40 / -1** ;
2. `expert/IaMemsService.cpp` — **+39 / -16** ;
3. `expert/IaResponseLogicTest.cpp` — **+23 / -0**.

Aucun autre fichier n’est présent dans le diff :
- aucun protocole ECU ;
- aucune acquisition/RAM ;
- aucune UI ou responsive ;
- aucun `CMakeLists.txt` ;
- aucun workflow GitHub ;
- aucun helper temporaire ;
- aucun fichier 32 bits.

`MEMSX64` a été avancée **sans force** sur `cd6e52c714ee35a3fffb405797f14a58c2da4fa5`.

## BUILD #100 — VALIDATION GITHUB ACTIONS COMPLÈTE

Workflow officiel : **BUILD**, run **`33165636478`**, run number **100**, branche `MEMSX64`, HEAD **`cd6e52c714ee35a3fffb405797f14a58c2da4fa5`**.

Conclusion finale : **SUCCESS**.

Étapes validées :
- installation des outils de validation : **SUCCESS** ;
- Qt 5.15.2 MSVC 2019 x64 : **SUCCESS** ;
- ONNX Runtime GenAI + ONNX Runtime Windows x64 CPU : **SUCCESS** ;
- `Validate protocol guards before build` : **SUCCESS** ;
- compilation **ECU MEMS Manager x64 avec IA ONNX native** : **SUCCESS** ;
- `Run deterministic self-tests` : **SUCCESS** ;
- génération/validation base expert r20 : **SUCCESS** ;
- téléchargement Qwen3-0.6B ONNX INT4 CPU + vérification des hashes : **SUCCESS** ;
- `Run production LocalAiClient native ONNX self-test` : **SUCCESS** ;
- assemblage du package portable ONNX : **SUCCESS** ;
- validation du package portable complet : **SUCCESS** ;
- `Validate packaged production LocalAiClient with Qwen` : **SUCCESS** ;
- smoke launch ECU MEMS Manager avec runtime ONNX natif : **SUCCESS** ;
- génération manifeste/hashes : **SUCCESS** ;
- upload artefact : **SUCCESS**.

Le job `build_x64` s’est terminé le **28/08/2026 à 11:13:55Z** avec conclusion **SUCCESS**.

## ARTEFACT OFFICIEL BUILD #100

- ID artefact : **`9683725737`**.
- Nom : **`ECU-MEMS-Manager-x64-BUILD-100-v1.0.100`**.
- Taille : **386 852 374 octets**.
- Digest : **`sha256:c875ec951a9cb9aba4c0d332bf26dc4fc240ca98002c3bb5ec2675e39c2616d1`**.
- Branche : `MEMSX64`.
- HEAD : `cd6e52c714ee35a3fffb405797f14a58c2da4fa5`.

## ÉTAT COURANT APRÈS #100

- **Production x64 active : BUILD #100 / v1.0.100**.
- HEAD `MEMSX64` : **`cd6e52c714ee35a3fffb405797f14a58c2da4fa5`**.
- #100 est **validé côté compilation, self-tests, IA ONNX/Qwen, package et smoke launch GitHub Actions**.
- **#100 n’est pas encore déclaré validé sur le PC réel utilisateur** : aucun test réel post-#100 n’a encore été consigné.
- Les protections protocole sont passées vertes et aucun code protocole n’a changé dans ce lot.
- Le test ECU réellement connecté reste indépendant et ne doit être déclaré que lorsqu’un véhicule est disponible.

# PROCHAINE ACTION EXACTE — APRÈS BUILD #100 VERT

1. **Ne pas lancer #101 avant le test utilisateur de #100**, sauf demande explicite contraire.
2. Installer/tester l’artefact **`ECU-MEMS-Manager-x64-BUILD-100-v1.0.100`** sur le PC réel.
3. Rejouer en priorité les scénarios IA documentaires qui ont motivé le lot : filtrage MPi/SPi/Japan, pertinence MAP, couleurs lambda, déduplication et réponses factuelles ciblées.
4. Vérifier que les corrections #99 restent intactes, notamment le rendu/visibilité des couleurs XML et l’absence de régression UI.
5. Consigner immédiatement dans ce rapport les résultats réels, captures ou défauts observés avant toute nouvelle modification.
6. Ne pas toucher au protocole ECU, acquisition, RAM ou 32 bits sans nouvelle demande explicite.

**Référence de reprise obligatoire : `MEMSX64` BUILD #100 `cd6e52c714ee35a3fffb405797f14a58c2da4fa5`, GitHub Actions run `33165636478`, artefact `9683725737`.**
# TEST RÉEL BUILD #100 ET CORRECTION BUILD #101 — 28 AOÛT 2026

Cette section **supplante comme état courant** la section `PROCHAINE ACTION EXACTE — APRÈS BUILD #100 VERT`. Les sections précédentes restent conservées pour l’historique.

## TEST RÉEL UTILISATEUR BUILD #100

Artefact testé : **`ECU-MEMS-Manager-x64-BUILD-100-v1.0.100`**, HEAD `MEMSX64` **`cd6e52c714ee35a3fffb405797f14a58c2da4fa5`**.

Résultats confirmés sur le PC réel :
- **`Broche MAP Mini MPi 1997` : VALIDÉ**. Réponse ciblée MPi : C186, fils RG/YP/KB, ECU C159-36/C159-8 et retour C159-13 ; aucun fait SPi Japon/CKP/bobine parasite.
- **`Broche MAP Mini SPi Japan 1997` : VALIDÉ**. IA explique correctement que, sur la Mini SPi documentée, le MAP est intégré au calculateur MEMS et relié au collecteur par une durite de dépression ; **aucune broche de signal MAP externe ne doit être inventée**.
- **`Couple de serrage sonde ECT` : VALIDÉ**. Réponse constructeur ciblée **15 Nm**, procédure cohérente, sans IAT, doublon ni fuite interne.
- **Fiches XML MEMS 1.6 / 1.9 : rendu VALIDÉ**. Les couleurs de fils sont visibles et les rectangles/fonds gris rendent notamment le noir lisible ; la correction graphique #99 est préservée dans #100.
- **`Broche MAP Mini` → `CHERCHE` : comportement de preuve VALIDÉ**. ECU déconnecté et sans référence forte, IA refuse de choisir arbitrairement SPi/MPi et demande une référence ECU ou année + marché.
- **`Valeur MAP ?` posée seule après relance : VALIDÉ**. ECU déconnecté => `Je n'ai encore aucune mesure ECU disponible.` Le routage de mesure MAP live lui-même est donc correct.

### Défauts réels restant dans #100

1. **Couleurs lambda — PARTIEL** : `Couleur des fils sonde lambda` remonte notamment BG noir/vert (commande relais C159-36) et B noir (blindage C159-29), mais pas les deux voies signal disponibles dans la base : C159-7 `S` gris (+VE) et C159-18 `LGS` vert clair/gris (-VE).
2. **Documentation MEMS 1.9 — réponse doublée** : la fiche est bien proposée et son rendu est correct, mais le texte IA énumère deux assets locaux (connecteur ECU + OBD 16 broches) alors que la fiche XML est déjà proposée comme ressource documentaire.
3. **Contexte de clarification non fermé** : après `Broche MAP Mini` → `CHERCHE` sans preuve, une question suivante `Valeur MAP ?` peut être polluée par l’ancienne clarification et redemander SPi/MPi. Après redémarrage, `Valeur MAP ?` seule fonctionne normalement.

## CAUSES EXACTES AUDITÉES DANS #100

- `expert/IaMemsService.cpp` : `knowledgeQueryKind()` reconnaissait `fil` au singulier mais pas **`fils` au pluriel**. `Couleur des fils sonde lambda` restait donc en classement général au lieu du chemin WireColor spécialisé.
- Le chemin documentaire MEMS 1.9 laissait `knowledgeAnswer()` énumérer les deux assets techniques alors que l’UI propose déjà la fiche XML ; la demande de fiche/documentation devait être distinguée d’une demande d’illustrations individuelles.
- `iamemstab.cpp` : dans `sendQuestion()`, `m_pendingClarificationQuestion.clear()` était exécuté après plusieurs branches avec `return`. Le cas `CHERCHE` non résolu quittait donc la fonction avant nettoyage et laissait la question de brochage en attente.

## CORRECTION CANDIDAT TEMPORAIRE

Branche de validation utilisée : **`tmp-fix100-realtest-final`**, créée depuis exactement #100 `cd6e52c714ee35a3fffb405797f14a58c2da4fa5`.

Workflow temporaire : **`TEMP FIX100 REALTEST VALIDATION`**, run **`33177675749`**, job **`98870422470`** — **SUCCESS**.

Validations vertes :
- portée exacte des fichiers source ;
- protections protocole existantes ;
- Qt 5.15.2 MSVC 2019 x64 ;
- configuration x64 ;
- compilation de `ecu_mems_manager` et `ia_response_selftest` ;
- nouveaux tests déterministes IA ;
- suppression finale des fichiers temporaires de validation.

Arbre final temporaire validé : **`3c023d5c548a4d98ece5762789a83c65b0a31213`**.

## BUILD #101 — CORRECTION DES DÉFAUTS RÉELS #100

Commit production propre : **`22dbe75ed14e0a61e694159d505ef72245116b48`** — `Build #101: fix #100 real-test IA regressions`.

Parent direct : **#100 `cd6e52c714ee35a3fffb405797f14a58c2da4fa5`**. `MEMSX64` avancée **sans force**.

Comparaison #100 → #101 : **1 commit, exactement 4 fichiers** :
1. `expert/IaMemsConversationRouting.h` — +24 / -0 ;
2. `expert/IaMemsService.cpp` — +6 / -7 ;
3. `expert/IaResponseLogicTest.cpp` — +6 / -0 ;
4. `iamemstab.cpp` — +1 / -0.

Aucun changement protocole ECU, acquisition/RAM, UI générale/responsive, base RAVE, Qwen/ONNX, workflow de production ou 32 bits.

### Correction fonctionnelle #101

- ajout d’un classifieur commun de demande de couleur de fil reconnaissant **`fil` et `fils`**, utilisé par `IaMemsService` ;
- ajout d’une détection distincte des demandes de **fiche/documentation MEMS X.X** : réponse documentaire unique et bouton de fiche, sans énumération doublée des assets ECU/OBD ;
- consommation/effacement immédiat de la clarification en attente avant toute branche susceptible de sortir par `return` ; la question suivante ne doit plus hériter du brochage précédent ;
- tests de régression pour `Couleur des fils sonde lambda`, la fiche MEMS 1.9 unique et `Valeur MAP ?` sans clarification SPi/MPi.

## BUILD OFFICIEL #101 — VALIDATION GITHUB ACTIONS

Workflow officiel **BUILD**, run **`33178021830`**, job **`98871634811`**, run number **101**, HEAD **`22dbe75ed14e0a61e694159d505ef72245116b48`** — **SUCCESS**.

Toutes les étapes principales sont vertes : protections protocole, compilation x64 ONNX native, self-tests déterministes, base expert r20, modèle Qwen3-0.6B ONNX INT4 et hashes, LocalAiClient natif, assemblage/validation package, LocalAiClient packagé, smoke launch, manifeste/hashes et upload.

Artefact officiel :
- nom : **`ECU-MEMS-Manager-x64-BUILD-101-v1.0.101`** ;
- ID : **`9688684682`** ;
- taille : **386 852 607 octets** ;
- digest : **`sha256:9374152a904feb7d46a99089bd854d2c385e7d77279607296f6f73aa80a05df3`**.

## ÉTAT COURANT

- Production x64 : **BUILD #101 / v1.0.101**.
- HEAD `MEMSX64` : **`22dbe75ed14e0a61e694159d505ef72245116b48`**.
- #101 est validé côté CI/package/smoke, **mais les trois corrections ci-dessus ne sont pas encore déclarées validées sur le PC réel**.
- Aucun test ECU connecté supplémentaire n’a été effectué dans cette correction.

# PROCHAINE ACTION EXACTE — APRÈS BUILD #101 VERT

Tester sur le PC réel **uniquement les régressions corrigées**, puis les contrôles de non-régression essentiels :
1. `Couleur des fils sonde lambda` → vérifier que les faits de signal +VE/-VE sont correctement privilégiés/étiquetés avec relais et blindage, sans description HO2S générique en tête ;
2. `Je cherche la documentation MEMS 1.9` → une réponse documentaire unique + fiche proposée, sans double liste ECU/OBD ;
3. `Broche MAP Mini` → `CHERCHE` sans preuve → puis `Valeur MAP ?` **sans relancer l’application** : la dernière question doit être indépendante et répondre absence de mesure si ECU déconnecté ;
4. contrôle rapide : MAP MPi 1997 reste ciblé ; MAP SPi reste MAP intégré sans broche externe ; ECT reste 15 Nm ; rectangles gris/couleurs XML restent corrects.

**Référence de reprise obligatoire : `MEMSX64` BUILD #101 `22dbe75ed14e0a61e694159d505ef72245116b48`, run `33178021830`, artefact `9688684682`.**
