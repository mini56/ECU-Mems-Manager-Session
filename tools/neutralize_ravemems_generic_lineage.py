#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def replace_once(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f"{label}: expected exactly one occurrence, found {count}")
    return text.replace(old, new, 1)


def write(path: Path, text: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="utf-8")


# 1) Build a neutral core from the already-validated historical core.
legacy_core_path = ROOT / "ravemems/v2/prototype_extract.py"
core = legacy_core_path.read_text(encoding="utf-8")
core = replace_once(
    core,
    'parser = argparse.ArgumentParser(description="RAVEMEMS V2 RCL0193ENG prototype extractor")',
    'parser = argparse.ArgumentParser(description="RAVEMEMS V2 document extractor")',
    "neutral core argparse description",
)
core = replace_once(
    core,
    'db_path = args.out / "ravemems_v2_rcl0193eng.sqlite"',
    'db_path = args.out / str(profile.get("db_filename") or "ravemems_v2.sqlite")',
    "neutral core database filename",
)
core = replace_once(
    core,
    'raise SystemExit("Expected publication code RCL0193ENG not found in source PDF")',
    'raise SystemExit("Expected document identity pattern not found in source PDF")',
    "neutral publication error",
)
core = replace_once(
    core,
    '    document_key = "DOC_RCL0193ENG"\n    revision_key = "REV_RCL0193ENG_SOURCE"',
    '    identity_token = str(profile.get("document_identity_token") or publication_code or sha256_file(args.pdf)[:16].upper())\n'
    '    identity_token = re.sub(r"[^A-Z0-9]+", "", identity_token.upper()) or sha256_file(args.pdf)[:16].upper()\n'
    '    document_key = str(profile.get("document_key") or f"DOC_{identity_token}")\n'
    '    revision_key = str(profile.get("revision_key") or f"REV_{identity_token}_SOURCE")',
    "neutral identity keys",
)
core = replace_once(
    core,
    '(document_key, publication_code, profile["language"], "workshop_manual", "Rover", publication_code),',
    '(document_key, publication_code or identity_token, profile["language"], profile.get("initial_document_kind", "document"), profile.get("manufacturer"), publication_code or identity_token),',
    "neutral initial document metadata",
)
core = replace_once(
    core,
    '        "prototype": "RCL0193ENG",',
    '        "engine_id": str(profile.get("engine_id") or "ravemems_v2_generic"),',
    "neutral manifest engine id",
)
core = core.replace('"prototype_extract"', '"core_extract"')
core = core.replace('RCL0193ENG_SEQUENCE_DIAGNOSTICS_', 'GENERIC_SEQUENCE_DIAGNOSTICS_')
core = core.replace('RCL0193ENG_PAGES_131_136_SEMANTIC_', 'GENERIC_PAGES_131_136_SEMANTIC_')
if "RCL0193" in core.upper():
    raise RuntimeError("neutral core still contains a publication-specific RCL0193 token")
write(ROOT / "ravemems/v2/core_extract.py", core)

# 2) Copy the validated semantic PASS2 algorithm, changing only the neutral core import.
legacy_semantic = (ROOT / ".github/pass2/rcl0193eng_zero_defect_pass2.py").read_text(encoding="utf-8")
semantic = replace_once(
    legacy_semantic,
    "import prototype_extract as pe",
    "import core_extract as pe",
    "generic semantic core import",
)
if "RCL0193" in semantic.upper():
    raise RuntimeError("generic semantic module still contains a publication-specific RCL0193 token")
write(ROOT / ".github/pass2/generic_semantic_pass2.py", semantic)

# 3) Exact visual geometry patch, now importing the neutral semantic module.
precise = '''#!/usr/bin/env python3
from __future__ import annotations

import json
from typing import Any

import generic_semantic_pass2 as pass2

pe = pass2.pe
_original_render_visuals = pe.render_visuals


def _exact_bbox_json(rect: Any) -> str:
    values = tuple(rect) if isinstance(rect, pe.fitz.Rect) else rect
    return json.dumps([float(value) for value in values], separators=(",", ":"))


def _render_visuals_with_exact_geometry(*args: Any, **kwargs: Any) -> int:
    previous_bbox_json = pe.bbox_json
    pe.bbox_json = _exact_bbox_json
    try:
        return _original_render_visuals(*args, **kwargs)
    finally:
        pe.bbox_json = previous_bbox_json


pe.render_visuals = _render_visuals_with_exact_geometry


if __name__ == "__main__":
    raise SystemExit(pe.main())
'''
write(ROOT / ".github/pass2/generic_precise_visual_extract.py", precise)

# 4) Point the generic document wrapper at the neutral modules and neutral DB.
wrapper_path = ROOT / ".github/pass2/ravemems_generic_precise_extract.py"
wrapper = wrapper_path.read_text(encoding="utf-8")
wrapper = replace_once(
    wrapper,
    "import rcl0193eng_precise_visual_extract as precise",
    "import generic_precise_visual_extract as precise",
    "generic wrapper precise import",
)
wrapper = replace_once(
    wrapper,
    '    profile["publication_code_regex"] = _publication_match_regex(probe.get("publication_code"))\n    path = out / "_runtime_profile.json"',
    '    profile["publication_code_regex"] = _publication_match_regex(probe.get("publication_code"))\n'
    '    profile["document_identity_token"] = str(probe["identity"])\n'
    '    profile["db_filename"] = "ravemems_v2.sqlite"\n'
    '    profile["engine_id"] = "ravemems_v2_generic"\n'
    '    path = out / "_runtime_profile.json"',
    "runtime neutral identity metadata",
)
wrapper = replace_once(
    wrapper,
    'db_path=out/"ravemems_v2_rcl0193eng.sqlite"',
    'db_path=out/"ravemems_v2.sqlite"',
    "generic wrapper database path",
)
if "RCL0193" in wrapper.upper():
    raise RuntimeError("generic wrapper still contains a publication-specific RCL0193 token")
write(wrapper_path, wrapper)

# 5) Build a neutral visual-link validator from the validated evidence validator.
legacy_visual = (ROOT / ".github/pass2/rcl0193eng_visual_validate.py").read_text(encoding="utf-8")
visual = legacy_visual.replace("rcl0193eng", "generic").replace("RCL0193ENG", "GENERIC")
visual = visual.replace('ravemems_v2_generic.sqlite', 'ravemems_v2.sqlite')
if "RCL0193" in visual.upper():
    raise RuntimeError("generic visual validator still contains a publication-specific RCL0193 token")
write(ROOT / ".github/pass2/generic_visual_validate.py", visual)

# 6) Build an exact extraction-order replay with no publication-specific expected counts.
legacy_replay = (ROOT / ".github/pass2/rcl0193eng_visual_replay_validate.py").read_text(encoding="utf-8")
replay = legacy_replay
replay = replace_once(replay, "import rcl0193eng_zero_defect_pass2 as pass2", "import generic_semantic_pass2 as pass2", "generic replay semantic import")
replay = replace_once(replay, "import rcl0193eng_visual_validate as visual_base", "import generic_visual_validate as visual_base", "generic replay visual validator import")
replay = replay.replace('ravemems_v2_rcl0193eng.sqlite', 'ravemems_v2.sqlite')
replay = replay.replace('RCL0193ENG_REPLAY_', 'GENERIC_REPLAY_')
replay = replay.replace('RAVEMEMS_V2_RCL0193ENG_EXACT_VISUAL_REPLAY_GLOBAL_ZERO_PASS', 'RAVEMEMS_V2_GENERIC_EXACT_VISUAL_REPLAY_GLOBAL_ZERO_PASS')
replay = replay.replace('Replay RCL0193ENG visual extraction in exact page order', 'Replay visual extraction in exact document page order')
replay = replace_once(
    replay,
    'def replay_visuals(pdf_path: Path, out_dir: Path) -> tuple[list[dict[str, Any]], list[dict[str, Any]]]:',
    'def replay_visuals(pdf_path: Path, out_dir: Path) -> dict[str, Any]:',
    "generic replay return annotation",
)
old_ok = '''        ok = (
            expected_total == 738
            and len(seen_keys) == 738
            and len(verified) == 738
            and not failed
            and len(links_verified) == 401
            and not links_failed
            and str(integrity).lower() == "ok"
            and not foreign_keys
            and not issues
        )
        if ok:
            print("RAVEMEMS_V2_GENERIC_EXACT_VISUAL_REPLAY_GLOBAL_ZERO_PASS")
        return verified, failed if not ok else []
'''
new_ok = '''        ok = (
            len(seen_keys) == int(expected_total)
            and len(verified) == int(expected_total)
            and not failed
            and not links_failed
            and str(integrity).lower() == "ok"
            and not foreign_keys
            and not issues
        )
        if ok:
            print("RAVEMEMS_V2_GENERIC_EXACT_VISUAL_REPLAY_GLOBAL_ZERO_PASS")
        return summary
'''
replay = replace_once(replay, old_ok, new_ok, "generic replay zero-audit guard")
old_main = '''    verified, failed = replay_visuals(args.pdf, args.out)
    return 0 if len(verified) == 738 and not failed else 1
'''
new_main = '''    result = replay_visuals(args.pdf, args.out)
    ok = (
        result["replayed_visual_count"] == result["db_visual_count"]
        and result["visual_fidelity_verified_count"] == result["db_visual_count"]
        and result["visual_fidelity_failed_count"] == 0
        and result["visual_link_failed_count"] == 0
        and result["missing_replay_count"] == 0
        and result["unexpected_replay_count"] == 0
        and str(result["sqlite_integrity"]).lower() == "ok"
        and result["foreign_key_issue_count"] == 0
        and result["audit_issue_count_after_validation"] == 0
    )
    return 0 if ok else 1
'''
replay = replace_once(replay, old_main, new_main, "generic replay CLI guard")
if "RCL0193" in replay.upper():
    raise RuntimeError("generic replay still contains a publication-specific RCL0193 token")
write(ROOT / ".github/pass2/ravemems_generic_visual_replay_validate.py", replay)

# 7) Keep the reusable RCL0179 regression workflow compatible with the neutral DB name.
workflow_path = ROOT / ".github/workflows/tmp-ravemems-v2-generic-source-rcl0179.yml"
workflow = workflow_path.read_text(encoding="utf-8")
workflow = workflow.replace("ravemems_v2_rcl0193eng.sqlite", "ravemems_v2.sqlite")
write(workflow_path, workflow)

# The generic execution path must contain no publication-specific RCL0193 token.
generic_paths = [
    ROOT / "ravemems/v2/core_extract.py",
    ROOT / "ravemems/v2/source_input.py",
    ROOT / "ravemems/v2/generic_document_rules.py",
    ROOT / ".github/pass2/generic_semantic_pass2.py",
    ROOT / ".github/pass2/generic_precise_visual_extract.py",
    ROOT / ".github/pass2/ravemems_generic_precise_extract.py",
    ROOT / ".github/pass2/generic_visual_validate.py",
    ROOT / ".github/pass2/ravemems_generic_visual_replay_validate.py",
    ROOT / ".github/pass2/ravemems_extract_source.py",
]
for path in generic_paths:
    value = path.read_text(encoding="utf-8")
    if "RCL0193" in value.upper():
        raise RuntimeError(f"publication-specific residue remains in generic execution path: {path}")
print("GENERIC_LINEAGE_NEUTRALIZATION_GENERATED")
for path in generic_paths:
    print(path.relative_to(ROOT))
