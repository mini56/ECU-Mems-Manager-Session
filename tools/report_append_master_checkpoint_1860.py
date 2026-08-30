from pathlib import Path

report = Path('RAPPORT_SUIVI_ECU_MEMS_MANAGER.md')
marker = '## 2026-08-30 - RAPPORT MAITRE UNIQUE : CONSOLIDATION VALIDEE A DISTANCE'
current = report.read_text(encoding='utf-8')
if marker in current:
    print('master checkpoint already present; nothing to append')
    raise SystemExit(0)
raise SystemExit('master checkpoint unexpectedly missing; refusing duplicate helper path')
