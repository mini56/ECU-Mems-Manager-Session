# Rapport 34 — Bancs d'essai ECU MEMS : les deux pages d'Andrew Revill

**Enregistré le 23 novembre 2026 — sources externes, documentation uniquement.**  
**Auteur des pages et des schémas : Andrew Revill.** Les schémas et illustrations d'origine restent consultables sur son site ; ce document conserve leurs liens, leur identification et les distinctions techniques utiles à ECU MEMS Manager SESSION.

## 1. Les deux pages originales à conserver

**Page A — [Building a Minimal Bench Harness for a MEMS ECU](https://andrewrevill.co.uk/MinimalBenchHarness.htm)**  
Adresse permanente relevée : https://andrewrevill.co.uk/MinimalBenchHarness.htm

L'auteur présente les câblages *minimaux* permettant **l'alimentation du calculateur et sa communication par prise OBDII**, sans les voyants d'injecteurs, de bobines ni les circuits d'antidémarrage prévus pour son montage plus complet. La page contient **quatre schémas distincts**, identifiés ci-dessous.

| Schéma original | Génération ou calculateur réellement indiqué | Lien direct conservé |
|---|---|---|
| Minimal MEMS3 Bench Harness | **MEMS 3** | [Image originale — MEMS 3](https://andrewrevill.co.uk/MinimalBenchHarness_files/image025.png) |
| Minimal MEMS 2J Bench Harness | **MEMS 2J** | [Image originale — MEMS 2J](https://andrewrevill.co.uk/MinimalBenchHarness_files/image002.png) |
| Minimal MEMS 1.9 Bench Harness | **MEMS 1.9** | [Image originale — MEMS 1.9](https://andrewrevill.co.uk/MinimalBenchHarness_files/image029.png) |
| Minimal TD5 Bench Harness | **TD5** (cas distinct, hors MEMS essence) | [Image originale — TD5](https://andrewrevill.co.uk/MinimalBenchHarness_files/image008.png) |

**Constat important : aucun de ces quatre schémas n'est un schéma MEMS 1.6 SPi.** Ils ne doivent donc pas être utilisés tels quels pour le calculateur 1.6 du véhicule du projet. Les connecteurs, alimentation, masses et ligne de diagnostic doivent être pris sur la documentation spécifique à la génération et au boîtier.

**Page B — [Building a Key Programming & ECU Immobiliser Testing Interface](https://andrewrevill.co.uk/KeyProgrammer.htm)**  
Adresse permanente relevée : https://andrewrevill.co.uk/KeyProgrammer.htm

L'auteur détaille un banc **plus complet** réunissant ECU, antidémarrage, prise diagnostic/outil de programmation et, en option, un simulateur des signaux vilebrequin/arbre à cames. Il distingue :
- une version simple avec alimentation continue **12 V**, commutateur simulant le contact, masses, prise OBDII et connexions ECU–antidémarrage ;
- une version évoluée avec indicateurs des sorties injecteurs, bobines et pompe, plus les raccordements pour un simulateur vilebrequin/arbre à cames ;
- un câblage initialement conçu pour **EU3 / MEMS 3**, puis adapté par l'auteur à **EU2 / MEMS 2J et MEMS 1.9** au moyen de connecteurs ou d'adaptateurs spécifiques ;
- la différence entre des **voyants de commande** prouvant l'activité de certaines sorties et le **fonctionnement véritable d'un moteur**, lequel n'est pas recréé sur ce banc.

### Observation à ne pas généraliser

Dans la page B, Andrew Revill relate, pour **son montage** et ses essais d'antidémarrage, que la suppression de l'injection peut coexister avec la poursuite des commandes d'allumage tant qu'un signal vilebrequin est présent. Il précise que la pompe peut continuer tant que ce signal demeure. **Observation propre au dispositif/ECU testé**, non une loi universelle pour toutes les générations et sans conclusion de brochage sur MEMS 1.6.

## 2. Pourquoi ces références sont utiles à notre projet

- **MEMS 1.9, 2J et 3 :** sources de comparaison pour une éventuelle réalisation de banc, pour le câblage d'alimentation, le contact simulé et la ligne de communication. **Aucune validation du protocole ni connexion réelle n'est acquise par la seule présence des schémas.**
- **Lucas 5AS et clés :** la page B est une référence de contexte pour le matériel de banc et les tests d'antidémarrage ; elle ne remplace pas nos études séparées des commandes 5AS et de leurs limites.
- **MEMS 1.6 SPi du véhicule :** **non couvert** par les quatre schémas minimaux. Pour ses mesures réelles de sonde lambda, de temps de charge bobine et de température, les relevés doivent venir de la voiture moteur tournant, avec le contexte d'utilisation documenté.
- **Diagnostic automatique :** un banc uniquement alimenté, sans combustion ni vraie sonde lambda chaude, **ne peut pas confirmer** la plage physique de la lambda ou la durée normale de charge d'une bobine en fonctionnement.

## 3. Précautions d'interprétation et de réalisation

1. Relever la **référence exacte du calculateur et la version du connecteur** avant de choisir l'un des plans. L'ordre des broches dépend de la vue et du connecteur ; ne jamais déduire une broche simplement de la famille MEMS.
2. Sur un banc physique, vérifier hors tension la polarité, les masses, la continuité et l'absence de court-circuit ; employer une alimentation 12 V **protégée et à courant limité**, adaptée au montage.
3. Ne pas confondre une liaison **USB–TTL 5 V** des MEMS 1.6 à prise trois broches avec l'interface **KKL automobile / OBD** d'autres générations.
4. La page B comprend des fonctions d'antidémarrage et de programmation ; **ne pas transposer ces actions** au Manager sans analyse distincte et validation explicite.
5. Les schémas sont la propriété de leur auteur. **Références et liens directs conservés, sans redistribution d'images, de fichiers projet ni de firmware** dans le dépôt.

## 4. Raccordement à notre documentation existante

- [Rapport 22 — Câbles RC5 / 5AS / MEMS 1.6 et distinction TTL–KKL](22_GUIDE_FABRICATION_CABLES_RC5_5AS_PLIP_20261121.md)
- [Rapport 25 — Sources Andrew Revill et limites des captures 5AS](25_JOURNAL_ANDREW_REVILL_5AS_AUDIT_ET_TRACE_A_RECHERCHER_20261121.md)
- [Étude des conversions lambda et bobine](../VERIFICATION_LAMBDA_BOBINE_CONVERSIONS_20261123.md)
- [Sommaire de la recherche MEMS 1.9/2J/3](README.md)

**Décision : archivage de références et comparaison documentaire seulement.** Aucun changement de `ECU MEMS Manager SESSION`, de `mems_manager_x64.dll`, du protocole, de la base, du Setup, de la publication MAJ ou des versions de programme.
