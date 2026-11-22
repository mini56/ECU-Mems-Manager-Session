#!/usr/bin/env python3
"""Check both historical *binary* DLL contracts against our new DLL and Manager.

Works with the PE files themselves (not with promises or just header text).
Does not execute hardware commands or modify an ECU.
"""
import argparse
import hashlib
import re
import struct
from pathlib import Path

ORIGINAL_22_SHA256 = "a5ad466f8f1a198a6f8259a5ea5ab59775815c8cbc9637aa584db9e9c811d38f"
ORIGINAL_45_GIT_BLOB = "2b334adfd63d3eb606195ad2ac806ffe6fbd4748"
ORIGINAL_22 = {
    "mems_reset_ECU": 0xFA,
    "mems_reset_adjustments": 0x0F,
    "mems_clear_faults": 0xCC,
    "mems_heartbeat": 0xF4,
}
ORIGINAL_45 = {
    # All original one-byte named actuator/tuning commands, verified directly
    # in the published LeopoldG x86 machine-code entrypoints, not the header.
    "mems_Boost_Valve_Off": 0x0B,
    "mems_Boost_Valve_On": 0x1B,
    "mems_CloseIAC": 0xFE,
    "mems_Fan1_Off": 0x0D,
    "mems_Fan1_On": 0x1D,
    "mems_Fan2_Off": 0x0E,
    "mems_Fan2_On": 0x1E,
    "mems_O2Heater_Off": 0x09,
    "mems_O2Heater_On": 0x19,
    "mems_OpenIAC": 0xFD,
    "mems_Purge_Valve_Off": 0x08,
    "mems_Purge_Valve_On": 0x18,
    "mems_Save": 0x81,
    "mems_clear_faults": 0xCC,
    "mems_fuel_trim_minus": 0x7A,
    "mems_fuel_trim_plus": 0x79,
    "mems_heartbeat": 0xF4,
    "mems_idle_decay_minus": 0x8A,
    "mems_idle_decay_plus": 0x89,
    "mems_idle_speed_minus": 0x92,
    "mems_idle_speed_plus": 0x91,
    "mems_ignition_advance_minus": 0x94,
    "mems_ignition_advance_plus": 0x93,
    "mems_reset_ADJ": 0xFA,
    "mems_reset_ECU": 0xAF,
    "mems_reset_EMI": 0x0F,
    "mems_move_iac": 0xFB,
    "mems_read_iac_position": 0xFB,
    "mems_read": 0x80,
    "mems_read_raw": 0x80,
    "mems_init_link": 0xCA,
}


class PE:
    def __init__(self, path):
        self.path = Path(path)
        self.data = self.path.read_bytes()
        self.check(self.data[:2] == b"MZ", "MZ marker")
        pe = self.u32(0x3C)
        self.check(self.data[pe:pe + 4] == b"PE\x00\x00", "PE marker")
        self.machine = self.u16(pe + 4)
        nsections = self.u16(pe + 6)
        sizeopt = self.u16(pe + 20)
        opt = pe + 24
        magic = self.u16(opt)
        self.check(magic in (0x10B, 0x20B), "PE32/PE32+")
        self.is64 = magic == 0x20B
        self.directory = opt + (112 if self.is64 else 96)
        self.sections = []
        for i in range(nsections):
            x = opt + sizeopt + 40 * i
            virtual_size = self.u32(x + 8)
            virtual_address = self.u32(x + 12)
            raw_size = self.u32(x + 16)
            raw_offset = self.u32(x + 20)
            self.sections.append((virtual_address, max(raw_size, virtual_size), raw_offset))
        self.exports = self.read_exports()
        self.imports = self.read_imports()

    def check(self, condition, message):
        if not condition:
            raise AssertionError(f"{self.path}: {message}")

    def u16(self, offset):
        return struct.unpack_from("<H", self.data, offset)[0]

    def u32(self, offset):
        return struct.unpack_from("<I", self.data, offset)[0]

    def u64(self, offset):
        return struct.unpack_from("<Q", self.data, offset)[0]

    def offset(self, rva):
        for base, size, raw_offset in self.sections:
            if base <= rva < base + size:
                result = raw_offset + rva - base
                self.check(result < len(self.data), f"RVA {rva:x} out of file")
                return result
        raise AssertionError(f"{self.path}: missing section for RVA {rva:x}")

    def string(self, offset):
        end = self.data.index(b"\x00", offset)
        return self.data[offset:end].decode("ascii")

    def read_exports(self):
        rva = self.u32(self.directory)
        if not rva:
            return {}
        x = self.offset(rva)
        base = self.u32(x + 16)
        count = self.u32(x + 24)
        functions = self.offset(self.u32(x + 28))
        names = self.offset(self.u32(x + 32))
        ordinals = self.offset(self.u32(x + 36))
        values = {}
        for i in range(count):
            name = self.string(self.offset(self.u32(names + 4 * i)))
            ordinal_index = self.u16(ordinals + 2 * i)
            values[name] = (base + ordinal_index, self.u32(functions + 4 * ordinal_index))
        return values

    def read_imports(self):
        directory_rva = self.u32(self.directory + 8)
        if not directory_rva:
            return {}
        start = self.offset(directory_rva)
        imports = {}
        for i in range(512):
            pos = start + i * 20
            original_thunk = self.u32(pos)
            name_rva = self.u32(pos + 12)
            first_thunk = self.u32(pos + 16)
            if not (original_thunk or name_rva or first_thunk):
                break
            library = self.string(self.offset(name_rva)).lower()
            thunk = self.offset(original_thunk or first_thunk)
            width = 8 if self.is64 else 4
            mask = 1 << (width * 8 - 1)
            records = []
            for j in range(4096):
                value = self.u64(thunk + j * width) if self.is64 else self.u32(thunk + j * width)
                if value == 0:
                    break
                records.append(("ordinal", value & 0xffff) if value & mask
                               else ("name", self.string(self.offset(value) + 2)))
            imports[library] = records
        return imports

    def command(self, symbol):
        ordinal, rva = self.exports[symbol]
        start = self.offset(rva)
        # Both originals used the x86 instruction:
        # C7 44 24 04 xx 00 00 00 (mov dword [esp+4], command).
        first = self.data[start:start + 120]
        # LeopoldG's link initializer loads its first byte CA with MOV byte
        # (C6 44 24 3A CA), unlike its one-byte command wrappers (C7).
        if symbol == "mems_init_link":
            self.check(b"\xc6\x44\x24\x3a\xca" in first,
                       "LeopoldG init must begin with 0xCA")
            return 0xCA
        candidates = re.findall(rb"\xc7\x44\x24\x04(.)\x00\x00\x00", first)
        candidates = [v[0] for v in candidates if v[0] != 0xff]
        self.check(len(candidates) >= 1, f"{symbol} lacks x86 command immediate")
        return candidates[0]


def assert_equal(actual, expected, label):
    if actual != expected:
        raise AssertionError(f"{label}: got {actual!r}, expected {expected!r}")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--old22", required=True)
    parser.add_argument("--old45", required=True)
    parser.add_argument("--new", required=True)
    parser.add_argument("--manager", required=True)
    args = parser.parse_args()

    old22 = PE(args.old22)
    old45 = PE(args.old45)
    rebuilt = PE(args.new)
    manager = PE(args.manager)

    assert_equal(hashlib.sha256(old22.data).hexdigest(),
                 ORIGINAL_22_SHA256, "Manager original DLL provenance")
    blob = b"blob " + str(len(old45.data)).encode() + b"\x00" + old45.data
    assert_equal(hashlib.sha1(blob).hexdigest(), ORIGINAL_45_GIT_BLOB,
                 "LeopoldG original DLL provenance")
    assert_equal(len(old22.data), 49672, "original Manager size")
    assert_equal(len(old45.data), 63161, "original LeopoldG size")
    assert_equal(old22.machine, 0x14C, "Manager original is x86")
    assert_equal(old45.machine, 0x14C, "LeopoldG original is x86")
    assert_equal(rebuilt.machine, 0x8664, "new DLL must be AMD64")
    assert_equal(manager.machine, 0x8664, "Manager executable must be AMD64")
    assert_equal(len(old22.exports), 22, "Manager original export count")
    assert_equal(len(old45.exports), 45, "LeopoldG export count")
    assert_equal(len(rebuilt.exports), 91, "new DLL export count")
    assert_equal(rebuilt.exports["mems_reset_ECU_leopold"][0], 54, "explicit LeopoldG ECU reset ordinal")

    for name, code in ORIGINAL_22.items():
        assert_equal(old22.command(name), code, f"real Manager DLL {name}")
    for name, code in ORIGINAL_45.items():
        assert_equal(old45.command(name), code, f"real LeopoldG DLL {name}")
    for name, (ordinal, _) in old45.exports.items():
        assert_equal(rebuilt.exports[name][0], ordinal,
                     f"LeopoldG symbol/ordinal {name}")
    missing = set(old22.exports) - set(rebuilt.exports)
    assert_equal(missing, set(), "Manager original export names preserved")

    # Different x86 DLLs did not share ordinal numbering. The new Manager
    # must import by NAME; otherwise same-name functions could be misrouted.
    assert_equal(len([x for x in old22.exports if
                      old22.exports[x][0] != rebuilt.exports[x][0]]),
                 22, "old Manager and LeopoldG ordinal difference")
    assert "librosco.dll" not in manager.imports, (
        "Manager STILL imports the old incompatible librosco.dll")
    dll_imports = manager.imports.get("mems_manager_x64.dll")
    assert dll_imports, "Manager executable does not import mems_manager_x64.dll"
    # MSVC may legitimately import by ordinal when linking to a .def file.
    # The new x64 Manager is compiled against the new import library, so an
    # ordinal is valid only if it resolves to the EXPECTED rebuilt export.
    # Never compare an old 32-bit ordinal with a new x64 one numerically.
    new_by_ordinal = {ordinal: name for name, (ordinal, _) in rebuilt.exports.items()}
    assert_equal(len(new_by_ordinal), 91, "new DLL unique export ordinals")
    resolved_imports = set()
    for kind, value in dll_imports:
        if kind == "name":
            assert value in rebuilt.exports, f"Manager import name absent in DLL: {value}"
            resolved_imports.add(value)
        else:
            assert value in new_by_ordinal, f"Manager imports unknown new DLL ordinal: {value}"
            resolved_imports.add(new_by_ordinal[value])
    for symbol in ("mems_connect", "mems_init_link", "mems_read",
                   "mems_reset_ECU", "mems_reset_adjustments", "mems_disconnect"):
        assert symbol in resolved_imports, f"Manager missing resolved import {symbol}"

    # Session196 really uses mems_reset_adjustments=0F and mems_reset_ECU=FA.
    # LeopoldG uses the same FA/0F bytes under different NAMES.
    # The actual installed header/dispatcher is authoritative for ABI compatibility.
    source = Path("memsinterface_dispatch.cpp").read_text(encoding="utf-8")
    adjustment = source.split("bool MEMSInterface::guardedResetAdjustments()", 1)[1].split(
        "bool MEMSInterface::guardedResetEcu()", 1)[0]
    ecu = source.split("bool MEMSInterface::guardedResetEcu()", 1)[1].split(
        "void MEMSInterface::onProtocolCommandRequested", 1)[0]
    assert "MemsProtocolSafety::allowsMutation(ecuFamily(), diagnosticMode(), 0x0Fu)" in adjustment
    assert "return ::mems_reset_adjustments(&m_memsinfo);" in adjustment
    assert "mems_command_kline19(&m_memsinfo, 0x0Fu" in adjustment
    assert "MemsProtocolSafety::allowsMutation(ecuFamily(), diagnosticMode(), 0xFAu)" in ecu
    assert "return ::mems_reset_ECU(&m_memsinfo);" in ecu
    assert "mems_command_kline19(&m_memsinfo, 0xFAu" in ecu

    print("HISTORICAL_DLL_BINARY_COMPARE_PASS: old Manager 22 exports, LeopoldG 45, new x64 91 (family, transient-injection and 2J/3/RC5 codec extensions)")
    print("HISTORICAL_COMMANDS_PASS: old Manager ECU=FA, adjustments=0F; LeopoldG ECU=AF, ADJ=FA, EMI=0F, Save=81")
    print("MANAGER_IMPORTS_PASS: new DLL imports resolve to correct names/ordinals, no x86 librosco.dll")
    print("MANAGER_COMMAND_ROUTING_PASS: Session196 byte contract FA ECU / 0F adjustments preserved (physical meanings not proved)")
    print("NOTE: no real ECU was connected; write/reset behaviour is not verified on hardware")


if __name__ == "__main__":
    main()
