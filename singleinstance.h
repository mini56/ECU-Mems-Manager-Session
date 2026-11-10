#ifndef SINGLEINSTANCE_H
#define SINGLEINSTANCE_H
class QApplication;
// The launch and replacement mechanism is owned by the application lifecycle.
void installSingleInstanceManager(QApplication *app);
#endif
