#!/usr/bin/env python3
from pathlib import Path

path = Path("iamemstab.cpp")
text = path.read_text(encoding="utf-8")
old = '''    if (containsAny(text, {QStringLiteral("anormal"), QStringLiteral("normal ?"),\n                           QStringLiteral("diagnostic"), QStringLiteral("panne"),\n                           QStringLiteral("probleme"), QStringLiteral("cause"),\n                           QStringLiteral("hypothese")}))\n        return analysisAnswer(false);\n'''
new = '''    if (containsAny(text, {QStringLiteral("anormal"), QStringLiteral("normal"),\n                           QStringLiteral("normale"), QStringLiteral("coherent"),\n                           QStringLiteral("coherente"), QStringLiteral("diagnostic"),\n                           QStringLiteral("panne"), QStringLiteral("probleme"),\n                           QStringLiteral("cause"), QStringLiteral("hypothese")}))\n        return analysisAnswer(false);\n'''
if old not in text and 'QStringLiteral("coherente")' not in text:
    raise SystemExit("diagnostic routing block not found")
text = text.replace(old, new, 1)
old2 = '''    if (containsAny(text, {QStringLiteral("source"), QStringLiteral("certain"),\n                           QStringLiteral("sait-on"), QStringLiteral("sais tu"),\n'''
new2 = '''    if (containsAny(text, {QStringLiteral("source"), QStringLiteral("certain"),\n                           QStringLiteral("certaine"), QStringLiteral("fiable"),\n                           QStringLiteral("preuve"), QStringLiteral("sait-on"), QStringLiteral("sais tu"),\n'''
if old2 not in text and 'QStringLiteral("certaine")' not in text:
    raise SystemExit("knowledge routing block not found")
text = text.replace(old2, new2, 1)
path.write_text(text, encoding="utf-8")
print("IA MEMS natural question routing patched")
