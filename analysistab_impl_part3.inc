    mainRow->addWidget(m_stackScrollArea,1);

    m_overlayChart=new ChartWidget(this);m_overlayChart->setObjectName(QStringLiteral("analysisOverlayChart"));m_overlayChart->hide();mainRow->addWidget(m_overlayChart,1);

    QWidget *rightPanel=new QWidget(this);rightPanel->setObjectName(QStringLiteral("analysisRightPanel"));rightPanel->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Expanding);
    QVBoxLayout *right=new QVBoxLayout(rightPanel);right->setContentsMargins(7,7,7,7);right->setSpacing(5);
    right->addWidget(m_loadButton);
    right->addWidget(m_fileLabel);
    right->addWidget(m_diagnosticLiveButton);
    right->addWidget(m_injectionLiveButton);
    QLabel *channels=new QLabel(I18n::text(6447),rightPanel);channels->setObjectName(QStringLiteral("analysisParametersTitle"));right->addWidget(channels);
    m_checkboxScrollArea=new QScrollArea(rightPanel);m_checkboxScrollArea->setWidgetResizable(true);m_checkboxContainer=new QWidget();m_checkboxLayout=new QVBoxLayout(m_checkboxContainer);m_checkboxLayout->setContentsMargins(4,4,4,4);m_checkboxLayout->addStretch();m_checkboxScrollArea->setWidget(m_checkboxContainer);right->addWidget(m_checkboxScrollArea,1);
    QLabel *tools=new QLabel(I18n::text(7100),rightPanel);tools->setObjectName(QStringLiteral("analysisToolsTitle"));right->addWidget(tools);
    QHBoxLayout *selectRow=new QHBoxLayout();selectRow->setSpacing(5);selectRow->addWidget(m_selectAllButton);selectRow->addWidget(m_selectNoneButton);right->addLayout(selectRow);right->addWidget(m_overlayButton);
    mainRow->addWidget(rightPanel,0);

    connect(m_loadButton,&QPushButton::clicked,this,&AnalysisTab::onLoadFileClicked);
    connect(m_selectAllButton,&QPushButton::clicked,this,&AnalysisTab::onSelectAllClicked);
    connect(m_selectNoneButton,&QPushButton::clicked,this,&AnalysisTab::onSelectNoneClicked);
    connect(m_overlayButton,&QPushButton::toggled,this,&AnalysisTab::onOverlayToggled);
    connect(m_diagnosticLiveButton,&QPushButton::clicked,this,&AnalysisTab::onDiagnosticLiveClicked);
    connect(m_injectionLiveButton,&QPushButton::clicked,this,&AnalysisTab::onInjectionLiveClicked);

    setDiagnosticLiveAvailable(false);
    setInjectionLiveAvailable(false,I18n::text(7922));
}

void AnalysisTab::setLiveButtons(bool diagnostic,bool injection)
{
    const bool exclusive=m_liveButtonGroup->exclusive();m_liveButtonGroup->setExclusive(false);
    {QSignalBlocker b1(m_diagnosticLiveButton);QSignalBlocker b2(m_injectionLiveButton);m_diagnosticLiveButton->setChecked(diagnostic);m_injectionLiveButton->setChecked(injection);}
    m_liveButtonGroup->setExclusive(exclusive);
}

void AnalysisTab::setDiagnosticLiveAvailable(bool available)
{
    m_diagnosticLiveButton->setEnabled(available);
    if(!available && diagnosticLiveSelected()) setLiveButtons(false,false);
    else if(available && diagnosticLiveSelected()) setLiveButtons(true,false);
}

void AnalysisTab::setInjectionLiveAvailable(bool available,const QString &reason)
{
    Q_UNUSED(reason);
    m_injectionLiveButton->setEnabled(available);
    m_injectionLiveButton->setToolTip(QString());
    if(!available && injectionLiveSelected()){
        if(m_diagnosticLiveButton->isEnabled()) selectDiagnosticLive(true);
        else setLiveButtons(false,false);
    }
}

void AnalysisTab::selectDiagnosticLive(bool notify)
{
    if(!m_diagnosticLiveButton->isEnabled()) return;
    if(m_liveSource!=DiagnosticLiveSource) prepareDiagnosticLive();
    m_liveSource=DiagnosticLiveSource;setProperty("analysisSourceFile",QString());setLiveButtons(true,false);
    m_fileLabel->setText(I18n::text(7918));
    if(notify) emit liveSourceRequested(false);
}

void AnalysisTab::selectInjectionLive(bool notify)
{
    if(!m_injectionLiveButton->isEnabled()) return;
    if(m_liveSource!=InjectionLiveSource) prepareInjectionLive();
    m_liveSource=InjectionLiveSource;setProperty("analysisSourceFile",QString());setLiveButtons(false,true);
    m_fileLabel->setText(I18n::text(7919));
    if(notify) emit liveSourceRequested(true);
}

void AnalysisTab::onDiagnosticLiveClicked(){selectDiagnosticLive(true);}
void AnalysisTab::onInjectionLiveClicked(){selectInjectionLive(true);}

void AnalysisTab::prepareDiagnosticLive()
{
    m_time.clear();m_columns.clear();m_columnNames.clear();
    const QStringList raw=diagnosticRawNames();
    for(const QString &s:raw)m_columnNames<<friendlyColumnName(s);
    m_columns.resize(m_columnNames.size());m_liveClock.restart();rebuildCheckboxes();
}

void AnalysisTab::prepareInjectionLive()
{
    m_time.clear();m_columns.clear();
    m_columnNames=QStringList()<<I18n::text(7901)<<I18n::text(7902)<<I18n::text(7903)<<I18n::text(7904)<<I18n::text(7920)<<I18n::text(7921);
    m_columns.resize(m_columnNames.size());m_liveClock.restart();rebuildCheckboxes();
}

void AnalysisTab::appendDiagnosticData(const mems_data *data)
{
    if(m_liveSource!=DiagnosticLiveSource||!data)return;
    appendLiveRow(diagnosticValues(data));
}

void AnalysisTab::appendInjectionSample(double finalMs,double baseMs,double transientMs,
                                        quint16 baseRaw,quint16 transientRaw,quint8 transientCounter)
{
    if(m_liveSource!=InjectionLiveSource)return;
    QVector<double> v;v<<finalMs<<baseMs<<transientMs<<double(transientCounter)<<double(baseRaw)<<double(transientRaw);appendLiveRow(v);
}

void AnalysisTab::appendLiveRow(const QVector<double> &values)
{
    if(values.size()!=m_columns.size())return;
    if(!m_liveClock.isValid())m_liveClock.start();
    const double now=m_liveClock.elapsed()/1000.0;m_time.append(now);
    for(int i=0;i<values.size();++i)m_columns[i].append(values[i]);
    const double cutoff=now-120.0;
    while(!m_time.isEmpty()&&m_time.first()<cutoff){m_time.remove(0);for(QVector<double> &c:m_columns)if(!c.isEmpty())c.remove(0);}
    refreshLiveCharts();
}

void AnalysisTab::refreshLiveCharts()
{
    m_overlayChart->setData(m_time,m_columns,m_columnNames,m_colors);
    for(auto it=m_stackedCharts.begin();it!=m_stackedCharts.end();++it){int i=it.key();if(i>=0&&i<m_columns.size())it.value()->setData(m_time,m_columns[i]);}
    updateChartVisibility();
}

void AnalysisTab::onLoadFileClicked()
{
    const QString path=QFileDialog::getOpenFileName(this,I18n::text(6448),"logs",I18n::text(6449));
    if(!path.isEmpty())loadFile(path);
}

void AnalysisTab::loadFile(const QString &path){parseCsv(path);}

void AnalysisTab::parseCsv(const QString &path)
{
    QFile file(path);if(!file.open(QFile::ReadOnly|QFile::Text)){QMessageBox::warning(this,I18n::text(6450),I18n::text(6451).arg(path));return;}
    QTextStream stream(&file);if(!stream.atEnd())stream.readLine();
    if(stream.atEnd()){QMessageBox::warning(this,I18n::text(6452),I18n::text(6453));return;}
    QStringList headers=stream.readLine().split(',');if(!headers.isEmpty()&&headers[0].startsWith('#'))headers[0]=headers[0].mid(1);
    const int columnCount=headers.size()-1;if(columnCount<=0){QMessageBox::warning(this,I18n::text(6454),I18n::text(6455));return;}
    m_columnNames.clear();for(int i=1;i<headers.size();++i)m_columnNames<<friendlyColumnName(headers[i].trimmed());
    m_time.clear();m_columns.clear();m_columns.resize(columnCount);QTime firstTime;int lines=0;
    while(!stream.atEnd()){
        const QString line=stream.readLine();if(line.trimmed().isEmpty())continue;const QStringList fields=line.split(',');if(fields.size()<2)continue;
        QTime t=QTime::fromString(fields[0],"hh:mm:ss");double sec=lines;if(t.isValid()){if(!firstTime.isValid())firstTime=t;sec=firstTime.secsTo(t);if(sec<0)sec+=86400;}
        m_time.append(sec);for(int c=0;c<columnCount;++c)m_columns[c].append(c+1<fields.size()?fields[c+1].trimmed().toDouble():0.0);++lines;
    }
    file.close();if(!lines){QMessageBox::warning(this,I18n::text(6456),I18n::text(6457));return;}
    m_liveSource=FileSource;setProperty("analysisSourceFile",QFileInfo(path).fileName());setLiveButtons(false,false);emit liveSourceRequested(false);
    m_fileLabel->setText(QFileInfo(path).fileName()+QString(" (%1 points)").arg(lines));m_overlayChart->setData(m_time,m_columns,m_columnNames,m_colors);rebuildCheckboxes();
}

void AnalysisTab::rebuildCheckboxes()
{
    for(QCheckBox *cb:m_checkboxes){m_checkboxLayout->removeWidget(cb);delete cb;}m_checkboxes.clear();
    qDeleteAll(m_stackedCharts);m_stackedCharts.clear();
    QLayoutItem *stretch=nullptr;if(m_checkboxLayout->count()>0)stretch=m_checkboxLayout->takeAt(m_checkboxLayout->count()-1);
    for(int i=0;i<m_columnNames.size();++i){
        QCheckBox *cb=new QCheckBox(m_columnNames[i],m_checkboxContainer);const QColor c=m_colors[i%m_colors.size()];
        cb->setStyleSheet(QString("QCheckBox{padding:3px 0;}QCheckBox::indicator{width:13px;height:13px;border:1px solid %1;border-radius:2px;background:#0b1015;}QCheckBox::indicator:checked{background:%1;}").arg(c.name()));
        bool checked=false;
        if(m_liveSource==InjectionLiveSource)checked=(i<3);
        else checked=m_columnNames[i].contains(I18n::text(6458),Qt::CaseInsensitive)||m_columnNames[i].contains(I18n::text(6459),Qt::CaseInsensitive)||m_columnNames[i].contains(I18n::text(6460),Qt::CaseInsensitive);