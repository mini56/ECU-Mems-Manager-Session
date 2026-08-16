#include <QApplication>
#include <QCoreApplication>
#include <QEvent>
#include <QListWidget>
#include <QMainWindow>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QTabWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include "i18n.h"
#include "database/MemsDatabaseBrowser.h"

namespace {

static QWidget *realPage(QWidget *tab)
{
    if (!tab) return nullptr;
    if (QScrollArea *scroll = qobject_cast<QScrollArea*>(tab)) return scroll->widget();
    return tab;
}

static QWidget *findInteractiveTab(QTabWidget *tabs)
{
    if (!tabs) return nullptr;
    for (int i = 0; i < tabs->count(); ++i) {
        QWidget *tab = tabs->widget(i);
        QWidget *page = realPage(tab);
        if (page && page->objectName() == QStringLiteral("ECU")) return tab;
    }
    return nullptr;
}

static QWidget *findDatabaseTab(QTabWidget *tabs)
{
    if (!tabs) return nullptr;
    for (int i = 0; i < tabs->count(); ++i) {
        QWidget *tab = tabs->widget(i);
        QWidget *page = realPage(tab);
        if (page && page->objectName() == QStringLiteral("database_tab")) return tab;
    }
    return nullptr;
}

static void buildDatabasePage(QWidget *database)
{
    if (!database) return;
    if (database->findChild<MemsDatabaseBrowser*>(QStringLiteral("memsDatabaseBrowser"), Qt::FindDirectChildrenOnly)) return;

    database->setMinimumSize(0,0);
    database->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
    database->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    database->setAttribute(Qt::WA_StyledBackground,true);
    database->setStyleSheet(QStringLiteral("#database_tab{background:#090e13;}"));

    QVBoxLayout *root = qobject_cast<QVBoxLayout*>(database->layout());
    if (!root) {
        root = new QVBoxLayout(database);
        root->setContentsMargins(0,0,0,0);
        root->setSpacing(0);
    }

    MemsDatabaseBrowser *browser = new MemsDatabaseBrowser(database);
    browser->setObjectName(QStringLiteral("memsDatabaseBrowser"));
    browser->setMinimumSize(0,0);
    browser->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
    browser->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    root->addWidget(browser,1);
    database->setProperty("databaseFunctionalBrowserBuilt",true);
}

static void syncNavigation(QMainWindow *window,QTabWidget *tabs)
{
    if (!window || !tabs) return;
    QListWidget *nav = window->findChild<QListWidget*>(QStringLiteral("uiRebuildNav"));
    if (!nav) return;
    const QSignalBlocker blocker(nav);
    nav->clear();
    for (int i=0;i<tabs->count();++i) nav->addItem(tabs->tabText(i).trimmed());
    nav->setCurrentRow(qBound(0,tabs->currentIndex(),qMax(0,tabs->count()-1)));
}

static void prepareDatabaseAndInteractiveOrder(QMainWindow *window)
{
    if (!window) return;
    QTabWidget *tabs = window->findChild<QTabWidget*>(QStringLiteral("Tab_main"));
    if (!tabs) return;

    QWidget *interactive = findInteractiveTab(tabs);
    if (!interactive) return;
    QWidget *database = findDatabaseTab(tabs);

    if (database && tabs->count() >= 2
        && tabs->indexOf(database) == tabs->count()-2
        && tabs->indexOf(interactive) == tabs->count()-1) {
        buildDatabasePage(realPage(database));
        tabs->setTabText(tabs->indexOf(database),I18n::text(7152));
        syncNavigation(window,tabs);
        window->setProperty("databaseTabPrepared",true);
        return;
    }

    if (!database) {
        database = new QWidget(tabs);
        database->setObjectName(QStringLiteral("database_tab"));
        database->setMinimumSize(0,0);
        database->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
        database->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    }
    buildDatabasePage(realPage(database));

    QWidget *current = tabs->currentWidget();
    const QString interactiveTitle = tabs->tabText(tabs->indexOf(interactive));
    const int databaseIndex = tabs->indexOf(database);
    if (databaseIndex >= 0) tabs->removeTab(databaseIndex);
    const int interactiveIndex = tabs->indexOf(interactive);
    if (interactiveIndex >= 0) tabs->removeTab(interactiveIndex);
    tabs->addTab(database,I18n::text(7152));
    tabs->addTab(interactive,interactiveTitle);
    if (current && tabs->indexOf(current) >= 0) tabs->setCurrentWidget(current);
    syncNavigation(window,tabs);
    window->setProperty("databaseTabPrepared",true);
}

class TabOrderInstaller : public QObject
{
public:
    explicit TabOrderInstaller(QObject *parent=nullptr):QObject(parent){}
protected:
    bool eventFilter(QObject *watched,QEvent *event) override
    {
        QMainWindow *window=qobject_cast<QMainWindow*>(watched);
        if (!window || window->objectName()!=QStringLiteral("MainWindow")) return QObject::eventFilter(watched,event);
        if ((event->type()==QEvent::Show || event->type()==QEvent::Polish)
            && !window->property("databaseTabScheduled").toBool()) {
            window->setProperty("databaseTabScheduled",true);
            QTimer::singleShot(1400,window,[window](){prepareDatabaseAndInteractiveOrder(window);});
            QTimer::singleShot(2400,window,[window](){prepareDatabaseAndInteractiveOrder(window);});
            QTimer::singleShot(3600,window,[window](){prepareDatabaseAndInteractiveOrder(window);});
        } else if (event->type()==QEvent::LanguageChange
                   && window->property("databaseTabPrepared").toBool()) {
            QTimer::singleShot(0,window,[window](){prepareDatabaseAndInteractiveOrder(window);});
        }
        return QObject::eventFilter(watched,event);
    }
};

void installTabOrderPatch()
{
    QApplication *app=qobject_cast<QApplication*>(QCoreApplication::instance());
    if (app) app->installEventFilter(new TabOrderInstaller(app));
}

}

Q_COREAPP_STARTUP_FUNCTION(installTabOrderPatch)
