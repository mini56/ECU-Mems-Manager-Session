#!/usr/bin/env python3
from __future__ import annotations

from typing import Any

import fitz

import rcl0193eng_visual_validate as base


def validate_visual_fidelity_png(
    db,
    doc: fitz.Document,
    out_dir,
) -> tuple[list[dict[str, Any]], list[dict[str, Any]]]:
    """Prove crop fidelity by byte-identical independent PNG rerender.

    The extracted asset was originally produced by PyMuPDF Pixmap.save().
    Re-render the same PDF page/crop/matrix independently and encode that
    pixmap again with PyMuPDF. The asset is accepted only if the complete PNG
    byte stream is identical to the independently generated PNG, in addition
    to DB SHA, bbox and dimension checks.

    We still record whether PyMuPDF's PNG decoder reproduces the source raw
    samples, but that decoder-side diagnostic is deliberately not the fidelity
    criterion: the independently re-encoded PNG itself is the artifact being
    proved.
    """
    verified: list[dict[str, Any]] = []
    failed: list[dict[str, Any]] = []
    rows = db.execute(
        "SELECT v.visual_key,v.page_key,v.relative_path,v.sha256,v.width,v.height,"
        "v.render_method,v.source_bbox_json,v.crop_bbox_json,p.physical_page "
        "FROM ravemems_visual v JOIN ravemems_page p ON p.page_key=v.page_key "
        "ORDER BY v.visual_key"
    ).fetchall()

    for row in rows:
        visual_key, page_key, relative_path, stored_sha, width, height, render_method, source_json, crop_json, physical_page = row
        evidence: dict[str, Any] = {
            "visual_key": visual_key,
            "page_key": page_key,
            "physical_page": int(physical_page),
            "render_method": render_method,
            "proof": "byte_identical_independent_png_rerender",
        }
        try:
            if render_method != "pdf_page_render_crop":
                raise ValueError(f"unexpected render_method={render_method}")
            page = doc[int(physical_page) - 1]
            source = base._bbox(source_json)
            crop = base._bbox(crop_json)
            if source.is_empty or crop.is_empty:
                raise ValueError("empty source/crop bbox")
            if not base._rect_contains(page.rect, crop):
                raise ValueError("crop bbox outside source page")
            if not base._rect_contains(crop, source):
                raise ValueError("source bbox not contained by crop bbox")

            asset_path = out_dir / str(relative_path)
            asset_png = asset_path.read_bytes()
            asset_sha = base._sha256(asset_png)
            if asset_sha != stored_sha:
                raise ValueError(f"asset sha mismatch db={stored_sha} file={asset_sha}")

            rerender = page.get_pixmap(matrix=fitz.Matrix(1.5, 1.5), clip=crop, alpha=False)
            if (rerender.width, rerender.height) != (int(width), int(height)):
                raise ValueError(
                    f"rerender dimensions mismatch db={(width, height)} rerender={(rerender.width, rerender.height)}"
                )

            rerender_png = rerender.tobytes("png")
            rerender_png_sha = base._sha256(rerender_png)
            if rerender_png != asset_png:
                decoded = fitz.Pixmap(str(asset_path))
                raise ValueError(
                    "rerender PNG mismatch "
                    f"expected_png={rerender_png_sha} asset_png={asset_sha} "
                    f"source_samples={base._sha256(bytes(rerender.samples))} "
                    f"decoded_samples={base._sha256(bytes(decoded.samples))}"
                )

            decoded = fitz.Pixmap(str(asset_path))
            if (decoded.width, decoded.height) != (int(width), int(height)):
                raise ValueError(
                    f"decoded dimensions mismatch db={(width, height)} png={(decoded.width, decoded.height)}"
                )

            source_samples_sha = base._sha256(bytes(rerender.samples))
            decoded_samples_sha = base._sha256(bytes(decoded.samples))
            db.execute(
                "UPDATE ravemems_visual SET fidelity_status='verified' WHERE visual_key=?",
                (visual_key,),
            )
            evidence.update(
                {
                    "status": "verified",
                    "asset_sha256": asset_sha,
                    "rerender_png_sha256": rerender_png_sha,
                    "source_samples_sha256": source_samples_sha,
                    "decoded_samples_sha256": decoded_samples_sha,
                    "decoder_samples_equal": source_samples_sha == decoded_samples_sha,
                    "width": rerender.width,
                    "height": rerender.height,
                    "source_bbox": list(source),
                    "crop_bbox": list(crop),
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


base.validate_visual_fidelity = validate_visual_fidelity_png


if __name__ == "__main__":
    raise SystemExit(base.main())
