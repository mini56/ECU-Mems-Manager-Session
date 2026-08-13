#!/usr/bin/env python3
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
keyed=ROOT/'main_keyed.cpp'
main=ROOT/'main.cpp'
cmake=ROOT/'CMakeLists.txt'
if not keyed.exists(): raise SystemExit('main_keyed.cpp missing')
main.write_text(keyed.read_text(encoding='utf-8'),encoding='utf-8')
text=cmake.read_text(encoding='utf-8')
if 'main_keyed.cpp' not in text: raise SystemExit('main_keyed.cpp not found in CMakeLists.txt')
cmake.write_text(text.replace('main_keyed.cpp','main.cpp',1),encoding='utf-8')
print('main.cpp synchronized from keyed startup source')
print('CMake now compiles main.cpp')
