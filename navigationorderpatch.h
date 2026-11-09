#ifndef NAVIGATIONORDERPATCH_H
#define NAVIGATIONORDERPATCH_H

class QApplication;
class QMainWindow;

void installFinalNavigation(QApplication *app, QMainWindow *window);
// Synchronize the full 14-page menu after the sidebar exists.
bool refreshFinalNavigation(QMainWindow *window);

#endif
