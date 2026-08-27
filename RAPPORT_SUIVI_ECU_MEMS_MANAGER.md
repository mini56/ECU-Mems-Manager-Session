# RAPPORT DE CONTINUITÉ — ECU MEMS MANAGER

> **RÈGLE OBLIGATOIRE** : relire ce fichier avant toute modification. Branche rapport : `RAPPORT`. Branche x64 active : `MEMSX64`.
>
> **RÈGLE QUALITÉ — AUCUNE RUSTINE** : rechercher la cause réelle et produire une solution propre, générale et maintenable. Ne pas supprimer une capacité juste pour faire passer un test.
>
> **SUIVI IMMÉDIAT** : avant chaque nouvelle étape, inscrire ici l’étape et son objectif ; après chaque résultat, l’inscrire avant la suite.
>
> **NOMMAGE UTILISATEUR** : `ECU MEMS Manager x64 #NN — Commit xxxxxxx`. `#NN` = GitHub Actions, pas le BUILD logiciel.

## ÉTAT COURANT — 27 AOÛT 2026

- Dépôt : `mini56/ECU-Mems-Manager-Session`.
- Branche active : **`MEMSX64`**.
- HEAD x64 courant : **`16b99c3f40985b7ef5439ee8834eb5e8dd8126bf`**.
- BUILD logiciel : **#30 / v1.0.30**. Aucun BUILD #31 sans demande explicite.
- Dernier run entièrement validé : **#90 — SUCCESS**, run `33044945315`, commit `fab2e4cf`.
- Run **#91 — FAILURE**, run `33047008070`, commit `897b51c8` : échec uniquement à `Validate packaged production LocalAiClient with Qwen` après assemblage ; moteur ONNX et réponses déterministes prêts, puis réponse générative rejetée par le contrôle de langue active.
- Run **#92 — EN COURS**, run `33058810115`, commit `16b99c3f` : validation de la correction de déterminisme ONNX.
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
Identifier exactement le prompt génératif, les paramètres de génération et le contrôle `réponse exploitable dans la langue active`. Corriger la cause générale qui peut rejeter de manière non déterministe une génération valide, **sans supprimer ni affaiblir le garde de langue de production** et sans masquer l'échec par une simple boucle de relance. Valider ensuite le self-test natif ET packagé sur GitHub Actions en restant BUILD #30 / v1.0.30.

Aucun changement protocole, ECU, UI, 32 bits, Qwen/ONNX de version ou BUILD logiciel n'est autorisé dans cette correction.

## NOUVELLE ACTION AUTORISÉE — SCHÉMAS PROPOSÉS PAR IA MEMS

### Objectif avant toute modification
Ajouter à IA MEMS une capacité **séparée du moteur IA** : lorsqu’une question correspond à un schéma local connu (ex. brochage MEMS 1.3, connecteur MEMS 1.9, ROSCO 3 broches), l’onglet IA peut proposer un bouton explicite **« Ouvrir le schéma … »** pour visualiser le fichier local correspondant.

### Contraintes obligatoires
- **NE PAS MODIFIER** Qwen, ONNX, `LocalAiClient`, le prompt, les budgets de tokens ou la génération qui fonctionne, hors correction ciblée de l'incident #91 ci-dessus nécessaire pour rétablir le HEAD vert.
- La détection de schéma doit être déterministe et locale, indépendante du LLM.
- Réutiliser le mécanisme d’affichage de schéma existant s’il existe ; sinon créer le plus petit composant de visualisation local possible.
- Aucun accès réseau nécessaire pour ouvrir un schéma.
- Ne jamais ouvrir automatiquement : seulement proposer, puis ouvrir sur clic utilisateur.
- Ne jamais inventer de schéma ; n’utiliser que des fichiers réellement présents dans le package / manifeste.
- Préparer sur branche temporaire, ajouter un self-test spécifique, comparer le diff, puis seulement envisager `MEMSX64` après validation.
- Aucun changement protocole, ECU, RAVE, 32 bits ou BUILD logiciel.

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

Correction ciblée retenue : **conserver `do_sample=true`, les températures, `top_p`, `top_k`, le prompt, les budgets de tokens et tous les gardes de production, mais fixer `random_seed=42` dans les paramètres ONNX GenAI**. Cela rend une même requête reproductible sans transformer la génération en greedy, sans affaiblir le contrôle de langue et sans ajouter de boucle de relance. Modification limitée à `expert/LocalAiClient.cpp`, puis contrôle du diff et push sur `MEMSX64` en restant BUILD #30 / v1.0.30. Le run complet devra valider le self-test natif et le self-test packagé avant toute autre fonction.

### RÉSULTAT DE LA CORRECTION

- Commit `MEMSX64` : **`16b99c3f40985b7ef5439ee8834eb5e8dd8126bf`** (`BUILD #30 make ONNX sampling reproducible`).
- Diff contrôlé après push : **exactement une ligne ajoutée** dans `expert/LocalAiClient.cpp`, `OgaGeneratorParamsSetSearchNumber(params, "random_seed", 42.0)` ; aucun autre fichier ni aucune autre ligne de code modifiée.
- Le garde `likelyWrongLanguage()`, `containsInternalInstructionLeak()`, le prompt, `do_sample=true`, `temperature`, `top_p`, `top_k`, les budgets de tokens et les fallbacks restent inchangés.
- GitHub Actions **#92**, run `33058810115`, a démarré sur ce commit et est actuellement en cours. Ne reprendre aucune autre modification avant son verdict, en particulier les étapes natives et packagées LocalAiClient/Qwen.

## PROCHAINE ACTION EXACTE

**Attendre le verdict complet de GitHub Actions #92 sur `16b99c3f`. Si #92 est vert, consigner les étapes LocalAiClient native + packagée et reprendre ensuite l'audit `IaMemsTab` / `IaMemsService` pour la fonction de schémas. Si #92 est rouge, analyser précisément l'étape fautive et son journal avant toute nouvelle modification. Rester BUILD #30 / v1.0.30.**