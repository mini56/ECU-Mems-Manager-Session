# RCL0193FRE — LOT 1790 — PRE-POUSSE V5

Date : 2026-08-29

## Etat à préserver

- Production `MEMSX64` vérifiée avant pousse : BUILD #101, HEAD `22dbe75ed14e0a61e694159d505ef72245116b48`.
- Branche documentaire cible unique : `tmp-rave-visual-backfill`.
- HEAD documentaire avant pousse : `dfb5b8525b1d5685d1070914eb78c64f30aa7ff6`.
- Aucun changement ECU/protocole/UI/IA/build de production.

## Source

- Document utilisateur : `Manuel Rover MPI.pdf`.
- Publication identifiée : `RCL0193FRE`, manuel atelier Mini français, 5e édition.
- PDF source : 371 pages, 67 009 217 octets.
- SHA-256 source : `0c7fef28d0d0f0673ba321d6625a019c005823103caa98afb3258114e1fec713`.
- Couche texte décodée sans OCR approximatif.

## Lot 1790 final préparé

Premier bloc prioritaire :
- Information / réglages / couples / fluides : PDF 34-49 ;
- Gestion moteur MEMS : PDF 104-135 ;
- Système d’alimentation : PDF 136-145.

Pages utiles retenues : **52**. Les pages blanches/intercalaires ne sont pas conservées.

Contenu structuré V5 :
- `mems_applicability_scope` : +4 portées RCL0193FRE ;
- connaissances : **429** ;
- liaisons connaissance-portée : **429** ;
- spécifications : **350** ;
- valeurs de spécifications : **351** ;
- procédures/phases : **51** pour **27 opérations constructeur** ;
- étapes ordonnées : **344** ;
- exigences/avertissements/outils/notes : **50** ;
- relations : **379** ;
- alias : **51**.

## Corrections de portée avant pousse

- PDF 38 : portée explicite **SPi boîte manuelle**.
- PDF 39 : portée explicite **SPi boîte automatique**.
- PDF 40 : portée explicite **MPi**.
- Bloc gestion moteur PDF 104-135 : portée **MPi**.
- Bloc alimentation PDF 136-145 : portée **MPi**.
- Les tableaux généraux de couples restent Mini génériques lorsque le document ne donne pas une portée MPi.
- Aucune génération MEMS (`1.6`, `1.9`, etc.) n’est inventée à partir du manuel : `mems_family` reste NULL dans les nouvelles portées tant que la source ne la nomme pas explicitement.

## Contrôles particuliers conservés

- Culasse : séquence constructeur structurée en deux étapes : `34 N.m`, puis `34 N.m de plus`.
- PDF 45 : doublon imprimé « Boulon d’ancrage supérieur de ceinture avant » à **32 N.m** et **30 N.m** conservé comme conflit constructeur à vérifier ; aucune valeur choisie arbitrairement.
- Dépose/repose appairées et ordre constructeur conservé.

## Validation locale avant pousse

Candidat reconstruit avec tous les lots antérieurs :
- `PRAGMA integrity_check = ok` ;
- `PRAGMA user_version = 20` ;
- faits historiques `mems_rave_fact` : **93**, inchangés ;
- faits historiques `mems_expert_fact_external` : **105**, inchangés ;
- aucune connaissance RCL0193FRE sans portée ;
- aucune table historique modifiée ;
- `research_enrichment_1790.qz64` : 84 037 octets, SHA-256 `165d3bedb57b38b7fa1550a4360c9f484828209beea51b63b4aab20c5f9885da` ;
- SQL décompressé : 828 045 octets, SHA-256 `28f0ffc78b957679672608731f0cb0faa3487d1646977304423e23d701465725`.

## Autorisation technique suivante

Pousser uniquement le lot documentaire 1790 sur `tmp-rave-visual-backfill`, faire reconstruire et revalider la base depuis les octets committés, vérifier que `MEMSX64` est toujours #101, journaliser le résultat immédiatement, puis poursuivre l’extraction des sections restantes de RCL0193FRE.
