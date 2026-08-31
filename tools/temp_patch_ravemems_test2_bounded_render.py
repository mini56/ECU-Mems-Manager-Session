from pathlib import Path

p = Path('tools/ravemems_test2_raster_text.py')
s = p.read_text(encoding='utf-8')
old = '''        draw.rectangle(b, fill=(255, 255, 255))
        y = b[1]
        for line in lines:
            draw.text((b[0], y), line, font=font, fill=(0, 0, 0))
            y += line_h + spacing
        if y - spacing > b[3] + 1:
            raise RuntimeError(f"post-render overflow: {op['text']!r}")
'''
new = '''        # Render into a bbox-sized patch first. PIL glyph antialiasing / negative
        # left bearings must never be allowed to write outside the declared mask.
        patch_w = b[2] - b[0]
        patch_h = b[3] - b[1]
        patch = Image.new("RGB", (patch_w, patch_h), (255, 255, 255))
        patch_draw = ImageDraw.Draw(patch)
        y = 0
        for line in lines:
            probe = patch_draw.textbbox((0, y), line, font=font)
            x = max(0, -probe[0])
            bounds = patch_draw.textbbox((x, y), line, font=font)
            if bounds[0] < 0 or bounds[1] < 0 or bounds[2] > patch_w or bounds[3] > patch_h:
                raise RuntimeError(f"bounded text patch would clip: {line!r}, bounds={bounds}, patch={(patch_w, patch_h)}")
            patch_draw.text((x, y), line, font=font, fill=(0, 0, 0))
            y += line_h + spacing
        if y - spacing > patch_h + 1:
            raise RuntimeError(f"post-render overflow: {op['text']!r}")
        rendered.paste(patch, (b[0], b[1]))
'''
count = s.count(old)
if count != 1:
    raise SystemExit(f'expected one direct page render block, got {count}')
s = s.replace(old, new, 1)
p.write_text(s, encoding='utf-8', newline='\n')
check = p.read_text(encoding='utf-8')
assert 'rendered.paste(patch, (b[0], b[1]))' in check
assert 'draw.text((b[0], y)' not in check
assert 'for m in replacement_masks:' in check
assert 'outside_identical = guard_a.tobytes() == guard_b.tobytes()' in check
print('RAVEMEMS_TEST2_BOUNDED_PATCH_RENDER_OK')
