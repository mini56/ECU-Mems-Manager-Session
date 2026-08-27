# RAPPORT DE CONTINUITÉ — ECU MEMS MANAGER

> **RÈGLE OBLIGATOIRE** : relire ce fichier avant toute modification. Branche rapport : `RAPPORT`. Branche x64 active : `MEMSX64`.
>
> **RÈGLE QUALITÉ — AUCUNE RUSTINE** : rechercher la cause réelle et produire une solution propre, générale et maintenable. Ne pas supprimer une capacité juste pour faire passer un test.
>
> **SUIVI IMMÉDIAT** : avant chaque nouvelle étape, inscrire ici l’étape et son objectif ; après chaque résultat, l’inscrire avant la suite.
>
> **NOMMAGE UTILISATEUR** : `ECU MEMS Manager x64 #NN — Commit xxxxxxx`. `#NN` = GitHub Actions.
>
> **VERSIONNAGE ACTIF** : le numéro de version du programme suit désormais le numéro du run/build GitHub Actions visible par l’utilisateur : **#94 => v1.0.94**, #95 => v1.0.95, etc. La fenêtre de démarrage/splash, About, l’aide et toute autre occurrence de version doivent rester synchronisés. **#92 reste historiquement v1.0.30** ; #93 a inauguré la reprise dynamique avec v1.0.93.

## ÉTAT COURANT — 27 AOÛT 2026

- Dépôt : `mini56/ECU-Mems-Manager-Session`.
- Branche active : **`MEMSX64`**.
- HEAD x64 courant : **`39543d694507d4c01c16d54cf2f6b01f3043fd6f`**.
- Dernier run entièrement validé : **#94 = v1.0.94 — SUCCESS**, run `33068860732`, commit `39543d69`.
- Artefact #94 : ID **`9645083399`**, nom **`ECU-MEMS-Manager-x64-BUILD-94-v1.0.94`**, taille **386 779 885 octets**, SHA-256 **`11503728fc75dbbff8104d68f6876cdca82cc8f41e234c939e65b7913fe698db`**.
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

- État historique : **ECU MEMS Manager x64 #92 = v1.0.30**.
- Reprise dynamique appliquée : **#93 = v1.0.93**, puis **#94 = v1.0.94**.
- À partir de maintenant : #95 = v1.0.95, #96 = v1.0.96, etc., via `github.run_number`.
- La version affichée dans la fenêtre de démarrage/splash est la référence visible par l’utilisateur et doit correspondre au run/build livré.
- About, aide, métadonnées et toute autre occurrence du numéro de version doivent utiliser la même valeur ; éviter plusieurs numéros incohérents dans le même package.

## ORDRE DE TRAVAIL VALIDÉ APRÈS #92

L’ordre demandé était strictement :
1. **Consigner #92 vert** et conserver la validation LocalAiClient native + packagée — fait.
2. **Auditer `IaMemsTab` / `IaMemsService`** avant modification — fait.
3. **Ajouter la fonction de proposition de schéma local** — fait et validé dans #94.
4. **Respecter toutes les contraintes de séparation et de sécurité** — fait : aucun changement Qwen, ONNX, `LocalAiClient`, protocole, ECU, RAVE ou 32 bits pour cette fonction.

**La fonction schémas est maintenant stabilisée par le build #94. Le lot suivant redevient RAVE 1720, à traiter séparément.**

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
- L’échec survient lorsque le `POST_BUILD` tente d’exécuter `Release\\ia_mems_diagram_selftest.exe`; MSBuild remonte **`-1073741515`** (`0xC0000135`, runtime DLL introuvable).
- `ia_mems_diagram_selftest` est lié à `Qt5::Core`. Au moment du `POST_BUILD`, le workflow n’a pas encore ajouté `C:\\Qt\\5.15.2\\msvc2019_64\\bin` au `PATH`.
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

## RAVE — AUDIT DE COUVERTURE MARCHÉ / ANNÉE APRÈS #94

- Base de l’artefact #94 inspectée directement : **86 faits RAVE / 98 faits experts**.
- Répartition RAVE par variante : **40** `SPi_Japan_97MY_from_VIN_SAXXNNAXKBD_134455`, **26** `Mini_1997_2000`, **15** `MPi_97MY_from_VIN_SAXXNNAZEBD_134455`, **3** `MPi_1997_plus`, **2** `SPi_1997_plus`.
- Conclusion : la Mini **SPi classique non-Japon est nettement sous-couverte** par rapport à la SPi Japon ; la fréquence des réponses IA mentionnant le Japon est cohérente avec cette répartition documentaire et ne doit pas être corrigée en masquant les faits Japan.
- Les 86 faits RAVE portent déjà un millésime, une année ou une plage d’années dans leur variante ; le déficit principal concerne la **portée marché explicite** pour les faits non-Japon et le manque de faits constructeur sur les SPi 1991–1996.
- Le sommaire constructeur `RCL0194ENG` 97MY sépare lui-même `Engine Management System: MPi 20.1` et `SPi (JAPAN) 20.3`, avec une portée VIN `Japan only — SAXXNNAXKBD 134455` et `All other vehicles — SAXXNNAZEBD 134455`. Continuer 20.3/20.4 augmenterait donc mécaniquement la surreprésentation SPi Japon.
- Publication Rover à rechercher en priorité pour la SPi classique : **AKM6799 — Mini Single Point Injection 1991 to 1996**. `AKM7169` (Mini All Models 1993 on) pourra être utilisé en recoupement si ses pages donnent une portée constructeur exploitable.

## RAVE — RÈGLE DE PORTÉE VARIANTE / ANNÉE / MARCHÉ

- Chaque nouveau fait Mini doit conserver **exactement la portée disponible dans la source constructeur** : type d’injection (SPi/MPi), millésime/année ou plage VIN, marché (UK/Europe/Japon/export/etc.) et, lorsqu’ils sont distingués, transmission, Cooper/non-Cooper, climatisation ou autre équipement.
- **Ne jamais inférer un marché** et ne jamais généraliser un fait spécifique à une autre variante.
- Si la source constructeur ne précise pas un axe, noter **`non précisé`** plutôt que de supposer UK, Europe, Japon ou « toutes Mini ».
- Les faits Japon doivent rester explicitement Japon dans la variante et dans le contexte restitué par IA MEMS ; ils ne doivent pas être présentés comme une vérité universelle SPi.
- Les sources secondaires (MEMS FCR, catalogues de pièces, sites techniques, forums) peuvent servir à orienter la recherche ou à signaler une divergence, mais **ne deviennent pas `verifie_constructeur`** sans recoupement avec une documentation Rover primaire correspondante.

## PROCHAINE ACTION EXACTE

**Suspendre temporairement les anciens candidats RAVE 1720 Japon (C159-5 GU, C159-25 WB / C161-18 WS et C159-19 RW) sans les supprimer. Avant d’ajouter davantage de faits Japan, rechercher puis lire une source Rover primaire couvrant la Mini SPi classique 1991–1996, en priorité AKM6799 puis AKM7169 si pertinent. Extraire un premier lot uniquement si les relations techniques et leur portée année/marché/VIN sont directement vérifiables. Préparer ce lot sur une branche temporaire, valider la base, l’intégrité et le diff, puis seulement envisager `MEMSX64`. Reprendre ensuite le RAVE 1720 Japon séparément.**
