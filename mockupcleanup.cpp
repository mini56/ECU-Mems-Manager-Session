#include <QApplication>
#include <QCoreApplication>
#include <QEvent>
#include <QFrame>
#include <QListWidget>
#include <QMainWindow>
#include <QTabWidget>
#include <QTimer>
#include <QWidget>

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
                QTimer::singleShot(1200, window, [window]() { cleanup(window); });
            }
        }
        return QObject::eventFilter(watched, event);
    }

private:
    static void cleanup(QMainWindow *window)
    {
        if (!window) return;

        // The approved mockup has one top header only. Remove the extra mode bar
        // introduced by the first modernization pass.
        if (QFrame *modeBar = window->findChild<QFrame*>(QStringLiteral("modernModeBar"))) {
            modeBar->hide();
            modeBar->deleteLater();
        }

        // The mockup uses the 2-minute trace inside each gauge card. Remove the
        // duplicate legacy trend panel that was added below the gauges.
        if (QFrame *trendPanel = window->findChild<QFrame*>(QStringLiteral("trendPanel2min"))) {
            trendPanel->hide();
            trendPanel->deleteLater();
        }

        QTabWidget *tabs = window->findChild<QTabWidget*>(QStringLiteral("Tab_main"));
        QListWidget *nav = window->findChild<QListWidget*>(QStringLiteral("modernNavigation"));
        if (!tabs) return;

        // Remove the placeholder tab that is not part of the approved mockup.
        QWidget *database = tabs->findChild<QWidget*>(QStringLiteral("database_tab"));
        if (database) {
            const int index = tabs->indexOf(database);
            if (index >= 0) {
                if (nav && index < nav->count()) {
                    delete nav->takeItem(index);
                }
                tabs->removeTab(index);
            }
            database->deleteLater();
        }

        // The approved mockup shows the real application pages directly.
        // Do not hide ECU/ROSCO or automatic diagnostics behind an invented mode.
        for (int i = 0; i < tabs->count(); ++i) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
            tabs->setTabVisible(i, true);
#else
            tabs->setTabEnabled(i, true);
#endif
            if (nav && i < nav->count())
                nav->item(i)->setHidden(false);
        }

        if (nav && nav->count() > 0 && tabs->currentIndex() >= 0 && tabs->currentIndex() < nav->count())
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
