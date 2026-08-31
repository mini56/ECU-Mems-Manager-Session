from pathlib import Path
p=Path('tools/rave_v2_mems_display_sim.py')
s=p.read_text(encoding='utf-8')
old="icon_box=(int(452*SCALE),int(17*SCALE),int(553*SCALE),int(59*SCALE))"
new="icon_box=(int(470*SCALE),int(17*SCALE),int(553*SCALE),int(59*SCALE))"
if old not in s:
    raise SystemExit('header icon crop anchor not found')
p.write_text(s.replace(old,new,1),encoding='utf-8')
