#include "uishellsmoketest.h"

#include "mainwindow.h"
#include "summarytab.h"
#include "i18n.h"
#include "database/DatabaseManager.h"
#include "database/MemsReferenceDatabase.h"
#include "database/MemsReferencePackageRefresh.h"
#include "database/MemsReferenceLanguageSync.h"
#include "database/MemsGlobalSearchIndex.h"

#include <QApplication>
#include <QCoreApplication>
#include <QEvent>
#include <QFrame>
#include <QListWidget>
#include <QLabel>
#include <QHeaderView>
#include <QTableWidget>
#include <QtMath>
#include <QScrollArea>
#include <QStandardPaths>
#include <QTabWidget>
#include <QTimer>
#include <QWidget>
#include <QDebug>
#include <QFile>
#include <QDir>
#include <QDateTime>

namespace {

void logSmoke(const QString &message)
{
    QFile file(QDir::current().filePath(QStringLiteral("session_ui_smoke_trace.log")));
    if (file.open(QIODevice::WriteOnly|QIODevice::Append|QIODevice::Text)){
        file.write((QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)+
                    QStringLiteral(" ")+message+QStringLiteral("\n")).toUtf8());
    }
}

QWidget *actualPage(QWidget *tab)
{
    if (QScrollArea *scroll=qobject_cast<QScrollArea*>(tab))
        return scroll->widget();
    return tab;
}


bool verifyApprovedVisualRules(MainWindow &window)
{
    auto check=[](bool correct,const char *description){
        if(!correct){
            qCritical().noquote() << "SESSION_UI_STYLE_REGRESSION" << description;
            logSmoke(QStringLiteral("STYLE FAIL ")+QString::fromLatin1(description));
        }
        return correct;
    };
    auto tenPointSegoe=[](const QFont &font){
        return font.family()==QStringLiteral("Segoe UI") &&
               qAbs(font.pointSizeF()-10.0)<0.2;
    };
    bool ok=true;

    // The raw 7D/80 pages share Segoe UI 10 pt, with regular data rows
    // and bold column headings.
    QWidget *raw=window.findChild<QWidget*>(QStringLiteral("raw"));
    ok=check(raw!=nullptr,"Raw data page absent") && ok;
    if(raw){
        QWidget *a=raw->findChild<QWidget*>(QStringLiteral("raw_1"));
        QWidget *b=raw->findChild<QWidget*>(QStringLiteral("raw_2"));
        ok=check(a && b,"Both raw data tables required") && ok;
        for(QWidget *block:{a,b}){
            if(!block)continue;
            int rows=0;
            for(QLabel *label:block->findChildren<QLabel*>()){
                if(label->objectName().isEmpty())continue;
                const bool header=label->objectName().startsWith(QStringLiteral("header_"))||
                                  label->objectName().startsWith(QStringLiteral("Aheader_"));
                ok=check(tenPointSegoe(label->font()),"Raw data font changed") && ok;
                ok=check(label->font().bold()==header,"Raw data font weight changed") && ok;
                ++rows;
            }
            ok=check(rows>=10,"Raw table labels unexpectedly missing") && ok;
        }
    }

    // The three measurement tables must preserve typography and a
    // sufficiently wide Interpreted column.
    SummaryTab *summary=window.findChild<SummaryTab*>();
    ok=check(summary!=nullptr,"Measurement page absent") && ok;
    if(summary){
        const QList<QTableWidget*> tables=summary->findChildren<QTableWidget*>();
        ok=check(tables.size()==3,"Three measurement tables required") && ok;
        for(QTableWidget *table:tables){
            ok=check(tenPointSegoe(table->font()) && !table->font().bold(),
                     "Measurement body typography changed") && ok;
            ok=check(tenPointSegoe(table->horizontalHeader()->font()) &&
                     table->horizontalHeader()->styleSheet().contains(QStringLiteral("font-weight:700")),
                     "Measurement header typography changed") && ok;
            ok=check(table->columnCount()==4,"Measurement column count changed") && ok;
        }
    }

    // Approved Actuators arrangement: names at left, lambda help after
    // its title, stepper commands grouped, and large live system indicator.
    QLabel *lambda=window.findChild<QLabel*>(QStringLiteral("m_O2Heater"));
    QLabel *function=window.findChild<QLabel*>(QStringLiteral("m_PTCRelay"));
    ok=check(lambda && function,"Actuator labels missing") && ok;
    for(QLabel *label:{lambda,function}){
        if(label)ok=check((label->alignment() & Qt::AlignHorizontal_Mask)==Qt::AlignLeft,
                          "Actuator function labels must be left-aligned") && ok;
    }
    if(lambda){
        ok=check(!lambda->property("helpBubbleLeft").toBool() && !lambda->toolTip().isEmpty(),
                 "Lambda help must appear after text") && ok;
    }
    QWidget *state=window.findChild<QWidget*>(QStringLiteral("actuatorSystemState"));
    ok=check(state!=nullptr,"Actuator state indicator absent") && ok;
    if(state){
        ok=check(state->minimumWidth()>=270 && state->minimumHeight()>=215 &&
                 state->maximumWidth()>=350,
                 "Actuator state indicator was reduced") && ok;
    }

    // The interactive ECU labels must wrap rather than clip.
    QWidget *interactive=window.findChild<QWidget*>(QStringLiteral("ECU"));
    ok=check(interactive && interactive->layout(),"Interactive page layout missing") && ok;
    if(interactive){
        for(const char *id:{"label_23","label_24","label_48","label_49",
                            "label_50","ecuResetSettingsWarningLabel","label_22"}){
            QLabel *label=interactive->findChild<QLabel*>(QString::fromLatin1(id));
            ok=check(label && label->wordWrap() && tenPointSegoe(label->font()),
                     "Interactive ECU text styling regressed") && ok;
        }
    }
    return ok;
}

bool verifyUi(MainWindow &window)
{
    auto check=[](bool ok,const char *description){
        if (!ok) {
            qCritical().noquote() << "SESSION_UI_SMOKE_FAIL" << description;
            logSmoke(QStringLiteral("FAIL ")+QString::fromLatin1(description));
        }
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
    ok=verifyApprovedVisualRules(window) && ok;
    ok=check(injector!=nullptr,"Native injection dwell source missing") && ok;
    if (injector)
        ok=check(injector->property("maximum").toDouble()==5.0,
                 "Normal-frame injection dwell must use the 0-5ms scale") && ok;

    for (const QSize size : {QSize(1024,700),QSize(1200,768),QSize(1280,800),QSize(1366,768),QSize(1920,1080)}){
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
                               "five window sizes, approved visual style, English language, MEMS/Settings/ROSCO, dwell 0-5ms";
    return ok;
}

} // namespace

int runUiShellSmokeTest(QApplication &app)
{
    logSmoke(QStringLiteral("ENTER smoke test"));
    // Never touch the real install's user profile or persistent database.
    QStandardPaths::setTestModeEnabled(true);
    QCoreApplication::setOrganizationName(QStringLiteral("ECU Mems Manager Smoke"));
    QCoreApplication::setApplicationName(QStringLiteral("ECU Mems Manager UI Smoke"));
    if (!I18n::load(QStringLiteral("fr"))) {
        qCritical() << "SESSION_UI_SMOKE_FAIL French translations unavailable";
        return 2;
    }
    I18n::install(&app);
    // The real application prepares the reference cache, user database and
    // index before it creates MainWindow. Reproduce that lifecycle rather than
    // forcing the page constructor to rebuild the first-run index on the UI thread.
    logSmoke(QStringLiteral("Before reference package preparation"));
    MemsReferencePackageAction action=MemsReferencePackageAction::Unavailable;
    if (!refreshMemsReferencePackage(&action)) {
        logSmoke(QStringLiteral("FAIL reference package preparation"));
        return 4;
    }
    logSmoke(QStringLiteral("Reference package prepared"));
    {
        MemsReferenceDatabase reference;
        if (!reference.open()) {
            logSmoke(QStringLiteral("FAIL reference database open"));
            return 5;
        }
        reference.close();
    }
    synchronizeMemsReferenceLanguage();
    logSmoke(QStringLiteral("Reference language synchronized"));
    DatabaseManager database;
    if (!database.open()) {
        logSmoke(QStringLiteral("FAIL user test database open"));
        return 6;
    }
    logSmoke(QStringLiteral("Before global reference index build"));
    QString indexError;
    if (!MemsGlobalSearchIndex::ensureBuilt(&indexError)) {
        logSmoke(QStringLiteral("FAIL global reference index: ")+indexError);
        return 7;
    }
    logSmoke(QStringLiteral("Global reference index ready"));
    logSmoke(QStringLiteral("Before MainWindow constructor"));

    MainWindow window;
    logSmoke(QStringLiteral("After MainWindow constructor"));
    window.setAttribute(Qt::WA_QuitOnClose,false);
    window.setProperty("ecuSkipExitConfirmation",true);
    window.resize(1280,768);
    window.show();
    logSmoke(QStringLiteral("After show()"));

    // The real showEvent schedules the native shell on the next event turn.
    // No test-only mutation of the page layouts is performed.
    QTimer::singleShot(350,&window,[&app,&window](){
        logSmoke(QStringLiteral("Enter timer / start checks"));
        const bool ok=verifyUi(window);
        logSmoke(ok?QStringLiteral("Checks PASS"):QStringLiteral("Checks FAIL"));
        window.close();
        logSmoke(QStringLiteral("After close"));
        app.exit(ok?0:1);
    });
    QTimer::singleShot(240000,&app,[&app](){
        qCritical() << "SESSION_UI_SMOKE_FAIL UI timeout";
        logSmoke(QStringLiteral("Timeout in event loop"));
        app.exit(3);
    });
    logSmoke(QStringLiteral("Enter Qt event loop"));
    const int result=app.exec();
    logSmoke(QStringLiteral("Qt event loop returned: ")+QString::number(result));
    return result;
}
