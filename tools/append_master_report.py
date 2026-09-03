from __future__ import annotations

from hashlib import sha256
from pathlib import Path

REPORT = Path("RAPPORT_SUIVI_ECU_MEMS_MANAGER.md")
QUEUE = Path(".journal_queue")


def main() -> None:
    if not REPORT.is_file():
        raise SystemExit("master report missing")
    entries = sorted(p for p in QUEUE.glob("*.md") if p.is_file()) if QUEUE.exists() else []
    if not entries:
        print("MASTER_REPORT_QUEUE_EMPTY")
        return

    report = REPORT.read_bytes()
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

    REPORT.write_bytes(report)

    for path in entries:
        if path.exists():
            raise SystemExit(f"queue cleanup failed: {path}")

    print(f"MASTER_REPORT_APPEND_PASS entries={appended} sha256={sha256(report).hexdigest()}")


if __name__ == "__main__":
    main()
