from pathlib import Path
import xml.etree.ElementTree as ET

root=ET.parse('mainwindow.ui').getroot()
source=Path('mainwindow.cpp').read_text(encoding='utf-8')
names={w.get('name') for w in root.iter('widget')}
for grid,raw in {
    'HF_table':['m_7D_15','m_7D_17','m_7D_19','m_7D_1B','m_7D_1D','m_7D_1F'],
    'HF_table_2':['m_80_0F','m_80_11','m_80_1B'],
}.items():
    layout=next(x for x in root.iter('layout') if x.get('name')==grid)
    occupied={}
    for item in layout.findall('item'):
        for row in range(int(item.get('row','0')),int(item.get('row','0'))+int(item.get('rowspan','1'))):
            for col in range(int(item.get('column','0')),int(item.get('column','0'))+int(item.get('colspan','1'))):
                occupied[row,col]=item
    for name in raw:
        item=next(i for i in layout.findall('item') if i.find('widget') is not None and i.find('widget').get('name')==name)
        row=int(item.get('row'));assert item.get('column')=='2' and (row,3) not in occupied,(grid,name)
        # TPS references use the hiRaw array rather than an individual extra() call.
        assert f'm_ui->{name}' in source,name
for label in ('label_80x0D','label_80x0E','label_0x05','label_0x0E','label_0x11','label_0x16','label_0x10','label_0x13','label_0x06','label_0x09','label_0x0A','m_JackCountLabel','m_engineerrorLabel','m_long_trim_label'):
    assert label in names and f'm_ui->{label}' in source,label
assert 'm_naFaultHistoryIdentity.clear()' in source
assert 'if(!m_naFaultHistoryIdentity.isEmpty())m_prevFaultMaskValid=false;' in source
print('MEMS16NA_UI_CONTRACT_OK: nine calculated cells available; seven DTC help targets; context reset')

assert 'showNaPages(Mems16Na::SessionEvidence(),true);\n  TooltipBubbleManager::install(this);' in source
assert source.count('showNaPages(Mems16Na::SessionEvidence(),true);')==2
summary=Path('summarytab.cpp').read_text(encoding='utf-8')
assert 'showNaDocumentation();\n  applyVisualTheme' in summary
assert 'dtcDocumentation(groups[i])' in summary
assert 'dtcDocumentation(r.group)' in source
print('MEMS16NA_OFFLINE_SOURCE_CONTRACT_OK')
