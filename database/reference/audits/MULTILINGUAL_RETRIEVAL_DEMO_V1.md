# MULTILINGUAL RETRIEVAL DEMO V1

## Purpose
Validate the generic multilingual/document extraction foundation against the SQLite actually packaged in BUILD #103, without modifying historical tables or production manifests.

## Source baseline
- Production baseline: BUILD #103, commit `1d6316bd1746d6f2b4cfb751cab88d18e27ef730`
- BUILD #103 Actions run: `33334306835`
- Expert SQLite: `database/expert/ia_mems_reference_r20.sqlite`
- Baseline SQLite `user_version`: 20
- Foundation prototype migrates a test copy to `user_version=21`.

## Real BUILD #103 sample
1. `RAVE:RCL0194ENG:20.4`
   - original visual: `images/rave/RCL0194ENG_20_4.png`
   - SHA-256 from BUILD #103: `7a04d84ee0296ab41333650f369aa3624f3ac7988a528584237a7b8fb021bea0`
   - dimensions verified from packaged PNG: 1696 x 1190
   - visual label verified: `COOLANT TEMPERATURE SENSOR (094)`
   - linked legacy knowledge: `RAVE-WIR-SPIJ-008`
2. `PROC-RCL0193FRE-18-30-10-remove-1`
   - ECT removal procedure from RCL0193FRE PDF p.128
   - step 3 intentionally exercises the legacy page-header residue `SYSTEME DE GESTION MOTEUR - MEMS`.
3. RCL0193FRE PDF p.159
   - thermostat opening temperature: `88 °C` from `SPEC-RCL0193FRE-1810-THERMOSTAT-OPEN`
   - upper thermostat housing bolt torque: `11 N.m` in exact source statement from `SPEC-RCL0193FRE-1810-0009`

## Required properties
- Historical BUILD #103 tables remain unchanged.
- Legacy source strings remain exact in `source_exact`.
- Clean display text is stored separately.
- One immutable constructor visual is reused across locales.
- Visual text is modeled as a region/label with separate translations.
- Numeric values and units are independent from language.
- Locale fallback is data-driven.
- Current locales remain FR/EN/ES/IT/PT/DE.
- Future `ja`, `zh-CN`, `hi` are addable without `ALTER TABLE`.
- Demo translations are `draft`; they are not production-approved translations.
- Applying the demo twice must be idempotent.
- `PRAGMA integrity_check` must remain `ok`.
- `PRAGMA foreign_key_check` must return zero rows.

## Correction to pre-push journal
The pre-push journal mentioned thermostat data on RCL0193FRE p.34. Inspection of the packaged BUILD #103 SQLite proves that the representative thermostat values used by this demo are on **PDF p.159**. This audit and the validator use the verified p.159 source.
