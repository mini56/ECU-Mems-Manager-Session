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
