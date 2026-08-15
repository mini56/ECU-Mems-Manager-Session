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

    // Wait until the existing Interactive page has completed its normal
    // dedicated-layout initialization. Reordering earlier would change the
    // legacy numeric tab indexes used during startup.
    QWidget *interactivePage=realPage(interactive);
    if(interactivePage && !interactivePage->property("strictInteractiveBuilt").toBool())
        return;

    QWidget *database=findDatabaseTab(tabs);
    if(!database) {
        database=new QWidget(tabs);
        database->setObjectName(QStringLiteral("database_tab"));
        database->setMinimumSize(0,0);
        database->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
        database->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    }

    QWidget *current=tabs->currentWidget();
    const QString interactiveTitle=tabs->tabText(tabs->indexOf(interactive));

    int databaseIndex=tabs->indexOf(database);
    if(databaseIndex>=0)
        tabs->removeTab(databaseIndex);

    const int interactiveIndex=tabs->indexOf(interactive);
    if(interactiveIndex>=0)
        tabs->removeTab(interactiveIndex);

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
            QTimer::singleShot(1200,w,[w](){prepareDatabaseAndInteractiveOrder(w);});
            QTimer::singleShot(1800,w,[w](){prepareDatabaseAndInteractiveOrder(w);});
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
