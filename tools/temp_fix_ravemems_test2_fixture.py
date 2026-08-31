from pathlib import Path
import json

cfg_path = Path('.github/ravemems_test2_page7_fr.json')
cfg = json.loads(cfg_path.read_text(encoding='utf-8'))
ops = cfg['operations']

def one_paragraph(anchor):
    matches = [op for op in ops if op.get('mode') == 'paragraph' and op.get('anchor') == anchor]
    if len(matches) != 1:
        raise SystemExit(f'expected one paragraph op for {anchor!r}, got {len(matches)}')
    return matches[0]

# R042: one complete paragraph replacement, never two destructive writes to the same bbox.
r42 = one_paragraph('The connector number has two parts')
r42['text'] = "Le numéro de connecteur comporte deux parties : l'identifiant, ex. C53, puis le numéro de broche, ex. C53-3, indiquant que le fil est raccordé à la broche 3. À utiliser avec la section Détails des connecteurs de la Bibliothèque de référence électrique."
old_len = len(ops)
ops[:] = [op for op in ops if not (op.get('mode') == 'paragraph' and op.get('anchor') == 'is connected to pin number 3')]
if len(ops) != old_len - 1:
    raise SystemExit('failed to remove the duplicate R042 paragraph operation')

# R025 must contain only its own source sentence.
r25 = one_paragraph('Connector - Direction of the')
r25['text'] = "Connecteur — l'orientation du symbole indique les moitiés mâle et femelle du connecteur."

# R026 gets its own translated content in its own bbox.
r25_index = ops.index(r25)
r26 = {
    'mode': 'paragraph',
    'anchor': 'A. Plug on lead (Flylead)',
    'text': 'A. Prise sur faisceau volant, câblée directement au composant. B. Connecteur branché directement au composant.'
}
ops.insert(r25_index + 1, r26)

# R006: preserve constructor refs C24 and 1, replace only the human word EARTH.
word_start = next((i for i, op in enumerate(ops) if op.get('mode') == 'words'), len(ops))
ops.insert(word_start, {
    'mode': 'words',
    'paragraph_anchor': 'C24 EARTH 1',
    'words': ['EARTH'],
    'text': 'MASSE'
})

cfg['required_absent_source_phrases'] = [
    'EARTH 1',
    'Plug on lead',
    'Connector plugs directly into component',
    'Connector - Direction of the',
    'The connector number has two parts'
]

cfg_path.write_text(json.dumps(cfg, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')

# Exact fixture guards.
check = json.loads(cfg_path.read_text(encoding='utf-8'))
paragraph_anchors = [op.get('anchor') for op in check['operations'] if op.get('mode') == 'paragraph']
assert paragraph_anchors.count('The connector number has two parts') == 1
assert 'is connected to pin number 3' not in paragraph_anchors
assert paragraph_anchors.count('A. Plug on lead (Flylead)') == 1
assert any(op.get('mode') == 'words' and op.get('paragraph_anchor') == 'C24 EARTH 1' and op.get('words') == ['EARTH'] and op.get('text') == 'MASSE' for op in check['operations'])
assert len(check['operations']) == 34, len(check['operations'])
print('RAVEMEMS_TEST2_TRANSLATION_COMPLETENESS_FIXTURE_OK')
