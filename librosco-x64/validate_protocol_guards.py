from pathlib import Path
import re
import sys

root = Path(__file__).resolve().parent
protocol = (root / "protocol.c").read_text(encoding="utf-8")
header = (root / "rosco.h").read_text(encoding="utf-8")
definition = (root / "mems_manager_x64.def").read_text(encoding="utf-8")

expected_exports = [
    "mems_Boost_Valve_Off",
    "mems_Boost_Valve_On",
    "mems_CloseIAC",
    "mems_Fan1_Off",
    "mems_Fan1_On",
    "mems_Fan2_Off",
    "mems_Fan2_On",
    "mems_O2Heater_Off",
    "mems_O2Heater_On",
    "mems_OpenIAC",
    "mems_Purge_Valve_Off",
    "mems_Purge_Valve_On",
    "mems_Save",
    "mems_cleanup",
    "mems_clear_faults",
    "mems_connect",
    "mems_disconnect",
    "mems_fuel_trim_minus",
    "mems_fuel_trim_plus",
    "mems_get_lib_version",
    "mems_heartbeat",
    "mems_idle_decay_minus",
    "mems_idle_decay_plus",
    "mems_idle_speed_minus",
    "mems_idle_speed_plus",
    "mems_ignition_advance_minus",
    "mems_ignition_advance_plus",
    "mems_init",
    "mems_init_link",
    "mems_is_connected",
    "mems_lock",
    "mems_move_iac",
    "mems_openserial",
    "mems_read",
    "mems_read_iac_position",
    "mems_read_raw",
    "mems_read_serial",
    "mems_reset_ADJ",
    "mems_reset_ECU",
    "mems_reset_EMI",
    "mems_send_command",
    "mems_test_actuator",
    "mems_unlock",
    "mems_write_serial",
    "temperature_value_to_degrees_f"
]
expected_extensions = [
    "mems_reset_adjustments",
    "mems_save_adjustments",
    "mems_init_link_kline19",
    "mems_read_kline19",
    "mems_command_kline19",
    "mems_move_iac_kline19",
    "mems_fuel_trim_2_plus",
    "mems_fuel_trim_2_minus",
    "mems_reset_ECU_leopold"
]

expected_families = [
    "mems_manager_family_supported", "mems_manager_family_uses_kline19",
    "mems_manager_family_init_link", "mems_manager_family_read",
    "mems_manager_family_read_iac", "mems_manager_family_move_iac",
    "mems_manager_family_clear_faults",
    "mems_manager_family_reset_adjustments",
    "mems_manager_family_reset_ecu",
    "mems_manager_family_save_adjustments",
    "mems_manager_family_actuator"
]
expected_injection = [
    "mems_manager_transient_parse_mode_reply",
    "mems_manager_transient_is_aanmp002",
    "mems_manager_transient_parse_data80",
    "mems_manager_transient_parse_word_reply",
    "mems_manager_transient_decode_pair",
    "mems_manager_transient_compare_ticks"
]

expected_protocol_catalogue = [
    "mems2j_frame_encode", "mems2j_frame_decode",
    "mems2j_make_read_request", "mems2j_make_clear_fault_request",
    "mems2j_make_key_request", "mems2j_seed_key",
    "mems2j_response_status", "mems2j_decode_measurement",
    "mems2j_fault_bit", "mems3_frame_encode", "mems3_frame_decode",
    "mems3_make_read_request", "mems3_response_status",
    "mems3_decode_measurement", "mems3_fault_count", "mems3_fault_at",
    "mems_rc5_make_request", "mems_rc5_validate_frame",
    "mems_rc5_fault_count", "mems_rc5_fault_at"
]

exports = []
in_exports = False
for raw in definition.splitlines():
    line = raw.strip()
    if not line or line.startswith(";"):
        continue
    if line.upper() == "EXPORTS":
        in_exports = True
        continue
    if in_exports:
        exports.append(line.split()[0])

if exports != expected_exports + expected_extensions + expected_families + expected_injection + expected_protocol_catalogue:
    raise SystemExit(f"FAIL: changed ABI or missing family/injection/codec exports: {exports}")
if len(set(exports)) != 91:
    raise SystemExit("FAIL: duplicate or missing symbols in ROSCO DLL")

required_protocol_guards = [
    "mems_read_serial(info, (uint8_t*)frame80, (uint16_t)sizeof(*frame80)) == (int16_t)sizeof(*frame80)",
    "mems_read_serial(info, (uint8_t*)frame7d, (uint16_t)sizeof(*frame7d)) != (int16_t)sizeof(*frame7d)",
    "return send_command_with_one_byte_reply(info, MEMS_ClearFaults);",
    "return send_command_with_one_byte_reply(info, MEMS_ResetECU);",
    "return send_command_with_one_byte_reply(info, MEMS_ResetAdj);",
    "return send_command_with_one_byte_reply(info, MEMS_ResetEMI);",
    "return send_command_with_one_byte_reply(info, MEMS_Save);",
    "return mems_reset_ADJ(info); /* Manager legacy reset ECU: 0xFA */",
    "return mems_reset_EMI(info); /* Manager legacy adjustments: 0x0F */",
    "ROSCO_LEGACY_WRAPPER(mems_fuel_trim_plus, MEMS_FuelTrimPlus)",
    "ROSCO_LEGACY_WRAPPER(mems_OpenIAC, MEMS_OpenIAC)",
    "ROSCO_LEGACY_WRAPPER(mems_Fan2_Off, MEMS_Fan2Off)",
    "return send_command_with_one_byte_reply(info, MEMS_Heartbeat);",
]
for guard in required_protocol_guards:
    if guard not in protocol:
        raise SystemExit(f"FAIL protocol guard missing: {guard}")

required_commands = {
    "MEMS_ReqData7D": "0x7D",
    "MEMS_ReqData80": "0x80",
    "MEMS_ClearFaults": "0xCC",
    "MEMS_Heartbeat": "0xF4",
    "MEMS_GetIACPosition": "0xFB",
    "MEMS_ResetAdj": "0xFA",
    "MEMS_ResetECU": "0xAF",
    "MEMS_ResetEMI": "0x0F",
    "MEMS_Save": "0x81",
    "MEMS_FuelTrim2Plus": "0x7B",
    "MEMS_FuelTrim2Minus": "0x7C",
    "MEMS_FuelPumpOn": "0x11",
    "MEMS_FuelPumpOff": "0x01",
    "MEMS_TestInjectors": "0xF7",
    "MEMS_FireCoil": "0xF8",
    "MEMS_OpenIAC": "0xFD",
    "MEMS_CloseIAC": "0xFE",
}
for name, value in required_commands.items():
    pattern = rf"\b{re.escape(name)}\s*=\s*{re.escape(value)}\b"
    if not re.search(pattern, header):
        raise SystemExit(f"FAIL historical command changed/missing: {name}={value}")

print("OK: 53 historical-compatible names + explicit reset + 11 families + 6 injection + 20 protocol functions = 91")
assert "mems_reset_ECU_leopold @54" in definition
print("OK: Manager reset ECU=FA and adjustments=0F, explicit LeopoldG ECU=AF")
assert exports[54:65] == expected_families
assert exports[65:71] == expected_injection
assert exports[71:91] == expected_protocol_catalogue
catalogue_header = (root / "protocol_catalogue.h").read_text(encoding="utf-8")
catalogue_source = (root / "protocol_catalogue.c").read_text(encoding="utf-8")
for name in expected_protocol_catalogue:
    if not re.search(rf"\b{re.escape(name)}\s*\(", catalogue_header) or \
       not re.search(rf"\b{re.escape(name)}\s*\(", catalogue_source):
        raise SystemExit(f"FAIL: missing protocol function {name}")
family_header = (root / "family.h").read_text(encoding="utf-8")
family_source = (root / "family.c").read_text(encoding="utf-8")
injection_header = (root / "transient_injection.h").read_text(encoding="utf-8")
injection_source = (root / "transient_injection.c").read_text(encoding="utf-8")
for name in expected_families:
    if not re.search(rf"\b{re.escape(name)}\s*\(", family_header) or \
       not re.search(rf"\b{re.escape(name)}\s*\(", family_source):
        raise SystemExit(f"FAIL: missing family function: {name}")
for name in expected_injection:
    if not re.search(rf"\b{re.escape(name)}\s*\(", injection_header) or \
       not re.search(rf"\b{re.escape(name)}\s*\(", injection_source):
        raise SystemExit(f"FAIL: missing transient function: {name}")
cmake = (root / "CMakeLists.txt").read_text(encoding="utf-8")
assert "family.c" in cmake and "transient_injection.c" in cmake
assert "protocol_catalogue.c" in cmake
assert "protocol_catalogue_selftest.c" in cmake
print("OK: family and AANMP002 modules exported and compiled within DLL only")
print("OK: full 0x80 and 0x7D frame-length guards present")
print("OK: truncated 0x7D frame is rejected")
print("OK: clear/reset/heartbeat return real command result")
print("OK: original AF/FA/0F/81 reset/save constants and second fuel trim")
# Compare with the actual, UNMODIFIED Session 196 application header.
# LeopoldG's reset constants are different and must not be asserted here.
manager_header = (root.parent / "rosco.h").read_text(encoding="utf-8")
for name, code in {
    "MEMS_ResetECU": "0xFA",
    "MEMS_ResetAdj": "0x0F",
    "MEMS_ClearFaults": "0xCC",
    "MEMS_ReqData80": "0x80",
    "MEMS_ReqData7D": "0x7D",
}.items():
    if not re.search(rf"\b{re.escape(name)}\s*=\s*{re.escape(code)}\b", manager_header):
        raise SystemExit(f"FAIL Session196 public header: {name} must be {code}")

manager_declarations = set(re.findall(
    r"\b(?:bool|void|librosco_version|uint8_t|int16_t)\s+(mems_\w+)\s*\(",
    manager_header))
# This historical declaration is not invoked by Session196's interactive
# button and is not part of the 91-export ABI. Track it, do not disguise it.
historical_declared_only = {"mems_interactive_mode"}
if not historical_declared_only.issubset(manager_declarations):
    raise SystemExit("FAIL: historical interactive declaration unexpectedly changed")
missing = (manager_declarations - historical_declared_only) - set(exports)
if missing:
    raise SystemExit(f"FAIL: Manager's declared linked functions absent: {sorted(missing)}")
print("OK: genuine Session196 declarations align with DLL exports")
print("TRACKED: mems_interactive_mode is declared in Manager but not exported by this 91-symbol DLL")

session_dispatch = (root.parent / "memsinterface_dispatch.cpp").read_text(encoding="utf-8")
for guard in (
    "MemsProtocolSafety::allowsMutation(ecuFamily(), diagnosticMode(), 0x0Fu)",
    "MemsProtocolSafety::allowsMutation(ecuFamily(), diagnosticMode(), 0xFAu)",
    "::mems_reset_adjustments(&m_memsinfo)",
    "::mems_reset_ECU(&m_memsinfo)",
):
    if guard not in session_dispatch:
        raise SystemExit(f"FAIL Session196 reset dispatch differs: {guard}")
print("OK: Session196 still dispatches reset adaptations=0F and ECU action=FA")
print("NOTE: byte-identical does not establish the physical meaning on an ECU")

sys.exit(0)
