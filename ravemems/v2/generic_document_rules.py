#!/usr/bin/env python3
from __future__ import annotations

from datetime import date
from pathlib import PurePosixPath
import re


_BARE_DOTTED_TRIPLET = re.compile(r"^\s*(\d{2})\.(\d{2})\.(\d{2})\s*$")
_PUBLICATION_TOKEN_CLEANER = re.compile(r"[^A-Z0-9]+")


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


def canonical_publication_token(publication_code: str) -> str:
    """Return a stable corpus key token derived only from the publication code."""
    token = _PUBLICATION_TOKEN_CLEANER.sub("", publication_code.upper())
    if not token:
        raise ValueError("publication code does not contain an alphanumeric identity")
    return token


def document_kind_for_source_path(source_relative_path: str) -> str:
    """Classify a RAVE document from stable corpus structure when available."""
    normalized = source_relative_path.replace("\\", "/")
    parts = [part.casefold() for part in PurePosixPath(normalized).parts]
    if "mini tech bulletins" in parts:
        return "technical_bulletin"
    return "workshop_manual"


def document_kind_for_evidence(source_relative_path: str, evidence_text: str) -> str:
    """Classify a document from source evidence before using the path fallback.

    Document families share the same ``rave/xn`` directory, so the directory is
    not sufficient to distinguish a workshop manual from an electrical
    reference library.  Stable title wording from the source itself is stronger
    evidence and remains corpus-generic.
    """
    normalized = re.sub(r"\s+", " ", evidence_text).casefold()
    if "electrical reference library" in normalized:
        return "electrical_reference_library"
    if "workshop manual" in normalized:
        return "workshop_manual"
    return document_kind_for_source_path(source_relative_path)
