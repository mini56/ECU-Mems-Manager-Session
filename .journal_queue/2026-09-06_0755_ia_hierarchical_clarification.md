## IA MEMS — clarification documentaire hiérarchique après test réel FREIN

État validé par la discussion utilisateur : le build correctif `8aa57b4b3afac8771ed9146357eeb88f57bcc677` a amélioré les libellés mais a supprimé le niveau parent qui représentait tout le chapitre FREINS. Le test réel a montré qu'en choisissant `1`, l'utilisateur doit pouvoir descendre dans les sous-rubriques au lieu de recevoir immédiatement tout le contenu ou un sous-sujet arbitraire.

Décision explicite : ne pas empiler sur `8aa57b...`. La branche active `tmp-ia-clarification-test-1b106ee` a été replacée exactement sur le précédent état `acca4fc1a1b158f5595bba588675be25050cecd9`, puis la correction hiérarchique a été appliquée en un seul commit actif :

- `2551880a73e8a167ed1e2285d9f8d05da32ef4a2` — `Add hierarchical documentary clarification from acca baseline`
- comparaison `acca4fc1... -> 2551880...` : `ahead_by=1`, `behind_by=0`, un seul fichier modifié : `database/MemsSearchCompletenessPatch.cpp`.

Comportement visé par ce commit :

1. Une recherche large `FREIN` conserve un premier choix parent `Freins — documentation générale` au lieu de perdre le chapitre général.
2. Si l'utilisateur choisit ce parent, aucune réponse finale n'est encore envoyée : un second menu est construit à partir des rubriques FREINS connues, notamment purge du circuit, contacteur de feux stop, tambour arrière, témoin de défaillance, soupape de tarage, maître-cylindre, pédale, plaquettes avant, segments arrière, servocommande, étrier avant, cylindre arrière et frein à main/câble de frein à main.
3. Si un choix du second niveau mène encore à au moins deux sujets distincts, un nouveau menu numéroté est proposé. La clarification peut donc continuer en cascade autant que nécessaire.
4. La réponse documentaire normale n'est appelée que lorsqu'il ne reste plus au moins deux choix distincts.
5. Le transcript conserve la saisie réelle de l'utilisateur (`1`, `2`, etc.) au lieu d'afficher à sa place une requête interne cachée.

La branche a été force-reset sur `acca4fc1...` avant le commit final afin que l'état actif ne contienne pas les essais transitoires effectués pendant la préparation. L'historique actif utile est donc bien `acca4fc1...` puis `2551880...`.

Build déclenché automatiquement par le push final : run GitHub Actions `34020372901`, workflow `TMP IA Clarification Test from BUILD105`, HEAD `2551880a73e8a167ed1e2285d9f8d05da32ef4a2`. Au moment de ce journal, le run est `pending` et aucun verdict réel PC n'est encore donné.
