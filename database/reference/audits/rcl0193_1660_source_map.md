# RCL0193ENG — mapping source vérifié des 31 faits historiques 1660

Date: 2026-08-29

Audit uniquement : aucun fait historique, qz64 de données ou asset visuel n’est modifié.

- faits RCL0193 parsés: **31/31**; clés uniques **31/31**; `image_ref` vide **31/31**
- pages physiques uniques réellement citées: **25**
- qz64 1660 SHA-256 `5d56f198a6f5ccaf36d7f94646e3b9af4efd465e7f80da0ebea3d74f3d894093`
- SQL SHA-256 `b978f7e73487c6243c8a6c355b4e5dedd90cf449bee223e9c1eb81e08eb0a7cd`
- PDF constructeur: 372 pages, SHA-256 `c050a3eebe50c5a85bf8a69b7722bd2052079944e09d58578a498984ecf06715`

## Correction de l’inventaire initial

Le parseur SQL structure confirme que la page 170 ne fait partie d’aucun `source_section` des 31 faits `mems_rave_fact` RCL0193. La couverture correcte est donc 25 pages uniques, pas 26.

## Preuve de l’index PDF

Les références historiques `PDF p.N` correspondent directement à l’index physique N du PDF retenu. Contrôles durs passés sur p.38, p.40, p.109, p.118, p.127, p.128 et p.129; aucun offset n’est appliqué.

## Mapping 31/31

| Fact key | Variante | Source section | Pages | Type visuel | Ancres vues |
|---|---|---|---|---|---|
| `RAVE-ADJ-THROTTLE-CABLE-001` | `Mini_1997_2000` | PDF p.118 / Engine Management System - MEMS / Adjustments / Throttle cable, 19.20.05 | `118` | `procedure_page` | cable, throttle |
| `RAVE-ADJ-LOST-MOTION-001` | `SPi_1997_plus` | PDF p.38-39 / Engine Tuning Data | `38,39` | `table_page` | tuning |
| `RAVE-CAUTION-THROTTLE-STOP-001` | `Mini_1997_2000` | PDF p.118 et p.114 / Throttle cable + TP/IACV | `118` | `technical_description_page` | cable, throttle |
| `RAVE-REP-THROTTLE-CABLE-001` | `Mini_1997_2000` | PDF p.120 / Throttle cable, 19.20.06 | `120` | `procedure_page` | cable, throttle |
| `RAVE-REP-THROTTLE-PEDAL-001` | `Mini_1997_2000` | PDF p.121 / Throttle pedal, 19.20.01 | `121` | `procedure_page` | pedal, throttle |
| `RAVE-REP-THROTTLE-HOUSING-001` | `Mini_1997_2000` | PDF p.122-123 / Throttle housing, 19.22.45 | `122,123` | `procedure_page` | housing, throttle |
| `RAVE-REP-TP-SENSOR-001` | `Mini_1997_2000` | PDF p.123 / Throttle housing - TP sensor | `123` | `technical_description_page` | housing, throttle |
| `RAVE-REP-IACV-001` | `Mini_1997_2000` | PDF p.127 / IACV, 18.30.05 | `127` | `procedure_page` | iacv |
| `RAVE-REP-IAT-001` | `Mini_1997_2000` | PDF p.128 / IAT sensor, 18.30.09 | `128` | `procedure_page` | iat |
| `RAVE-REP-ECT-001` | `Mini_1997_2000` | PDF p.128-129 / ECT sensor, 18.30.10 | `128,129` | `procedure_page` | ect |
| `RAVE-REP-CKP-001` | `Mini_1997_2000` | PDF p.129-130 / CKP sensor, 18.30.12 | `129,130` | `procedure_page` | ckp |
| `RAVE-REP-MAP-001` | `Mini_1997_2000` | PDF p.131 / MAP sensor, 18.30.56 | `131` | `procedure_page` | map |
| `RAVE-REP-ECM-001` | `Mini_1997_2000` | PDF p.126-127 / ECM, 18.30.01 | `126,127` | `procedure_page` | ecm |
| `RAVE-REP-COIL-001` | `Mini_1997_2000` | PDF p.125-126 / Ignition coil, 18.20.43 | `125,126` | `procedure_page` | coil, ignition |
| `RAVE-FUEL-PRESSURE-SPI-001` | `SPi_1997_plus` | PDF p.38-39 / Engine Tuning Data | `38,39` | `table_page` | tuning |
| `RAVE-FUEL-PRESSURE-MPI-001` | `MPi_1997_plus` | PDF p.40 / Engine Tuning Data | `40` | `table_page` | tuning |
| `RAVE-TP-VOLTAGE-001` | `Mini_1997_2000` | PDF p.38-40 / Engine Tuning Data | `38,39,40` | `table_page` | tuning |
| `RAVE-SEN-CKP-001` | `Mini_1997_2000` | PDF p.108 / Basic ignition timing - CKP | `108` | `technical_description_page` | basic, ckp, ignition, timing |
| `RAVE-SEN-MAP-001` | `Mini_1997_2000` | PDF p.109 / MAP sensor | `109` | `technical_description_page` | map |
| `RAVE-SEN-ECT-001` | `Mini_1997_2000` | PDF p.109 / ECT sensor | `109` | `technical_description_page` | ect |
| `RAVE-SEN-IAT-001` | `Mini_1997_2000` | PDF p.112 / IAT sensor | `112` | `technical_description_page` | iat |
| `RAVE-SEN-TP-001` | `Mini_1997_2000` | PDF p.113-114 / Throttle position sensor | `113,114` | `technical_description_page` | position, throttle |
| `RAVE-ACT-IACV-001` | `Mini_1997_2000` | PDF p.114 / Idle air control valve | `114` | `technical_description_page` | air, control, idle, valve |
| `RAVE-ACT-INJECTORS-001` | `MPi_1997_plus` | PDF p.112 / Injectors | `112` | `technical_description_page` | injectors |
| `RAVE-REP-INJECTORS-001` | `MPi_1997_plus` | PDF p.135 et p.170 / Fuel injectors + inlet manifold | `135` | `technical_description_page` | fuel, injectors, inlet, manifold |
| `RAVE-EVAP-PURGE-001` | `Mini_1997_2000` | PDF p.98 / EVAP canister purge valve | `98` | `technical_description_page` | canister, evap, purge, valve |
| `RAVE-REP-PURGE-001` | `Mini_1997_2000` | PDF p.101 / Emission canister purge valve, 17.15.39 | `101` | `technical_description_page` | canister, emission, purge, valve |
| `RAVE-STRAT-BACKUP-001` | `Mini_1997_2000` | PDF p.107 / Engine management system | `107` | `technical_description_page` |  |
| `RAVE-STRAT-OVERRUN-001` | `Mini_1997_2000` | PDF p.117 / Over-run fuel cut-off | `117` | `technical_description_page` | cut, fuel, off, over, run |
| `RAVE-STRAT-OVERSPEED-001` | `Mini_1997_2000` | PDF p.117 / Over-speed fuel cut-off | `117` | `technical_description_page` | cut, fuel, off, over, speed |
| `RAVE-SEN-HO2S-001` | `Mini_1997_2000` | PDF p.117 / Heated oxygen sensor | `117` | `technical_description_page` | heated, oxygen |

## 25 pages physiques à conserver

| Index | Images | Dessins | Texte | Aperçu |
|---:|---:|---:|---:|---|
| 38 | 1 | 1 | 2829 | INFORMATION ENGINE TUNING DATA 1 Model: SPi with manual gearbox Engine Type / Capacity 12A2EK71/1275 cm3 . . . . . . . . . . . . . . . . . . . . . . . . . . . . . Firing  |
| 39 | 0 | 1 | 2831 | INFORMATION 2 ENGINE TUNING DATA Model: SPi with automatic gearbox Engine Type / Capacity 12A2EK72/1275 cm3 . . . . . . . . . . . . . . . . . . . . . . . . . . . . . Firi |
| 40 | 1 | 1 | 2782 | INFORMATION ENGINE TUNING DATA 3 Model: MPi Engine Type / Capacity 12A2LK70/1275 cm3 . . . . . . . . . . . . . . . . . . . . . . . . . . . . . Firing order 1 - 3 - 4 - 2  |
| 98 | 1 | 1 | 3318 | EMISSION CONTROL DESCRIPTION AND OPERATION 1 EMISSION CONTROL SYSTEMS The vehicle is fitted with various emission and evaporative control components to comply with emissi |
| 101 | 2 | 1 | 1331 | EMISSION CONTROL 2 REPAIRS REVISED: 12/96 6. Disconnect vent pipe from emission canister. 7. Release strap securing emission canister to mounting bracket beneath wing. 8. |
| 107 | 0 | 1 | 2454 | ENGINE MANAGEMENT SYSTEM - MEMS 2 DESCRIPTION AND OPERATION REVISED: 12/96 ENGINE MANAGEMENT SYSTEM The Engine Control Module (ECM) monitors the conditions required for o |
| 108 | 3 | 1 | 1783 | ENGINE MANAGEMENT SYSTEM - MEMS DESCRIPTION AND OPERATION 3 REVISED: 12/96 BASIC IGNITION TIMING Crankshaft position sensor The speed and position of the engine is detect |
| 109 | 2 | 1 | 1675 | ENGINE MANAGEMENT SYSTEM - MEMS 4 DESCRIPTION AND OPERATION Manifold absolute pressure sensor The manifold absolute pressure (MAP) sensor is mounted directly on the inlet |
| 112 | 3 | 1 | 1560 | ENGINE MANAGEMENT SYSTEM - MEMS DESCRIPTION AND OPERATION 7 REVISED: 12/96 Intake air temperature sensor The intake air temperature (IAT) sensor is located in the side of |
| 113 | 1 | 1 | 1757 | ENGINE MANAGEMENT SYSTEM - MEMS 8 DESCRIPTION AND OPERATION Throttle housing The throttle housing is located between the inlet manifold and air intake hose and is sealed  |
| 114 | 3 | 1 | 2272 | ENGINE MANAGEMENT SYSTEM - MEMS DESCRIPTION AND OPERATION 9 Idle air control valve The idle air control valve (IACV) is mounted on the inlet manifold and controlled by th |
| 117 | 1 | 1 | 2902 | ENGINE MANAGEMENT SYSTEM - MEMS 12 DESCRIPTION AND OPERATION Heated oxygen sensor The modular engine management system operates a closed loop emission system to ensure th |
| 118 | 3 | 1 | 850 | ENGINE MANAGEMENT SYSTEM - MEMS ADJUSTMENTS 1 THROTTLE CABLE Service repair no - 19.20.05 NOTE: Before adjusting cable, ensure that cable is correctly routed and located. |
| 120 | 3 | 1 | 872 | ENGINE MANAGEMENT SYSTEM - MEMS REPAIRS 1 THROTTLE CABLE Service repair no - 19.20.06 Remove 1. Remove air cleaner. See this section. 2. Remove ECM. See this section. 3.  |
| 121 | 1 | 1 | 868 | ENGINE MANAGEMENT SYSTEM - MEMS 2 REPAIRS THROTTLE PEDAL Service repair no - 19.20.01 Remove 1. Release heater air intake duct from adaptor inside vehicle. 2. Position ai |
| 122 | 3 | 1 | 725 | ENGINE MANAGEMENT SYSTEM - MEMS REPAIRS 3 THROTTLE HOUSING Service repair no - 19.22.45 Remove 1. Disconnect battery earth lead. 2. Remove air cleaner. See this section.  |
| 123 | 2 | 1 | 1734 | ENGINE MANAGEMENT SYSTEM - MEMS 4 REPAIRS Do not carry out further dismantling if component is removed for access only. 13. Remove and discard 2 screws and wave washers s |
| 125 | 3 | 1 | 654 | ENGINE MANAGEMENT SYSTEM - MEMS 6 REPAIRS SPARK PLUGS Service repair no - 18.20.02 Remove 1. Disconnect h.t. leads from spark plugs. 2. Using a spark plug socket, remove  |
| 126 | 3 | 1 | 455 | ENGINE MANAGEMENT SYSTEM - MEMS REPAIRS 7 Refit 1. Position ignition coil to mounting bracket, fit bolts and tighten to 10 Nm. 2. Identify and connect h.t. leads to ignit |
| 127 | 1 | 1 | 1104 | ENGINE MANAGEMENT SYSTEM - MEMS 8 REPAIRS Refit 1. Position ECM and connect 2 multiplugs. 2. Locate ECM on mounting bracket. CAUTION: Ensure mounting rubbers are correctl |
| 128 | 3 | 1 | 572 | ENGINE MANAGEMENT SYSTEM - MEMS REPAIRS 9 INTAKE AIR TEMPERATURE (IAT) SENSOR Service repair no - 18.30.09 Remove 1. Disconnect multiplug from IAT sensor. 2. Remove IAT s |
| 129 | 2 | 1 | 569 | ENGINE MANAGEMENT SYSTEM - MEMS 10 REPAIRS Refit 1. Clean threads of ECT sensor. 2. Clean mating faces of ECT sensor and thermostat housing. 3. Fit new sealing washer to  |
| 130 | 2 | 1 | 822 | ENGINE MANAGEMENT SYSTEM - MEMS REPAIRS 11 Refit 1. Clean CKP sensor and mating face on flywheel housing. 2. Position CKP sensor, fit bolts and tighten to 6 Nm. 3. Positi |
| 131 | 2 | 1 | 1640 | ENGINE MANAGEMENT SYSTEM - MEMS 12 REPAIRS Refit 1. Clean mating faces of throttle housing and TP sensor. 2. Fit TP sensor to throttle spindle. Ensure that during fitting |
| 135 | 1 | 1 | 1792 | ENGINE MANAGEMENT SYSTEM - MEMS 16 REPAIRS Refit 1. Lubricate new ’O’ rings with silicone grease and fit to injectors. 2. Position fuel rail and fit to injectors. 3. Fit  |

Liste exacte:

`38,39,40,98,101,107,108,109,112,113,114,117,118,120,121,122,123,125,126,127,128,129,130,131,135`

## Prochaine action exacte

Préparer un lot visuel additif RCL0193 qui rend une seule fois ces 25 pages du PDF constructeur exact, calcule leurs SHA-256, les déclare dans le manifeste et relie les 31 faits à une ou plusieurs pages via les tables additives d’illustrations. Les faits historiques restent inchangés.

La base/IA/RAVE reste consultative et ne prend jamais la main sur la communication ECU.
