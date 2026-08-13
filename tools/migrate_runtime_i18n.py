#!/usr/bin/env python3
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]

# One-time source migration. This script is NOT part of the final build.
CPP_FILES = [
    'main.cpp', 'mainwindow.cpp', 'aboutbox.cpp', 'optionsdialog.cpp',
    'helpviewer.cpp', 'analysistab.cpp', 'captureviewer.cpp',
    'diagnosticpanel.cpp', 'summarytab.cpp'
]

for rel in CPP_FILES:
    p = ROOT / rel
    if not p.exists():
        continue
    s = p.read_text(encoding='utf-8')
    s = s.replace('#include <QTranslator>\n', '')
    s = s.replace('QObject::tr(', 'I18n::text(')
    if '#include "i18n.h"' not in s:
        lines = s.splitlines(True)
        last_inc = max((i for i,l in enumerate(lines) if l.lstrip().startswith('#include')), default=-1)
        lines.insert(last_inc + 1, '#include "i18n.h"\n#define tr I18n::text\n')
        s = ''.join(lines)
    p.write_text(s, encoding='utf-8')

# main.cpp: replace QTranslator/.qm loading with direct dictionary loading.
p = ROOT / 'main.cpp'
s = p.read_text(encoding='utf-8')
s = re.sub(
    r'\n\s*QTranslator translator;\s*\n\s*const QString translationPath =.*?\n\s*if \(translator\.load\(translationPath\)\)\s*\n\s*app\.installTranslator\(&translator\);',
    '\n    I18n::load(language);\n    I18n::install(&app);',
    s, flags=re.S)
p.write_text(s, encoding='utf-8')

# CMake: remove LinguistTools and all .qm generation; copy dictionaries as data.
p = ROOT / 'CMakeLists.txt'
s = p.read_text(encoding='utf-8')
s = s.replace(' Charts Sql LinguistTools)', ' Charts Sql)')
s = s.replace('    main.cpp\n', '    main.cpp\n    i18n.cpp\n    i18n.h\n', 1)
start = s.find('# Traductions multilingues')
end = s.find('\nif(MSVC)', start)
if start != -1 and end != -1:
    replacement = '''# Dictionnaires multilingues charges directement par l'application.\n# Aucun .qm et aucune traduction n'est generee pendant la compilation.\nadd_custom_command(TARGET ${PNAME} POST_BUILD\n    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PNAME}>/translations"\n    COMMAND ${CMAKE_COMMAND} -E copy_if_different\n        "${CMAKE_SOURCE_DIR}/translations/ECUMemsManager_fr.ts"\n        "${CMAKE_SOURCE_DIR}/translations/ECUMemsManager_en.ts"\n        "${CMAKE_SOURCE_DIR}/translations/ECUMemsManager_es.ts"\n        "${CMAKE_SOURCE_DIR}/translations/ECUMemsManager_it.ts"\n        "${CMAKE_SOURCE_DIR}/translations/ECUMemsManager_pt.ts"\n        "${CMAKE_SOURCE_DIR}/translations/ECUMemsManager_de.ts"\n        "$<TARGET_FILE_DIR:${PNAME}>/translations"\n)\n'''
    s = s[:start] + replacement + s[end:]
p.write_text(s, encoding='utf-8')

print('Runtime I18n migration complete')
