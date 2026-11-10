#ifndef UI_SHELL_SMOKE_TEST_H
#define UI_SHELL_SMOKE_TEST_H

class QApplication;

// Executes the normal native UI inside a test-only profile, without ECU commands.
int runUiShellSmokeTest(QApplication &app);

#endif
