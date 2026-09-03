#!/usr/bin/env python3
from __future__ import annotations

import json
import sqlite3
import sys
from pathlib import Path
from typing import Any

import rcl0193eng_precise_visual_extract as precise
from generic_document_rules import document_kind_for_source_path, is_bare_calendar_date_identifier


pe = precise.pe


class GenericSemanticParser(precise.pass2.Pass2SemanticParser):
    """PASS2 semantic parser plus corpus-generic document identity guards."""

    def _operation_code(self, text: str) -> str | None:
        code = super()._operation_code(text)
        if code and is_bare_calendar_date_identifier(text, code):
            return None
        return code


pe.SemanticParser = GenericSemanticParser


def _argument_value(name: str) -> str:
    try:
        index = sys.argv.index(name)
    except ValueError as exc:
        raise SystemExit(f"missing required argument {name}") from exc
    if index + 1 >= len(sys.argv):
        raise SystemExit(f"missing value for argument {name}")
    return sys.argv[index + 1]


def main() -> int:
    out = Path(_argument_value("--out"))
    source_relative_path = _argument_value("--source-relative-path")
    result = pe.main()
    if result != 0:
        return result

    document_kind = document_kind_for_source_path(source_relative_path)
    db_path = out / "ravemems_v2_rcl0193eng.sqlite"
    db = sqlite3.connect(db_path)
    db.execute("UPDATE ravemems_document SET document_kind=?", (document_kind,))
    db.commit()
    db.close()

    manifest_path = out / "manifest.json"
    manifest: dict[str, Any] = json.loads(manifest_path.read_text(encoding="utf-8"))
    manifest["document_kind"] = document_kind
    manifest_path.write_text(
        json.dumps(manifest, ensure_ascii=False, indent=2) + "\n",
        encoding="utf-8",
    )
    print("RAVEMEMS_DOCUMENT_KIND", document_kind)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
