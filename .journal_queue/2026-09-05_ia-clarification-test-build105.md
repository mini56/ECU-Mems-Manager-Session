## 2026-09-05 — TEST PC À PRÉPARER — CLARIFICATION DOCUMENTAIRE DEPUIS `1b106ee`

### Constat utilisateur réel avant modification
L'utilisateur a retesté plusieurs états applicatifs sur son PC.

- `a55427affeec84643f916621df6247adf29e80fb` fonctionne sur PC pour l'IA locale, avec réponse rapide, mais les réponses documentaires ne sont pas suffisamment triées.
- Exemple réel dans IA MEMS : requête simple `frein` -> plusieurs gros blocs documentaires sont renvoyés directement au lieu de proposer un périmètre.
- Dans le même état, l'onglet **Base de données** recherche `frein`, affiche une liste de résultats et laisse l'utilisateur choisir la réponse pertinente.
- `1b106eed05e1fd665b857f73a719f02ee6b6b2ac` (#105) a été retesté de la même façon : l'onglet Base de données conserve le choix utilisateur et l'IA MEMS conserve le même défaut de tri sur `frein`.

Décision utilisateur : travailler pour ce test à partir de `1b106eed05e1fd665b857f73a719f02ee6b6b2ac`, car cet état reste rapide et ne présente pas la régression lente/incohérente observée ensuite en #108.

### Vérification des deux chemins de recherche dans `1b106ee`
Le code confirme que les deux onglets ne passent pas par le même moteur de recherche.

**Onglet Base de données** :
- base runtime : `ecu_mems_reference_r5.sqlite`, reconstruite depuis `database/reference` et les lots `research_enrichment*.qz64` ;
- index global : `mems_global_search_r1.sqlite` ;
- moteur : `MemsGlobalSearchIndex` / FTS5 quand disponible ;
- l'UI affiche les résultats et laisse l'utilisateur sélectionner la ligne voulue.

**IA MEMS** :
- base experte historique/additive : `database/expert/ia_mems_reference_r20.sqlite` via `ExpertKnowledgeReader` ;
- source supplémentaire #105 : `MEMSLibrary.dll` + `MEMSLibrary_Pack_001/knowledge.sqlite` via `IaMemsLibraryBridge` ;
- `askWithLibrary()` fusionne le grounding legacy r20 et le grounding MEMSLibrary ;
- dans `LocalAiClient`, si la question n'exige pas de génération diagnostique et que le grounding n'est pas vide, le grounding est renvoyé directement : Qwen n'est pas appelé pour sélectionner ou clarifier.

Conséquence : sur une requête documentaire trop générale comme `frein`, l'IA peut afficher directement plusieurs faits/procédures alors que l'onglet Base de données permet à l'utilisateur de choisir le périmètre.

### Fonction de clarification déjà existante
`iamemstab.cpp` / `IaMemsConversationRouting.h` contiennent déjà une logique de clarification conversationnelle pour certains cas :
- Mini SPi / MPi ;
- génération MEMS 1.2 / 1.3 / 1.6 / 1.9 ;
- reprise de la question initiale après la réponse de l'utilisateur.

Le mécanisme de conversation permettant de poser une question puis reprendre la demande existe donc déjà. Le cas manquant est la **clarification documentaire générale lorsque beaucoup de résultats couvrent plusieurs périmètres**.

### Comportement cible du test
Exemple : utilisateur `frein`.

Au lieu de déverser plusieurs procédures, IA MEMS doit proposer des périmètres issus des résultats réellement trouvés, par exemple selon les données disponibles :
1. freins avant ;
2. freins arrière ;
3. frein à main ;
4. circuit hydraulique / purge ;
5. maître-cylindre / servocommande.

L'utilisateur peut répondre par un numéro ou par le périmètre. La question originale est alors affinée et le chemin #105 normal reprend.

Une question déjà précise, par exemple `connecteur sonde CKP`, ne doit pas déclencher inutilement cette clarification générale.

### Test technique isolé lancé
Branche créée exactement depuis `1b106eed05e1fd665b857f73a719f02ee6b6b2ac` :
`tmp-ia-clarification-test-1b106ee`

Modification technique de test :
- fichier modifié : `database/MemsSearchCompletenessPatch.cpp` ;
- commit : `97e63f261fde00b49346be9f80cd8c304b5a4b3d` ;
- le test réutilise `MemsGlobalSearchIndex` pour détecter une recherche documentaire très large ;
- uniquement pour une requête à un terme non spécifique et avec au moins 10 résultats, il construit jusqu'à 5 choix de périmètre à partir des résultats existants ;
- la réponse utilisateur (numéro ou texte) affine ensuite la question avant de réutiliser le `sendQuestion()` existant ;
- les requêtes courantes ou précises continuent par le chemin #105 normal.

Aucun changement du modèle Qwen, ONNX Runtime GenAI, paramètres de génération, corpus RAVE/RAVEMEMS, contenu SQLite, Pack001, MEMSLibrary.dll, catalogue visuel, protocole ECU, acquisition, RAM, écriture ou reset.

Workflow temporaire ajouté :
`.github/workflows/tmp-ia-clarification-build.yml`

Commit de lancement :
`8149a9e631669f76244a378fa6e55dcf35e34cf9`

Workflow :
`TMP IA Clarification Test from BUILD105`

Run :
`33979077418`

État au moment de cette journalisation : **IN_PROGRESS**.
Aucune conclusion CI n'est encore déclarée et aucun verdict PC n'est possible avant récupération de l'artefact et test réel utilisateur.

### Point de méthode
Cette entrée est préparée immédiatement à la demande explicite de l'utilisateur pendant que le run est encore en cours. Les deux commits techniques de la branche temporaire avaient déjà été poussés avant cette entrée ; cela ne doit pas être présenté comme une journalisation pré-pousse conforme. Le présent journal fixe l'état exact avant toute action technique supplémentaire.

### PROCHAINE ACTION EXACTE
1. Ne rien modifier d'autre pendant ce test.
2. À la fin du run `33979077418`, journaliser son résultat réel et l'artefact s'il est produit.
3. Faire tester sur le PC utilisateur au minimum : `frein`, réponse par numéro à la clarification, puis une question précise comme `connecteur sonde CKP`.
4. Vérifier que la vitesse et les réponses rapides de `1b106ee` sont conservées.
5. Ne promouvoir aucune correction et ne modifier aucun autre sous-système avant le verdict PC utilisateur.
