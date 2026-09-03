#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import re
import tarfile
import zipfile
from pathlib import Path
from typing import Any

import fitz


_PUBLICATION_PATTERNS = [
    re.compile(
        r"(?i)\b(?:publication(?:\s+part)?\s+no\.?|part\s+no\.?)\s*[:#-]?\s*"
        r"((?:[A-Z]{2,8})\s*\d{2,8}[A-Z]{2,8})(?:/\d+)?\b"
    ),
    re.compile(r"(?i)\b((?:RCL|AKM|LRL)\s*\d{3,6}[A-Z]{2,8})(?:/\d+)?\b"),
]
_LANGUAGE_SUFFIXES = {
    "ENG": "en", "FRE": "fr", "FRA": "fr", "FR": "fr",
    "GER": "de", "DEU": "de", "DE": "de",
    "ITA": "it", "IT": "it", "SPA": "es", "ESP": "es", "ES": "es",
    "POR": "pt", "PT": "pt", "NLD": "nl", "DUT": "nl", "NL": "nl",
}
_LANGUAGE_TITLE_HINTS = [
    (re.compile(r"(?i)\b(?:eng|english)\b"), "en"),
    (re.compile(r"(?i)\b(?:fre|fra|french|fran[cç]ais)\b"), "fr"),
    (re.compile(r"(?i)\b(?:ger|deu|german|deutsch)\b"), "de"),
    (re.compile(r"(?i)\b(?:ita|italian|italiano)\b"), "it"),
    (re.compile(r"(?i)\b(?:spa|esp|spanish|espa[nñ]ol)\b"), "es"),
    (re.compile(r"(?i)\b(?:por|portuguese|portugu[eê]s)\b"), "pt"),
    (re.compile(r"(?i)\b(?:dut|nld|dutch|nederlands)\b"), "nl"),
]


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def canonical_publication_code(value: str) -> str:
    value = value.upper().strip()
    value = re.sub(r"/\d+\s*$", "", value)
    return re.sub(r"[^A-Z0-9]+", "", value)


def detect_publication_code(text: str) -> str | None:
    candidates: list[tuple[int, str]] = []
    for pattern_index, pattern in enumerate(_PUBLICATION_PATTERNS):
        for match in pattern.finditer(text):
            code = canonical_publication_code(match.group(1))
            if code:
                candidates.append((pattern_index, code))
    if not candidates:
        return None
    candidates.sort(key=lambda item: item[0])
    return candidates[0][1]


def detect_language(publication_code: str | None, metadata_title: str, evidence_text: str) -> str:
    if publication_code:
        compact = canonical_publication_code(publication_code)
        for suffix, language in sorted(_LANGUAGE_SUFFIXES.items(), key=lambda item: len(item[0]), reverse=True):
            if compact.endswith(suffix):
                return language
    title = metadata_title or ""
    for pattern, language in _LANGUAGE_TITLE_HINTS:
        if pattern.search(title):
            return language
    sample = re.sub(r"\s+", " ", evidence_text[:20000]).casefold()
    phrase_hints = [
        ("the information", "en"), ("warning", "en"),
        ("manuel du conducteur", "fr"), ("avertissement", "fr"),
        ("betriebsanleitung", "de"), ("achtung", "de"),
        ("manuale di istruzioni", "it"), ("attenzione", "it"),
        ("manual del conductor", "es"), ("advertencia", "es"),
        ("manual do propriet", "pt"), ("atenção", "pt"),
        ("instruktieboekje", "nl"),
    ]
    scores: dict[str, int] = {}
    for phrase, language in phrase_hints:
        if phrase in sample:
            scores[language] = scores.get(language, 0) + 1
    if scores:
        return max(scores.items(), key=lambda item: item[1])[0]
    return "und"


def probe_pdf(pdf_path: Path, page_limit: int = 20) -> dict[str, Any]:
    pdf_path = pdf_path.resolve()
    source_sha256 = sha256_file(pdf_path)
    with fitz.open(pdf_path) as doc:
        page_count = doc.page_count
        metadata = dict(doc.metadata or {})
        texts = [doc[index].get_text("text", sort=True) for index in range(min(page_count, page_limit))]
    evidence_text = "\n".join(texts)
    publication_code = detect_publication_code(evidence_text)
    language = detect_language(publication_code, str(metadata.get("title") or ""), evidence_text)
    identity = publication_code or f"PDFSHA256{source_sha256[:16].upper()}"
    first_lines = [
        re.sub(r"\s+", " ", line).strip()
        for line in evidence_text.splitlines() if line.strip()
    ][:80]
    return {
        "pdf_path": str(pdf_path), "source_sha256": source_sha256, "page_count": page_count,
        "metadata": metadata, "publication_code": publication_code,
        "publication_detected": publication_code is not None, "identity": identity,
        "language": language, "first_nonempty_lines": first_lines,
    }


def _safe_member_parts(name: str) -> tuple[str, ...] | None:
    normalized = name.replace("\\", "/")
    candidate = Path(normalized)
    if candidate.is_absolute():
        return None
    parts = tuple(part for part in candidate.parts if part not in {"", "."})
    if not parts or ".." in parts:
        return None
    return parts


def _discover_directory(directory: Path) -> list[Path]:
    return sorted(
        (path.resolve() for path in directory.rglob("*") if path.is_file() and path.suffix.casefold() == ".pdf"),
        key=lambda path: str(path).casefold(),
    )


def _extract_zip(source: Path, work_dir: Path) -> list[Path]:
    root = (work_dir / "zip").resolve()
    root.mkdir(parents=True, exist_ok=True)
    extracted: list[Path] = []
    with zipfile.ZipFile(source) as archive:
        for info in archive.infolist():
            if info.is_dir() or Path(info.filename).suffix.casefold() != ".pdf":
                continue
            parts = _safe_member_parts(info.filename)
            if parts is None:
                raise ValueError(f"unsafe archive member: {info.filename!r}")
            target = root.joinpath(*parts).resolve()
            if root not in target.parents:
                raise ValueError(f"archive member escapes extraction root: {info.filename!r}")
            target.parent.mkdir(parents=True, exist_ok=True)
            with archive.open(info) as src, target.open("wb") as dst:
                while True:
                    chunk = src.read(1024 * 1024)
                    if not chunk:
                        break
                    dst.write(chunk)
            extracted.append(target)
    return sorted(extracted, key=lambda path: str(path).casefold())


def _extract_tar(source: Path, work_dir: Path) -> list[Path]:
    root = (work_dir / "tar").resolve()
    root.mkdir(parents=True, exist_ok=True)
    extracted: list[Path] = []
    with tarfile.open(source, mode="r:*") as archive:
        for member in archive.getmembers():
            if not member.isfile() or Path(member.name).suffix.casefold() != ".pdf":
                continue
            parts = _safe_member_parts(member.name)
            if parts is None:
                raise ValueError(f"unsafe archive member: {member.name!r}")
            target = root.joinpath(*parts).resolve()
            if root not in target.parents:
                raise ValueError(f"archive member escapes extraction root: {member.name!r}")
            target.parent.mkdir(parents=True, exist_ok=True)
            src = archive.extractfile(member)
            if src is None:
                continue
            with src, target.open("wb") as dst:
                while True:
                    chunk = src.read(1024 * 1024)
                    if not chunk:
                        break
                    dst.write(chunk)
            extracted.append(target)
    return sorted(extracted, key=lambda path: str(path).casefold())


def discover_pdf_sources(source: Path, work_dir: Path) -> list[Path]:
    """Resolve PDF, directory or archive; names never classify document content."""
    source = source.expanduser().resolve()
    work_dir = work_dir.expanduser().resolve()
    work_dir.mkdir(parents=True, exist_ok=True)
    if source.is_dir():
        pdfs = _discover_directory(source)
    elif source.is_file() and source.suffix.casefold() == ".pdf":
        pdfs = [source]
    elif source.is_file() and zipfile.is_zipfile(source):
        pdfs = _extract_zip(source, work_dir)
    elif source.is_file() and tarfile.is_tarfile(source):
        pdfs = _extract_tar(source, work_dir)
    elif source.is_file():
        raise ValueError(f"unsupported source file: {source}")
    else:
        raise FileNotFoundError(source)
    if not pdfs:
        raise ValueError(f"no PDF found in source: {source}")
    return pdfs


def source_kind(source: Path) -> str:
    source = source.expanduser().resolve()
    if source.is_dir(): return "directory"
    if source.is_file() and source.suffix.casefold() == ".pdf": return "pdf"
    if source.is_file() and zipfile.is_zipfile(source): return "zip"
    if source.is_file() and tarfile.is_tarfile(source): return "tar"
    return "unsupported"


def main() -> int:
    parser = argparse.ArgumentParser(description="Resolve PDF input from a PDF, directory or archive")
    parser.add_argument("source", type=Path)
    parser.add_argument("--work-dir", type=Path, default=Path(".ravemems-source"))
    parser.add_argument("--probe", action="store_true")
    args = parser.parse_args()
    pdfs = discover_pdf_sources(args.source, args.work_dir)
    result: dict[str, Any] = {"source_kind": source_kind(args.source), "pdf_count": len(pdfs), "pdfs": [str(path) for path in pdfs]}
    if args.probe:
        result["documents"] = [probe_pdf(path) for path in pdfs]
    print(json.dumps(result, ensure_ascii=False, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
