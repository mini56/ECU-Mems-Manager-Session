from pathlib import Path
import xml.etree.ElementTree as ET

root=ET.parse('mainwindow.ui').getroot()
source=Path('mainwindow.cpp').read_text(encoding='utf-8')
names={w.get('name') for w in root.iter('widget')}
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

# SESSION 209: shared qualified diagnostic and help styles.
service=Path('expert/IaMemsService.cpp').read_text(encoding='utf-8')
route=Path('expert/IaMemsLibraryIntegration.cpp').read_text(encoding='utf-8')
assert 'setNaDiagnostic(naEvidence,*data,m_diagnosticPanel->naDiagnosticReport())' in source
assert 'IaMemsNa::observation(na,data' in service and 'm_naReport+' in service
assert 'naAnswer(trimmed)' in route
assert 'documentedUnusedDtc(r.group)' in source and 'documentedUnusedDtc(groups[i])' in summary
shell=Path('uishell.cpp').read_text(encoding='utf-8')
main=Path('main.cpp').read_text(encoding='utf-8')
options=Path('optionsdialog.cpp').read_text(encoding='utf-8')
assert 'file->setParent(fileGroup)' in shell and 'file->setParent(utils)' not in shell
assert 'file->setVisible(!collapsed)' not in shell and 'statusFileEditor' in shell
assert main.index('buildUiShell(&window)') < main.index('window.showMaximized()') < main.index('splash->finish(&window)')
assert main.index('STARTUP_FIRST_RENDER_MS') < main.index('splash->finish(&window)')
assert 'm_options->getThemeChanged() || m_options->getLanguageChanged()' in source
assert 'waitForRestartParent(pid)' in main and 'ecuRestartRequested' in main
assert 'confirm.exec()!=QMessageBox::Yes' in options and 'settings.sync();' in options
print('SESSION_212_STARTUP_RESTART_STATUS_CONTRACT_OK')

# SESSION 214: deterministic page creation before first shell build.
installer=Path('mappedinjection_runtime_part3.inc').read_text(encoding='utf-8')
assert 'bool ensureMappedInjectionPage(MainWindow *window)' in installer
assert 'MappedInjectionInstaller' not in installer and 'QTimer::singleShot(0,window,[window,tabs,analysis]' not in installer
assert 'Q_COREAPP_STARTUP_FUNCTION(installMappedInjectionUi)' not in Path('mappedinjection_runtime_part4.inc').read_text(encoding='utf-8')
assert shell.index('if(!ensureMappedInjectionPage(') < shell.index('rebuild(window, central, root, tabs, legacy);')
assert 'uiShellNavigationReady(&window,true)' in main and 'menu=16' in main
assert 'uiShellNavigationReady(&window,true)' in Path('uishellsmoketest.cpp').read_text(encoding='utf-8')
print('SESSION_214_STARTUP_NAVIGATION_CONTRACT_OK')

# Technical measurement coverage and translation keys, not graphical approval.
from pathlib import Path
import json,re
root=Path(__file__).resolve().parents[1]
def read(p): return (root/p).read_text(encoding='utf-8')
summary=read('summarytab.cpp')
for family,n in [('2J',31),('3',12)]:
    fields=summary.split('static const Field fields'+family+'[] = {',1)[1].split('};',1)[0]
    assert len(re.findall(r'\{"[^"]+",\d+,',fields))==n
for key in ('DATA_00_RAW','DATA_06_RAW','_received'):
    assert key in summary
for p in ('mems2jprotocol.h','mems3protocol.h'):
    assert 'raw.insert(key,received)' in read(p)
ids={str(x) for x in (*range(29110,29115),*range(29120,29156))}
for lang in ('fr','en','de','es','it','pt'):
    j=json.loads(read(f'translations/{lang}/{lang}_session215.json'))
    assert set(j)==ids and all(isinstance(v,str) and v.strip() for v in j.values()),lang
print('SESSION_215_STYLE_MEASUREMENT_HELP_TRANSLATION_CONTRACT_PASS')
