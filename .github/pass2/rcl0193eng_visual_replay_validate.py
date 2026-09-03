#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import sqlite3
import tempfile
from pathlib import Path
from typing import Any

import fitz

import rcl0193eng_zero_defect_pass2 as pass2

pe = pass2.pe


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def rect_values(rect: fitz.Rect) -> list[float]:
    return [float(v) for v in rect]


def rect_close(stored_json: str | None, actual: fitz.Rect, tolerance: float = 1e-6) -> bool:
    stored = json.loads(stored_json or "null")
    if not isinstance(stored, list) or len(stored) != 4:
        return False
    return all(abs(float(a) - float(b)) <= tolerance for a, b in zip(stored, actual))


def save_png_bytes(pixmap: fitz.Pixmap) -> bytes:
    handle = tempfile.NamedTemporaryFile(suffix=".png", delete=False)
    temp_path = Path(handle.name)
    handle.close()
    try:
        pixmap.save(str(temp_path))
        return temp_path.read_bytes()
    finally:
        temp_path.unlink(missing_ok=True)


def replay_visuals(pdf_path: Path, out_dir: Path) -> tuple[list[dict[str, Any]], list[dict[str, Any]]]:
    db_path = out_dir / "ravemems_v2_rcl0193eng.sqlite"
    db = sqlite3.connect(db_path)
    doc = fitz.open(pdf_path)
    verified: list[dict[str, Any]] = []
    failed: list[dict[str, Any]] = []
    seen_keys: set[str] = set()

    try:
        expected_total = db.execute("SELECT COUNT(*) FROM ravemems_visual").fetchone()[0]
        for index in range(doc.page_count):
            page = doc[index]
            physical_page = index + 1
            page_key = f"PAGE_{physical_page:04d}"

            # Replay the page-state preparation used by prototype_extract.main().
            page.get_text("text", sort=True)
            pe.read_lines(page)
            candidates = pe.visual_candidate_rects(page)

            replay_ordinal = 0
            for source_rect in candidates:
                clip = pe.expanded_clip(source_rect, page.rect)
                if clip.is_empty or clip.width < 20 or clip.height < 20:
                    continue
                replay_ordinal += 1
                visual_key = f"VIS_P{physical_page:04d}_{replay_ordinal:03d}"
                seen_keys.add(visual_key)
                row = db.execute(
                    "SELECT relative_path,sha256,width,height,render_method,source_bbox_json,crop_bbox_json "
                    "FROM ravemems_visual WHERE visual_key=? AND page_key=?",
                    (visual_key, page_key),
                ).fetchone()
                evidence: dict[str, Any] = {
                    "visual_key": visual_key,
                    "page_key": page_key,
                    "physical_page": physical_page,
                    "replay_source_bbox": rect_values(source_rect),
                    "replay_crop_bbox": rect_values(clip),
                }
                try:
                    if row is None:
                        raise ValueError("replayed visual missing from semantic DB")
                    relative_path, stored_sha, width, height, render_method, source_json, crop_json = row
                    if render_method != "pdf_page_render_crop":
                        raise ValueError(f"unexpected render_method={render_method}")
                    if not rect_close(source_json, source_rect):
                        raise ValueError(f"source bbox mismatch stored={source_json} replay={rect_values(source_rect)}")
                    if not rect_close(crop_json, clip):
                        raise ValueError(f"crop bbox mismatch stored={crop_json} replay={rect_values(clip)}")

                    pix = page.get_pixmap(matrix=fitz.Matrix(1.5, 1.5), clip=clip, alpha=False)
                    if (pix.width, pix.height) != (int(width), int(height)):
                        raise ValueError(
                            f"dimension mismatch stored={(width,height)} replay={(pix.width,pix.height)}"
                        )
                    replay_png = save_png_bytes(pix)
                    replay_sha = sha256(replay_png)
                    asset_path = out_dir / str(relative_path)
                    asset_png = asset_path.read_bytes()
                    asset_sha = sha256(asset_png)
                    if asset_sha != stored_sha:
                        raise ValueError(f"asset sha mismatch db={stored_sha} file={asset_sha}")
                    if replay_png != asset_png:
                        raise ValueError(
                            f"exact extraction replay mismatch replay_sha={replay_sha} asset_sha={asset_sha}"
                        )

                    evidence.update(
                        {
                            "status": "verified",
                            "asset_sha256": asset_sha,
                            "replay_sha256": replay_sha,
                            "width": pix.width,
                            "height": pix.height,
                        }
                    )
                    verified.append(evidence)
                except Exception as exc:
                    evidence.update({"status": "failed", "reason": str(exc)})
                    failed.append(evidence)

        db_keys = {str(row[0]) for row in db.execute("SELECT visual_key FROM ravemems_visual")}
        missing_replay = sorted(db_keys - seen_keys)
        unexpected_replay = sorted(seen_keys - db_keys)
        if missing_replay:
            failed.append({"status": "failed", "reason": "DB visuals absent from replay", "visual_keys": missing_replay})
        if unexpected_replay:
            failed.append({"status": "failed", "reason": "replayed visuals absent from DB", "visual_keys": unexpected_replay})

        summary = {
            "method": "full extraction-order replay page 1..372 with get_text, read_lines, visual_candidate_rects, expanded_clip, Matrix(1.5,1.5), Pixmap.save",
            "db_visual_count": int(expected_total),
            "replayed_visual_count": len(seen_keys),
            "verified_count": len(verified),
            "failed_count": len(failed),
            "missing_replay_count": len(missing_replay),
            "unexpected_replay_count": len(unexpected_replay),
            "verified": verified,
            "failed": failed,
        }
        (out_dir / "visual_replay_validation.json").write_text(
            json.dumps(summary, ensure_ascii=False, indent=2), encoding="utf-8"
        )

        print("RCL0193ENG_REPLAY_DB_VISUALS", expected_total)
        print("RCL0193ENG_REPLAY_GENERATED_VISUALS", len(seen_keys))
        print("RCL0193ENG_REPLAY_VERIFIED", len(verified))
        print("RCL0193ENG_REPLAY_FAILED", len(failed))
        print("RCL0193ENG_REPLAY_MISSING", len(missing_replay))
        print("RCL0193ENG_REPLAY_UNEXPECTED", len(unexpected_replay))
        if expected_total != 738 or len(seen_keys) != 738 or len(verified) != 738 or failed:
            return verified, failed
        print("RAVEMEMS_V2_RCL0193ENG_EXACT_VISUAL_REPLAY_PASS")
        return verified, failed
    finally:
        db.close()
        doc.close()


def main() -> int:
    parser = argparse.ArgumentParser(description="Replay RCL0193ENG visual extraction in exact page order")
    parser.add_argument("--pdf", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    args = parser.parse_args()
    verified, failed = replay_visuals(args.pdf, args.out)
    return 0 if len(verified) == 738 and not failed else 1


if __name__ == "__main__":
    raise SystemExit(main())
