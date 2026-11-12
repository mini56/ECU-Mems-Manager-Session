#!/usr/bin/env python3
"""Reproducible, resumable *draft* English -> French Rover translation batches.

This utility is intentionally separate from the installed MEMS application.
It never touches original manufacturer SQLite tables, does not manufacture a
"reviewed" status, and cannot publish a user Setup.  All generated translations
are source-bound drafts and remain subject to a technical human review.
"""
from __future__ import annotations

import argparse
import json
import os
import re
import sqlite3
import sys
import tempfile
from pathlib import Path

from rover_multilingual import (
    SCHEMA_VERSION, check_localized_schema, import_pack, sha,
    units_from_source, validate_translation,
)

MODEL = "Helsinki-NLP/opus-mt-en-fr"
KIND_PRIORITY = {
    "procedure_title": 0,
    "procedure_step": 1,
    "candidate_title": 2,
    "candidate_step": 3,
    "page_ocr": 4,
}


def read_checkpoint(path: Path, units: dict) -> dict:
    stored = {}
    if not path.is_file():
        return stored
    with path.open("r", encoding="utf-8") as stream:
        for number, line in enumerate(stream, 1):
            if not line.strip():
                continue
            record = json.loads(line)
            uid = record.get("unit_id")
            if uid not in units:
                raise ValueError(f"CHECKPOINT_HAS_UNKNOWN_SOURCE at line {number}")
            kind, owner, source = units[uid]
            if record.get("source_sha256") != sha(source):
                raise ValueError(f"CHECKPOINT_SOURCE_CHANGED {uid}")
            if record.get("status") not in ("draft", "rejected"):
                raise ValueError(f"CHECKPOINT_BAD_STATUS {uid}")
            if record["status"] == "draft":
                validate_translation(source, record["translated_text"])
            stored[uid] = record
    return stored


def percent(number: int, total: int) -> str:
    return f"{100.0 * number / total:.2f}%" if total else "0.00%"


def snapshot(records: dict, total: int, report: Path, source_sha: str,
             model: str, *, review_count: int = 0) -> dict:
    accepted = sum(r["status"] == "draft" for r in records.values())
    rejected = sum(r["status"] == "rejected" for r in records.values())
    processed = accepted + rejected
    values = {
        "language": "fr",
        "source_units": total,
        "original_sqlite_sha256": source_sha,
        "model": model,
        "processed_units": processed,
        "draft_translated": accepted,
        "draft_percent": round(100 * accepted / total, 2) if total else 0,
        "reviewed": review_count,
        "reviewed_percent": round(100 * review_count / total, 2) if total else 0,
        "rejected_for_manual_attention": rejected,
        "remaining_unprocessed": total - processed,
        "ready_for_user_testing": False,
    }
    report.parent.mkdir(parents=True, exist_ok=True)
    temp = report.with_suffix(report.suffix + ".tmp")
    temp.write_text(json.dumps(values, ensure_ascii=False, sort_keys=True, indent=2),
                    encoding="utf-8")
    temp.replace(report)
    print(
        f"ROVER_FR_PROGRESS | processed {processed}/{total} "
        f"({percent(processed,total)}) | machine drafts {accepted}/{total} "
        f"({percent(accepted,total)}) | reviewed {review_count}/{total} "
        f"({percent(review_count,total)}) | rejected {rejected}",
        flush=True,
    )
    return values


def split_for_model(source: str, tokenizer, max_tokens: int = 330) -> list[str]:
    """Split long OCR paragraphs without truncating their source text."""
    if len(tokenizer.encode(source, add_special_tokens=True)) <= max_tokens:
        return [source]
    words = re.findall(r"\S+\s*", source)
    chunks, current = [], ""
    for word in words:
        candidate = current + word
        if current and len(tokenizer.encode(candidate, add_special_tokens=True)) > max_tokens:
            chunks.append(current.strip())
            current = word
        else:
            current = candidate
        if len(tokenizer.encode(current, add_special_tokens=True)) > max_tokens:
            # An unbreakable OCR fragment must be reviewed; never truncate it.
            raise ValueError("OCR_FRAGMENT_EXCEEDS_TRANSLATION_INPUT")
    if current.strip():
        chunks.append(current.strip())
    if not chunks:
        raise ValueError("EMPTY_TRANSLATION_CHUNKS")
    return chunks


class MarianTranslator:
    def __init__(self, model_name: str):
        import torch
        from transformers import MarianMTModel, MarianTokenizer
        self.torch = torch
        self.tokenizer = MarianTokenizer.from_pretrained(model_name)
        self.model = MarianMTModel.from_pretrained(model_name)
        self.model.eval()
        # Prevent excessive CPU thread contention on the hosted runner.
        torch.set_num_threads(min(4, max(1, os.cpu_count() or 1)))

    def batch(self, sources: list[str]) -> list[str]:
        tokenizer, torch = self.tokenizer, self.torch
        pieces: list[str] = []
        owners: list[int] = []
        for index, source in enumerate(sources):
            for chunk in split_for_model(source, tokenizer):
                pieces.append(chunk)
                owners.append(index)
        translations = [[] for _ in sources]
        for offset in range(0, len(pieces), 8):
            current = pieces[offset: offset + 8]
            encoded = tokenizer(
                current, return_tensors="pt", padding=True, truncation=False
            )
            with torch.inference_mode():
                generated = self.model.generate(
                    **encoded,
                    num_beams=3,
                    max_new_tokens=440,
                    early_stopping=True,
                )
            texts = tokenizer.batch_decode(generated, skip_special_tokens=True)
            for index, text in enumerate(texts, offset):
                translations[owners[index]].append(text.strip())
        return [" ".join(parts) for parts in translations]


def export_pack(path: Path, records: dict, units: dict, source_sha: str) -> dict:
    entries = []
    for uid in sorted(records):
        record = records[uid]
        if record["status"] != "draft":
            continue
        kind, owner, source = units[uid]
        entries.append({
            "unit_id": uid,
            "source_sha256": sha(source),
            "translated_text": record["translated_text"],
            "review_status": "machine_draft",
            "reviewer": "",
        })
    pack = {
        "schema_version": SCHEMA_VERSION,
        "original_sqlite_sha256": source_sha,
        "language": "fr",
        "entries": entries,
    }
    path.parent.mkdir(parents=True, exist_ok=True)
    partial = path.with_suffix(path.suffix + ".tmp")
    partial.write_text(json.dumps(pack, ensure_ascii=False, indent=2), encoding="utf-8")
    partial.replace(path)
    return pack


def check_pack_does_not_change_source(database: Path, pack: dict, original_sha: str):
    # A disposable database is the only place where machine drafts are
    # imported for integration tests. The actual corpus remains immutable.
    with tempfile.TemporaryDirectory() as directory:
        sample = Path(directory) / "review_pack.sqlite"
        import shutil
        shutil.copy2(database, sample)
        with sqlite3.connect(sample) as connection:
            imported = import_pack(connection, pack)
            status = check_localized_schema(connection, original_sha)
            if imported != len(pack["entries"]) or \
                    status["coverage"].get("fr", 0) != len(pack["entries"]):
                raise ValueError("FRENCH_DRAFT_PACK_IMPORT_COUNT_MISMATCH")
            if status["french"]["reviewed"] != 0:
                raise ValueError("MACHINE_DRAFTS_MUST_NEVER_SELF_APPROVE")
    print(f"ROVER_FR_DRAFT_SQLITE_IMPORT_OK {imported}", flush=True)


def write_summary(summary: dict) -> None:
    target = os.environ.get("GITHUB_STEP_SUMMARY")
    if not target:
        return
    with open(target, "a", encoding="utf-8") as stream:
        stream.write(
            "## Rover — traduction française (brouillons uniquement)\n\n"
            "| Indicateur | Progression réelle |\n"
            "|---|---:|\n"
            f"| Textes sources | {summary['source_units']:,} |\n"
            f"| Traduits automatiquement | {summary['draft_translated']:,}"
            f" ({summary['draft_percent']:.2f} %) |\n"
            f"| Relus et validés techniquement | {summary['reviewed']:,}"
            f" ({summary['reviewed_percent']:.2f} %) |\n"
            f"| Rejetés à examiner | {summary['rejected_for_manual_attention']:,} |\n"
            f"| En attente de traduction | {summary['remaining_unprocessed']:,} |\n\n"
            "**Aucun Setup utilisateur ni validation technique automatique.**\n"
        )


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--database", type=Path, required=True)
    parser.add_argument("--checkpoint", type=Path, required=True)
    parser.add_argument("--progress", type=Path, required=True)
    parser.add_argument("--pack", type=Path, required=True)
    parser.add_argument("--max-new-units", type=int, default=100)
    parser.add_argument("--batch-size", type=int, default=6)
    parser.add_argument("--model", default=MODEL)
    parser.add_argument("--verify-only", action="store_true")
    args = parser.parse_args()
    if args.max_new_units < 0 or args.batch_size < 1:
        parser.error("Invalid max-new-units or batch-size.")
    connection = sqlite3.connect(f"file:{args.database.resolve()}?mode=ro", uri=True)
    try:
        meta = dict(connection.execute(
            "SELECT key,value FROM documentary_language_meta"
        ))
        source_sha = meta["original_sqlite_sha256"]
        if meta["schema_version"] != str(SCHEMA_VERSION):
            raise ValueError("INVALID_MULTILINGUAL_SOURCE_SCHEMA")
        units = {uid: (kind, owner, original) for uid, kind, owner, original
                 in units_from_source(connection)}
        source_status = check_localized_schema(connection, source_sha)
        total = source_status["source_units"]
        if total != len(units):
            raise ValueError("SOURCE_UNIT_KEY_COLLISION")
        records = read_checkpoint(args.checkpoint, units)
        progress = snapshot(records, total, args.progress, source_sha, args.model,
                            review_count=source_status["french"]["reviewed"])
        if args.verify_only:
            pack = export_pack(args.pack, records, units, source_sha)
            check_pack_does_not_change_source(args.database, pack, source_sha)
            write_summary(progress)
            return
        to_process = [
            (uid, kind, original) for uid, (kind, owner, original) in units.items()
            if uid not in records
        ]
        to_process.sort(key=lambda e: (
            KIND_PRIORITY.get(e[1], 100), len(e[2]), e[0]
        ))
        if args.max_new_units:
            to_process = to_process[:args.max_new_units]
        if not to_process:
            print("ROVER_FR_NO_NEW_SOURCE_UNITS", flush=True)
            write_summary(progress)
            return
        print("ROVER_FR_TRANSLATOR_START",
              json.dumps({"model": args.model, "new_units": len(to_process),
                          "total": total}, sort_keys=True), flush=True)
        generator = MarianTranslator(args.model)
        args.checkpoint.parent.mkdir(parents=True, exist_ok=True)
        with args.checkpoint.open("a", encoding="utf-8", buffering=1) as stream:
            for offset in range(0, len(to_process), args.batch_size):
                chunk = to_process[offset:offset + args.batch_size]
                try:
                    translated = generator.batch([item[2] for item in chunk])
                    if len(translated) != len(chunk):
                        raise ValueError("MODEL_BATCH_RESULT_MISMATCH")
                    outputs = [(text, "") for text in translated]
                except Exception as error:
                    # Do not silently invent a translation after model failure.
                    outputs = [("", f"GENERATION_FAILED:{type(error).__name__}:"
                                    f"{str(error)[:160]}") for _ in chunk]
                for (uid, kind, original), (translated, reason) in zip(chunk, outputs):
                    if not reason:
                        try:
                            validate_translation(original, translated)
                            if translated.casefold().strip() == original.casefold().strip():
                                raise ValueError("NO_TRANSLATION_OUTPUT")
                        except Exception as error:
                            reason = f"{type(error).__name__}: {str(error)[:160]}"
                    record = {
                        "unit_id": uid, "unit_kind": kind,
                        "source_sha256": sha(original),
                        "status": "rejected" if reason else "draft",
                    }
                    if reason:
                        record["reason"] = reason
                    else:
                        record["translated_text"] = translated
                    stream.write(json.dumps(record, ensure_ascii=False) + "\n")
                    records[uid] = record
                stream.flush()
                progress = snapshot(records, total, args.progress, source_sha,
                                    args.model,
                                    review_count=source_status["french"]["reviewed"])
                export_pack(args.pack, records, units, source_sha)
        pack = export_pack(args.pack, records, units, source_sha)
        check_pack_does_not_change_source(args.database, pack, source_sha)
        write_summary(progress)
    finally:
        connection.close()


if __name__ == "__main__":
    try:
        main()
    except Exception as error:
        print(f"ROVER_FR_TRANSLATOR_ERROR {type(error).__name__}: {error}",
              file=sys.stderr, flush=True)
        raise
