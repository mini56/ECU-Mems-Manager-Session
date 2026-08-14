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
                // Run soon after the modern UI has created its widgets so obsolete
                // layers never remain visible long enough to disturb the approved layout.
                QTimer::singleShot(560, window, [window]() { cleanup(window); });
            }
        }
        return QObject::eventFilter(watched, event);
    }

private:
    static void cleanup(QMainWindow *window)
    {
        if (!window) return;

        if (QFrame *modeBar = window->findChild<QFrame*>(QStringLiteral("modernModeBar"))) {
            modeBar->hide();
            modeBar->deleteLater();
        }

        if (QFrame *trendPanel = window->findChild<QFrame*>(QStringLiteral("trendPanel2min"))) {
            trendPanel->hide();
            trendPanel->deleteLater();
        }

        QTabWidget *tabs = window->findChild<QTabWidget*>(QStringLiteral("Tab_main"));
        QListWidget *nav = window->findChild<QListWidget*>(QStringLiteral("modernNavigation"));
        if (!tabs) return;

        QWidget *database = tabs->findChild<QWidget*>(QStringLiteral("database_tab"));
        if (database) {
            const int index = tabs->indexOf(database);
            if (index >= 0) {
                if (nav && index < nav->count())
                    delete nav->takeItem(index);
                tabs->removeTab(index);
            }
            database->deleteLater();
        }

        // Keep all real application pages available. No invented expert/basic filtering.
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
