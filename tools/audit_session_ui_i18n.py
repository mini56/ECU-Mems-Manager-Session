#!/usr/bin/env python3
"""SESSION six-language UI regression gate; no modification of program data."""
import json
import re
import sys
from pathlib import Path

root = Path(__file__).resolve().parents[1]
langs = ('fr', 'en', 'es', 'it', 'pt', 'de')
tabs = (1001,7900,2001,4001,3001,7013,7940,7018,7017,7012,5001,7152,6002,19000,16200,16800)
keys = (list(range(16200,16220))+list(range(16300,16336))+
        list(range(16400,16425))+list(range(16500,16507))+
        list(range(16600,16612))+list(range(16700,16714))+
        list(range(16801,16828))+[16831,16832,16833]+
        list(range(16840,16864))+[16880]+list(range(16890,16898))+
        list(range(16900,16939))+list(range(16940,16960))+[6208])
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
    check(('Lucas 5AS' in guide or 'Lucas-5AS' in guide) and
          'chapter-19' in guide,
          f'{lang}: short Lucas 5AS guide missing')
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
check('return I18n::text(16800);' in menu,'Other ECU tab not translated')
other=read('othercontrolunitstab.h')
check('class OtherControlUnitsTab' in other and
      'I18n::text(16801+i)' in other and
      'createPlipPanel(' in other and
      'rc5protocol.h' in other and
      'lucas5asreader.h' in other and
      'connect5As()' in other and
      'read5As()' in other and
      'connectRc5()' in other and
      'clearRc5()' in other and
      'rc5CableInfoButton' in other and
      'immobiliserCableInfoButton' in other and
      'keysCableInfoButton' in other and
      'showCableHelp(int index)' in other,
      'RC5 and 5AS diagnostic pages or cable help buttons missing')
check(all(s in other for s in (
    'lucas5asTestHornButton','lucas5asTestLedButton',
    'lucas5asTestHazardsButton','lucas5asLiveStatusButton',
    'lucas5asSaveLogButton','QMessageBox confirmation',
    'confirmation.setDefaultButton(QMessageBox::Cancel)',
    'm_5asLiveTimer->setInterval(1000)',
    'QMetaObject::invokeMethod(m_5asWorker,"pulseOutput"')),
    'Lucas 5AS output tests must explain risks and require confirmation')
check(all(x in other for x in
          ('lucas5asPlipControlButton','lucas5asPlipResetButton',
           'recordPlipSnapshot(fields)','m_5asLiveActive || m_plipMonitoring',
           'lucas5asplipmonitor.h')) and
      'lucas5as_plip_selftest' in cmake,
      'PLIP monitor must share the 5AS transport and have a selftest')
for lang in langs:
    help_catalog = json.loads(read(f'translations/{lang}/{lang}_other_ecus.json'))
    for key in (16892,16894,16896):
        help_text = help_catalog.get(str(key),'')
        check(help_text.startswith('<html><body>') and
              help_text.endswith('</body></html>') and
              len(help_text)>900,
              f'{lang}: missing substantive offline cable text {key}')
    rc5_help=help_catalog.get('16892','')
    five_help=help_catalog.get('16894','')
    keys_help=help_catalog.get('16896','')
    check(all(fragment in rc5_help for fragment in ('7','13','KKL')),
          f'{lang}: RC5 DIY OBD pin 7 to pin 13 instructions missing')
    check(all(fragment in five_help for fragment in
              ('FT232R','CH340G','FT_PROG','Invert RXD','15','16','1','3','7')),
          f'{lang}: FTDI/CH340G and 5AS DIY cable instructions missing')
    check(all(fragment in keys_help for fragment in ('Pektron','10AS','5AS')),
          f'{lang}: PLIP DIY cable guidance or variants missing')
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
print('SESSION_UI_I18N_AUDIT_OK','languages=6','tabs=16',
      f'new_keys={len(keys)}','guide_chapters=24')
