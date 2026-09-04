from __future__ import annotations

from hashlib import sha256
from pathlib import Path

REPORT = Path("RAPPORT_SUIVI_ECU_MEMS_MANAGER.md")
QUEUE = Path(".journal_queue")


def _decode_single_legacy_byte(value: int) -> bytes:
    raw = bytes([value])
    try:
        text = raw.decode("cp1252")
    except UnicodeDecodeError:
        text = raw.decode("latin-1")
    return text.encode("utf-8")


def repair_invalid_utf8(data: bytes) -> tuple[bytes, list[tuple[int, int, bytes]]]:
    """Repair only bytes that make the stream invalid UTF-8.

    Already-valid UTF-8 bytes are copied byte-for-byte unchanged. Each isolated
    offending byte is interpreted as Windows-1252, with Latin-1 fallback, then
    re-encoded as UTF-8.
    """
    out = bytearray()
    repairs: list[tuple[int, int, bytes]] = []
    cursor = 0

    while cursor < len(data):
        try:
            data[cursor:].decode("utf-8", errors="strict")
        except UnicodeDecodeError as exc:
            bad_pos = cursor + exc.start
            out.extend(data[cursor:bad_pos])
            bad_byte = data[bad_pos]
            replacement = _decode_single_legacy_byte(bad_byte)
            out.extend(replacement)
            repairs.append((bad_pos, bad_byte, replacement))
            cursor = bad_pos + 1
        else:
            out.extend(data[cursor:])
            break

    repaired = bytes(out)
    repaired.decode("utf-8", errors="strict")
    return repaired, repairs


def main() -> None:
    if not REPORT.is_file():
        raise SystemExit("master report missing")

    original_report = REPORT.read_bytes()
    report, repairs = repair_invalid_utf8(original_report)

    entries = sorted(p for p in QUEUE.glob("*.md") if p.is_file()) if QUEUE.exists() else []
    appended = 0

    for path in entries:
        entry = path.read_bytes().strip()
        if not entry:
            raise SystemExit(f"empty journal entry: {path}")
        try:
            entry.decode("utf-8")
        except UnicodeDecodeError as exc:
            raise SystemExit(f"journal entry is not UTF-8: {path}: {exc}") from exc

        digest = sha256(entry).hexdigest()
        marker = f"<!-- journal-entry-sha256:{digest} -->".encode("ascii")
        if marker not in report:
            if report and not report.endswith(b"\n"):
                report += b"\n"
            report += b"\n" + entry + b"\n" + marker + b"\n"
            appended += 1
        path.unlink()

    # Final strict validation before and after writing the canonical report.
    report.decode("utf-8", errors="strict")
    REPORT.write_bytes(report)
    REPORT.read_bytes().decode("utf-8", errors="strict")

    for path in entries:
        if path.exists():
            raise SystemExit(f"queue cleanup failed: {path}")

    if repairs:
        details = ",".join(
            f"offset={pos}:0x{value:02X}->{replacement.hex()}"
            for pos, value, replacement in repairs
        )
        print(f"MASTER_REPORT_UTF8_REPAIRED count={len(repairs)} {details}")

    if not entries and not repairs:
        print("MASTER_REPORT_QUEUE_EMPTY_UTF8_OK")
        return

    print(
        f"MASTER_REPORT_APPEND_PASS entries={appended} repairs={len(repairs)} "
        f"sha256={sha256(report).hexdigest()}"
    )


if __name__ == "__main__":
    main()
