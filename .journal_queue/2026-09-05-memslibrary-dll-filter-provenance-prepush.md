## 2026-09-05 — MEMSLibrary.dll — CORRECTION FILTRAGE / PROVENANCE — AVANT POUSSE

### CONTEXTE ET AUTORISATION

L audit forensique en lecture seule de la chaine `RAVE -> RAVEMEMS V2 -> Pack001 -> MEMSLibrary.dll -> IA MEMS Manager` a confirme que le contenu constructeur correct existe bien en amont, mais que l interface publique de recherche MEMSLibrary ne permet pas de garantir l isolation documentaire et linguistique requise. L utilisateur autorise maintenant explicitement : **corriger la DLL**.

Incident reel a corriger : pour la question `Quel est le jeu axial du pignon primaire et comment le contrôler ?`, le BUILD #104 a pu recevoir comme preuve `DOC_RCL0193ENG` pages 342-343, contenu antenne/coax sans rapport, alors que RAVEMEMS V2 contient l operation correcte `12.21.28` `ENGINE CRANKSHAFT PRIMARY GEAR END-FLOAT`, page physique 53, valeur `0.089 to 0.165 mm`, controle avec cales d epaisseur entre backing ring et primary gear, visuels `VIS_P0053_001` et `VIS_P0053_002`.

### CAUSE RACINE ETABLIE AVANT MODIFICATION

- RAVEMEMS V2 conserve les champs de provenance, notamment `source_language`, `revision_key`, `page_key` et les liens visuels exacts.
- Pack001 conserve `source_language` dans sa table de recherche.
- L ABI2 publique `MEMSLibrary_SearchPack(...)` ne fournit aucun parametre d entree permettant de contraindre strictement la langue, le document, la publication ou la revision.
- La recherche ABI2 repose sur un `search_text LIKE ?` large, sans filtres stricts de provenance.
- Le resultat ABI2 ne restitue pas `source_language` ni `revision_key`.
- Le bridge du BUILD #104 ne peut donc pas imposer une isolation de langue/document/revision avec le contrat DLL actuel.

**Point de rupture confirme : `Pack001 -> MEMSLibrary_SearchPack() / ABI2`.** Le probleme n est pas l ancien incident de runtime VC++/0xC0000142, deja corrige et clos.

### PERIMETRE STRICT DE LA CORRECTION AUTORISEE

Modifier uniquement les sources, tests et workflow temporaire necessaires a **MEMSLibrary.dll** sur une branche temporaire issue de la base Pack001 validee. Ne pas modifier :
- `MEMSX64` ni BUILD #104 ;
- `IaMemsLibraryBridge.cpp` ou toute autre partie de l application ;
- RAVE, RAVEMEMS V2 ou le contenu de Pack001 ;
- protocole ECU, acquisition, RAM, write/reset, UI, Qwen/ONNX ou 32 bits.

La correction doit etre propre, generale et retrocompatible : ne pas casser l ABI2 existante et ne pas masquer le defaut par un classement ad hoc sur la seule question du pignon primaire.

### OBJECTIF TECHNIQUE AVANT POUSSE

1. Relire la source exacte MEMSLibrary du Pack001 valide (`tmp-memslibrary-pack001`, commit historique valide `9cf3d00840cec1749ef9358f708d3e68a2c8bc4d`).
2. Conserver integralement l export ABI2 existant pour compatibilite binaire.
3. Ajouter une capacite de recherche filtree/provenance retrocompatible, en utilisant uniquement les colonnes reellement presentes dans le schema Pack001 apres verification du code : filtres stricts optionnels sur les metadonnees disponibles (langue, document/publication, revision) et retour des champs de provenance correspondants.
4. Utiliser des parametres SQL lies et un ordre deterministe ; aucun SQL construit a partir d entree utilisateur.
5. Ne modifier aucun contenu documentaire du Pack.

### VALIDATION OBLIGATOIRE AVANT TOUTE INTEGRATION APPLICATION

Le workflow GitHub Actions temporaire devra prouver au minimum :
- compilation Windows x64 de `MEMSLibrary.dll` ;
- ABI2 toujours exportee et smoke tests historiques toujours verts ;
- nouvelle recherche filtree capable d exclure un document/langue/revision non demandes lorsque les metadonnees correspondantes existent ;
- restitution de la provenance par la nouvelle interface ;
- cas de regression `primary gear end float` : preuve correcte `DOC_RCL0193ENG` page 53 / operation `12.21.28`, et absence de selection de la page 342 antenne/coax dans le cas contraint ;
- `PRAGMA integrity_check=ok`, aucune FK cassee, Pack001 intact ;
- diff limite au perimetre DLL/tests/workflow temporaire.

### INCIDENT DE JOURNALISATION ET RETABLISSEMENT

La tentative precedente de mise a jour directe du rapport maitre a ete suspendue car le connecteur Contents ne renvoyait pas de facon exploitable le contenu complet du fichier de plus de 350 Ko pour une reecriture sure. Aucune progression technique n a ete faite pendant cet echec. Le depot contient toutefois le mecanisme canonique `report-master-journal.yml` + `tools/append_master_report.py`, qui ajoute des entrees depuis `.journal_queue/` sans reecrire manuellement l historique et verifie le blob distant. Ce mecanisme est desormais utilise pour retablir la tracabilite avant toute pousse DLL.

### PROCHAINE ACTION EXACTE

Attendre et verifier le SUCCESS du journal maitre declenche par cette entree, verifier que la queue est vide et que l entree est effectivement presente dans `RAPPORT_SUIVI_ECU_MEMS_MANAGER.md`. **Seulement apres cette preuve**, inspecter les sources exactes MEMSLibrary du commit Pack001 valide, creer une branche temporaire dediee et appliquer la correction DLL minimale et retrocompatible. Aucun changement `MEMSX64`/BUILD #104.
