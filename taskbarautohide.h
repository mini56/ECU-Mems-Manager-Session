#ifndef TASKBARAUTOHIDE_H
#define TASKBARAUTOHIDE_H

#include <QtGlobal>

// Windows taskbar is a desktop-wide preference. This controller owns only
// changes requested explicitly by the user, and restores the prior state.
namespace TaskbarAutoHide
{
    bool apply(bool requested);
    void restore();
    void recoverPreviousSession();
    int monitorParent(quint32 parentProcessId,quint64 sessionToken);
}

#endif // TASKBARAUTOHIDE_H
