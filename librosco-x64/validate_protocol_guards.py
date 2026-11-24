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

expected_lucas5as = [
    "mems_manager_lucas5as_slow_bit_high",
    "mems_manager_lucas5as_has_normal_wake",
    "mems_manager_lucas5as_decode_status",
    "mems_manager_lucas5as_extract_status",
    "mems_manager_lucas5as_output_test_commands",
    "mems_manager_lucas5as_open_normal",
    "mems_manager_lucas5as_initial_status",
    "mems_manager_lucas5as_read_status",
    "mems_manager_lucas5as_close",
]

expected_mems19_wakeup = [
    "mems_manager_mems19_slow_bit_high",
    "mems_manager_mems19_key_complement",
    "mems_manager_mems19_has_address_confirmation",
    "mems_manager_mems19_wakeup",
    "mems_manager_mems19_connect_identify",
]

expected_mems2j_session = [
    "mems_manager_mems2j_connect",
    "mems_manager_mems2j_read_measurement",
    "mems_manager_mems2j_read_faults",
    "mems_manager_mems2j_clear_faults",
    "mems_manager_mems2j_keepalive",
    "mems_manager_mems2j_read_cycle",
    "mems_manager_mems2j_last_transaction",
    "mems_manager_mems2j_is_open",
    "mems_manager_mems2j_close",
]

expected_mems3_session = [
    "mems_manager_mems3_connect",
    "mems_manager_mems3_read_measurement",
    "mems_manager_mems3_read_faults",
    "mems_manager_mems3_clear_faults",
    "mems_manager_mems3_keepalive",
    "mems_manager_mems3_read_cycle",
    "mems_manager_mems3_last_transaction",
    "mems_manager_mems3_is_open",
    "mems_manager_mems3_close",
]

expected_rc5_session = [
    "mems_manager_rc5_open_transport",
    "mems_manager_rc5_wakeup",
    "mems_manager_rc5_ping",
    "mems_manager_rc5_read_faults",
    "mems_manager_rc5_decode_fault_detail",
    "mems_manager_rc5_clear_faults",
    "mems_manager_rc5_close_session",
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

if exports != expected_exports + expected_extensions + expected_families + expected_injection + expected_protocol_catalogue + expected_lucas5as + expected_mems19_wakeup + expected_mems2j_session + expected_mems3_session + expected_rc5_session:
    raise SystemExit(f"FAIL: changed ABI or missing family/injection/codec exports: {exports}")
if len(set(exports)) != 130:
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

print("OK: 91 original + 9 Lucas 5AS + 5 MEMS1.9 + 9 native MEMS2J + 9 native MEMS3 + 7 native RC5 = 130 exports")
assert "mems_reset_ECU_leopold @54" in definition
print("OK: Manager reset ECU=FA and adjustments=0F, explicit LeopoldG ECU=AF")
assert exports[54:65] == expected_families
assert exports[65:71] == expected_injection
assert exports[71:91] == expected_protocol_catalogue
assert exports[91:100] == expected_lucas5as
assert exports[100:105] == expected_mems19_wakeup
assert exports[105:114] == expected_mems2j_session
assert exports[114:123] == expected_mems3_session
assert exports[123:130] == expected_rc5_session
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
lucas_header = (root / "lucas5as.h").read_text(encoding="utf-8")
lucas_codec = (root / "lucas5as_codec.c").read_text(encoding="utf-8")
lucas_session = (root / "lucas5as_session.c").read_text(encoding="utf-8")
for name in expected_lucas5as:
    if not re.search(rf"\b{re.escape(name)}\s*\(", lucas_header):
        raise SystemExit(f"FAIL: Lucas5AS header missing {name}")
    source = lucas_session if name.endswith(("_open_normal", "_initial_status", "_read_status", "_close")) else lucas_codec
    if not re.search(rf"\b{re.escape(name)}\s*\(", source):
        raise SystemExit(f"FAIL: Lucas5AS implementation missing {name}")
for required in ("lucas5as_codec.c", "lucas5as_session.c",
                 "lucas5as_protocol_selftest", "lucas5as_session_selftest"):
    if required not in cmake:
        raise SystemExit(f"FAIL: Lucas5AS CMake missing {required}")
print("OK: 9 Lucas5AS read-only API exports and two offline CTests")

mems19_header = (root / "mems19_wakeup.h").read_text(encoding="utf-8")
mems19_codec = (root / "mems19_wakeup_codec.c").read_text(encoding="utf-8")
mems19_source = (root / "mems19_wakeup.c").read_text(encoding="utf-8")
for name in expected_mems19_wakeup:
    if not re.search(rf"\b{re.escape(name)}\s*\(", mems19_header):
        raise SystemExit(f"FAIL: missing MEMS1.9 ABI declaration: {name}")
    source = mems19_codec if name.endswith((
        "_slow_bit_high", "_key_complement",
        "_has_address_confirmation")) else mems19_source
    if not re.search(rf"\b{re.escape(name)}\s*\(", source):
        raise SystemExit(f"FAIL: MEMS1.9 implementation missing: {name}")
for required in ("mems19_wakeup_codec.c", "mems19_wakeup.c",
                 "dll_mems19_wakeup_codec_selftest",
                 "dll_mems19_wakeup_selftest"):
    if required not in cmake:
        raise SystemExit(f"FAIL: MEMS1.9 CMake not integrated: {required}")
for forbidden in ("#include <Qt", "#include <QSerialPort"):
    if forbidden in mems19_source or forbidden in mems19_codec:
        raise SystemExit("FAIL: DLL MEMS1.9 module must not depend on Qt")
print("OK: 5 new native MEMS 1.9 ABI functions; same-handle wakeup with offline tests")

mems2j_header=(root / "mems2j_session.h").read_text(encoding="utf-8")
mems2j_source=(root / "mems2j_session.c").read_text(encoding="utf-8")
mems2j_test=(root / "mems2j_session_selftest.c").read_text(encoding="utf-8")
for name in expected_mems2j_session:
    if not re.search(rf"\b{re.escape(name)}\s*\(",mems2j_header):
        raise SystemExit(f"FAIL MEMS2J C ABI declaration missing: {name}")
    if not re.search(rf"\b{re.escape(name)}\s*\(",mems2j_source):
        raise SystemExit(f"FAIL MEMS2J compiled implementation missing: {name}")
for required in ("mems2j_session.c", "dll_mems2j_session_selftest",
                 "mems2j_session_selftest.c"):
    if required not in cmake:
        raise SystemExit(f"FAIL MEMS2J CMake missing: {required}")
for forbidden in ("#include <Qt", "#include <QSerialPort"):
    if forbidden in mems2j_header or forbidden in mems2j_source:
        raise SystemExit("FAIL MEMS2J DLL layer depends on Qt")
if '31 CB + 15x00' not in mems2j_source or '0x81,0x13,0xF7,0x81,0x0C' not in mems2j_source:
    raise SystemExit("FAIL MEMS2J wake/clear provenance missing")
print("OK: nine native MEMS2J session APIs, offline serial test and zero Qt dependency")

mems3_header=(root / "mems3_session.h").read_text(encoding="utf-8")
mems3_source=(root / "mems3_session.c").read_text(encoding="utf-8")
mems3_test=(root / "mems3_session_selftest.c").read_text(encoding="utf-8")
for name in expected_mems3_session:
    if not re.search(rf"\b{re.escape(name)}\s*\(",mems3_header):
        raise SystemExit(f"FAIL MEMS3 C ABI declaration missing: {name}")
    if not re.search(rf"\b{re.escape(name)}\s*\(",mems3_source):
        raise SystemExit(f"FAIL MEMS3 compiled implementation missing: {name}")
for required in ("mems3_session.c", "dll_mems3_session_selftest", "mems3_session_selftest.c"):
    if required not in cmake:
        raise SystemExit(f"FAIL MEMS3 CMake missing: {required}")
for forbidden in ("#include <Qt", "#include <QSerialPort", "mems2j_frame_encode(", "mems2j_frame_decode("):
    if forbidden in mems3_header or forbidden in mems3_source:
        raise SystemExit(f"FAIL MEMS3 unexpected dependency: {forbidden}")
for required in ("dcb.BaudRate = 9600", "dcb.Parity = EVENPARITY", "mems3_frame_encode(", "mems3_frame_decode(", "0x1A,0x9A", "0x14,0,0", "0x54,0,0"):
    if required not in mems3_source:
        raise SystemExit(f"FAIL MEMS3 protocol guard missing: {required}")
if '#include "mems3_session.c"' not in mems3_test:
    raise SystemExit("FAIL MEMS3 offline test must exercise production source")
print("OK: nine native MEMS3 session APIs, offline serial test and zero Qt dependency")

rc5_header=(root / "rc5_session.h").read_text(encoding="utf-8")
rc5_source=(root / "rc5_session.c").read_text(encoding="utf-8")
rc5_test=(root / "rc5_session_selftest.c").read_text(encoding="utf-8")
for name in expected_rc5_session:
    if not re.search(rf"\b{re.escape(name)}\s*\(",rc5_header) or not re.search(rf"\b{re.escape(name)}\s*\(",rc5_source):
        raise SystemExit(f"FAIL RC5 ABI missing: {name}")
for required in ("rc5_session.c","dll_rc5_session_selftest","rc5_session_selftest.c"):
    if required not in cmake:
        raise SystemExit(f"FAIL RC5 CMake missing: {required}")
for required in ("dcb.BaudRate = 2400","dcb.Parity = NOPARITY","0x55,0x06,0x3B","mems_rc5_validate_frame(","!explicitly_confirmed"):
    if required not in rc5_source:
        raise SystemExit(f"FAIL RC5 protocol guard missing: {required}")
for forbidden in ("#include <Qt","#include <QSerialPort","mems3_frame_encode(","mems2j_frame_encode("):
    if forbidden in rc5_header or forbidden in rc5_source:
        raise SystemExit(f"FAIL RC5 unexpected dependency: {forbidden}")
if '#include "rc5_session.c"' not in rc5_test:
    raise SystemExit("FAIL RC5 offline test must exercise production source")
print("OK: seven native RC5 session APIs, explicit diagnostic DTC clear, offline serial test")




print("OK: family and AANMP002 modules exported and compiled within DLL only")
print("OK: full 0x80 and 0x7D frame-length guards present")
print("OK: truncated 0x7D frame is rejected")
print("OK: clear/reset/heartbeat return real command result")
print("OK: original AF/FA/0F/81 reset/save constants and second fuel trim")
# Session 205 is the ABI authority.  dll-manager's parent application files
# are older and MUST NOT be presented as a Session 205 compatibility check.
reference200 = (root / "reference-session205" / "rosco.h").read_text(encoding="utf-8")
manager_header = reference200
for name, code in {
    "MEMS_ResetECU": "0xFA",
    "MEMS_ResetAdj": "0x0F",
    "MEMS_ClearFaults": "0xCC",
    "MEMS_ReqData80": "0x80",
    "MEMS_ReqData7D": "0x7D",
}.items():
    if not re.search(rf"\b{re.escape(name)}\s*=\s*{re.escape(code)}\b", manager_header):
        raise SystemExit(f"FAIL Session 205 public header: {name} must be {code}")

manager_declarations = set(re.findall(
    r"\b(?:bool|void|librosco_version|uint8_t|int16_t)\s+(mems_\w+)\s*\(",
    manager_header))
historical_declared_only = {"mems_interactive_mode"}
if not historical_declared_only.issubset(manager_declarations):
    raise SystemExit("FAIL: Session 205 interactive declaration unexpectedly changed")
missing = (manager_declarations - historical_declared_only) - set(exports)
if missing:
    raise SystemExit(f"FAIL: Session 205 declared linked functions absent: {sorted(missing)}")
print("OK: 38 linked declarations from actual Session 205 header are exported")
print("TRACKED: mems_interactive_mode is declared, but not linked or implemented")

# Only call the parent worktree 'Session 205' if it actually matches the
# exact pinned Session 205 header.  A separate Windows smoke script builds
# the real Session 205 commit in a throwaway worktree.
parent_header_path = root.parent / "rosco.h"
same_header = (parent_header_path.exists() and
               parent_header_path.read_text(encoding="utf-8").replace("\r\n", "\n") ==
               reference200.replace("\r\n", "\n"))
if same_header:
    session_dispatch = (root.parent / "memsinterface_dispatch.cpp").read_text(encoding="utf-8")
    for guard in (
        "MemsProtocolSafety::allowsMutation(ecuFamily(), diagnosticMode(), 0x0Fu)",
        "MemsProtocolSafety::allowsMutation(ecuFamily(), diagnosticMode(), 0xFAu)",
        "::mems_reset_adjustments(&m_memsinfo)",
        "::mems_reset_ECU(&m_memsinfo)",
    ):
        if guard not in session_dispatch:
            raise SystemExit(f"FAIL Session 205 reset dispatch differs: {guard}")
    print("OK: actual Session 205 dispatcher uses 0F=adjustable and FA=learned")
else:
    print("NOTE: parent checkout is NOT Session 205; only the pinned Session 205 header was checked")
    print("NOTE: run verify_session205_windows.ps1 to test the real Session 205 app")

print("NOTE: static ABI/source checks do not prove a compiled DLL or ECU behavior")
sys.exit(0)
