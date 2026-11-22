#include "uishellsmoketest.h"

#include "mainwindow.h"
#include "helpviewer.h"
#include "optionsdialog.h"
#include "styles.h"
#include "applicationvisualstyle.h"
#include "summarytab.h"
#include "analysistab.h"
#include "tooltipbubblemanager.h"
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
#include <QGroupBox>
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
#include <QTextEdit>
#include <QPlainTextEdit>
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
        QGroupBox *notesBox=interactive->findChild<QGroupBox*>(
            QStringLiteral("interactiveNotesBox"));
        QPlainTextEdit *notesEditor=interactive->findChild<QPlainTextEdit*>(
            QStringLiteral("interactiveNotesEditor"));
        ok=check(notesBox && notesEditor &&
                 notesBox->parentWidget()==interactive &&
                 notesEditor->parentWidget()==notesBox &&
                 !notesEditor->isReadOnly() &&
                 notesBox->title()==I18n::text(18120),
                 "Interactive notepad must be editable, translated and owned by Mode interactif") && ok;
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
    ok=check(tabs && tabs->count()==16,"Official 16 pages are not present") && ok;
    ok=check(navigation && navigation->count()==16,"Sidebar does not contain the 16 official entries") && ok;
    if (!tabs || !navigation || tabs->count()!=16 || navigation->count()!=16)
        return false;

    // The ECU / ROSCO owner is restored byte-for-byte to SESSION 194.
    // In particular its single hexadecimal output must remain read-only,
    // and the misplaced SESSION 195 notepad must no longer be here.
    QWidget *roscoPage=actualPage(tabs->widget(9));
    QTextEdit *roscoOutput=roscoPage?roscoPage->findChild<QTextEdit*>():nullptr;
    ok=check(roscoPage && roscoOutput && roscoOutput->isReadOnly() &&
             !roscoPage->findChild<QGroupBox*>(QStringLiteral("ecuRoscoNotesBox")),
             "ECU ROSCO must retain the read-only SESSION 194 session panel") && ok;

    QWidget *otherPage=window.findChild<QWidget*>(QStringLiteral("other_ecus_tab"));
    QFrame *rc5Panel=window.findChild<QFrame*>(QStringLiteral("otherEcusAirbagRc5"));
    QFrame *immobiliserPanel=window.findChild<QFrame*>(QStringLiteral("otherEcusImmobiliser"));
    QFrame *keysPanel=window.findChild<QFrame*>(QStringLiteral("otherEcusKeys"));
    ok=check(otherPage && rc5Panel && immobiliserPanel && keysPanel,
             "Other ECUs must have three independent columns") && ok;
    if(otherPage){
        // PLIP monitoring shares the 5AS worker, with no second ECU connection.
        const QList<QPushButton*> actions=otherPage->findChildren<QPushButton*>();
        ok=check(actions.size()==19,
                 "Four RC5, eight Lucas 5AS, four PLIP and three cable-help buttons required") && ok;
        const char *const ids[]={
            "rc5ConnectButton","rc5DisconnectButton",
            "rc5ReadFaultsButton","rc5ClearFaultsButton"
        };
        for(int i=0;i<4;++i)
            ok=check(otherPage->findChild<QPushButton*>(QString::fromLatin1(ids[i]))
                     !=nullptr,"Required RC5 action missing") && ok;
        QPushButton *connectAction=otherPage->findChild<QPushButton*>(
            QStringLiteral("rc5ConnectButton"));
        QPushButton *disconnectAction=otherPage->findChild<QPushButton*>(
            QStringLiteral("rc5DisconnectButton"));
        QPushButton *readAction=otherPage->findChild<QPushButton*>(
            QStringLiteral("rc5ReadFaultsButton"));
        QPushButton *clearAction=otherPage->findChild<QPushButton*>(
            QStringLiteral("rc5ClearFaultsButton"));
        ok=check(connectAction && connectAction->isEnabled() &&
                 disconnectAction && !disconnectAction->isEnabled() &&
                 readAction && !readAction->isEnabled() &&
                 clearAction && !clearAction->isEnabled(),
                 "RC5 must start disconnected with no automatic ECU command") && ok;
        QTableWidget *faultGrid=otherPage->findChild<QTableWidget*>(
            QStringLiteral("rc5FaultsTable"));
        ok=check(faultGrid && faultGrid->columnCount()==3 &&
                 faultGrid->rowCount()==0,
                 "RC5 DTC table must start empty with three columns") && ok;
        for(QFrame *panel:{rc5Panel,immobiliserPanel,keysPanel}){
            if(panel)
                ok=check(panel->parentWidget()==otherPage &&
                         panel->sizePolicy().horizontalPolicy()==QSizePolicy::Expanding,
                         "Module must occupy a flexible column") && ok;
        }
        const char *const helpIds[]={
            "rc5CableInfoButton","immobiliserCableInfoButton","keysCableInfoButton"
        };
        const QFrame *const helpPanels[]={rc5Panel,immobiliserPanel,keysPanel};
        for(int i=0;i<3;++i){
            const QFrame *panel=helpPanels[i];
            QPushButton *help=otherPage->findChild<QPushButton*>(
                QString::fromLatin1(helpIds[i]));
            ok=check(help && help->isEnabled() && panel &&
                     help->parentWidget()==panel &&
                     panel->layout() &&
                     panel->layout()->itemAt(panel->layout()->count()-1)->widget()==help,
                     "Cable information button must be active at bottom of its own module") && ok;
        }
        if(keysPanel){
            QPushButton *plipControl=otherPage->findChild<QPushButton*>(
                QStringLiteral("lucas5asPlipControlButton"));
            QPushButton *plipReset=otherPage->findChild<QPushButton*>(
                QStringLiteral("lucas5asPlipResetButton"));
            QTableWidget *plipTable=otherPage->findChild<QTableWidget*>(
                QStringLiteral("lucas5asPlipTable"));
            QPushButton *learnStart=otherPage->findChild<QPushButton*>(
                QStringLiteral("lucas5asPlipLearnStartButton"));
            QPushButton *learnStop=otherPage->findChild<QPushButton*>(
                QStringLiteral("lucas5asPlipLearnStopButton"));
            QLabel *learningStatus=otherPage->findChild<QLabel*>(
                QStringLiteral("lucas5asPlipLearningStatus"));
            ok=check(keysPanel->findChildren<QPushButton*>().size()==5 &&
                     learnStart && !learnStart->isEnabled() &&
                     learnStop && !learnStop->isEnabled() &&
                     learningStatus && learningStatus->text()==I18n::text(16962) &&
                     plipControl && plipControl->isEnabled() &&
                     plipReset && plipReset->isEnabled() &&
                     plipTable && plipTable->rowCount()==4 &&
                     plipTable->columnCount()==3 &&
                     plipTable->verticalScrollBarPolicy()==Qt::ScrollBarAlwaysOn,
                     "PLIP monitor starts idle with four raw slots and no ECU command") && ok;
        }
        QPushButton *fiveConnect=otherPage->findChild<QPushButton*>(
            QStringLiteral("lucas5asConnectButton"));
        QPushButton *fiveDisconnect=otherPage->findChild<QPushButton*>(
            QStringLiteral("lucas5asDisconnectButton"));
        QPushButton *fiveRead=otherPage->findChild<QPushButton*>(
            QStringLiteral("lucas5asReadStatusButton"));
        QTableWidget *fiveTable=otherPage->findChild<QTableWidget*>(
            QStringLiteral("lucas5asStatusTable"));
        QPlainTextEdit *fiveLog=otherPage->findChild<QPlainTextEdit*>(
            QStringLiteral("lucas5asDiagnosticLog"));
        ok=check(fiveConnect && fiveConnect->isEnabled() &&
                 fiveDisconnect && !fiveDisconnect->isEnabled() &&
                 fiveRead && !fiveRead->isEnabled() &&
                 fiveTable && fiveTable->columnCount()==2 &&
                 fiveTable->rowCount()==0 &&
                 fiveTable->verticalScrollBarPolicy()==Qt::ScrollBarAlwaysOn &&
                 fiveLog && fiveLog->isReadOnly(),
                 "Lucas 5AS starts disconnected, no automatic command, with a read-only log") && ok;
        if(immobiliserPanel)
            ok=check(immobiliserPanel->findChildren<QPushButton*>().size()==9,
                     "Lucas 5AS needs eight diagnostic buttons and its cable help") && ok;
        const char *const extraIds[]={
            "lucas5asLiveStatusButton","lucas5asTestHornButton",
            "lucas5asTestLedButton","lucas5asTestHazardsButton",
            "lucas5asSaveLogButton"
        };
        for(const char *id:extraIds){
            QPushButton *button=otherPage->findChild<QPushButton*>(
                QString::fromLatin1(id));
            ok=check(button && !button->isEnabled(),
                     "Live/test/save commands must start disabled without ECU") && ok;
        }
        QTimer *liveTimer=otherPage->findChild<QTimer*>(
            QStringLiteral("lucas5asLiveTimer"));
        ok=check(liveTimer && !liveTimer->isActive() &&
                 liveTimer->interval()==1000,
                 "Lucas 5AS live monitoring must begin stopped") && ok;

        // Exercise the actual offline help in each module, not only its
        // translation string or existence. Auto-close the modal from the
        // local event loop; never open a COM port or talk to an ECU.
        for(int i=0;i<3;++i){
            QPushButton *help=otherPage->findChild<QPushButton*>(
                QString::fromLatin1(helpIds[i]));
            if(!help)continue;
            bool opened=false;
            QTimer::singleShot(0,&window,[&window,&opened,i](){
                QDialog *dialog=window.findChild<QDialog*>(
                    QStringLiteral("otherEcusCableHelpDialog"));
                QTextBrowser *text=dialog
                    ? dialog->findChild<QTextBrowser*>(
                        QStringLiteral("otherEcusCableHelpContents")) : nullptr;
                opened=dialog && dialog->isVisible() && text &&
                    text->isReadOnly() &&
                    dialog->windowTitle()==I18n::text(16891+2*i) &&
                    text->toPlainText().size()>150 &&
                    text->verticalScrollBar()!=nullptr;
                if(dialog)dialog->accept();
                else if(QWidget *modal=qApp->activeModalWidget())modal->close();
            });
            help->click();
            ok=check(opened,
                     "Cable help must open a scrollable translated offline dialog") && ok;
        }
    }

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
    QAction *manualUpdateAction=window.findChild<QAction*>(
        QStringLiteral("programUpdateCheckAction"));
    ok=check(manualUpdateAction && !manualUpdateAction->icon().isNull(),
             "Manual update menu action must have a SESSION-orange icon") && ok;
    if(manualUpdateAction && !manualUpdateAction->icon().isNull()){
        const QImage icon=manualUpdateAction->icon().pixmap(24,24).toImage()
            .convertToFormat(QImage::Format_ARGB32);
        int orange=0;
        for(int y=0;y<icon.height();++y)
            for(int x=0;x<icon.width();++x){
                const QColor pixel=icon.pixelColor(x,y);
                if(pixel.alpha()>160 && pixel.red()>220 &&
                   pixel.green()>90 && pixel.green()<190 &&
                   pixel.blue()<100) ++orange;
            }
        ok=check(orange>24,
                 "Manual program update icon must use the orange SESSION outline style") && ok;
    }
    ok=check(manual.minimumWidth()>=850 &&
             manual.styleSheet().contains(
                qApp->property("ecuDarkTheme").toBool()
                    ?QStringLiteral("#090e13"):QStringLiteral("#ffffff")),
             "Help dialog must be larger and follow the selected theme") && ok;

    // The actual Help-menu actions must use the native SESSION orange
    // outline icons in both themes, not blue/grey platform QStyle art.
    QAction *contentsAction=window.findChild<QAction*>(
        QStringLiteral("m_helpContentsAction"));
    QAction *aboutAction=window.findChild<QAction*>(
        QStringLiteral("m_helpAboutAction"));
    ok=check(contentsAction && aboutAction &&
             !contentsAction->icon().isNull() && !aboutAction->icon().isNull(),
             "Help and About actions must retain their actual menu icons") && ok;
    if(contentsAction && aboutAction &&
       !contentsAction->icon().isNull() && !aboutAction->icon().isNull()){
        const QImage helpMenu=contentsAction->icon().pixmap(24,24).toImage().convertToFormat(QImage::Format_ARGB32);
        const QImage aboutMenu=aboutAction->icon().pixmap(24,24).toImage().convertToFormat(QImage::Format_ARGB32);
        auto orangePixels=[](const QImage &icon){
            int orange=0;
            for(int y=0;y<icon.height();++y)
                for(int x=0;x<icon.width();++x){
                    const QColor pixel=icon.pixelColor(x,y);
                    if(pixel.alpha()>160 && pixel.red()>220 &&
                       pixel.green()>90 && pixel.green()<190 &&
                       pixel.blue()<100) ++orange;
                }
            return orange;
        };
        int changed=0;
        if(helpMenu.size()==aboutMenu.size()){
            for(int y=0;y<helpMenu.height();++y)
                for(int x=0;x<helpMenu.width();++x)
                    if(helpMenu.pixel(x,y)!=aboutMenu.pixel(x,y))++changed;
        }
        ok=check(orangePixels(helpMenu)>24 && orangePixels(aboutMenu)>24 &&
                 changed>5,
                 "Help / About menu icons must be distinct orange outline drawings") && ok;
    }

    // Both Options commands must have distinct SESSION-orange pictograms.
    // This checks their actual QAction icons without triggering or modifying
    // the existing parameter dialog and Always-on-top toggle.
    QAction *settingsMenuAction=window.findChild<QAction*>(
        QStringLiteral("m_editSettingsAction"));
    QAction *alwaysOnTopMenuAction=window.findChild<QAction*>(
        QStringLiteral("sessionAlwaysOnTopAction"));
    ok=check(settingsMenuAction && alwaysOnTopMenuAction &&
             !settingsMenuAction->icon().isNull() &&
             !alwaysOnTopMenuAction->icon().isNull() &&
             alwaysOnTopMenuAction->isCheckable(),
             "Options settings and Always-on-top must have orange icons with original toggle") && ok;
    if(settingsMenuAction && alwaysOnTopMenuAction &&
       !settingsMenuAction->icon().isNull() && !alwaysOnTopMenuAction->icon().isNull()){
        const QImage settingsIcon=settingsMenuAction->icon().pixmap(24,24).toImage()
            .convertToFormat(QImage::Format_ARGB32);
        const QImage pinIcon=alwaysOnTopMenuAction->icon().pixmap(24,24).toImage()
            .convertToFormat(QImage::Format_ARGB32);
        auto countAccent=[](const QImage &picture){
            int count=0;
            for(int y=0;y<picture.height();++y)
                for(int x=0;x<picture.width();++x){
                    const QColor pixel=picture.pixelColor(x,y);
                    if(pixel.alpha()>160 && pixel.red()>220 &&
                       pixel.green()>90 && pixel.green()<190 &&
                       pixel.blue()<100) ++count;
                }
            return count;
        };
        int different=0;
        if(settingsIcon.size()==pinIcon.size()){
            for(int y=0;y<settingsIcon.height();++y)
                for(int x=0;x<settingsIcon.width();++x)
                    if(settingsIcon.pixel(x,y)!=pinIcon.pixel(x,y))++different;
        }
        ok=check(countAccent(settingsIcon)>24 && countAccent(pinIcon)>24 &&
                 different>5,
                 "Options settings and pin icons must be distinct orange drawings") && ok;
    }

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
    // Light theme help icons must be ORANGE / WHITE, never black.
    // Dark mode still uses its original icon resource.
    const QPixmap lightHelp=TooltipBubbleManager::helpBubblePixmap(false);
    const QPixmap darkHelp=TooltipBubbleManager::helpBubblePixmap(true);
    int orange=0,white=0,black=0;
    const QImage helpPixels=lightHelp.toImage().convertToFormat(
        QImage::Format_ARGB32);
    for(int y=0;y<helpPixels.height();++y)
        for(int x=0;x<helpPixels.width();++x){
            const QColor pixel=helpPixels.pixelColor(x,y);
            if(pixel.alpha()<180)continue;
            if(pixel.red()>220 && pixel.green()>85 && pixel.green()<190 &&
               pixel.blue()<90)++orange;
            if(pixel.red()>235 && pixel.green()>235 && pixel.blue()>235)++white;
            if(pixel.red()<65 && pixel.green()<65 && pixel.blue()<65)++black;
        }
    logSmoke(QStringLiteral("Help icon pixels: orange=%1 white=%2 black=%3")
             .arg(orange).arg(white).arg(black));
    ok=check(!lightHelp.isNull() && !darkHelp.isNull() &&
             orange>40 && white>10 && black==0,
             "Light Help bubble must be orange with white text, not black") && ok;

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
    ok=check(startupRowHeight>=25 && startupRowHeight<=35,
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
    for (int i=0;i<16;i++){
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
    for (int i=15;i>=0;i--){
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
        ok=check(tabs->count()==16 && navigation->count()==16,
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
        tabs->setCurrentIndex(15);
        QCoreApplication::processEvents();
        if(otherPage && rc5Panel && immobiliserPanel && keysPanel){
            const int a=rc5Panel->width(),b=immobiliserPanel->width(),d=keysPanel->width();
            logSmoke(QStringLiteral("Other ECU 3 columns at %1px: %2 %3 %4")
                .arg(size.width()).arg(a).arg(b).arg(d));
            ok=check(qAbs(a-b)<=2 && qAbs(b-d)<=2 && a>=150 &&
                     rc5Panel->isVisibleTo(&window) &&
                     immobiliserPanel->isVisibleTo(&window) &&
                     keysPanel->isVisibleTo(&window),
                     "Three ECU columns must have equal widths and be visible") && ok;
            if(size.width()>=1366){
                QPushButton *rc5=otherPage->findChild<QPushButton*>(QStringLiteral("rc5ConnectButton"));
                QPushButton *five=otherPage->findChild<QPushButton*>(QStringLiteral("lucas5asConnectButton"));
                QPushButton *plip=otherPage->findChild<QPushButton*>(QStringLiteral("lucas5asPlipControlButton"));
                QPushButton *reset=otherPage->findChild<QPushButton*>(QStringLiteral("lucas5asPlipResetButton"));
                const int rc5Y=rc5?rc5->mapTo(otherPage,QPoint(0,0)).y():-1000;
                const int fiveY=five?five->mapTo(otherPage,QPoint(0,0)).y():-2000;
                const int plipY=plip?plip->mapTo(otherPage,QPoint(0,0)).y():-3000;
                const int resetY=reset?reset->mapTo(otherPage,QPoint(0,0)).y():-4000;
                logSmoke(QStringLiteral("Other ECU first controls at %1px: RC5=%2 5AS=%3 PLIP=%4 reset=%5")
                    .arg(size.width()).arg(rc5Y).arg(fiveY).arg(plipY).arg(resetY));
                ok=check(rc5 && five && plip && reset &&
                         qAbs(rc5Y-fiveY)<=1 && qAbs(fiveY-plipY)<=1 && plipY==resetY,
                         "Top six ECU buttons must share exactly the same horizontal row") && ok;
            }
            QTableWidget *plipTable=otherPage->findChild<QTableWidget*>(QStringLiteral("lucas5asPlipTable"));
            QHeaderView *plipHeader=plipTable?plipTable->horizontalHeader():nullptr;
            const int firstHeaderNeeded=plipHeader
                ? plipHeader->fontMetrics().horizontalAdvance(I18n::text(16950))+6 : -1;
            ok=check(plipHeader &&
                     plipHeader->sectionResizeMode(0)==QHeaderView::ResizeToContents &&
                     plipHeader->sectionResizeMode(1)==QHeaderView::Stretch &&
                     plipHeader->sectionResizeMode(2)==QHeaderView::ResizeToContents &&
                     plipTable->columnWidth(0)>=firstHeaderNeeded,
                     "PLIP Emplacement heading must be fully visible; byte column uses remaining width") && ok;
        }
        tabs->setCurrentIndex(previousTab);
        QCoreApplication::processEvents();
    }

    // SESSION 157 let the dark-only rebuild override the saved "Clair"
    // choice at startup. Test a real two-way theme switch on the native
    // 16-page shell, with the existing geometry and selection intact.
    const bool initialDark=qApp->property("ecuDarkTheme").toBool();
    const int originalTab=tabs->currentIndex();
    const int originalWidth=navigation->width();
    const int originalRow=navigation->item(0)->sizeHint().height();
    for(const bool dark:{false,true,false}){
        qApp->setProperty("ecuDarkTheme",dark);
        qApp->setStyleSheet(dark?STYLE_DARK:STYLE_LIGHT);
        applyApplicationVisualStyle(&window);
        QCoreApplication::processEvents();
        if(rc5Panel && immobiliserPanel && keysPanel){
            const QString expected=dark?QStringLiteral("#10161c"):QStringLiteral("#ffffff");
            ok=check(rc5Panel->styleSheet().contains(expected) &&
                     immobiliserPanel->styleSheet().contains(expected) &&
                     keysPanel->styleSheet().contains(expected),
                     "Three ECU columns must match light/dark theme") && ok;
        }
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
        ok=check(tabs->count()==16 && navigation->count()==16 &&
                 navigation->width()==originalWidth &&
                 navigation->item(0)->sizeHint().height()==originalRow &&
                 tabs->currentIndex()==originalTab &&
                 navigation->currentRow()==originalTab &&
                 navigation->selectedItems().size()==1 &&
                 navigation->selectedItems().first()==
                     navigation->item(originalTab),
                 "Theme changes must not disturb 16 pages, sidebar geometry or selection") && ok;

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

    // Native six-language visual regression: all 16 page titles follow the
    // selected SESSION language, not the Windows system locale.
    const int officialTitleKeys[16] = {
        1001,7900,2001,4001,3001,7013,7940,7018,7017,7012,
        5001,7152,6002,19000,16200,16800
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
        ok=check(tabs->count()==16 && navigation->count()==16,
                 "Language changed the official 16-page count") && ok;
        for (int i=0;i<16;i++) {
            ok=check(navigation->item(i)->text()==tabs->tabText(i),
                     "Sidebar and page titles disagree in selected language") && ok;
            ok=check(tabs->tabText(i)==I18n::text(officialTitleKeys[i]),
                     "Page title does not match selected SESSION language") && ok;
        }
        QWidget *adjust = window.findChild<QWidget*>(QStringLiteral("settingsAdjust"));
        QPushButton *save = adjust ? adjust->findChild<QPushButton*>(
            QStringLiteral("m_saveAdjustmentsButton")) : nullptr;
        ok=check(save && save->isVisibleTo(adjust) && !save->isEnabled() &&
                 save->text()==I18n::text(7080) && !save->text().startsWith(QLatin1Char('[')) &&
                 !I18n::text(7081).startsWith(QLatin1Char('[')),
                 "ECU settings save button must be translated and disabled offline") && ok;
        const char *const headings[3]={
            "otherEcusAirbagTitle","otherEcusImmobiliserTitle","otherEcusKeysTitle"
        };
        for(int i=0;i<3;++i){
            QLabel *heading=window.findChild<QLabel*>(QString::fromLatin1(headings[i]));
            ok=check(heading && heading->text()==I18n::text(16801+i),
                     "Other ECU column title is not translated") && ok;
        }
        const char *const actionNames[]={
            "rc5ConnectButton","rc5DisconnectButton",
            "rc5ReadFaultsButton","rc5ClearFaultsButton"
        };
        for(int i=0;i<4;++i){
            QPushButton *button=window.findChild<QPushButton*>(
                QString::fromLatin1(actionNames[i]));
            ok=check(button && button->text()==I18n::text(16805+i),
                     "RC5 button must follow the six selected languages") && ok;
        }
        const char *const cableIds[]={
            "rc5CableInfoButton","immobiliserCableInfoButton",
            "keysCableInfoButton"
        };
        QPushButton *fiveConnect=window.findChild<QPushButton*>(
            QStringLiteral("lucas5asConnectButton"));
        QPushButton *fiveDisconnect=window.findChild<QPushButton*>(
            QStringLiteral("lucas5asDisconnectButton"));
        QPushButton *fiveRead=window.findChild<QPushButton*>(
            QStringLiteral("lucas5asReadStatusButton"));
        ok=check(fiveConnect && fiveConnect->text()==I18n::text(16805) &&
                 fiveDisconnect && fiveDisconnect->text()==I18n::text(16806) &&
                 fiveRead && fiveRead->text()==I18n::text(16906),
                 "Lucas 5AS diagnostic buttons must follow six languages") && ok;
        const char *const extraIds[]={
            "lucas5asLiveStatusButton","lucas5asTestHornButton",
            "lucas5asTestLedButton","lucas5asTestHazardsButton",
            "lucas5asSaveLogButton"
        };
        const int extraKeys[]={16940,16924,16925,16926,16935};
        for(int i=0;i<5;++i){
            QPushButton *button=window.findChild<QPushButton*>(
                QString::fromLatin1(extraIds[i]));
            ok=check(button && button->text()==I18n::text(extraKeys[i]),
                     "Lucas 5AS extra actions must follow six languages") && ok;
        }
        QPushButton *plipControl=window.findChild<QPushButton*>(
            QStringLiteral("lucas5asPlipControlButton"));
        QPushButton *plipReset=window.findChild<QPushButton*>(
            QStringLiteral("lucas5asPlipResetButton"));
        QTableWidget *plipTable=window.findChild<QTableWidget*>(
            QStringLiteral("lucas5asPlipTable"));
        QLabel *plipWarning=window.findChild<QLabel*>(
            QStringLiteral("lucas5asPlipWarning"));
        QPushButton *learnStart=window.findChild<QPushButton*>(
            QStringLiteral("lucas5asPlipLearnStartButton"));
        QPushButton *learnStop=window.findChild<QPushButton*>(
            QStringLiteral("lucas5asPlipLearnStopButton"));
        ok=check(learnStart && learnStart->text()==I18n::text(16960) &&
                 learnStop && learnStop->text()==I18n::text(16961) &&
                 plipControl && plipControl->text()==I18n::text(16943) &&
                 plipReset && plipReset->text()==I18n::text(16945) &&
                 plipWarning && plipWarning->text()==I18n::text(16954) &&
                 plipTable && plipTable->horizontalHeaderItem(0) &&
                 plipTable->horizontalHeaderItem(0)->text()==I18n::text(16950) &&
                 plipTable->horizontalHeaderItem(1) &&
                 plipTable->horizontalHeaderItem(1)->text()==I18n::text(16951) &&
                 plipTable->horizontalHeaderItem(2) &&
                 plipTable->horizontalHeaderItem(2)->text()==I18n::text(16952),
                 "PLIP interface and limitations translated in six languages") && ok;
        QTableWidget *fiveTable=window.findChild<QTableWidget*>(
            QStringLiteral("lucas5asStatusTable"));
        if(fiveTable)
            ok=check(fiveTable->horizontalHeaderItem(0) &&
                     fiveTable->horizontalHeaderItem(0)->text()==I18n::text(16920) &&
                     fiveTable->horizontalHeaderItem(1) &&
                     fiveTable->horizontalHeaderItem(1)->text()==I18n::text(16921),
                     "Lucas 5AS status columns must be translated") && ok;
        for(int i=0;i<3;++i){
            QPushButton *cable=window.findChild<QPushButton*>(
                QString::fromLatin1(cableIds[i]));
            ok=check(cable && cable->text()==I18n::text(16890),
                     "Cable help button must match the selected language") && ok;
            const QString help=I18n::text(16892+2*i);
            ok=check(!I18n::text(16891+2*i).isEmpty() &&
                     help.contains(QStringLiteral("<html>")) &&
                     help.contains(QStringLiteral("</html>")) &&
                     help.size()>350,
                     "Offline cable documentation must be translated and substantive") && ok;
        }
        QTableWidget *rc5Table=window.findChild<QTableWidget*>(
            QStringLiteral("rc5FaultsTable"));
        if(rc5Table)
            for(int col=0;col<3;++col)
                ok=check(rc5Table->horizontalHeaderItem(col) &&
                         rc5Table->horizontalHeaderItem(col)->text()==
                            I18n::text(16820+col),
                         "RC5 DTC headings must follow selected language") && ok;
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
                            // The first scroll can itself settle a pending
                            // resizable-content layout. Use the CURRENT range,
                            // not the stale pre-layout maximum.
                            bar->setValue(range);
                            QCoreApplication::processEvents();
                            bar->setValue(bar->maximum());
                            QCoreApplication::processEvents();
                            ok=check(bar->value()==bar->maximum() &&
                                     bar->maximum()>0,
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
                QPushButton *curvesMode=analysis->findChild<QPushButton*>(
                    QStringLiteral("analysisCurvesModeButton"));
                QPushButton *loadFile=analysis->findChild<QPushButton*>(
                    QStringLiteral("analysisLoadButton"));
                QWidget *modeControls=analysis->findChild<QWidget*>(
                    QStringLiteral("analysisViewModeControls"));
                QWidget *topBar=analysis->findChild<QWidget*>(
                    QStringLiteral("analysisTopBar"));
                QWidget *fileToolbar=analysis->findChild<QWidget*>(
                    QStringLiteral("analysisFileToolbar"));
                ok=check(topBar && fileToolbar &&
                         fileToolbar->parentWidget()==topBar &&
                         modeControls && modeControls->parentWidget()==topBar,
                         "Native Analysis layout must keep load and modes in ONE top bar") && ok;
                QLabel *modeHelp=analysis->findChild<QLabel*>(
                    QStringLiteral("analysisViewModeHelp"));
                ok=check(curvesMode && loadFile && modeControls && modeHelp,
                         "Two centred Analysis display buttons and external help icon missing") && ok;
                if(curvesMode && loadFile && modeControls && modeHelp){
                    const auto midY=[&window](QWidget *w){
                        return w->mapTo(&window,QPoint(0,w->height()/2)).y();
                    };
                    logSmoke(QStringLiteral("Analysis toolbar centers: load=%1 curves=%2 columns=%3")
                        .arg(midY(loadFile)).arg(midY(curvesMode)).arg(midY(viewMode)));
                    ok=check(qAbs(midY(loadFile)-midY(curvesMode))<=4 &&
                             qAbs(midY(loadFile)-midY(viewMode))<=4,
                             "Analysis mode controls must share the load-file toolbar row") && ok;
                    const QPoint helpPos=modeHelp->mapTo(modeControls,QPoint(0,0));
                    const QPoint colPos=viewMode->mapTo(modeControls,
                                                      QPoint(viewMode->width(),0));
                    ok=check(helpPos.x()>=colPos.x() &&
                             !modeHelp->toolTip().isEmpty() &&
                             !modeHelp->pixmap()->isNull() &&
                             !viewMode->findChild<QLabel*>(
                                 QStringLiteral("_ecuHelpBubble"),
                                 Qt::FindDirectChildrenOnly) &&
                             !curvesMode->findChild<QLabel*>(
                                 QStringLiteral("_ecuHelpBubble"),
                                 Qt::FindDirectChildrenOnly) &&
                             !modeHelp->findChild<QLabel*>(
                                 QStringLiteral("_ecuHelpBubble"),
                                 Qt::FindDirectChildrenOnly),
                             "Analysis help bubble must be AFTER buttons, not over their text") && ok;
                    for(const QSize size : {QSize(1200,680),QSize(1366,768),
                                            QSize(1920,1080)}){
                        window.resize(size);
                        QCoreApplication::processEvents();
                        const int groupMiddle=modeControls->mapTo(
                            analysis,QPoint(modeControls->width()/2,0)).x();
                        logSmoke(QStringLiteral("Analysis toolbar center at %1px: group=%2 content=%3 deltaY=%4")
                            .arg(size.width()).arg(groupMiddle).arg(analysis->width())
                            .arg(midY(loadFile)-midY(curvesMode)));
                        ok=check(qAbs(groupMiddle-analysis->width()/2)<=100 &&
                                 qAbs(midY(loadFile)-midY(curvesMode))<=4 &&
                                 modeHelp->isVisibleTo(&window),
                                 "Analysis curve/column toolbar off-center or clipped") && ok;
                    }
                    window.resize(originalSize);
                    QCoreApplication::processEvents();
                }
                QTableView *dataTable=analysis->findChild<QTableView*>(
                    QStringLiteral("analysisValuesTable"));
                QWidget *rightPanel=analysis->findChild<QWidget*>(
                    QStringLiteral("analysisRightPanel"));
                ok=check(viewMode && curvesMode && dataTable && rightPanel,
                         "Analysis left curves/columns selector is missing") && ok;
                if(viewMode && curvesMode && dataTable && rightPanel){
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
                    curvesMode->setChecked(true);
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
                        curvesMode->setChecked(true);
                        QCoreApplication::processEvents();
                        ok=check(plots->isVisibleTo(&window),
                                 "Original curves lost after reading a standard CSV") && ok;
                    }
                }
            }
        }
    }

    if (ok) qInfo().noquote() << "SESSION_UI_SMOKE_PASS 16 native pages, bidirectional navigation, "
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
