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
