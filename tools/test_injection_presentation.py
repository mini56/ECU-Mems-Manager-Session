from pathlib import Path
import json, re
root=Path(__file__).resolve().parents[1]
dashboard=(root/'mappedinjection_runtime_part2.inc').read_text(encoding='utf-8').split('static bool responseMode')[0]
status=(root/'mappedinjection_runtime_part3.inc').read_text(encoding='utf-8')
assert 'hexAddress(' not in dashboard
for id, value in ((7913,'m_baseRawValue'),(7914,'m_transientRawValue'),(7915,'m_counterValue')):
    assert f'I18n::text({id}).arg({value})' in dashboard
assert 'QStringLiteral("statusFile")' not in status
assert 'findChild<QLineEdit*>(QStringLiteral("m_logFileNameBox"))' in status
assert 'statusRefreshTimer' in status
for lang in ('fr','en','de','es','it','pt'):
    j=json.loads((root/'translations'/lang/f'{lang}_zz_injection.json').read_text(encoding='utf-8'))
    for id in (7907,7908,7910,7911,7913,7914,7915,7917,7918,7919,7922,7923,7924,7925,7927,7928,7929):
        assert not re.search(r'0x|7D/80|\bRAM\b|\bROM\b|(?:Mode|Modo|Modus|Modalità)\s*[34]', j[str(id)],re.I),(lang,id)
    for id in (7913,7914,7915):
        assert j[str(id)].count('%1')==1 and '%2' not in j[str(id)]
print('INJECTION_PRESENTATION_CONTRACT_OK')
