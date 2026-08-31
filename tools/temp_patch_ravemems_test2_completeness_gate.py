from pathlib import Path

p = Path('tools/ravemems_test2_raster_text.py')
s = p.read_text(encoding='utf-8')

old = '''    replacement_masks = []
    op_results = []

    for idx, op in enumerate(cfg["operations"], 1):
'''
new = '''    replacement_masks = []
    op_results = []
    paragraph_region_owners = {}

    for idx, op in enumerate(cfg["operations"], 1):
'''
if s.count(old) != 1:
    raise SystemExit('could not locate op setup block')
s = s.replace(old, new, 1)

old = '''            bbox = entry["bbox"]
            region_id = entry["region_id"]
        elif mode == "words":
'''
new = '''            bbox = entry["bbox"]
            region_id = entry["region_id"]
            previous = paragraph_region_owners.get(region_id)
            if previous is not None:
                raise RuntimeError(f"duplicate paragraph replacement for {region_id}: operations {previous} and {idx}")
            paragraph_region_owners[region_id] = idx
        elif mode == "words":
'''
if s.count(old) != 1:
    raise SystemExit('could not locate paragraph resolution block')
s = s.replace(old, new, 1)

old = '''    localized_path = out / "CDXN990E_P007_MEMS_SIM_FR.png"
    rendered.save(localized_path)

    manifest = {
'''
new = '''    localized_path = out / "CDXN990E_P007_MEMS_SIM_FR.png"
    rendered.save(localized_path)

    # Completeness gate: OCR the localized raster again and ensure known human-language
    # source phrases that must have been localized are no longer present. Technical
    # constructor tokens are deliberately excluded from this list and remain immutable.
    localized_words = ocr_words(rendered)
    localized_ocr_text = " ".join(w["text"] for w in localized_words)
    remaining_source_phrases = [
        phrase for phrase in cfg.get("required_absent_source_phrases", [])
        if norm(phrase) in norm(localized_ocr_text)
    ]
    source_human_phrases_removed = not remaining_source_phrases

    manifest = {
'''
if s.count(old) != 1:
    raise SystemExit('could not locate localized save block')
s = s.replace(old, new, 1)

old = '''            "outside_translation_masks_pixel_identical": outside_identical,
            "artificial_visible_ids": False,
            "output_width": rendered.width,
'''
new = '''            "outside_translation_masks_pixel_identical": outside_identical,
            "artificial_visible_ids": False,
            "paragraph_regions_unique": True,
            "remaining_source_phrases": remaining_source_phrases,
            "localized_ocr_words": len(localized_words),
            "output_width": rendered.width,
'''
if s.count(old) != 1:
    raise SystemExit('could not locate manifest render block')
s = s.replace(old, new, 1)

old = '''            "technical_tokens_pixel_identical": all(x["pixels_identical"] for x in immutable_results),
            "geometry_unchanged_outside_text_zones": outside_identical,
            "no_visible_internal_numbering": True,
'''
new = '''            "technical_tokens_pixel_identical": all(x["pixels_identical"] for x in immutable_results),
            "geometry_unchanged_outside_text_zones": outside_identical,
            "paragraph_regions_unique": True,
            "source_human_phrases_removed": source_human_phrases_removed,
            "no_visible_internal_numbering": True,
'''
if s.count(old) != 1:
    raise SystemExit('could not locate manifest gate block')
s = s.replace(old, new, 1)

p.write_text(s, encoding='utf-8', newline='\n')
check = p.read_text(encoding='utf-8')
for token in (
    'paragraph_region_owners = {}',
    'duplicate paragraph replacement for',
    'remaining_source_phrases = [',
    '"source_human_phrases_removed": source_human_phrases_removed',
):
    assert token in check, token
print('RAVEMEMS_TEST2_COMPLETENESS_GATE_PATCH_OK')
