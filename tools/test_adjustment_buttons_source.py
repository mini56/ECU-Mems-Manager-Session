#!/usr/bin/env python3
"""Source-only regression guard for Qt auto-connected ECU adjustment buttons."""
from pathlib import Path
import re
import xml.etree.ElementTree as ET

root = Path(__file__).resolve().parents[1]
w = (root/"mainwindow.cpp").read_text(encoding="utf8")
h = (root/"mainwindow.h").read_text(encoding="utf8")
m = (root/"memsinterface.cpp").read_text(encoding="utf8")
cmd = (root/"librosco-x64/rosco.h").read_text(encoding="utf8")
widgets = {x.get("name"): x.get("class") for x in ET.parse(root/"mainwindow.ui").iter("widget")}
command_map = {
    "fuel_trim_plus":("MEMS_FuelTrimPlus","0x79"),
    "fuel_trim_minus":("MEMS_FuelTrimMinus","0x7A"),
    "idle_decay_plus":("MEMS_IdleDecayPlus","0x89"),
    "idle_decay_minus":("MEMS_IdleDecayMinus","0x8A"),
    "idle_speed_plus":("MEMS_IdleSpeedPlus","0x91"),
    "idle_speed_minus":("MEMS_IdleSpeedMinus","0x92"),
    "ignition_advance_plus":("MEMS_IgnitionAdvancePlus","0x93"),
    "ignition_advance_minus":("MEMS_IgnitionAdvanceMinus","0x94"),
}
assert "m_ui->setupUi(this);" in w
worker_flat = re.sub(r"\s+", "", m)
assert "private slots:" in h
for name,(constant,hexval) in command_map.items():
    widget="m_"+name+"Button"
    slot="on_"+widget+"_clicked"
    assert widgets.get(widget)=="QPushButton", name
    assert h.count("void "+slot+"();")==1, name
    assert w.count("void MainWindow::"+slot+"()")==1, name
    assert "emit "+name+"();" in w, name
    assert "connect(this, SIGNAL("+name+"()), m_mems, SLOT("+slot+"()))" in w, name
    assert m.count("void MEMSInterface::"+slot+"()")==1, name
    assert re.search(r"\b"+constant+r"\s*=\s*"+hexval,cmd), name
    assert "guardedMemsTestActuator("+constant+",NULL)" in worker_flat, name
    assert not re.search(r"connect\s*\(\s*m_ui->"+widget+r"\s*,\s*SIGNAL\s*\(\s*clicked",w), name
assert "saveTimeouts.ReadTotalTimeoutConstant = 700;" in m
assert "SAVE81_ACK_00_DUREE_TRANSACTION_MS=%1" in m
print("ADJUSTMENTS_EIGHT_SINGLE_QT_SLOT_CONNECTIONS_PASS")
print("SAVE81_CONFIRMED_PATH_PRESERVED")
