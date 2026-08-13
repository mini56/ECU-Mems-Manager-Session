#!/usr/bin/env python3
from pathlib import Path
import base64
import io
import lzma
import tarfile

root = Path(__file__).resolve().parent
bundle = root / "translations_packed" / "translations_bundle.tar.xz.b64"
outdir = root / "translations"
outdir.mkdir(exist_ok=True)

raw = lzma.decompress(base64.b64decode(bundle.read_text(encoding="ascii")))
with tarfile.open(fileobj=io.BytesIO(raw), mode="r:") as archive:
    members = {m.name: m for m in archive.getmembers() if m.isfile()}
    for lang in ("es", "it", "pt", "de"):
        name = f"ECUMemsManager_{lang}.ts"
        member = members.get(name)
        if member is None:
            raise SystemExit(f"Missing translation in bundle: {name}")
        src = archive.extractfile(member)
        if src is None:
            raise SystemExit(f"Unable to read translation: {name}")
        data = src.read()
        (outdir / name).write_bytes(data)
        print(f"Generated {name}: {len(data)} bytes")
