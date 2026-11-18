#!/usr/bin/env python3
"""SESSION six-language UI regression gate; no modification of program data."""
import json
import re
import sys
from pathlib import Path

root = Path(__file__).resolve().parents[1]
langs = ('fr', 'en', 'es', 'it', 'pt', 'de')
tabs = (1001,7900,2001,4001,3001,7013,7940,7018,7017,7012,5001,7152,6002,19000,16200)
keys = (list(range(16200,16220))+list(range(16300,16336))+
        list(range(16400,16425))+list(range(16500,16507))+
        list(range(16600,16612))+list(range(16700,16714)))
errors = []
def check(condition, message):
    if not condition: errors.append(message)
def read(path):
    f = root/path
    check(f.is_file(), 'missing file '+path)
    return f.read_text(encoding='utf8') if f.is_file() else ''
all_text = {}
for lang in langs:
    dictionary = {}
    for module in sorted((root/'translations'/lang).glob(lang+'*.json')):
        fragment = json.loads(module.read_text(encoding='utf8'))
        dictionary.update(fragment)
    all_text[lang] = dictionary
    for key in list(tabs)+keys:
        check(bool(dictionary.get(str(key),'').strip()),f'{lang}: missing key {key}')
    help_name = {'fr':'help.html.in','en':'help_en.html.in'}.get(lang,f'help_{lang}.html.in')
    guide = read(help_name)
    anchors = [int(x) for x in re.findall(r'<h2 id="chapter-(\d+)"',guide)]
    check(anchors == list(range(1,25)), f'{lang}: 24 guide chapters required')
    check(f'lang="{lang}"' in guide and '@APP_VERSION@' in guide,
          f'{lang}: guide language/version missing')
    check('class="warn"' in guide, f'{lang}: safety warning missing')
for key in keys:
    expected = sorted(re.findall(r'%[1-9]',all_text['en'].get(str(key),'')))
    for lang in langs:
        actual = sorted(re.findall(r'%[1-9]',all_text[lang].get(str(key),'')))
        check(actual == expected, f'{lang}: placeholders in {key}')
cmake,qrc,guide = map(read,('CMakeLists.txt','resources.qrc','helpviewer.cpp'))
for lang in ('es','it','pt','de'):
    check(f'help_{lang}.html.in' in cmake and f'<file>help_{lang}.html</file>' in qrc,
          f'{lang}: guide not bundled')
    check(f':/help/help_{lang}.html' in guide, f'{lang}: guide not loaded')
check('I18n::language().left(2).toLower()' in guide,'guide uses wrong language')
usb,menu=map(read,('usbcablediagnostictab.h','uishell.cpp'))
check('QLocale().language()' not in usb,'USB tied to Windows language')
check('return I18n::text(16200);' in menu,'USB tab not translated')
sources = {
    'usbcablediagnostictab.h':range(16201,16220),
    'database/MemsDocumentaryUpdateDialog.cpp':range(16300,16336),
    'iamemstab.cpp':range(16400,16425),
    'database/MemsDocumentaryImageViewer.cpp':range(16500,16504),
    'helpviewer.cpp':range(16504,16507),
    'expert/IaMemsService.cpp':range(16600,16603),
    'expert/LocalAiClient.cpp':range(16603,16612),
    'expert/IaMemsDocumentaryPipeline.cpp':range(16700,16714),
}
for filename,range_ in sources.items():
    content=read(filename)
    for key in range_:
        check(f'I18n::text({key})' in content or
              (filename=='iamemstab.cpp' and key==16415 and 'I18n::text(16415)' in content),
              f'{filename}: missing text {key}')
updater=read('database/MemsDocumentaryUpdateDialog.cpp')
check('new ui(' not in updater and ';ui(' not in updater,'updater corrupted')
check('return m_language == QStringLiteral("en") ? en : fr;' in updater,
      'updater not multilingual')
if errors:
    for problem in errors: print('I18N_AUDIT_FAILED:',problem,file=sys.stderr)
    sys.exit(1)
print('SESSION_UI_I18N_AUDIT_OK','languages=6','tabs=15',
      f'new_keys={len(keys)}','guide_chapters=24')
