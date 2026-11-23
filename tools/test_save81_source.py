#!/usr/bin/env python3
"""Scope-specific source contract for the real-car SAVE 0x81 follow-up.
No serial device is touched and no ECU command is transmitted by this test.
"""
from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
worker = (root / "memsinterface.cpp").read_text(encoding="utf-8")
header = (root / "memsinterface.h").read_text(encoding="utf-8")
window = (root / "mainwindow.cpp").read_text(encoding="utf-8")
protocol = (root / "librosco-x64" / "protocol.c").read_text(encoding="utf-8")
setup = (root / "librosco-x64" / "setup.c").read_text(encoding="utf-8")

start = worker.index("void MEMSInterface::onSaveAdjustmentsRequested()")
end = worker.index("\n/**", start)
save = worker[start:end]
assert "static_cast<actuator_cmd>(0x81)" in save
assert save.count("guardedMemsTestActuator(saveAdjustments, &reply)") == 2
assert "family == MemsEcuFamily::Mems19" in save
assert "saveTimeouts.ReadTotalTimeoutConstant = 700;" in save
assert "saveTimeouts.ReadTotalTimeoutMultiplier = 0;" in save
assert "GetCommTimeouts(port, &previousTimeouts)" in save
assert "SetCommTimeouts(port, &saveTimeouts)" in save
assert "SetCommTimeouts(port, &previousTimeouts)" in save
assert save.index("SetCommTimeouts(port, &previousTimeouts)") < save.index(
    "emit adjustmentsSaveAcknowledged();", save.index("QElapsedTimer transactionTimer;")
)
rosco = save[save.index("QElapsedTimer transactionTimer;"):]
assert "emit errorSendingCommand();" not in rosco, "No false error after 0x81"
assert "SAVE81_ACK_00_DUREE_TRANSACTION_MS=%1" in rosco
assert "SAVE81_CONFIRMATION_NON_RECUE_DUREE_TRANSACTION_MS=%1_REPONSE=%2" in rosco
assert "void adjustmentsSaveDiagnostic(QString detail);" in header
assert "EcuCommunicationLog::note(detail)" in window
assert "m_serviceLoopRunning" in worker
assert "QCoreApplication::processEvents();" in worker
assert "commTimeouts.ReadTotalTimeoutConstant = 100;" in setup
assert "mems_test_actuator(mems_info* info, actuator_cmd cmd, uint8_t* data)" in protocol

# This historical trace demonstrates the actual defect. The new timeout
# includes 145 ms; compilation cannot validate an actual ECU response.
previous_wait_ms, observed_ack_ms, new_wait_ms = 100, 145, 700
assert previous_wait_ms < observed_ack_ms < new_wait_ms

print("SAVE81_SERIAL_TIMEOUT_ISOLATED_SOURCE_CONTRACT_PASS")
print("SAVE81_NO_REAL_ECU_TEST_CI_ONLY")
