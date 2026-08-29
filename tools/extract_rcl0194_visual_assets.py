#!/usr/bin/env python3
import argparse
import hashlib
import json
from pathlib import Path

import fitz

SOURCE_DOCUMENT = "RCL0194ENG"
SOURCE_URL = "https://www.goclassic.eu/upload/content/e68ce7_9c0fd6a5b3a14a3585ecddb02b3ceb9d.pdf"
EXPECTED_PDF_SHA256 = "64e64f8a7c24f362913e2661403bc474e4e7ef07f96db618ef661645e0d0f051"
EXPECTED_PAGE_COUNT = 41

PAGES = [
    {
        "label": "COLOUR_CODES",
        "source_page": "COLOUR CODES",
        "pdf_index": 5,
        "filename": "RCL0194ENG_COLOUR_CODES.png",
        "asset_kind": "legend",
        "required_text": ["COLOUR CODES", "BLACK", "GREEN", "SLATE"],
    },
    {
        "label": "15.1",
        "source_page": "15.1",
        "pdf_index": 13,
        "filename": "RCL0194ENG_15_1.png",
        "asset_kind": "wiring_diagram",
        "required_text": ["CHARGING", "STARTING", "MPi"],
    },
    {
        "label": "20.1",
        "source_page": "20.1",
        "pdf_index": 15,
        "filename": "RCL0194ENG_20_1.png",
        "asset_kind": "wiring_diagram",
        "required_text": ["ENGINE MANAGEMENT", "MEMS", "MPi"],
    },
    {
        "label": "20.2",
        "source_page": "20.2",
        "pdf_index": 16,
        "filename": "RCL0194ENG_20_2.png",
        "asset_kind": "wiring_diagram",
        "required_text": ["ENGINE MANAGEMENT", "MEMS", "MPi"],
    },
    {
        "label": "39.2",
        "source_page": "39.2",
        "pdf_index": 25,
        "filename": "RCL0194ENG_39_2.png",
        "asset_kind": "wiring_diagram",
        "required_text": ["COOLING FAN", "MPi"],
    },
]


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def normalized_text(page) -> str:
    return " ".join(page.get_text("text").split())


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--pdf", required=True)
    parser.add_argument("--out", required=True)
    args = parser.parse_args()

    pdf = Path(args.pdf).resolve()
    out = Path(args.out).resolve()
    out.mkdir(parents=True, exist_ok=True)

    if not pdf.is_file():
        raise SystemExit(f"missing PDF: {pdf}")

    pdf_hash = sha256(pdf)
    if pdf_hash.lower() != EXPECTED_PDF_SHA256:
        raise SystemExit(
            f"RCL0194ENG SHA256 mismatch: got={pdf_hash} expected={EXPECTED_PDF_SHA256}"
        )

    document = fitz.open(pdf)
    try:
        if document.page_count != EXPECTED_PAGE_COUNT:
            raise SystemExit(
                f"unexpected RCL0194ENG page count: {document.page_count}"
            )

        assets = []
        for spec in PAGES:
            page = document.load_page(spec["pdf_index"])
            text = normalized_text(page)
            missing = [token for token in spec["required_text"] if token.lower() not in text.lower()]
            if missing:
                raise SystemExit(
                    f"page identity check failed for {spec['source_page']}: missing {missing}"
                )

            pix = page.get_pixmap(matrix=fitz.Matrix(2.0, 2.0), alpha=False)
            target = out / spec["filename"]
            pix.save(target)
            assets.append(
                {
                    "source_document": SOURCE_DOCUMENT,
                    "source_page": spec["source_page"],
                    "pdf_index": spec["pdf_index"],
                    "asset_kind": spec["asset_kind"],
                    "filename": spec["filename"],
                    "sha256": sha256(target),
                    "width_px": pix.width,
                    "height_px": pix.height,
                    "identity_text": text[:500],
                }
            )
    finally:
        document.close()

    metadata = {
        "source_document": SOURCE_DOCUMENT,
        "source_url": SOURCE_URL,
        "source_pdf_sha256": pdf_hash,
        "source_pdf_page_count": EXPECTED_PAGE_COUNT,
        "production_base": "MEMSX64 BUILD #101 22dbe75ed14e0a61e694159d505ef72245116b48",
        "historical_facts_modified": False,
        "ecu_communication_modified": False,
        "assets": assets,
    }
    (out / "RCL0194_VISUAL_ASSETS.json").write_text(
        json.dumps(metadata, indent=2, ensure_ascii=False) + "\n",
        encoding="utf-8",
    )

    print(f"PASS source SHA256 {pdf_hash}")
    for asset in assets:
        print(
            f"PASS {asset['source_page']} index={asset['pdf_index']} "
            f"sha256={asset['sha256']} size={asset['width_px']}x{asset['height_px']}"
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
