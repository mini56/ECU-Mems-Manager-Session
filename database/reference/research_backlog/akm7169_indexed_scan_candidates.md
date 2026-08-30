# AKM7169ENG — indexed-scan research backlog

Date: 2026-08-29

Status for every item in this file unless explicitly promoted later:

`OCR_INDEXED_SCAN_CANDIDATE — PRIMARY PAGE VERIFICATION REQUIRED`

This file deliberately preserves useful information found during research without falsely promoting OCR/indexed text to a constructor-verified database fact. The exact physical AKM7169ENG scan must be acquired, SHA-256 hashed and visually checked before any candidate below becomes an authoritative structured fact.

Primary indexed source used for discovery:
https://www.scribd.com/document/734770360/Rover-Mini-1992-1996-Repair-Manual

Corroborating indexed copy:
https://www.scribd.com/document/625575179/Mini-Repair-Manual-92-96-Autom

Official publication identity reference:
https://store.otpubs.com/service-manual-mini-all-models-1993-on-akm7169/

## Already present in batch 1720 — DO NOT DUPLICATE

The indexed scan reconfirms the seven historical facts already retained by batch 1720:

- AKM7169ENG / Rover Group Limited / 1993 / Mini from VIN 049349 scope;
- manual ECU `MNE101040`;
- automatic ECU `MNE101060`;
- high-compression ECU `MNE101070`;
- indirect single-point injection controlled by ECU using speed/density air-flow calculation;
- TPS approximately 0–1 V closed and 4–5 V open;
- fuel pressure 1.0 bar ±4%, constant.

Rediscovery is evidence/corroboration, not a reason to create duplicate facts.

## Candidate — Engine Tuning Data 3 / SPi manual gearbox

The indexed scan associates this page with a 1.3 Mini SPi manual gearbox application and exposes additional values not retained by the seven-fact historical lot. Candidates to verify on the physical page include:

- engine type/capacity: `12A2DF75 / 1275 cm³`;
- firing order: `1-3-4-2`;
- idle speed under ECU control: about `850 ±25 rev/min`;
- exhaust CO at idle: indexed text indicates `0.5% max`;
- valve/rocker cold clearance: `0.27–0.33 mm`;
- programmed ignition / distributor reference: indexed text includes `NJC10034`;
- crankshaft sensor reference: `ADU7340`;
- ignition coil candidate: `AUU1326 or ADU8779`;
- primary coil resistance: `0.71–0.81 ohm` at 20°C;
- engine-idling coil current/consumption candidate: `0.25–0.75 A average`;
- spark plug reference: `GSP4462`;
- spark plug gap: `0.85 ±0.05 mm`;
- injector / pressure-regulator unit candidate: `JZX3300`;
- fuel grade: `95 RON minimum — unleaded`.

The compression-ratio/ignition-timing row is OCR-alignment-sensitive in the indexed text and is intentionally **not converted into a numeric fact here** until the source page is visually inspected.

## Candidate — Engine Tuning Data 4 / SPi automatic gearbox

Additional indexed values associated with the automatic application include:

- engine type/capacity: `12A2DF76 / 1275 cm³`;
- firing order: `1-3-4-2`;
- compression ratio candidate: `10.1:1`;
- idle speed under ECU control: about `850 ±25 rev/min`;
- exhaust CO at idle: indexed text indicates `0.5% max`;
- valve/rocker cold clearance: `0.27–0.33 mm`;
- distributor/programmed-ignition family appears the same as the manual page;
- crankshaft sensor candidate: `ADU7340`;
- ignition coil candidate: `AUU1326 or ADU8779`;
- primary resistance candidate: `0.71–0.81 ohm`;
- spark plug candidate: `GSP4462`, gap `0.85 ±0.05 mm`;
- injector/pressure-regulator unit candidate: `JZX3300`;
- fuel grade candidate: `95 RON minimum — unleaded`.

Every repeated field must still be verified on Tuning Data 4 itself before database promotion; similarity with Tuning Data 3 is not proof.

## Candidate — Engine Tuning Data 5 / SPi high-compression engine

Additional indexed values associated with the high-compression application include:

- engine type candidate: `12A2EF77`;
- indexed capacity OCR appears as `1274 cm³`; this is explicitly flagged as **OCR-ambiguous** and must not be corrected or stored as 1275 merely by expectation;
- firing order candidate: `1-3-4-2`;
- compression ratio candidate: `10.1:1`;
- idle speed under ECU control: about `850 ±25 rev/min`;
- exhaust CO at idle: indexed text indicates `0.5% max`;
- valve/rocker cold clearance candidate: `0.27–0.33 mm`;
- distributor/programmed-ignition candidate: `NJC10034`;
- crankshaft sensor candidate: `ADU7340`;
- ignition coil candidate: `AUU1326 or ADU8779`;
- primary resistance candidate: `0.71–0.81 ohm`;
- spark plug candidate: `GSP4462`, gap `0.85 ±0.05 mm`;
- injector/pressure-regulator unit candidate: `JZX3300`;
- fuel grade candidate: `95 RON minimum — unleaded`.

Do not rename the application `Cooper`; retain the factory wording `high compression engine` unless another primary source explicitly establishes a model mapping.

## Candidate — General Data / SPi fuel system

The indexed General Data section contains additional SPi fuel-system candidates:

- electric immersed fuel pump;
- pump maximum-pressure candidate around `2.7 bar` at `16 V`;
- regulated injection-pressure range candidate `1.0–1.1 bar`.

These are separate concepts from the tuning-page fact `1.0 bar ±4% constant` and must not be merged automatically.

## Candidate — Maintenance / SPi tooling

The indexed maintenance text states that injection-engine tuning is to be carried out using Rover-era diagnostic equipment identified as `Microcheck` or `Cobest`.

Potential user-facing value after source-page verification:

- historical diagnostic-tool compatibility / workshop procedure context;
- link to MEMS diagnostic history, without implying that MEMS Manager must emulate those tools or change its communication protocol.

## Candidate — Maintenance / SPi under-bonnet reference

The indexed scan contains an `UNDERBONNET LOCATIONS — SPi` view/list identifying components including the engine-management ECU, fuse box, coil, distributor, fuel cut-off switch, radiator/cooling fan and service points.

This is a strong candidate for a future **visual reference asset** once the exact source page is available, because it provides useful physical-location information that cannot be represented as well by a single text fact.

## Candidate — torque/service data near SPi section

The indexed text exposes several torque/service values around `MODULAR ENGINE MANAGEMENT SYSTEM — SPi`, including candidates such as:

- throttle body to manifold: `10 Nm`;
- distributor-cap screws: `2 Nm`;
- air-temperature sensor to air cleaner: `7 Nm`;
- oxygen sensor to front pipe: `55 Nm`;
- injector housing to throttle body screws: `5 Nm`;
- fuel-pipe bracket to throttle body nuts: `10 Nm`.

Some neighbouring OCR values are incomplete/garbled; they are intentionally omitted rather than guessed.

## Data-model implications

If primary-page verification confirms these candidates, existing structured tables may cover specifications/values. If not, create additive storage rather than discard data. Likely useful semantic categories include:

- application-specific tuning specification;
- component OE/service reference;
- torque specification;
- workshop-tool compatibility/context;
- component physical-location visual;
- warning/caution tied to a procedure or specification.

No candidate in this backlog may modify or control ECU communication. Any future program feature suggested by this material must be proposed separately to the user before implementation.
