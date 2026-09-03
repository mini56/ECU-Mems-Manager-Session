#!/usr/bin/env python3
from __future__ import annotations

from datetime import date
from pathlib import PurePosixPath
import re


_BARE_DOTTED_TRIPLET = re.compile(r"^\s*(\d{2})\.(\d{2})\.(\d{2})\s*$")


def is_bare_calendar_date_identifier(text: str, candidate: str) -> bool:
    """Return True when a bare xx.xx.xx token is a plausible calendar date.

    Manufacturer operation identifiers can share the same dotted shape as a
    date.  We only reject the ambiguous *bare* form.  A code accompanied by an
    explicit operation label (for example ``Service Repair No.``) is preserved
    by the caller because the complete line no longer matches this bare-date
    shape.
    """
    match = _BARE_DOTTED_TRIPLET.fullmatch(text)
    if not match or candidate != ".".join(match.groups()):
        return False
    day, month, year = (int(value) for value in match.groups())
    try:
        date(2000 + year, month, day)
    except ValueError:
        return False
    return True


def document_kind_for_source_path(source_relative_path: str) -> str:
    """Classify a RAVE document from stable corpus structure when available."""
    normalized = source_relative_path.replace("\\", "/")
    parts = [part.casefold() for part in PurePosixPath(normalized).parts]
    if "mini tech bulletins" in parts:
        return "technical_bulletin"
    return "workshop_manual"
