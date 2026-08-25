// BUILD #30 deterministic-navigation wrapper for IaMemsTab.
//
// The historical iamemstab.cpp still registers a startup installer that can
// reinsert the IA sidebar item on timers. MEMSX64 owns tab creation/order in
// navigationorderpatch.cpp, so suppress only that legacy startup hook.
//
// BUILD #30 additionally moves the LocalAiClient object to QApplication scope
// immediately before the IA tab is shown. The tab keeps the same pointer and
// signals, but closing/destroying a view no longer owns the llama-server
// lifecycle. LocalAiClient is then destroyed only with QApplication, where its
// existing shutdown() terminates the sidecar normally.

#include <QtCore/qcoreapplication.h>
#include <QApplication>
#include <QEvent>
#include <QObject>

#pragma push_macro("Q_COREAPP_STARTUP_FUNCTION")
#ifdef Q_COREAPP_STARTUP_FUNCTION
#undef Q_COREAPP_STARTUP_FUNCTION
#endif
#define Q_COREAPP_STARTUP_FUNCTION(AFUNC)

#include "iamemstab.cpp"

#pragma pop_macro("Q_COREAPP_STARTUP_FUNCTION")

namespace {

class Build30AiLifetimeFilter final : public QObject
{
public:
    explicit Build30AiLifetimeFilter(QObject *parent = nullptr)
        : QObject(parent)
    {
    }

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if (event && event->type() == QEvent::Show) {
            IaMemsTab *tab = qobject_cast<IaMemsTab*>(watched);
            if (tab && qApp) {
                LocalAiClient *client = tab->findChild<LocalAiClient*>();
                if (client && client->parent() != qApp) {
                    client->setObjectName(QStringLiteral("iaMemsApplicationService"));
                    client->setParent(qApp);
                }
            }
        }
        return QObject::eventFilter(watched, event);
    }
};

void installBuild30AiLifetime()
{
    if (!qApp)
        return;
    qApp->installEventFilter(new Build30AiLifetimeFilter(qApp));
}

Q_COREAPP_STARTUP_FUNCTION(installBuild30AiLifetime)

} // namespace
