#ifndef SINGLEINSTANCE_H
#define SINGLEINSTANCE_H
#include <QtGlobal>
class QApplication;
// The launch and replacement mechanism is owned by the application lifecycle.
void installSingleInstanceManager(QApplication *app);
bool waitForRestartParent(quint32 pid);
#endif
