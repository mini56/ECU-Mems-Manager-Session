#!/usr/bin/env python3
"""Check physical PE32+ exports and Session 205 imports against our DLL.

This test must be executed ONLY after building the DLL and the real
Session 205 Manager executable (not dll-manager's older Manager checkout).
It performs no ECU communication and makes no changes to application code.
"""
import argparse
import re
from pathlib import Path
from audit_historical_dll_compat import PE

SESSION205_SHA = "b5cd9b61bbaed6251f17559885481b1bd03f4380"
SESSION205_HEADER_GIT_BLOB = "dd8d88048ce4e879ef728b6bcd72837730bec654"

def read_definition(path):
    declarations = []
    for line in Path(path).read_text(encoding="utf-8").splitlines():
        m = re.fullmatch(r"\s*(\w+)\s+@(\d+)\s*", line)
        if m:
            declarations.append((m.group(1), int(m.group(2))))
    return declarations

def check(new_path, manager_path, definition_path):
    dll = PE(new_path)
    app = PE(manager_path)
    expected = read_definition(definition_path)
    if len(expected) != 114:
        raise AssertionError(f"DLL definition must declare 114 exports, got {len(expected)}")
    # Original 91 names/ordinals must remain byte-exact in order.
    # Static guard validates the historical list; PE guard protects it again
    # when loading the Session 205 executable.
    if expected[90] != ("mems_rc5_fault_at", 91):
        raise AssertionError("Last historical ordinal changed")
    if expected[91] != ("mems_manager_lucas5as_slow_bit_high", 92):
        raise AssertionError("Lucas5AS first new ordinal must be 92")
    # Preserve the ABI already validated with the published Session 205.
    # First 91 historical, next nine Lucas 5AS are immutable.
    expected_new_names = [
        "mems_manager_mems19_slow_bit_high",
        "mems_manager_mems19_key_complement",
        "mems_manager_mems19_has_address_confirmation",
        "mems_manager_mems19_wakeup",
        "mems_manager_mems19_connect_identify",
    ]
    if expected[100:105] != [
        (name, ordinal) for ordinal, name
        in enumerate(expected_new_names, start=101)
    ]:
        raise AssertionError("MEMS1.9 new symbols/ordinals must be 101..105")
    mems2j_new_names = [
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
    if expected[105:] != [
        (name, ordinal) for ordinal, name
        in enumerate(mems2j_new_names, start=106)
    ]:
        raise AssertionError("MEMS2J new symbols/ordinals must be 106..114")
    if len(set(name for name, _ in expected)) != len(expected):
        raise AssertionError("Duplicate symbol in .def")
    if [ordinal for _, ordinal in expected] != list(range(1, 115)):
        raise AssertionError("Non-sequential / changed DLL export ordinals")
    if dll.machine != 0x8664 or app.machine != 0x8664:
        raise AssertionError("Both the built DLL and real Session 205 executable must be AMD64")
    found = {name: ordinal for name, (ordinal, _) in dll.exports.items()}
    if found != dict(expected):
        missing = sorted(set(dict(expected)) - set(found))
        extra = sorted(set(found) - set(dict(expected)))
        changed = sorted(n for n, ordinal in expected if n in found and found[n] != ordinal)
        raise AssertionError(f"PE export table mismatch; missing={missing}, extra={extra}, wrong_ordinals={changed}")
    if "librosco.dll" in app.imports:
        raise AssertionError("Session 205 executable still imports incompatible librosco.dll")
    imported = app.imports.get("mems_manager_x64.dll")
    if not imported:
        raise AssertionError("Session 205 executable does not import mems_manager_x64.dll")
    by_ordinal = {ordinal: name for name, ordinal in expected}
    resolved = []
    for kind, value in imported:
        if kind == "name":
            if value not in found:
                raise AssertionError(f"Session 205 imports a missing DLL symbol: {value}")
            resolved.append(value)
        elif kind == "ordinal":
            if value not in by_ordinal:
                raise AssertionError(f"Session 205 imports a missing DLL ordinal: {value}")
            resolved.append(by_ordinal[value])
        else:
            raise AssertionError(f"Unexpected PE import: {kind} {value}")
    if "mems_interactive_mode" in resolved:
        raise AssertionError("Session 205 requires mems_interactive_mode, which is not implemented")
    if not {"mems_connect", "mems_read", "mems_disconnect"}.issubset(resolved):
        raise AssertionError("Missing baseline serial/reading imports in Manager")
    print(f"SESSION205_BINARY_COMPAT_PASS: AMD64, {len(expected)} exact exports/ordinals, "
          f"{len(resolved)} resolved Manager imports, no x86 librosco.dll")
    print("SCOPE: PE linking/import ABI only; not ECU hardware or Manager UI runtime test")
    return 0

if __name__ == "__main__":
    p = argparse.ArgumentParser()
    p.add_argument("--dll", required=True)
    p.add_argument("--manager", required=True)
    p.add_argument("--def-file", dest="def_file",
                   default=str(Path(__file__).with_name("mems_manager_x64.def")))
    args = p.parse_args()
    raise SystemExit(check(args.dll, args.manager, args.def_file))
