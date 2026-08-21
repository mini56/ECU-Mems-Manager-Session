        m_firmware=new QLabel(raw);m_baseRaw=new QLabel(raw);m_transientRaw=new QLabel(raw);m_counterRaw=new QLabel(raw);m_mode=new QLabel(raw);
        for(QLabel *l:{m_firmware,m_baseRaw,m_transientRaw,m_counterRaw,m_mode}){l->setStyleSheet(QStringLiteral("border-bottom:1px solid #25313a;padding:4px 2px;"));rv->addWidget(l);}
        rv->addStretch();bottom->addWidget(raw,1);root->addLayout(bottom,1);
        m_clock.start();m_modeText=I18n::text(7912);clearValues();
    }

    void setMapping(const QString &firmware,bool available,quint16 baseAddr=0,quint16 transientAddr=0,quint16 counterAddr=0)
    {
        m_firmwareId=firmware;m_available=available;m_baseAddr=baseAddr;m_transientAddr=transientAddr;m_counterAddr=counterAddr;
        m_firmware->setText(I18n::text(7916).arg(firmware.isEmpty()?QStringLiteral("--"):firmware));
        if(available){
            m_base->setTitle(I18n::text(7923).arg(hexAddress(baseAddr)));
            m_transient->setTitle(I18n::text(7924).arg(hexAddress(transientAddr)));
            m_counter->setTitle(I18n::text(7925).arg(hexAddress(counterAddr)));
            m_status->setText(I18n::text(7917));m_status->setStyleSheet(QStringLiteral("color:#65db79;"));
        } else {
            m_status->setText(I18n::text(7922));m_status->setStyleSheet(QStringLiteral("color:#ff9828;"));
        }
        refreshRaw();
    }

    void setReadMode(const QString &state)
    {
        if(state==QStringLiteral("diagnostic"))m_modeText=I18n::text(7910);
        else if(state==QStringLiteral("injection"))m_modeText=I18n::text(7911);
        else if(state==QStringLiteral("transition"))m_modeText=I18n::text(7926);
        else if(state==QStringLiteral("rom"))m_modeText=I18n::text(7927);
        else if(state==QStringLiteral("mode3"))m_modeText=I18n::text(7928);
        else m_modeText=I18n::text(7912);
        refreshRaw();
    }

    void clearValues()
    {
        m_finalValue=m_baseValue=m_transientValue=0.0;m_counterValue=0;m_baseRawValue=m_transientRawValue=0;
        m_time.clear();m_finalHistory.clear();m_final->setValue(0);m_base->setValue(0);m_transient->setValue(0);m_counter->setValue(0);if(m_history)m_history->setData(m_time,m_finalHistory);refreshRaw();
    }

    void appendSample(double finalMs,double baseMs,double transientMs,quint16 baseRaw,quint16 transientRaw,quint8 counter)
    {
        m_finalValue=finalMs;m_baseValue=baseMs;m_transientValue=transientMs;m_counterValue=counter;m_baseRawValue=baseRaw;m_transientRawValue=transientRaw;
        m_final->setValue(finalMs);m_base->setValue(baseMs);m_transient->setValue(transientMs);m_counter->setValue(counter);
        const double now=m_clock.elapsed()/1000.0;m_time.append(now);m_finalHistory.append(finalMs);const double cutoff=now-120.0;while(!m_time.isEmpty()&&m_time.first()<cutoff){m_time.remove(0);m_finalHistory.remove(0);}m_history->setData(m_time,m_finalHistory);refreshRaw();
    }

private:
    void refreshRaw()
    {
        if(!m_firmware)return;
        m_firmware->setText(I18n::text(7916).arg(m_firmwareId.isEmpty()?QStringLiteral("--"):m_firmwareId));
        m_baseRaw->setText(I18n::text(7913).arg(m_baseAddr?hexAddress(m_baseAddr):QStringLiteral("----")).arg(m_baseRawValue));
        m_transientRaw->setText(I18n::text(7914).arg(m_transientAddr?hexAddress(m_transientAddr):QStringLiteral("----")).arg(m_transientRawValue));
        m_counterRaw->setText(I18n::text(7915).arg(m_counterAddr?hexAddress(m_counterAddr):QStringLiteral("----")).arg(m_counterValue));
        m_mode->setText(I18n::text(7909).arg(m_modeText.isEmpty()?I18n::text(7912):m_modeText));
    }
    QLabel *m_status=nullptr,*m_firmware=nullptr,*m_baseRaw=nullptr,*m_transientRaw=nullptr,*m_counterRaw=nullptr,*m_mode=nullptr;
    InjectionGaugeCard *m_final=nullptr,*m_base=nullptr,*m_transient=nullptr,*m_counter=nullptr;InjectionHistoryWidget *m_history=nullptr;
    QElapsedTimer m_clock;QVector<double> m_time,m_finalHistory;QString m_firmwareId,m_modeText;bool m_available=false;quint16 m_baseAddr=0,m_transientAddr=0,m_counterAddr=0,m_baseRawValue=0,m_transientRawValue=0;quint8 m_counterValue=0;double m_finalValue=0,m_baseValue=0,m_transientValue=0;
};

static bool responseMode(const QByteArray &response,quint8 *mode)
{
    return decodeKnownModeReply(response,mode);
}

static void stopMappedInjectionDirect(MEMSInterface *mems)
{
    if(!mems)return;
    QMetaObject::invokeMethod(mems,"onMappedInjectionStopRequested",Qt::DirectConnection);
}

static void startMappedInjection(MEMSInterface *mems,const QString &firmware)
{
    if(!mems||firmware.trimmed().isEmpty())return;
    // Arm the desired state immediately in a thread-safe atomic before the
    // worker receives the queued start. A rapid tab change can then cancel a
    // not-yet-processed start and cannot leave a stale Mode-4 request queued.
    QMetaObject::invokeMethod(mems,"onMappedInjectionRequestState",Qt::DirectConnection,Q_ARG(bool,true));
    QMetaObject::invokeMethod(mems,"onMappedInjectionModeRequested",Qt::QueuedConnection,
                              Q_ARG(bool,true),Q_ARG(QString,firmware));
}

class InjectionModeController : public QObject
{
public:
    InjectionModeController(MainWindow *window,MEMSInterface *mems,QTabWidget *tabs,AnalysisTab *analysis,InjectionDashboard *dashboard,QWidget *injectionTab)
        : QObject(window),m_window(window),m_mems(mems),m_tabs(tabs),m_analysis(analysis),m_dashboard(dashboard),m_injectionTab(injectionTab),m_previous(tabs?tabs->currentWidget():nullptr)
    {
        if(!m_window||!m_mems||!m_tabs||!m_analysis||!m_dashboard)return;
        m_window->setProperty("ecuReadMode",QStringLiteral("disconnected"));m_dashboard->setReadMode(QStringLiteral("disconnected"));
        m_analysis->setDiagnosticLiveAvailable(false);

        QObject::connect(m_mems,&MEMSInterface::connected,m_window,[this](){m_analysis->setDiagnosticLiveAvailable(true);updateFirmware(QString());m_window->setProperty("ecuReadMode",QStringLiteral("diagnostic"));m_dashboard->setReadMode(QStringLiteral("diagnostic"));updateStatusSoon();});
        QObject::connect(m_mems,&MEMSInterface::disconnected,m_window,[this](){m_analysis->setDiagnosticLiveAvailable(false);updateFirmware(QString());m_window->setProperty("ecuReadMode",QStringLiteral("disconnected"));m_dashboard->setReadMode(QStringLiteral("disconnected"));updateStatusSoon();});
        QObject::connect(m_mems,&MEMSInterface::readModeChanged,m_window,[this](const QString &mode){m_window->setProperty("ecuReadMode",mode);m_dashboard->setReadMode(mode);updateStatusSoon();});
        QObject::connect(m_mems,&MEMSInterface::dataReady,m_window,[this](){m_analysis->appendDiagnosticData(m_mems->getData());});
        QObject::connect(m_mems,&MEMSInterface::injectionLiveSample,m_window,[this](double finalMs,double baseMs,double transientMs,quint16 baseRaw,quint16 transientRaw,quint8 counter){
            m_window->setProperty("lastInjectionMs",finalMs);m_dashboard->appendSample(finalMs,baseMs,transientMs,baseRaw,transientRaw,counter);m_analysis->appendInjectionSample(finalMs,baseMs,transientMs,baseRaw,transientRaw,counter);
        });
        QObject::connect(m_mems,&MEMSInterface::protocolResponse,m_window,[this](quint8 command,const QByteArray &response){
            if(command==quint8(0xD1)){
                const QString id=EcuIdentification::extendedIdentifierFromReply(response);if(!id.isEmpty()){m_window->setProperty("ecuExtendedIdentifier",id);updateFirmware(id);}
            } else if(command==quint8(0xF0)){
                quint8 mode=0;if(responseMode(response,&mode)){
                    if(mode==0x50)m_window->setProperty("ecuReadMode",QStringLiteral("diagnostic"));
                    else if(mode==0x1E)m_window->setProperty("ecuReadMode",QStringLiteral("injection"));
                    else if(mode==0x14)m_window->setProperty("ecuReadMode",QStringLiteral("mode3"));
                    m_dashboard->setReadMode(m_window->property("ecuReadMode").toString());updateStatusSoon();
                }
            }
        });
        QObject::connect(m_analysis,&AnalysisTab::liveSourceRequested,m_window,[this](bool injection){
            if(injection){if(m_supported)startMappedInjection(m_mems,m_firmware);}else stopMappedInjectionDirect(m_mems);
        });
        QObject::connect(m_tabs,&QTabWidget::currentChanged,m_window,[this](int index){onTabChanged(index);});

        updateFirmware(m_window->property("ecuExtendedIdentifier").toString());
        QTimer *probe=new QTimer(this);probe->setInterval(250);QObject::connect(probe,&QTimer::timeout,this,[this,probe](){if(bindStatusBar()){probe->stop();probe->deleteLater();}});probe->start();
        QTimer::singleShot(20,this,[this](){bindStatusBar();});
    }

private:
    void updateFirmware(const QString &firmware)
    {
        m_firmware=firmware.trimmed().toUpper();InjectionRamMap map; m_supported=injectionRamMapForFirmware(m_firmware,&map);
        if(m_supported){m_map=map;m_dashboard->setMapping(m_firmware,true,map.baseAddress,map.transientSourceAddress,map.transientCounterAddress);m_analysis->setInjectionLiveAvailable(true);}
        else {m_dashboard->setMapping(m_firmware,false);m_analysis->setInjectionLiveAvailable(false,I18n::text(7922));}
    }

    void onTabChanged(int index)
    {