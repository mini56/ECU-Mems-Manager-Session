from pathlib import Path

p = Path('tools/ravemems_test2_raster_text.py')
text = p.read_text(encoding='utf-8')

marker = '''\ndef paragraph_entries(ocr):\n'''
helper = '''\ndef ocr_words(image):\n    data = pytesseract.image_to_data(image, lang="eng", config="--psm 3", output_type=Output.DICT)\n    words = []\n    for i, txt in enumerate(data["text"]):\n        txt = (txt or "").strip()\n        if not txt:\n            continue\n        try:\n            conf = float(data["conf"][i])\n        except Exception:\n            conf = -1\n        if conf < 25:\n            continue\n        x, y, w, h = (int(data[k][i]) for k in ("left", "top", "width", "height"))\n        words.append({\n            "text": txt,\n            "confidence": conf,\n            "left": x,\n            "top": y,\n            "width": w,\n            "height": h,\n            "bbox": (x, y, x + w, y + h),\n            "block": int(data["block_num"][i]),\n            "par": int(data["par_num"][i]),\n            "line": int(data["line_num"][i]),\n        })\n    return words\n\n\ndef paragraph_entries(ocr):\n'''
if text.count(marker) != 1:
    raise SystemExit(f'paragraph marker count={text.count(marker)}')
if 'def ocr_words(image):' in text:
    raise SystemExit('ocr_words helper already exists')
text = text.replace(marker, helper, 1)

old = '''    # OCR is used only because this page has zero native text and all language is burned into pixels.\n    data = pytesseract.image_to_data(image, lang="eng", config="--psm 3", output_type=Output.DICT)\n    words = []\n    for i, txt in enumerate(data["text"]):\n        txt = (txt or "").strip()\n        if not txt:\n            continue\n        try:\n            conf = float(data["conf"][i])\n        except Exception:\n            conf = -1\n        if conf < 25:\n            continue\n        x, y, w, h = (int(data[k][i]) for k in ("left", "top", "width", "height"))\n        words.append({\n            "text": txt,\n            "confidence": conf,\n            "left": x,\n            "top": y,\n            "width": w,\n            "height": h,\n            "bbox": (x, y, x + w, y + h),\n            "block": int(data["block_num"][i]),\n            "par": int(data["par_num"][i]),\n            "line": int(data["line_num"][i]),\n        })\n\n'''
new = '''    # OCR is used only because this page has zero native text and all language is burned into pixels.\n    words = ocr_words(image)\n\n'''
if text.count(old) != 1:
    raise SystemExit(f'inline OCR block count={text.count(old)}')
text = text.replace(old, new, 1)

if text.count('def ocr_words(image):') != 1:
    raise SystemExit('helper definition guard failed')
if text.count('words = ocr_words(image)') != 1:
    raise SystemExit('source OCR call guard failed')
if text.count('localized_words = ocr_words(rendered)') != 1:
    raise SystemExit('localized OCR call guard failed')
if 'lang="eng", config="--psm 3", output_type=Output.DICT' not in text or 'if conf < 25:' not in text:
    raise SystemExit('OCR semantics changed unexpectedly')

p.write_text(text, encoding='utf-8', newline='\n')
print('RAVEMEMS_TEST2_OCR_HELPER_FACTORIZATION_OK')
