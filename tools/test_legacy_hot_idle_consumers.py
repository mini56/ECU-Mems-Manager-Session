from pathlib import Path
for file,count in {"logger.cpp":1,"analysistab_impl_part1.inc":1,"mainwindow.cpp":1,
                   "summarytab.cpp":1,"diagnosticpanel.cpp":2}.items():
    text=Path(file).read_text(encoding="utf-8")
    assert text.count("LegacyHotIdle::corrected(")==count, file
    assert "LegacyHotIdle::raw(" in text, file
print("LEGACY_HOT_IDLE_CONSUMERS_PASS")
