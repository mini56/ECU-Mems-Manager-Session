#include "uishell.h"
#include "uishellsmoketest.h"

#include "mainwindow.h"
#include "helpviewer.h"
#include "optionsdialog.h"
#include "styles.h"
#include "applicationvisualstyle.h"
#include "sessioninformationmessage.h"
#include "summarytab.h"
#include "analysistab.h"
#include "tooltipbubblemanager.h"
#include "expert/IaMemsService.h"
#include "diagnosticpanel.h"
#include <QHelpEvent>
#include <QToolTip>
#include <QVBoxLayout>
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
#include <QLCDNumber>
#include <QLineEdit>
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

    // Regression SESSION 207: even before connecting an ECU, the two
    // data pages must contain the documented NA reference names and help.
    // No firmware or received value may be fabricated.
    struct NaLabel { const char *name; int key; };
    const NaLabel naLabels[]={
        {"label_80x0B",29022},{"label_80x0C",29023},{"label_80x0F",29024},
        {"label_80x10",29025},{"label_80x11",29026},{"label_80x09",29027},
        {"label_80x1B",29028},{"label_0x0B",29029},{"label_0x0F",29030},
        {"label_0x15",29031},{"label_0x1F",29033},
        {"label_80x1A",29079},{"label_0x03",29080},{"label_0x10",29081}};
    for(const auto &entry:naLabels) {
        auto *label=window.findChild<QLabel*>(QString::fromLatin1(entry.name));
        ok=check(label&&label->text()==I18n::text(entry.key)&&!label->toolTip().isEmpty(),
                 entry.name)&&ok;
    }
    const char *refNames[]={"label_0x17","label_0x19","label_0x1B","label_0x1D"};
    for(int i=0;i<4;++i){
        auto *label=window.findChild<QLabel*>(QString::fromLatin1(refNames[i]));
        ok=check(label&&label->text()==I18n::text(29032).arg(i+1)&&!label->toolTip().isEmpty(),
                 refNames[i])&&ok;
    }
    for(const char *name:{"label_80x0D","label_80x0E","label_0x05",
                         "label_0x0E","label_0x11","label_0x16","label_0x10"}){
        auto *label=window.findChild<QLabel*>(QString::fromLatin1(name));
        auto *bubble=label?label->findChild<QLabel*>(QStringLiteral("_ecuHelpBubble"),Qt::FindDirectChildrenOnly):nullptr;
        ok=check(label&&label->toolTip().contains(I18n::text(29074))&&bubble&&!bubble->isHidden()
            &&bubble->size()==QSize(18,18),name)&&ok;
    }
    auto *dtc1=window.findChild<QLabel*>(QStringLiteral("label_80x0E"));
    ok=check(dtc1&&dtc1->toolTip().contains(I18n::text(29012))&&
        dtc1->toolTip().contains(I18n::text(29013))&&dtc1->toolTip().contains(I18n::text(29015)),
        "DTC1 offline variant help")&&ok;
    for(const char *name:{"m_80_0D","m_80_0E","m_7D_05","m_7D_17","m_80_11","r_7D_05"}){
        auto *label=window.findChild<QLabel*>(QString::fromLatin1(name));
        ok=check(label&&label->text()==QStringLiteral("--"),name)&&ok;
    }
    for(const char *name:{"r_7D_0E","r_7D_10","r_7D_11","r_7D_16"}){
        auto *label=window.findChild<QLabel*>(QString::fromLatin1(name));
        ok=check(label&&label->text()==I18n::text(29082),name)&&ok;
    }
    if(ok)logSmoke(QStringLiteral("MEMS16NA_RAW_OFFLINE_HELP_AND_LABELS_PASS"));

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
    // its title, stepper commands grouped, gauge test, and compact live indicator.
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
    QWidget *gauge=window.findChild<QWidget*>(QStringLiteral("actuatorGaugeTestCard"));
    QWidget *move=window.findChild<QWidget*>(QStringLiteral("m_moveIACButton"));
    QLabel *reason=window.findChild<QLabel*>(QStringLiteral("gaugeTestReason"));
    ok=check(state && gauge && move && reason,"Actuator temperature test sections absent") && ok;
    if(state && gauge && move){
        QWidget *right=state->parentWidget();
        ok=check(state->minimumWidth()>=200 && state->minimumHeight()>=96 &&
                 state->maximumHeight()<=126 && state->maximumWidth()==QWIDGETSIZE_MAX &&
                 gauge->parentWidget()==right && move->parentWidget()==right &&
                 right->layout() && right->layout()->indexOf(gauge)>=0 &&
                 right->layout()->indexOf(gauge)<right->layout()->indexOf(state),
                 "Actuator compact state indicator must follow the gauge test") && ok;
    }
    ok=check(reason && gauge && reason->parentWidget()==gauge && reason->wordWrap() &&
             reason->text()==I18n::text(2971805),
             "Disconnected gauge test must display its translated explanation") && ok;
    for(const char *name:{"gaugeTestMinimum","gaugeTestMaximum","gaugeTestStop"}){
        auto *button=window.findChild<QPushButton*>(QString::fromLatin1(name));
        ok=check(button && gauge && button->parentWidget()==gauge && !button->isEnabled() &&
                 button->toolTip()==I18n::text(2971805),
                 "Disconnected gauge commands must be locked with an explanation") && ok;
    }

    auto *unit=window.findChild<QLabel*>(QStringLiteral("session217InjectionUnit"));
    ok=check(unit&&unit->text()==QStringLiteral("µs")&&unit->toolTip().isEmpty(),
             "Injection unit visible at rest; no second help bubble")&&ok;
    for(const char *name:{"e_FuelTrim_lcd","e_IdleDecay_lcd","e_IdleSpeed_lcd","e_IgnitionAdvance_lcd"}){
        auto *lcd=window.findChild<QLCDNumber*>(QString::fromLatin1(name));
        ok=check(lcd&&lcd->value()==0.0,"All four ECU adjustment displays start at zero")&&ok;
    }
    // Verify the real actuator stepper slider, not a decorative or duplicate
    // element. Preserve the original movement range and ECU signal wiring.
    QSlider *stepperSlider=window.findChild<QSlider*>(
        QStringLiteral("m_iacPositionSlider"));
    ok=check(stepperSlider &&
             stepperSlider->styleSheet().contains(QStringLiteral(
                 "QSlider::handle:horizontal{background:#ff8a1c")) &&
             stepperSlider->minimum()==0 && stepperSlider->maximum()==4,
             "Actuator stepper slider has the orange handle and original range") && ok;

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

    // Verify actual painted Qt tooltip borders, not only stylesheet strings.
    auto renderedTooltip=[&](bool dark){
        for(QWidget *tip:QApplication::topLevelWidgets()){
            if(tip->windowType()!=Qt::ToolTip||!tip->inherits("QTipLabel")||!tip->isVisible())continue;
            const QImage pixels=tip->grab().toImage();
            if(pixels.isNull())continue;
            int orange=0,background=0;
            for(int y=0;y<pixels.height();++y)for(int x=0;x<pixels.width();++x){
                const QColor c=pixels.pixelColor(x,y);
                if(c.red()>190&&c.green()>80&&c.green()<190&&c.blue()<95&&
                   (x<4||y<4||x>=pixels.width()-4||y>=pixels.height()-4))++orange;
                if(dark?(c.red()<40&&c.green()<40&&c.blue()<40):
                    (c.red()>235&&c.green()>235&&c.blue()>235))++background;
            }
            return orange>8&&background>pixels.width()&&tip->styleSheet()==helpTooltipStyle(dark);
        }
        return false;
    };
    auto verifyScrollbars=[&](QWidget *root,bool dark){
        bool valid=true;
        for(QScrollBar *bar:root->findChildren<QScrollBar*>()){
            if(!bar->isVisibleTo(root))continue;
            const int extent=bar->orientation()==Qt::Vertical?bar->width():bar->height();
            if(extent!=kSessionScrollBarExtent||bar->styleSheet()!=sessionScrollBarStyle(dark)){
                logSmoke(QStringLiteral("Scrollbar mismatch: %1 extent=%2").arg(bar->objectName()).arg(extent));
                valid=false;
            }
        }
        return valid;
    };
    if(SummaryTab *summary=window.findChild<SummaryTab*>()){
        auto visibleRows=[summary](){
            int rows=0;
            for(QTableWidget *table:summary->findChildren<QTableWidget*>())
                for(int r=0;r<table->rowCount();++r)if(!table->isRowHidden(r))++rows;
            return rows;
        };
        ok=check(visibleRows()==56,"All 56 classic measurement rows must be visible offline")&&ok;
        QVariantMap received;received.insert(QStringLiteral("battery_v"),12345);
        QVariantMap sample;sample.insert(QStringLiteral("_received"),received);
        sample.insert(QStringLiteral("battery_v"),12.345);
        sample.insert(QStringLiteral("DATA_00_RAW"),QStringLiteral("61 00 AA BB"));
        sample.insert(QStringLiteral("DATA_06_RAW"),QStringLiteral("61 06 01 02"));
        summary->update2JData(sample,QStringLiteral("MEMS 2J"));
        ok=check(visibleRows()==31,"Every acquired 2J field must have a row")&&ok;
        bool raw1=false,raw2=false,rawBattery=false;
        for(QTableWidget *table:summary->findChildren<QTableWidget*>())
            for(int r=0;r<table->rowCount();++r)if(!table->isRowHidden(r)){
                const QString raw=table->item(r,2)->text();
                raw1|=raw==QStringLiteral("61 00 AA BB");
                raw2|=raw==QStringLiteral("61 06 01 02");
                rawBattery|=raw==QStringLiteral("12345");
            }
        ok=check(raw1&&raw2&&rawBattery,"2J raw groups and original numeric value are preserved")&&ok;
        summary->update2JData(QVariantMap(),QStringLiteral("MEMS 3"));
        ok=check(visibleRows()==12,"Every acquired MEMS 3 field must have a row")&&ok;
        summary->showNaDocumentation();
        ok=check(visibleRows()==56,"All 56 rows restored after family change")&&ok;
    }


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
    ok=check(helpBody && helpBody->toPlainText().contains(QStringLiteral("point d’interrogation orange"))
             &&helpBody->toHtml().contains(QStringLiteral(":/icons/helpbubble.png")),
             "Offline Help must explain and display the actual new question mark")&&ok;
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

    // Both actual information-dialog entry points share one Qt visual style.
    // Check both dark and light palettes without opening a modal window or
    // triggering an update or language restart.
    const bool priorInformationDialogTheme=qApp->property("ecuDarkTheme").toBool();
    for(bool darkInformationDialog : {true,false}){
        qApp->setProperty("ecuDarkTheme",darkInformationDialog);
        QMessageBox dialog(QMessageBox::Information,QStringLiteral("Test"),
                           QStringLiteral("Message"),QMessageBox::Ok,&window);
        SessionInformationMessage::configure(dialog);
        const QPixmap iconPixmap=dialog.iconPixmap();
        int orange=0;
        if(!iconPixmap.isNull()){
            const QImage pixels=iconPixmap.toImage().convertToFormat(QImage::Format_ARGB32);
            for(int y=0;y<pixels.height();++y)
                for(int x=0;x<pixels.width();++x){
                    const QColor pixel=pixels.pixelColor(x,y);
                    if(pixel.alpha()>160 && pixel.red()>220 &&
                       pixel.green()>90 && pixel.green()<190 &&
                       pixel.blue()<100) ++orange;
                }
        }
        const QColor interior=iconPixmap.isNull()
            ?QColor(255,255,255,255)
            :iconPixmap.toImage().pixelColor(10,16);
        const QColor letter=iconPixmap.isNull()
            ?QColor(0,0,0,0)
            :iconPixmap.toImage().pixelColor(16,19);
        ok=check(dialog.objectName()==QStringLiteral("sessionInformationMessageBox") &&
                 interior.alpha()<64 && letter.alpha()>160 &&
                 dialog.standardButtons()==QMessageBox::Ok &&
                 dialog.styleSheet().contains(darkInformationDialog
                     ?QStringLiteral("background:#090e13")
                     :QStringLiteral("background:#ffffff")) &&
                 dialog.styleSheet().contains(QStringLiteral("QMessageBox QPushButton{background:#ff")) &&
                 orange>24,
                 "Language restart and program update dialogs must display a visible orange information letter, button and themed title") && ok;
    }
    qApp->setProperty("ecuDarkTheme",priorInformationDialogTheme);

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

    // The same QAction handles Quit in every translated language.
    // Recolor its actual Qt icon instead of changing the close behavior.
    QAction *quitMenuAction=window.findChild<QAction*>(
        QStringLiteral("m_exitAction"));
    ok=check(quitMenuAction && !quitMenuAction->icon().isNull(),
             "File Quit action must retain its Qt icon in every language") && ok;
    if(quitMenuAction && !quitMenuAction->icon().isNull()){
        const QImage quitImage=quitMenuAction->icon().pixmap(24,24).toImage()
            .convertToFormat(QImage::Format_ARGB32);
        int orange=0,wrongColor=0;
        for(int y=0;y<quitImage.height();++y)
            for(int x=0;x<quitImage.width();++x){
                const QColor pixel=quitImage.pixelColor(x,y);
                if(pixel.alpha()<180)continue;
                if(pixel.red()>220 && pixel.green()>90 && pixel.green()<190 &&
                   pixel.blue()<100)++orange;
                else ++wrongColor;
            }
        ok=check(orange>24 && wrongColor==0,
                 "File Quit Qt icon must be orange with no black or blue pixels") && ok;
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
    // Same 14px transparent orange question mark, in both themes.
    const QPixmap lightHelp=TooltipBubbleManager::helpBubblePixmap(false);
    const QPixmap darkHelp=TooltipBubbleManager::helpBubblePixmap(true);
    const QImage helpPixels=lightHelp.toImage().convertToFormat(QImage::Format_ARGB32);
    int orange=0,transparent=0,other=0;
    for(int y=0;y<helpPixels.height();++y)
        for(int x=0;x<helpPixels.width();++x){
            const QColor pixel=helpPixels.pixelColor(x,y);
            if(pixel.alpha()<10){++transparent;continue;}
            if(pixel.alpha()<180)continue;
            if(pixel.red()>190 && pixel.green()>75 && pixel.green()<135 && pixel.blue()<25)++orange;
            else ++other;
        }
    ok=check(!lightHelp.isNull()&&!darkHelp.isNull() &&
             lightHelp.toImage()==darkHelp.toImage() &&
             qRound(lightHelp.width()/lightHelp.devicePixelRatio())==14 &&
             qRound(lightHelp.height()/lightHelp.devicePixelRatio())==14 &&
             orange>40&&transparent>40&&other==0,
             "All help icons use the same transparent 14px orange question mark")&&ok;

    // Dark and light have explicit independent foreground/background pairs.
    // Test the genuine stylesheet text used by both application startup and
    // rebuilt SESSION shell, without any theme-dependent hardcoded widget patch.
    const QString darkTip=STYLE_DARK.mid(STYLE_DARK.lastIndexOf(QStringLiteral("QToolTip {")));
    const QString lightTip=STYLE_LIGHT.mid(STYLE_LIGHT.lastIndexOf(QStringLiteral("QToolTip {")));
    ok=check(darkTip.contains(QStringLiteral("background-color: #000000;")) &&
             darkTip.contains(QStringLiteral("color: #ffffff;")) &&
             lightTip.contains(QStringLiteral("background-color: #ffffff;")) &&
             lightTip.contains(QStringLiteral("color: #000000;")) &&
             darkTip.contains(QStringLiteral("border: 1px solid #ff8a22;")) &&
             lightTip.contains(QStringLiteral("border: 1px solid #ff8a22;")) &&
             darkTip.contains(QStringLiteral("border-radius: 8px;")) &&
             lightTip.contains(QStringLiteral("border-radius: 8px;")) &&
             qApp->styleSheet().contains(QStringLiteral("QToolTip")),
             "Tooltips must have legible contrasting colors in both themes") && ok;

    // Actual hover presentation: two adjacent targets, both themes,
    // no duplicate line and the correct tooltip text.
    {
        QWidget probe(&window,Qt::Tool);
        auto *layout=new QVBoxLayout(&probe);
        auto *first=new QLabel(QStringLiteral("DTC 0"),&probe);
        auto *second=new QLabel(QStringLiteral("DTC 1"),&probe);
        first->setStyleSheet(QStringLiteral("QLabel{background:transparent;border:0;}"));
        first->setToolTip(QStringLiteral("DTC zero unique"));
        second->setToolTip(QStringLiteral("DTC one unique\nDTC one unique"));
        for(auto *label:{first,second}){
            label->setProperty("helpBubbleLeft",true);
            layout->addWidget(label);
        }
        const bool previousDark=qApp->property("ecuDarkTheme").toBool();
        const QString previousStyle=qApp->styleSheet();
        probe.resize(500,100);
        TooltipBubbleManager::install(&probe);
        probe.show();QCoreApplication::processEvents();
        for(bool dark:{false,true}){
            qApp->setProperty("ecuDarkTheme",dark);
            qApp->setStyleSheet(dark?STYLE_DARK:STYLE_LIGHT);
            QCoreApplication::processEvents();
            for(auto *label:{first,second}){
                auto *icon=label->findChild<QLabel*>(QStringLiteral("_ecuHelpBubble"),Qt::FindDirectChildrenOnly);
                ok=check(icon&&!icon->isHidden(),"Help icon exists")&&ok;
                if(!icon)continue;
                ok=check(qAbs(icon->geometry().center().y()-label->rect().center().y())<=1,"Help icon vertical alignment")&&ok;
                const QPoint point=icon->geometry().center();
                QHelpEvent event(QEvent::ToolTip,point,label->mapToGlobal(point));
                QApplication::sendEvent(label,&event);
                const QString text=QToolTip::text();
                const QString expected=label==first?QStringLiteral("DTC zero unique"):QStringLiteral("DTC one unique");
                const QString other=label==first?QStringLiteral("DTC one unique"):QStringLiteral("DTC zero unique");
                ok=check(text.contains(label->text())&&text.count(expected)==1&&!text.contains(other)
                    &&text.contains(QStringLiteral("<table width=")),"Correct row help, wrapped and deduplicated")&&ok;
                ok=check(renderedTooltip(dark),"Actual Qt tooltip has orange border and contrasting background")&&ok;
            }
        }
        QToolTip::hideText();probe.hide();
        qApp->setProperty("ecuDarkTheme",previousDark);
        qApp->setStyleSheet(previousStyle);
        QCoreApplication::processEvents();
    }
    auto *fileEditor=window.findChild<QLineEdit*>(QStringLiteral("m_logFileNameBox"));
    auto *statusEditor=window.findChild<QWidget*>(QStringLiteral("statusFileEditor"));
    ok=check(fileEditor&&statusEditor&&statusEditor->isAncestorOf(fileEditor)&&
             window.findChildren<QLineEdit*>(QStringLiteral("m_logFileNameBox")).size()==1&&
             !window.findChild<QLabel*>(QStringLiteral("statusFile")),
             "Single editable recording filename is in the bottom status")&&ok;
    ok=check(window.findChild<QTimer*>(QStringLiteral("statusRefreshTimer"))!=nullptr,
             "Status refresh remains connected after moving the recording editor")&&ok;
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
        auto *portHelp=options->findChild<QLabel*>(QStringLiteral("optionsSerialDeviceHelpLabel"));
        auto *portIcon=portHelp?portHelp->findChild<QLabel*>(QStringLiteral("_ecuHelpBubble"),Qt::FindDirectChildrenOnly):nullptr;
        ok=check(portIcon&&portIcon->isVisibleTo(options)&&portIcon->size()==QSize(18,18),
                 "Options COM help has the same small icon beside its label")&&ok;
        for(auto *combo:options->findChildren<QComboBox*>())
            if(combo->property("helpBubbleNoIcon").toBool()){
                auto *overlap=combo->findChild<QLabel*>(QStringLiteral("_ecuHelpBubble"),Qt::FindDirectChildrenOnly);
                ok=check(!overlap||overlap->isHidden(),"Options combo arrow unobstructed")&&ok;
            }
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
        if(page)for(auto *icon:page->findChildren<QLabel*>(QStringLiteral("_ecuHelpBubble")))
            if(icon->isVisibleTo(page))
                ok=check(icon->size()==QSize(18,18)&&icon->pixmap()&&!icon->pixmap()->isNull(),
                         "Every page uses the common small help icon")&&ok;
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
        if(size.width()>=1200){
            QWidget *actuatorPage=window.findChild<QWidget*>(QStringLiteral("actuators"));
            int actuatorIndex=-1;
            for(int i=0;i<tabs->count();++i)
                if(actualPage(tabs->widget(i))==actuatorPage){actuatorIndex=i;break;}
            ok=check(actuatorIndex>=0,"Actuator page missing from navigation") && ok;
            if(actuatorIndex>=0){
                tabs->setCurrentIndex(actuatorIndex);
                QCoreApplication::processEvents();
                QCoreApplication::processEvents();
                auto *gauge=actuatorPage->findChild<QWidget*>(QStringLiteral("actuatorGaugeTestCard"));
                auto *state=actuatorPage->findChild<QWidget*>(QStringLiteral("actuatorSystemState"));
                auto *move=actuatorPage->findChild<QWidget*>(QStringLiteral("m_moveIACButton"));
                auto *reason=actuatorPage->findChild<QLabel*>(QStringLiteral("gaugeTestReason"));
                if(gauge&&state&&move&&reason){
                    const QRect g(gauge->mapTo(actuatorPage,QPoint(0,0)),gauge->size());
                    const QRect st(state->mapTo(actuatorPage,QPoint(0,0)),state->size());
                    const QRect m(move->mapTo(actuatorPage,QPoint(0,0)),move->size());
                    const QRect msg(reason->mapTo(gauge,QPoint(0,0)),reason->size());
                    logSmoke(QStringLiteral("SESSION_218_ACTUATOR_LAYOUT %1px: stepperBottom=%2 gauge=%3..%4 state=%5..%6")
                        .arg(size.width()).arg(m.bottom()).arg(g.top()).arg(g.bottom()).arg(st.top()).arg(st.bottom()));
                    ok=check(move->isVisibleTo(&window)&&gauge->isVisibleTo(&window)&&
                             state->isVisibleTo(&window)&&reason->isVisibleTo(&window)&&
                             m.bottom()<g.top()&&g.bottom()<st.top()&&
                             actuatorPage->rect().contains(g)&&actuatorPage->rect().contains(st)&&
                             gauge->rect().contains(msg)&&state->height()>=96&&state->height()<=126,
                             "Actuator sections must be visible, ordered and non-overlapping") && ok;
                    auto *dial=gauge->findChild<QWidget*>(QStringLiteral("actuatorTemperatureDial"));
                    auto *btn=gauge->findChild<QPushButton*>(QStringLiteral("gaugeTestMinimum"));
                    auto *title=actuatorPage->findChild<QLabel*>(QStringLiteral("actuatorStepperTitle"));
                    auto *heater=actuatorPage->findChild<QLabel*>(QStringLiteral("m_O2Heater"));
                    ok=check(dial&&btn&&title&&dial->width()>=132&&dial->height()>=88&&
                             btn->height()==26&&state->width()>=gauge->width()-2&&
                             title->height()==22&&title->y()<30,
                             "Readable dial, compact buttons, full-width system card and top-aligned stepper title")&&ok;
                    ok=check(heater&&heater->wordWrap(),"Heater label permits wrapping instead of clipping")&&ok;
                    if(auto *scroll=qobject_cast<QScrollArea*>(tabs->widget(actuatorIndex)))
                        ok=check(scroll->verticalScrollBar()->maximum()==0,
                                 "Actuator page must not require vertical scrolling") && ok;
                }else ok=check(false,"Actuator section missing after resize") && ok;
            }
        }
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
                // Visual review of installed 197 requested a further 4px downward
                // adjustment for PLIP, while RC5/5AS remain unchanged.
                ok=check(rc5 && five && plip && reset &&
                         qAbs(rc5Y-fiveY)<=1 &&
                         qAbs((plipY-fiveY)-4)<=1 && plipY==resetY,
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
        // Inspect every page's actual visible help labels after the theme switch.
        int helpTargets=0;
        for(auto *icon:window.findChildren<QLabel*>(QStringLiteral("_ecuHelpBubble"))){
            if(icon->isHidden())continue;
            ++helpTargets;
            ok=check(icon->size()==QSize(18,18)&&icon->pixmap()&&!icon->pixmap()->isNull(),
                     "Theme switch preserves every small help target")&&ok;
        }
        ok=check(helpTargets>=7,"Permanent help targets must remain present")&&ok;
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

        // Capture the real installed widgets, rather than treating geometry checks
        // as human visual approval. Restore the original size and selected page.
        const QSize priorSize=window.size();
        QDir().mkpath(QStringLiteral("session221-visual"));
        for(const QSize captureSize:{QSize(1200,768),QSize(1366,768),QSize(1920,1080)}){
            window.resize(captureSize);
            for(int page:{0,2,3}){
                tabs->setCurrentIndex(page);QCoreApplication::processEvents();QCoreApplication::processEvents();
                const QString name=QStringLiteral("session221-visual/%1-%2-%3.png")
                    .arg(dark?QStringLiteral("dark"):QStringLiteral("light"))
                    .arg(captureSize.width()).arg(page);
                ok=check(window.grab().save(name,"PNG"),"Save installed UI visual evidence")&&ok;
            }
        }
        window.resize(priorSize);tabs->setCurrentIndex(originalTab);QCoreApplication::processEvents();
        // Test each actual page after its native layout is visible.
        for(int page=0;page<tabs->count();++page){
            tabs->setCurrentIndex(page);
            QCoreApplication::processEvents();
            ok=check(verifyScrollbars(tabs->widget(page),dark),"Every visible page scrollbar must use the common 9px style")&&ok;
        }
        tabs->setCurrentIndex(10);
        QCoreApplication::processEvents();
        if(QLabel *label=window.findChild<QLabel*>(QStringLiteral("label_0x0F"))){
            auto *icon=label->findChild<QLabel*>(QStringLiteral("_ecuHelpBubble"),Qt::FindDirectChildrenOnly);
            if(icon && icon->isVisible()){
                const QPoint p=icon->geometry().center();
                QHelpEvent e(QEvent::ToolTip,p,label->mapToGlobal(p));
                QApplication::sendEvent(label,&e);
                ok=check(renderedTooltip(dark),"Real All data tooltip uses the shared border")&&ok;
            }else ok=check(false,"Real All data help icon not visible")&&ok;
        }
        QToolTip::hideText();
        tabs->setCurrentIndex(8);
        QCoreApplication::processEvents();
        if(SummaryTab *summary=window.findChild<SummaryTab*>()){
            bool checked=false;
            for(QTableWidget *table:summary->findChildren<QTableWidget*>()){
                for(int row=0;row<table->rowCount();++row){
                    auto *item=table->item(row,1);
                    if(!item||item->toolTip().isEmpty()||table->isRowHidden(row))continue;
                    const QRect r=table->visualItemRect(item);
                    if(!r.intersects(table->viewport()->rect()))continue;
                    const QPoint p=r.center();
                    QHelpEvent e(QEvent::ToolTip,p,table->viewport()->mapToGlobal(p));
                    QApplication::sendEvent(table->viewport(),&e);
                    checked=renderedTooltip(dark);break;
                }
                if(checked)break;
            }
            ok=check(checked,"Real All measurements tooltip uses the shared border")&&ok;
        }
        QToolTip::hideText();
        tabs->setCurrentIndex(originalTab);
        QCoreApplication::processEvents();
        manual.show();
        QCoreApplication::processEvents();
        ok=check(verifyScrollbars(&manual,dark),"Help scrollbars must use the common 9px style")&&ok;
        if(chapters){
            ok=check(chapters->font().pointSizeF()>=10.9 && chapters->wordWrap() &&
                     chapters->textElideMode()==Qt::ElideNone && chapters->width()>=300,
                     "Help chapter navigation must be readable and wrapped")&&ok;
        }
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
                            QStringLiteral("#ff8a22"));
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
                                 bar->width()==kSessionScrollBarExtent && orange,
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
                ok=check(curvesMode && viewMode && loadFile &&
                         modeControls && !modeHelp,
                         "Two centred Analysis display buttons must have no redundant HELP icon") && ok;
                if(curvesMode && viewMode && loadFile && modeControls && !modeHelp){
                    const auto midY=[&window](QWidget *w){
                        return w->mapTo(&window,QPoint(0,w->height()/2)).y();
                    };
                    logSmoke(QStringLiteral("Analysis toolbar centers: load=%1 curves=%2 columns=%3")
                        .arg(midY(loadFile)).arg(midY(curvesMode)).arg(midY(viewMode)));
                    ok=check(qAbs(midY(loadFile)-midY(curvesMode))<=4 &&
                             qAbs(midY(loadFile)-midY(viewMode))<=4,
                             "Analysis mode controls must share the load-file toolbar row") && ok;
                    QLabel *filename=analysis->findChild<QLabel*>(
                        QStringLiteral("analysisFileLabel"));
                    ok=check(filename && filename->toolTip().isEmpty() &&
                             !filename->findChild<QLabel*>(
                                 QStringLiteral("_ecuHelpBubble"),
                                 Qt::FindDirectChildrenOnly) &&
                             !loadFile->findChild<QLabel*>(
                                 QStringLiteral("_ecuHelpBubble"),
                                 Qt::FindDirectChildrenOnly) &&
                             !viewMode->findChild<QLabel*>(
                                 QStringLiteral("_ecuHelpBubble"),
                                 Qt::FindDirectChildrenOnly) &&
                             !curvesMode->findChild<QLabel*>(
                                 QStringLiteral("_ecuHelpBubble"),
                                 Qt::FindDirectChildrenOnly),
                             "Analysis CSV/TXT and Curves/Columns must not display HELP bubbles") && ok;
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
                                 !modeHelp,
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
    // Exercise the actual service and library routing, using the same
    // validated snapshot as Diagnostic. No ECU or model activation.
    {
        using namespace Mems16Na;
        SessionEvidence na;
        auto tx=[&](std::uint8_t command,const std::vector<std::uint8_t> &response){
            na.tx({command});na.rx(response);na.finish();
        };
        tx(0xd0,{0xd0,0x98,0,2,2});
        tx(0xd1,{0xd1,'A','A','N','M','P','0','0','2',0x98,0,2,2});
        std::vector<std::uint8_t> a(29,0),b(33,0);
        a[0]=0x80;a[1]=28;a[5]=255;a[7]=255;a[16]=8;a[17]=32;
        b[0]=0x7d;b[1]=32;b[6]=0x40;b[10]=1;b[12]=129;
        b[15]=255;b[17]=255;b[18]=255;b[20]=126;b[21]=0x7f;b[22]=0xff;b[23]=255;
        tx(0x80,a);tx(0x7d,b);
        if(!na.ready()){logSmoke(QStringLiteral("FAIL IA NA test identity"));return 8;}
        mems_data d{};d.engine_rpm=900;d.battery_voltage=138;d.coolant_temp=100;d.map_kpa=40;d.coil_time=1250;
        DiagnosticPanel panel;panel.updateData(&d,&na);
        auto *service=IaMemsService::instance();
        QString answer;
        const auto connection=QObject::connect(service,&IaMemsService::responseReady,&app,[&](const QString &value){answer=value;});
        auto ask=[&](const QString &question){
            answer.clear();service->askWithLibrary(question);return answer;
        };
        service->setNaDiagnostic(na,d,panel.naDiagnosticReport());
        const QString diagnostic=ask(QStringLiteral("diagnostic moteur"));
        bool passed=diagnostic.contains(panel.naDiagnosticReport().section(QLatin1Char('\n'),0,0))
            &&diagnostic.contains(I18n::text(29019))&&diagnostic.contains(QStringLiteral("8 µs"))
            &&diagnostic.contains(QStringLiteral("-1 RPM"));
        passed=ask(QStringLiteral("DTC 4")).contains(I18n::text(29001))&&passed;
        passed=ask(QStringLiteral("DTC 2")).contains(I18n::text(29019))&&passed;
        passed=ask(QStringLiteral("correction temps injection")).contains(QStringLiteral("8 µs"))&&passed;
        passed=ask(QStringLiteral("historique")).contains(I18n::text(29083).arg(1))&&passed;
        na.discardPair();panel.updateData(&d,&na);
        service->setNaDiagnostic(na,d,panel.naDiagnosticReport());
        passed=ask(QStringLiteral("diagnostic moteur")).contains(I18n::text(29048))&&passed;
        service->setConnected(false);service->clearNaDiagnostic();
        passed=ask(QStringLiteral("AANMP002 DTC 4")).contains(I18n::text(29001))&&passed;
        QObject::disconnect(connection);
        service->clearConversation();
        service->setProperty("iaMemsActiveDocumentaryComponent",QVariant());
        if(!passed){logSmoke(QStringLiteral("FAIL IA NA service routing"));return 8;}
        logSmoke(QStringLiteral("SESSION_209_IA_NA_SERVICE_PASS"));
    }
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
    // Exercise the same pre-show initialization order as the real startup.
    window.ensurePolished();
    buildUiShell(&window);
    window.show();
    logSmoke(QStringLiteral("After show()"));

    // Check the first rendered menu before any page navigation.
    QTimer::singleShot(350,&window,[&app,&window](){
        logSmoke(QStringLiteral("Enter timer / start checks"));
        const bool startupMenu=uiShellNavigationReady(&window,true);
        if(!startupMenu)logSmoke(QStringLiteral("FAIL first rendered startup navigation"));
        const bool ok=verifyUi(window)&&startupMenu;
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
