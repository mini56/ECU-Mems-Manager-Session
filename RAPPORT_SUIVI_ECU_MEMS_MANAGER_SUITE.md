# ECU MEMS MANAGER — RAPPORT DE SUIVI — SUITE

## STATUT DE CE FICHIER

Ce fichier est le **rapport actif officiel n°2** du projet ECU MEMS Manager a compter du **5 septembre 2026**.

Le fichier historique `RAPPORT_SUIVI_ECU_MEMS_MANAGER.md` reste conserve integralement sur la branche `RAPPORT`. Il constitue le **rapport maitre n°1 / historique complet** et ne doit pas etre supprime, tronque ou reconstruit.

A partir de cette bascule, les nouvelles entrees de suivi doivent etre ajoutees dans **ce fichier uniquement** via le mecanisme `.journal_queue` + `tools/append_master_report.py`.

Regle de reprise :
1. lire d'abord ce fichier ;
2. consulter l'ancien rapport uniquement si un detail historique non repris ici est necessaire ;
3. la derniere section `PROCHAINE ACTION EXACTE` de ce fichier fait foi.

---

## 1. REGLES DE TRAVAIL TOUJOURS OBLIGATOIRES

- Faire uniquement ce qui est explicitement demande par l'utilisateur.
- Ne pas modifier un sous-systeme non concerne.
- Ne pas introduire de regression volontaire ou de reconstruction a partir d'un etat ancien.
- Compilation utilisateur : GitHub Actions uniquement.
- RAVE / base documentaire / IA : **consultatif uniquement** ; aucun controle de communication ECU, protocole, acquisition, RAM, ecriture ou reset.
- Corriger les causes racines, pas empiler des rustines.
- Avant une pousse technique : journaliser l'action prevue.
- Apres chaque test/run : journaliser immediatement le resultat reel.
- Si la journalisation echoue, arreter la progression technique jusqu'a restauration de la tracabilite.
- Aucun nouveau BUILD de production sans validation prealable de la branche temporaire concernee.

Workflow de reference :
**RAPPORT AVANT POUSSE -> POUSSE -> TEST -> RAPPORT IMMEDIAT -> correction -> RAPPORT AVANT POUSSE suivante.**

---

## 2. ETAT PRODUCTION AU MOMENT DE LA BASCULE

Production courante : **BUILD #104**

- commit : `a55427affeec84643f916621df6247adf29e80fb`
- run GitHub Actions : `33921162926`
- job : `101179566087`
- statut CI : **SUCCESS**
- artefact : `ECU-MEMS-Manager-x64-BUILD-104-v1.0.104-CLEAN-V2`
- artifact ID : `9955204052`
- SHA256 ZIP : `0eda9f49db3904a6f0d6773341573aae381edb22b8bd116dbb0b76445220c693`

BUILD #103 reste un ancien baseline protege, mais **n'est plus l'etat de production courant** :
- commit #103 : `1d6316bd1746d6f2b4cfb751cab88d18e27ef730`

Ne pas revenir a #103 sauf demande explicite.

Aucun BUILD #105 n'a ete lance a la date de cette bascule.

---

## 3. INCIDENT DOCUMENTAIRE IA QUI A MOTIVE LE TRAVAIL EN COURS

Question utilisateur reelle :
`Quel est le jeu axial du pignon primaire et comment le contrôler ?`

Comportement incorrect constate dans #104 :
- selection `DOC_RCL0193ENG` pages 342-343 ;
- contenu antenne/coax sans rapport ;
- mauvais schema p342 propose.

Preuve correcte presente dans RAVEMEMS V2 :
- document : `DOC_RCL0193ENG`
- operation : `12.21.28`
- titre : `ENGINE CRANKSHAFT PRIMARY GEAR END-FLOAT`
- page physique : **53**
- valeur : **0.089 a 0.165 mm**
- controle : cales d'epaisseur entre backing ring et primary gear
- visuels lies : `VIS_P0053_001`, `VIS_P0053_002`

Le probleme est donc une mauvaise selection de preuve/document/page dans la chaine aval, et non l'absence de la donnee source.

---

## 4. RAVEMEMS V2 / PACK001 — ETAT VALIDE

Source RAVE canonique : `main/rave/`
- 47 PDF
- 1 359 pages au total dans le corpus traite

RAVEMEMS V2 pertinent :
- branche : `tmp-ravemems-v2-language-fix`
- tip : `4942f1e7bd11bddf3c0f4cf9bcc5cbe0b6e11c4d`
- moteur herite : `5232215f5853241572f580cf1696c9f881b254f4`

V2 conserve notamment :
- `source_language`
- `revision_key`
- `page_key`
- les liens visuels exacts

Pack001 valide :
- branche : `tmp-memslibrary-pack001`
- base validee : `9cf3d00840cec1749ef9358f708d3e68a2c8bc4d`
- 47 documents
- 1 359 pages
- 1 667 visuels
- 5 453 entrees de recherche
- 0 review ouverte
- SQLite integrity : OK
- FK cassees : 0
- SHA256 Pack : `827208b703324335828d72093994ec5f7bf9e0a6fd0cd3b0082d033bfa38628b`

Important : les **images binaires ne sont pas contenues dans MEMSLibrary.dll**. La DLL exploite la base et les references/liens ; les assets visuels restent des fichiers separes du runtime/package.

---

## 5. CAUSE RACINE ISOLEE DANS MEMSLibrary.dll

L'ancien export public utilise par le bridge est :
`MEMSLibrary_SearchPack(...)`

Il ne permettait pas au demandeur de contraindre la recherche par :
- document ;
- revision ;
- langue source.

Le Pack001 contenait deja ces informations, mais l'ABI publique de recherche ne les exposait pas suffisamment pour isoler la provenance.

Rupture precise identifiee :
**Pack001 -> MEMSLibrary_SearchPack() / ABI2**

---

## 6. CORRECTION ISOLEE DE MEMSLibrary.dll — VALIDEE VERTE

Branche temporaire :
`tmp-memslibrary-dll-provenance-fix`

Base exacte :
`9cf3d00840cec1749ef9358f708d3e68a2c8bc4d`

HEAD final :
`a768d0784da451367e9ee08efa11f4621e13656b`

Correction additive et backward-compatible :
- ABI historique conservee a `2` ;
- ancien `MEMSLibrary_SearchPack(...)` conserve ;
- ajout de `MEMSLibrary_SearchPackFiltered(...)` ;
- nouveaux filtres exacts disponibles : `document_key`, `revision_key`, `source_language`, `entity_kind` ;
- nouveau resultat retourne egalement la provenance ;
- requetes SQL parametrees ;
- mismatch de filtre = succes avec 0 resultat ;
- ordre deterministe conserve/etendu.

Validation finale :
- run : **`33953367833`**
- job : **`101272147404`**
- HEAD : `a768d0784da451367e9ee08efa11f4621e13656b`
- conclusion : **SUCCESS**

Preuves du smoke test :
- ancien ABI2 primaire -> `DOC_RCL0193ENG:p53`
- ancien ABI2 batterie -> `DOC_RCL0221ENG:p20`
- recherche filtree primaire -> `DOC_RCL0193ENG:p53`
- langue retournee -> `en`
- revision -> `REV_RCL0193ENG_SOURCE`
- mauvais document -> 0 resultat
- mauvaise langue -> 0 resultat
- mauvaise revision -> 0 resultat
- p342 dans le test contraint -> 0 resultat
- Pack002 corrompu rejete sans casser Pack001

SHA256 nouvelle DLL :
`77d73dd7c64fa2271fa26bb5d724b1c3b0c343c0669af3af5647236546437b7a`

Artefact isole :
- `MEMSLibrary-DLL-Provenance-33953367833`
- artifact ID : `9965560001`
- ZIP SHA256 : `f6987c821d312a505019e02cfdd3b254bc04e1a3b86657fc1f3f87dd3e99c491`

Pack apres tests : toujours byte-identique :
`827208b703324335828d72093994ec5f7bf9e0a6fd0cd3b0082d033bfa38628b`

---

## 7. CE QUI N'EST PAS ENCORE FAIT

La correction DLL est validee, mais **BUILD #104 ne l'utilise pas encore**.

Le bridge actuel appelle encore l'ancien export :
`MEMSLibrary_SearchPack(...)`

Aucune integration de :
`MEMSLibrary_SearchPackFiltered(...)`

dans `expert/IaMemsLibraryBridge.cpp` n'a encore ete effectuee.

Aucune modification de production n'a ete faite pour cette integration.
Aucun BUILD #105 n'a ete lance.

La chaine complete IA/documentation ne doit donc **pas** etre declaree globalement verte a ce stade.

---

## 8. IMAGES / VISUELS — ETAT EXACT

- Les assets images reels sont separes de `MEMSLibrary.dll`.
- Pack001/RAVEMEMS V2 contiennent les metadonnees et liens permettant d'identifier les visuels associes a une page/operation.
- Pour le cas primaire p53, les visuels de preuve attendus sont `VIS_P0053_001` et `VIS_P0053_002`.
- Le bridge devra selectionner les liens visuels correspondant a la **preuve retenue**, pas un visuel d'une page concurrente.
- Aucun changement visuel non demande ne doit etre introduit.

---

## 9. TRACABILITE DE LA BASCULE RAPPORT 1 -> RAPPORT 2

Derniere correction DLL journalisee dans le rapport n°1 :
- queue commit : `dda369be1a849e2b27e496b8a569a8d6c78cf52e`
- writer run : `33953473746`
- conclusion : **SUCCESS**
- commit d'append automatique : `06b0cb019244a457ebdd5387192b3a1dc519a071`

Plan de bascule vers ce rapport n°2 journalise dans le rapport n°1 :
- queue commit : `7c386791b2797c560ef176f5593f0aaafaac3865`
- writer run : `33953899426`
- conclusion : **SUCCESS**

L'ancien rapport est desormais l'historique de reference. Ce rapport n°2 devient le journal actif.

---

# PROCHAINE ACTION EXACTE

**Raccorder la DLL corrigee au bridge IA de MEMS Manager sur une branche temporaire, et uniquement ce raccordement.**

Sequence imposee :
1. journaliser le plan de modification dans ce rapport n°2 avant toute pousse technique ;
2. modifier le bridge pour utiliser `MEMSLibrary_SearchPackFiltered(...)` sans casser l'ancien comportement de secours ;
3. transmettre des contraintes de provenance uniquement lorsqu'elles sont connues/justifiees ; ne jamais hardcoder `RCL0193ENG`, p53 ou une reponse particuliere ;
4. faire suivre a la preuve retenue les liens visuels exacts associes ;
5. tester la question reelle `Quel est le jeu axial du pignon primaire et comment le contrôler ?` ;
6. resultat attendu : p53, operation 12.21.28, 0.089-0.165 mm, controle aux cales d'epaisseur, visuels p53, et **aucune selection p342 antenne/coax** ;
7. tester plusieurs autres questions afin de prouver que le correctif est general ;
8. journaliser immediatement le resultat reel ;
9. **ne pas lancer BUILD #105** tant que la branche temporaire et les tests de chaine ne sont pas verts.

## 2026-09-05 — ACTIVATION DU RAPPORT ACTIF N°2

Migration de journalisation effectuee :
- `RAPPORT_SUIVI_ECU_MEMS_MANAGER.md` reste conserve comme historique maitre n°1 ;
- `RAPPORT_SUIVI_ECU_MEMS_MANAGER_SUITE.md` est desormais le rapport actif officiel ;
- `tools/append_master_report.py` cible le rapport n°2 ;
- `.github/workflows/report-master-journal.yml` ajoute et verifie le rapport n°2 ;
- le mecanisme `.journal_queue` reste inchange dans son principe.

Cette entree sert de test reel de la nouvelle cible de journalisation.

PROCHAINE ACTION EXACTE : raccorder `MEMSLibrary_SearchPackFiltered(...)` au bridge IA sur branche temporaire, avec tests document/page/visuels, sans BUILD #105 avant validation verte.
<!-- journal-entry-sha256:a5468a8fdfcb83fe3a3a77cb84dd4f9f42011253fcc690cf0b16321ef38d2d87 -->

## 2026-09-05 — VALIDATION DE LA BASCULE RAPPORT N°2

Test réel du nouveau mécanisme de journalisation :
- queue commit de test : `28be30e49f45b1241f2ece218d2c118c8a0ac302` ;
- workflow : `RAPPORT master journal writer` ;
- run : `33953968090` ;
- job : `101273783373` ;
- conclusion : **SUCCESS** ;
- commit d'append automatique : `c832dfaef53b8917658656321bcaeafc4c9f14e1`.

Contrôle de périmètre après bascule : le compare depuis le dernier état du rapport n°1 montre uniquement :
- `.github/workflows/report-master-journal.yml` ;
- `tools/append_master_report.py` ;
- `RAPPORT_SUIVI_ECU_MEMS_MANAGER_SUITE.md`.

`RAPPORT_SUIVI_ECU_MEMS_MANAGER.md` n'a donc pas été modifié après sa clôture comme historique maître n°1.

La migration est **VALIDÉE VERTE**. Le rapport n°2 est désormais la seule cible active de journalisation.

PROCHAINE ACTION EXACTE : raccorder `MEMSLibrary_SearchPackFiltered(...)` au bridge IA sur branche temporaire, avec tests document/page/visuels, sans BUILD #105 avant validation verte.
<!-- journal-entry-sha256:58d20a83489148728ed4587cf77bf7cbab04f93b1062e28d0259e4a67ed8c898 -->

## 2026-09-05 — PLAN AVANT POUSSE — RACCORDEMENT DLL FILTREE AU BRIDGE IA

Autorisation utilisateur : `OK ON CONTINUE`.

Périmètre strict de la prochaine modification technique : **raccorder la DLL MEMSLibrary corrigée au bridge IA de MEMS Manager sur une branche temporaire, et uniquement ce raccordement**.

État de départ :
- production courante : BUILD #104, commit `a55427affeec84643f916621df6247adf29e80fb` ;
- nouvelle DLL MEMSLibrary validée verte sur `tmp-memslibrary-dll-provenance-fix`, HEAD `a768d0784da451367e9ee08efa11f4621e13656b` ;
- export historique `MEMSLibrary_SearchPack(...)` conservé ;
- nouvel export disponible `MEMSLibrary_SearchPackFiltered(...)` ;
- aucun BUILD #105 lancé.

Travail autorisé :
1. inspecter le chargement réel de `MEMSLibrary.dll`, le bridge et le rattachement des visuels dans #104 ;
2. créer une branche temporaire dédiée depuis l'état applicatif #104 pertinent ;
3. modifier uniquement les fichiers nécessaires au bridge / tests temporaires / workflow temporaire pour appeler `MEMSLibrary_SearchPackFiltered(...)` lorsque cette API est disponible ;
4. conserver un comportement de secours compatible avec l'ancien export lorsque nécessaire ;
5. transmettre des contraintes de provenance uniquement lorsqu'elles sont réellement connues ou dérivées d'une preuve déjà sélectionnée ; **aucun hardcode de `DOC_RCL0193ENG`, p53, opération 12.21.28 ou de la réponse utilisateur** ;
6. faire suivre à la preuve retenue les liens/visuels exacts RAVEMEMS V2 associés, sans modifier l'UI ni les règles visuelles hors de ce raccordement ;
7. intégrer/stager la DLL corrigée uniquement dans le workflow temporaire de test si le packaging l'exige ; ne pas modifier la production avant validation.

Tests obligatoires avant toute production :
- question réelle : `Quel est le jeu axial du pignon primaire et comment le contrôler ?` ;
- preuve attendue : `DOC_RCL0193ENG`, page physique 53, opération 12.21.28, valeur 0.089–0.165 mm, contrôle aux cales d'épaisseur ;
- visuels attendus : `VIS_P0053_001` / `VIS_P0053_002` ou leurs liens runtime exacts ;
- exclusion explicite : aucune sélection p342 antenne/coax ;
- plusieurs autres questions documentaires distinctes doivent également passer afin de prouver que le correctif est général et non spécifique au cas p53.

Interdictions :
- aucun changement ECU/protocole/acquisition/RAM/écriture/reset ;
- aucun changement UI/scroll/rendu visuel non requis ;
- aucun changement du corpus RAVE/RAVEMEMS V2/Pack001 ;
- aucun BUILD #105 avant validation verte de la branche temporaire et journalisation du résultat.

PROCHAINE ACTION EXACTE : terminer l'inspection du bridge #104 et du packaging, puis effectuer le raccordement minimal sur branche temporaire et lancer les tests de chaîne dédiés.
<!-- journal-entry-sha256:7ae3fcc0ec3984cfda3695c6fce3cc7b60e75a84c83f7a34e7f67f6598f16391 -->

## 2026-09-05 — RÉSULTAT TEST — BRIDGE IA MEMSLibrary FILTRÉ : VERT

Autorisation de reprise : utilisateur `OK ON CONTINUE`, puis confirmation `OK TU AVANCE`.

Branche temporaire : `tmp-ai-memslibrary-filtered-bridge`
Base stricte : BUILD #104 `a55427affeec84643f916621df6247adf29e80fb`
HEAD testé : `c62386a540a7bd89007fb8f5b0835603ca634591`
Workflow temporaire : `TMP IA MEMSLibrary Filtered Bridge`
Run : **33955239592**
Job : **101277276899**
Conclusion : **SUCCESS**

Périmètre vérifié par le workflow : 6 fichiers seulement par rapport à #104 :
- `.github/workflows/tmp-ai-memslibrary-filtered-bridge.yml`
- `expert/IaMemsLibraryBridge.cpp`
- `expert/IaMemsLibraryBridge.h`
- `expert/IaMemsLibraryBridgeSelfTest.cpp`
- `expert/bridge-selftest/CMakeLists.txt`
- `memslibrary/include/MEMSLibrary.h`

Fichiers de production explicitement prouvés inchangés :
- `CMakeLists.txt`
- `iamemstab.cpp`
- `expert/IaMemsDiagramCatalog.cpp`
- `expert/IaMemsDiagramCatalog.h`
- `expert/IaMemsLibraryIntegration.cpp`

Entrées runtime épinglées et vérifiées :
- DLL corrigée artifact `9965560001`, digest `sha256:f6987c821d312a505019e02cfdd3b254bc04e1a3b86657fc1f3f87dd3e99c491`
- DLL SHA256 `77d73dd7c64fa2271fa26bb5d724b1c3b0c343c0669af3af5647236546437b7a`
- Pack001 SQLite SHA256 `827208b703324335828d72093994ec5f7bf9e0a6fd0cd3b0082d033bfa38628b`
- catalogue visuel V2 propre artifact `9954393075`, digest `sha256:b056ada5b925648824274f6b04cfa95b81fb6ba8bd07c101f3210757f5ef4df8`

Cause racine reproduite et corrigée côté bridge :
- l'ancien moteur SQL fait des `LIKE` sous-chaîne ; le terme français `axial` pouvait donc correspondre à `coaxial` dans les opérations antenne/coax p342-p343 ;
- #104 arrêtait ensuite la collecte dès qu'il avait assez de premiers résultats, ce qui pouvait injecter cette mauvaise preuve dans Qwen et dans le routage visuel ;
- le bridge temporaire utilise maintenant l'export additif `MEMSLibrary_SearchPackFiltered(...)` quand disponible, conserve la provenance structurée, vérifie les termes par mots complets avant acceptation et classe les candidats au lieu d'accepter le premier faux positif ;
- l'ancien export ABI2 reste un fallback compatible si le nouvel export n'est pas disponible.

Question réelle testée :
`Quel est le jeu axial du pignon primaire et comment le contrôler ?`

Sortie binaire exacte :
`IA_MEMSLIBRARY_BRIDGE_FILTERED_PASS primary=DOC_RCL0193ENG:p53 revision=REV_RCL0193ENG_SOURCE language=en visual=VIS_P0053_001 battery=DOC_RCL0221ENG:p20 throttle=DOC_RCL0195ENG:p35 axial_coaxial_rejected=1 p342=0`

Preuves obligatoires validées :
- document : `DOC_RCL0193ENG`
- page : `53`
- révision : `REV_RCL0193ENG_SOURCE`
- langue source : `en`
- opération / contenu : `12.21.28`, `ENGINE CRANKSHAFT PRIMARY GEAR END-FLOAT`
- contrôle : `Using feeler gauges...`
- valeur : `0.089 to 0.165 mm`
- p342/p343 antenne/coax : **absentes**
- garde sous-chaîne `axial -> coaxial` : **rejetée**

Visuel :
- le catalogue V2 propre contient bien `VIS_P0053_001` et `VIS_P0053_002` pour `RCL0193ENG` p53 ;
- le routage visuel existant, sans modification, a suivi la preuve corrigée et a sélectionné `VIS_P0053_001` ;
- aucune modification de la logique visuelle/UI n'a été nécessaire.

Régressions documentaires distinctes validées :
- batterie : `DOC_RCL0221ENG:p20`
- throttle potentiometer : `DOC_RCL0195ENG:p35`

Statut : **bridge + DLL + Pack001 + rattachement visuel de la preuve = VERT en test isolé réel Windows x64**.

Important :
- `MEMSX64` n'a pas été modifiée ;
- BUILD #104 reste la production ;
- aucun BUILD #105 n'a été lancé ;
- ce run n'est pas encore une validation du package applicatif complet.

PROCHAINE ACTION EXACTE : faire un préflight applicatif temporaire complet depuis cette branche, en compilant le vrai exécutable et en stageant la DLL corrigée + Pack001 + référence V2 propre, puis exécuter les self-tests existants et le nouveau test bridge dans le même runtime. Ne pas pousser sur `MEMSX64` et ne pas lancer BUILD #105 avant que ce préflight applicatif complet soit VERT et journalisé.
<!-- journal-entry-sha256:a1c14a2e27cead4294bb661e5d3f40dd9d6a451c757456ae3a042d899ca0046d -->

## 2026-09-05 — PLAN AVANT POUSSE — PRÉFLIGHT APPLICATIF COMPLET DU BRIDGE FILTRÉ

Point de départ validé :
- bridge temporaire `tmp-ai-memslibrary-filtered-bridge`
- HEAD vert `c62386a540a7bd89007fb8f5b0835603ca634591`
- run bridge `33955239592` SUCCESS
- DLL corrigée SHA256 `77d73dd7c64fa2271fa26bb5d724b1c3b0c343c0669af3af5647236546437b7a`
- Pack001 SQLite SHA256 `827208b703324335828d72093994ec5f7bf9e0a6fd0cd3b0082d033bfa38628b`
- preuve réelle p53 + visuel `VIS_P0053_001` validés.

Prochaine étape autorisée par la reprise utilisateur : **préflight du vrai exécutable**, toujours hors production.

Méthode :
1. créer `tmp-ai-memslibrary-app-preflight` depuis le HEAD vert `c62386a...` ;
2. ajouter uniquement un workflow temporaire de préflight complet ;
3. compiler le vrai `ecu_mems_manager.exe` en Windows x64 avec la même configuration Qt/ONNX/librosco que la voie MEMSX64 ;
4. télécharger et épingler la base runtime IA déjà validée, la référence V2 propre, la DLL MEMSLibrary corrigée et le Pack001 inchangé ;
5. stager dans le runtime de test la DLL corrigée + Pack001 + référence V2 propre ;
6. exécuter les self-tests existants (`ia_response_selftest`, `ia_mems_diagram_selftest`, `rosco_abi_selftest`) + le nouveau `ia_memslibrary_bridge_selftest` ;
7. assembler un **artefact temporaire de préflight**, sans nom BUILD production, puis vérifier intégrité SQLite, architecture x64 et effectuer un smoke launch de l'exécutable ;
8. journaliser immédiatement le résultat.

Contraintes :
- `MEMSX64` reste sur BUILD #104 `a55427affeec84643f916621df6247adf29e80fb` ;
- aucun BUILD #105 ;
- aucun changement UI, scroll, catalogue visuel, protocole ECU, acquisition, RAM, écriture/reset ;
- aucun changement RAVE/RAVEMEMS V2/Pack001 ;
- aucun changement de `memsx64.yml` ; le préflight utilise son propre workflow temporaire ;
- pas de promotion en production tant que ce préflight applicatif complet n'est pas VERT et journalisé.

PROCHAINE ACTION EXACTE : créer la branche temporaire de préflight, ajouter le workflow dédié, lancer le vrai build applicatif x64 et vérifier tous les tests + smoke launch avec la DLL corrigée.
<!-- journal-entry-sha256:166d557197c40ddd1193bc445c41d87c4326a7c4db8206da65aa8de70185a75f -->

## 2026-09-05 — BRIDGE IA / MEMSLibrary FILTRÉ — RUN 1 VERT MAIS ISOLATION DE PAGE À DURCIR

Branche temporaire : `tmp-ai-memslibrary-filtered-bridge`
Base protégée : BUILD #104 `a55427affeec84643f916621df6247adf29e80fb`
HEAD testé : `c62386a540a7bd89007fb8f5b0835603ca634591`

Run : `33955239592`
Job : `101277276899`
Conclusion GitHub Actions : **SUCCESS**.

Preuves réelles du run :
- garde d'ascendance BUILD #104 et périmètre : PASS ;
- DLL corrigée épinglée SHA256 `77d73dd7c64fa2271fa26bb5d724b1c3b0c343c0669af3af5647236546437b7a` : PASS ;
- Pack001 byte-identique SHA256 `827208b703324335828d72093994ec5f7bf9e0a6fd0cd3b0082d033bfa38628b` : PASS ;
- visuels p53 `VIS_P0053_001` et `VIS_P0053_002` présents et vérifiés : PASS ;
- question réelle `Quel est le jeu axial du pignon primaire et comment le contrôler ?` : `DOC_RCL0193ENG:p53`, révision `REV_RCL0193ENG_SOURCE`, langue `en`, p342=0 ;
- visuel réellement sélectionné : `VIS_P0053_001` ;
- batterie : `DOC_RCL0221ENG:p20` ;
- throttle potentiometer : `DOC_RCL0195ENG:p35` ;
- faux positif `axial` -> `coaxial` rejeté ;
- fichiers production protégés (`CMakeLists.txt`, `iamemstab.cpp`, catalogue visuel, intégration service) inchangés.

Constat post-run avant intégration production : le bridge sélectionne correctement p53, mais la construction finale des extraits conserve encore des candidats de la même provenance document/révision/langue provenant d'autres pages. Sur le cas primaire, le classement peut donc ajouter après les extraits p53 une ligne d'une autre page (ex. p92). Le self-test interdit déjà p342/coaxial mais ne verrouille pas encore l'absence de toute page différente de la page sélectionnée.

Ce point est traité comme défaut résiduel de sélection de preuve : **ne pas déclarer la chaîne finale verte et ne pas lancer BUILD #105**.

### CORRECTION SUIVANTE AUTORISÉE DANS LE MÊME PÉRIMÈTRE

Modifier uniquement `expert/IaMemsLibraryBridge.cpp` et le self-test associé afin que, lorsqu'une page physique valide a été sélectionnée, les extraits finaux soient strictement limités à `document + revision + source_language + selectedPage`. Aucune modification de l'UI, du catalogue visuel, du service d'intégration, du protocole ECU ou de MEMSX64.

Relancer le workflow temporaire et exiger :
- p53 uniquement pour la question primaire ;
- aucune p342/p343/coaxial/aerial ;
- aucune autre page (p92/p187/p196/etc.) dans le contexte primaire ;
- `VIS_P0053_001` ou `VIS_P0053_002` ;
- régressions batterie/throttle toujours vertes.

PROCHAINE ACTION EXACTE : durcir l'isolation à la page sélectionnée sur la branche temporaire, relancer le test, journaliser le résultat. Aucun BUILD #105.
<!-- journal-entry-sha256:2f81a7c97efb80e4db950f81cd5199889e1aab513369577a4c366256a6984ea0 -->

## 2026-09-05 — BRIDGE IA / MEMSLibrary — RUN 3 ROUGE APRÈS ISOLATION STRICTE DE PAGE

Branche : `tmp-ai-memslibrary-filtered-bridge`
Base protégée : BUILD #104 `a55427affeec84643f916621df6247adf29e80fb`
HEAD testé : `16c679ad3730da2ad92967216e37a855616584d7`

Run : `33955644103`
Job : `101278376491`
Conclusion : **FAILURE**.

Étapes avant le test documentaire :
- ascendance/périmètre : PASS ;
- DLL corrigée + Pack001 épinglés et SHA vérifiés : PASS ;
- visuels p53 `VIS_P0053_001` / `VIS_P0053_002` : PASS ;
- configuration du self-test : PASS ;
- compilation du self-test : PASS.

Échec réel :
`FAIL battery regression failed doc=DOC_RCL0193ENG page=136`
Self-test exit 17.

Le durcissement à la page sélectionnée compile donc correctement, mais le classement général des groupes de preuve peut maintenant sélectionner un mauvais groupe documentaire pour la question batterie. La correction ne doit pas être contournée en relâchant l'isolation de page, ni en hardcodant `DOC_RCL0221ENG:p20`.

Aucun fichier de production protégé n'a été poussé vers MEMSX64 et aucun BUILD #105 n'a été lancé.

### PROCHAINE ACTION EXACTE
Analyser les résultats Pack001 exacts produits pour les requêtes batterie/restoration afin d'identifier pourquoi `DOC_RCL0193ENG:p136` bat la preuve attendue `DOC_RCL0221ENG:p20`, puis corriger uniquement la logique générale de ranking/couverture sémantique du bridge et son self-test. Conserver l'isolation stricte document+révision+langue+page. Relancer le workflow temporaire, puis journaliser le résultat. Aucun BUILD #105.
<!-- journal-entry-sha256:352c018e747ba903b52a4053913ef605e288c4bfae4f15eb67f85b75be72e260 -->

## 2026-09-05 — BRIDGE IA / MEMSLibrary FILTRÉ — VALIDATION FINALE VERTE

Branche temporaire : `tmp-ai-memslibrary-filtered-bridge`
Base exacte protégée : BUILD #104 `a55427affeec84643f916621df6247adf29e80fb`
HEAD final testé : `981b110f5506c9938ffcb7f7720920b898123c16`

Run GitHub Actions : **`33955854456`**
Job : **`101278959443`**
Conclusion : **SUCCESS**.

Preuve terminale réelle du self-test :
`IA_MEMSLIBRARY_BRIDGE_FILTERED_PASS primary=DOC_RCL0193ENG:p53 revision=REV_RCL0193ENG_SOURCE language=en primary_sources=6 page_pure=1 visual=VIS_P0053_001 battery=DOC_RCL0221ENG:p20 throttle=DOC_RCL0195ENG:p35 axial_coaxial_rejected=1 p342=0`

Autres preuves du run :
- `BRIDGE_SCOPE_PASS changed=6` ;
- `PINNED_RUNTIME_INPUTS_PASS` ;
- DLL corrigée SHA256 `77d73dd7c64fa2271fa26bb5d724b1c3b0c343c0669af3af5647236546437b7a` ;
- Pack001 SHA256 inchangé `827208b703324335828d72093994ec5f7bf9e0a6fd0cd3b0082d033bfa38628b` ;
- `P53_VISUAL_PROVENANCE_PASS assets=VIS_P0053_001,VIS_P0053_002` ;
- `PROTECTED_PRODUCTION_FILES_UNCHANGED_PASS`.

Question réelle validée :
`Quel est le jeu axial du pignon primaire et comment le contrôler ?`

Résultat validé :
- document `DOC_RCL0193ENG` ;
- révision `REV_RCL0193ENG_SOURCE` ;
- langue source `en` ;
- page physique **53** ;
- 6 extraits et **tous strictement p53** (`page_pure=1`) ;
- preuve contenant opération `12.21.28`, valeur `0.089–0.165 mm` et contrôle aux `feeler gauges` ;
- visuel réellement suivi depuis la preuve : `VIS_P0053_001` ;
- faux positif `axial` -> `coaxial` rejeté ;
- page 342 exclue (`p342=0`).

Régressions générales incluses :
- batterie : `DOC_RCL0221ENG:p20` ;
- throttle potentiometer : `DOC_RCL0195ENG:p35`.

La correction générale comprend :
- résolution dynamique additive de `MEMSLibrary_SearchPackFiltered(...)` ;
- conservation de l'ancien export ABI2 en fallback ;
- validation en tokens exacts afin de rejeter les faux positifs de sous-chaîne ;
- ranking où la précision du meilleur résultat prime sur le volume de résultats génériques ;
- regroupement et revalidation par document + révision + langue + page ;
- contexte final envoyé à Qwen limité strictement à la page sélectionnée ;
- aucune valeur RCL0193/p53 hardcodée dans le bridge.

Compare BUILD #104 -> HEAD final :
- statut `ahead` ;
- `ahead_by=10` ;
- `behind_by=0` ;
- merge-base = BUILD #104 exact ;
- exactement 6 fichiers autorisés modifiés/ajoutés :
  1. `.github/workflows/tmp-ai-memslibrary-filtered-bridge.yml`
  2. `expert/IaMemsLibraryBridge.cpp`
  3. `expert/IaMemsLibraryBridge.h`
  4. `expert/IaMemsLibraryBridgeSelfTest.cpp`
  5. `expert/bridge-selftest/CMakeLists.txt`
  6. `memslibrary/include/MEMSLibrary.h`

Fichiers production protégés explicitement inchangés :
- `CMakeLists.txt`
- `iamemstab.cpp`
- `expert/IaMemsDiagramCatalog.cpp`
- `expert/IaMemsDiagramCatalog.h`
- `expert/IaMemsLibraryIntegration.cpp`

### ÉTAT À CE CHECKPOINT
La correction bridge + DLL + preuve documentaire + suivi visuel est **VERTE SUR BRANCHE TEMPORAIRE**.
La production BUILD #104 reste **inchangée** et ne bénéficie pas encore de cette correction.
**Aucun BUILD #105 n'a été lancé.**

### PROCHAINE ACTION EXACTE
STOP à ce checkpoint. Attendre l'autorisation explicite de l'utilisateur avant toute promotion/intégration de la correction validée vers `MEMSX64` et avant tout lancement d'un BUILD #105. Lors de l'autorisation : journaliser d'abord le plan de promotion dans ce rapport n°2, puis intégrer uniquement les fichiers applicatifs nécessaires et la nouvelle DLL validée, lancer le préflight/build prévu, tester, puis journaliser immédiatement le résultat.
<!-- journal-entry-sha256:9fe12793184b86c70b0ffcf246b34f35a90ad5a7d65e6932d2a2cc8ad8a26e61 -->

## 2026-09-05 — BUILD #105 : autorisation utilisateur et plan de pousse

Autorisation explicite reçue : `GO` après validation VERTE de `tmp-ai-memslibrary-filtered-bridge` HEAD `981b110f5506c9938ffcb7f7720920b898123c16`, run `33955854456` SUCCESS.

État production avant pousse :
- branche `MEMSX64`
- HEAD `a55427affeec84643f916621df6247adf29e80fb`
- BUILD #104 actuel ; aucun #105 encore lancé.

Périmètre autorisé pour #105 : intégrer uniquement la correction validée du bridge MEMSLibrary / provenance filtrée nécessaire à l'application, avec la DLL corrigée déjà validée et son contrat public associé. Aucun changement protocole ECU, acquisition, écritures RAM, UI, scroll, catalogue visuel ou autre sous-système non nécessaire.

Procédure :
1. reprendre les seuls fichiers applicatifs validés depuis `tmp-ai-memslibrary-filtered-bridge` ;
2. vérifier le diff exact par rapport à #104 ;
3. mettre à jour uniquement ce qui est nécessaire au build/packaging pour embarquer la DLL corrigée si la production #104 embarque encore l'ancienne ;
4. lancer BUILD #105 via GitHub Actions ;
5. vérifier compilation, packaging, self-tests documentaires réels et preuve p53 ;
6. journaliser immédiatement le résultat avant toute correction ou nouvelle pousse.

Preuve attendue : question `Quel est le jeu axial du pignon primaire et comment le contrôler ?` -> `DOC_RCL0193ENG`, page 53, révision `REV_RCL0193ENG_SOURCE`, langue `en`, valeur `0.089–0.165 mm`, contrôle aux cales d'épaisseur, visuel p53, aucune p342/coaxial, contexte page-pure.
<!-- journal-entry-sha256:e664ad5135d6f516faecc25225e83bac400207f490e86ceb357b57f7b1d24bf0 -->

## 2026-09-05 — BUILD #105 préflight temporaire : run 1 ROUGE sur transport ONNX externe

Branche temporaire : `tmp-memsx64-build105-filtered-bridge`
HEAD testé : `16e9a22fbac367a51b534ad3d867db15a13d09c5`
Run : `33956355098`
Job : `101280296674`
Conclusion : FAILURE.

Étapes passées avant l'échec :
- checkout exact source : SUCCESS ;
- garde source directe / aucune injection : SUCCESS (`DIRECT_SOURCE_NO_PATCH_PASS`) ;
- Python : SUCCESS ;
- Qt 5.15.2 MSVC x64 : SUCCESS.

Échec exact : étape historique `Download pinned ONNX Runtime development files`, avant le téléchargement de la DLL MEMSLibrary corrigée, avant compilation application/bridge, avant self-tests et avant packaging.
Log : `curl: (35) Send failure: Connection was reset`, puis `GenAI download failed`.

Diagnostic : défaillance de transport réseau externe sur le téléchargement GitHub Release ONNX GenAI. Ce run ne fournit aucune preuve d'un défaut du bridge filtré, de la DLL corrigée ou du Pack001 puisqu'aucun de ces tests n'a été exécuté.

PROCHAINE POUSSE AUTORISÉE APRÈS CE JOURNAL : correction workflow uniquement dans `.github/workflows/memsx64.yml`, sans changement applicatif : conserver les URLs, versions et SHA épinglés et ajouter aux deux commandes curl existantes la robustesse `--retry-all-errors --retry-delay 2` en plus de `--retry 3`. Aucun contournement des contrôles SHA, aucun changement bridge/DLL/Pack/protocole/UI.

Après pousse : relancer le même préflight temporaire #105 et journaliser immédiatement son résultat avant toute intégration dans `MEMSX64`.
<!-- journal-entry-sha256:c0d68bdafa6cba51cc6fd288e9fc9d92f09f56eb1f94de9f9daf6a34792553c5 -->

## 2026-09-05 — BUILD #105 : préflight complet VERT, autorisation de pousse production propre

Branche temporaire : `tmp-memsx64-build105-filtered-bridge`
HEAD validé : `d6c80891efcdd9304f29001ed1dc2f156203ded6`
Run préflight : `33956557318`
Job : `101280834195`
Conclusion : SUCCESS.

Validation complète obtenue :
- source directe / aucune injection : PASS ;
- téléchargement ONNX épinglé : PASS après ajout workflow-only de `--retry-all-errors --retry-delay 2` ;
- DLL MEMSLibrary corrigée épinglée : SHA256 `77d73dd7c64fa2271fa26bb5d724b1c3b0c343c0669af3af5647236546437b7a` ;
- Pack001 SQLite historique conservé inchangé : SHA256 `827208b703324335828d72093994ec5f7bf9e0a6fd0cd3b0082d033bfa38628b` ;
- référence V2 propre : `visuals=1667 legacy=0 p53_visuals=2` ;
- garde protocole : PASS ;
- compilation application x64 + self-test bridge : PASS ;
- preuve source inchangée après build : PASS ;
- IA response self-test : PASS ;
- IA diagram self-test : PASS ;
- ROSCO ABI self-test : PASS ;
- bridge MEMSLibrary : `IA_MEMSLIBRARY_BRIDGE_FILTERED_PASS primary=DOC_RCL0193ENG:p53 revision=REV_RCL0193ENG_SOURCE language=en primary_sources=6 page_pure=1 visual=VIS_P0053_001 battery=DOC_RCL0221ENG:p20 throttle=DOC_RCL0195ENG:p35 axial_coaxial_rejected=1 p342=0` ;
- paquet : `CLEAN_BUILD_PACKAGE_PASS visuals=1667 legacy=0 docs=47 pages=1359 search=5453 corrected_dll=1` ;
- smoke launch : `BUILD_105_SMOKE_PASS` ;
- upload artefact temporaire : PASS.

PROCHAINE ACTION EXACTE : créer un commit production propre directement sur la base BUILD #104 `a55427affeec84643f916621df6247adf29e80fb`, sans fusionner l'historique ni le workflow temporaire. Le commit production doit modifier exactement les 6 fichiers nécessaires :
1. `.github/workflows/memsx64.yml`
2. `expert/IaMemsLibraryBridge.cpp`
3. `expert/IaMemsLibraryBridge.h`
4. `expert/IaMemsLibraryBridgeSelfTest.cpp`
5. `expert/bridge-selftest/CMakeLists.txt`
6. `memslibrary/include/MEMSLibrary.h`

Aucun autre fichier ne doit changer. Le workflow temporaire `.github/workflows/tmp-build105-filtered-bridge-preflight.yml` ne doit PAS entrer dans `MEMSX64`.

Après pousse : vérifier que le workflow production `ECU MEMS Manager x64` est bien le run_number 105, sur le SHA exact du nouveau commit. Ne pas déclencher de build suivant en cas d'échec ; journaliser immédiatement le verdict réel #105.
<!-- journal-entry-sha256:7344d2ab149a9702c6fbc0a6e518abf90c9dfe5f9eb54aa72653f71a6b63d0ef -->

## 2026-09-05 — BUILD #105 PRODUCTION + TEST PC RÉEL — SUIVI VISUEL/RÉPONSE

### BUILD #105 production

- branche : `MEMSX64`
- commit : `1b106eed05e1fd665b857f73a719f02ee6b6b2ac`
- run : `33956856201`
- job : `101281627904`
- conclusion : **SUCCESS**
- artefact : `ECU-MEMS-Manager-x64-BUILD-105-v1.0.105-CLEAN-V2`
- artifact ID : `9979320548`
- taille : `490453458` octets
- digest : `sha256:4e9127fb76da6776f8ea367b9b69f5e4c941bbe724dcc9a699b75b009baf86f9`
- log package : `PACKAGE_OK build=105 version=1.0.105`

Le BUILD #105 intègre la DLL MEMSLibrary corrigée, conserve Pack001 inchangé et a passé compilation, self-tests, validation du paquet et smoke launch.

### Test réel utilisateur sur PC

Question testée :
`Quel est le jeu axial du pignon primaire et comment le contrôler ?`

Résultat documentaire : **correct**.
- preuve retenue : `DOC_RCL0193ENG p.53`
- révision : `REV_RCL0193ENG_SOURCE`
- opération : `12.21.28`
- valeur : `0,089 à 0,165 mm`
- méthode : contrôle avec des cales d'épaisseur entre bague d'appui et pignon primaire
- aucune contamination p342 / antenne / coaxial constatée

Deux défauts utilisateur restent visibles :
1. la fenêtre visuelle est bien routée sur `RCL0193ENG p.53`, mais affiche un petit fragment/icône recadré au lieu de l'illustration technique pertinente de l'opération ;
2. la réponse IA expose un très long contexte brut MEMSLibrary/RAVE au lieu d'une réponse finale concise et propre.

Conclusion : la recherche documentaire, la page, la valeur et la méthode sont validées sur PC ; la sélection/extraction du visuel dans la preuve retenue et la synthèse/présentation finale de la réponse restent à corriger.

### PROCHAINE ACTION EXACTE

Créer une branche temporaire strictement depuis BUILD #105 et corriger **uniquement** :
- la sélection/extraction du visuel pertinent à l'intérieur de la preuve/page déjà retenue ;
- la synthèse/présentation de la réponse IA afin que le contexte documentaire reste interne et ne soit pas déversé tel quel à l'utilisateur.

Ne pas modifier le classement/recherche documentaire validé p53. Ne pas toucher au protocole ECU, acquisition ou autres sous-systèmes. Le correctif visuel doit être générique et ne doit pas hardcoder RCL0193ENG p53 ni un VIS particulier.

Tester au minimum la question primaire réelle, puis des régressions documentaires/visuelles non liées. **Aucun BUILD #106 avant validation verte de la branche temporaire et autorisation explicite.**
<!-- journal-entry-sha256:9226c31c3d4d2f16bb1901f5538506fa05b951099180677ff7f17409a05d4a04 -->

## 2026-09-05 — TEST CIBLE BUILD #105 — VISUEL CORRIGE, SYNTHESE QWEN EN ECHEC

Branche temporaire : `tmp-build105-visual-answer-fix`

Commit teste : `dd77dcbebe4a34de58218573a90392fc10c745cf`

Run GitHub Actions : `33959189293`

Resultat reel :
- job `preflight / build_x64` : **SUCCESS** ;
- job cible `visual_and_answer_regression` : **FAILURE** ;
- compilation du test cible : SUCCESS ;
- runtime Qwen valide et assets V2 propres correctement charges ;
- selection visuelle p53 : **PASS**, asset retenu `VIS_P0053_002` ;
- Qwen demarre et atteint l'etat `IA locale prête` ;
- echec final : `Le modèle local n'a pas produit de réponse exploitable dans la langue active.`

Conclusion :
- le correctif de selection visuelle est valide pour le cas p53 ;
- le probleme restant est limite a la synthese documentaire de Qwen a partir d'une preuve anglaise vers une reponse en francais ;
- aucune validation globale de #105 a ce stade ;
- aucun nouveau build de production ne doit etre lance.

PROCHAINE ACTION EXACTE : corriger uniquement l'instruction de synthese documentaire envoyee a Qwen afin d'exiger une reponse concise dans la langue active a partir de la preuve source, puis relancer exactement le meme test cible sans modifier le selecteur visuel ni les sous-systemes ECU/protocole/acquisition.
<!-- journal-entry-sha256:1562a6cf84a77744061a197f0d74b068205fba9131f9f13bef0038cb66635220 -->

## 2026-09-05 — CHANGEMENT DE STRATÉGIE — AUDIT DU BASELINE ET RECONSTRUCTION DOCUMENTAIRE RAVE

Constat utilisateur après test réel du BUILD #105 : la chaîne documentaire actuelle continue à révéler des défauts de fond (sélection d'images non pertinentes, heuristiques de recherche, synthèse IA fragile). L'utilisateur demande d'arrêter l'empilement de correctifs aval et de repartir plus en arrière afin de déterminer le bon baseline applicatif avant de reconstruire correctement la fondation documentaire.

Décision immédiate :
- figer le BUILD #105 ;
- ne pas intégrer le correctif temporaire `tmp-build105-visual-answer-fix` dans `MEMSX64` ;
- ne lancer aucun BUILD #106 avant validation de la nouvelle fondation ;
- conserver l'historique existant uniquement comme preuve et comparaison, sans le prendre automatiquement comme nouvelle base documentaire.

Objectif de l'audit de baseline : déterminer factuellement si la reprise applicative doit partir de #105, #104, #103 ou d'un état antérieur, en comparant les changements réellement introduits dans les builds et en séparant :
1. le socle applicatif/UI/protocole valide ;
2. les intégrations IA/MEMSLibrary/RAVEMEMS qui devront être rejetées ou remplacées.

Ordre de reconstruction demandé :
1. déterminer le baseline applicatif exact ;
2. corriger RAVEMEMS V2 avant toute nouvelle intégration afin que l'extraction et l'assemblage soient structurellement corrects ;
3. repartir des 47 PDF RAVE sources canoniques ;
4. refaire l'extraction texte/pages/opérations/publication/révision/langue sans mélange entre documents ;
5. corriger l'extraction des images afin de rejeter les pictogrammes, logos, icônes et éléments décoratifs de page qui ne constituent pas des preuves techniques ;
6. conserver pour chaque vrai visuel ses coordonnées, dimensions, type et lien exact vers page/section/opération ;
7. auditer manuellement et automatiquement les images et l'extraction avant assemblage ;
8. reconstruire l'assemblage SQLite avec relations explicites document -> révision -> langue -> page -> opération/section -> texte -> visuels ;
9. reconstruire ensuite MEMSLibrary.dll autour de ce schéma afin qu'elle retourne directement la provenance structurée et les identifiants visuels pertinents ;
10. valider base + DLL indépendamment de Qwen avant tout retour dans MEMS Manager.

Cas de référence obligatoire : question `Quel est le jeu axial du pignon primaire et comment le contrôler ?` -> RCL0193ENG p.53, opération 12.21.28, 0.089 à 0.165 mm, contrôle aux cales d'épaisseur, vrai schéma mécanique de contrôle ; aucun p342/coaxial et aucun pictogramme de page.

PROCHAINE ACTION EXACTE : auditer les builds #103, #104, #105 et, si nécessaire, les builds immédiatement antérieurs afin de fixer le baseline applicatif de reprise avant toute reconstruction RAVEMEMS V2.
<!-- journal-entry-sha256:4f253c2054f5c86c76ea5767fda6770c782c163a8e78dd88e87686eee6e5d785 -->

## 2026-09-05 — AUDIT DE RECUL — BASELINE APPLICATIF RETENU : BUILD #101

Suite au changement de stratégie demandé par l'utilisateur, comparaison factuelle des builds de production récents afin de déterminer jusqu'où revenir avant de reconstruire la fondation RAVE/RAVEMEMS/MEMSLibrary.

### BUILD #105 — REJETÉ COMME BASELINE
- commit : `1b106eed05e1fd665b857f73a719f02ee6b6b2ac`
- run : `33956856201`
- SUCCESS
- diff #104 -> #105 : 1 commit, 6 fichiers ; modification majeure de `expert/IaMemsLibraryBridge.cpp`, ajout du self-test bridge, extension du header MEMSLibrary et adaptation du workflow.
- raison du rejet : ce build contient précisément le raccordement provenance/MEMSLibrary que la nouvelle stratégie demande de reconstruire après une nouvelle extraction.

### BUILD #104 — REJETÉ COMME BASELINE
- commit : `a55427affeec84643f916621df6247adf29e80fb`
- run : `33921162926`
- SUCCESS
- diff #103 -> #104 : 4 commits ; introduction de `IaMemsLibraryBridge.*`, `IaMemsLibraryIntegration.cpp`, header `memslibrary/include/MEMSLibrary.h`, modification importante du catalogue visuel et de `iamemstab.cpp`, nouveau packaging V2.
- raison du rejet : c'est le point où l'application reçoit directement la chaîne V2/MEMSLibrary et où la logique visuelle documentaire problématique entre dans le produit.

### BUILD #103 — REJETÉ COMME BASELINE
- commit : `1d6316bd1746d6f2b4cfb751cab88d18e27ef730`
- run : `33334306835`
- SUCCESS
- diff #102 -> #103 : données documentaires uniquement, avec nouvel audit AKM7169, lot `research_enrichment_1870.qz64`, modification du manifest et nombreuses images RAVE AKM7169.
- raison du rejet : encore postérieur à l'intégration documentaire massive et ajoute de nouvelles données/images issues de l'ancienne chaîne.

### BUILD #102 — REJETÉ COMME BASELINE
- commit : `06eca1a478db3d32e9ae88d040e1a34e2cc98650`
- run : `33326675806`
- SUCCESS
- message : `Build #102: integrate validated reference database through batch 1860`.
- diff #101 -> #102 : un commit massif ajoutant audits, manifest, qz64 et un très grand nombre d'images/captures RAVE, notamment RCL0193ENG/FRE et AKM6348.
- raison du rejet : c'est le point d'intégration de la fondation documentaire historique que l'utilisateur demande maintenant de refaire depuis les PDF sources.

### BUILD #101 — BASELINE APPLICATIF RETENU
- commit : `22dbe75ed14e0a61e694159d505ef72245116b48`
- run : `33178021830`
- conclusion : SUCCESS
- message : `Build #101: fix #100 real-test IA regressions`.
- parent #100 : `cd6e52c714ee35a3fffb405797f14a58c2da4fa5`.
- diff #100 -> #101 : seulement 4 fichiers applicatifs (`expert/IaMemsConversationRouting.h`, `expert/IaMemsService.cpp`, `expert/IaResponseLogicTest.cpp`, `iamemstab.cpp`), 37 additions / 7 suppressions ; aucune intégration massive de données RAVE.

### DÉCISION
Le **baseline applicatif de reprise est BUILD #101 `22dbe75ed14e0a61e694159d505ef72245116b48`**.

Important : cela ne signifie pas que les données documentaires déjà présentes dans #101 sont validées pour la nouvelle fondation. La nouvelle reconstruction RAVE ne doit réutiliser comme vérité que les **47 PDF RAVE sources canoniques** et les outils RAVEMEMS V2 corrigés. Les données dérivées historiques seront considérées comme comparaison/audit, pas comme source de reconstruction.

#105 reste figé comme dernier build installé/testé ; il n'est pas supprimé ni réécrit. Aucun #106 ne doit être lancé avant reconstruction et validation de la nouvelle fondation.

### PROCHAINE ACTION EXACTE
Auditer puis corriger **RAVEMEMS V2 avant toute nouvelle extraction** :
1. moteur d'extraction texte/page/opération ;
2. extraction et classification des visuels ;
3. exclusion structurelle des icônes, logos, pictogrammes et décorations de page ;
4. relations exactes visuel -> page -> section/opération ;
5. assemblage SQLite sans mélange de document/révision/langue ;
6. seulement après correction du moteur, relancer une extraction depuis les 47 PDF RAVE sources et auditer résultats/images avant de reconstruire SQLite puis MEMSLibrary.dll.
<!-- journal-entry-sha256:273afc224aad60a6d696f6a1ed334791ffc9e261a8455fed54ed7c3102370188 -->

## 2026-09-05 — AUDIT RAVEMEMS V2 — MARGE DE CAPTURE VISUELLE CONFIRMÉE TROP LARGE

Observation utilisateur : lors de la capture des images RAVE, une marge est ajoutée autour de l'image et elle paraît trop importante.

Vérification dans le moteur neutre RAVEMEMS V2 `5232215f5853241572f580cf1696c9f881b254f4`, fichier `ravemems/v2/core_extract.py` :
- `expanded_clip(..., margin: float = 24.0)` ajoute 24 points PDF à gauche, en haut et à droite ;
- la marge basse est encore plus grande : `margin * 1.35`, soit 32.4 points PDF ;
- cette expansion est appliquée à chaque `source_rect` avant le rendu PNG.

Le filtre des candidats est également insuffisant pour les petits éléments décoratifs :
- image acceptée si largeur/hauteur >= 18 points ;
- image acceptée si sa surface représente >= 0.003 de la page ;
- aucune classification explicite ne rejette actuellement icônes, pictogrammes, logos ou décorations ;
- lorsque le PDF n'expose pas d'image raster candidate, le fallback `page.get_drawings()` peut unir de nombreux tracés de la page en une seule grande zone.

Conclusion : l'observation utilisateur est confirmée. La marge de 24/32.4 points peut capturer du texte ou des éléments voisins, mais la correction ne doit pas se limiter à réduire une constante : il faut également corriger la sélection des candidats visuels pour empêcher les icônes/pictogrammes de devenir des preuves techniques.

Correction à intégrer dans la refonte RAVEMEMS V2 AVANT toute nouvelle extraction des 47 PDF :
1. marge de crop réduite et/ou adaptative, symétrique sauf justification documentée ;
2. conservation séparée de `source_bbox` et `crop_bbox` ;
3. règles structurelles d'exclusion des visuels décoratifs/répétitifs de page ;
4. contrôle du fallback vectoriel pour ne pas unir arbitrairement toute la page ;
5. test visuel de non-régression sur vrais schémas mécaniques, électriques et vues éclatées ;
6. audit du cas RCL0193ENG p.53 pour garantir que le schéma du contrôle du jeu axial est capturé proprement sans éléments parasites.
<!-- journal-entry-sha256:20445ad64d8c02e4356422ec75f0feab647366f53c52c9aac2ac9fd520bed23b -->

## RAVEMEMS V2 — décision d’extraction visuelle : exclure uniquement l’en-tête pour les images

Clarification utilisateur confirmée pendant l’audit de refondation RAVEMEMS V2.

Règle à appliquer :
- l’en-tête de page reste intégralement disponible pour l’extraction du texte, des pages, opérations, références et autres données documentaires ;
- l’exclusion concerne uniquement la détection/capture des visuels ;
- les images/objets graphiques situés dans la zone d’en-tête sont ignorés avant création des assets visuels, car cette zone contient les icônes/pictogrammes répétitifs inutiles ;
- les vraies illustrations techniques commencent sous l’en-tête selon le corpus RAVE observé ;
- la marge de capture doit être appliquée après cette exclusion et ne doit pas réintroduire la zone d’en-tête dans le crop final ;
- cette correction doit être réalisée dans RAVEMEMS V2 avant toute nouvelle extraction complète du corpus, puis validée sur les images extraites avant assemblage SQLite et reconstruction DLL.

Aucun changement MEMSX64 / aucun nouveau build production dans cette étape.
<!-- journal-entry-sha256:ce72c7fe8c70c5dd633ddeed533e136a5e23e90aeb17db419422d4c224bb6698 -->

## 2026-09-05 — Nouveau planning officiel : extraction RAVE validée sémantiquement avant SQLite/DLL

Décision utilisateur : abandonner la logique où RAVEMEMS V2 tente de décider seul, par heuristiques, de la structure sémantique et des visuels pertinents. La nouvelle méthode fait de RAVEMEMS V2 un moteur d’extraction fidèle et reproductible, tandis que l’analyse sémantique des procédures/illustrations est validée explicitement avant assemblage en base.

Baseline applicatif confirmé : BUILD #101 `22dbe75e...`. Les BUILD #102 à #105 ne sont pas utilisés comme fondation documentaire. Aucun BUILD #106 ne doit être lancé pendant cette reconstruction.

### Planning officiel

1. Figer les sources et le périmètre
- Source documentaire : exactement les 47 PDF RAVE originaux du corpus canonique.
- Inventaire SHA/blob/nom/page-count avant traitement.
- Aucun réemploi de SQLite, DLL ou visuels dérivés des chaînes #102+ comme source de vérité.

2. Corriger RAVEMEMS V2 comme moteur d’extraction brute fidèle
- Conserver lecture texte native, géométrie de spans, numéros de pages, métadonnées, provenance.
- Pour les images uniquement : exclure la zone d’en-tête ; ne rien exclure de cette zone pour le texte ou la structure documentaire.
- Réduire la marge de capture actuelle (`24 pt` gauche/haut/droite et `32.4 pt` bas) après mesure sur pages tests ; la marge ne doit jamais remonter dans l’en-tête.
- Corriger le regroupement des tracés vectoriels afin d’éviter les unions artificielles de zones sans rapport.
- Préserver rectangle source exact et rectangle final de crop séparément.
- Rejeter pictogrammes/icônes/logos décoratifs non utiles comme preuves techniques.

3. Construire une couche intermédiaire auditable par document/page
Pour chaque page utile, produire des enregistrements structurés avec : document, révision, langue, page physique, section/titre, opération constructeur, phase, étape, texte exact, avertissement/note/spécification, références croisées, rectangles source, visuels retenus, visuels rejetés et motif, SHA/provenance.

4. Validation sémantique contrôlée
- Commencer par `RCL0193ENG` comme document pilote.
- Vérifier page par page les opérations/procédures/illustrations réelles contre le PDF source.
- Cas de référence obligatoire : p.53, opération `12.21.28`, `ENGINE CRANKSHAFT PRIMARY GEAR END-FLOAT`, valeur `0.089–0.165 mm`, contrôle par feeler gauges, vraie illustration mécanique retenue, icône d’en-tête rejetée.
- Aucun passage au corpus complet tant que le document pilote n’est pas propre.

5. Étendre par lots au corpus complet
- Traiter les 47 PDF par lots contrôlés.
- Après chaque lot : vérifier pages, texte, opérations, étapes, visuels, exclusions, géométrie, provenance et absence de mélange entre documents.
- Toute anomalie de moteur est corrigée puis le lot est rejoué avant acceptation.

6. Assemblage SQLite seulement après validation de l’extraction
- Construire une nouvelle base à partir de la couche intermédiaire validée.
- Liens explicites : document -> révision -> langue -> page -> opération -> phase -> étape -> texte -> visuels associés.
- Contrôles : `PRAGMA integrity_check=ok`, `foreign_key_check=0`, unicité des clés, absence de collisions document/page/langue, conformité des SHA et reproductibilité byte-identical si possible.

7. Audit des images avant DLL
- Générer inventaire visuel global avec dimensions, bbox source, crop bbox, page, opération/étape liée et statut retenu/rejeté.
- Contrôle visuel par échantillons + cas critiques.
- Aucun pictogramme d’en-tête ne doit être présent comme illustration technique.

8. Reconstruire la DLL après validation SQLite
- Nouveau contrat orienté provenance structurée.
- La DLL doit retourner directement document, révision, langue, page, opération/section, extrait, IDs de visuels pertinents et provenance.
- Pas de reconstruction de provenance dans MEMS Manager à partir de chaînes texte.
- Tests DLL indépendants de Qwen/MEMS Manager sur plusieurs questions réelles.

9. Validation documentaire sans IA
- Question -> résultat base/DLL -> bonne page/opération/valeur/image, sans Qwen.
- Cas pignon primaire obligatoire, plus batterie, papillon et plusieurs cas hors RCL0193ENG.
- Aucun faux positif de sous-chaîne type `axial` -> `coaxial`.

10. Retour dans MEMS Manager uniquement après tout ce qui précède
- Repartir du baseline applicatif BUILD #101.
- Intégrer la nouvelle DLL + nouvelle SQLite + nouveau catalogue visuel validé sur branche temporaire.
- Tests IA et UI seulement après validation de la couche documentaire.
- Aucun build production suivant sans validation et autorisation explicites.

### Jalons de validation
J0 sources figées ; J1 moteur RAVEMEMS V2 corrigé ; J2 RCL0193ENG pilote validé ; J3 corpus complet extrait et audité ; J4 SQLite validée ; J5 images validées ; J6 DLL validée sans IA ; J7 intégration temporaire MEMS Manager ; J8 seulement alors décision de nouveau build production.

### PROCHAINE ACTION EXACTE
Créer une branche temporaire dédiée à cette nouvelle fondation RAVEMEMS V2 à partir du moteur de référence, sans toucher MEMSX64. Première tâche : produire un audit géométrique sur plusieurs pages RAVE afin de fixer précisément la frontière d’en-tête image, la nouvelle marge de crop et les règles de regroupement vectoriel, puis journaliser ces paramètres avant toute pousse du moteur corrigé.
<!-- journal-entry-sha256:0c86238c48810d7afc712ed8dfc8fc777bafec813f00091d8aa6492529be2b55 -->

## RAVEMEMS — planning maître révisé

Décision utilisateur : avant toute nouvelle extraction, ne pas supposer que les 47 PDF sont 47 contenus techniques indépendants. Plusieurs peuvent être le même manuel dans plusieurs langues. Il faut éviter d'extraire 2, 3 ou 5 fois le même contenu et les mêmes images.

### Baseline applicatif
- Reprise future de MEMS Manager depuis BUILD #101, dernier socle avant l'intégration documentaire massive de #102+.
- Aucun BUILD #106 pendant la reconstruction documentaire.

### Planning officiel

1. INVENTAIRE ET REGROUPEMENT DES PDF RAVE
- Examiner les PDF présents dans le corpus RAVE.
- Regrouper les documents qui représentent le même contenu technique dans des langues différentes.
- Distinguer au contraire les éditions, années, variantes, bulletins et manuels réellement différents.
- Pour chaque groupe multilingue, choisir un PDF maître pour l'extraction technique complète, probablement l'anglais quand c'est le meilleur document, mais le choix doit être vérifié groupe par groupe.
- Ne pas extraire plusieurs fois les mêmes images et procédures uniquement parce qu'elles existent dans plusieurs langues.
- Une autre langue n'est extraite comme contenu technique distinct que si elle contient réellement une procédure, une variante, une page, un schéma, une valeur ou une révision différente.

2. CORRIGER RAVEMEMS V2 AVANT L'EXTRACTION
- RAVEMEMS V2 devient d'abord un moteur d'extraction fidèle, pas un moteur qui devine seul le sens documentaire.
- Conserver l'extraction du texte, des pages et de la géométrie.
- Pour la capture d'images uniquement : exclure la zone d'en-tête, car les icônes inutiles sont en haut de page. Cette exclusion ne doit pas supprimer le texte d'en-tête du reste de l'extraction.
- Réduire la marge de crop actuellement trop grande.
- Empêcher le regroupement abusif de dessins vectoriels distincts.
- Conserver séparément la zone source et la zone de crop finale.

3. PREMIER DOCUMENT PILOTE
- Commencer par RCL0193ENG ou par le PDF maître correspondant après regroupement.
- Contrôler directement le texte, les opérations, procédures, valeurs et images utiles.
- Test obligatoire : procédure du jeu axial du pignon primaire, opération 12.21.28, valeur 0.089–0.165 mm, contrôle aux cales, vraie illustration mécanique conservée, icône d'en-tête rejetée.

4. COUCHE INTERMÉDIAIRE VALIDÉE
- Avant SQLite, produire une représentation structurée et lisible du contenu réellement retenu : section/opération, procédure, étapes, valeurs, notes et images utiles.
- Le but est de pouvoir vérifier le résultat avant de l'enfermer dans la base.

5. EXTRACTION DES DOCUMENTS TECHNIQUEMENT UNIQUES
- Appliquer la méthode validée au reste des PDF maîtres et aux documents réellement distincts.
- Ne pas répéter le travail pour de simples traductions du même manuel.
- Vérifier chaque lot avant le suivant.

6. CONTRÔLE DES IMAGES
- Vérifier que les icônes d'en-tête ne sont plus capturées.
- Vérifier que les vraies illustrations ne sont ni coupées ni entourées d'une marge excessive.
- Vérifier les schémas vectoriels et leur regroupement.
- Une illustration technique identique entre plusieurs langues doit être stockée une seule fois.

7. CONSTRUCTION SQLITE
- Construire SQLite seulement après validation de l'extraction et des images.
- La base doit contenir uniquement les informations nécessaires pour retrouver correctement procédures, valeurs et visuels utiles.
- Éviter les duplications linguistiques inutiles.

8. CONSTRUCTION DE LA DLL
- Construire la DLL à partir de cette nouvelle base propre.
- La DLL doit retrouver directement les bonnes informations et les bons visuels sans heuristiques spécifiques dans MEMS Manager.

9. TESTS SANS QWEN / SANS MEMS MANAGER
- Tester d'abord SQLite + DLL seules.
- Une question doit retrouver la bonne procédure, la bonne valeur et le bon visuel avant toute reformulation IA.
- Utiliser plusieurs cas de référence sur plusieurs documents.

10. RETOUR DANS MEMS MANAGER
- Repartir du BUILD #101.
- Intégrer la nouvelle base, la DLL et les visuels sur une branche temporaire.
- Tester les réponses utilisateur finales dans les langues de MEMS Manager.
- La langue de réponse est gérée à la fin ; elle ne doit pas imposer de dupliquer toute l'extraction technique.
- Aucun build production suivant sans validation complète et autorisation explicite.

### PROCHAINE ACTION EXACTE
Faire l'inventaire du corpus RAVE et identifier les groupes de PDF correspondant au même contenu technique dans des langues différentes, afin de déterminer quels PDF maîtres devront réellement être extraits avant de modifier RAVEMEMS V2.
<!-- journal-entry-sha256:60112709984096e81a8dd1f73aa6c507aeb35a13cfc66c51184d538ced7c3e88 -->

# RAVE source inventory from user-supplied rave(6).zip

## Scope
Inventory only. No RAVEMEMS code change, no SQLite rebuild, no DLL rebuild, no MEMS Manager change.

## Source result
The user-supplied archive `rave(6).zip` contains exactly 47 PDF files totaling 1,359 pages.

Breakdown:
- `rave/Mini Tech Bulletins`: 35 PDFs / 73 pages
- `rave/general/testbook`: 2 PDFs / 216 pages
- `rave/library`: 1 PDF / 1 page
- `rave/xn`: 9 PDFs / 1,069 pages

## Language finding
The corpus is not five full copies of the same manuals in five languages. The PDFs are overwhelmingly English technical documents. Several cover pages print document titles in multiple languages, but their body content is English.

Examples:
- `wmxn990e.pdf`: MINI Workshop Manual, publication RCL0193ENG, English body text.
- `cdxn990e.pdf`: Electrical Circuit Diagrams, RCL0194ENG, English body text.
- `roxn990e.pdf`: Repair Operation Times, RCL0195ENG, English body text.
- `elxn970e.pdf`: Electrical Library, RCL0213ENG, English body text.
- `tb12212e.pdf`: TestBook 1 User Manual, RCL0221ENG, English body text.
- `tb22382e.pdf`: TestBook 2 User Manual, RCL0238ENG, English body text.
- `hr01795e.pdf`: Owner's Handbook, RCL0179ENG, English body text.
- `Hr01791x.pdf`: separate RCL0179ENX handbook variant/edition, English body text; not merely a French/German/etc duplicate.

Therefore the new extraction must NOT multiply the technical extraction by application output language. Technical content and images are extracted once from the actual source document; final answer language is handled later.

## Duplicate finding
No exact duplicate PDFs/text bodies were found in the 47-file source set. Apparent same-topic files can be distinct technical publications and must not be collapsed solely by title. Example: `R6255BU.PDF` and `X6255BU.PDF` both concern wiring harness connectors but are different bulletins with different dates/scope/content.

## Proposed corpus classification
### Keep as technical/source knowledge
All substantive vehicle/service/diagnostic documents, including:
- Workshop Manual RCL0193ENG (`wmxn990e.pdf`)
- Electrical Circuit Diagrams RCL0194ENG (`cdxn990e.pdf`)
- Repair Operation Times RCL0195ENG (`roxn990e.pdf`)
- Electrical Library RCL0213ENG (`elxn970e.pdf`)
- Maintenance Check Sheet RCL0225ENG (`mcxn960e.pdf`)
- Paint Refinishing Times combined English publication (`prxn990e.pdf`)
- TestBook manuals RCL0221ENG and RCL0238ENG
- both owner-handbook variants RCL0179ENG / RCL0179ENX
- In-Car Entertainment manual (English)
- substantive Mini Technical Bulletins

### Exclude from the vehicle-answer knowledge index
These files are RAVE/CD/navigation/support metadata rather than useful vehicle technical procedures:
- `rave/library/libxn.pdf`: blank one-page Mini Library shell
- `rave/Mini Tech Bulletins/Xndb2eng.pdf`: one-page multilingual Technical Bulletins menu/index
- `rave/Mini Tech Bulletins/X8386BU.PDF`: RAVE Technical Bulletins CD introduction
- `rave/Mini Tech Bulletins/X8408BU.PDF`: RAVE Technical Bulletins CD version information
- `rave/Mini Tech Bulletins/X8417BU.PDF`: maintaining RAVE CD updates
- `rave/Mini Tech Bulletins/RT0003BU.PDF`: TestBook Help Desk reporting/support-process bulletin; not vehicle repair knowledge

This yields 41 substantive PDFs proposed for technical extraction and 6 metadata/support PDFs excluded from the end-user technical answer corpus.

## Consequence for the revised plan
1. The language-duplication concern is resolved at source: this RAVE archive does not contain five complete language copies to extract repeatedly.
2. The technical extraction should operate once on the substantive English source set.
3. RAVEMEMS V2 must next be corrected before any new full extraction: image-header exclusion only for image capture, reduced crop margin, correct vector grouping, faithful text/procedure extraction.
4. Start validation with `wmxn990e.pdf` / RCL0193ENG, including the known primary-gear end-float page as a mandatory reference case.
5. No SQLite or DLL rebuild until the corrected extraction and images are validated.

## Production state
MEMS Manager production remains frozen. No BUILD #106 is authorized or launched. Application reintegration baseline remains BUILD #101 after the documentary stack is rebuilt and validated independently.
<!-- journal-entry-sha256:cb3987bf47f73c52995f240119b47d09255b08da0b35e317d4129426a10a7a59 -->

# RAVEMEMS V2 visual header rule refined from real source pages

## Context
After inventorying the user-supplied `rave(6).zip`, real PDF pages were inspected before any RAVEMEMS V2 code change.

## Confirmed RCL0193ENG behavior
In `rave/xn/wmxn990e.pdf` (Workshop Manual RCL0193ENG), physical PDF page 53 contains:
- the repeated ENGINE header icon at approximately y=17.7..58.0 pt;
- the main horizontal header separator around y=57..58 pt;
- the genuine primary-gear technical illustration much lower on the page, source rectangle approximately x=63.9..252.0, y=210.6..502.5 pt.

This confirms that the workshop-manual header/icon must be excluded from IMAGE capture only. Text extraction remains unchanged and continues to read the full page.

## Important corpus-wide exception
A blanket top-of-page image exclusion cannot be applied to all 47 source PDFs.

Example: `rave/Mini Tech Bulletins/R3591BU.PDF`, page 5, contains genuine technical illustrations beginning near y=35 pt and occupying the top half of the page. Rejecting all image candidates in a fixed top band would destroy valid technical diagrams.

Therefore the image-header exclusion must be document/layout aware:
- for RAVE manual pages with the standard repeated header/icon layout, reject header visuals before capture and prevent crop margin from re-entering the header;
- for technical bulletins/pages without that standard header, do not reject a genuine large technical illustration merely because it starts high on the page.

## Crop margin finding
Current RAVEMEMS V2 code uses 24 pt on left/top/right and 32.4 pt below (`margin * 1.35`). This is excessive.

On the RCL0193ENG p53 reference illustration:
- 24 pt includes neighboring procedure text above/below;
- 8 pt keeps the complete technical drawing and its figure identifier without neighboring procedure lines;
- 4 pt also keeps the complete drawing on this reference page.

No final global margin value is fixed from one page alone. The next pilot must test a small representative set before choosing the generic margin or adaptive rule.

## Code status
No RAVEMEMS V2 technical correction has been pushed yet. Current audited source remains `tmp-ravemems-v2-language-fix` commit `4942f1e7bd11bddf3c0f4cf9bcc5cbe0b6e11c4d`.

## Next exact action
Create a new isolated RAVEMEMS V2 foundation branch from the audited V2 engine, implement only the pilot visual-capture correction for RCL0193ENG first, and test it against real pages before extending rules to the rest of the corpus. Do not rebuild SQLite/DLL/MEMS Manager yet.
<!-- journal-entry-sha256:0d54c2e7cd3672ea19b70ae3441648d724dc0d6a9f9ab989f00c97d7ed224609 -->

## RAVEMEMS V2 — PRE-POUSSE PILOTE VISUEL RCL0193ENG

Décision avant toute modification technique sur `tmp-ravemems-v2-source-rebuild`.

### Périmètre
- Pilote uniquement : `RCL0193ENG` / `rave/xn/wmxn990e.pdf`.
- Aucun changement MEMSX64, protocole, acquisition, RAM, écriture ECU, SQLite runtime ou DLL.
- L'extraction du texte et la lecture de l'en-tête restent inchangées. L'exclusion d'en-tête concerne uniquement les candidats visuels.

### Constats directs sur le PDF source
- Page physique 53 : icône ENGINE raster dans l'en-tête, bbox env. `[471.8,17.7,552.4,58.0]`.
- Page physique 53 : vraie illustration mécanique raster, bbox env. `[63.9,210.6,252.0,502.5]`.
- Le rectangle raster source exact contient déjà l'illustration complète et son repère `12M3440` ; aucune marge de crop n'est nécessaire pour ce candidat.
- Les petits pictogrammes de sections (ENGINE, batterie, freins, SRS, etc.) sont des ressources raster répétées de nombreuses fois dans le document et expliquent une part importante des faux visuels historiques.

### Correctif pilote à pousser
1. Ajouter une politique visuelle séparée du parseur de texte.
2. Pour le profil Workshop Manual RCL0193ENG, rejeter un candidat raster entièrement contenu dans la zone d'en-tête utilisée par la géométrie de lecture (`7.5 %` de la hauteur de page).
3. Rejeter les petits raster décoratifs/pictogrammes réutilisés au moins 5 fois dans le même document, avec garde de petite taille/aire afin de ne pas supprimer une grande illustration technique réutilisée.
4. Pour les raster retenus, utiliser le `source_bbox` exact comme `crop_bbox` : marge raster = 0. La marge historique 24 pt / 32.4 pt ne doit plus être appliquée à ces images.
5. Conserver séparément `source_bbox` et `crop_bbox` dans les données, même s'ils sont égaux pour le raster exact.
6. Ne pas appliquer cette exclusion d'en-tête globalement aux bulletins techniques : certains bulletins possèdent de vraies illustrations près du haut de page.
7. Le fallback vectoriel n'est pas validé par ce pilote RCL0193ENG (les pages visuelles du manuel sont raster). Il reste à traiter/tester séparément avant le corpus complet.

### Tests obligatoires du pilote
- extraction complète RCL0193ENG seulement ;
- intégrité SQLite/FK du résultat d'extraction ;
- p.53 : l'icône ENGINE ne doit pas être créée comme visuel ;
- p.53 : exactement la vraie illustration mécanique doit rester ;
- son `source_bbox` doit être égal au `crop_bbox` ;
- le crop p.53 ne doit contenir ni texte voisin ni icône d'en-tête ;
- les pictogrammes répétitifs connus doivent être absents des visuels retenus ;
- les procédures/étapes texte de p.53 doivent rester inchangées, notamment opération `12.21.28`, contrôle aux cales et valeur `0.089 to 0.165 mm`.

Après le run, rapport immédiat avant toute autre pousse.
<!-- journal-entry-sha256:06145ed26e66cb880572b3893fb74dc21040fefda2de35c75f279bc8674cce0d -->
