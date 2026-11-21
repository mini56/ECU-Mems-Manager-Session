#ifndef OTHERCONTROLUNITSTAB_H
#define OTHERCONTROLUNITSTAB_H

// SESSION 187: first of three independent workspaces is RC5 SRS diagnostics.
// Immobiliser and key-programming columns remain intentionally unconnected.
#include <QApplication>
#include <QFrame>
#include <QGridLayout>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QMetaObject>
#include <QPushButton>
#include <QResizeEvent>
#include <QSerialPortInfo>
#include <QSizePolicy>
#include <QString>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QThread>
#include <QVariantList>
#include <QVBoxLayout>
#include <QWidget>
#include "i18n.h"
#include "rc5protocol.h"
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

        retranslate();
        refreshTheme();
        updateButtons();
    }

    ~OtherControlUnitsTab() override
    {
        if(m_worker)m_worker->requestCancel();
        m_rc5Thread.quit();
        m_rc5Thread.wait();
    }

    void retranslate()
    {
        for(int i=0;i<3;++i){
            m_headings[i]->setText(I18n::text(16801+i));
            if(m_placeholders[i])
                m_placeholders[i]->setText(I18n::text(16804));
        }
        m_connect->setText(I18n::text(16805));
        m_disconnect->setText(I18n::text(16806));
        m_read->setText(I18n::text(16807));
        m_clear->setText(I18n::text(16808));
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
        for(QLabel *label:{m_status,m_portLabel,m_rawLabel,m_cableNote})
            label->setStyleSheet(
                QStringLiteral("color:%1;background:transparent;border:0;")
                    .arg(label==m_status?foreground:muted));
        const QString buttonStyle=QStringLiteral(
            "QPushButton{background:%1;color:%2;border:1px solid %2;"
            "border-radius:4px;padding:4px 3px;}"
            "QPushButton:hover:enabled{background:%3;}"
            "QPushButton:disabled{color:%4;border:1px solid %4;background:%1;}"
        ).arg(dark?QStringLiteral("#090e13"):QStringLiteral("#ffffff"),
              highlight,
              dark?QStringLiteral("#19130a"):QStringLiteral("#fff0dd"),
              dark?QStringLiteral("#5d6870"):QStringLiteral("#aab4be"));
        for(QPushButton *button:{m_connect,m_disconnect,m_read,m_clear})
            button->setStyleSheet(buttonStyle);
        m_table->setStyleSheet(
            QStringLiteral("QTableWidget{background:%1;color:%2;"
                           "gridline-color:%3;border:1px solid %3;}"
                           "QHeaderView::section{background:%1;color:%4;"
                           "border:0;border-bottom:1px solid %3;padding:3px;}")
                .arg(surface,foreground,border,highlight));
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
};
#endif // OTHERCONTROLUNITSTAB_H
