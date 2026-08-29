# RAPPORT DE CONTINUITÉ — ECU MEMS MANAGER

> **RÈGLE OBLIGATOIRE** : relire ce fichier avant toute modification. Branche rapport : `RAPPORT`. Branche x64 active : `MEMSX64`.
>
> **RÈGLE QUALITÉ — AUCUNE RUSTINE** : rechercher la cause réelle et produire une solution propre, générale et maintenable. Ne pas supprimer une capacité juste pour faire passer un test.
>
> **SUIVI IMMÉDIAT** : avant chaque nouvelle étape, inscrire ici l’étape et son objectif ; après chaque résultat, l’inscrire avant la suite. **Toute modification, validation, échec, test réel et décision doit être ajouté au rapport au fur et à mesure afin de conserver l’historique complet entre les discussions.**
>
> **RÈGLE FONDAMENTALE — PRIORITÉ ABSOLUE À LA TRAÇABILITÉ** : si une tentative de mise à jour du rapport échoue, **toute autre action est immédiatement suspendue**. La cause de cet échec de journalisation devient la priorité n°1 : elle doit être diagnostiquée et corrigée, puis le rapport doit être effectivement écrit et vérifié avant toute nouvelle pousse, correction, migration, test ou build. L’échec de journalisation lui-même doit être consigné dès que le canal de rapport est rétabli. **Aucune progression technique ne doit contourner un rapport en échec.**
>
> **RÈGLE DE JOURNALISATION OBLIGATOIRE — 29 AOÛT 2026** : pour ECU MEMS Manager, l’ordre est strict : **RAPPORT AVANT CHAQUE POUSSE → POUSSE → TEST RÉEL → RAPPORT IMMÉDIAT → correction éventuelle → RAPPORT AVANT LA POUSSE SUIVANTE**. Chaque test doit conserver la question/manipulation réelle, la réponse observée, le verdict ✅/⚠️/❌, les régressions, la conclusion et la prochaine action. Un échec ne doit jamais être recouvert par une correction sans avoir été consigné.
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

# PILOTE ILLUSTRATIONS RAVE 1680 — TESTS RÉELS — 29 AOÛT 2026

Cette section **supplante comme point de suivi courant** la fin précédente du rapport. Elle documente les échanges, décisions, échecs et progrès du pilote illustrations RAVE. La production `MEMSX64` reste **BUILD #101** et n'est pas modifiée par ce pilote.

## DÉCISION D'ARCHITECTURE — ILLUSTRATIONS RAVE

Constat confirmé : les tables RAVE historiques contiennent des références telles que `image_ref` / `figure_ref`, mais **les illustrations RAVE originales elles-mêmes n'étaient pas stockées dans la SQLite**. Les références permettaient de retrouver la page source, pas de relire localement le dessin original.

Décision utilisateur validée :
- pour la partie RAVE déjà présente dans la base, ajouter maintenant les illustrations originales correspondantes ;
- pour les futures parties RAVE encore non extraites, capturer **données structurées + portée + illustration + référence** pendant la même passe d'enrichissement ;
- conserver les images comme fichiers locaux du package et utiliser une couche SQLite additive de liaison, plutôt que gonfler les tables historiques avec des BLOB ;
- ne modifier ni supprimer les faits historiques ;
- profiter du rattachement visuel pour auditer les interprétations techniques existantes.

## PILOTE LIMITÉ AU LOT RAVE 1680

Base de production conservée : **`MEMSX64` #101 `22dbe75ed14e0a61e694159d505ef72245116b48`**.

Branche temporaire : **`tmp-rave-1680-illustrations`**.

Périmètre volontairement limité à trois illustrations originales du document constructeur `RCL0194ENG` déjà référencées par le lot :
- `RCL0194ENG:20.3` ;
- `RCL0194ENG:20.4` ;
- `RCL0194ENG:39.3`.

Le pilote extrait les pages originales du PDF Rover et les place sous `database/reference/images/rave/`. Il ajoute de façon additive :
- `mems_rave_illustration` ;
- `mems_rave_illustration_link` ;
- la vue `mems_rave_fact_illustration_view` ;
- `research_enrichment_1750.qz64` ;
- les entrées correspondantes dans le manifeste.

`database_revision` reste **20** et aucune table historique n'est modifiée.

## PREMIER ARTEFACT PILOTE — BUILD TECHNIQUE VERT, TEST RÉEL ÉCHOUÉ

Premier run temporaire : **`33190463268` — SUCCESS**.

Artefact :
- **`ECU-MEMS-Manager-x64-TEST-RAVE1680-FROM-101`** ;
- ID **`9693533551`** ;
- digest **`sha256:52604b450bb59a138721d5d1780a603f811093c32191e985cc6afeb82b496b0c`**.

Le package contenait bien les trois illustrations originales et leurs liaisons additives. **Cela n'a pas été considéré comme validation utilisateur.**

### Test réel utilisateur — échec de routage

Formulations réellement utilisées, volontairement différentes de la phrase de test préparée :
- `sonde lambda Mini SPi Japon 1997` → **`Je n'ai encore aucune mesure ECU disponible.`** ;
- `schéma sonde lambda Mini SPi Japon 1997` → demande inutile de génération MEMS `1.2 / 1.3 / 1.6 / 1.9` ;
- `cherche` → aucune génération déterminée ;
- `Montre le schéma sonde lambda Mini SPi Japon 1997` → nouvelle demande de génération.

Verdict : **❌ PILOTE V1 NON VALIDÉ SUR PC RÉEL**.

Cause auditée : dans `IaMemsTab::sendQuestion()`, `clarificationPrompt()` était exécuté **avant** `updateDiagramSuggestion()`. Une ressource RAVE exacte pouvait donc être présente et résoluble, tout en étant bloquée par la clarification avant que l'UI ait l'occasion de la proposer.

### Règle de test décidée par l'utilisateur

L'utilisateur a volontairement changé les formulations proposées par l'assistant, avec le principe suivant : **une phrase préparée sachant exactement la réponse attendue est un test trop facile**. Les tests IA doivent utiliser des formulations naturelles et libres afin de mesurer le comportement réel, pas seulement un mot-clé prévu par le développeur.

Cette règle est adoptée pour la suite des validations IA MEMS.

## PILOTE V2 — CORRECTION DU ROUTAGE ET BUILD TEMPORAIRE

Correction préparée uniquement sur la branche pilote : lorsqu'un schéma local exact peut être résolu à partir de la demande, sa proposition ne doit pas être bloquée par une clarification de génération inutile. Les autres demandes ambiguës conservent leur garde de clarification.

HEAD du build V2 : **`2bede1ff5704c42dd65d2d6337bf79f991cad1a9`**.

Run V2 : **`33241431317` — SUCCESS**, job **`99071341649`**.

Toutes les étapes du build pilote V2 sont vertes : compilation x64, tests déterministes, récupération de la base #101, remplacement limité de l'exécutable de test, extraction des trois pages RAVE, validation du périmètre et upload.

Artefact V2 :
- **`ECU-MEMS-Manager-x64-TEST-RAVE1680-V2`** ;
- ID **`9711539092`** ;
- taille **387 364 252 octets** ;
- digest **`sha256:058e01054f8bf58440088a48ed41aa46b6d1a2db03d30c6db32563716393c713`**.

## TEST RÉEL V2 — AVANCÉE MAJEURE CONFIRMÉE

L'utilisateur poursuit volontairement les tests avec ses propres formulations.

Après des essais précédents où `image` / `1.9` faisaient encore proposer les ressources génériques `MEMS 1.9 ECU` et `fiche XML MEMS 1.9`, la formulation naturelle suivante a été utilisée :

**`cablage sonde lambda Mini SPi Japon 1997`**

IA MEMS répond alors avec un **brochage vérifié** basé sur les faits constructeur SPi Japon 97MY, notamment :
- relais/chauffage sonde : `C159-36`, `BG` noir/vert ;
- blindage / screen ground : `C159-29`, `B` noir ;
- signal `OXYGEN SENSOR +VE` : `C159-7`, `S` gris ardoise/gris ;
- la réponse cite explicitement plusieurs fois **`Illustration locale: RCL0194ENG:20.3 — preuve : constructeur`**.

Surtout, l'interface affiche désormais le bouton exact :

**`Ouvrir le schéma RAVE RCL0194ENG 20.3 Mini SPi Japon 97MY MEMS lambda relais vilebrequin`**

### Verdict exact

✅ **VALIDÉ : sélection et proposition de la nouvelle illustration RAVE `RCL0194ENG:20.3`.**

Cette ressource n'est pas une image fournie manuellement par l'utilisateur et n'est pas le schéma générique MEMS 1.9 déjà présent. C'est **l'une des trois illustrations originales extraites de RAVE spécifiquement pour ce pilote** et enregistrées dans le package/base de référence pendant le test.

La capture réelle prouve donc pour la première fois la chaîne :
**question naturelle utilisateur → faits RAVE compatibles → référence `RCL0194ENG:20.3` → sélection de l'asset RAVE nouvellement enregistré → proposition du bouton d'ouverture exact.**

⚠️ **L'ouverture du bouton RAVE 20.3 exact n'est pas encore déclarée validée dans ce test précis tant que l'utilisateur n'a pas confirmé le clic et l'affichage de cette image.** Les boutons génériques MEMS 1.9 ECU et fiche XML 1.9 ont, eux, déjà été confirmés fonctionnels.

## RECTIFICATION DE TRAÇABILITÉ MAP / `CHERCHE`

- La formulation antérieure selon laquelle **le MAP intégré concernait nécessairement le MPi** ne doit pas être retenue : l'utilisateur a précisé le 29 août 2026 que **le MAP intégré concerne la SPi** et a demandé une vérification distincte de la base pour le MPi.
- Le statut documentaire MAP MPi doit donc être vérifié contre la base/RAVE avant toute nouvelle conclusion ; ne pas déduire son architecture de la règle SPi.
- Dans l'épisode original #100, le mot `CHERCHE` apparaissait dans la réponse de l'IA ; il ne faut pas réécrire l'historique comme si l'utilisateur l'avait nécessairement saisi. Un **test contrôlé ultérieur #101** a bien utilisé `CHERCHE`, mais c'est un test distinct.


## INCIDENT DU MÉCANISME DE RAPPORT — CONSIGNÉ

- Première tentative de mise à jour automatique du présent rapport après ce test : commit temporaire `0c437823a7f2e444a5664d121a95190a553de61b`.
- Workflow run **`33243046088` — FAILURE sans job** : YAML invalide dans le bloc de texte multiligne du mécanisme temporaire de rapport.
- **Aucun impact sur `MEMSX64`, le pilote, l'application, la base RAVE ou l'ECU.**
- La correction concerne uniquement le mécanisme d'écriture du rapport ; l'échec est conservé ici afin de ne pas masquer la progression/régression de l'outillage de suivi.

## ÉTAT COURANT APRÈS CE TEST

- `MEMSX64` production reste strictement **#101 `22dbe75ed14e0a61e694159d505ef72245116b48`**.
- Aucune promotion #102 n'est autorisée sur la seule base de ce succès partiel.
- Le pilote V2 démontre une **avancée majeure réelle** : l'IA peut maintenant proposer une illustration RAVE nouvellement enregistrée à partir d'une formulation naturelle que l'utilisateur a choisie lui-même.
- Prochaine validation minimale avant toute nouvelle pousse fonctionnelle : cliquer le bouton RAVE `RCL0194ENG 20.3` exact et confirmer que l'illustration originale correspondante s'ouvre correctement ; poursuivre ensuite quelques formulations naturelles non préparées pour vérifier la robustesse du routage.
- Conformément à la règle du 29 août : **aucune nouvelle pousse de code ne doit avoir lieu avant que ce résultat soit présent dans le rapport.**

# VALIDATION COMPLÈTE PILOTE VISUEL RAVE ET RÈGLE FONDAMENTALE — 29 AOÛT 2026

## VALIDATION RÉELLE DE L'OUVERTURE RAVE 20.3

Après la validation précédente de la sélection/proposition de `RCL0194ENG:20.3`, l'utilisateur confirme explicitement sur le PC réel que **le bouton ouvre correctement l'image RAVE 20.3 elle-même**.

Verdict : **✅ PILOTE VISUEL RAVE VALIDÉ DE BOUT EN BOUT**.

Chaîne réellement validée :
**formulation naturelle utilisateur → faits RAVE compatibles → référence d'illustration → asset RAVE original local → bouton exact → ouverture correcte de l'illustration RAVE**.

Le principe d'ajouter les captures/pages/illustrations/supports visuels issus de RAVE pour les rendre accessibles à l'utilisateur final est donc **VALIDÉ**.

## RÈGLE FONDAMENTALE PERMANENTE — CONSERVATION MAXIMALE DE L'INFORMATION

Cette règle devient une règle de fond du projet **ECU MEMS Manager** et doit survivre à toute coupure/changement de discussion :

1. **But produit** : rendre accessible à l'utilisateur final le maximum d'informations techniques utiles et traçables disponibles.
2. Pour les lots RAVE déjà intégrés dans la base, reprendre les sources et ajouter leurs supports visuels utiles : pages, schémas, illustrations, tableaux, vues techniques et tout autre support pertinent, avec liaison à la connaissance et à la source.
3. Pour les futurs lots RAVE, capturer dans la même passe autant que possible : **données structurées + portée véhicule/moteur + source/page + texte + tableaux + schémas + illustrations + vues + autres supports visuels**.
4. Si une information utile ne rentre pas dans le schéma actuel, **ne jamais l'abandonner** : créer de manière additive la table, relation, champ, index, type d'asset ou structure nécessaire pour la conserver correctement et la rendre accessible au programme.
5. Les tables historiques ne doivent pas être détruites ni réécrites arbitrairement ; les extensions doivent rester additives, traçables et testées.
6. Toute information technique utile découverte pendant des recherches externes/Internet et absente de la base doit être **conservée pour intégration future**, avec URL/source, date de consultation, niveau de preuve, portée connue et contenu exploitable. Elle ne doit pas disparaître à la fin de la conversation.
7. Les découvertes externes peuvent être accumulées au fil de l'eau sur une branche/dépôt de travail de type **recherche à ajouter dans la base**, puis intégrées par lots après validation ; il n'est pas nécessaire de modifier la production à chaque découverte.
8. Une information trouvée mais non encore intégrée doit conserver un statut explicite (`à vérifier`, `source secondaire`, `constructeur`, `contradictoire`, etc.) ; une source Internet secondaire ne devient jamais automatiquement `verifie_constructeur`.
9. Lorsqu'une source contient à la fois texte et preuve visuelle, conserver les deux et les relier ; le support visuel ne remplace pas la donnée structurée et inversement.
10. Le mécanisme utilisateur doit, autant que possible, permettre de retrouver et ouvrir la preuve/source visuelle pertinente depuis la connaissance affichée, comme validé avec `RCL0194ENG:20.3`.

Cette règle complète les règles existantes de portée, provenance, `NULL = UNKNOWN`, absence d'invention, sécurité protocole et journalisation.

## CHANTIER AUTORISÉ — REPRISE DES LOTS RAVE DÉJÀ PRÉSENTS

Prochaine action autorisée par l'utilisateur : **préparer puis exécuter sur branche temporaire la reprise des lots RAVE déjà présents dans la base afin d'ajouter leurs captures/illustrations/supports visuels utiles selon le mécanisme validé par le pilote 20.3.**

Ordre obligatoire :
1. inventorier les lots/faits RAVE déjà présents, leurs documents, `image_ref`/pages/figures et les supports déjà packagés ;
2. identifier les références visuelles manquantes et les doublons afin de ne stocker qu'une fois un même asset source ;
3. récupérer les sources constructeur correspondantes et capturer les pages/figures utiles ;
4. rattacher additivement les assets aux faits/knowledge items sans modifier les faits historiques ;
5. conserver SHA-256, document, page/figure, chemin local et provenance ;
6. vérifier visuellement les supports pendant la reprise et signaler toute divergence entre un fait historique et ce que montre réellement la source ;
7. ajouter/étendre le schéma uniquement lorsqu'un type de donnée/support n'est pas représentable proprement ;
8. ajouter des validations d'intégrité, d'existence des fichiers, de hashes, de liens fait↔asset et de résolution utilisateur ;
9. travailler uniquement sur branche temporaire ; **`MEMSX64` reste #101 `22dbe75ed14e0a61e694159d505ef72245116b48` tant que ce chantier n'est pas validé** ;
10. respecter la règle : **rapport avant chaque pousse, test réel, rapport immédiat après test**.

## INCIDENT DU MÉCANISME DE RAPPORT — 29 AOÛT 2026

- Tentative de mise à jour persistante au commit temporaire `08aadc95a524b4ea1c9560d11681b17994d58c14`.
- Run GitHub Actions `33245879771` : **FAILURE sans job**, YAML invalide du helper temporaire de rapport (contenu multiligne insuffisamment indenté).
- Aucun impact sur `MEMSX64`, le pilote, la base, le programme ou l'ECU.
- Cet échec d'outillage est conservé avant sa correction conformément à la règle de journalisation.

## ÉTAT AU DÉMARRAGE DU CHANTIER

- Production : `MEMSX64` **BUILD #101** `22dbe75ed14e0a61e694159d505ef72245116b48` — inchangée.
- Pilote V2 validé : branche `tmp-rave-1680-illustrations`, HEAD de build `2bede1ff5704c42dd65d2d6337bf79f991cad1a9`, run `33241431317`, artefact `9711539092`.
- `RCL0194ENG:20.3` : **sélection, proposition, clic et ouverture sur PC réel VALIDÉS**.
- Le chantier de reprise doit réutiliser le mécanisme validé, pas reconstruire une architecture parallèle.

# CHANTIER RAVE VISUEL — INVENTAIRE DES LOTS EXISTANTS — 29 AOÛT 2026

## POUSSE INVENTAIRE / RÉSULTAT

- Branche temporaire créée exactement depuis la production #101 : `tmp-rave-visual-backfill`.
- Base de départ : `MEMSX64` #101 `22dbe75ed14e0a61e694159d505ef72245116b48`, inchangée.
- Premier commit du chantier : `4a5a9f3173496bcd6cfe5aab6d3cc2788ac0e490` — `Inventory existing RAVE facts for visual backfill`.
- Fichier ajouté : `database/reference/audits/rave_visual_backfill_inventory.md`.
- Cette pousse ne réécrit aucun fait RAVE historique, ne touche pas le protocole, l'IA, l'UI, la RAM ni le 32 bits ; elle enregistre uniquement l'inventaire du chantier.

## INVENTAIRE EXACT DE LA BASE #101

L'inspection directe de la SQLite officielle #101 confirme **93 faits RAVE** provenant de sept lots documentaires :

- `1660` : `RCL0193ENG` — 31 faits ;
- `1670` : `RCL0194ENG MPi 97MY` — 15 faits ;
- `1680` : `RCL0194ENG SPi Japon 97MY wiring` — 14 faits ;
- `1690` : `RCL0194ENG SPi Japon couleurs` — 7 faits ;
- `1700` : `RCL0194ENG SPi Japon injecteur/purge/IAC couleurs` — 8 faits ;
- `1710` : `RCL0194ENG 20.3 couleurs` — 11 faits ;
- `1720` : `AKM7169ENG SPi classique` — 7 faits.

Total : **93 faits**.

### `RCL0193ENG` — 31 faits

Aucun des 31 faits ne porte actuellement d'`image_ref`. Les `source_section` conservent cependant les pages PDF utilisées. Les 31 faits renvoient à **26 pages PDF distinctes** :

`38, 39, 40, 98, 101, 107, 108, 109, 112, 113, 114, 117, 118, 120, 121, 122, 123, 125, 126, 127, 128, 129, 130, 131, 135, 170`.

Ces pages couvrent notamment données de réglage moteur, pression carburant, TPS, EVAP, CKP, MAP, ECT, IAT, HO2S, IACV, injecteurs, câble/pédale/papillon, bobine, ECM et procédures de dépose/repose.

### `RCL0194ENG` — 55 faits

Les 55 faits portent déjà des références visuelles textuelles. Les **sept pages constructeur uniques** requises par les faits existants sont :

`15.1`, `20.1`, `20.2`, `20.3`, `20.4`, `39.1`, `39.3`.

Le pilote a déjà extrait et validé `20.3`, `20.4` et `39.3` ; elles doivent être réutilisées, sans duplication.

Les pages directes restant à capturer sont donc :
`15.1`, `20.1`, `20.2`, `39.1`.

Les lots 1690–1710 utilisent en plus la **légende constructeur des codes couleurs**. Cette page doit être identifiée précisément, capturée une seule fois puis reliée comme preuve complémentaire.

### `AKM7169ENG` — 7 faits

Les références logiques existantes sont :
- `Introduction / publication scope`;
- `Engine Tuning Data 3`;
- `Engine Tuning Data 4`;
- `Engine Tuning Data 5`;
- faits communs `Tuning Data 3–5`.

Les index PDF exacts doivent être vérifiés contre le scan constructeur retenu avant toute extraction. Aucun numéro de page ne sera inventé.

## INCIDENT DU MÉCANISME DE RAPPORT APRÈS L'INVENTAIRE

- Première tentative d'ajout de cette section : commit temporaire `cec4d5ad89318573513bf24a9c59614172903856`.
- Run `33246383027` : **FAILURE sans job** à cause d'un contenu encodé incorrect dans le helper temporaire de rapport.
- Aucun impact sur `MEMSX64`, `tmp-rave-visual-backfill`, la base, le programme ou l'ECU.
- L'incident est conservé conformément à la règle de journalisation avant sa correction.

## PROCHAINE ACTION EXACTE

1. Compléter d'abord `RCL0194ENG`, dont la cartographie est déjà explicite.
2. Vérifier dans le PDF constructeur exact les index physiques correspondant à `15.1`, `20.1`, `20.2`, `39.1` et à la légende couleurs.
3. Préparer ensuite l'extraction additive de ces quatre pages + légende, tout en réutilisant `20.3`, `20.4`, `39.3` déjà validées par le pilote.
4. Vérifier fichiers, SHA-256, document/page, liens fait→asset et résolution utilisateur.
5. Mettre le rapport à jour avant la prochaine pousse du chantier.
6. Aucun build officiel #102 : `MEMSX64` reste strictement #101.

# AUDIT VISUEL RCL0194ENG AVANT BACKFILL — 29 AOÛT 2026

## SOURCE CONSTRUCTEUR ET CARTOGRAPHIE PHYSIQUE VÉRIFIÉE

Le document constructeur retenu est `RCL0194ENG`, MINI Electrical Circuit Diagrams, 3rd Edition, 41 pages PDF. Le PDF est le même que celui utilisé avec succès par le pilote V2 ; son SHA-256 déjà enregistré par le pilote est :

`64e64f8a7c24f362913e2661403bc474e4e7ef07f96db618ef661645e0d0f051`.

La cartographie physique vérifiée dans ce PDF est :

- index PDF `5` : **COLOUR CODES**, page de légende Rover `RCL 0145` ;
- index PDF `13` : **Charging and Starting — MPi — 15.1** ;
- index PDF `15` : **Engine Management System (MEMS) — MPi — 20.1** ;
- index PDF `16` : **Engine Management System (MEMS) — MPi — 20.2** ;
- index PDF `17` : **Engine Management System (MEMS) — SPi Japan — 20.3** — déjà validé par le pilote ;
- index PDF `18` : **Engine Management System (MEMS) — SPi Japan — 20.4** — déjà extrait par le pilote ;
- index PDF `24` : **Heater Blower — 39.1** ;
- index PDF `25` : **Cooling Fan — MPi — 39.2** ;
- index PDF `26` : **Cooling Fan — SPi Japan — 39.3** — déjà extrait par le pilote.

## DIVERGENCE HISTORIQUE DÉCOUVERTE GRÂCE AU BACKFILL VISUEL

L'inventaire initial indiquait `39.1` parce que trois faits historiques de la base portent actuellement cette référence. La lecture directe du document constructeur prouve cependant que :

**`39.1` = HEATER BLOWER**  
**`39.2` = COOLING FAN MPi**

Les trois faits historiques concernés sont exactement :

- `RAVE-WIR-MPI-006` — `cooling_fan_wiring` — référence actuelle `RCL0194ENG:20.1,39.1` ;
- `RAVE-WIR-MPI-008` — `coolant_sensor_wiring` — référence actuelle `RCL0194ENG:20.2,39.1` ;
- `RAVE-WIR-MPI-012` — `sensor_ground` — référence actuelle `RCL0194ENG:20.2,39.1`.

Le contenu technique de ces faits correspond bien à la vraie page **39.2** : le circuit Cooling Fan MPi y montre notamment `C159-28`, le relais `C019`, le ventilateur `C005`, ainsi que le capteur `C169` relié à `C159-15` / `C159-13`.

### Décision de traçabilité

- **Aucune correction silencieuse n'est effectuée.**
- Les qz64 historiques restent inchangés à ce stade.
- La divergence est enregistrée avant toute correction, conformément à la règle fondamentale.
- Le backfill doit capturer la vraie page `39.2`, pas `39.1`.
- La manière de corriger la référence historique devra conserver l'audit de l'ancienne valeur et de la nouvelle valeur vérifiée.

Cette découverte confirme l'intérêt majeur de la règle décidée par l'utilisateur : rattacher les supports visuels originaux permet aussi de contrôler les faits déjà enregistrés et de détecter des erreurs de provenance qui resteraient invisibles avec du texte seul.

## PROCHAINE POUSSE AUTORISÉE DU CHANTIER

Avant toute extraction d'images supplémentaires, la prochaine pousse sur `tmp-rave-visual-backfill` sera limitée à la documentation du chantier :

1. ajouter un audit de cartographie RCL0194 avec les index ci-dessus ;
2. corriger dans l'inventaire de chantier la cible visuelle `39.1` → **`39.2`**, tout en mentionnant explicitement que les trois faits legacy portent encore `39.1` ;
3. ne modifier encore aucun qz64, fait historique, code IA, protocole, UI ou build.

Après cette pousse documentaire et sa consignation au rapport, le chantier pourra préparer les assets RCL0194 manquants : `15.1`, `20.1`, `20.2`, `39.2` et la légende couleurs, en réutilisant `20.3`, `20.4`, `39.3`.

`MEMSX64` reste strictement BUILD #101.

# REGLES PERMANENTES — SEPARATION BASE / COMMUNICATION ECU ET SIGNALEMENT DES EVOLUTIONS — 29 AOUT 2026

## SEPARATION ABSOLUE BASE DOCUMENTAIRE / COMMUNICATION ECU

Regle fondamentale fixee par l'utilisateur :
- Les informations de la base de donnees, RAVE, recherches externes, illustrations, procedures, tableaux ou connaissances IA sont documentaires et consultatives.
- Elles ne doivent jamais prendre la main sur le programme de communication avec l'ECU.
- La connexion, detection, protocole, trames, polling 7D/80, Mode 4, commandes diagnostic, securites, ecritures, resets et protections restent pilotes exclusivement par le code/protocole valide.
- Une donnee documentaire ne peut pas, a elle seule, declencher une commande ECU, modifier un mode de communication, choisir une commande dangereuse ou contourner une protection.
- Toute future integration base/IA doit preserver cette frontiere d'architecture et etre testee comme telle.

## SIGNALEMENT DES EVOLUTIONS POSSIBLES

Lorsqu'une recherche RAVE, Internet, un audit de base ou une comparaison de source revele une information qui pourrait permettre une evolution du programme lui-meme, l'assistant doit conserver l'information, la signaler explicitement a l'utilisateur, expliquer brievement l'evolution possible et ne pas modifier le programme sans autorisation explicite.

## MARQUEUR DE DECISION IMPORTANTE

Quand une information, anomalie ou possibilite d'evolution necessite une reponse/decision de l'utilisateur, utiliser le marqueur :
**🟥 IMPORTANT — REPONSE DE TA PART NECESSAIRE**
et attendre cette reponse avant de franchir la decision concernee. Pour un point notable sans decision obligatoire, utiliser **🔴 IMPORTANT** sans bloquer le travail.

Ces regles sont permanentes et s'ajoutent aux regles de journalisation, provenance, securite protocole et conservation maximale des informations.

# AVANT POUSSE — OUTILLAGE D'EXTRACTION VISUELLE RCL0194 — 29 AOUT 2026

Prochaine pousse autorisee uniquement sur `tmp-rave-visual-backfill`.

Objectif : ajouter un outillage temporaire et deterministe pour extraire depuis le PDF constructeur exact RCL0194ENG les supports visuels manquants deja verifies dans l'audit :
- page de legende `COLOUR CODES` — PDF index 5 ;
- `15.1` — PDF index 13 ;
- `20.1` — PDF index 15 ;
- `20.2` — PDF index 16 ;
- `39.2` Cooling Fan MPi — PDF index 25.

L'outillage devra egalement reutiliser les assets pilotes `20.3`, `20.4`, `39.3` sans duplication et verifier le SHA-256 du PDF source.

Cette pousse d'outillage ne doit modifier aucun fait historique, aucun protocole ECU, aucune acquisition/RAM, aucune commande ECU, aucune protection et aucun fichier 32 bits. `MEMSX64` reste strictement #101 `22dbe75ed14e0a61e694159d505ef72245116b48`.

La divergence legacy `39.1` / source reelle `39.2` reste auditee ; aucune correction silencieuse du fait historique n'est autorisee.

# INCIDENT OUTILLAGE BRANCHE TEMPORAIRE — 29 AOUT 2026

Pendant la preparation de la pousse atomique de l'outillage RCL0194, une mauvaise action du connecteur GitHub a cree le commit parasite `9f7c0041572fa2d3c26543c5968b0fdaf38a5883` sur `tmp-rave-visual-backfill`.

Audit immediat : comparaison avec le parent `939a18d1b197cd8a2277180b9aa7384e05225807` = **un seul fichier ajoute, `dummy`, une ligne**. Aucun code applicatif, aucune base, aucun protocole ECU, aucune acquisition/RAM, aucune protection et aucun fichier 32 bits n'ont ete modifies.

Le vrai commit atomique prepare avant l'incident est `5342d25536a0c71354fe6ed6d1f366957494f75a`, parent direct `939a18d1...`, et contient uniquement :
- `tools/extract_rcl0194_visual_assets.py` ;
- `.github/workflows/temp-rcl0194-visual-extract.yml`.

Action de nettoyage autorisee sur la branche temporaire uniquement : replacer `tmp-rave-visual-backfill` sur `5342d255...`, ce qui supprime le commit parasite de la tete active de cette branche de travail. `MEMSX64` reste strictement #101 `22dbe75ed14e0a61e694159d505ef72245116b48`.

# RCL0194 — EXTRACTION VISUELLE VERTE ET CANDIDAT BACKFILL 1750 — 29 AOUT 2026

## RESULTAT OUTILLAGE D'EXTRACTION

Branche temporaire : `tmp-rave-visual-backfill`.
Commit outillage propre : `5342d25536a0c71354fe6ed6d1f366957494f75a`.
Workflow `TEMP RCL0194 VISUAL EXTRACT` : run `33248136914`, job `99089056030` — **SUCCESS**.
Artefact : `RCL0194-VISUAL-ASSETS-BACKFILL-PREP`, ID `9713497504`, taille 854907 octets, digest `sha256:e55c5e37ab97d12ee60648036e67e5c9239d489b4655250d6df9bfec463d60de`.

Source constructeur utilisee : `RCL0194ENG`, MINI Electrical Circuit Diagrams, 3rd Edition, 41 pages PDF.
SHA-256 du PDF exact : `64e64f8a7c24f362913e2661403bc474e4e7ef07f96db618ef661645e0d0f051`.

Cinq nouveaux supports ont ete extraits et verifies :
- `COLOUR CODES`, index PDF 5, 1700x696, SHA-256 `718af9c48d7cc466b5c50278680ecb286de81a869d8e2293207796afe08833d3` ;
- `15.1`, index 13, 1696x1190, SHA-256 `fa3a9f7620b660dc1405adf5bb330f74b9cf627e6ebc7bf2e3ac1b7a42016eeb` ;
- `20.1`, index 15, 1696x1190, SHA-256 `334eea87abb0b9b5692c18ef157f51aa4a2edcc1580dd3d911ba7c3262bb9052` ;
- `20.2`, index 16, 1696x1190, SHA-256 `3736908a793e46a5e3bee32d1d51544caf52557475317a2a5f96ca3d16177c9e` ;
- `39.2`, index 25, 1696x1190, SHA-256 `e3e4c30e318aca2f9aeeb6984840397ec0bc5432d3c15034e2e68e6b2c0a6d22`.

Reutilisation sans duplication des trois assets deja valides par le pilote :
- `20.3` SHA-256 `2988614881aa2d713788eac084c97ae148595bbb19da21fd92b484041ef3e4c8` ;
- `20.4` SHA-256 `7a04d84ee0296ab41333650f369aa3624f3ac7988a528584237a7b8fb021bea0` ;
- `39.3` SHA-256 `b8da82236a56034cd3afde692752ae6ccdef2bc792f5dacfe14392ffc0f00188`.

Les cinq nouveaux supports ont ete controles visuellement. La legende couleurs est bien la page Rover `COLOUR CODES`; `20.1`/`20.2` sont bien les schemas MEMS MPi; `39.2` porte bien le titre `COOLING FAN — MPi`.

## CONFIRMATION TECHNIQUE MAP MPi PAR SOURCE PRIMAIRE

L'inspection directe de `RCL0194ENG:20.2` confirme un **MAP SENSOR externe (204) sur le MPi**, connecteur C186, avec les liaisons visibles vers l'ECM notamment C159-36 (RG), C159-8 (YP) et retour capteurs C159-13 (KB). Cette preuve constructeur confirme la distinction avec la SPi, dont le MAP est integre. Aucun changement du programme ECU n'est deduit ni applique automatiquement de cette confirmation.

## CANDIDAT ADDITIF `research_enrichment_1750.qz64`

Un candidat local complet a ete genere a partir de la base officielle #101, sans modifier les faits historiques.

Le lot 1750 ajoute :
- `mems_rave_illustration` avec type d'asset (`wiring_diagram`, `legend`) et provenance/hash ;
- `mems_rave_illustration_link` permettant un fait vers plusieurs supports ;
- `mems_rave_source_reference_correction` pour enregistrer les corrections de provenance sans reecrire silencieusement l'historique ;
- index et vue de resolution fait-vers-support.

La divergence historique `39.1` est conservee dans le fait legacy et corrigee additivement vers la vraie source `39.2` pour `RAVE-WIR-MPI-006`, `RAVE-WIR-MPI-008` et `RAVE-WIR-MPI-012`.

Les references composites (`20.1-20.2`, `20.3-20.4`) sont liees aux deux pages correspondantes. Les 26 faits couleur SPi Japon recoivent en plus la page `COLOUR CODES` comme legende de preuve complementaire.

Validation sur copie exacte de la SQLite officielle #101 :
- `PRAGMA integrity_check = ok` ;
- `user_version = 20` ;
- 93 faits RAVE conserves ;
- 105 faits experts conserves ;
- 8 assets RCL0194 ;
- 55 faits RCL0194 possedent au moins un lien visuel direct/corrige ;
- 26 faits couleur possedent la legende complementaire ;
- 3 corrections de reference `39.1 -> 39.2` ;
- aucune image physique `39.1` n'est creee comme faux support de Cooling Fan MPi.

`research_enrichment_1750.qz64` : 3105 octets, SHA-256 `969c2b23b8e33e9bcac4ff653752186fad2fde8b5952439bf808d1ac2b56de02`.
SQL decompresse : 36596 octets, SHA-256 `6ca6a2f82e948bacdd6995615fbea98d80571b3f87f80f8b93f46869a5bb0174`.

## AVANT PROCHAINE POUSSE — PERIMETRE EXACT

Prochaine pousse autorisee uniquement sur `tmp-rave-visual-backfill`, en un commit atomique :
1. les 8 images RCL0194 sous `database/reference/images/rave/` ;
2. `database/reference/research_enrichment_1750.qz64` ;
3. mise a jour additive de `database/reference/manifest.json` ;
4. audit `database/reference/audits/rcl0194_visual_backfill_1750_audit.md`.

Aucun code de communication ECU, protocole, acquisition/RAM, UI de production, Qwen/ONNX, 32 bits ni branche `MEMSX64` ne doit etre modifie par cette pousse. `MEMSX64` reste strictement BUILD #101 `22dbe75ed14e0a61e694159d505ef72245116b48`.

# AVANT POUSSE — MECANISME ATOMIQUE DES ASSETS BINAIRES RCL0194 — 29 AOUT 2026

Pour garantir que le vrai lot de donnees soit conserve en un commit coherent malgre les PNG binaires, la prochaine pousse sur `tmp-rave-visual-backfill` ajoute uniquement un workflow temporaire auto-supprimant de packaging. Ce helper reconstruira les 8 PNG depuis le PDF Rover exact avec PyMuPDF 1.26.4, verifiera tous les SHA-256 deja valides, installera le `research_enrichment_1750.qz64`, mettra a jour le manifeste et ajoutera l'audit, puis creera un seul commit de donnees.

Le helper n'est pas un changement de production : aucun protocole ECU, aucune acquisition/RAM, aucun code IA/communication, aucun 32 bits et aucun `MEMSX64` ne sont modifies. Le commit de donnees resultant doit contenir uniquement les 8 images RCL0194, le lot 1750, le manifeste, l'audit, plus la suppression du helper temporaire lui-meme. Toute difference de hash doit faire echouer le workflow avant commit.

# ECHEC INTERMEDIAIRE PACKAGING RCL0194 1750 — 29 AOUT 2026

Workflow temporaire : `TEMP RCL0194 COMMIT BACKFILL`, run `33248437871`, job `99089830021` — **FAILURE avant tout commit de donnees**.

Etapes confirmees vertes avant l'echec : checkout, Python, PyMuPDF 1.26.4, telechargement du PDF constructeur avec SHA-256 exact, puis rendu/verifications des **8 images RCL0194**. Les huit assets ont retrouve exactement les SHA-256 deja valides, incluant les trois assets du pilote.

L'echec survient uniquement a l'etape `Install validated batch 1750 payload` : le qz64 recopie dans un heredoc du workflow ne reproduit pas exactement le SHA-256 attendu `969c2b23b8e33e9bcac4ff653752186fad2fde8b5952439bf808d1ac2b56de02`. Le garde de hash a donc correctement bloque la suite. Aucun manifeste, audit ou commit de donnees n'a ete pousse par ce run.

Conclusion : les assets visuels sont valides ; le defaut concerne seulement le mecanisme de transfert du petit fichier qz64 dans le helper. Ne pas regenerer le SQL ni modifier les donnees pour contourner le garde.

Correction retenue avant nouvelle tentative : ajouter le **qz64 exact deja valide** comme blob GitHub, verifier son hash dans le workflow, puis ne plus le reconstruire par heredoc. Apres succes, produire un commit candidat propre base directement sur l'etat de chantier avant helpers, afin que l'etat final ne conserve que les donnees/audits voulus et pas l'outillage temporaire.

`MEMSX64` reste strictement BUILD #101 `22dbe75ed14e0a61e694159d505ef72245116b48` et n'a subi aucun changement.

# SECOND ECHEC INTERMEDIAIRE QZ64 RCL0194 1750 — 29 AOUT 2026

Run temporaire `33248530197` — **FAILURE avant extraction/commit de donnees**. Le workflow s'est arrete sur le premier garde `Verify exact batch 1750 payload already committed` : le blob envoye via la voie texte UTF-8 ne reproduisait toujours pas les octets exacts du qz64 valide.

Aucun asset, manifeste, audit ou commit de donnees n'a ete produit par ce run. L'echec confirme que le probleme est strictement le transport octet-par-octet du qz64, pas son SQL ni sa validation semantique.

Correction suivante : utiliser l'API GitHub `create_blob` en **encoding base64** a partir des octets exacts du fichier local valide, puis verifier le SHA-256 `969c2b23b8e33e9bcac4ff653752186fad2fde8b5952439bf808d1ac2b56de02` avant toute autre etape. Ne pas modifier/regenerer les donnees pour faire passer le test.

`MEMSX64` reste #101 `22dbe75ed14e0a61e694159d505ef72245116b48` sans modification.

# TROISIEME ECHEC DE TRANSFERT QZ64 ET CHANGEMENT DE METHODE — 29 AOUT 2026

La relance du run `33248530197` apres tentative de transfert binaire/base64 du qz64 s'est de nouveau arretee sur le garde SHA-256 avant toute autre etape. Aucun asset/manifeste/audit n'a ete pousse par cette tentative.

Decision technique : **ne pas affaiblir le garde et ne plus transporter le qz64 a travers la conversation**. La nouvelle tentative reconstruira deterministement `research_enrichment_1750.qz64` dans GitHub Actions a partir de la SQLite officielle BUILD #101 et des 55 faits `RCL0194ENG` reels. Le workflow utilisera les `image_ref` historiques comme entree, appliquera explicitement l'audit `39.1 -> 39.2` uniquement aux trois faits identifies, ajoutera la legende couleurs aux 26 faits `RAVE-COLOR-SPIJ-*`, puis generera le qCompress/qz64 localement.

Avant commit, le SQL genere sera applique a une copie exacte de `ia_mems_reference_r20.sqlite` #101 et devra verifier au minimum : `integrity_check=ok`, `user_version=20`, 93 faits RAVE, 105 faits experts, 8 assets RCL0194, 55/55 faits RCL0194 lies, 26 liens de legende couleurs et exactement 3 corrections de provenance. Les 8 PNG devront egalement retrouver leurs SHA-256 deja valides.

Cette methode peut produire un qz64 de bytes/hash differents du candidat local precedent en raison du formatage deterministe regenere, mais **les donnees et invariants cibles ne changent pas**. Le nouveau hash sera enregistre apres validation ; aucun fait historique ne sera reecrit.

`MEMSX64` reste strictement BUILD #101 `22dbe75ed14e0a61e694159d505ef72245116b48`.

## 2026-08-29 — RCL0194 visual backfill 1750 — deterministic validation run 33249238690

### State before test
- Production remains strictly `MEMSX64`_ BUILD #101 / `22dbe75ed14e0a61e694159d505ef72245116b48`.
- Working branch: `tmp-rave-visual-backfill`.
- Validation commit: `55ada9d9f7296e88d4d04a66802647efc5c946e3`.
- No production build #102 was started.
- Scope remains documentary/database only; ECU communication/protocol/acquisition/write safety and 32-bit path are untouched.

### Test executed
GitHub Actions workflow `TEMP RCL0194 DETERMINISTIC 1750 VALIDATE`, run `33249238690`, job `99091921358`.
The workflow rebuilt the reference SQLite from the shipped BUILD #101 seed/enrichment set, regenerated `research_enrichment_1750.qz64` using the Qt qCompress-compatible framing expected by MEMS Manager, applied the generated lot to a copy of that reconstructed SQLite, rendered the eight exact RCL0194 source visuals from the verified Rover PDF, then ran invariants and a documentary-scope guard.

### Actual result
**Overall verdict: ❌ FAIL at the final scope guard, with the actual 1750 generation/database validation itself ✅ PASS.**

Successful deterministic checks before the guard failure:
- `RCL0194_1750_PASS`;
- `PRAGMA integrity_check = ok`;
- `PRAGMA user_version = 20`;
- historical `mems_rave_fact` count remains **93**;
- historical expert fact count remains **105**;
- RCL0194 source assets = **8**;
- direct/effective visual coverage = **55/55 RCL0194 facts**;
- SPi Japan colour-code legend links = **26/26**;
- explicit additive source corrections `39.1 -> 39.2` = **3**;
- all eight PNGs reproduced their previously verified SHA-256 values exactly;
- generated qz64 SHA-256 = `200b2d7ec0ba24d93d7192fdf63f86845c53f49ad4a28cb997ede9d39fb5f51d`;
- decompressed SQL SHA-256 = `e9b10b33b67165dc14effc81fb8a62308ab5340ed8a5d38b0d7f6aec43191090`.

Failure cause:
- the documentary-scope guard also saw two **ephemeral runner work products**: `.tmp-rcl0194-1750/` and `RCL0194ENG.pdf`;
- it therefore returned `Unexpected candidate paths: .tmp-rcl0194-1750/, RCL0194ENG.pdf`;
- artifact creation/upload was consequently skipped;
- **no 1750 data commit and no RCL0194 PNG commit occurred**.

### Technical conclusion
The previous qz64 transport problem is solved by deterministic reconstruction: the generated lot round-trips and applies correctly using the project qz64 contract. The failure is isolated to the guard seeing temporary build inputs, not to the database, SQL, qz64, RAVE mapping, visual hashes, or historical-data preservation.

### Exact next action
Before any new data push, correct only the temporary validation workflow so that `.tmp-rcl0194-1750/` and `RCL0194ENG.pdf` are removed after generation and before the Git scope guard. Re-run the same deterministic validation unchanged. If it passes, record that test in this report before committing the generated 1750/PNG/manifest/audit candidate.

## 2026-08-29 — RCL0194 backfill 1750 — PASS déterministe

Test GitHub Actions : `TEMP RCL0194 DETERMINISTIC 1750 VALIDATE`, run `33249367487`, job `99092252320`, HEAD `11e891e15ce761201f1b649d4ae18949d7dc4ca7`.

**Verdict : ✅ PASS.** La seule correction depuis le run précédent a été la suppression des fichiers temporaires `.tmp-rcl0194-1750/` et `RCL0194ENG.pdf` avant le garde de périmètre.

Validations :
- `PRAGMA integrity_check = ok`, `user_version = 20`;
- 93 faits RAVE et 105 faits expert historiques inchangés;
- 8 visuels RCL0194;
- couverture visuelle directe/effective 55/55 faits RCL0194;
- 26/26 faits couleurs SPi Japon reliés à la légende constructeur;
- 3 corrections additives `39.1 -> 39.2`, sans réécrire les faits historiques;
- qz64 SHA-256 `200b2d7ec0ba24d93d7192fdf63f86845c53f49ad4a28cb997ede9d39fb5f51d`;
- SQL SHA-256 `e9b10b33b67165dc14effc81fb8a62308ab5340ed8a5d38b0d7f6aec43191090`;
- garde documentaire ✅; artefact candidat ✅.

Artefact : `RCL0194-1750-DETERMINISTIC-CANDIDATE`, ID `9713857193`, 1,356,869 octets, digest `sha256:5f82d02140edb7a1232f3372b2da269db4db3b5b6f025dcdb370f31fa8b86201`.

Production reste `MEMSX64` #101 `22dbe75ed14e0a61e694159d505ef72245116b48`; aucun #102. Communication ECU et 32 bits intacts.

**Prochaine action exacte :** vérifier `SHA256SUMS.txt` de l'artefact puis pousser exactement qz64 1750 + manifeste + audit + 8 PNG sur `tmp-rave-visual-backfill`, nettoyer les workflows temporaires, puis lancer une validation post-pousse et l'inscrire au rapport avant le lot suivant.

## 2026-08-29 — Vérification artefact RCL0194 1750 après PASS

Après le PASS du run `33249367487`, l'artefact `9713857193` a été téléchargé et son `SHA256SUMS.txt` vérifié avant toute pousse.

**Verdict artefact : ❌ à refaire pour un défaut d'emballage uniquement.**
Les 12 fichiers utiles vérifiés correspondent tous à leurs SHA-256 déclarés : qz64 1750, manifeste, audit, 8 PNG et résumé de validation. Le qz64 reste `200b2d7ec0ba24d93d7192fdf63f86845c53f49ad4a28cb997ede9d39fb5f51d`.

Seul défaut : `SHA256SUMS.txt` s'inclut lui-même dans la commande `find`; sa ligne propre contient donc le SHA-256 du fichier vide avant remplissage et devient invalide après écriture. Aucune donnée n'a été poussée.

**Prochaine action exacte :** corriger uniquement la construction de `SHA256SUMS.txt` pour exclure `SHA256SUMS.txt` lui-même, relancer le même test déterministe, puis revalider l'artefact avant toute pousse. Production #101 et communication ECU inchangées.

## 2026-08-29 - RCL0194 1750 - run3 + artefact PASS

Test: workflow `TEMP RCL0194 DETERMINISTIC 1750 VALIDATE`, run `33249539952`, job `99092714072`, HEAD `cd5ec9fbef4687cfaa782f8ae314c6fa50e481b1`.

Verdict: PASS. Toutes les etapes GitHub sont vertes. Invariants: integrity=ok, user_version=20, 93 faits RAVE historiques, 105 faits expert historiques, 8 visuels RCL0194, couverture 55/55, legende couleurs 26/26, 3 corrections additives 39.1 -> 39.2, sans reecriture des faits historiques.

Artefact `RCL0194-1750-DETERMINISTIC-CANDIDATE`, ID `9713908641`, taille `1,356,818` octets, digest GitHub `sha256:f5f0c864314a2addb0e94640e8ecd2d15216b1a59cdba7d308a0970ec26ff525`. Qz64 SHA-256 `200b2d7ec0ba24d93d7192fdf63f86845c53f49ad4a28cb997ede9d39fb5f51d`.

Verification locale de l'artefact: PASS. `sha256sum -c SHA256SUMS.txt` retourne OK pour 12/12 fichiers utiles. `SHA256SUMS.txt` ne se reference plus lui-meme. Les 8 PNG correspondent aux hashes deja verifies.

Conclusion: le candidat 1750 est reproductible et verifie de bout en bout avant commit. Les PNG restent des preuves visuelles associees a la base structuree. La base/IA reste consultative et ne prend jamais la main sur la communication ECU.

Prochaine action exacte: pousser atomiquement les octets valides du qz64 1750, manifeste, audit et 8 PNG sur `tmp-rave-visual-backfill`, conserver le generateur, retirer les workflows temporaires inutiles, puis lancer une validation post-pousse depuis les fichiers reellement commites et mettre le rapport a jour avant le lot suivant. Production reste MEMSX64 BUILD #101 `22dbe75ed14e0a61e694159d505ef72245116b48`; aucun #102.

## 2026-08-29 - RCL0194 1750 - plan exact avant pousse des donnees

Etat: production `MEMSX64` reste BUILD #101 `22dbe75ed14e0a61e694159d505ef72245116b48`, aucun #102. Branche de travail `tmp-rave-visual-backfill`, HEAD avant nouvelle pousse `cd5ec9fbef4687cfaa782f8ae314c6fa50e481b1`. Le candidat du run #3 `33249539952` et son artefact `9713908641` ont ete valides, y compris 12/12 SHA-256.

Constat avant pousse: l'ancien helper `.github/workflows/temp-rcl0194-commit-backfill.yml` est obsolete et contient encore l'ancien hash qz64 `969c2b...`; il ne doit pas etre utilise tel quel.

Objectif de la prochaine pousse technique: remplacer uniquement ce helper sur la branche temporaire par un mecanisme qui regenere le candidat avec `tools/build_rcl0194_visual_backfill_1750.py`, exige les hashes valides du run #3 (`research_enrichment_1750.qz64` = `200b2d7ec0ba24d93d7192fdf63f86845c53f49ad4a28cb997ede9d39fb5f51d`), verifie manifeste/audit/8 PNG, committe uniquement le lot documentaire valide, supprime les helpers temporaires RCL0194 devenus inutiles, puis pousse sur `tmp-rave-visual-backfill`.

Validation obligatoire apres cette pousse: le meme run doit faire un `fetch` du nouveau commit distant, se replacer exactement dessus, regenerer 1750 depuis la base BUILD #101 et le PDF Rover verifie, refaire les invariants SQLite et exiger un diff nul entre les fichiers regeneres et les fichiers reellement committes. Cette validation post-pousse doit etre inscrite au rapport avant toute etape suivante.

Perimetre interdit: aucune modification de `MEMSX64`, communication/protocole/acquisition/RAM/ecriture/reset ECU, Qwen/ONNX, UI ou voie 32 bits. Base/IA/RAVE restent consultatives et ne prennent jamais la main sur la communication ECU.

## 2026-08-29 - RCL0194 visual backfill 1750 - PUSH + POST-VALIDATION COMPLETE

Branche de travail: `tmp-rave-visual-backfill`. Commit documentaire final: `6fcb3da9ac6b4d433108194639f568cd1f4717f6` (`Add deterministic RCL0194 visual backfill 1750`). Run GitHub `33249974116`, job `99093873039`: **✅ SUCCESS complet**.

Tous les controles ont passe: production verifiee encore exactement `MEMSX64` BUILD #101 `22dbe75ed14e0a61e694159d505ef72245116b48`; PDF Rover exact verifie; candidat 1750 regenere; hashes run #3 exiges; garde de perimetre documentaire; commit/push; relecture du commit distant; verification des octets reellement committes; reproduction de 1750 depuis le commit distant avec **diff nul**.

Etat valide du lot RCL0194: `research_enrichment_1750.qz64` SHA-256 `200b2d7ec0ba24d93d7192fdf63f86845c53f49ad4a28cb997ede9d39fb5f51d`; 8 supports visuels constructeur; 55/55 faits RCL0194 avec preuve visuelle directe/effective; 26/26 faits couleur SPi Japon relies a la legende; 3 corrections additives explicites `39.1 -> 39.2`; faits historiques RAVE/expert non reecrits; `user_version=20`; integrity=ok.

Conclusion: **RCL0194 1750 est valide sur la branche de chantier.** Les PNG restent des preuves/illustrations associees a la base structuree, jamais un remplacement de la connaissance structuree. La base/IA/RAVE reste strictement consultative et ne prend jamais la main sur la communication ECU.

Prochaine action exacte: commencer en lecture seule l'inventaire visuel de `RCL0193ENG`, qui porte 31 faits RAVE deja presents. Identifier toutes les pages/figures/tableaux/vues correspondant a ces 31 faits, detecter les references composites ou erronees, puis mettre le rapport a jour **avant toute pousse** du lot RCL0193. Aucun #102 et aucune modification `MEMSX64`, protocole ECU, acquisition, RAM, ecriture/reset, Qwen/ONNX, UI ou 32 bits.

## 2026-08-29 - DEMARRAGE LOT RCL0193 - INVENTAIRE AVANT ENRICHISSEMENT

Autorisation utilisateur: continuer l'enrichissement de la base. La prochaine source traitee est `RCL0193ENG` (Mini Workshop Manual), qui correspond a **31 faits RAVE deja presents** dans le lot historique `research_enrichment_1660.qz64`.

Objectif avant toute pousse de donnees: extraire exactement ces 31 faits existants et leurs `source_section/image_ref`, retrouver le PDF constructeur RCL0193 exact, cartographier chaque fait vers la vraie page/figure/tableau, detecter les references composites, incompletes ou erronees, puis seulement preparer un lot additif de supports visuels/metadonnees. Aucune reecriture des faits historiques.

Contraintes: `MEMSX64` reste strictement sur BUILD #101 `22dbe75ed14e0a61e694159d505ef72245116b48`; aucun #102; aucune modification protocole ECU, acquisition 7D/80, RAM Mode 4, ecriture/reset, IA Qwen/ONNX, UI, packaging production ou 32 bits. Les illustrations restent des preuves associees a la connaissance structuree et ne la remplacent jamais.

Prochaine action exacte: sur `tmp-rave-visual-backfill`, produire un inventaire RCL0193 reproductible a partir du `1660` commite et du manuel constructeur, sans modifier la base de production; verifier cet inventaire avant de creer le nouveau lot additif.

## 2026-08-29 - RCL0193 INVENTAIRE RUN #1 - ECHEC DU GARDE, DONNEES NON POUSSEES

Run temporaire `33250697659`, job `99095759561`, commit declencheur `5906694b67dc9338958a9fd497618865e4836d57`: **❌ FAILURE**.

Resultat detaille: checkout ✅; installation PyMuPDF ✅; decodage reproductible de `research_enrichment_1660.qz64` + telechargement/indexation du PDF RCL0193 ✅; etape `Guard scope` ❌; commit final saute, donc **aucun audit RCL0193 et aucune donnee RCL0193 n'ont ete pousses**.

Cause identifiee: le garde utilisait `git diff --name-only` pour exiger que le seul changement soit le nouveau fichier `database/reference/audits/rcl0193_1660_inventory.md`. Or un fichier nouvellement cree et encore non suivi n'apparait pas dans `git diff --name-only`; le garde echoue donc meme si le perimetre est correct. Ce n'est pas un echec de decodage du lot 1660 ni du PDF constructeur.

Correction autorisee avant nouvelle pousse: remplacer uniquement ce controle de perimetre par un controle `git status --porcelain` qui accepte exactement le nouveau fichier d'audit et refuse tout autre changement; conserver tous les autres controles et ne modifier aucune donnee historique.

## 2026-08-29 - RCL0193 INVENTAIRE RUN #2 - SUCCESS

Correction du garde appliquee apres journalisation de l'echec #1. Run `33250747167`, job `99095887607`, declencheur `28b39dd92e097401a83bb765345d39b0a7321b35`: **✅ SUCCESS**. Le workflow a decode `research_enrichment_1660.qz64`, telecharge/indexe le manuel constructeur RCL0193, valide le perimetre puis pousse uniquement l'audit et retire son helper.

Commit final de l'audit sur `tmp-rave-visual-backfill`: `05497561d33e0407c4faac0e5d64c911d010b465` (`Inventory RCL0193 visual backfill sources`). Fichier: `database/reference/audits/rcl0193_1660_inventory.md`.

Empreintes verifiees par l'audit: qz64 1660 SHA-256 `5d56f198a6f5ccaf36d7f94646e3b9af4efd465e7f80da0ebea3d74f3d894093`; SQL decode 41443 octets, SHA-256 `b978f7e73487c6243c8a6c355b4e5dedd90cf449bee223e9c1eb81e08eb0a7cd`; PDF RCL0193 372 pages, SHA-256 `c050a3eebe50c5a85bf8a69b7722bd2052079944e09d58578a498984ecf06715`.

L'audit confirme que les faits RCL0193 historiques ont `image_ref` vide: le chantier visuel est donc bien additif. Aucun fait historique, aucune table historique et aucun fichier production n'ont ete modifies.

Prochaine action exacte: parser proprement les 31 lignes `mems_rave_fact` RCL0193 (sans couper sur les points-virgules internes), resoudre chaque `source_section` vers la page PDF constructeur reelle et produire la liste unique des pages/figures necessaires. Apres verification et nouvelle journalisation, preparer le lot visuel additif RCL0193 en reutilisant `mems_rave_illustration` + `mems_rave_illustration_link`. `MEMSX64` reste #101 inchange.

## 2026-08-29 - RCL0193 CARTOGRAPHIE RUN #1 - FILTRE DOCUMENT TROP STRICT

Run temporaire `33250884473`, job `99096243600`, declencheur `09d3160ad18b6b06717e4f8bc7c4d01406fd2fa0`: **❌ FAILURE**, avant toute ecriture de l'audit ou de la base.

Les etapes checkout et PyMuPDF sont passees. La reconstruction de la base de reference a fonctionne, mais le script a exige `document='RCL0193ENG'` et a obtenu 0 ligne. Verification directe du lot historique 1660: le champ `document` reel des faits vaut **`Mini Workshop Manual — RCL0193ENG`**. Exemple confirme: `RAVE-ADJ-THROTTLE-CABLE-001`, source `SRC-RCL0193`, document `Mini Workshop Manual — RCL0193ENG`.

Conclusion: les 31 faits historiques ne sont pas absents; c'est uniquement le filtre de cartographie qui etait trop strict. Aucun fait historique, aucun qz64, aucune image et aucun fichier production n'ont ete modifies ou pousses par ce run.

Correction suivante autorisee: remplacer uniquement le filtre SQL par la valeur documentaire historique exacte `Mini Workshop Manual — RCL0193ENG`, conserver l'assertion de 31 faits et tous les autres controles. Continuer a parser le 1660 avec `sqlite3.complete_statement` afin de ne plus couper sur les points-virgules contenus dans les textes.

## 2026-08-29 - RCL0193 - PLAN AVANT PARSING 31/31 ET CARTOGRAPHIE SOURCE

Etat avant pousse technique: l'inventaire reproductible 1660 est valide et commite (`05497561d33e0407c4faac0e5d64c911d010b465`). Production reste strictement `MEMSX64` BUILD #101 `22dbe75ed14e0a61e694159d505ef72245116b48`; aucun #102.

Objectif de la prochaine pousse sur `tmp-rave-visual-backfill`: ajouter uniquement un workflow temporaire d'audit qui decode `research_enrichment_1660.qz64`, parse proprement les INSERT `mems_rave_fact` en respectant les chaines SQL, apostrophes doublees et points-virgules internes, exige exactement 31 faits RCL0193 uniques, puis resout chaque `source_section` vers les pages physiques du PDF constructeur RCL0193 (372 pages, SHA-256 deja valide `c050a3eebe50c5a85bf8a69b7722bd2052079944e09d58578a498984ecf06715`).

Le mapping ne doit jamais supposer un offset de page: pour chaque reference `PDF p.N` ou plage, le workflow doit verifier le contenu de la page candidate par rapport au sujet/section attendu et signaler toute divergence. Il doit produire un audit textuel lisible listant les 31 faits, leurs pages, types de support utiles (tableau, procedure, illustration, vue, avertissement) et la liste unique des assets a capturer.

Aucun qz64 de donnees, aucune image et aucun fait historique ne doivent etre modifies par cette etape. La base/IA/RAVE reste consultative et ne prend jamais la main sur la communication ECU. Toute decouverte pouvant justifier une evolution du programme sera signalee explicitement a l'utilisateur.

## 2026-08-29 - RCL0193 CARTOGRAPHIE RUN #2 - MAPPING OK, GARDE FINAL EN ECHEC

Run temporaire `33250967976`, job `99096454241`, correction declencheur `c85a8022d657bc16a5a469c29d6b683c1438734c`: **⚠️ cartographie technique reussie, workflow final FAILURE au garde de perimetre**.

Resultats de l'etape de cartographie avant le garde: `FACTS_PASS 31`; `STATEMENTS_PASS 31` avec `sqlite3.complete_statement`; **25 pages PDF uniques** indexes `[38,39,40,98,101,107,108,109,112,113,114,117,118,120,121,122,123,125,126,127,128,129,130,131,135]`; `WEAK_CASES 1`. Le PDF exact et les assertions documentaires sont passes.

Le garde a ensuite echoue parce que la reconstruction de la base cree volontairement le workspace local `.tmp-rcl0193-map/` avec le SQLite temporaire, alors que le garde exigeait que le seul fichier non suivi soit l'audit Markdown. Le commit final a ete saute: **aucun audit de mapping, aucune image et aucune donnee RCL0193 n'ont ete pousses par ce run**.

Correction suivante autorisee: supprimer uniquement `.tmp-rcl0193-map/` apres construction de l'audit et avant `git status --porcelain`, puis conserver exactement le meme mapping et les memes assertions. Le cas `WEAK_CASES 1` devra etre identifie dans l'audit commite avant de generer le lot visuel additif.

## 2026-08-29 - RCL0193 MAPPING 31/31 RUN #1 - ECHEC UTILE / ANCIEN INVENTAIRE CORRIGE

Run `33251002269`, job `99096541223`, declencheur `bf6c4126ca5f6044f00496c72412e2fba56cc244`: **❌ FAILURE avant tout commit d'audit**. Aucune donnee, image, qz64 ni fait historique RCL0193 n'a ete pousse.

Etapes validees avant l'echec: production encore exactement MEMSX64 #101; qz64 1660 SHA-256 valide; PDF constructeur RCL0193 372 pages et SHA-256 valide; decodage SQL 41443 octets et hash valide; parseur SQL robuste respectant les apostrophes doublees et points-virgules internes; exactement **31 faits mems_rave_fact RCL0193 uniques** retrouves, tous `verifie_constructeur` et tous avec `image_ref` historique vide.

Cause exacte de l'echec: le garde comparait la liste des pages issue du parseur robuste avec l'ancienne liste d'inventaire qui contenait `170`. Le parseur 31/31 retourne en realite les pages uniques suivantes: `38,39,40,98,101,107,108,109,112,113,114,117,118,120,121,122,123,125,126,127,128,129,130,131,135`, soit **25 pages uniques**. La page `170` n'est citee par aucun des 31 faits RCL0193 correctement parses.

Conclusion: l'ancienne mention `26 pages` / page `170` provenait de l'inventaire initial imparfait qui analysait les statements SQL sans parser correctement les chaines contenant des points-virgules. Cette ancienne conclusion est desormais remplacee par le resultat du parseur structure 31/31: **25 pages uniques pour les 31 faits historiques RCL0193 du lot 1660**. Aucun fait historique n'est modifie par cette correction d'audit.

Correction autorisee avant relance: modifier uniquement le garde du workflow temporaire pour exiger les 25 pages reellement extraites, conserver les controles durs de contenu et poursuivre la verification page/sujet. Si une autre divergence apparait, arret + rapport avant toute correction suivante.

## 2026-08-29 - RCL0193 CARTOGRAPHIE RUN #3 + WORKFLOW PARALLELE - ECHECS JOURNALISES

### Run principal de cartographie #3

Commit declencheur `8291a0764b62562afad403b016daae5f2bc63a82` (`Fix RCL0193 mapping temporary workspace guard`), run `33251020966`, job `99096588037`: **⚠️ mapping reussi, garde final FAILURE**.

Le coeur de la cartographie repasse exactement: `FACTS_PASS 31`, `STATEMENTS_PASS 31`, `UNIQUE_PAGES 25 [38,39,40,98,101,107,108,109,112,113,114,117,118,120,121,122,123,125,126,127,128,129,130,131,135]`, `WEAK_CASES 1`. Les assertions PDF/1660 et le document historique exact passent. Apres suppression de `.tmp-rcl0193-map`, le test strict `git status --porcelain --untracked-files=all` echoue encore. Les logs de ce run ne montrent pas le chemin supplementaire; il reste donc a l'identifier explicitement avant de modifier le garde. Aucun audit final, aucune image et aucune donnee RCL0193 n'ont ete commis par ce run.

### Workflow parallele `TEMP RCL0193 MAP 1660`

Commit `bf6c4126ca5f6044f00496c72412e2fba56cc244`, run `33251002269`, job `99096541223`: **❌ FAILURE avant commit**. Les entrees immuables passent: production `MEMSX64` toujours exactement BUILD #101 `22dbe75ed14e0a61e694159d505ef72245116b48`, qz64 1660 exact, PDF RCL0193 exact, 31 faits parses, 31 cles uniques, tous `verifie_constructeur`, 31 `image_ref` historiques vides.

Cet autre script a ensuite echoue sur une attente codee en dur de **26 pages**, parce qu'il ajoutait la page `170`. L'extraction reelle des 31 `source_section` donne la meme liste de **25 pages** que le run principal et ne contient pas `170`. Conclusion actuelle: **page 170 exclue**; elle ne doit pas etre ajoutee sans nouvelle preuve constructeur explicite. Le workflow parallele n'a pousse ni audit ni donnee.

### Etat de travail apres ces tests

La conclusion factuelle est donc: **31/31 faits RCL0193 historiques -> 25 pages PDF physiques uniques actuellement citees**. Un seul cas a faible recouvrement lexical (`WEAK_CASES 1`) reste a identifier/revoir avant creation du lot visuel. `MEMSX64` reste #101 inchange; aucun #102.

Prochaine action exacte avant toute nouvelle pousse de donnees: inspecter les helpers temporaires encore presents sur `tmp-rave-visual-backfill`; modifier le run principal uniquement pour afficher le `git status --porcelain --untracked-files=all` exact, supprimer seulement les artefacts temporaires identifies (notamment cache Python si confirme), conserver le garde strict, puis recommencer la cartographie. Le workflow parallele errone ne doit pas servir de source de verite pour la page 170.

## 2026-08-29 - RCL0193 MAPPING V2 - RUN VERT MAIS AUDIT SEMANTIQUE NON VALIDE

Run `33251081226`, job `99096744789`: GitHub **SUCCESS** et commit d'audit `8f228440529f02e2b9b1bc3ff85da0c64ce2d837`. Cependant une relecture immediate de l'audit avant toute creation d'asset a revele un defaut semantique du parseur de references de pages. Le run ne doit donc PAS etre considere comme validation finale du mapping RCL0193.

Cause exacte: la regex `PDF p.N(-M)?` sait developper une plage contigue comme `p.38-39`, mais ne collecte que la premiere occurrence lorsqu'un `source_section` contient plusieurs references non contigues avec `et p.N`.

Deux cas deja verifies dans les 31 faits historiques:
- `RAVE-CAUTION-THROTTLE-STOP-001`: source `PDF p.118 et p.114 / Throttle cable + TP/IACV`; le V2 n'a lie que p.118, alors que p.114 doit aussi etre conserve.
- `RAVE-REP-INJECTORS-001`: source `PDF p.135 et p.170 / Fuel injectors + inlet manifold`; le V2 n'a lie que p.135, alors que p.170 doit aussi etre conserve.

Consequence: la conclusion V2 `25 pages uniques` est invalide. La page `170` est bien citee par un fait RCL0193 et la couverture attendue redevient **26 pages uniques**. L'ancienne conclusion 26 pages n'etait donc pas necessairement une erreur; l'erreur venait de la nouvelle extraction qui ignorait les references secondaires non contigues.

Aucun PNG, qz64 de donnees, manifeste RCL0193 ou fait historique n'a ete pousse a partir de ce mapping incomplet. L'erreur est detectee avant enrichissement effectif.

Correction obligatoire avant toute suite: parser **toutes** les occurrences `p.N` et toutes les plages `p.N-M` dans chaque `source_section`, developper les plages et conserver les references non contigues. Recalculer les 31 mappings, verifier que `RAVE-CAUTION-THROTTLE-STOP-001` contient 114+118 et `RAVE-REP-INJECTORS-001` contient 135+170, puis verifier les 26 pages constructeur. Rapport obligatoire avant toute creation d'assets.

## 2026-08-29 - RCL0193 CARTOGRAPHIE RUN #4 - SUCCESS COMPLET

Commit declencheur `bfca3b9c3bc39e476e48d83a751b018c85ecd7eb`, run `33251145156`, job `99096912361`: **✅ SUCCESS complet**. Etapes cartographie, garde strict, commit et push toutes vertes.

Resultats reproduits: `FACTS_PASS 31`; `STATEMENTS_PASS 31`; **25 pages PDF physiques uniques** `[38,39,40,98,101,107,108,109,112,113,114,117,118,120,121,122,123,125,126,127,128,129,130,131,135]`; `WEAK_CASES 1`. Le lot historique 1660 reste inchange et les 31 `image_ref` historiques restent vides.

Le chemin parasite des runs precedents est maintenant confirme par le log, sans supposition: avant nettoyage, le workspace contenait uniquement le SQLite temporaire `.tmp-rcl0193-map/baseline.sqlite`, le futur audit Markdown et le cache Python genere par l'import `tools/__pycache__/build_rcl0194_visual_backfill_1750.cpython-312.pyc`. Apres suppression des deux artefacts temporaires autorises, le garde strict ne voyait plus que `database/reference/audits/rcl0193_visual_mapping_audit.md`.

Commit final produit par le workflow sur `tmp-rave-visual-backfill`: `a1efab597b8cce35072686490721cb5731d7c499` (`Map RCL0193 facts to original PDF pages`). Le helper `.github/workflows/temp-rcl0193-visual-mapping.yml` a ete retire automatiquement. Aucun fichier production n'a ete modifie.

Conclusion: la correspondance historique **31 faits RCL0193 -> 25 pages constructeur** est maintenant commitee et reproductible. La page `170` n'appartient pas a la liste extraite des 31 faits et reste exclue sans nouvelle preuve. Il reste exactement **1 cas a faible recouvrement lexical** a identifier et verifier dans l'audit avant de creer les images/liaisons additives.

Prochaine action exacte: lire `database/reference/audits/rcl0193_visual_mapping_audit.md`, identifier le `WEAK_CASES 1`, verifier directement son `source_section`, son fait historique et le contenu de sa/ses page(s) PDF citee(s). Mettre ensuite le rapport a jour avec la conclusion avant toute pousse du lot visuel RCL0193. `MEMSX64` reste strictement BUILD #101 `22dbe75ed14e0a61e694159d505ef72245116b48`; aucun #102.

## 2026-08-29 - RCL0193 MAPPING V3 - VALIDATION COMPLETE AVANT ASSETS

Run `33251197186`, job `99097047023`: **✅ SUCCESS complet**. Commit final d'audit sur `tmp-rave-visual-backfill`: `0ba6e25dc7390f7c5e75cc4885d1e0454041a902` (`Verify complete RCL0193 31-fact multi-page mapping V3`).

Le parseur V3 collecte toutes les occurrences `p.N` ainsi que toutes les plages `p.N-M` dans chaque `source_section`, y compris les references non contigues. Invariants passes: 31/31 faits RCL0193 uniques, tous `verifie_constructeur`, 31/31 `image_ref` historiques vides, qz64 1660 et PDF constructeur exacts verifies.

Cas durs explicitement valides: `RAVE-CAUTION-THROTTLE-STOP-001` -> pages 114 + 118; `RAVE-REP-INJECTORS-001` -> pages 135 + 170. Couverture finale: **26 pages physiques uniques**: `38,39,40,98,101,107,108,109,112,113,114,117,118,120,121,122,123,125,126,127,128,129,130,131,135,170`.

Des controles de contenu ont egalement passe sur les pages representatives, notamment Tuning Data SPi/MPi, EVAP, gestion moteur, CKP, MAP, IAT, injecteurs, IACV, HO2S, cable/pedale/boitier papillon, ECM, ECT, MAP repairs et inlet manifold p.170. Aucun offset de page n'est suppose.

Le mapping V2 reste conserve dans la chronologie comme run techniquement vert mais semantiquement incomplet. V3 est desormais la reference fonctionnelle pour RCL0193.

Prochaine pousse technique autorisee sur la branche temporaire: preparer un **candidat additif RCL0193 1760** qui rend une seule fois les 26 pages constructeur, calcule chaque SHA-256, ajoute les metadonnees/manifeste et relie les 31 faits a toutes leurs pages via `mems_rave_illustration` / `mems_rave_illustration_link`. Les faits historiques ne doivent pas etre reecrits. Le candidat doit etre applique a une reconstruction de la base #101 + 1750 et verifier integrity, user_version, comptes historiques et couverture 31/31 avant tout commit des assets.

Production reste `MEMSX64` BUILD #101 `22dbe75ed14e0a61e694159d505ef72245116b48`; aucun #102. Base/IA/RAVE restent consultatives et ne prennent jamais la main sur la communication ECU.

## 2026-08-29 - RCL0193 WEAK_CASES 1 - RESOLU PAR VERIFICATION DIRECTE DU MANUEL

Audit commite examine: `database/reference/audits/rcl0193_visual_mapping_audit.md` au commit `a1efab597b8cce35072686490721cb5731d7c499`.

Le seul cas a faible recouvrement lexical est `RAVE-REP-ECM-001`, source historique `PDF p.126-127 / ECM, 18.30.01`, topic francais `ECM ECU calculateur depose repose antidémarrage programmation`.

Verification directe du PDF constructeur RCL0193 exact:
- index PDF 126: titre `ENGINE CONTROL MODULE (ECM)`, service repair no `18.30.01`, procedure `Remove`, deconnexion batterie, desserrage/support, deconnexion des 2 multiplugs, retrait ECM;
- index PDF 127: procedure `Refit`, reconnexion des 2 multiplugs, remise en place/fixation ECM, reconnexion batterie;
- meme page 127: note constructeur indiquant que si un ECM neuf est monte, il doit etre programme avec le code de l'unite antivol (`anti-theft security unit`) avec TestBook avant que le moteur puisse demarrer.

Conclusion: **✅ mapping `RAVE-REP-ECM-001` confirme sur p.126-127**. Le `WEAK_CASES 1` etait uniquement un faux faible score lexical cause par les termes francais du topic face au texte anglais de la source; ce n'est ni une mauvaise page ni une provenance douteuse. Les **31/31 faits RCL0193 sont donc maintenant couverts par une cartographie source verifiee**, sur **25 pages physiques uniques**.

### Incident de journalisation sur la premiere tentative

La premiere tentative d'ecriture de cette conclusion, run RAPPORT `33251235882`, job `99097148820`, a correctement ajoute et verifie le texte localement mais son `git push origin RAPPORT` a ete **rejete en non-fast-forward**: pendant le run, la branche distante a avance sur `cbeaa0ed5007863ae5a4f7a9fb518d81ce49e781` (`Record complete RCL0193 mapping V3 validation`). Conformement a la regle de tracabilite, toute progression technique a ete suspendue. Aucun changement technique n'a ete pousse apres cet echec de journalisation.

Cause: concurrence de deux ecritures sur `RAPPORT`, et non erreur du contenu. Cette seconde tentative utilise `git fetch` + `git rebase origin/RAPPORT` avant le push afin de conserver les deux historiques sans ecrasement.

Prochaine action exacte apres confirmation de cette ecriture: inspecter le dernier numero de lot additif disponible et le generateur RCL0194 1750 valide; preparer un generateur RCL0193 strictement additif qui capture une seule fois les 25 pages exactes, calcule leurs SHA-256, cree les metadonnees `mems_rave_illustration` et liens `mems_rave_illustration_link` vers les 31 faits sans reecrire `mems_rave_fact`, garde `user_version=20`, et produit un audit de reproductibilite. `MEMSX64` reste BUILD #101 inchange; aucun #102.

## 2026-08-29 - RCL0193 1760 - GENERATEUR REPRODUCTIBLE POUSSE, DONNEES NON ENCORE GENEREES

Pousse technique effectuee apres validation V3: commit `4fe8ea77bae5b1651b4ef9256a2090562531184b` sur `tmp-rave-visual-backfill`, fichier unique `tools/build_rcl0193_visual_backfill_1760.py`.

Ce commit ne contient encore aucun PNG RCL0193, aucun `research_enrichment_1760.qz64`, aucune modification de manifeste ni de fait historique. Il ajoute uniquement le generateur documentaire reproductible.

Fonction prevue: reconstruire la base de reference jusqu'au lot 1750 inclus, exiger 31 faits RCL0193 et le mapping V3 complet, rendre les 26 pages constructeur exactes, calculer leurs SHA-256, creer 43 liens fait->page additifs, generer le qz64 1760, appliquer le candidat a une copie SQLite puis verifier `integrity_check`, `user_version=20`, 93 faits RAVE et 105 faits experts inchanges, couverture RCL0193 31/31 et preservation des 8 assets RCL0194.

Prochaine pousse autorisee: ajouter uniquement un workflow temporaire de validation du generateur. Ce workflow devra utiliser le PDF constructeur exact SHA-256 `c050a3eebe50c5a85bf8a69b7722bd2052079944e09d58578a498984ecf06715`, garder les sorties hors commit, verifier strictement le perimetre, produire un artefact candidat avec SHA256SUMS et ne committer aucune donnee 1760. Le resultat doit etre rapporte avant toute pousse des assets.

Production `MEMSX64` reste BUILD #101 `22dbe75ed14e0a61e694159d505ef72245116b48`. Base/IA/RAVE consultatives uniquement.

## 2026-08-29 - RCL0193 1760 - CANDIDAT DETERMINISTE VALIDE AVANT COMMIT DES DONNEES

Workflow `TEMP RCL0193 1760 VALIDATE`, run `33251336531`, job `99097421134`, declencheur `71fd91d7d8a0f3882994321f0454daccb527cfaf`: **✅ SUCCESS complet**. Aucune donnee candidate n'a ete commitee par ce run.

Validations: production `MEMSX64` toujours exactement #101 `22dbe75ed14e0a61e694159d505ef72245116b48`; PDF RCL0193 exact SHA-256 `c050a3eebe50c5a85bf8a69b7722bd2052079944e09d58578a498984ecf06715`; 26 pages rendues en PNG 1190x1682; garde de perimetre = 29 chemins generes attendus; `PRAGMA integrity_check=ok`; `user_version=20`; 93 faits RAVE et 105 faits experts historiques inchanges; 26 assets RCL0193; couverture visuelle 31/31 faits; 43 liens source fait->page; 8 assets RCL0194 du lot 1750 preserves.

Empreintes candidat: `research_enrichment_1760.qz64` SHA-256 `d3c3580d9f32d92e42db6eeb81e92a3e0be591b007ec1071d1a95baf6057ebc8`; SQL decode SHA-256 `ce6e1ba2b441f1102f78e49ea87f81dadb3db66ccbf64d63b76fd0e1974d8173`; manifeste candidat SHA-256 `59c9e212d9a656668969ac71dde1c8159f51c4ff44b0d4785d4dcce7749f23cd`; audit candidat SHA-256 `9a148d1706252b312ed2e1e524314e7ca7fe6e79be76436cf9064ecf085d32b0`.

Les 26 PNG ont chacun passe leur SHA-256 et `SHA256SUMS.txt` de l'artefact passe integralement. Artefact `RCL0193-1760-DETERMINISTIC-CANDIDATE`: ID `9714454237`, taille `5986424` octets, digest archive `sha256:c0b6bcc1d8d17f519bb4992ba9fc797fbc87d7b2e04ea9eb6efd003a235f242f`, 31 fichiers au total dont 26 PNG.

Prochaine pousse technique autorisee: commit atomique sur `tmp-rave-visual-backfill` des 26 PNG, `research_enrichment_1760.qz64`, manifeste et audit, avec suppression du workflow temporaire de validation. Le mecanisme doit regenerer le candidat depuis le PDF exact, exiger les hashes ci-dessus AVANT commit, pousser, relire le commit distant, regenerer depuis ce commit et exiger un diff nul. Rapport obligatoire apres ce test post-pousse avant toute suite.

Aucun changement de protocole/communication ECU, acquisition, RAM, write/reset, UI, Qwen/ONNX ou 32 bits. Base/IA/RAVE consultatives uniquement.

## 2026-08-29 - RCL0193 1760 - LOT VISUEL COMMITTE ET POST-VALIDE

Workflow `TEMP RCL0193 COMMIT VALIDATED 1760`, run `33251420230`, job `99097640866`: **✅ SUCCESS complet**. Commit final sur `tmp-rave-visual-backfill`: `6d92a286e4b423b2a6cc391056fc7694a1a6dbf6` (`Add deterministic RCL0193 visual backfill 1760`).

Toutes les etapes critiques ont passe: production #101 inchangee; source PDF exacte; regeneration candidat; verification des 29 empreintes attendues (qz64 + manifeste + audit + 26 PNG); garde de perimetre; commit/push; relecture exacte du commit distant; controle des octets effectivement committes; regeneration complete depuis le commit distant; `integrity_check=ok`; couverture RCL0193 31/31; 43 liens source; 8 assets RCL0194 preserves; **diff final nul**.

Le lot RCL0193 historique 1660 dispose maintenant sur la branche temporaire de 26 pages constructeur originales associees additivement a ses 31 faits. Les faits a sources multiples conservent toutes leurs pages: notamment 114+118 et 135+170. Aucune ligne historique `mems_rave_fact` n'a ete reecrite.

Empreintes principales conservees: `research_enrichment_1760.qz64` SHA-256 `d3c3580d9f32d92e42db6eeb81e92a3e0be591b007ec1071d1a95baf6057ebc8`; manifeste `59c9e212d9a656668969ac71dde1c8159f51c4ff44b0d4785d4dcce7749f23cd`; audit `9a148d1706252b312ed2e1e524314e7ca7fe6e79be76436cf9064ecf085d32b0`. Le run candidat precedent `33251336531` / artefact `9714454237` reste la preuve independante avant commit.

Etat du chantier historique apres 1750+1760: couverture visuelle complete des 55 faits RCL0194 existants et des 31 faits RCL0193 existants. Le bloc historique restant a reprendre est `AKM7169ENG` / batch 1720, 7 faits, avec sections `Introduction / publication scope`, `Engine Tuning Data 3`, `4`, `5` et `Tuning Data 3-5` a mapper sur le scan constructeur exact avant capture.

Prochaine action exacte: audit en lecture seule AKM7169ENG, verification du document/edition/hash et des index physiques des vues/tables necessaires; aucune capture ou qz64 AKM avant cartographie validee.

Production `MEMSX64` reste strictement BUILD #101 `22dbe75ed14e0a61e694159d505ef72245116b48`; aucun #102. La base/IA/RAVE reste consultative et ne prend jamais la main sur la communication ECU.

## 2026-08-29 - AKM7169ENG / 1720 - RECHERCHE SOURCE AVANT CAPTURE

Etat: les backfills visuels RCL0194/1750 et RCL0193/1760 sont termines et post-valides sur `tmp-rave-visual-backfill`. Production reste `MEMSX64` #101 `22dbe75ed14e0a61e694159d505ef72245116b48`.

Recherche en lecture seule effectuee pour le dernier bloc historique `AKM7169ENG` / batch 1720 (7 faits). Identite constructeur recoupee: Rover Group Limited, `AKM7169ENG`, Mini Repair Manual, Publication Part No. AKM7169, 1993, porte globale annoncee a partir du VIN `049349`.

Sources trouvees:
- scan indexe Scribd `Rover Mini 1992-1996 Repair Manual`, document 734770360, annonce **482 pages**; son texte indexe montre directement la couverture `AKM7169ENG`, le VIN 049349, puis les pages `ENGINE TUNING DATA 3`, `4`, `5` avec les donnees SPi;
- autre scan Scribd `Mini Repair Manual 92-96 (Autom)`, document 625575179, egalement annonce 482 pages et meme identite AKM7169ENG;
- Original Technical Publications, `Service Manual - Mini All Models 1993 on (AKM7169)`, annonce **463 pages**;
- eManual/eManualOnline propose un PDF commercial 1992-1996 de **41.87 MB**;
- plusieurs vendeurs Tradebit proposent le PDF AKM7169ENG, acces payant.

Point de securite documentaire: la divergence 482 pages / 463 pages montre qu'un numero d'index physique ne peut pas etre transpose d'un scan a un autre. Aucun index de page ne sera devine. Aucun asset AKM ne doit etre cree avant possession d'un scan exact, telechargeable/reproductible, avec SHA-256 et verification de ses pages physiques.

Le texte indexe du scan confirme toutefois sans ambiguite la sequence utile du lot 1720: `Engine Tuning Data 3` = SPi boite manuelle avec `MNE 101040`; `Engine Tuning Data 4` = SPi automatique avec `MNE 101060`; `Engine Tuning Data 5` = SPi high compression avec `MNE 101070`. Il confirme aussi speed/density, pression carburant 1.0 bar +/-4.0% constante et TPS 0-1 V ferme / 4-5 V ouvert. Ces elements correspondent aux 7 faits deja integres et ne doivent pas etre dupliques comme nouveaux faits simplement parce qu'ils ont ete retrouves sur le Web.

Recherche dans la File Library utilisateur: aucun PDF/scan complet AKM7169ENG retrouve. Les fichiers trouves sont des travaux MEMS derives, pas la source constructeur complete.

Prochaine pousse autorisee sur `tmp-rave-visual-backfill`: ajouter uniquement un audit textuel AKM7169 de provenance/etat-source et un backlog des informations complementaires visibles dans le scan indexe mais absentes de la base, sans qz64 ni image tant que le scan exact n'est pas acquis. Continuer en parallele la recherche d'une source constructeur directement reproductible. Toute future source doit etre hashee avant capture.

Base/IA/RAVE restent consultatives et ne prennent jamais la main sur la communication ECU.

## 2026-08-29 - AKM7169 SOURCE AUDIT - POUSSE ET RELECTURE DISTANTE VALIDEES

Pousse documentaire sur `tmp-rave-visual-backfill`: commit `a8e31f5831bf2831468a1dfffffdf3d4c9390661`, ajout unique `database/reference/audits/akm7169_1720_source_backfill_audit.md`.

Test apres pousse: relecture directe du fichier depuis la branche distante **✅ PASS**. Blob SHA GitHub `9bb417f81ad289ddaf793aa093d1727c2dc1b4ef`. Le fichier confirme l'identite constructeur AKM7169ENG, les sources publiques/commerciales trouvees, la divergence de pagination 482/463, l'absence de source binaire reproductible selectionnee et l'interdiction de fabriquer un index physique ou un asset tant que le PDF exact n'est pas hashe.

Aucun qz64, PNG, manifeste, fait historique, code programme ou protocole ECU n'a ete modifie par cette pousse.

Prochaine pousse autorisee: conserver dans un fichier backlog separe les informations techniques complementaires visibles dans le texte indexe AKM7169 mais non encore integrees, avec statut explicite `OCR/indexed-scan candidate - primary page verification required`. Ce backlog ne doit pas modifier la base. Apres cette pousse: relecture + rapport avant toute autre action.

## 2026-08-29 - AKM7169 RESEARCH BACKLOG - POUSSE ET RELECTURE VALIDEES

Pousse sur `tmp-rave-visual-backfill`: commit `a4c1dea0b4bae048a1deabd676de54ac12dd978a`, ajout `database/reference/research_backlog/akm7169_indexed_scan_candidates.md`.

Relecture directe depuis GitHub: **✅ PASS**, blob SHA `ed9419da5810647b9efab77a52f7cf6ea348a443`. Le fichier porte explicitement le statut global `OCR_INDEXED_SCAN_CANDIDATE — PRIMARY PAGE VERIFICATION REQUIRED` et separe les 7 faits 1720 deja presents des informations supplementaires uniquement candidates.

Informations preservees pour verification future: references moteur/application, ralenti/CO, allumage/capteurs/coil/plugs, unite injecteur-regulateur, carburant, General Data pompe/pression, outillage Microcheck/Cobest, vue Underbonnet SPi et plusieurs couples de serrage. Les OCR ambigus sont explicitement marques et ne sont pas devines. Aucun de ces candidats n'est ajoute a la base comme fait constructeur a ce stade.

Cette pousse realise la regle fondamentale de conservation des recherches Internet: une information utile trouvee mais non encore verifiable n'est plus perdue; elle reste dans un backlog trace avec sa provenance et son niveau de confiance.

Aucun qz64, PNG, manifeste, fait historique ni code ECU n'a ete modifie. Prochaine action: continuer la recherche d'un binaire AKM7169ENG exact/reproductible et, en parallele, verifier si une source anterieure du lot 1720 peut etre retrouvee dans l'historique du depot ou les artefacts de recherche. Rapport avant toute nouvelle pousse.
