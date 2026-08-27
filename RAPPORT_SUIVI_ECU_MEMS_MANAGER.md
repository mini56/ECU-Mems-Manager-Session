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
- HEAD x64 courant : **`f2e97b3e3a432785e159d30bbeca7b7bef2fdcb4`**.
- Dernier run entièrement validé : **#95 = v1.0.95 — SUCCESS**, run `33076089248`, commit `f2e97b3e`.
- Artefact #95 réel vérifié via l’API GitHub du run `33076089248` : ID **`9648135346`**, nom **`ECU-MEMS-Manager-x64-BUILD-95-v1.0.95`**, taille **386 785 918 octets**, SHA-256 **`6491cd545d4770476f13ce31929aa665fabe15a6068cd28e5c2619cc1db444af`**. Le ZIP téléchargé a été contrôlé localement avec le même SHA-256.
- #95 intègre le lot RAVE 1720 Classic SPi / AKM7169 sans changement de code : génération base expert r20, IA native, IA packagée Qwen, package complet et smoke launch tous SUCCESS.
- Inspection directe de la SQLite réellement livrée dans #95 : **93 faits RAVE / 105 faits experts**, `PRAGMA integrity_check = ok`, `user_version = 20`.
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
- À partir de maintenant : #95 = v1.0.95, #96 = v1.0.96, etc., via `github.run_number`.
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

## PROCHAINE ACTION EXACTE — À EFFECTUER DANS UNE NOUVELLE DISCUSSION

**Ne plus modifier la base dans cette discussion.** Ouvrir une nouvelle discussion afin d’éviter une coupure pendant la migration.

Ordre obligatoire dans la nouvelle discussion :
1. lire `RAPPORT_SUIVI_ECU_MEMS_MANAGER.md` sur `RAPPORT` ;
2. lire immédiatement `RAPPORT_SCHEMA_SOCLE_RAVE_EXACT.md` sur `RAPPORT` ;
3. vérifier que `MEMSX64` est toujours #95 `f2e97b3e3a432785e159d30bbeca7b7bef2fdcb4` ;
4. reprendre exclusivement `tmp-rave-knowledge-foundation` ;
5. construire `research_enrichment_1730.qz64` avec les **11 tables + index uniquement**, sans suppression ni modification des tables historiques ;
6. reconstruire localement la base r20 et vérifier au minimum 93 RAVE / 105 experts / `integrity_check=ok` / `user_version=20` ;
7. ajouter le self-test de fondation ;
8. seulement ensuite migrer les 93 faits et leurs portées prouvées ;
9. ne pas avancer `MEMSX64` et ne pas lancer #96 avant validation complète du candidat temporaire.

AKM6799 reste en recherche lecture seule jusqu’à validation de ce socle. Les anciens candidats Japon RCL0194 restent séparés et en attente.
