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
#include <QWidget>
#include "i18n.h"

namespace {

static QWidget *realPage(QWidget *tab)
{
    if(!tab) return nullptr;
    if(QScrollArea *scroll=qobject_cast<QScrollArea*>(tab)) return scroll->widget();
    return tab;
}

static QWidget *findInteractiveTab(QTabWidget *tabs)
{
    if(!tabs) return nullptr;
    for(int i=0;i<tabs->count();++i) {
        QWidget *tab=tabs->widget(i);
        QWidget *page=realPage(tab);
        if(page && page->objectName()==QStringLiteral("ECU"))
            return tab;
    }
    return nullptr;
}

static QWidget *findDatabaseTab(QTabWidget *tabs)
{
    if(!tabs) return nullptr;
    for(int i=0;i<tabs->count();++i) {
        QWidget *tab=tabs->widget(i);
        QWidget *page=realPage(tab);
        if(page && page->objectName()==QStringLiteral("database_tab"))
            return tab;
    }
    return nullptr;
}

static void syncNavigation(QMainWindow *w,QTabWidget *tabs)
{
    if(!w || !tabs) return;
    QListWidget *nav=w->findChild<QListWidget*>(QStringLiteral("uiRebuildNav"));
    if(!nav) return;
    const QSignalBlocker blocker(nav);
    nav->clear();
    for(int i=0;i<tabs->count();++i)
        nav->addItem(tabs->tabText(i).trimmed());
    nav->setCurrentRow(qBound(0,tabs->currentIndex(),qMax(0,tabs->count()-1)));
}

static void prepareDatabaseAndInteractiveOrder(QMainWindow *w)
{
    if(!w) return;
    QTabWidget *tabs=w->findChild<QTabWidget*>(QStringLiteral("Tab_main"));
    if(!tabs) return;

    QWidget *interactive=findInteractiveTab(tabs);
    if(!interactive) return;

    QWidget *database=findDatabaseTab(tabs);

    // If the requested order is already in place, only refresh the translated
    // Database title and keep the navigation list synchronized.
    if(database && tabs->count()>=2 &&
       tabs->indexOf(database)==tabs->count()-2 &&
       tabs->indexOf(interactive)==tabs->count()-1) {
        tabs->setTabText(tabs->indexOf(database),I18n::text(7152));
        syncNavigation(w,tabs);
        w->setProperty("databaseTabPrepared",true);
        return;
    }

    if(!database) {
        database=new QWidget(tabs);
        database->setObjectName(QStringLiteral("database_tab"));
        database->setMinimumSize(0,0);
        database->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
        database->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    }

    QWidget *current=tabs->currentWidget();
    const QString interactiveTitle=tabs->tabText(tabs->indexOf(interactive));

    const int databaseIndex=tabs->indexOf(database);
    if(databaseIndex>=0)
        tabs->removeTab(databaseIndex);

    const int interactiveIndex=tabs->indexOf(interactive);
    if(interactiveIndex>=0)
        tabs->removeTab(interactiveIndex);

    // Required final menu order: Database immediately before Interactive,
    // with Interactive always the last tab.
    tabs->addTab(database,I18n::text(7152));
    tabs->addTab(interactive,interactiveTitle);

    if(current && tabs->indexOf(current)>=0)
        tabs->setCurrentWidget(current);

    syncNavigation(w,tabs);
    w->setProperty("databaseTabPrepared",true);
}

class TabOrderInstaller : public QObject
{
public:
    explicit TabOrderInstaller(QObject *parent=nullptr):QObject(parent){}
protected:
    bool eventFilter(QObject *watched,QEvent *event) override
    {
        QMainWindow *w=qobject_cast<QMainWindow*>(watched);
        if(!w || w->objectName()!=QStringLiteral("MainWindow"))
            return QObject::eventFilter(watched,event);

        if((event->type()==QEvent::Show || event->type()==QEvent::Polish) &&
           !w->property("databaseTabScheduled").toBool()) {
            w->setProperty("databaseTabScheduled",true);
            // Run after the normal page initialization. Multiple harmless
            // passes make the final menu order deterministic at startup.
            QTimer::singleShot(1400,w,[w](){prepareDatabaseAndInteractiveOrder(w);});
            QTimer::singleShot(2400,w,[w](){prepareDatabaseAndInteractiveOrder(w);});
            QTimer::singleShot(3600,w,[w](){prepareDatabaseAndInteractiveOrder(w);});
        } else if(event->type()==QEvent::LanguageChange && w->property("databaseTabPrepared").toBool()) {
            QTimer::singleShot(0,w,[w](){prepareDatabaseAndInteractiveOrder(w);});
        }
        return QObject::eventFilter(watched,event);
    }
};

void installTabOrderPatch()
{
    QApplication *app=qobject_cast<QApplication*>(QCoreApplication::instance());
    if(app) app->installEventFilter(new TabOrderInstaller(app));
}

}

Q_COREAPP_STARTUP_FUNCTION(installTabOrderPatch)
