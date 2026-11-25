from pathlib import Path
import json,re
root=Path(__file__).resolve().parents[1]
def read(p): return (root/p).read_text(encoding='utf-8')
style=read('styles.h')
assert 'constexpr int kSessionScrollBarExtent=9;' in style
assert 'sessionScrollBarStyle(false)' in style and 'sessionScrollBarStyle(true)' in style
assert 'width:%2px' in style and 'height:%2px' in style
assert style.count('QScrollBar:vertical{')==1
assert '#ff8a22' in style and 'QTipLabel, QToolTip' in style
for p in ('darkstyle.cpp','uishell.cpp','analysistab_impl_part1.inc','database/MemsDatabaseBrowser.cpp','database/MemsDatabaseExplorerV2.cpp'):
    assert not re.search(r'QScrollBar(?:::handle|::add-line|::sub-line|::add-page|::sub-page|:vertical|:horizontal)',read(p)),p
for p in ('analysistab_impl_part1.inc','iamemstab.cpp','database/MemsDatabaseBrowser.cpp'):
    assert 'setFixedWidth(kSessionScrollBarExtent)' in read(p),p
assert 'surface->setStyleSheet(style)' in read('tooltipbubblemanager.cpp')
assert 'bar->setStyleSheet(style)' in read('tooltipbubblemanager.cpp')
summary=read('summarytab.cpp')
assert len(re.findall(r'= addRow\(',summary))==56
assert 'setRowHidden(lo[i]%m_rowsPerTable,false)' in summary
assert 'setRowHidden(m_rowUk10%m_rowsPerTable,false)' in summary
for family,n in [('2J',31),('3',12)]:
    fields=summary.split('static const Field fields'+family+'[] = {',1)[1].split('};',1)[0]
    assert len(re.findall(r'\{"[^"]+",\d+,',fields))==n
for key in ('DATA_00_RAW','DATA_06_RAW','_received'):
    assert key in summary
for p in ('mems2jprotocol.h','mems3protocol.h'):
    assert 'raw.insert(key,received)' in read(p)
help=read('helpviewer.cpp')
assert 'setTextElideMode(Qt::ElideNone)' in help and 'font-size:11pt' in help
assert 'HelpChapterDelegate' in help
options=read('optionsdialog.cpp')
assert 'StyleDark' not in options and 'I18n::text(6105)' in options and 'I18n::text(29110)' in options
ids={str(x) for x in (*range(29110,29115),*range(29120,29156))}
for lang in ('fr','en','de','es','it','pt'):
    j=json.loads(read(f'translations/{lang}/{lang}_session215.json'))
    assert set(j)==ids and all(isinstance(v,str) and v.strip() for v in j.values()),lang
print('SESSION_215_STYLE_MEASUREMENT_HELP_TRANSLATION_CONTRACT_PASS')
