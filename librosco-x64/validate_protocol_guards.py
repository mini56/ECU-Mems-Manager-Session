from pathlib import Path
import re
import sys

root = Path(__file__).resolve().parent
protocol = (root / "protocol.c").read_text(encoding="utf-8")
header = (root / "rosco.h").read_text(encoding="utf-8")
definition = (root / "mems_manager_x64.def").read_text(encoding="utf-8")

expected_exports = [
    "mems_cleanup", "mems_clear_faults", "mems_connect", "mems_disconnect",
    "mems_get_lib_version", "mems_heartbeat", "mems_init", "mems_init_link",
    "mems_is_connected", "mems_lock", "mems_move_iac", "mems_openserial",
    "mems_read", "mems_read_iac_position", "mems_read_raw", "mems_read_serial",
    "mems_reset_ECU", "mems_reset_adjustments", "mems_send_command",
    "mems_test_actuator", "mems_unlock", "mems_write_serial",
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

if exports != expected_exports:
    raise SystemExit(f"FAIL exports: expected 22 historical symbols, got {exports}")

required_protocol_guards = [
    "mems_read_serial(info, (uint8_t*)frame80, (uint16_t)sizeof(*frame80)) == (int16_t)sizeof(*frame80)",
    "mems_read_serial(info, (uint8_t*)frame7d, (uint16_t)sizeof(*frame7d)) != (int16_t)sizeof(*frame7d)",
    "return send_command_with_one_byte_reply(info, MEMS_ClearFaults);",
    "return send_command_with_one_byte_reply(info, MEMS_ResetECU);",
    "return send_command_with_one_byte_reply(info, MEMS_ResetAdj);",
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
    "MEMS_ResetAdj": "0x0F",
    "MEMS_ResetECU": "0xFA",
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

print("OK: 22 historical exports")
print("OK: full 0x80 and 0x7D frame-length guards present")
print("OK: truncated 0x7D frame is rejected")
print("OK: clear/reset/heartbeat return real command result")
print("OK: historical command constants preserved")
sys.exit(0)
