# SCHÉMA EXACT — SOCLE RAVE COMPLET ET ÉVOLUTIF

> Projet : ECU MEMS Manager
> État de départ : `MEMSX64` #95 / v1.0.95, commit `f2e97b3e3a432785e159d30bbeca7b7bef2fdcb4`.
> Branche de travail autorisée : `tmp-rave-knowledge-foundation`.
> Ce document est une spécification de migration. Il ne constitue pas encore une modification de la SQLite.

## 1. BUT

Construire un noyau de connaissances capable d’absorber progressivement **tout RAVE** : ECU/protocole, électricité, câblage, capteurs/actionneurs, diagnostic, mécanique, couples de serrage, réglages, tolérances, capacités, fluides, procédures de dépose/repose, outils, précautions, contrôles et illustrations.

La structure doit résoudre deux problèmes distincts :
1. **ce que dit la documentation** ;
2. **à quelle variante véhicule/moteur cette information s’applique**.

Ces deux dimensions ne doivent plus être confondues dans une chaîne libre telle que `SPi_Japan_97MY_from_VIN_...`.

## 2. PRINCIPES NON NÉGOCIABLES

- Migration **additive** : ne supprimer ni renommer `mems_rave_fact`, `mems_expert_fact_external`, `ecu_fitment` ou les autres tables existantes pendant la transition.
- Les 93 faits RAVE #95 restent la référence brute/auditable.
- `variant` reste conservé pour compatibilité et traçabilité, mais cesse progressivement d’être la seule source de portée.
- Une absence d’information dans la source est stockée comme **NULL / non précisé**, jamais comme « toutes variantes ».
- Une universalité explicitement écrite par Rover doit être représentée explicitement (`any`), et non déduite d’un NULL.
- Un fait incompatible avec une variante demandée doit pouvoir être éliminé **avant Qwen**.
- Un fait de marché non précisé peut rester candidat avec un rang inférieur ; il ne doit pas être traité comme « tous marchés ».
- Toute donnée normalisée doit conserver un lien vers sa preuve brute et sa source.
- Les sources constructeur, projet décodé, recoupé et source externe restent distinguées via `verification_level`.
- Ne pas déduire `Cooper`, UK, Europe, Japon, boîte ou équipement si la source ne le dit pas.
- Ne pas augmenter `PRAGMA user_version` uniquement parce que de nouvelles tables sont ajoutées. L’évolution de révision sera décidée séparément si un lecteur runtime incompatible l’exige.

## 3. AUDIT #95 QUI JUSTIFIE LE SCHÉMA

SQLite réelle de l’artefact #95 :
- 64 tables au total en comptant la table SQLite interne ; 63 tables métier ;
- 93 lignes `mems_rave_fact` ;
- 105 lignes `mems_expert_fact_external` ;
- `PRAGMA integrity_check = ok` ;
- `PRAGMA user_version = 20`.

`mems_rave_fact` possède actuellement : `fact_key, source_key, document, variant, topic, statement, source_section, verification_level, image_ref, notes`.

`mems_expert_fact_external` possède principalement : `source_key, fact_key, family, firmware_code, topic, statement, verification_level, notes`.

`ecu_fitment` possède déjà des axes utiles (`injection, make, model, variant, transmission, engine, market, year_from, year_to, vin_from, vin_to`), mais reste une table de compatibilité ECU et ne peut pas représenter à elle seule une procédure mécanique ou un couple de serrage.

Aucune table dédiée `procedure`, `torque`, `mechanical`, `tool` ou `warning` n’existe actuellement. Pourtant des faits RAVE existants contiennent déjà des données mécaniques telles que CKP 6/3 Nm, bobine 10 Nm, ECT 15 Nm, IACV 7 Nm, IAT 7 Nm, MAP 6 Nm, boîtier papillon 8 Nm, pédale 25 Nm, TPS 1,5 Nm, ainsi que des instructions de dépose/repose.

## 4. TABLE 1 — `mems_applicability_scope`

Une ligne représente une portée réutilisable. Elle n’est pas liée uniquement à un ECU.

```sql
CREATE TABLE mems_applicability_scope (
    scope_key TEXT PRIMARY KEY,
    scope_kind TEXT NOT NULL,
    make TEXT,
    model TEXT,
    engine_family TEXT,
    engine_code TEXT,
    displacement_cc INTEGER,
    engine_variant TEXT,
    induction TEXT,
    mems_family TEXT,
    transmission TEXT,
    year_from INTEGER,
    year_to INTEGER,
    vin_from TEXT,
    vin_to TEXT,
    market TEXT,
    compression_variant TEXT,
    catalyst_state TEXT,
    air_conditioning_state TEXT,
    source_scope_text TEXT,
    notes TEXT
);
```

### Sémantique

`scope_kind` : `vehicle`, `engine`, `system`, `general`.

Valeurs communes :
- `induction` : `carburettor`, `SPi`, `MPi`, NULL si non précisé ;
- `transmission` : `manual`, `automatic`, NULL si non précisé ;
- `market` : valeur explicite telle que `Japan`, `UK`, `Europe`, NULL si non précisé ;
- `catalyst_state`, `air_conditioning_state` : `required`, `excluded`, NULL si non précisé.

**NULL ne signifie jamais ANY.**

## 5. TABLE 2 — `mems_scope_constraint`

Cette table couvre les cas que les colonnes communes ne peuvent pas exprimer proprement : exclusion, « all other vehicles », plage inhabituelle, équipement particulier ou futurs axes RAVE.

```sql
CREATE TABLE mems_scope_constraint (
    scope_key TEXT NOT NULL,
    dimension TEXT NOT NULL,
    operator TEXT NOT NULL,
    value_text TEXT,
    value_to_text TEXT,
    value_num REAL,
    value_to_num REAL,
    unit TEXT,
    source_text TEXT,
    PRIMARY KEY (scope_key, dimension, operator, value_text, value_num),
    FOREIGN KEY (scope_key) REFERENCES mems_applicability_scope(scope_key)
);
```

Opérateurs autorisés au départ : `eq`, `neq`, `gte`, `lte`, `between`, `contains`, `any`.

Exemples :
- Japon uniquement : `dimension=market, operator=eq, value_text=Japan` ;
- « All other vehicles » dans une section séparée Japon : `dimension=market, operator=neq, value_text=Japan` ;
- à partir d’un VIN : `dimension=vin, operator=gte, value_text=...` ;
- toutes transmissions explicitement indiquées : `dimension=transmission, operator=any`.

Cette table est le mécanisme d’extension : une nouvelle discrimination RAVE ne doit pas obliger à refaire tout le schéma.

## 6. TABLE 3 — `mems_knowledge_item`

Table centrale : une connaissance atomique, quelle que soit sa nature.

```sql
CREATE TABLE mems_knowledge_item (
    knowledge_key TEXT PRIMARY KEY,
    domain TEXT NOT NULL,
    knowledge_type TEXT NOT NULL,
    topic TEXT NOT NULL,
    component_key TEXT,
    source_key TEXT,
    document TEXT,
    source_section TEXT,
    verification_level TEXT NOT NULL,
    legacy_rave_fact_key TEXT UNIQUE,
    source_text TEXT,
    image_ref TEXT,
    notes TEXT
);
```

### `domain` initial

- `ecu_protocol`
- `electrical`
- `sensor_actuator`
- `diagnostic`
- `mechanical`
- `service`
- `fluids_consumables`
- `documentation`

### `knowledge_type` initial

- `fact`
- `wiring`
- `specification`
- `torque`
- `adjustment`
- `capacity`
- `procedure`
- `inspection`
- `warning`
- `tool_requirement`
- `fluid`

La combinaison domain/type évite de créer une table pour chaque thème tout en permettant un routage déterministe.

`source_text` conserve le texte technique/source utile. Pour les 93 faits existants, `legacy_rave_fact_key` doit permettre de retrouver exactement la ligne `mems_rave_fact` d’origine.

## 7. TABLE 4 — `mems_knowledge_scope`

Relation plusieurs-à-plusieurs entre une connaissance et ses portées.

```sql
CREATE TABLE mems_knowledge_scope (
    knowledge_key TEXT NOT NULL,
    scope_key TEXT NOT NULL,
    applicability TEXT NOT NULL DEFAULT 'applies',
    PRIMARY KEY (knowledge_key, scope_key),
    FOREIGN KEY (knowledge_key) REFERENCES mems_knowledge_item(knowledge_key),
    FOREIGN KEY (scope_key) REFERENCES mems_applicability_scope(scope_key)
);
```

`applicability` initial : `applies`. Les exclusions doivent de préférence être exprimées comme contraintes de portée plutôt que comme faits négatifs dupliqués.

Un même couple ou une même procédure peut ainsi viser plusieurs véhicules sans dupliquer le contenu.

## 8. TABLE 5 — `mems_specification`

En-tête d’une valeur structurée : couple, jeu, pression, résistance, tension, capacité, température, tolérance, limite, etc.

```sql
CREATE TABLE mems_specification (
    spec_key TEXT PRIMARY KEY,
    knowledge_key TEXT NOT NULL,
    component_key TEXT,
    target_key TEXT,
    operation TEXT,
    parameter TEXT NOT NULL,
    default_unit TEXT,
    condition_text TEXT,
    sequence_ref TEXT,
    notes TEXT,
    FOREIGN KEY (knowledge_key) REFERENCES mems_knowledge_item(knowledge_key)
);
```

Exemples :
- composant `cylinder_head`, opération `tighten`, paramètre `torque` ;
- composant `fuel_system`, paramètre `fuel_pressure` ;
- composant `cooling_system`, paramètre `capacity`.

## 9. TABLE 6 — `mems_specification_value`

Une spécification peut avoir une valeur simple, une plage ou plusieurs étapes de serrage.

```sql
CREATE TABLE mems_specification_value (
    spec_key TEXT NOT NULL,
    sequence_no INTEGER NOT NULL DEFAULT 1,
    value_numeric REAL,
    value_min REAL,
    value_max REAL,
    tolerance_minus REAL,
    tolerance_plus REAL,
    angle_deg REAL,
    value_text TEXT,
    unit TEXT,
    instruction_text TEXT,
    PRIMARY KEY (spec_key, sequence_no),
    FOREIGN KEY (spec_key) REFERENCES mems_specification(spec_key)
);
```

### Exemples représentables sans perte

- `15 Nm` : `value_numeric=15, unit=Nm` ;
- `1,0 bar ±4 %` : valeur + tolérance ou `value_text` si la forme constructeur doit rester exacte ;
- `20 Nm puis 90° puis 90°` : trois lignes `sequence_no=1..3`, les deux dernières utilisant `angle_deg` ;
- `0–1 V fermé / 4–5 V ouvert` : deux connaissances/spécifications conditionnées ou deux valeurs distinctes suivant la formulation source.

Il est interdit d’écraser une séquence constructeur multi-étapes en une seule valeur.

## 10. TABLE 7 — `mems_procedure`

En-tête d’une procédure de service.

```sql
CREATE TABLE mems_procedure (
    procedure_key TEXT PRIMARY KEY,
    knowledge_key TEXT NOT NULL,
    component_key TEXT NOT NULL,
    operation TEXT NOT NULL,
    paired_procedure_key TEXT,
    title_source TEXT,
    notes TEXT,
    FOREIGN KEY (knowledge_key) REFERENCES mems_knowledge_item(knowledge_key)
);
```

`operation` initial : `remove`, `install`, `disassemble`, `assemble`, `adjust`, `inspect`, `test`, `drain`, `fill`.

`paired_procedure_key` permet par exemple de relier « dépose culasse » à « repose culasse » sans les fusionner.

## 11. TABLE 8 — `mems_procedure_step`

Étapes ordonnées : l’ordre constructeur est une donnée et ne doit jamais être confié au LLM.

```sql
CREATE TABLE mems_procedure_step (
    procedure_key TEXT NOT NULL,
    step_no INTEGER NOT NULL,
    instruction_source TEXT NOT NULL,
    condition_text TEXT,
    figure_ref TEXT,
    related_spec_key TEXT,
    PRIMARY KEY (procedure_key, step_no),
    FOREIGN KEY (procedure_key) REFERENCES mems_procedure(procedure_key),
    FOREIGN KEY (related_spec_key) REFERENCES mems_specification(spec_key)
);
```

Une étape peut donc pointer vers le couple exact applicable plutôt que répéter une valeur en texte libre.

## 12. TABLE 9 — `mems_procedure_requirement`

Table flexible pour précautions, outils, consommables et contrôles associés à une procédure ou à une étape.

```sql
CREATE TABLE mems_procedure_requirement (
    procedure_key TEXT NOT NULL,
    step_no INTEGER,
    requirement_no INTEGER NOT NULL,
    requirement_type TEXT NOT NULL,
    requirement_source TEXT NOT NULL,
    part_number TEXT,
    quantity REAL,
    unit TEXT,
    figure_ref TEXT,
    PRIMARY KEY (procedure_key, step_no, requirement_no),
    FOREIGN KEY (procedure_key) REFERENCES mems_procedure(procedure_key)
);
```

`requirement_type` initial : `warning`, `prerequisite`, `tool`, `special_tool`, `consumable`, `lubricant`, `replacement_part`, `postcheck`, `note`.

Cela permet par exemple de conserver « toujours remplacer le joint », « utiliser l’outil Rover ... », ou un contrôle après repose sans inventer une nouvelle table à chaque fois.

## 13. TABLE 10 — `mems_knowledge_relation`

Relation générique entre connaissances.

```sql
CREATE TABLE mems_knowledge_relation (
    from_key TEXT NOT NULL,
    relation_type TEXT NOT NULL,
    to_key TEXT NOT NULL,
    notes TEXT,
    PRIMARY KEY (from_key, relation_type, to_key)
);
```

Relations initiales : `related_to`, `requires`, `uses_specification`, `supersedes`, `conflicts_with`, `cross_check`, `removal_pair`, `installation_pair`.

Cette table est importante pour conserver les divergences entre manuels/plages sans écraser une ancienne valeur.

## 14. TABLE 11 — `mems_term_alias`

Pour que « culasse », « cylinder head », « CKP », « capteur PMH » ou des variantes orthographiques convergent vers la même clé technique sans laisser Qwen deviner le composant.

```sql
CREATE TABLE mems_term_alias (
    entity_type TEXT NOT NULL,
    entity_key TEXT NOT NULL,
    language TEXT NOT NULL,
    alias TEXT NOT NULL,
    normalized_alias TEXT NOT NULL,
    PRIMARY KEY (entity_type, entity_key, language, normalized_alias)
);
```

`entity_type` peut viser `component`, `topic`, `operation`, `knowledge_type`.

Cette table servira au routage déterministe et aux six langues sans multiplier les colonnes par langue dans toutes les nouvelles tables.

## 15. INDEXES OBLIGATOIRES

```sql
CREATE INDEX idx_scope_vehicle ON mems_applicability_scope(make, model, year_from, year_to, market);
CREATE INDEX idx_scope_powertrain ON mems_applicability_scope(engine_family, engine_code, induction, transmission, mems_family);
CREATE INDEX idx_scope_constraint_lookup ON mems_scope_constraint(dimension, operator, value_text, value_num);
CREATE INDEX idx_knowledge_class ON mems_knowledge_item(domain, knowledge_type, topic);
CREATE INDEX idx_knowledge_component ON mems_knowledge_item(component_key);
CREATE INDEX idx_knowledge_legacy ON mems_knowledge_item(legacy_rave_fact_key);
CREATE INDEX idx_knowledge_scope_scope ON mems_knowledge_scope(scope_key, knowledge_key);
CREATE INDEX idx_spec_lookup ON mems_specification(component_key, operation, parameter);
CREATE INDEX idx_procedure_lookup ON mems_procedure(component_key, operation);
CREATE INDEX idx_alias_lookup ON mems_term_alias(language, normalized_alias);
```

## 16. RÈGLE DE COMPATIBILITÉ — TROIS ÉTATS

Le filtre de portée ne doit pas être binaire. Pour chaque dimension connue de la question :

1. **MATCH EXACT** : même valeur/plage ou contrainte explicite compatible ;
2. **UNKNOWN** : la source ne précise pas cette dimension ; le fait peut rester candidat mais doit être moins bien classé ;
3. **INCOMPATIBLE** : valeur/contrainte explicitement opposée ; le fait est éliminé avant grounding/Qwen.

Exemple : question « Mini SPi Europe 1995 » :
- fait SPi Europe 1993–96 = exact ;
- fait SPi marché non précisé 1993+ = unknown sur marché, donc candidat secondaire ;
- fait SPi Japan-only 97MY = incompatible marché + année, donc exclu ;
- fait MPi = incompatible injection, donc exclu.

## 17. MIGRATION DES 93 FAITS RAVE #95

La migration initiale doit être **reproductible et non destructive**.

### Étape A — tables seulement

Créer les 11 tables + indexes via un nouveau lot `research_enrichment_1730.qz64` temporaire. Ne modifier aucune ligne historique.

### Étape B — miroir des 93 faits

Créer exactement 93 `mems_knowledge_item`, chacun relié par `legacy_rave_fact_key` à son `fact_key` d’origine. Le texte `statement`, source, document, section et niveau de vérification doivent rester traçables sans altération.

### Étape C — portées existantes

Les variantes #95 sont seulement huit formes distinctes :
- `SPi_Japan_97MY_from_VIN_SAXXNNAXKBD_134455` — 40 faits ;
- `Mini_1997_2000` — 26 ;
- `MPi_97MY_from_VIN_SAXXNNAZEBD_134455` — 15 ;
- `Mini_SPi_AKM7169_1993_on_market_non_precise` — 4 ;
- `MPi_1997_plus` — 3 ;
- `SPi_1997_plus` — 2 ;
- `Mini_SPi_automatic_AKM7169_1993_on_market_non_precise` — 1 ;
- `Mini_SPi_high_compression_AKM7169_1993_on_market_non_precise` — 1 ;
- `Mini_SPi_manual_AKM7169_1993_on_market_non_precise` — 1.

Note : la liste textuelle comporte neuf libellés car les trois spécialisations AKM7169 sont distinctes du scope général. Le contrôle de migration doit compter les libellés réellement présents en SQLite, pas un nombre hardcodé dans le code.

Ne remplir les dimensions qu’à partir des audits/sources déjà prouvés. Une chaîne `variant` n’est pas, à elle seule, une autorisation d’inventer une portée supplémentaire.

### Étape D — mécanique structurée pilote

Ne pas convertir automatiquement tous les paragraphes par regex. Structurer manuellement/explicitement un échantillon constructeur déjà vérifié : quelques couples simples + au moins une procédure dépose/repose. Vérifier que la donnée brute reste accessible et que la valeur structurée donne exactement la même information.

### Étape E — extension progressive

Après validation du pilote, convertir les autres données mécaniques RAVE par lots audités, puis reprendre AKM6799 en injectant directement les nouvelles données dans le socle structuré.

## 18. TESTS AVANT TOUT #96

Un self-test dédié au socle doit vérifier au minimum :

- présence des 11 tables ;
- `PRAGMA integrity_check = ok` ;
- révision attendue ;
- 93 faits RAVE historiques toujours présents et inchangés ;
- 105 faits experts historiques toujours présents ;
- exactement 93 miroirs `mems_knowledge_item` pour la première migration complète ;
- aucun `legacy_rave_fact_key` orphelin ou dupliqué ;
- chaque connaissance migrée possède au moins une portée ou une justification `general`/non précisée ;
- aucune portée Japan-only ne matche Europe/UK ;
- aucun fait MPi ne matche une requête SPi ;
- une portée NULL produit `UNKNOWN`, pas `EXACT` ;
- un `operator=any` explicite produit l’universalité ;
- une spécification multi-étapes conserve l’ordre ;
- une procédure conserve les numéros d’étapes sans trou/duplication ;
- une étape référant un `spec_key` pointe vers une spécification existante ;
- détection des conflits : deux valeurs différentes sur la même portée ne doivent pas être fusionnées silencieusement ;
- le générateur r20 historique et la reconstruction fallback continuent de fonctionner.

## 19. CE QUI NE CHANGE PAS PENDANT CETTE MIGRATION

- `MEMSX64` reste sur #95 tant que le prototype n’est pas validé.
- Aucun #96 avant validation du diff et des self-tests.
- Aucun changement protocole MEMS, commandes sensibles, acquisition, RAM, calculs, UI, responsive ou SVG.
- Aucun changement Qwen/ONNX, sampling, prompt ou budget de tokens.
- `mems_rave_fact`, `mems_expert_fact_external`, `ecu_fitment` restent présents et lisibles.
- AKM6799 reste en recherche lecture seule jusqu’à validation du socle.

## 20. ROUTAGE IA CIBLE APRÈS MIGRATION DES DONNÉES

Le futur lecteur doit suivre cet ordre :

`question → normalisation/alias → domaine/type demandé → composant/opération → contexte véhicule/moteur → filtre EXACT/UNKNOWN/INCOMPATIBLE → niveau de preuve/source → grounding limité → Qwen`.

Exemples attendus :
- « couple de serrage de la culasse Mini SPi 1995 » → domaine mécanique + type torque + composant culasse + scope 1995/SPi ;
- « comment déposer la culasse » → type procedure/remove + composant culasse, étapes Rover ordonnées ;
- « couleur fils sonde température Mini SPi Japon 97 » → electrical/wiring + portée Japan 97 ;
- « que sais-tu sur Mini SPi ? » → priorité aux faits SPi, sans laisser remonter MPi comme fait équivalent ;
- si aucune donnée constructeur compatible : réponse explicite « donnée vérifiée non disponible », sans valeur ou procédure inventée.

## 21. PROCHAINE ACTION EXACTE POUR LA NOUVELLE DISCUSSION

1. Lire `RAPPORT_SUIVI_ECU_MEMS_MANAGER.md` puis ce fichier `RAPPORT_SCHEMA_SOCLE_RAVE_EXACT.md` sur la branche `RAPPORT`.
2. Vérifier que `MEMSX64` est toujours exactement #95 `f2e97b3e3a432785e159d30bbeca7b7bef2fdcb4`.
3. Reprendre uniquement `tmp-rave-knowledge-foundation`, créée depuis #95.
4. Construire **le lot de schéma additif** `research_enrichment_1730.qz64` contenant d’abord les 11 tables + indexes, sans suppression/modification des tables historiques.
5. Valider localement la reconstruction r20 et les invariants 93 RAVE / 105 experts avant de commencer la migration des 93 faits.
6. Ajouter ensuite le self-test de fondation et seulement après migrer les portées prouvées.
7. Ne pas pousser `MEMSX64` et ne pas lancer #96 avant validation complète du candidat temporaire.
