#include <QApplication>
#include <QCoreApplication>
#include <QEvent>
#include <QMainWindow>
#include <QScrollArea>
#include <QTabWidget>
#include <QTimer>
#include <QWidget>

namespace {

static QWidget *realPage(QWidget *tab)
{
    if (!tab) return nullptr;
    if (QScrollArea *s=qobject_cast<QScrollArea*>(tab)) return s->widget();
    return tab;
}

static bool usesDedicatedLayout(QWidget *page,const QString &title)
{
    if (!page) return false;
    const QString name=page->objectName();
    const QString cls=QString::fromLatin1(page->metaObject()->className());
    const QString t=title.toLower();

    if (name==QStringLiteral("overview_tab") ||
        name==QStringLiteral("emission_tab") ||
        name==QStringLiteral("errors") ||
        name==QStringLiteral("actuators") ||
        name==QStringLiteral("raw") ||
        name==QStringLiteral("ECU")) return true;

    if (cls==QStringLiteral("SummaryTab") ||
        cls==QStringLiteral("AnalysisTab") ||
        cls==QStringLiteral("DiagnosticPanel")) return true;

    return t.contains(QStringLiteral("mesures")) ||
           t.contains(QStringLiteral("données")) ||
           t.contains(QStringLiteral("interactif")) ||
           t.contains(QStringLiteral("rosco")) ||
           t.contains(QStringLiteral("diagnostic")) ||
           t.contains(QStringLiteral("régl")) ||
           t.contains(QStringLiteral("erreur")) ||
           t.contains(QStringLiteral("actionneur"));
}

static void preflight(QMainWindow *window)
{
    if (!window) return;
    QTabWidget *tabs=window->findChild<QTabWidget*>(QStringLiteral("Tab_main"));
    if (!tabs) return;

    // uirebuild.cpp used to create a synthetic recorder tab whenever no child
    // called recorder_tab existed. This invisible child blocks that legacy
    // branch before the shell is assembled; it is removed immediately after.
    if (!tabs->findChild<QWidget*>(QStringLiteral("recorder_tab"))) {
        QWidget *blocker=new QWidget(tabs);
        blocker->setObjectName(QStringLiteral("recorder_tab"));
        blocker->setProperty("uiRecorderBlocker",true);
        blocker->hide();
        QTimer::singleShot(250,blocker,[blocker](){ blocker->deleteLater(); });
    }

    // Dedicated pages must not first be rearranged by composeGenericPage().
    // Mark them before uirebuild handles the same Show/Polish event.
    for (int i=0;i<tabs->count();++i) {
        QWidget *page=realPage(tabs->widget(i));
        if (usesDedicatedLayout(page,tabs->tabText(i)))
            page->setProperty("uiRebuiltPage",true);
    }
}

class UiPreflightInstaller : public QObject
{
public:
    explicit UiPreflightInstaller(QObject *parent=nullptr):QObject(parent) {}
protected:
    bool eventFilter(QObject *watched,QEvent *event) override
    {
        if (!watched || (event->type()!=QEvent::Show && event->type()!=QEvent::Polish))
            return QObject::eventFilter(watched,event);

        QMainWindow *window=qobject_cast<QMainWindow*>(watched);
        if (!window || window->objectName()!=QStringLiteral("MainWindow") ||
            window->property("uiPreflightDone").toBool())
            return QObject::eventFilter(watched,event);

        window->setProperty("uiPreflightDone",true);
        preflight(window);
        return QObject::eventFilter(watched,event);
    }
};

void installUiPreflight()
{
    QApplication *app=qobject_cast<QApplication*>(QCoreApplication::instance());
    if (app) app->installEventFilter(new UiPreflightInstaller(app));
}

}

Q_COREAPP_STARTUP_FUNCTION(installUiPreflight)
