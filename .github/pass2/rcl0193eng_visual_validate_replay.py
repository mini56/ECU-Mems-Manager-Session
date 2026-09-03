#!/usr/bin/env python3
from __future__ import annotations

import tempfile
from collections import defaultdict
from pathlib import Path
from typing import Any

import fitz

import rcl0193eng_precise_visual_extract as precise
import rcl0193eng_visual_validate as base


pe = precise.pe


def _save_png_bytes(pixmap: fitz.Pixmap) -> bytes:
    handle = tempfile.NamedTemporaryFile(suffix=".png", delete=False)
    temp_path = Path(handle.name)
    handle.close()
    try:
        pixmap.save(str(temp_path))
        return temp_path.read_bytes()
    finally:
        temp_path.unlink(missing_ok=True)


def _rect_close(a: fitz.Rect, b: fitz.Rect, tolerance: float = 1e-6) -> bool:
    return all(abs(float(x) - float(y)) <= tolerance for x, y in zip(a, b))


def validate_visual_fidelity_replay(
    db,
    doc: fitz.Document,
    out_dir,
) -> tuple[list[dict[str, Any]], list[dict[str, Any]]]:
    """Reproduce the exact sequential source-page state before each visual render.

    The extraction pass walks every PDF page in physical order, obtains sorted
    text, reconstructs line geometry, scans visual candidates, then renders the
    candidate crops. This validator independently repeats that source traversal
    from the pinned PDF, recomputes every source/crop rectangle, renders through
    the same Matrix(1.5, 1.5) + Pixmap.save path and requires byte-identical PNGs.
    No extracted image is trusted as a source of geometry or pixels.
    """
    verified: list[dict[str, Any]] = []
    failed: list[dict[str, Any]] = []
    rows = db.execute(
        "SELECT v.visual_key,v.page_key,v.relative_path,v.sha256,v.width,v.height,"
        "v.render_method,v.source_bbox_json,v.crop_bbox_json,p.physical_page "
        "FROM ravemems_visual v JOIN ravemems_page p ON p.page_key=v.page_key "
        "ORDER BY p.physical_page,v.visual_key"
    ).fetchall()

    by_page: dict[int, list[tuple[Any, ...]]] = defaultdict(list)
    for row in rows:
        by_page[int(row[9])].append(row)

    for page_index in range(doc.page_count):
        physical_page = page_index + 1
        page = doc[page_index]

        # Replay the source access sequence used by the extractor before render_visuals().
        page_text = page.get_text("text", sort=True)
        pe.read_lines(page)
        candidates = pe.visual_candidate_rects(page)
        page_rows = by_page.get(physical_page, [])
        if not page_rows:
            continue

        if len(candidates) != len(page_rows):
            reason = f"candidate count mismatch source={len(candidates)} db={len(page_rows)}"
            for row in page_rows:
                visual_key = str(row[0])
                db.execute(
                    "UPDATE ravemems_visual SET fidelity_status='failed' WHERE visual_key=?",
                    (visual_key,),
                )
                failed.append(
                    {
                        "visual_key": visual_key,
                        "page_key": str(row[1]),
                        "physical_page": physical_page,
                        "render_method": str(row[6]),
                        "proof": "sequential_source_replay_byte_identical_png",
                        "status": "failed",
                        "reason": reason,
                    }
                )
            continue

        for ordinal, (row, source_rect) in enumerate(zip(page_rows, candidates), start=1):
            (
                visual_key,
                page_key,
                relative_path,
                stored_sha,
                width,
                height,
                render_method,
                source_json,
                crop_json,
                _physical_page,
            ) = row
            evidence: dict[str, Any] = {
                "visual_key": visual_key,
                "page_key": page_key,
                "physical_page": physical_page,
                "ordinal": ordinal,
                "render_method": render_method,
                "proof": "sequential_source_replay_byte_identical_png",
            }
            try:
                if render_method != "pdf_page_render_crop":
                    raise ValueError(f"unexpected render_method={render_method}")

                stored_source = base._bbox(source_json)
                stored_crop = base._bbox(crop_json)
                recomputed_source = fitz.Rect(source_rect)
                recomputed_crop = pe.expanded_clip(recomputed_source, page.rect)
                if not _rect_close(stored_source, recomputed_source):
                    raise ValueError(
                        f"source bbox mismatch stored={list(stored_source)} recomputed={list(recomputed_source)}"
                    )
                if not _rect_close(stored_crop, recomputed_crop):
                    raise ValueError(
                        f"crop bbox mismatch stored={list(stored_crop)} recomputed={list(recomputed_crop)}"
                    )
                if not base._rect_contains(page.rect, recomputed_crop):
                    raise ValueError("recomputed crop bbox outside source page")
                if not base._rect_contains(recomputed_crop, recomputed_source):
                    raise ValueError("recomputed source bbox not contained by crop bbox")

                asset_path = out_dir / str(relative_path)
                asset_png = asset_path.read_bytes()
                asset_sha = base._sha256(asset_png)
                if asset_sha != stored_sha:
                    raise ValueError(f"asset sha mismatch db={stored_sha} file={asset_sha}")

                rerender = page.get_pixmap(
                    matrix=fitz.Matrix(1.5, 1.5),
                    clip=recomputed_crop,
                    alpha=False,
                )
                if (rerender.width, rerender.height) != (int(width), int(height)):
                    raise ValueError(
                        f"rerender dimensions mismatch db={(width, height)} rerender={(rerender.width, rerender.height)}"
                    )
                rerender_png = _save_png_bytes(rerender)
                rerender_sha = base._sha256(rerender_png)
                if rerender_png != asset_png:
                    asset_pix = fitz.Pixmap(str(asset_path))
                    raise ValueError(
                        "sequential replay PNG mismatch "
                        f"rerender_png={rerender_sha} asset_png={asset_sha} "
                        f"rerender_samples={base._sha256(bytes(rerender.samples))} "
                        f"asset_samples={base._sha256(bytes(asset_pix.samples))} "
                        f"rerender_origin=({rerender.x},{rerender.y})"
                    )

                db.execute(
                    "UPDATE ravemems_visual SET fidelity_status='verified' WHERE visual_key=?",
                    (visual_key,),
                )
                evidence.update(
                    {
                        "status": "verified",
                        "asset_sha256": asset_sha,
                        "rerender_png_sha256": rerender_sha,
                        "width": rerender.width,
                        "height": rerender.height,
                        "source_bbox": list(recomputed_source),
                        "crop_bbox": list(recomputed_crop),
                        "rerender_origin": [rerender.x, rerender.y],
                    }
                )
                verified.append(evidence)
            except Exception as exc:
                db.execute(
                    "UPDATE ravemems_visual SET fidelity_status='failed' WHERE visual_key=?",
                    (visual_key,),
                )
                evidence.update({"status": "failed", "reason": str(exc)})
                failed.append(evidence)

    return verified, failed


base.validate_visual_fidelity = validate_visual_fidelity_replay


if __name__ == "__main__":
    raise SystemExit(base.main())
