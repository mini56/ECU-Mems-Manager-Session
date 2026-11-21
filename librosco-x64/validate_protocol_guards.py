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
    "mems_fuel_trim_2_minus"
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

if exports != expected_exports + expected_extensions:
    raise SystemExit(f"FAIL: original 45 + 8 additive exports changed: {exports}")
if len(set(exports)) != 53:
    raise SystemExit("FAIL: duplicate or missing symbols in ROSCO DLL")

required_protocol_guards = [
    "mems_read_serial(info, (uint8_t*)frame80, (uint16_t)sizeof(*frame80)) == (int16_t)sizeof(*frame80)",
    "mems_read_serial(info, (uint8_t*)frame7d, (uint16_t)sizeof(*frame7d)) != (int16_t)sizeof(*frame7d)",
    "return send_command_with_one_byte_reply(info, MEMS_ClearFaults);",
    "return send_command_with_one_byte_reply(info, MEMS_ResetECU);",
    "return send_command_with_one_byte_reply(info, MEMS_ResetAdj);",
    "return send_command_with_one_byte_reply(info, MEMS_ResetEMI);",
    "return send_command_with_one_byte_reply(info, MEMS_Save);",
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

print("OK: 45 LeopoldG ROM-tested exports + 8 extensions")
print("OK: full 0x80 and 0x7D frame-length guards present")
print("OK: truncated 0x7D frame is rejected")
print("OK: clear/reset/heartbeat return real command result")
print("OK: original AF/FA/0F/81 reset/save constants and second fuel trim")
# Check the distinct header actually consumed by the C++ application.
manager_header = (root.parent / "rosco.h").read_text(encoding="utf-8")
for name, code in {"MEMS_ResetECU": "0xAF", "MEMS_ResetAdj": "0xFA",
                   "MEMS_ResetEMI": "0x0F", "MEMS_Save": "0x81",
                   "MEMS_FuelTrim2Plus": "0x7B", "MEMS_FuelTrim2Minus": "0x7C"}.items():
    if not re.search(rf"\b{re.escape(name)}\s*=\s*{re.escape(code)}\b", manager_header):
        raise SystemExit(f"FAIL Manager public header: {name} must be {code}")
for exported in expected_exports + expected_extensions:
    if not re.search(rf"\b{re.escape(exported)}\s*\(", manager_header):
        raise SystemExit(f"FAIL Manager public header is missing export declaration: {exported}")
print("OK: Manager source header matches 45 original + 8 added DLL exports")

sys.exit(0)
