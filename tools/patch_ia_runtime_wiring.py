#!/usr/bin/env python3
from pathlib import Path

ia = Path("iamemstab.cpp")
text = ia.read_text(encoding="utf-8")
text = text.replace('#include "database/MemsReferenceDatabase.h"',
                    '#include "expert/ExpertRuntimeDatabase.h"')
old = '''        MemsReferenceDatabase reference;\n        const bool ok = reference.open();\n        const QString path = ok ? reference.databasePath() : QString();\n        const QString error = ok ? QString() : QStringLiteral("Impossible de construire ou d'ouvrir la base MEMS fusionnée.");\n        reference.close();\n'''
new = '''        ExpertRuntimeDatabase reference;\n        const bool ok = reference.buildOrOpen();\n        const QString path = ok ? reference.databasePath() : QString();\n        const QString error = ok ? QString() : reference.lastError();\n'''
if old not in text and "ExpertRuntimeDatabase reference;" not in text:
    raise SystemExit("IA runtime database block not found")
text = text.replace(old, new, 1)
ia.write_text(text, encoding="utf-8")

cmake = Path("CMakeLists.txt")
text = cmake.read_text(encoding="utf-8")
old = '''    iamemstab.cpp iamemstab.h expert/ExpertEngine.cpp expert/ExpertEngine.h expert/ExpertKnowledgeReader.cpp expert/ExpertKnowledgeReader.h expert/ExpertTypes.h\n'''
new = '''    iamemstab.cpp iamemstab.h expert/ExpertEngine.cpp expert/ExpertEngine.h expert/ExpertKnowledgeReader.cpp expert/ExpertKnowledgeReader.h expert/ExpertRuntimeDatabase.cpp expert/ExpertRuntimeDatabase.h expert/ExpertTypes.h\n'''
if old not in text and "expert/ExpertRuntimeDatabase.cpp" not in text:
    raise SystemExit("CMake IA source line not found")
text = text.replace(old, new, 1)
cmake.write_text(text, encoding="utf-8")
print("IA MEMS compact runtime wiring applied")
