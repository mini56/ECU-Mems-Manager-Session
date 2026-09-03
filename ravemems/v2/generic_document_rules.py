#!/usr/bin/env python3
from __future__ import annotations

from datetime import date
import re


_BARE_DOTTED_TRIPLET = re.compile(r"^\s*(\d{2})\.(\d{2})\.(\d{2})\s*$")
_PUBLICATION_TOKEN_CLEANER = re.compile(r"[^A-Z0-9]+")


def is_bare_calendar_date_identifier(text: str, candidate: str) -> bool:
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
    token = _PUBLICATION_TOKEN_CLEANER.sub("", publication_code.upper())
    if not token:
        raise ValueError("document identity does not contain an alphanumeric token")
    return token


def document_kind_for_source_path(source_relative_path: str) -> str:
    _ = source_relative_path
    return "document"


def document_kind_for_evidence(source_relative_path: str, evidence_text: str, metadata_title: str = "") -> str:
    _ = source_relative_path
    normalized = re.sub(r"\s+", " ", f"{metadata_title}\n{evidence_text}").casefold()
    if re.search(r"\b(?:technical|service)\s+bulletin\b", normalized):
        return "technical_bulletin"
    if re.search(r"\belectrical\s+(?:reference\s+)?library\b", normalized):
        return "electrical_reference_library"
    if re.search(r"\bworkshop\s+manual\b", normalized):
        return "workshop_manual"
    if re.search(r"\bowner(?:['’]s|s)?\s+handbook\b", normalized):
        return "owner_handbook"
    return "document"
