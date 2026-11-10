#include "uishellsmoketest.h"

#include "mainwindow.h"
#include "i18n.h"

#include <QApplication>
#include <QCoreApplication>
#include <QEvent>
#include <QFrame>
#include <QListWidget>
#include <QScrollArea>
#include <QStandardPaths>
#include <QTabWidget>
#include <QTimer>
#include <QWidget>
#include <QDebug>

namespace {

QWidget *actualPage(QWidget *tab)
{
    if (QScrollArea *scroll=qobject_cast<QScrollArea*>(tab))
        return scroll->widget();
    return tab;
}

bool verifyUi(MainWindow &window)
{
    auto check=[](bool ok,const char *description){
        if (!ok) qCritical().noquote() << "SESSION_UI_SMOKE_FAIL" << description;
        return ok;
    };

    bool ok=true;
    QTabWidget *tabs=window.findChild<QTabWidget*>(QStringLiteral("Tab_main"));
    QListWidget *navigation=window.findChild<QListWidget*>(QStringLiteral("uiRebuildNav"));
    ok=check(window.property("uiShellReady").toBool(),"Native shell not constructed") && ok;
    ok=check(tabs && tabs->count()==14,"Official 14 pages are not present") && ok;
    ok=check(navigation && navigation->count()==14,"Sidebar does not contain the 14 official entries") && ok;
    if (!tabs || !navigation || tabs->count()!=14 || navigation->count()!=14)
        return false;

    // A switch must work from both the sidebar and the underlying tab widget.
    // No test clicks ECU connection, actuator, reset or session buttons.
    for (int i=0;i<14;i++){
        QWidget *page=actualPage(tabs->widget(i));
        ok=check(page!=nullptr,"Missing page widget") && ok;
        ok=check(!tabs->tabText(i).trimmed().isEmpty(),"Missing translated page title") && ok;
        navigation->setCurrentRow(i);
        QCoreApplication::processEvents();
        ok=check(tabs->currentIndex()==i,"Sidebar to page navigation mismatch") && ok;
    }
    for (int i=13;i>=0;i--){
        tabs->setCurrentIndex(i);
        QCoreApplication::processEvents();
        ok=check(navigation->currentRow()==i,"Page to sidebar navigation mismatch") && ok;
    }

    ok=check(window.findChild<QWidget*>(QStringLiteral("memsDatabaseBrowser"))!=nullptr,
             "Native MEMS database page missing") && ok;
    ok=check(window.findChild<QFrame*>(QStringLiteral("settingsMetrics"))!=nullptr,
             "Native Settings gauges missing") && ok;
    ok=check(window.findChild<QWidget*>(QStringLiteral("injectionRamTestButton"))!=nullptr,
             "Native ROSCO AANMP002 action missing") && ok;
    QWidget *injector=window.findChild<QWidget*>(QStringLiteral("m_injector_time"));
    ok=check(injector!=nullptr,"Native injection dwell source missing") && ok;
    if (injector)
        ok=check(injector->property("maximum").toDouble()==5.0,
                 "Normal-frame injection dwell must use the 0-5ms scale") && ok;

    for (const QSize size : {QSize(1024,700),QSize(1280,800),QSize(1366,768)}){
        window.resize(size);
        QCoreApplication::processEvents();
        ok=check(tabs->count()==14 && navigation->count()==14,
                 "Page count changed on resize") && ok;
        ok=check(tabs->currentIndex()==navigation->currentRow(),
                 "Navigation desynchronized on resize") && ok;
    }

    if (I18n::load(QStringLiteral("en"))){
        QEvent languageChange(QEvent::LanguageChange);
        QCoreApplication::sendEvent(&window,&languageChange);
        QCoreApplication::processEvents();
        ok=check(tabs->count()==14 && navigation->count()==14,
                 "Language change modified the official page count") && ok;
        for (int i=0;i<14;i++)
            ok=check(navigation->item(i)->text()==tabs->tabText(i),
                     "Sidebar and page translations disagree") && ok;
    } else {
        ok=check(false,"English translation catalog is unavailable") && ok;
    }

    if (ok) qInfo().noquote() << "SESSION_UI_SMOKE_PASS 14 native pages, bidirectional navigation, "
                               "three window sizes, English language, MEMS/Settings/ROSCO, dwell 0-5ms";
    return ok;
}

} // namespace

int runUiShellSmokeTest(QApplication &app)
{
    // Never touch the real install's user profile or persistent database.
    QStandardPaths::setTestModeEnabled(true);
    QCoreApplication::setOrganizationName(QStringLiteral("ECU Mems Manager Smoke"));
    QCoreApplication::setApplicationName(QStringLiteral("ECU Mems Manager UI Smoke"));
    if (!I18n::load(QStringLiteral("fr"))) {
        qCritical() << "SESSION_UI_SMOKE_FAIL French translations unavailable";
        return 2;
    }
    I18n::install(&app);

    MainWindow window;
    window.setAttribute(Qt::WA_QuitOnClose,false);
    window.setProperty("ecuSkipExitConfirmation",true);
    window.resize(1280,768);
    window.show();

    // The real showEvent schedules the native shell on the next event turn.
    // No test-only mutation of the page layouts is performed.
    QTimer::singleShot(350,&window,[&app,&window](){
        const bool ok=verifyUi(window);
        window.close();
        app.exit(ok?0:1);
    });
    QTimer::singleShot(60000,&app,[&app](){
        qCritical() << "SESSION_UI_SMOKE_FAIL UI timeout";
        app.exit(3);
    });
    return app.exec();
}
