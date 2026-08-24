// Deterministic-navigation build wrapper for IaMemsTab.
//
// The original iamemstab.cpp also registers a Q_COREAPP startup installer that
// repeatedly reinserts the IA MEMS sidebar item with timers.  MEMSX64 now owns
// tab creation/order centrally in navigationorderpatch.cpp, so compile the same
// IaMemsTab implementation while suppressing only that legacy startup hook.
// No IA conversation, ECU observation, expert-engine or LocalAiClient behavior
// is changed here.

#include <QtCore/qcoreapplication.h>

#ifdef Q_COREAPP_STARTUP_FUNCTION
#undef Q_COREAPP_STARTUP_FUNCTION
#endif
#define Q_COREAPP_STARTUP_FUNCTION(AFUNC)

#include "iamemstab.cpp"
