#!/usr/bin/env python3
from __future__ import annotations

import importlib.util
import re
from pathlib import Path
from typing import Any

import prototype_extract as pe


PASS2_PATH = Path(__file__).resolve().parents[1] / "pass2" / "rcl0193eng_zero_defect_pass2.py"
spec = importlib.util.spec_from_file_location("ravemems_pass2", PASS2_PATH)
if spec is None or spec.loader is None:
    raise RuntimeError(f"cannot load PASS2 wrapper: {PASS2_PATH}")
pass2 = importlib.util.module_from_spec(spec)
spec.loader.exec_module(pass2)


class BoundaryCleanSemanticParser(pass2.Pass2SemanticParser):
    """Prevent the next operation title from becoming previous-step text.

    RCL0193ENG prints a new operation title before its manufacturer operation
    number. The title is therefore seen while the previous step is still open.
    At the instant the following operation number is recognized, the same title
    has become authoritative structural evidence. Remove only that exact suffix
    (and an immediately preceding printed page/column number) before the normal
    operation transition flushes the old step.
    """

    @staticmethod
    def _boundary_clean_text(text: str, next_title: str) -> str:
        text = re.sub(r"\s+", " ", text).strip()
        title = re.sub(r"\s+", " ", next_title).strip()
        if not text or not title:
            return text
        title_pattern = re.escape(title).replace(r"\ ", r"\s+")
        pattern = re.compile(
            rf"(?:\s+\d{{1,3}})?\s+{title_pattern}\s*$",
            re.IGNORECASE,
        )
        return pattern.sub("", text).strip()

    def _new_operation(
        self,
        code: str,
        title: str,
        page_key: str,
        bbox: tuple[float, float, float, float],
    ) -> str:
        if self.current_step and title.strip():
            original = re.sub(r"\s+", " ", " ".join(self.current_step.get("parts", []))).strip()
            cleaned = self._boundary_clean_text(original, title)
            if cleaned != original:
                self.current_step["parts"] = [cleaned] if cleaned else []
        return super()._new_operation(code, title, page_key, bbox)


pe.SemanticParser = BoundaryCleanSemanticParser


if __name__ == "__main__":
    raise SystemExit(pe.main())
