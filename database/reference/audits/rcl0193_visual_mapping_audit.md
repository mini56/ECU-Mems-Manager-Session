# RCL0193ENG visual source mapping audit

Date: 2026-08-29  
Working branch: `tmp-rave-visual-backfill`  
Production reference: `MEMSX64` BUILD #101 `22dbe75ed14e0a61e694159d505ef72245116b48`

## Invariants

- historical RCL0193 facts reconstructed from BUILD #101 knowledge set: **31**
- exact historical document value: `Mini Workshop Manual — RCL0193ENG`
- complete RCL0193 `mems_rave_fact` SQL statements in historical 1660: **31**
- historical `image_ref`: **empty for all 31 facts**
- RCL0193 PDF: **372 pages**, SHA-256 `c050a3eebe50c5a85bf8a69b7722bd2052079944e09d58578a498984ecf06715`
- historical `PDF p.N` is treated as zero-based PDF index N; viewer page is N+1; the actual page excerpts below are retained for verification
- no historical fact or qz64 is modified by this audit

## Exact 31 fact mappings

| Fact key | Topic | Historical source section | PDF index(es) | Viewer page(s) |
|---|---|---|---:|---:|
| `RAVE-ACT-IACV-001` | IAC IACV ralenti bypass demarrage froid | PDF p.114 / Idle air control valve | 114 | 115 |
| `RAVE-ACT-INJECTORS-001` | injecteurs MPI solenoide pulse width commande masse | PDF p.112 / Injectors | 112 | 113 |
| `RAVE-ADJ-LOST-MOTION-001` | lost motion gap jeu perdu papillon ralenti | PDF p.38-39 / Engine Tuning Data | 38, 39 | 39, 40 |
| `RAVE-ADJ-THROTTLE-CABLE-001` | cable accelerateur papillon reglage | PDF p.118 / Engine Management System - MEMS / Adjustments / Throttle cable, 19.20.05 | 118 | 119 |
| `RAVE-CAUTION-THROTTLE-STOP-001` | vis butee papillon ralenti cable | PDF p.118 et p.114 / Throttle cable + TP/IACV | 118 | 119 |
| `RAVE-EVAP-PURGE-001` | purge canister EVAP 70C ralenti catalyseur | PDF p.98 / EVAP canister purge valve | 98 | 99 |
| `RAVE-FUEL-PRESSURE-MPI-001` | pression carburant MPI injection multipoint | PDF p.40 / Engine Tuning Data | 40 | 41 |
| `RAVE-FUEL-PRESSURE-SPI-001` | pression carburant SPI injection monopoint | PDF p.38-39 / Engine Tuning Data | 38, 39 | 39, 40 |
| `RAVE-REP-CKP-001` | CKP capteur vilebrequin depose repose couple | PDF p.129-130 / CKP sensor, 18.30.12 | 129, 130 | 130, 131 |
| `RAVE-REP-COIL-001` | bobine allumage depose repose couple | PDF p.125-126 / Ignition coil, 18.20.43 | 125, 126 | 126, 127 |
| `RAVE-REP-ECM-001` | ECM ECU calculateur depose repose antidémarrage programmation | PDF p.126-127 / ECM, 18.30.01 | 126, 127 | 127, 128 |
| `RAVE-REP-ECT-001` | ECT sonde temperature liquide refroidissement depose repose couple | PDF p.128-129 / ECT sensor, 18.30.10 | 128, 129 | 129, 130 |
| `RAVE-REP-IACV-001` | IAC IACV moteur pas a pas depose repose couple | PDF p.127 / IACV, 18.30.05 | 127 | 128 |
| `RAVE-REP-IAT-001` | IAT sonde temperature air admission depose repose couple | PDF p.128 / IAT sensor, 18.30.09 | 128 | 129 |
| `RAVE-REP-INJECTORS-001` | injecteur depose repose joints toriques protection | PDF p.135 et p.170 / Fuel injectors + inlet manifold | 135 | 136 |
| `RAVE-REP-MAP-001` | MAP capteur pression collecteur depose repose couple joint | PDF p.131 / MAP sensor, 18.30.56 | 131 | 132 |
| `RAVE-REP-PURGE-001` | vanne purge canister depose repose joint | PDF p.101 / Emission canister purge valve, 17.15.39 | 101 | 102 |
| `RAVE-REP-THROTTLE-CABLE-001` | cable accelerateur depose repose | PDF p.120 / Throttle cable, 19.20.06 | 120 | 121 |
| `RAVE-REP-THROTTLE-HOUSING-001` | boitier papillon depose repose joint couple | PDF p.122-123 / Throttle housing, 19.22.45 | 122, 123 | 123, 124 |
| `RAVE-REP-THROTTLE-PEDAL-001` | pedale accelerateur depose repose couple | PDF p.121 / Throttle pedal, 19.20.01 | 121 | 122 |
| `RAVE-REP-TP-SENSOR-001` | capteur position papillon TP TPS depose repose | PDF p.123 / Throttle housing - TP sensor | 123 | 124 |
| `RAVE-SEN-CKP-001` | CKP vilebrequin regime position air gap reluctor | PDF p.108 / Basic ignition timing - CKP | 108 | 109 |
| `RAVE-SEN-ECT-001` | ECT temperature liquide thermistance injection avance | PDF p.109 / ECT sensor | 109 | 110 |
| `RAVE-SEN-HO2S-001` | lambda HO2S sonde oxygene boucle fermee chauffage | PDF p.117 / Heated oxygen sensor | 117 | 118 |
| `RAVE-SEN-IAT-001` | IAT temperature air admission NTC correction injection | PDF p.112 / IAT sensor | 112 | 113 |
| `RAVE-SEN-MAP-001` | MAP pression absolue collecteur charge moteur injection avance | PDF p.109 / MAP sensor | 109 | 110 |
| `RAVE-SEN-TP-001` | TP TPS papillon 5V ralenti acceleration deceleration | PDF p.113-114 / Throttle position sensor | 113, 114 | 114, 115 |
| `RAVE-STRAT-BACKUP-001` | ECM strategie secours capteur defaillant performance reduite | PDF p.107 / Engine management system | 107 | 108 |
| `RAVE-STRAT-OVERRUN-001` | coupure injection deceleration overrun 1600 | PDF p.117 / Over-run fuel cut-off | 117 | 118 |
| `RAVE-STRAT-OVERSPEED-001` | limiteur regime coupure injection 6500 | PDF p.117 / Over-speed fuel cut-off | 117 | 118 |
| `RAVE-TP-VOLTAGE-001` | TP TPS tension papillon valeur controle | PDF p.38-40 / Engine Tuning Data | 38, 39, 40 | 39, 40, 41 |

## Unique source pages required

- PDF index **38** / viewer page **39**: INFORMATION / ENGINE TUNING DATA / 1 / Model: SPi with manual gearbox / Engine / Type / Capacity / 12A2EK71/1275 cm3 / . . . . . . . . . . . . . . . . . . . . . . . . . . . . . / Firing order / 1 - 3 - 4 - 2 / . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . / Compression ratio
- PDF index **39** / viewer page **40**: INFORMATION / 2 / ENGINE TUNING DATA / Model: SPi with automatic gearbox / Engine / Type / Capacity / 12A2EK72/1275 cm3 / . . . . . . . . . . . . . . . . . . . . . . . . . . . . . / Firing order / 1 - 3 - 4 - 2 / . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . / Compression ratio
- PDF index **40** / viewer page **41**: INFORMATION / ENGINE TUNING DATA / 3 / Model: MPi / Engine / Type / Capacity / 12A2LK70/1275 cm3 / . . . . . . . . . . . . . . . . . . . . . . . . . . . . . / Firing order / 1 - 3 - 4 - 2 / . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . / Compression ratio
- PDF index **98** / viewer page **99**: EMISSION CONTROL / DESCRIPTION AND OPERATION / 1 / EMISSION CONTROL SYSTEMS / The vehicle is fitted with various emission and / evaporative control components to comply with / emission regulation requirements. / The following control systems are used to reduce / harmful emissions released into the atmosphere at / all times, and under all conditions: / 1. Exhaust emission control / 2. Fuel vapour evaporative loss control
- PDF index **101** / viewer page **102**: EMISSION CONTROL / 2 / REPAIRS / REVISED: 12/96 / 6. Disconnect vent pipe from emission canister. / 7. Release strap securing emission canister to / mounting bracket beneath wing. / 8. Release emission canister from mounting / bracket and remove. / Refit / 1. Fit emission canister to mounting bracket and / secure with strap.
- PDF index **107** / viewer page **108**: ENGINE MANAGEMENT SYSTEM - MEMS / 2 / DESCRIPTION AND OPERATION / REVISED: 12/96 / ENGINE MANAGEMENT SYSTEM / The Engine Control Module (ECM) monitors the / conditions required for optimum combustion of fuel / in the cylinder through sensors located at strategic / points around the engine. As a result of these / sensor inputs, the engine control module can adjust / the fuel quantity and timing of the fuel being / delivered to the cylinders.
- PDF index **108** / viewer page **109**: ENGINE MANAGEMENT SYSTEM - MEMS / DESCRIPTION AND OPERATION / 3 / REVISED: 12/96 / BASIC IGNITION TIMING / Crankshaft position sensor / The speed and position of the engine is detected by / the crankshaft position (CKP) sensor which is bolted / to, and projects through, the engine adaptor plate / adjacent to the flywheel. / The CKP sensor is an inductive sensor consisting of / a bracket mounted body containing a coil and a
- PDF index **109** / viewer page **110**: ENGINE MANAGEMENT SYSTEM - MEMS / 4 / DESCRIPTION AND OPERATION / Manifold absolute pressure sensor / The manifold absolute pressure (MAP) sensor is / mounted directly on the inlet manifold. The MAP / sensor converts pressure variations within the inlet / manifold into graduated electrical signals which can / be read by the ECM. Increases and decreases in the / manifold pressure provide the ECM with an accurate / representation of the load being placed on the / engine allowing the ECM to adjust the quantity of
- PDF index **112** / viewer page **113**: ENGINE MANAGEMENT SYSTEM - MEMS / DESCRIPTION AND OPERATION / 7 / REVISED: 12/96 / Intake air temperature sensor / The intake air temperature (IAT) sensor is located in / the side of the inlet manifold. The IAT sensor is of / the negative temperature coefficient (NTC) type, / designed to reduce its resistance with increasing / temperature. The ECM receives a signal / proportional to the temperature of the intake air. / When used in conjunction with the signal from the
- PDF index **113** / viewer page **114**: ENGINE MANAGEMENT SYSTEM - MEMS / 8 / DESCRIPTION AND OPERATION / Throttle housing / The throttle housing is located between the inlet / manifold and air intake hose and is sealed to the / manifold by an ’O’ ring. The throttle housing / incorporates a throttle disc which is connected to the / throttle pedal via the throttle lever and a cable. / There are two breather pipes connected to the / throttle housing; one either side of the throttle disc. / When the engine is running with the throttle disc
- PDF index **114** / viewer page **115**: ENGINE MANAGEMENT SYSTEM - MEMS / DESCRIPTION AND OPERATION / 9 / Idle air control valve / The idle air control valve (IACV) is mounted on the / inlet manifold and controlled by the ECM. The IACV / opens a pintle valve situated in an air passage in the / throttle housing, allowing air to bypass the throttle / disc and flow straight into the inlet manifold. / By changing the amount the IACV is open the ECM / can control engine idle speed and cold start air flow / requirements by adjusting the flow of air in the
- PDF index **117** / viewer page **118**: ENGINE MANAGEMENT SYSTEM - MEMS / 12 / DESCRIPTION AND OPERATION / Heated oxygen sensor / The modular engine management system operates / a closed loop emission system to ensure the most / efficient level of exhaust gas conversion. / A heated oxygen sensor (HO2S) fitted in the / exhaust manifold monitors the exhaust gases. It / supplies a small voltage proportional to exhaust / oxygen content to the ECM. As the air/fuel mixture / weakens, the exhaust oxygen content increases and
- PDF index **118** / viewer page **119**: ENGINE MANAGEMENT SYSTEM - MEMS / ADJUSTMENTS / 1 / THROTTLE CABLE / Service repair no - 19.20.05 / NOTE: Before adjusting cable, ensure that / cable is correctly routed and located. Do / not attempt to adjust throttle cable or / engine idle speed by means of the throttle stop / screw. / Adjust / 1. Remove air cleaner. See Repairs.
- PDF index **120** / viewer page **121**: ENGINE MANAGEMENT SYSTEM - MEMS / REPAIRS / 1 / THROTTLE CABLE / Service repair no - 19.20.06 / Remove / 1. Remove air cleaner. See this section. / 2. Remove ECM. See this section. / 3. Release throttle cable adjustment nut from / abutment bracket. / 4. Disconnect inner cable from throttle cam. / 5. Release heater air intake duct from adaptor
- PDF index **121** / viewer page **122**: ENGINE MANAGEMENT SYSTEM - MEMS / 2 / REPAIRS / THROTTLE PEDAL / Service repair no - 19.20.01 / Remove / 1. Release heater air intake duct from adaptor / inside vehicle. / 2. Position air intake duct aside. / 3. Remove clip securing inner cable to throttle / pedal. / 4. Release inner cable from pedal.
- PDF index **122** / viewer page **123**: ENGINE MANAGEMENT SYSTEM - MEMS / REPAIRS / 3 / THROTTLE HOUSING / Service repair no - 19.22.45 / Remove / 1. Disconnect battery earth lead. / 2. Remove air cleaner. See this section. / 3. Release throttle cable adjusting nut from / abutment bracket. / 4. Disconnect throttle cable from throttle cam and / position aside.
- PDF index **123** / viewer page **124**: ENGINE MANAGEMENT SYSTEM - MEMS / 4 / REPAIRS / Do not carry out further dismantling if / component is removed for access only. / 13. Remove and discard 2 screws and wave / washers securing TP sensor to throttle / housing. / 14. Remove TP sensor specification plate. / 15. Pull TP sensor from throttle spindle. / CAUTION: Do not twist or apply leverage / to TP sensor.
- PDF index **125** / viewer page **126**: ENGINE MANAGEMENT SYSTEM - MEMS / 6 / REPAIRS / SPARK PLUGS / Service repair no - 18.20.02 / Remove / 1. Disconnect h.t. leads from spark plugs. / 2. Using a spark plug socket, remove 4 spark / plugs. / Refit / 1. Fit terminals to new spark plugs. / 2. Set gap of each spark plug to 0.85 mm.
- PDF index **126** / viewer page **127**: ENGINE MANAGEMENT SYSTEM - MEMS / REPAIRS / 7 / Refit / 1. Position ignition coil to mounting bracket, fit / bolts and tighten to 10 Nm. / 2. Identify and connect h.t. leads to ignition coil. / 3. Connect multiplug to ignition coil. / ENGINE CONTROL MODULE (ECM) / Service repair no - 18.30.01 / Remove / 1. Disconnect battery earth lead.
- PDF index **127** / viewer page **128**: ENGINE MANAGEMENT SYSTEM - MEMS / 8 / REPAIRS / Refit / 1. Position ECM and connect 2 multiplugs. / 2. Locate ECM on mounting bracket. / CAUTION: Ensure mounting rubbers are / correctly located in mounting bracket. / 3. Fit and tighten bolt securing ECM to mounting / bracket to 10 Nm. / 4. Connect battery earth lead. / NOTE: If a new ECM has been fitted, the
- PDF index **128** / viewer page **129**: ENGINE MANAGEMENT SYSTEM - MEMS / REPAIRS / 9 / INTAKE AIR TEMPERATURE (IAT) SENSOR / Service repair no - 18.30.09 / Remove / 1. Disconnect multiplug from IAT sensor. / 2. Remove IAT sensor. / Refit / 1. Clean threads of IAT sensor. / 2. Clean mating faces of IAT sensor and inlet / manifold.
- PDF index **129** / viewer page **130**: ENGINE MANAGEMENT SYSTEM - MEMS / 10 / REPAIRS / Refit / 1. Clean threads of ECT sensor. / 2. Clean mating faces of ECT sensor and / thermostat housing. / 3. Fit new sealing washer to ECT sensor. / 4. Fit ECT sensor and tighten to 15 Nm. / 5. Connect multiplug to ECT sensor. / 6. Top-up coolant. See / MAINTENANCEMaintenance.
- PDF index **130** / viewer page **131**: ENGINE MANAGEMENT SYSTEM - MEMS / REPAIRS / 11 / Refit / 1. Clean CKP sensor and mating face on flywheel / housing. / 2. Position CKP sensor, fit bolts and tighten to 6 / Nm. / 3. Position CKP sensor multiplug to flywheel / housing, fit bolt and tighten to 3 Nm. / 4. Connect harness to CKP sensor multiplug. / THROTTLE POSITION (TP) SENSOR
- PDF index **131** / viewer page **132**: ENGINE MANAGEMENT SYSTEM - MEMS / 12 / REPAIRS / Refit / 1. Clean mating faces of throttle housing and TP / sensor. / 2. Fit TP sensor to throttle spindle. Ensure that / during fitting, the machined flat on the throttle / spindle is aligned with the mating portion of the / TP sensor. / CAUTION: The TP sensor can be easily / damaged during fitting. When pressing the
- PDF index **135** / viewer page **136**: ENGINE MANAGEMENT SYSTEM - MEMS / 16 / REPAIRS / Refit / 1. Lubricate new ’O’ rings with silicone grease / and fit to injectors. / 2. Position fuel rail and fit to injectors. / 3. Fit 2 spring clips securing fuel rail to injectors. / 4. Position air cleaner mounting bracket to fuel / rail and inlet manifold. / 5. Align fuel return pipe ’P’ clip to inlet manifold. / 6. Fit 3 bolts securing air cleaner mounting

## Low lexical-overlap cases retained for explicit review

- `RAVE-REP-ECM-001` → indexes [126, 127]; source tokens not found directly: `['antid', 'calculateur', 'depose', 'marrage', 'programmation', 'repose']`

## Consequence for additive visual backfill

- unique original PDF pages needed to cover all 31 facts: **25**
- each page will be retained once and linked to every supported fact
- composite/range provenance remains composite; no historical provenance is silently rewritten
- next step: verify the page list/content, then generate the additive visual lot with the existing `mems_rave_illustration` + `mems_rave_illustration_link` schema
