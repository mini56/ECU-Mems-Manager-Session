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
