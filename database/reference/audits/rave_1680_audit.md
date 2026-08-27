# RAVE batch 1680 — RCL0194ENG SPi Japan 97MY

## Source

- Rover/Mini factory document: `RCL0194ENG`, Mini Electrical Circuit Diagrams, 3rd Edition.
- Vehicle scope used here: Mini 97MY SPi Japan from VIN `SAXXNNAXKBD 134455`.
- Pages reviewed: `Engine Management System (MEMS) SPi (JAPAN) 20.3`, `20.4`, and `Cooling Fan SPi (JAPAN) 39.3`.

## Data policy

- Only visually unambiguous circuit relationships are included.
- The diagram's SPi Japan scope is preserved; facts are not generalized to MPi or older European SPi cars.
- No MEMS generation number is inferred from RCL0194 alone.
- No diagnostic command, DTC meaning, write capability, or undocumented ECU function is inferred from wiring.
- Existing unresolved code 23 / immobiliser evidence is not changed by this batch.
- All facts are stored as `verifie_constructeur` and mirrored into `mems_expert_fact_external` for read-only IA MEMS retrieval.

## Added facts

| Fact key | Topic | Factory relationship retained |
|---|---|---|
| RAVE-WIR-SPIJ-001 | wiring_ecu | SPi Japan 97MY scope from VIN SAXXNNAXKBD 134455 |
| RAVE-WIR-SPIJ-002 | relay_module | MEMS relay module 693: main, O2 heater, fuel pump, starter relays |
| RAVE-WIR-SPIJ-003 | oxygen_sensor_wiring | O2 C159-7/+VE, C159-18/-VE, C159-29 screen ground; heater relay control C159-36 |
| RAVE-WIR-SPIJ-004 | fuel_pump_circuit | ECU relay control C159-20; inertia switch C123; fuel pump C205 |
| RAVE-WIR-SPIJ-005 | injector_wiring | Injector C522-1 to C159-1 and C522-2 to C159-24 |
| RAVE-WIR-SPIJ-006 | purge_valve_wiring | Purge valve C152 control to C159-21 |
| RAVE-WIR-SPIJ-007 | iac_wiring | IAC C177 four phases to C159-3, C159-27, C159-22, C159-2 |
| RAVE-WIR-SPIJ-008 | coolant_sensor_wiring | ECT C165 signal C159-33 and sensor earth C159-30 |
| RAVE-WIR-SPIJ-009 | throttle_sensor_wiring | TPS C175 to C159-8, C159-9 and sensor earth C159-30 |
| RAVE-WIR-SPIJ-010 | iat_sensor_wiring | IAT C174 signal C159-16 and sensor earth C159-30 |
| RAVE-WIR-SPIJ-011 | sensor_ground | ECT/TPS/IAT common sensor earth C159-30 via SJ5 |
| RAVE-WIR-SPIJ-012 | diagnostic_socket_wiring | Diagnostic socket C549 signal lines to C159-10 and C159-15 |
| RAVE-WIR-SPIJ-013 | manifold_heater_wiring | Manifold heater C224 controlled through relay C269; no ambiguous ECU pin asserted |
| RAVE-WIR-SPIJ-014 | cooling_fan_wiring | SPi Japan fan relay C019 controlled by high-temperature switch C370/C371; no MEMS ECU connection shown on page 39.3 |

## Validation before commit

The SQL payload was applied to a copy of `ia_mems_reference_r20.sqlite` extracted from GitHub Actions #86.

- `RAVE-WIR-SPIJ-*` in `mems_rave_fact`: 14
- mirrored `RAVE-WIR-SPIJ-*` in `mems_expert_fact_external`: 14
- total RAVE facts after application: 60
- total expert facts after application: 72
- all verification levels: `verifie_constructeur`
- one variant only: `SPi_Japan_97MY_from_VIN_SAXXNNAXKBD_134455`
- database schema/revision unchanged: r20
- decompressed SQL length: 17194 bytes
- decompressed SQL SHA-256: `dec464ad3d5446a78886cfc995a1dc18553aab4f86ac2676803a56ac62d916dd`
