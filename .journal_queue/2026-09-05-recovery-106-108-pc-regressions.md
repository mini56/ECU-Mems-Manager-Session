## 2026-09-05 — RATTRAPAGE DE TRACABILITE — BUILDS #106/#108, TESTS PC ET ROLLBACK

### Constat de méthode
La règle obligatoire `RAPPORT AVANT POUSSE -> POUSSE -> TEST REEL -> RAPPORT IMMEDIAT` n'a pas été respectée pendant les essais #106/#108. Cette rupture de journalisation est désormais traitée comme incident de méthode prioritaire. Aucun nouveau changement technique ne doit être entrepris tant que le présent rattrapage n'est pas écrit et vérifié dans le rapport actif.

### BUILD #106 — test réel utilisateur
Branche production : `MEMSX64`.
Commit #106 : `ed9770a03bd67ec0a8326193003ebcb925bbf9ef`.
Run GitHub Actions : `33968278857` — SUCCESS côté CI.

Objectif visé : faire formuler par Qwen une vraie réponse documentaire au lieu de déverser le grounding brut, tout en gardant la recherche MEMSLibrary validée.

Test PC réel 1 : `Quel est le jeu axial du pignon primaire ?`
Résultat observé :
- bonne source `DOC_RCL0193ENG` page 53 ;
- bonne valeur `0.089 to 0.165 mm` ;
- bonne opération `12.21.28` ;
- MAIS réponse affichée sous forme de grounding brut en anglais avec métadonnées `DOC_...`, `REV_...`, `type step` ;
- visuel affiché non pertinent : pictogramme/en-tête ENGINE au lieu du schéma mécanique utile.
Verdict : ❌ BUILD #106 non validé sur PC.

Test PC réel 2 : `Comment contrôler le jeu axial du pignon primaire ?`
Résultat observé :
- bonne page/source/méthode ;
- grounding brut toujours affiché ;
- aucune vraie synthèse utilisateur en français.
Verdict : ❌ défaut confirmé de construction finale de réponse.

Test PC réel 3 : `Explique-moi la procédure 12.21.28.`
Résultat observé :
- la procédure exacte n'est pas retrouvée correctement ;
- réponse hors sujet issue de l'ancien moteur : joint de collecteurs SPi, alternateur, phares additionnels Cooper ;
- preuve que la référence de procédure n'est pas traitée comme identifiant exact prioritaire.
Verdict : ❌ défaut distinct de routage/recherche par référence.

Décision utilisateur : #106 rejeté ; retour exact au BUILD #105.
Référence restaurée : `MEMSX64` commit `1b106eed05e1fd665b857f73a719f02ee6b6b2ac`.

### BUILD #108 — tentative générale puis régression aggravée
Commit testé : `92ddaf6f1807a4b475b94c0e9dafbc9ec04662bb`.
Run GitHub Actions : `33970147790`.
Premier essai rouge uniquement sur téléchargement Qt/certificat miroir ; même run relancé sans changement de source ; seconde tentative SUCCESS côté CI.

Objectifs visés :
- reconnaissance générique des références de procédure du type `xx.xx.xx` ;
- construction réelle de réponse ;
- réponse dans la langue active ;
- aucun hardcode `12.21.28`.

Test PC réel : `Quel est le jeu axial du pignon primaire ?`
Résultat observé :
- grounding brut RAVEMEMS toujours affiché ;
- puis message `Moteur conversationnel local indisponible : La documentation a été trouvée, mais le modèle local n'a pas produit de réponse exploitable dans la langue active.`
Verdict : ❌ #108 n'a pas résolu le problème de réponse.

Test PC réel : `FREIN`
Résultat observé après environ 3 minutes :
`[1 and 2,3,4,5, and 5,6,7, and 8,9,10,11,1,2,3,1, but to be, but, and, but, to be, and, and, ...]`
Comportement : latence extrême + génération incohérente/bouclée.
Verdict utilisateur : `108 na rien resolu juste degrader les reponce`.
Verdict final : ❌ BUILD #108 rejeté comme régression fonctionnelle de l'IA conversationnelle.

Décision : retour exact à BUILD #105, commit `1b106eed05e1fd665b857f73a719f02ee6b6b2ac`.

### Cause de méthode reconnue
L'assistant a avancé sans maintenir correctement le rapport actif et a donc perdu la continuité obligatoire. Le présent rattrapage n'efface pas l'incident : il le consigne explicitement.

### Etat actuel à préserver
- `MEMSX64` doit rester sur BUILD #105 `1b106eed05e1fd665b857f73a719f02ee6b6b2ac` tant qu'un meilleur baseline réel n'a pas été identifié.
- #106 et #108 sont rejetés pour validation PC.
- Aucun nouveau build n'est autorisé avant audit historique réel du dernier comportement IA satisfaisant.
- Ne pas modifier RAVEMEMS, MEMSLibrary, protocole ECU, UI, images, Qwen/ONNX ou routage avant cet audit.

### PROCHAINE ACTION EXACTE
1. Vérifier que cette entrée a bien été ajoutée et validée par le writer du rapport actif n°2.
2. Remonter les tests réels utilisateur documentés dans les rapports/builds antérieurs pour identifier le dernier build où l'IA conversationnelle complète était réellement satisfaisante sur PC : date, réponses courtes, clarification/ambiguïté, continuité de conversation.
3. Distinguer strictement `CI vert` de `validé sur PC`.
4. Produire une chronologie factuelle des builds candidats (#92 à #105 au minimum) avant toute modification technique.
5. Aucun nouveau build avant validation explicite de ce baseline par l'utilisateur.
