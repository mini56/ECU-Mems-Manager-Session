#ifndef NAVIGATIONORDERPATCH_H
#define NAVIGATIONORDERPATCH_H

class QApplication;
class QMainWindow;

void installFinalNavigation(QApplication *app, QMainWindow *window);

// Reapply only after the asynchronously created real sidebar exists.
// Preserves all existing tab pages; returns true when the full 14 identities resolved.
bool refreshFinalNavigation(QMainWindow *window);

#endif
