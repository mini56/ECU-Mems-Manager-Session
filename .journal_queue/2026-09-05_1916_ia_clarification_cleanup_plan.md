## 2026-09-05 — PLAN AVANT POUSSE — NETTOYAGE CLARIFICATION IA APRÈS TEST PC

Autorisation utilisateur : `GO` après tests PC réels du build temporaire de clarification.

Base de travail stricte : branche `tmp-ia-clarification-test-1b106ee`, build de test vert issu de BUILD #105 `1b106eed05e1fd665b857f73a719f02ee6b6b2ac`.

Constats PC confirmés :
- `FREIN` et `embrayage` déclenchent correctement une clarification : mécanisme validé ;
- les choix restent parfois incompréhensibles (`embrayage — PDF p.178`) ou non discriminants (`embrayage — clutch`) ;
- après sélection, la recherche reste trop large et déverse encore plusieurs faits/pages ;
- des consignes internes et métadonnées de grounding sont affichées à l’utilisateur (`Documentation RAVEMEMS retrouvée...`, `DOC_...`, `REV_...`, `type step`, pages/provenance), ce qui rend les réponses indigestes.

Périmètre strict autorisé :
1. conserver le déclenchement de clarification déjà validé ;
2. fabriquer des libellés de choix à partir de vrais sujets/sections/opérations compréhensibles, jamais à partir d’un simple numéro de page ;
3. rejeter les faux choix qui ne font que répéter/traduire le terme principal ;
4. utiliser réellement le choix retenu comme filtre de la recherche suivante ;
5. masquer dans la réponse normale les instructions internes RAVEMEMS/MEMSLibrary et les métadonnées de provenance ;
6. conserver ces données en interne pour la vérification et ne les afficher que si l’utilisateur demande explicitement source/page/procédure ;
7. conserver le chemin rapide de BUILD #105 : aucun passage systématique de toute la documentation dans Qwen.

Interdictions : aucun changement Qwen/ONNX, ECU/protocole/acquisition/RAM/écriture/reset, RAVEMEMS/Pack001, visuels, scroll ou autre UI non concernée.

Tests PC attendus après build : `FREIN`, sélection d’un couple précis, `embrayage`, sélection d’un vrai sous-thème, puis une question documentaire précise pour vérifier qu’elle répond directement sans clarification inutile.

PROCHAINE ACTION EXACTE : appliquer uniquement ces corrections sur la branche temporaire existante, relancer le build isolé, puis soumettre l’artefact au test PC utilisateur.
