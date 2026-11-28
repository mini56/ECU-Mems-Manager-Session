"""Source/translation contract; not a physical actuator test or proof of ECU compatibility."""
from pathlib import Path
import json
import re

root = Path(__file__).resolve().parents[1]
source = (root / "mainwindow.cpp").read_text(encoding="utf-8")
block = source.split("connect(m_mems,&MEMSInterface::actuatorTestReply", 1)[1].split("\n  });", 1)[0]
assert re.search(r'\(bool coil,bool received,int reply,QString\)', block)
assert 'coil?QStringLiteral("coilTestReply"):QStringLiteral("injectorTestReply")' in block
assert 'm_closing||!m_reconnectPolicy.connected()' in block
assert 'I18n::text(2972202).arg(reply)+QStringLiteral(" — ")+I18n::text(2972204)' in block
assert ': I18n::text(2972203)' in block
assert 'received?I18n::text(2972205):QString()' in block
assert not re.search(r'\bfirmware\b|AANMP\d|reply\s*==', re.sub(r'//[^\n]*', '', block))
for lang in ('fr','en','de','es','it','pt'):
    data = json.loads((root / 'translations' / lang / f'{lang}_session222.json').read_text(encoding='utf-8'))
    assert '%1' in data['2972202'], lang
    assert all(data[str(k)].strip() for k in range(2972202,2972206)), lang
print("ACTUATOR_REPLY_MULTIECU_SOURCE_CONTRACT_PASS")
