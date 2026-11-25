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

# SESSION 209: shared qualified diagnostic and help styles.
service=Path('expert/IaMemsService.cpp').read_text(encoding='utf-8')
route=Path('expert/IaMemsLibraryIntegration.cpp').read_text(encoding='utf-8')
assert 'setNaDiagnostic(naEvidence,*data,m_diagnosticPanel->naDiagnosticReport())' in source
assert 'IaMemsNa::observation(na,data' in service and 'm_naReport+' in service
assert 'naAnswer(trimmed)' in route
assert 'documentedUnusedDtc(r.group)' in source and 'documentedUnusedDtc(groups[i])' in summary
styles=Path('styles.h').read_text(encoding='utf-8')
assert 'inline QString helpTooltipStyle(bool dark)' in styles
assert 'helpTooltipStyle(false)' in styles and 'helpTooltipStyle(true)' in styles
assert 'border: 1px solid #ff8a22;' in styles and 'border-radius: 8px;' in styles
print('SESSION_209_SHARED_DIAGNOSTIC_AND_HELP_CONTRACT_OK')

# SESSION 210: one source of the small orange help pictogram.
from struct import unpack
icon_header=Path('helpbubble.png').read_bytes()[:29]
assert icon_header[:8]==b'\x89PNG\r\n\x1a\n' and icon_header[12:16]==b'IHDR'
assert unpack('>II',icon_header[16:24])==(42,42)
assert icon_header[25]==6, 'RGBA with transparent background required'
manager=Path('tooltipbubblemanager.cpp').read_text(encoding='utf-8')
header=Path('tooltipbubblemanager.h').read_text(encoding='utf-8')
assert 'helpIconSize() { return 14; }' in header
assert 'helpHitSize() { return 18; }' in header
assert 'QPixmap pixmap(QStringLiteral(":/icons/helpbubble.png"))' in manager
assert 'pixmap.setDevicePixelRatio(qreal(pixmap.width())/helpIconSize())' in manager
assert 'bubble->setScaledContents(false)' in manager
assert 'TooltipBubbleManager::install(dialog)' in manager
assert 'TooltipBubbleManager::helpIconSize()' in summary
assert 'HELP' not in Path('helpbubble.svg').read_text(encoding='utf-8')
assert '<file>helpbubble.png</file>' in Path('resources.qrc').read_text(encoding='utf-8')
assert '+ helpTooltipStyle(true)' in Path('uishell.cpp').read_text(encoding='utf-8')
print('SESSION_210_SHARED_SMALL_HELP_ICON_SOURCE_CONTRACT_OK')

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
