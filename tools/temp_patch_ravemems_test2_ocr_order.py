from pathlib import Path

p = Path('tools/ravemems_test2_raster_text.py')
s = p.read_text(encoding='utf-8')
old = '    for key, words in groups.items():\n        words = sorted(words, key=lambda x: (x["top"], x["left"]))\n        text = " ".join(w["text"] for w in words)\n'
new = '    for key, words in groups.items():\n        # Preserve Tesseract native token order. Re-sorting by pixel top/left can\n        # scramble words on the same visual line when glyph tops differ slightly.\n        text = " ".join(w["text"] for w in words)\n'
count = s.count(old)
if count != 1:
    raise SystemExit(f'expected exactly one OCR-order block, got {count}')
s = s.replace(old, new, 1)
p.write_text(s, encoding='utf-8', newline='\n')
check = p.read_text(encoding='utf-8')
assert 'words = sorted(words, key=lambda x: (x["top"], x["left"]))' not in check
assert 'Preserve Tesseract native token order' in check
print('RAVEMEMS_TEST2_OCR_ORDER_PATCH_OK')
