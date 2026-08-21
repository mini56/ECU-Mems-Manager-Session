        if(index<0||index>=m_tabs->count())return;QWidget *target=m_tabs->widget(index);
        const bool wasInjection=(m_previous==m_injectionTab)||(m_previous==m_analysis&&m_analysis->injectionLiveSelected());
        if(target==m_injectionTab){
            if(m_supported)startMappedInjection(m_mems,m_firmware);
        } else if(target==m_analysis){
            if(wasInjection&&m_supported)m_analysis->selectInjectionLive(false);
            else if(!m_analysis->fileSourceSelected())m_analysis->selectDiagnosticLive(false);
            if(m_analysis->injectionLiveSelected()&&m_supported)startMappedInjection(m_mems,m_firmware);else stopMappedInjectionDirect(m_mems);
        } else {
            if(wasInjection||m_window->property("ecuReadMode").toString()==QStringLiteral("injection")||m_window->property("ecuReadMode").toString()==QStringLiteral("transition"))stopMappedInjectionDirect(m_mems);
        }
        m_previous=target;updateStatusSoon();
    }

    bool bindStatusBar()
    {
        if(m_statusBound)return true;
        QFrame *bar=m_window->findChild<QFrame*>(QStringLiteral("uiRebuildStatus"));if(!bar)return false;
        if(!bar->findChild<QLabel*>(QStringLiteral("statusFile"))||
           !bar->findChild<QLabel*>(QStringLiteral("statusReadMode"))||
           !bar->findChild<QLabel*>(QStringLiteral("statusPort"))||
           !bar->findChild<QLabel*>(QStringLiteral("statusFirmware")))return false;

        m_statusBound=true;
        QTimer *timer=new QTimer(this);timer->setInterval(500);
        QObject::connect(timer,&QTimer::timeout,this,[this](){refreshStatusBar();});
        timer->start();
        refreshStatusBar();
        return true;
    }

    void updateStatusSoon(){QTimer::singleShot(0,m_window,[this](){if(m_statusBound)refreshStatusBar();});}

    void refreshStatusBar()
    {
        QLabel *file=m_window->findChild<QLabel*>(QStringLiteral("statusFile"));
        QLabel *mode=m_window->findChild<QLabel*>(QStringLiteral("statusReadMode"));
        QLabel *port=m_window->findChild<QLabel*>(QStringLiteral("statusPort"));
        QLabel *firmware=m_window->findChild<QLabel*>(QStringLiteral("statusFirmware"));
        if(!file||!mode||!port||!firmware)return;

        QString fileName;
        const bool analysisFile=(m_tabs->currentWidget()==m_analysis)&&m_analysis->fileSourceSelected();
        if(analysisFile)fileName=m_analysis->property("analysisSourceFile").toString().trimmed();
        if(fileName.isEmpty())if(QLineEdit *f=m_window->findChild<QLineEdit*>(QStringLiteral("m_logFileNameBox")))fileName=f->text().trimmed();
        if(fileName.isEmpty())fileName=QStringLiteral("--");
        file->setText(I18n::text(7115).arg(fileName));

        QString portName=m_mems?m_mems->getSerialDevice().trimmed():QString();
        const QString windowsPrefix=QStringLiteral("\\\\.\\");
        if(portName.startsWith(windowsPrefix,Qt::CaseInsensitive))portName.remove(0,windowsPrefix.size());
        if(portName.isEmpty())portName=QStringLiteral("--");
        port->setText(QStringLiteral("%1 : %2").arg(I18n::text(7114),portName));
        firmware->setText(I18n::text(7916).arg(m_firmware.isEmpty()?QStringLiteral("--"):m_firmware));

        const QString state=m_window->property("ecuReadMode").toString();
        const bool fileMode=analysisFile&&!m_analysis->property("analysisSourceFile").toString().trimmed().isEmpty();
        QString modeName=I18n::text(7912);
        if(fileMode)modeName=I18n::text(7933);
        else if(state==QStringLiteral("diagnostic"))modeName=I18n::text(7910);
        else if(state==QStringLiteral("injection"))modeName=I18n::text(7911);
        else if(state==QStringLiteral("transition"))modeName=I18n::text(7926);
        else if(state==QStringLiteral("rom"))modeName=I18n::text(7927);
        else if(state==QStringLiteral("mode3"))modeName=I18n::text(7928);
        mode->setText(I18n::text(7909).arg(modeName));

        const bool expectedInjection=(m_tabs->currentWidget()==m_injectionTab)||(m_tabs->currentWidget()==m_analysis&&m_analysis->injectionLiveSelected());
        const bool mismatch=!fileMode&&((state==QStringLiteral("injection")&&!expectedInjection)||(state==QStringLiteral("diagnostic")&&expectedInjection));
        QString color=QStringLiteral("#c9d1d7");
        if(state==QStringLiteral("transition"))color=QStringLiteral("#ff9828");
        else if(mismatch)color=QStringLiteral("#ff5b55");
        else if(fileMode||state==QStringLiteral("diagnostic")||state==QStringLiteral("injection")||state==QStringLiteral("rom")||state==QStringLiteral("mode3"))color=QStringLiteral("#65db79");
        else if(state==QStringLiteral("disconnected")||state.isEmpty())color=QStringLiteral("#8d99a3");
        mode->setStyleSheet(QStringLiteral("color:%1;background:transparent;border-right:1px solid #25313a;padding:0 10px;font-weight:700;").arg(color));
    }

    MainWindow *m_window=nullptr;MEMSInterface *m_mems=nullptr;QTabWidget *m_tabs=nullptr;AnalysisTab *m_analysis=nullptr;InjectionDashboard *m_dashboard=nullptr;QWidget *m_injectionTab=nullptr,*m_previous=nullptr;QString m_firmware;InjectionRamMap m_map{0,0,0};bool m_supported=false,m_statusBound=false;
};

class MappedInjectionInstaller : public QObject
{
public:
    explicit MappedInjectionInstaller(QObject *parent=nullptr):QObject(parent){}
protected:
    bool eventFilter(QObject *watched,QEvent *event) override
    {
        if(!watched||(event->type()!=QEvent::Show&&event->type()!=QEvent::Polish))return QObject::eventFilter(watched,event);
        MainWindow *window=qobject_cast<MainWindow*>(watched);if(!window||window->objectName()!=QStringLiteral("MainWindow")||window->property("mappedInjectionUiInstalled").toBool()||!window->memsInterface())return QObject::eventFilter(watched,event);
        QTabWidget *tabs=window->findChild<QTabWidget*>(QStringLiteral("Tab_main"));AnalysisTab *analysis=window->findChild<AnalysisTab*>();if(!tabs||!analysis)return QObject::eventFilter(watched,event);
        window->setProperty("mappedInjectionUiInstalled",true);
        QTimer::singleShot(0,window,[window,tabs,analysis](){
            InjectionDashboard *dashboard=new InjectionDashboard(tabs);tabs->insertTab(1,dashboard,I18n::text(7900));
            new InjectionModeController(window,window->memsInterface(),tabs,analysis,dashboard,dashboard);
        });
        return QObject::eventFilter(watched,event);
    }
};

static void installMappedInjectionUi()
{
    QApplication *app=qobject_cast<QApplication*>(QCoreApplication::instance());if(app)app->installEventFilter(new MappedInjectionInstaller(app));
}

} // namespace

void MEMSInterface::onMappedInjectionRequestState(bool enabled)
{
    m_mappedInjectionRequested.storeRelease(enabled?1:0);
}

void MEMSInterface::onMappedInjectionStopRequested()
{
    onMappedInjectionRequestState(false);
}

void MEMSInterface::onMappedInjectionModeRequested(bool enabled,QString firmwareIdentifier)
{
    if(!enabled){onMappedInjectionRequestState(false);return;}
    // A queued start may arrive after the user has already selected another
    // page. In that case the direct atomic cancel above wins and this stale
    // request is ignored before any ECU mode command is sent.
    if(m_mappedInjectionRequested.loadAcquire()==0)return;
    if(property("mappedInjectionModeActive").toBool())return;
    if(!(m_initComplete&&mems_is_connected(&m_memsinfo))||m_stopPolling||m_shutdownThread){onMappedInjectionRequestState(false);return;}