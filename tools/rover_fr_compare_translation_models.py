#!/usr/bin/env python3
"""Compare two offline English/French translators on the same Rover source IDs.

QA benchmark only: it NEVER imports new translations, claims manufacturer
approval, or releases a Setup.  The original corpus remains read-only.
"""
from __future__ import annotations

import argparse
import html
import json
import random
import sqlite3
from pathlib import Path

from rover_multilingual import sha, units_from_source, validate_translation


def read_baseline(path: Path) -> dict:
    records = {}
    for line in path.read_text(encoding="utf-8").splitlines():
        if line.strip():
            record = json.loads(line)
            records[record["unit_id"]] = record
    return records


def sample_units(units: dict, baseline: dict, count: int) -> list:
    # Fixed seed and length diversity prevent cherry-picking favorable terms.
    eligible = [uid for uid in baseline if uid in units]
    by_length = sorted(eligible, key=lambda uid: len(units[uid][2]))
    randomizer = random.Random(482)
    if count >= len(by_length):
        return by_length
    groups = [
        by_length[:len(by_length)//3],
        by_length[len(by_length)//3:2*len(by_length)//3],
        by_length[2*len(by_length)//3:],
    ]
    selected = []
    for group in groups:
        randomizer.shuffle(group)
        selected.extend(group[:count // 3])
    if len(selected) < count:
        for uid in by_length:
            if uid not in selected:
                selected.append(uid)
                if len(selected) >= count:
                    break
    return selected


def evaluate(source: str, text: str) -> dict:
    try:
        validate_translation(source, text)
        if text.casefold().strip() == source.casefold().strip():
            raise ValueError("UNTRANSLATED_ENGLISH_SOURCE")
        return {"passes_numeric_unit_checks": True, "issue": ""}
    except ValueError as error:
        return {"passes_numeric_unit_checks": False, "issue": str(error)}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--database", type=Path, required=True)
    parser.add_argument("--opus-checkpoint", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--count", type=int, default=18)
    parser.add_argument("--model-id", default="tencent/Hy-MT2-1.8B-GGUF")
    parser.add_argument("--filename", default="Hy-MT2-1.8B-Q4_K_M.gguf")
    args = parser.parse_args()
    with sqlite3.connect(f"file:{args.database.resolve()}?mode=ro", uri=True) as connection:
        units = {uid: (kind, owner, original) for uid, kind, owner, original
                 in units_from_source(connection)}
    baseline = read_baseline(args.opus_checkpoint)
    chosen = sample_units(units, baseline, args.count)
    if not chosen:
        raise ValueError("MISSING_SOURCE_BOUND_OPUS_BASELINE")
    from huggingface_hub import hf_hub_download
    from llama_cpp import Llama
    model_path = hf_hub_download(repo_id=args.model_id, filename=args.filename)
    model = Llama(model_path=model_path, n_ctx=2048, n_threads=4,
                  n_batch=512, verbose=False)
    results = []
    for index, uid in enumerate(chosen, 1):
        kind, owner, source = units[uid]
        if baseline[uid]["source_sha256"] != sha(source):
            raise ValueError("MODEL_COMPARISON_ORIGINAL_SOURCE_MISMATCH")
        prompt = (
            "Translate the following automotive workshop manual text into French. "
            "Use correct technical mechanical workshop terminology. "
            "Do not add an explanation or commentary. "
            "Keep every manufacturer's part identifier, numerical value, "
            "decimal separator, and measurement unit exactly unchanged. "
            "Only output the translated French text.\n\n"
            + source
        )
        reply = model.create_chat_completion(
            messages=[{"role": "user", "content": prompt}],
            max_tokens=250,
            temperature=0.2,
            top_p=0.6,
        )
        alternative = reply["choices"][0]["message"]["content"].strip()
        comparative = {
            "unit_id": uid,
            "unit_kind": kind,
            "source_sha256": sha(source),
            "original_english": source,
            "opus_fr": baseline[uid].get("translated_text", ""),
            "opus_status": baseline[uid]["status"],
            "hymt2_fr": alternative,
            **evaluate(source, alternative),
            "review_status": "NOT_REVIEWED",
        }
        results.append(comparative)
        good = sum(row["passes_numeric_unit_checks"] for row in results)
        print(f"ROVER_FR_MODEL_COMPARISON {index}/{len(chosen)} | "
              f"passed_numeric_checks={good} | technically_reviewed=0",
              flush=True)
    report = {
        "translation_language": "fr",
        "source_units_compared": len(results),
        "source_method": "seeded_length_stratified_baseline_sample",
        "baseline": "Helsinki-NLP/opus-mt-en-fr",
        "alternative": args.model_id + ":" + args.filename,
        "automatically_approved": 0,
        "manually_approved": 0,
        "comparisons": results,
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, ensure_ascii=False, indent=2),
                           encoding="utf-8")
    summary = __import__("os").environ.get("GITHUB_STEP_SUMMARY")
    if summary:
        with open(summary, "a", encoding="utf-8") as stream:
            stream.write(
                "## Contrôle comparatif de la traduction technique Rover\n\n"
                "L'acceptation de chiffres et d'unités ne valide pas le sens "
                "mécanique. Aucun résultat ne passe automatiquement en « relu ».\n\n"
                "| Original constructeur | OPUS-MT | Hy-MT2 | Valeurs préservées |\n"
                "|---|---|---|---|\n"
            )
            for row in results:
                cells = [
                    row["original_english"], row["opus_fr"] or "(rejeté)",
                    row["hymt2_fr"],
                    "oui" if row["passes_numeric_unit_checks"] else "non",
                ]
                clean = [html.escape(str(v).replace("\n", " ")[:160])
                         .replace("|", "&#124;") for v in cells]
                stream.write("| " + " | ".join(clean) + " |\n")
    print("ROVER_FR_MODEL_COMPARISON_FINISHED",
          json.dumps({"compared": len(results),
                      "numeric_ok": sum(r["passes_numeric_unit_checks"]
                                        for r in results),
                      "technically_reviewed": 0}),
          flush=True)


if __name__ == "__main__":
    main()
