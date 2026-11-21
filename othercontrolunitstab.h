#ifndef OTHERCONTROLUNITSTAB_H
#define OTHERCONTROLUNITSTAB_H

// SESSION 190: independent RC5 SRS and Lucas 5AS diagnostic workspaces.
// Output tests require an explicit warning confirmation; keys remain reserved.
#include <QApplication>
#include <QDialog>
#include <QFile>
#include <QFileDialog>
#include <QScreen>
#include <QTextBrowser>
#include <QFrame>
#include <QGridLayout>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QMetaObject>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QResizeEvent>
#include <QSerialPortInfo>
#include <QSizePolicy>
#include <QString>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QThread>
#include <QTimer>
#include <QVariantList>
#include <QVBoxLayout>
#include <QWidget>
#include "i18n.h"
#include "rc5protocol.h"
#include "lucas5asreader.h"
#include "serialadapterdetector.h"

class OtherControlUnitsTab final : public QWidget
{
public:
    explicit OtherControlUnitsTab(QWidget *parent=nullptr)
        : QWidget(parent)
    {
        setObjectName(QStringLiteral("other_ecus_tab"));
        setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
        setMinimumSize(0,0);
        QHBoxLayout *columns=new QHBoxLayout(this);
        columns->setContentsMargins(14,14,14,14);
        columns->setSpacing(12);

        static const char *const panels[3]={
            "otherEcusAirbagRc5","otherEcusImmobiliser","otherEcusKeys"
        };
        static const char *const titles[3]={
            "otherEcusAirbagTitle","otherEcusImmobiliserTitle",
            "otherEcusKeysTitle"
        };
        static const char *const helpIds[3]={
            "rc5CableInfoButton","immobiliserCableInfoButton",
            "keysCableInfoButton"
        };

        for(int i=0;i<3;++i){
            QFrame *panel=new QFrame(this);
            panel->setObjectName(QString::fromLatin1(panels[i]));
            panel->setAttribute(Qt::WA_StyledBackground,true);
            panel->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
            panel->setMinimumSize(0,0);
            QVBoxLayout *layout=new QVBoxLayout(panel);
            layout->setContentsMargins(10,13,10,11);
            layout->setSpacing(9);
            QLabel *title=new QLabel(panel);
            title->setObjectName(QString::fromLatin1(titles[i]));
            title->setAlignment(Qt::AlignCenter);
            title->setWordWrap(true);
            QFont font=title->font();
            font.setPointSizeF(10.0);
            font.setBold(true);
            title->setFont(font);
            layout->addWidget(title);

            QFrame *line=new QFrame(panel);
            line->setObjectName(QStringLiteral("otherEcusTitleDivider"));
            line->setFrameShape(QFrame::HLine);
            line->setFixedHeight(1);
            layout->addWidget(line);

            m_panels[i]=panel;
            m_headings[i]=title;
            m_dividers[i]=line;
            if(i==0)
                createRc5Panel(layout,panel);
            else if(i==1)
                create5AsPanel(layout,panel);
            else {
                layout->addStretch(1);
                QLabel *placeholder=new QLabel(panel);
                placeholder->setObjectName(QStringLiteral("otherEcusReservedLabel"));
                placeholder->setAlignment(Qt::AlignCenter);
                placeholder->setWordWrap(true);
                layout->addWidget(placeholder);
                layout->addStretch(2);
                m_placeholders[i]=placeholder;
            }
            // One independent offline cable help at the bottom of each column.
            // The key-programming module remains reserved.
            QPushButton *cableHelp=new QPushButton(panel);
            cableHelp->setObjectName(QString::fromLatin1(helpIds[i]));
            cableHelp->setMinimumHeight(32);
            cableHelp->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
            layout->addWidget(cableHelp);
            m_cableHelpButtons[i]=cableHelp;
            QObject::connect(cableHelp,&QPushButton::clicked,this,
                             [this,i](){showCableHelp(i);});
            columns->addWidget(panel,1);
        }

        m_worker=new Rc5Worker;
        m_worker->moveToThread(&m_rc5Thread);
        QObject::connect(&m_rc5Thread,&QThread::finished,
                         m_worker,&QObject::deleteLater);
        QObject::connect(m_worker,&Rc5Worker::probingPort,this,
                         [this](const QString &port){
            if(!m_busy)return;
            m_port=port;
            setStatus(16810);
            updatePortLabels();
        });
        QObject::connect(m_worker,&Rc5Worker::connectionChanged,this,
                         [this](bool connected,const QString &port){
            m_busy=false;
            m_connected=connected;
            if(connected){
                m_cancelRequested=false;
                m_port=port;
                setStatus(16811,port);
            } else {
                m_port.clear();
                m_raw.clear();
                m_faults.clear();
                showFaults();
                setStatus(m_cancelRequested?16826:16812);
                m_cancelRequested=false;
            }
            updatePortLabels();
            updateButtons();
        });
        QObject::connect(m_worker,&Rc5Worker::faultsRead,this,
                         [this](bool ok,const QVariantList &faults,
                                const QString &raw){
            if(!m_connected)return;
            m_busy=false;
            if(ok){
                m_faults=faults;
                m_raw=raw;
                showFaults();
                setStatus(faults.isEmpty()?16814:16815,
                          faults.isEmpty()?QString():QString::number(faults.size()));
            } else {
                setStatus(16816);
            }
            updatePortLabels();
            updateButtons();
        });
        QObject::connect(m_worker,&Rc5Worker::faultsCleared,this,
                         [this](bool ok){
            if(!m_connected)return;
            if(ok){
                setStatus(16818);
                // Worker immediately re-reads faults before becoming idle.
            }else{
                m_busy=false;
                setStatus(16819);
                updateButtons();
            }
        });
        QObject::connect(m_connect,&QPushButton::clicked,this,
                         [this](){connectRc5();});
        QObject::connect(m_disconnect,&QPushButton::clicked,this,
                         [this](){disconnectRc5();});
        QObject::connect(m_read,&QPushButton::clicked,this,
                         [this](){readRc5();});
        QObject::connect(m_clear,&QPushButton::clicked,this,
                         [this](){clearRc5();});
        m_rc5Thread.start();

        // 5AS runs independently: no shared serial handle or RC5 commands.
        m_5asWorker=new Lucas5AsWorker;
        m_5asWorker->moveToThread(&m_5asThread);
        QObject::connect(&m_5asThread,&QThread::finished,
                         m_5asWorker,&QObject::deleteLater);
        QObject::connect(m_5asWorker,&Lucas5AsWorker::probingPort,this,
                         [this](const QString &port){
            if(!m_5asBusy)return;
            m_5asPort=port;
            set5AsStatus(16900);
            update5AsPort();
        });
        QObject::connect(m_5asWorker,&Lucas5AsWorker::connectionChanged,this,
                         [this](bool connected,const QString &port){
            m_5asBusy=false;
            m_5asConnected=connected;
            if(connected){
                m_5asCancelRequested=false;
                m_5asPort=port;
                set5AsStatus(16901,port);
            }else{
                m_5asPort.clear();
                m_5asFields.clear();
                show5AsFields();
                m_5asLiveActive=false;
                if(m_5asLiveTimer)m_5asLiveTimer->stop();
                set5AsStatus(m_5asCancelRequested?16905:16902);
                m_5asCancelRequested=false;
            }
            update5AsPort();
            update5AsButtons();
        });
        QObject::connect(m_5asWorker,&Lucas5AsWorker::statusRead,this,
                         [this](bool ok,const QVariantMap &fields,
                                const QString &trace){
            if(!m_5asConnected)return;
            m_5asBusy=false;
            if(ok){
                m_5asFields=fields;
                show5AsFields();
                if(!trace.isEmpty())
                    m_5asTrace->appendPlainText(trace.trimmed());
                set5AsStatus(16907);
            }else{
                m_5asLiveActive=false;
                if(m_5asLiveTimer)m_5asLiveTimer->stop();
                set5AsStatus(16904);
            }
            update5AsButtons();
        });
        QObject::connect(m_5asWorker,&Lucas5AsWorker::outputTested,this,
                         [this](bool ok,int output,const QString &trace){
            Q_UNUSED(output)
            if(!m_5asConnected)return;
            m_5asBusy=false;
            if(!trace.isEmpty())m_5asTrace->appendPlainText(trace.trimmed());
            set5AsStatus(ok?16927:16928);
            update5AsButtons();
        });
        QObject::connect(m_5asConnect,&QPushButton::clicked,
                         this,[this](){connect5As();});
        QObject::connect(m_5asDisconnect,&QPushButton::clicked,
                         this,[this](){disconnect5As();});
        QObject::connect(m_5asRead,&QPushButton::clicked,
                         this,[this](){read5As();});
        QObject::connect(m_5asLive,&QPushButton::clicked,
                         this,[this](){toggle5AsLive();});
        QObject::connect(m_5asHorn,&QPushButton::clicked,
                         this,[this](){test5AsOutput(8);});
        QObject::connect(m_5asLed,&QPushButton::clicked,
                         this,[this](){test5AsOutput(9);});
        QObject::connect(m_5asHazards,&QPushButton::clicked,
                         this,[this](){test5AsOutput(13);});
        QObject::connect(m_5asSaveLog,&QPushButton::clicked,
                         this,[this](){save5AsLog();});
        m_5asLiveTimer=new QTimer(this);
        m_5asLiveTimer->setObjectName(QStringLiteral("lucas5asLiveTimer"));
        m_5asLiveTimer->setInterval(1000);
        QObject::connect(m_5asLiveTimer,&QTimer::timeout,this,
                         [this](){
            if(m_5asLiveActive && m_5asConnected && !m_5asBusy)read5As();
        });
        m_5asThread.start();

        retranslate();
        refreshTheme();
        updateButtons();
        update5AsButtons();
    }

    ~OtherControlUnitsTab() override
    {
        if(m_worker)m_worker->requestCancel();
        m_rc5Thread.quit();
        m_rc5Thread.wait();
        if(m_5asWorker)m_5asWorker->requestCancel();
        m_5asThread.quit();
        m_5asThread.wait();
    }

    void retranslate()
    {
        for(int i=0;i<3;++i){
            m_headings[i]->setText(I18n::text(16801+i));
            if(m_placeholders[i])
                m_placeholders[i]->setText(I18n::text(16804));
            m_cableHelpButtons[i]->setText(I18n::text(16890));
        }
        m_connect->setText(I18n::text(16805));
        m_disconnect->setText(I18n::text(16806));
        m_read->setText(I18n::text(16807));
        m_clear->setText(I18n::text(16808));
        m_5asConnect->setText(I18n::text(16805));
        m_5asDisconnect->setText(I18n::text(16806));
        m_5asRead->setText(I18n::text(16906));
        m_5asLive->setText(I18n::text(m_5asLiveActive?16941:16940));
        m_5asHorn->setText(I18n::text(16924));
        m_5asLed->setText(I18n::text(16925));
        m_5asHazards->setText(I18n::text(16926));
        m_5asSaveLog->setText(I18n::text(16935));
        m_5asTable->setHorizontalHeaderLabels({
            I18n::text(16920),I18n::text(16921)
        });
        m_5asCableNote->setText(I18n::text(16923));
        update5AsStatus();
        update5AsPort();
        show5AsFields();
        m_table->setHorizontalHeaderLabels({
            I18n::text(16820),I18n::text(16821),I18n::text(16822)
        });
        m_cableNote->setText(I18n::text(16827));
        updateStatus();
        updatePortLabels();
        showFaults();
    }

    void refreshTheme()
    {
        const bool dark=qApp && qApp->property("ecuDarkTheme").toBool();
        const QString surface=dark?QStringLiteral("#10161c"):QStringLiteral("#ffffff");
        const QString border=dark?QStringLiteral("#2d3943"):QStringLiteral("#d5dee7");
        const QString muted=dark?QStringLiteral("#aebcc5"):QStringLiteral("#637587");
        const QString foreground=dark?QStringLiteral("#e7edf0"):QStringLiteral("#202936");
        const QString highlight=dark?QStringLiteral("#ff9828"):QStringLiteral("#b75500");

        for(int i=0;i<3;++i){
            m_panels[i]->setStyleSheet(
                QStringLiteral("QFrame#%1{background:%2;border:1px solid %3;border-radius:6px;}")
                    .arg(m_panels[i]->objectName(),surface,border));
            m_headings[i]->setStyleSheet(
                QStringLiteral("color:%1;background:transparent;border:0;").arg(highlight));
            if(m_placeholders[i])
                m_placeholders[i]->setStyleSheet(
                    QStringLiteral("color:%1;background:transparent;border:0;").arg(muted));
            m_dividers[i]->setStyleSheet(
                QStringLiteral("background:%1;border:0;").arg(border));
        }
        for(QLabel *label:{m_status,m_portLabel,m_rawLabel,m_cableNote,
                           m_5asStatus,m_5asPortLabel,m_5asCableNote})
            label->setStyleSheet(
                QStringLiteral("color:%1;background:transparent;border:0;")
                    .arg(label==m_status || label==m_5asStatus?foreground:muted));
        const QString buttonStyle=QStringLiteral(
            "QPushButton{background:%1;color:%2;border:1px solid %2;"
            "border-radius:4px;padding:4px 3px;}"
            "QPushButton:hover:enabled{background:%3;}"
            "QPushButton:disabled{color:%4;border:1px solid %4;background:%1;}"
        ).arg(dark?QStringLiteral("#090e13"):QStringLiteral("#ffffff"),
              highlight,
              dark?QStringLiteral("#19130a"):QStringLiteral("#fff0dd"),
              dark?QStringLiteral("#5d6870"):QStringLiteral("#aab4be"));
        for(QPushButton *button:{m_connect,m_disconnect,m_read,m_clear,
                                  m_5asConnect,m_5asDisconnect,m_5asRead,m_5asLive,
                                   m_5asHorn,m_5asLed,m_5asHazards,m_5asSaveLog})
            button->setStyleSheet(buttonStyle);
        for(QPushButton *button:m_cableHelpButtons)
            button->setStyleSheet(buttonStyle);
        const QString tableStyle=QStringLiteral(
            "QTableWidget{background:%1;color:%2;"
            "gridline-color:%3;border:1px solid %3;}"
            "QHeaderView::section{background:%1;color:%4;"
            "border:0;border-bottom:1px solid %3;padding:3px;}")
                .arg(surface,foreground,border,highlight);
        m_table->setStyleSheet(tableStyle);
        m_5asTable->setStyleSheet(tableStyle);
        m_5asTrace->setStyleSheet(QStringLiteral(
            "QPlainTextEdit{background:%1;color:%2;"
            "border:1px solid %3;border-radius:4px;}")
            .arg(surface,foreground,border));
    }

protected:
    void resizeEvent(QResizeEvent *event) override
    {
        QWidget::resizeEvent(event);
        // The RC5 table has a much larger size hint than the two reserved
        // workspaces. The original equal stretch alone did not create equal
        // widths; assign exactly one third to each independent column.
        QHBoxLayout *columns=qobject_cast<QHBoxLayout*>(layout());
        if(!columns)return;
        int left=0,top=0,right=0,bottom=0;
        columns->getContentsMargins(&left,&top,&right,&bottom);
        const int usable=qMax(0,width()-left-right-2*columns->spacing());
        const int third=usable/3;
        for(int i=0;i<3;++i){
            if(!m_panels[i])continue;
            const int target=third+(i==2?usable-3*third:0);
            if(m_panels[i]->minimumWidth()!=target ||
               m_panels[i]->maximumWidth()!=target)
                m_panels[i]->setFixedWidth(target);
        }
    }

private:
    void showCableHelp(int index)
    {
        if(index<0 || index>=3)return;
        QDialog dialog(this);
        dialog.setObjectName(QStringLiteral("otherEcusCableHelpDialog"));
        dialog.setWindowTitle(I18n::text(16891+2*index));
        QVBoxLayout *dialogLayout=new QVBoxLayout(&dialog);
        dialogLayout->setContentsMargins(14,14,14,12);
        dialogLayout->setSpacing(10);

        // Local translation catalog: the help works without an internet
        // connection, a detected COM port, or a connected ECU.
        QTextBrowser *contents=new QTextBrowser(&dialog);
        contents->setObjectName(QStringLiteral("otherEcusCableHelpContents"));
        contents->setOpenExternalLinks(false);
        contents->setHtml(I18n::text(16892+2*index));
        const bool dark=qApp && qApp->property("ecuDarkTheme").toBool();
        contents->setStyleSheet(QStringLiteral(
            "QTextBrowser{background:%1;color:%2;"
            "border:1px solid %3;border-radius:4px;padding:9px;}")
            .arg(dark?QStringLiteral("#10161c"):QStringLiteral("#ffffff"),
                 dark?QStringLiteral("#e7edf0"):QStringLiteral("#202936"),
                 dark?QStringLiteral("#2d3943"):QStringLiteral("#d5dee7")));
        dialogLayout->addWidget(contents,1);

        QPushButton *closeButton=new QPushButton(I18n::text(16897),&dialog);
        closeButton->setObjectName(QStringLiteral("otherEcusCableHelpClose"));
        closeButton->setMinimumHeight(32);
        QObject::connect(closeButton,&QPushButton::clicked,
                         &dialog,&QDialog::accept);
        dialogLayout->addWidget(closeButton,0,Qt::AlignRight);

        const QRect screen=qApp && qApp->primaryScreen()
            ? qApp->primaryScreen()->availableGeometry()
            : QRect(0,0,1024,768);
        dialog.resize(qMax(320,qMin(760,screen.width()-60)),
                      qMax(300,qMin(570,screen.height()-70)));
        dialog.exec();
    }

    void createRc5Panel(QVBoxLayout *root,QWidget *parent)
    {
        m_status=new QLabel(parent);
        m_status->setObjectName(QStringLiteral("rc5ConnectionStatus"));
        m_status->setWordWrap(true);
        root->addWidget(m_status);

        m_portLabel=new QLabel(parent);
        m_portLabel->setObjectName(QStringLiteral("rc5PortStatus"));
        m_portLabel->setWordWrap(true);
        root->addWidget(m_portLabel);

        QGridLayout *commands=new QGridLayout;
        commands->setHorizontalSpacing(5);
        commands->setVerticalSpacing(6);
        m_connect=new QPushButton(parent);
        m_connect->setObjectName(QStringLiteral("rc5ConnectButton"));
        m_disconnect=new QPushButton(parent);
        m_disconnect->setObjectName(QStringLiteral("rc5DisconnectButton"));
        m_read=new QPushButton(parent);
        m_read->setObjectName(QStringLiteral("rc5ReadFaultsButton"));
        m_clear=new QPushButton(parent);
        m_clear->setObjectName(QStringLiteral("rc5ClearFaultsButton"));
        for(QPushButton *button:{m_connect,m_disconnect,m_read,m_clear}){
            button->setMinimumWidth(0);
            button->setMinimumHeight(32);
            button->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
        }
        commands->addWidget(m_connect,0,0);
        commands->addWidget(m_disconnect,0,1);
        commands->addWidget(m_read,1,0);
        commands->addWidget(m_clear,1,1);
        commands->setColumnStretch(0,1);
        commands->setColumnStretch(1,1);
        root->addLayout(commands);

        m_table=new QTableWidget(0,3,parent);
        m_table->setObjectName(QStringLiteral("rc5FaultsTable"));
        m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_table->setSelectionMode(QAbstractItemView::SingleSelection);
        m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        m_table->setAlternatingRowColors(false);
        m_table->verticalHeader()->setVisible(false);
        m_table->horizontalHeader()->setStretchLastSection(true);
        m_table->horizontalHeader()->setSectionResizeMode(0,QHeaderView::Fixed);
        m_table->horizontalHeader()->setSectionResizeMode(1,QHeaderView::Fixed);
        m_table->horizontalHeader()->setSectionResizeMode(2,QHeaderView::Stretch);
        m_table->setColumnWidth(0,64);
        m_table->setColumnWidth(1,81);
        m_table->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        m_table->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        m_table->setMinimumHeight(165);
        m_table->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
        root->addWidget(m_table,1);

        m_rawLabel=new QLabel(parent);
        m_rawLabel->setObjectName(QStringLiteral("rc5RawReply"));
        m_rawLabel->setWordWrap(true);
        root->addWidget(m_rawLabel);
        m_cableNote=new QLabel(parent);
        m_cableNote->setObjectName(QStringLiteral("rc5CableNote"));
        m_cableNote->setWordWrap(true);
        root->addWidget(m_cableNote);
    }

    void create5AsPanel(QVBoxLayout *root,QWidget *parent)
    {
        m_5asStatus=new QLabel(parent);
        m_5asStatus->setObjectName(QStringLiteral("lucas5asConnectionStatus"));
        m_5asStatus->setWordWrap(true);
        root->addWidget(m_5asStatus);

        m_5asPortLabel=new QLabel(parent);
        m_5asPortLabel->setObjectName(QStringLiteral("lucas5asPortStatus"));
        m_5asPortLabel->setWordWrap(true);
        root->addWidget(m_5asPortLabel);

        QGridLayout *commands=new QGridLayout;
        commands->setHorizontalSpacing(5);
        commands->setVerticalSpacing(6);
        m_5asConnect=new QPushButton(parent);
        m_5asConnect->setObjectName(QStringLiteral("lucas5asConnectButton"));
        m_5asDisconnect=new QPushButton(parent);
        m_5asDisconnect->setObjectName(QStringLiteral("lucas5asDisconnectButton"));
        m_5asRead=new QPushButton(parent);
        m_5asRead->setObjectName(QStringLiteral("lucas5asReadStatusButton"));
        m_5asLive=new QPushButton(parent);
        m_5asLive->setObjectName(QStringLiteral("lucas5asLiveStatusButton"));
        for(QPushButton *button:{m_5asConnect,m_5asDisconnect,m_5asRead,m_5asLive}){
            button->setMinimumHeight(32);
            button->setMinimumWidth(0);
            button->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
        }
        commands->addWidget(m_5asConnect,0,0);
        commands->addWidget(m_5asDisconnect,0,1);
        commands->addWidget(m_5asRead,1,0);
        commands->addWidget(m_5asLive,1,1);
        commands->setColumnStretch(0,1);
        commands->setColumnStretch(1,1);
        root->addLayout(commands);

        QHBoxLayout *outputTests=new QHBoxLayout;
        outputTests->setSpacing(5);
        m_5asHorn=new QPushButton(parent);
        m_5asHorn->setObjectName(QStringLiteral("lucas5asTestHornButton"));
        m_5asLed=new QPushButton(parent);
        m_5asLed->setObjectName(QStringLiteral("lucas5asTestLedButton"));
        m_5asHazards=new QPushButton(parent);
        m_5asHazards->setObjectName(QStringLiteral("lucas5asTestHazardsButton"));
        for(QPushButton *button:{m_5asHorn,m_5asLed,m_5asHazards}){
            button->setMinimumWidth(0);
            button->setMinimumHeight(32);
            button->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
            outputTests->addWidget(button,1);
        }
        root->addLayout(outputTests);

        m_5asTable=new QTableWidget(0,2,parent);
        m_5asTable->setObjectName(QStringLiteral("lucas5asStatusTable"));
        m_5asTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        m_5asTable->setSelectionMode(QAbstractItemView::SingleSelection);
        m_5asTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_5asTable->verticalHeader()->setVisible(false);
        m_5asTable->horizontalHeader()->setSectionResizeMode(0,QHeaderView::Interactive);
        m_5asTable->horizontalHeader()->setSectionResizeMode(1,QHeaderView::Stretch);
        m_5asTable->setColumnWidth(0,135);
        m_5asTable->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        m_5asTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        m_5asTable->setMinimumHeight(165);
        m_5asTable->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
        root->addWidget(m_5asTable,1);

        m_5asTrace=new QPlainTextEdit(parent);
        m_5asTrace->setObjectName(QStringLiteral("lucas5asDiagnosticLog"));
        m_5asTrace->setReadOnly(true);
        m_5asTrace->setMinimumHeight(72);
        m_5asTrace->setMaximumHeight(103);
        m_5asTrace->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        root->addWidget(m_5asTrace);
        m_5asSaveLog=new QPushButton(parent);
        m_5asSaveLog->setObjectName(QStringLiteral("lucas5asSaveLogButton"));
        m_5asSaveLog->setMinimumHeight(30);
        root->addWidget(m_5asSaveLog);

        m_5asCableNote=new QLabel(parent);
        m_5asCableNote->setObjectName(QStringLiteral("lucas5asCableNote"));
        m_5asCableNote->setWordWrap(true);
        root->addWidget(m_5asCableNote);
    }
    void set5AsStatus(int key,const QString &arg=QString())
    {
        m_5asStatusKey=key;
        m_5asStatusArg=arg;
        update5AsStatus();
    }
    void update5AsStatus()
    {
        const QString text=I18n::text(m_5asStatusKey);
        m_5asStatus->setText(
            m_5asStatusArg.isEmpty()?text:text.arg(m_5asStatusArg));
    }
    void update5AsPort()
    {
        m_5asPortLabel->setText(m_5asPort.isEmpty()
            ?QString():I18n::text(16823).arg(m_5asPort));
    }
    void update5AsButtons()
    {
        m_5asConnect->setEnabled(!m_5asConnected && !m_5asBusy);
        m_5asDisconnect->setEnabled(m_5asConnected || m_5asBusy);
        m_5asRead->setEnabled(m_5asConnected && !m_5asBusy);
        m_5asLive->setEnabled(m_5asConnected);
        m_5asLive->setText(I18n::text(m_5asLiveActive?16941:16940));
        for(QPushButton *button:{m_5asHorn,m_5asLed,m_5asHazards})
            button->setEnabled(m_5asConnected && !m_5asBusy);
        m_5asSaveLog->setEnabled(!m_5asTrace->toPlainText().isEmpty());
    }
    void show5AsFields()
    {
        static const char *const fields[]={
            "model","serial","ignition","doors","bonnet","possibleBoot",
            "plip1","plip2","plip3","plip4"
        };
        const int rows=m_5asFields.isEmpty()?0:10;
        m_5asTable->setRowCount(rows);
        for(int row=0;row<rows;++row){
            const QString label=I18n::text(16910+row);
            const QString value=m_5asFields.value(
                QString::fromLatin1(fields[row])).toString();
            QTableWidgetItem *left=new QTableWidgetItem(label);
            QTableWidgetItem *right=new QTableWidgetItem(value);
            left->setToolTip(label);
            right->setToolTip(value);
            m_5asTable->setItem(row,0,left);
            m_5asTable->setItem(row,1,right);
        }
    }
    void connect5As()
    {
        if(m_5asBusy || m_5asConnected)return;
        const QList<DetectedSerialAdapter> adapters=
            SerialAdapterDetector::availableAdapters();
        QStringList ports;
        for(const DetectedSerialAdapter &adapter:adapters)
            if(adapter.detectedBySystem && !adapter.qtPortName.isEmpty() &&
               !ports.contains(adapter.qtPortName))
                ports.append(adapter.qtPortName);
        if(ports.isEmpty()){
            set5AsStatus(16825);
            return;
        }
        m_5asFields.clear();
        m_5asPort.clear();
        m_5asLiveActive=false;
        if(m_5asLiveTimer)m_5asLiveTimer->stop();
        m_5asTrace->clear();
        show5AsFields();
        update5AsPort();
        m_5asCancelRequested=false;
        m_5asBusy=true;
        m_5asWorker->allowRequests();
        set5AsStatus(16900);
        update5AsButtons();
        QMetaObject::invokeMethod(m_5asWorker,"connectCandidates",
                                  Qt::QueuedConnection,Q_ARG(QStringList,ports));
    }
    void disconnect5As()
    {
        if(!m_5asBusy && !m_5asConnected)return;
        m_5asLiveActive=false;
        if(m_5asLiveTimer)m_5asLiveTimer->stop();
        m_5asCancelRequested=true;
        m_5asWorker->requestCancel();
        m_5asBusy=true;
        update5AsButtons();
        QMetaObject::invokeMethod(m_5asWorker,"disconnectModule",
                                  Qt::QueuedConnection);
    }
    void read5As()
    {
        if(!m_5asConnected || m_5asBusy)return;
        m_5asBusy=true;
        set5AsStatus(16903);
        update5AsButtons();
        QMetaObject::invokeMethod(m_5asWorker,"readStatus",
                                  Qt::QueuedConnection);
    }

    void toggle5AsLive()
    {
        if(!m_5asConnected)return;
        m_5asLiveActive=!m_5asLiveActive;
        if(m_5asLiveActive){
            m_5asLiveTimer->start();
            if(!m_5asBusy)read5As();
        }else m_5asLiveTimer->stop();
        update5AsButtons();
    }

    void test5AsOutput(int output)
    {
        if(!m_5asConnected || m_5asBusy ||
           (output!=8 && output!=9 && output!=13))return;
        // This physical output may operate audible or visible vehicle
        // equipment. Explain the consequence and require an affirmative
        // choice BEFORE queueing any diagnostic test command.
        const int warningKey=output==8?16930:(output==9?16931:16932);
        QMessageBox confirmation(QMessageBox::Warning,I18n::text(16929),
                                 I18n::text(warningKey),
                                 QMessageBox::Ok|QMessageBox::Cancel,this);
        confirmation.setDefaultButton(QMessageBox::Cancel);
        confirmation.button(QMessageBox::Ok)->setText(I18n::text(16933));
        confirmation.button(QMessageBox::Cancel)->setText(I18n::text(16934));
        if(confirmation.exec()!=QMessageBox::Ok)return;
        m_5asBusy=true;
        update5AsButtons();
        QMetaObject::invokeMethod(m_5asWorker,"pulseOutput",
                                  Qt::QueuedConnection,Q_ARG(int,output));
    }

    void save5AsLog()
    {
        if(m_5asTrace->toPlainText().isEmpty())return;
        const QString path=QFileDialog::getSaveFileName(
            this,I18n::text(16935),QStringLiteral("Lucas5AS-Diagnostic.log"),
            I18n::text(16936));
        if(path.isEmpty())return;
        QFile file(path);
        const QByteArray content=m_5asTrace->toPlainText().toUtf8();
        if(!file.open(QIODevice::WriteOnly|QIODevice::Truncate|QIODevice::Text) ||
           file.write(content)!=content.size()){
            set5AsStatus(16938);
            return;
        }
        file.close();
        set5AsStatus(16937,path);
    }

    void setStatus(int key,const QString &arg=QString())
    {
        m_statusKey=key;
        m_statusArg=arg;
        updateStatus();
    }
    void updateStatus()
    {
        const QString translated=I18n::text(m_statusKey);
        m_status->setText(m_statusArg.isEmpty()?translated:translated.arg(m_statusArg));
    }
    void updatePortLabels()
    {
        m_portLabel->setText(m_port.isEmpty()?QString():I18n::text(16823).arg(m_port));
        m_rawLabel->setText(m_raw.isEmpty()?QString():I18n::text(16824).arg(m_raw));
    }
    void updateButtons()
    {
        m_connect->setEnabled(!m_connected && !m_busy);
        m_disconnect->setEnabled(m_connected || m_busy);
        m_read->setEnabled(m_connected && !m_busy);
        m_clear->setEnabled(m_connected && !m_busy);
    }
    void showFaults()
    {
        m_table->setRowCount(m_faults.size());
        for(int row=0;row<m_faults.size();++row){
            const QVariantMap info=m_faults.at(row).toMap();
            const int id=info.value(QStringLiteral("code")).toInt();
            const int key=info.value(QStringLiteral("descriptionKey")).toInt();
            const QString code=QStringLiteral("%1").arg(id,4,16,QLatin1Char('0')).toUpper();
            const QString state=I18n::text(
                info.value(QStringLiteral("known")).toBool()
                ?(info.value(QStringLiteral("historical")).toBool()?16832:16831)
                :16833);
            const QString details=I18n::text(key);
            const QStringList values={code,state,details};
            for(int col=0;col<3;++col){
                QTableWidgetItem *cell=new QTableWidgetItem(values.at(col));
                cell->setToolTip(values.at(col));
                m_table->setItem(row,col,cell);
            }
        }
    }
    void connectRc5()
    {
        if(m_busy || m_connected)return;
        const QList<DetectedSerialAdapter> adapters=
            SerialAdapterDetector::availableAdapters();
        QStringList ports;
        for(const DetectedSerialAdapter &adapter:adapters)
            if(adapter.detectedBySystem && !adapter.qtPortName.isEmpty() &&
               !ports.contains(adapter.qtPortName))
                ports.append(adapter.qtPortName);
        if(ports.isEmpty()){
            setStatus(16825);
            return;
        }
        m_faults.clear();
        m_raw.clear();
        m_port.clear();
        showFaults();
        updatePortLabels();
        m_cancelRequested=false;
        m_busy=true;
        m_worker->allowRequests();
        setStatus(16810);
        updateButtons();
        QMetaObject::invokeMethod(m_worker,"connectCandidates",Qt::QueuedConnection,
                                  Q_ARG(QStringList,ports));
    }
    void disconnectRc5()
    {
        if(!m_busy && !m_connected)return;
        m_cancelRequested=true;
        m_worker->requestCancel();
        m_busy=true;
        updateButtons();
        QMetaObject::invokeMethod(m_worker,"disconnectModule",Qt::QueuedConnection);
    }
    void readRc5()
    {
        if(!m_connected || m_busy)return;
        m_busy=true;
        setStatus(16813);
        updateButtons();
        QMetaObject::invokeMethod(m_worker,"readFaults",Qt::QueuedConnection);
    }
    void clearRc5()
    {
        if(!m_connected || m_busy)return;
        m_busy=true;
        setStatus(16817);
        updateButtons();
        QMetaObject::invokeMethod(m_worker,"clearFaults",Qt::QueuedConnection);
    }

    QFrame *m_panels[3]={nullptr,nullptr,nullptr};
    QLabel *m_headings[3]={nullptr,nullptr,nullptr};
    QLabel *m_placeholders[3]={nullptr,nullptr,nullptr};
    QFrame *m_dividers[3]={nullptr,nullptr,nullptr};
    QPushButton *m_cableHelpButtons[3]={nullptr,nullptr,nullptr};
    QLabel *m_status=nullptr;
    QLabel *m_portLabel=nullptr;
    QLabel *m_rawLabel=nullptr;
    QLabel *m_cableNote=nullptr;
    QPushButton *m_connect=nullptr;
    QPushButton *m_disconnect=nullptr;
    QPushButton *m_read=nullptr;
    QPushButton *m_clear=nullptr;
    QTableWidget *m_table=nullptr;
    QThread m_rc5Thread;
    Rc5Worker *m_worker=nullptr;
    QVariantList m_faults;
    QString m_port,m_raw,m_statusArg;
    int m_statusKey=16809;
    bool m_connected=false;
    bool m_busy=false;
    bool m_cancelRequested=false;

    // Lucas 5AS owns its independent thread/serial port/diagnostic workspace.
    QThread m_5asThread;
    Lucas5AsWorker *m_5asWorker=nullptr;
    QLabel *m_5asStatus=nullptr;
    QLabel *m_5asPortLabel=nullptr;
    QLabel *m_5asCableNote=nullptr;
    QPushButton *m_5asConnect=nullptr;
    QPushButton *m_5asDisconnect=nullptr;
    QPushButton *m_5asRead=nullptr;
    QPushButton *m_5asLive=nullptr;
    QPushButton *m_5asHorn=nullptr;
    QPushButton *m_5asLed=nullptr;
    QPushButton *m_5asHazards=nullptr;
    QPushButton *m_5asSaveLog=nullptr;
    QTimer *m_5asLiveTimer=nullptr;
    QTableWidget *m_5asTable=nullptr;
    QPlainTextEdit *m_5asTrace=nullptr;
    QVariantMap m_5asFields;
    QString m_5asPort,m_5asStatusArg;
    int m_5asStatusKey=16809;
    bool m_5asConnected=false;
    bool m_5asBusy=false;
    bool m_5asCancelRequested=false;
    bool m_5asLiveActive=false;
};
#endif // OTHERCONTROLUNITSTAB_H
