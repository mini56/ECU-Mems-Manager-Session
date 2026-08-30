# AKM7169ENG / batch 1720 — source and visual-backfill audit

Date: 2026-08-29

## Scope

This is a documentary audit only. It does not modify `research_enrichment_1720.qz64`, historical `mems_rave_fact` rows, ECU communication, protocol, acquisition, RAM, write/reset, Qwen/ONNX, UI or 32-bit code.

Production remains `MEMSX64` BUILD #101 `22dbe75ed14e0a61e694159d505ef72245116b48`.

## Constructor identity confirmed

The retained historical batch identifies the factory publication as:

- Rover Group Limited;
- `AKM7169ENG`;
- Mini Repair Manual;
- Publication Part No. `AKM7169`;
- 1993;
- manual-wide scope: Mini models manufactured from VIN `049349`.

Independent/current source checks agree with this identity:

1. Scribd indexed scan `Rover Mini 1992-1996 Repair Manual`, document `734770360`, advertised as 482 pages. Its indexed text shows the `AKM7169ENG` cover, VIN `049349`, Rover Group Limited 1993 and the relevant SPi tuning pages.
   Source: https://www.scribd.com/document/734770360/Rover-Mini-1992-1996-Repair-Manual
2. Scribd indexed scan `Mini Repair Manual 92-96 (Autom)`, document `625575179`, also advertised as 482 pages and carrying the same AKM7169ENG identity.
   Source: https://www.scribd.com/document/625575179/Mini-Repair-Manual-92-96-Autom
3. Original Technical Publications lists `Service Manual - Mini All Models 1993 on (AKM7169)` and advertises 463 pages.
   Source: https://store.otpubs.com/service-manual-mini-all-models-1993-on-akm7169/
4. eManual/eManualOnline advertises a 1992-1996 Rover Mini PDF of 41.87 MB.
   Source: https://www.emanuals.com/1992-1996-rover-mini-service-repair-manual.html
5. Multiple Tradebit listings advertise the complete AKM7169ENG PDF as a commercial download.

## Physical-page safety conclusion

The public/indexed representations do not expose one common reproducible physical pagination: Scribd advertises 482 pages while the current official-publication listing advertises 463 pages. Therefore **no physical PDF index may be inferred or transferred between these sources**.

A future AKM visual batch requires an exact retained PDF/scan with:

- stable/reproducible acquisition;
- complete SHA-256;
- verified page count;
- direct text/title checks on every selected page;
- rendering/hash of each retained visual asset.

Until that exact binary source is available, no AKM PNG and no AKM visual qz64 is authorised.

## Content already confirmed by indexed factory scan

The indexed scan confirms the three tuning-data pages used by historical batch 1720:

- `ENGINE TUNING DATA 3`: 1.3 Mini SPi manual gearbox, ECU `MNE 101040`;
- `ENGINE TUNING DATA 4`: 1.3 Mini SPi automatic gearbox, ECU `MNE 101060`;
- `ENGINE TUNING DATA 5`: 1.3 Mini SPi high compression engine, ECU `MNE 101070`.

It also independently reconfirms the historical 1720 values already in the database:

- indirect single-point injection with ECU control using speed/density air-flow measurement;
- fuel pressure 1.0 bar ±4.0%, constant;
- throttle potentiometer: 0–1 V closed, 4–5 V open.

These are **not duplicated as new facts** merely because they were rediscovered on the Web.

## File Library check

A search of the user's previously uploaded files for `AKM7169ENG`, `AKM7169`, `Engine Tuning Data 3/4/5` and `Mini Repair Manual 92-96` did not return a complete AKM7169 factory scan. Returned MEMS documents are derived/project material, not the original complete source binary.

## Backfill state

- Existing structured historical facts: 7.
- Existing exact visual assets: 0.
- Visual backfill status: **blocked only on exact reproducible source binary**.
- Structured facts remain usable/consultative and unchanged.

## Next action

Continue searching for an exact reproducible AKM7169ENG scan. If found: hash first, verify edition/page count, locate Introduction/publication scope + Engine Tuning Data 3/4/5 physically, then build an additive visual batch using the same architecture validated by RCL0194/1750 and RCL0193/1760.
