#include "mainwindow.h"
#include "memsinterface.h"
#include "expert/IaResponseLogic.h"

#include <QApplication>
#include <QDateTime>
#include <QEvent>
#include <QPointer>

namespace {

class IaResponseContextPatch final : public QObject
{
public:
    explicit IaResponseContextPatch(QObject *parent = nullptr) : QObject(parent) {}

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        MainWindow *window = qobject_cast<MainWindow*>(watched);
        if (window && (event->type() == QEvent::Show || event->type() == QEvent::Polish))
            bind(window);
        return QObject::eventFilter(watched, event);
    }

private:
    void bind(MainWindow *window)
    {
        if (!window || m_window == window)
            return;
        m_window = window;

        MEMSInterface *mems = window->memsInterface();
        if (!mems)
            return;

        connect(mems, &MEMSInterface::dataReady, this, []() {
            IaResponseLogic::noteMeasurementTimestamp(QDateTime::currentMSecsSinceEpoch());
        }, Qt::QueuedConnection);
    }

    QPointer<MainWindow> m_window;
};

void installIaResponseContextPatch()
{
    if (!qApp)
        return;
    IaResponseContextPatch *patch = new IaResponseContextPatch(qApp);
    qApp->installEventFilter(patch);
}

Q_COREAPP_STARTUP_FUNCTION(installIaResponseContextPatch)

} // namespace
