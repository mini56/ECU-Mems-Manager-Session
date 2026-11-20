#include "uishellsmoketest.h"

#include "mainwindow.h"
#include "helpviewer.h"
#include "optionsdialog.h"
#include "styles.h"
#include "applicationvisualstyle.h"
#include "summarytab.h"
#include "analysistab.h"
#include "i18n.h"
#include "database/DatabaseManager.h"
#include "database/MemsReferenceDatabase.h"
#include "database/MemsReferencePackageRefresh.h"
#include "database/MemsReferenceLanguageSync.h"
#include "database/MemsGlobalSearchIndex.h"

#include <QApplication>
#include <QAction>
#include <QDialog>
#include <QSlider>
#include <QUrl>
#include <QCoreApplication>
#include <QEvent>
#include <QFrame>
#include <QListWidget>
#include <QLayout>
#include <QPushButton>
#include <QToolButton>
#include <QLabel>
#include <QCheckBox>
#include <QComboBox>
#include <QImage>
#include <QPixmap>
#include <QTextBrowser>
#include <QHeaderView>
#include <QTableWidget>
#include <QTableView>
#include <QtMath>
#include <QScrollArea>
#include <QScrollBar>
#include <QStandardPaths>
#include <QTabWidget>
#include <QTimer>
#include <QWidget>
#include <QDebug>
#include <QFile>
#include <QDir>
#include <QDateTime>
#include <QTemporaryFile>

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
            // Check the actual approved 10pt bold font, not a dark-only
            // inline stylesheet that the light theme intentionally clears.
            QHeaderView *header=table->horizontalHeader();
            const bool validHeader=header && tenPointSegoe(header->font()) &&
                     header->font().bold() && table->horizontalHeaderItem(0)!=nullptr;
            if(!validHeader && header)
                logSmoke(QStringLiteral("Measurement header: family=%1 pointSize=%2 weight=%3 bold=%4")
                    .arg(header->font().family()).arg(header->font().pointSizeF())
                    .arg(header->font().weight()).arg(header->font().bold()));
            ok=check(validHeader,"Measurement header typography changed") && ok;
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
    ok=check(tabs && tabs->count()==15,"Official 15 pages are not present") && ok;
    ok=check(navigation && navigation->count()==15,"Sidebar does not contain the 15 official entries") && ok;
    if (!tabs || !navigation || tabs->count()!=15 || navigation->count()!=15)
        return false;

    // The update action is a footer control, immediately after the footer
    // label. There must be only one button and no header duplicate.
    QFrame *databaseFooter=window.findChild<QFrame*>(
        QStringLiteral("memsDatabaseFooter"));
    QLabel *updateTitle=window.findChild<QLabel*>(
        QStringLiteral("memsDatabaseUpdateTitle"));
    QPushButton *databaseUpdate=window.findChild<QPushButton*>(
        QStringLiteral("documentaryUpdateButton"));
    ok=check(databaseFooter && updateTitle && databaseUpdate,
             "Database update action must be present in footer") && ok;
    if(databaseFooter && updateTitle && databaseUpdate){
        QLayout *footerLayout=databaseFooter->layout();
        ok=check(databaseUpdate->parentWidget()==databaseFooter &&
                 footerLayout &&
                 footerLayout->indexOf(databaseUpdate)==
                    footerLayout->indexOf(updateTitle)+1,
                 "Update button must follow database footer title") && ok;
        ok=check(databaseUpdate->text()==QStringLiteral(
                     "Rechercher une mise à jour"),
                 "French update button label is incorrect") && ok;
    }
    ok=check(window.findChildren<QPushButton*>(
                QStringLiteral("documentaryUpdateButton")).size()==1,
             "Duplicate database update action detected") && ok;

    // The database page now belongs to the actual document workspace:
    // no redundant six statistics cards or obsolete "disabled" message.
    for(int key : {7168,7169,7170,7171,7172,7173}){
        bool found=false;
        for(QLabel *item : window.findChildren<QLabel*>())
            found |= item->property("i18nKey").toInt()==key &&
                     item->property("i18nKey").isValid();
        ok=check(!found,
                 "Redundant database metric cards are still constructed") && ok;
    }
    ok=check(window.findChild<QLabel*>(
                 QStringLiteral("memsDatabaseUpdateState"))==nullptr,
             "Obsolete disabled database update status remains") && ok;
    QLabel *headerScope=window.findChild<QLabel*>(
        QStringLiteral("memsDatabaseScopeSummary"));
    ok=check(headerScope && headerScope->wordWrap() &&
             headerScope->text().contains(QStringLiteral("MEMS 1.9")),
             "Database heading must show full system list without clipping") && ok;

    QTableWidget *resultTable=window.findChild<QTableWidget*>(
        QStringLiteral("memsDatabaseResults"));
    ok=check(resultTable && resultTable->horizontalHeader(),
             "Database result columns not present") && ok;
    if(resultTable && resultTable->horizontalHeader()){
        QHeaderView *header=resultTable->horizontalHeader();
        ok=check(!header->stretchLastSection() &&
                 header->sectionResizeMode(0)==QHeaderView::Fixed &&
                 header->sectionResizeMode(1)==QHeaderView::Fixed &&
                 header->sectionResizeMode(2)==QHeaderView::Fixed &&
                 resultTable->columnCount()==3 &&
                 resultTable->columnWidth(1)==114 &&
                 resultTable->columnWidth(2)==120,
                 "Database layout: Result grows, Generation 114, Type 120") && ok;
    }

    // The revised help loads real bundled HTML into an offline dark Qt dialog.
    // It must contain the update instructions and a navigable section index.
    HelpViewer manual(QStringLiteral("SESSION UI TEST"),&window);
    QListWidget *chapters=manual.findChild<QListWidget*>(
        QStringLiteral("memsHelpNavigation"));
    QTextBrowser *helpBody=manual.findChild<QTextBrowser*>(
        QStringLiteral("memsHelpContent"));
    ok=check(chapters && chapters->count()>=29,
             "Help must provide an offline chapter navigation") && ok;
    ok=check(helpBody &&
             helpBody->toPlainText().contains(QStringLiteral(
                 "Rechercher une mise à jour")) &&
             helpBody->toPlainText().contains(QStringLiteral(
                 "Masquer automatiquement la barre des tâches")),
             "Help must document database updates and optional taskbar behaviour") && ok;
    ok=check(helpBody &&
             helpBody->toPlainText().contains(QStringLiteral("MEMS 2J")) &&
             helpBody->toHtml().contains(QStringLiteral("qrc:/help/images/")),
             "Help must document MEMS 2J and link bundled images for zoom") && ok;
    // Exercise the real bundled screenshot viewer, including interactive zoom.
    const QUrl screenshot(QStringLiteral(
        "qrc:/help/images/Capture%20d%E2%80%99%C3%A9cran%20(905).png"));
    const bool invoked=QMetaObject::invokeMethod(
        &manual,"onAnchorClicked",Qt::DirectConnection,Q_ARG(QUrl,screenshot));
    QDialog *imageDialog=manual.findChild<QDialog*>(
        QStringLiteral("memsHelpImageZoom"));
    QSlider *imageZoom=imageDialog?imageDialog->findChild<QSlider*>(
        QStringLiteral("memsHelpImageZoomSlider")):nullptr;
    QLabel *expanded=imageDialog?imageDialog->findChild<QLabel*>(
        QStringLiteral("memsHelpZoomedImage")):nullptr;
    const bool imageOpened=invoked && imageDialog && imageZoom &&
                           expanded && expanded->pixmap() &&
                           !expanded->pixmap()->isNull();
    int startingWidth=imageOpened?expanded->pixmap()->width():0;
    if(imageZoom) imageZoom->setValue(200);
    const bool zoomWorked=imageOpened && expanded->pixmap() &&
        expanded->pixmap()->width()>=startingWidth &&
        expanded->pixmap()->width()>550;
    ok=check(imageOpened && zoomWorked,
             "Help screenshot click must open a scalable offline image") && ok;
    if(imageDialog)imageDialog->close();
    ok=check(window.findChild<QPushButton*>(
                 QStringLiteral("uiProgramUpdateButton")) &&
             window.findChild<QAction*>(
                 QStringLiteral("programUpdateCheckAction")),
             "Program update badge and manual Help menu action missing") && ok;
    ok=check(manual.minimumWidth()>=850 &&
             manual.styleSheet().contains(
                qApp->property("ecuDarkTheme").toBool()
                    ?QStringLiteral("#090e13"):QStringLiteral("#ffffff")),
             "Help dialog must be larger and follow the selected theme") && ok;

    // Header regression: the old help-bubble icon covered the last
    // digits of the advertised version. Preserve the ordinary tooltip
    // but do not overlay its text or remove space for the version.
    QPushButton *updateBadge=window.findChild<QPushButton*>(
        QStringLiteral("uiProgramUpdateButton"));
    ok=check(updateBadge!=nullptr,
             "Missing visible-header program update button") && ok;
    if(updateBadge){
        updateBadge->setText(QStringLiteral("Nouvelle version 1.1.181"));
        updateBadge->setMinimumWidth(
            updateBadge->fontMetrics().horizontalAdvance(updateBadge->text())+30);
        updateBadge->show();
        QCoreApplication::processEvents();
        QLabel *bubble=updateBadge->findChild<QLabel*>(
            QStringLiteral("_ecuHelpBubble"),Qt::FindDirectChildrenOnly);
        ok=check((!bubble || !bubble->isVisible()) &&
                 !updateBadge->toolTip().isEmpty() &&
                 updateBadge->width()>=
                    updateBadge->fontMetrics().horizontalAdvance(updateBadge->text())+30,
                 "Update button version obscured by tooltip bubble or clipped") && ok;
        updateBadge->hide();
    }
    // Dark and light have explicit independent foreground/background pairs.
    // Test the genuine stylesheet text used by both application startup and
    // rebuilt SESSION shell, without any theme-dependent hardcoded widget patch.
    ok=check(STYLE_DARK.contains(QStringLiteral("background-color: #1b2731;")) &&
             STYLE_DARK.contains(QStringLiteral("color: #f4f7fa;")) &&
             STYLE_LIGHT.contains(QStringLiteral("background-color: #fff9f1;")) &&
             STYLE_LIGHT.contains(QStringLiteral("color: #202936;")) &&
             qApp->styleSheet().contains(QStringLiteral("QToolTip")),
             "Tooltips must have legible contrasting colors in both themes") && ok;

    OptionsDialog *options=window.findChild<OptionsDialog*>();
    QCheckBox *autoHide=options?options->findChild<QCheckBox*>(
        QStringLiteral("taskbarAutoHideCheckbox")):nullptr;
    QLabel *explanation=options?options->findChild<QLabel*>(
        QStringLiteral("taskbarAutoHideExplanation")):nullptr;
    ok=check(autoHide && explanation,
             "Taskbar auto-hide control and explanation must exist") && ok;
    if(autoHide && explanation && options){
        // A child can be visible *relative to its own hidden parent* in Qt;
        // isVisibleTo(options)==false was the wrong assertion. Open the
        // dialog and check the actual user-facing explanation.
        const bool defaultDisabled=!autoHide->isChecked();
        const bool explainSystemWide=explanation->text().contains(
            QStringLiteral("tout Windows"));
        const bool explainRestore=explanation->text().contains(
            QStringLiteral("restaur"));
        options->show();
        QCoreApplication::processEvents();
        const bool readable=explanation->isVisibleTo(options);
        options->hide();
        ok=check(defaultDisabled,
                 "Taskbar auto-hide must be unchecked by default") && ok;
        ok=check(explainSystemWide && explainRestore,
                 "Taskbar explanation must describe Windows-wide change and recovery") && ok;
        ok=check(readable,
                 "Taskbar explanation must be shown when the Options dialog opens") && ok;
    }

    QFrame *sidebar=window.findChild<QFrame*>(
        QStringLiteral("darkSidebar"));
    QToolButton *toggle=window.findChild<QToolButton*>(
        QStringLiteral("darkSidebarToggle"));
    ok=check(sidebar && toggle,
             "Native sidebar and collapse control missing") && ok;
    // Verify BEFORE clicking any tab. SESSION 151 used to start with a
    // narrower menu and only expand on the first navigation event.
    const int expandedWidth=sidebar ? sidebar->width() : 0;
    ok=check(!sidebar || !sidebar->property("collapsed").toBool(),
             "Sidebar must start expanded, without any click") && ok;
    ok=check(tabs->currentIndex()==0 && navigation->currentRow()==0,
             "The initial visible page and selection must both be Overview") && ok;
    ok=check(expandedWidth==236 && navigation->width()==236,
             "Startup sidebar width must already match its canonical 236px width") && ok;
    // The historical failure was VERTICAL spacing, not sidebar width.
    // The first frame must have its final row height before any tab click.
    const int startupRowHeight=navigation->item(0)->sizeHint().height();
    ok=check(startupRowHeight>=27 && startupRowHeight<=35,
             "Startup rows are too compact before first page click") && ok;
    ok=check(navigation->selectedItems().size()==1 &&
             navigation->selectedItems().first()==navigation->item(0),
             "Overview must be visually selected at startup") && ok;
    // Simulate the reported maximized Windows view BEFORE any navigation.
    // On a full-height window, all menu entries must already use the
    // spacious layout seen after clicking a page (35px).
    window.resize(1920,1080);
    QCoreApplication::processEvents();
    const int fullHeightBeforeClick=navigation->item(0)->sizeHint().height();
    ok=check(fullHeightBeforeClick==35,
             "Full-height menu must have its final 35px rows before clicking") && ok;
    navigation->setCurrentRow(6);
    QCoreApplication::processEvents();
    ok=check(navigation->item(0)->sizeHint().height()==fullHeightBeforeClick,
             "First page click must not change menu row spacing") && ok;
    ok=check(tabs->currentIndex()==6 &&
             navigation->selectedItems().size()==1 &&
             navigation->selectedItems().first()==navigation->item(6),
             "IA MEMS must be the one highlighted tab after selecting it") && ok;
    // Restore the normal test geometry and initial tab for the general tour.
    window.resize(1280,768);
    tabs->setCurrentIndex(0);
    QCoreApplication::processEvents();

    // A switch must work from both the sidebar and the underlying tab widget.
    // No test clicks ECU connection, actuator, reset or session buttons.
    for (int i=0;i<15;i++){
        QWidget *page=actualPage(tabs->widget(i));
        ok=check(page!=nullptr,"Missing page widget") && ok;
        ok=check(!tabs->tabText(i).trimmed().isEmpty(),"Missing translated page title") && ok;
        navigation->setCurrentRow(i);
        QCoreApplication::processEvents();
        ok=check(tabs->currentIndex()==i,"Sidebar to page navigation mismatch") && ok;
        ok=check(navigation->currentRow()==tabs->currentIndex() &&
                 navigation->currentItem() &&
                 navigation->currentItem()->text()==tabs->tabText(i) &&
                 navigation->selectedItems().size()==1 &&
                 navigation->selectedItems().first()==navigation->item(i),
                 "Displayed page must be the ONLY visually highlighted sidebar row") && ok;
        ok=check(navigation->item(0)->sizeHint().height()==startupRowHeight,
                 "Page click must not change row spacing at the same window height") && ok;
        if(sidebar)
            ok=check(sidebar->width()==236 &&
                     navigation->width()==236,
                     "Every page must preserve the original 236px menu width") && ok;
    }
    for (int i=13;i>=0;i--){
        tabs->setCurrentIndex(i);
        QCoreApplication::processEvents();
        ok=check(navigation->currentRow()==i &&
                 navigation->selectedItems().size()==1 &&
                 navigation->selectedItems().first()==navigation->item(i),
                 "Programmatic page change must also update the visual selection") && ok;
        if(sidebar)
            ok=check(sidebar->width()==expandedWidth,
                     "Page change must preserve sidebar width") && ok;
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

    if(toggle && sidebar){
        toggle->click();
        QCoreApplication::processEvents();
        ok=check(sidebar->width()==58 && sidebar->property("collapsed").toBool(),
                 "Collapse must produce the single canonical icon width") && ok;
        tabs->setCurrentIndex(6);
        QCoreApplication::processEvents();
        ok=check(sidebar->width()==58,
                 "Changing pages must not unexpectedly expand the sidebar") && ok;
        toggle->click();
        QCoreApplication::processEvents();
        ok=check(sidebar->width()==expandedWidth &&
                 !sidebar->property("collapsed").toBool(),
                 "Expand must restore the original sidebar width") && ok;
    }

    for (const QSize size : {QSize(1024,700),QSize(1200,768),QSize(1280,800),QSize(1366,768),QSize(1920,1080)}){
        window.resize(size);
        QCoreApplication::processEvents();
        ok=check(tabs->count()==15 && navigation->count()==15,
                 "Page count changed on resize") && ok;
        ok=check(tabs->currentIndex()==navigation->currentRow() &&
                 navigation->selectedItems().size()==1 &&
                 navigation->selectedItems().first()==
                    navigation->item(tabs->currentIndex()),
                 "Navigation visual selection desynchronized on resize") && ok;
        if(sidebar)
            ok=check(sidebar->width()==expandedWidth &&
                     navigation->width()==expandedWidth,
                     "Resizing must not impose a competing sidebar width") && ok;
        // Verify real geometry with the Database tab visible after layout and
        // resize, rather than merely inspecting section resize modes.
        const int previousTab=tabs->currentIndex();
        tabs->setCurrentIndex(11);
        QCoreApplication::processEvents();
        if(resultTable && resultTable->horizontalHeader()){
            const bool compactColumns=resultTable->columnWidth(1)==114 &&
                                      resultTable->columnWidth(2)==120;
            // The splitter can leave less than 234 px for the table at
            // small window widths. In that case horizontal scrolling is
            // expected: Result still gets every available pixel (min 40).
            const int expectedResult=qMax(40,resultTable->viewport()->width()-114-120);
            const bool resultGetsSpace=resultTable->columnWidth(0)==expectedResult;
            logSmoke(QStringLiteral("Database columns at %1px: viewport=%2 Result=%3 Generation=%4 Type=%5")
                .arg(size.width()).arg(resultTable->viewport()->width())
                .arg(resultTable->columnWidth(0)).arg(resultTable->columnWidth(1))
                .arg(resultTable->columnWidth(2)));
            ok=check(compactColumns && resultGetsSpace,
                     "Database displayed columns must keep Type 120 and expand Result") && ok;
        }
        tabs->setCurrentIndex(previousTab);
        QCoreApplication::processEvents();
    }

    // SESSION 157 let the dark-only rebuild override the saved "Clair"
    // choice at startup. Test a real two-way theme switch on the native
    // 15-page shell, with its original geometry and selection intact.
    const bool initialDark=qApp->property("ecuDarkTheme").toBool();
    const int originalTab=tabs->currentIndex();
    const int originalWidth=navigation->width();
    const int originalRow=navigation->item(0)->sizeHint().height();
    for(const bool dark:{false,true,false}){
        qApp->setProperty("ecuDarkTheme",dark);
        qApp->setStyleSheet(dark?STYLE_DARK:STYLE_LIGHT);
        applyApplicationVisualStyle(&window);
        QCoreApplication::processEvents();
        // The raw 7D/80 page must never show white data on a white surface.
        // Verify both sets of labels across light -> dark -> light switches.
        if(QWidget *raw=window.findChild<QWidget*>(QStringLiteral("raw"))){
            bool coloursOk=true;
            int dataLabels=0;
            for(const char *id:{"raw_1","raw_2"}){
                QWidget *block=raw->findChild<QWidget*>(QString::fromLatin1(id));
                if(!block){coloursOk=false;continue;}
                for(QLabel *label:block->findChildren<QLabel*>()){
                    const QString name=label->objectName();
                    const bool header=name.startsWith(QStringLiteral("header_")) ||
                                      name.startsWith(QStringLiteral("Aheader_"));
                    const QString expected=header
                        ?QStringLiteral("color:#ff9828;")
                        :(dark?QStringLiteral("color:#ffffff;")
                              :QStringLiteral("color:#202936;"));
                    if(!label->styleSheet().contains(expected)) coloursOk=false;
                    if(!header)++dataLabels;
                }
            }
            ok=check(coloursOk && dataLabels>=10,
                 "Raw 7D/80 text must remain readable in both themes") && ok;
        }
        const QString global=qApp->styleSheet();
        ok=check(dark
                ?global.contains(QStringLiteral("#1e2128"))
                :global.contains(QStringLiteral("#darkSidebar")) &&
                 global.contains(QStringLiteral("background:#f5f7fa")),
             "Global theme must actually change between white/light and dark") && ok;
        QFrame *sidebarShell=window.findChild<QFrame*>(
            QStringLiteral("darkSidebar"));
        ok=check(sidebarShell &&
                 (dark?sidebarShell->styleSheet().contains(
                            QStringLiteral("#070c11"))
                       :sidebarShell->styleSheet().contains(
                            QStringLiteral("#f5f7fa"))),
                 "Native sidebar must follow the light/dark theme") && ok;
        // Do not accept a mere setting/stylesheet string change: render
        // an actual sidebar pixel. This catches the SESSION 157 failure
        // where "Clair" was stored but the UI remained black.
        if(sidebarShell && sidebarShell->height()>150){
            const QImage drawn=sidebarShell->grab().toImage();
            const QPoint point(qMax(5,drawn.width()-15),
                               qBound(80,drawn.height()/2,drawn.height()-80));
            const QColor color=drawn.pixelColor(point);
            const bool actualLight=color.red()>150 &&
                                  color.green()>150 && color.blue()>150;
            const bool actualDark=color.red()<110 &&
                                 color.green()<110 && color.blue()<110;
            if(!(dark?actualDark:actualLight))
                logSmoke(QStringLiteral("Sidebar pixel mismatch: dark=%1 rgb=(%2,%3,%4) alpha=%5 at %6,%7 size=%8x%9")
                    .arg(dark).arg(color.red()).arg(color.green()).arg(color.blue())
                    .arg(color.alpha()).arg(point.x()).arg(point.y())
                    .arg(drawn.width()).arg(drawn.height()));
            ok=check(dark?actualDark:actualLight,
                     "Rendered menu surface must be light or dark, not merely a saved theme preference") && ok;
        }
        ok=check(tabs->count()==15 && navigation->count()==15 &&
                 navigation->width()==originalWidth &&
                 navigation->item(0)->sizeHint().height()==originalRow &&
                 tabs->currentIndex()==originalTab &&
                 navigation->currentRow()==originalTab &&
                 navigation->selectedItems().size()==1 &&
                 navigation->selectedItems().first()==
                     navigation->item(originalTab),
                 "Theme changes must not disturb 15 pages, sidebar geometry or selection") && ok;

        manual.show();
        QCoreApplication::processEvents();
        ok=check(manual.styleSheet().contains(dark
                        ?QStringLiteral("#090e13")
                        :QStringLiteral("#ffffff")) &&
                 manual.findChild<QListWidget*>(
                     QStringLiteral("memsHelpNavigation"))->count()>=24,
                 "Help window, chapters and title-bar theme must remain coherent") && ok;
        manual.hide();
    }
    qApp->setProperty("ecuDarkTheme",initialDark);
    qApp->setStyleSheet(initialDark?STYLE_DARK:STYLE_LIGHT);
    applyApplicationVisualStyle(&window);
    QCoreApplication::processEvents();

    // Also test the Options dialog itself, without accepting or writing
    // any preference. A visual preview cannot change the stored setting.
    if(options){
        QComboBox *theme=options->findChild<QComboBox*>(
            QStringLiteral("themeSelection"));
        ok=check(theme && theme->findData(QStringLiteral("light"))>=0 &&
                 theme->findData(QStringLiteral("dark"))>=0,
                 "Options must expose both real light and dark selections") && ok;
        if(theme){
            const int originalIndex=theme->currentIndex();
            for(const bool dark:{true,false}){
                theme->setCurrentIndex(theme->findData(
                    dark?QStringLiteral("dark"):QStringLiteral("light")));
                QCoreApplication::processEvents();
                ok=check(options->styleSheet().contains(dark
                        ?QStringLiteral("#090e13")
                        :QStringLiteral("#ffffff")),
                        "Options dialog must preview white/light and dark reliably") && ok;
            }
            theme->setCurrentIndex(originalIndex);
        }
    }

    // Native six-language visual regression: all 15 page titles follow the
    // selected SESSION language, not the Windows system locale.
    const int officialTitleKeys[15] = {
        1001,7900,2001,4001,3001,7013,7940,7018,7017,7012,
        5001,7152,6002,19000,16200
    };
    for (const QString &language : {
             QStringLiteral("en"),QStringLiteral("es"),
             QStringLiteral("it"),QStringLiteral("pt"),
             QStringLiteral("de"),QStringLiteral("fr")}) {
        if (!I18n::load(language)) {
            ok=check(false,"Six-language translation catalog unavailable") && ok;
            continue;
        }
        QEvent languageChange(QEvent::LanguageChange);
        QCoreApplication::sendEvent(&window,&languageChange);
        QCoreApplication::processEvents();
        ok=check(tabs->count()==15 && navigation->count()==15,
                 "Language changed the official 15-page count") && ok;
        for (int i=0;i<15;i++) {
            ok=check(navigation->item(i)->text()==tabs->tabText(i),
                     "Sidebar and page titles disagree in selected language") && ok;
            ok=check(tabs->tabText(i)==I18n::text(officialTitleKeys[i]),
                     "Page title does not match selected SESSION language") && ok;
        }
    }

    // Test the actual installed MainWindow Analysis page, NOT a separate
    // AnalysisTab widget. Load a synthetic CSV with more channels than fit,
    // then force enough graphs to exceed the available viewport height.
    // The previous regression only tested scrollbar policy, not visibility.
    {
        tabs->setCurrentIndex(7);
        QCoreApplication::processEvents();
        QCoreApplication::processEvents();
        QWidget *analysisPage=tabs->widget(7);
        AnalysisTab *analysis=qobject_cast<AnalysisTab*>(analysisPage);
        if (!analysis && analysisPage)
            analysis=analysisPage->findChild<AnalysisTab*>();
        ok=check(analysis!=nullptr,
                 "Installed Analysis page not found inside native window") && ok;
        if (analysis){
            QTemporaryFile sample(QDir::tempPath()+
                                  QStringLiteral("/session-analysis-XXXXXX.csv"));
            const bool fileReady=sample.open();
            ok=check(fileReady,"Synthetic Analysis CSV could not be created") && ok;
            if (fileReady){
                QByteArray header("time");
                for(int column=0;column<36;++column)
                    header.append(",synthetic_signal_").append(QByteArray::number(column));
                sample.write("# Smoke test only, no candidate or ECU data\n");
                sample.write(header+"\n");
                for(int row=0;row<40;++row){
                    QByteArray line=QStringLiteral("00:00:%1")
                        .arg(row,2,10,QChar('0')).toLatin1();
                    for(int column=0;column<36;++column)
                        line.append(',').append(QByteArray::number(row+column));
                    sample.write(line+"\n");
                }
                sample.flush();
                sample.close();
                analysis->loadFile(sample.fileName());
                QCoreApplication::processEvents();
                QCoreApplication::processEvents();
            }

            QScrollArea *channels=analysis->findChild<QScrollArea*>(
                QStringLiteral("analysisChannelsScroll"));
            QScrollArea *plots=analysis->findChild<QScrollArea*>(
                QStringLiteral("analysisStackScroll"));
            QPushButton *overlay=analysis->findChild<QPushButton*>(
                QStringLiteral("analysisOverlayButton"));
            ok=check(channels && plots && overlay,
                     "Analysis scroll areas or footer controls missing") && ok;
            if(channels && plots && overlay){
                QScrollBar *channelBar=channels->verticalScrollBar();
                QScrollBar *plotBar=plots->verticalScrollBar();
                const QList<QCheckBox*> channelBoxes=analysis->findChildren<QCheckBox*>();
                ok=check(channelBoxes.count()>=36,
                         "Loaded CSV channels were not created") && ok;
                for(int i=0;i<qMin(9,channelBoxes.count());++i)
                    channelBoxes.at(i)->setChecked(true);
                QCoreApplication::processEvents();
                QCoreApplication::processEvents();

                // The vertical scrollbars are mandatory, independent of
                // whether a particular monitor has space for every entry.
                // Qt must calculate the scroll range from ACTUAL content
                // height relative to the resized viewport, not a fixed pixel
                // threshold tied to one screen resolution.
                const QSize originalSize=window.size();
                for(const QSize size : {QSize(1200,680),QSize(1366,768),
                                        QSize(1920,1080)}){
                    window.resize(size);
                    QCoreApplication::processEvents();
                    QCoreApplication::processEvents();

                    ok=check(channels->isVisibleTo(&window) &&
                             plots->isVisibleTo(&window),
                             "Real Analysis scroll areas hidden at window size") && ok;
                    const int footerY=overlay->mapTo(&window,QPoint(0,0)).y();
                    for(QScrollArea *area:{channels,plots}){
                        QScrollBar *bar=area->verticalScrollBar();
                        const bool visible=bar && bar->isVisibleTo(&window);
                        const bool orange=bar && bar->styleSheet().contains(
                            QStringLiteral("#ff8a1c"));
                        const bool mandatory=area->verticalScrollBarPolicy()
                            ==Qt::ScrollBarAlwaysOn;
                        const int contentH=area->widget()?area->widget()->height():-1;
                        const int viewportH=area->viewport()?area->viewport()->height():-1;
                        const int range=bar?bar->maximum():-1;
                        logSmoke(QStringLiteral(
                            "Analysis size=%1x%2 area=%3 visible=%4 "
                            "barWidth=%5 mandatory=%6 contentHeight=%7 "
                            "viewportHeight=%8 max=%9 orange=%10")
                            .arg(size.width()).arg(size.height())
                            .arg(area==channels?QStringLiteral("channels"):
                                                QStringLiteral("plots"))
                            .arg(visible).arg(bar?bar->width():-1)
                            .arg(mandatory).arg(contentH)
                            .arg(viewportH).arg(range).arg(orange));
                        ok=check(mandatory && visible &&
                                 bar->width()>=15,
                                 "Mandatory Analysis vertical scrollbar not visible") && ok;
                        if(contentH>viewportH+1){
                            ok=check(range>0,
                                "Analysis content exceeds viewport but cannot scroll") && ok;
                        }
                        if(bar && range>0){
                            bar->setValue(range);
                            QCoreApplication::processEvents();
                            ok=check(bar->value()==range,
                                "Bottom Analysis data not reachable by scrollbar") && ok;
                            bar->setValue(0);
                        }
                    }
                    ok=check(overlay->mapTo(&window,QPoint(0,0)).y()==footerY,
                        "Analysis footer moved during scrollbar interaction") && ok;
                }
                window.resize(originalSize);
                QCoreApplication::processEvents();

                // Verify the SAME installed Analysis page toggles only its
                // left display. Channel selection remains on the right.
                QPushButton *viewMode=analysis->findChild<QPushButton*>(
                    QStringLiteral("analysisViewModeButton"));
                QTableView *dataTable=analysis->findChild<QTableView*>(
                    QStringLiteral("analysisValuesTable"));
                QWidget *rightPanel=analysis->findChild<QWidget*>(
                    QStringLiteral("analysisRightPanel"));
                ok=check(viewMode && dataTable && rightPanel,
                         "Analysis left curves/columns selector is missing") && ok;
                if(viewMode && dataTable && rightPanel){
                    int selected=0;
                    for(QCheckBox *cb:channelBoxes){
                        if(cb->isChecked())++selected;
                        ok=check(!cb->text().trimmed().isEmpty() &&
                                 cb->toolTip().contains(cb->text()),
                            "Analysis right-side channel name or full-name tooltip missing") && ok;
                    }
                    viewMode->setChecked(true);
                    QCoreApplication::processEvents();
                    ok=check(dataTable->isVisibleTo(&window) &&
                             !plots->isVisibleTo(&window) &&
                             rightPanel->isVisibleTo(&window),
                             "Analysis columns mode must replace only left-hand plots") && ok;
                    ok=check(dataTable->model() &&
                             dataTable->model()->rowCount()==40 &&
                             dataTable->model()->columnCount()==selected+1,
                             "CSV values table should show time and selected channels") && ok;
                    if(dataTable->model() && selected>0){
                        ok=check(dataTable->model()->headerData(
                              1,Qt::Horizontal).toString()==
                              channelBoxes.at(
                                  [&channelBoxes](){
                                      for(int i=0;i<channelBoxes.size();++i)
                                          if(channelBoxes.at(i)->isChecked())return i;
                                      return 0;
                                  }())->text(),
                            "Selected CSV channel name missing in table header") && ok;
                    }
                    for(const QSize size : {QSize(1200,680),QSize(1366,768),
                                            QSize(1920,1080)}){
                        window.resize(size);
                        QCoreApplication::processEvents();
                        ok=check(dataTable->isVisibleTo(&window) &&
                                 rightPanel->isVisibleTo(&window),
                                 "Columns table or right panel hidden after resize") && ok;
                    }
                    window.resize(originalSize);
                    viewMode->setChecked(false);
                    QCoreApplication::processEvents();
                    ok=check(plots->isVisibleTo(&window) &&
                             !dataTable->isVisibleTo(&window) &&
                             rightPanel->isVisibleTo(&window),
                             "Curve view not restored without changing right panel") && ok;

                    // Conventional CSV: header on FIRST line, UTF-8 BOM,
                    // quoted names, semicolon fields, French decimal comma.
                    // The old reader blindly dropped line one and accepted
                    // only comma delimiters, so user CSV channel names vanished.
                    QTemporaryFile frenchCsv(QDir::tempPath()+
                         QStringLiteral("/session-standard-csv-XXXXXX.csv"));
                    const bool csvReady=frenchCsv.open();
                    ok=check(csvReady,"Standard CSV test file unavailable") && ok;
                    if(csvReady){
                        frenchCsv.write(QByteArray::fromHex("efbbbf"));
                        frenchCsv.write("Heure;\"Regime moteur\";\"Temperature eau\"\n");
                        frenchCsv.write("12:00:00;1250;82,5\n");
                        frenchCsv.write("12:00:01;1350;83,0\n");
                        frenchCsv.flush();
                        frenchCsv.close();
                        analysis->loadFile(frenchCsv.fileName());
                        QCoreApplication::processEvents();
                        const QList<QCheckBox*> csvBoxes=
                            analysis->findChildren<QCheckBox*>();
                        ok=check(csvBoxes.size()==2 &&
                                 csvBoxes.at(0)->text()==QStringLiteral("Regime moteur") &&
                                 csvBoxes.at(1)->text()==QStringLiteral("Temperature eau") &&
                                 csvBoxes.at(0)->toolTip().contains(
                                    QStringLiteral("Regime moteur")),
                                 "Semicolon CSV must display both complete right-side names") && ok;
                        for(QCheckBox *cb:csvBoxes) cb->setChecked(true);
                        viewMode->setChecked(true);
                        QCoreApplication::processEvents();
                        if(dataTable->model()){
                            ok=check(dataTable->model()->columnCount()==3 &&
                                     dataTable->model()->rowCount()==2 &&
                                     dataTable->model()->headerData(
                                          1,Qt::Horizontal).toString()==QStringLiteral("Regime moteur") &&
                                     dataTable->model()->data(
                                          dataTable->model()->index(0,0)).toString()==QStringLiteral("12:00:00") &&
                                     dataTable->model()->data(
                                          dataTable->model()->index(0,2)).toString()==QStringLiteral("82,5"),
                                     "Standard CSV headings, time and exact decimal values not displayed") && ok;
                        }
                        ok=check(rightPanel->isVisibleTo(&window),
                                 "Right-side channel selection changed in columns mode") && ok;
                        viewMode->setChecked(false);
                        QCoreApplication::processEvents();
                        ok=check(plots->isVisibleTo(&window),
                                 "Original curves lost after reading a standard CSV") && ok;
                    }
                }
            }
        }
    }

    if (ok) qInfo().noquote() << "SESSION_UI_SMOKE_PASS 15 native pages, bidirectional navigation, "
                               "five window sizes, light/dark theme switching, Help and Options, "
                               "approved visual style, all six interface languages, MEMS/Settings/ROSCO, dwell 0-5ms";
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
