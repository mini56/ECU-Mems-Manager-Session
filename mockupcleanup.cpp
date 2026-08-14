#include <QApplication>
#include <QCoreApplication>
#include <QEvent>
#include <QListWidget>
#include <QMainWindow>
#include <QTabWidget>

namespace {

class MockupCleanupInstaller : public QObject
{
public:
    explicit MockupCleanupInstaller(QObject *parent = nullptr) : QObject(parent) {}

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if ((event->type() == QEvent::Show || event->type() == QEvent::Polish) && watched) {
            QMainWindow *window = qobject_cast<QMainWindow*>(watched);
            if (window && window->objectName() == QStringLiteral("MainWindow") &&
                !window->property("mockupCleanupInstalled").toBool()) {
                window->setProperty("mockupCleanupInstalled", true);
                ensureRealPagesVisible(window);
            }
        }
        return QObject::eventFilter(watched, event);
    }

private:
    static void ensureRealPagesVisible(QMainWindow *window)
    {
        if (!window) return;
        QTabWidget *tabs = window->findChild<QTabWidget*>(QStringLiteral("Tab_main"));
        if (!tabs) return;

        QListWidget *nav = window->findChild<QListWidget*>(QStringLiteral("modernNavigation"));
        for (int i = 0; i < tabs->count(); ++i) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
            tabs->setTabVisible(i, true);
#else
            tabs->setTabEnabled(i, true);
#endif
            if (nav && i < nav->count()) nav->item(i)->setHidden(false);
        }

        if (nav && tabs->currentIndex() >= 0 && tabs->currentIndex() < nav->count())
            nav->setCurrentRow(tabs->currentIndex());
    }
};

void installMockupCleanupHook()
{
    QApplication *app = qobject_cast<QApplication*>(QCoreApplication::instance());
    if (!app) return;
    MockupCleanupInstaller *installer = new MockupCleanupInstaller(app);
    app->installEventFilter(installer);
}

}

Q_COREAPP_STARTUP_FUNCTION(installMockupCleanupHook)
