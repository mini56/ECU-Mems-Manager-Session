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

    bool installStatusBar()
    {
        if(m_statusInstalled)return true;QFrame *bar=m_window->findChild<QFrame*>(QStringLiteral("uiRebuildStatus"));if(!bar)return false;QHBoxLayout *h=qobject_cast<QHBoxLayout*>(bar->layout());if(!h||h->count()<6)return false;
        const char *names[]={"statusFile","statusLoop","statusLambda","statusSystem","statusDwell","statusAir"};
        for(int i=0;i<6;++i)if(QWidget *w=h->itemAt(i)->widget())w->setObjectName(QString::fromLatin1(names[i]));
        for(QTimer *t:bar->findChildren<QTimer*>(QString(),Qt::FindDirectChildrenOnly))t->stop();
        QLabel *mode=new QLabel(bar);mode->setObjectName(QStringLiteral("statusReadMode"));h->insertWidget(3,mode,1);
        m_statusInstalled=true;QTimer *timer=new QTimer(bar);timer->setInterval(300);QObject::connect(timer,&QTimer::timeout,bar,[this](){refreshStatusBar();});timer->start();refreshStatusBar();return true;
    }

    void updateStatusSoon(){QTimer::singleShot(0,m_window,[this](){if(m_statusInstalled)refreshStatusBar();});}

    void refreshStatusBar()
    {
        QLabel *file=m_window->findChild<QLabel*>(QStringLiteral("statusFile"));QLabel *loop=m_window->findChild<QLabel*>(QStringLiteral("statusLoop"));QLabel *lambda=m_window->findChild<QLabel*>(QStringLiteral("statusLambda"));QLabel *system=m_window->findChild<QLabel*>(QStringLiteral("statusSystem"));QLabel *dwell=m_window->findChild<QLabel*>(QStringLiteral("statusDwell"));QLabel *air=m_window->findChild<QLabel*>(QStringLiteral("statusAir"));QLabel *mode=m_window->findChild<QLabel*>(QStringLiteral("statusReadMode"));if(!mode)return;
        if(file)if(QLineEdit *f=m_window->findChild<QLineEdit*>(QStringLiteral("m_logFileNameBox")))file->setText(I18n::text(7115).arg(f->text()));
        const QString state=m_window->property("ecuReadMode").toString();QString modeName=I18n::text(7212);
        if(state==QStringLiteral("diagnostic"))modeName=I18n::text(7210);else if(state==QStringLiteral("injection"))modeName=I18n::text(7211);else if(state==QStringLiteral("transition"))modeName=I18n::text(7226);else if(state==QStringLiteral("rom"))modeName=I18n::text(7227);else if(state==QStringLiteral("mode3"))modeName=I18n::text(7228);
        mode->setText(I18n::text(7209).arg(modeName));
        const bool expectedInjection=(m_tabs->currentWidget()==m_injectionTab)||(m_tabs->currentWidget()==m_analysis&&m_analysis->injectionLiveSelected());
        const bool mismatch=(state==QStringLiteral("injection")&&!expectedInjection)||(state==QStringLiteral("diagnostic")&&expectedInjection);
        const QString color=state==QStringLiteral("transition")?QStringLiteral("#ff9828"):(mismatch?QStringLiteral("#ff5b55"):QStringLiteral("#65db79"));mode->setStyleSheet(QStringLiteral("color:%1;background:transparent;border-right:1px solid #25313a;padding:0 8px;font-weight:700;").arg(color));
        if(state==QStringLiteral("disconnected")){
            if(loop)loop->setText(I18n::text(7116).arg(QStringLiteral("--")));
            if(lambda)lambda->setText(I18n::text(7119).arg(QStringLiteral("--")));
            if(system)system->setText(I18n::text(7212));
            if(dwell)dwell->setText(I18n::text(7122).arg(QStringLiteral("--")));
            if(air)air->setText(I18n::text(7123).arg(QStringLiteral("--")));
        } else if(state==QStringLiteral("injection")||state==QStringLiteral("transition")){
            if(loop)loop->setText(I18n::text(7116).arg(I18n::text(7211)));
            if(lambda)lambda->setText(I18n::text(7229));
            if(system)system->setText(I18n::text(7230));
            if(dwell)dwell->setText(I18n::text(7231).arg(m_window->property("lastInjectionMs").isValid()?QString::number(m_window->property("lastInjectionMs").toDouble(),'f',2):QStringLiteral("--")));
            if(air)air->setText(I18n::text(7232).arg(m_firmware.isEmpty()?QStringLiteral("--"):m_firmware));
        } else {
            if(loop)if(QWidget *x=m_window->findChild<QWidget*>(QStringLiteral("m_closedLoopLed")))loop->setText(I18n::text(7116).arg(x->property("checked").toBool()?I18n::text(7117):I18n::text(7118)));
            if(lambda)if(QWidget *x=m_window->findChild<QWidget*>(QStringLiteral("m_lambda_voltage")))lambda->setText(I18n::text(7119).arg(x->property("value").toString()));
            if(system)if(QWidget *x=m_window->findChild<QWidget*>(QStringLiteral("m_engine_error")))system->setText(x->property("checked").toBool()?I18n::text(7120):I18n::text(7121));
            if(dwell)if(QWidget *x=m_window->findChild<QWidget*>(QStringLiteral("m_injector_time")))dwell->setText(I18n::text(7122).arg(x->property("value").toString()));
            if(air)if(QWidget *x=m_window->findChild<QWidget*>(QStringLiteral("m_airTempGauge")))air->setText(I18n::text(7123).arg(x->property("value").toString()));
        }
    }

    MainWindow *m_window=nullptr;MEMSInterface *m_mems=nullptr;QTabWidget *m_tabs=nullptr;AnalysisTab *m_analysis=nullptr;InjectionDashboard *m_dashboard=nullptr;QWidget *m_injectionTab=nullptr,*m_previous=nullptr;QString m_firmware;InjectionRamMap m_map{0,0,0};bool m_supported=false,m_statusInstalled=false;
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
            InjectionDashboard *dashboard=new InjectionDashboard(tabs);tabs->insertTab(1,dashboard,I18n::text(7200));
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
