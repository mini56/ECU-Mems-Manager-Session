    // One left-hand display: original graphs OR the selected-value table.
    // The right-side channel selection panel below is retained as-is.
    m_chartPanel=new QWidget(this);
    m_chartPanel->setObjectName(QStringLiteral("analysisChartsPanel"));
    QVBoxLayout *charts=new QVBoxLayout(m_chartPanel);
    charts->setContentsMargins(0,0,0,0);
    charts->setSpacing(0);
    charts->addWidget(m_stackScrollArea,1);
    m_overlayChart=new ChartWidget(m_chartPanel);
    m_overlayChart->setObjectName(QStringLiteral("analysisOverlayChart"));
    m_overlayChart->hide();
    charts->addWidget(m_overlayChart,1);

    QWidget *leftPanel=new QWidget(this);
    leftPanel->setObjectName(QStringLiteral("analysisLeftPanel"));
    QVBoxLayout *left=new QVBoxLayout(leftPanel);
    left->setContentsMargins(0,0,0,0);
    left->setSpacing(5);
    m_leftStack=new QStackedWidget(leftPanel);
    m_leftStack->setObjectName(QStringLiteral("analysisLeftDisplay"));
    m_leftStack->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    m_leftStack->addWidget(m_chartPanel);
    m_valuesTable=new QTableView(leftPanel);
    m_valuesTable->setObjectName(QStringLiteral("analysisValuesTable"));
    m_valuesTable->setAlternatingRowColors(true);
    m_valuesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_valuesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_valuesTable->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_valuesTable->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_valuesTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_valuesTable->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    m_valuesTable->verticalHeader()->setDefaultSectionSize(24);
    m_valuesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_valuesTable->horizontalHeader()->setDefaultSectionSize(155);
    m_valuesTable->horizontalHeader()->setStretchLastSection(false);
    m_valuesModel=new AnalysisValuesTableModel(&m_time,&m_columns,&m_columnNames,
                     &m_originalTimeTexts,&m_originalColumnTexts,this);
    m_valuesTable->setModel(m_valuesModel);
    m_valuesTable->setColumnWidth(0,120);
    m_leftStack->addWidget(m_valuesTable);
    // Third independent view: historic raw TX/RX sessions. Measurements stay
    // in the original curves/columns pages and their importer is unchanged.
    QWidget *communicationPanel=new QWidget(leftPanel);
    communicationPanel->setObjectName(QStringLiteral("analysisCommunicationPanel"));
    QVBoxLayout *communicationLayout=new QVBoxLayout(communicationPanel);
    communicationLayout->setContentsMargins(4,4,4,4);
    QHBoxLayout *communicationToolbar=new QHBoxLayout;
    m_communicationFiles=new QComboBox(communicationPanel);
    m_communicationFiles->setObjectName(QStringLiteral("analysisCommunicationFileList"));
    m_communicationFiles->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
    m_communicationOpenButton=new QPushButton(I18n::text(18027),communicationPanel);
    m_communicationOpenButton->setObjectName(QStringLiteral("analysisCommunicationOpenButton"));
    communicationToolbar->addWidget(m_communicationFiles,1);
    communicationToolbar->addWidget(m_communicationOpenButton);
    communicationLayout->addLayout(communicationToolbar);
    m_communicationText=new QPlainTextEdit(communicationPanel);
    m_communicationText->setObjectName(QStringLiteral("analysisCommunicationText"));
    m_communicationText->setReadOnly(true);
    m_communicationText->setLineWrapMode(QPlainTextEdit::NoWrap);
    m_communicationText->setPlaceholderText(I18n::text(18028));
    m_communicationText->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    communicationLayout->addWidget(m_communicationText,1);
    m_leftStack->addWidget(communicationPanel);
    left->addWidget(m_leftStack,1);
    mainRow->addWidget(leftPanel,1);
    connect(m_viewModeButton,&QPushButton::toggled,this,&AnalysisTab::onViewModeToggled);
    connect(m_curvesModeButton,&QPushButton::clicked,this,[this](){
        onViewModeToggled(false);
    });
    connect(m_communicationsModeButton,&QPushButton::clicked,this,[this](){
        refreshCommunicationFiles();
        onViewModeToggled(false);
    });
    connect(m_communicationFiles,QOverload<int>::of(&QComboBox::activated),
            this,[this](int index){
        if(index>=0)showCommunicationFile(m_communicationFiles->itemData(index).toString());
    });
    connect(m_communicationOpenButton,&QPushButton::clicked,this,[this](){
        QString docs=QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        if(docs.isEmpty())docs=QDir::homePath();
        const QString directory=QDir(docs).filePath(QStringLiteral("ECU MEMS Manager/logs"));
        const QString file=QFileDialog::getOpenFileName(this,I18n::text(18029),directory,
                               QStringLiteral("Communications ECU (*.txt);;Tous les fichiers (*.*)"));
        if(!file.isEmpty())showCommunicationFile(file);
    });

    QWidget *rightPanel=new QWidget(this);rightPanel->setObjectName(QStringLiteral("analysisRightPanel"));rightPanel->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Expanding);
    QVBoxLayout *right=new QVBoxLayout(rightPanel);right->setContentsMargins(7,7,7,7);right->setSpacing(5);
    right->addWidget(m_diagnosticLiveButton);
    right->addWidget(m_injectionLiveButton);
    QLabel *channels=new QLabel(I18n::text(6447),rightPanel);channels->setObjectName(QStringLiteral("analysisParametersTitle"));right->addWidget(channels);
    // Only the channels list scrolls: the title, file controls and the
    // selection/overlay buttons stay fixed in the right-hand panel.
    m_checkboxScrollArea=new QScrollArea(rightPanel);
    m_checkboxScrollArea->setObjectName(QStringLiteral("analysisChannelsScroll"));
    m_checkboxScrollArea->setFrameShape(QFrame::NoFrame);
    m_checkboxScrollArea->setWidgetResizable(true);
    m_checkboxScrollArea->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Ignored);
    m_checkboxScrollArea->setMinimumSize(0,0);
    m_checkboxScrollArea->setFocusPolicy(Qt::StrongFocus);
    configureAnalysisVisibleScroll(m_checkboxScrollArea);

    m_checkboxContainer=new QWidget();
    m_checkboxContainer->setObjectName(QStringLiteral("analysisChannelsContent"));
    m_checkboxLayout=new QVBoxLayout(m_checkboxContainer);
    m_checkboxLayout->setContentsMargins(4,4,4,4);
    m_checkboxLayout->setSpacing(2);
    // Without a minimum-size constraint a resizable QScrollArea can fit its
    // content into the viewport and hide rows without producing a scroll range.
    m_checkboxLayout->setSizeConstraint(QLayout::SetMinAndMaxSize);
    m_checkboxLayout->addStretch();
    m_checkboxScrollArea->setWidget(m_checkboxContainer);
    right->addWidget(m_checkboxScrollArea,1);
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
    setInjectionRamReadAvailable(false,I18n::text(7922));
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

void AnalysisTab::setInjectionRamReadAvailable(bool available,const QString &reason)
{
    m_injectionLiveButton->setEnabled(available);
    m_injectionLiveButton->setToolTip(available?QString():reason);
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
    m_originalTimeTexts.clear();m_originalColumnTexts.clear();
    const QStringList raw=diagnosticRawNames();
    m_rawColumnNames=raw;
    for(const QString &s:raw)m_columnNames<<friendlyColumnName(s);
    m_columns.resize(m_columnNames.size());m_liveClock.restart();rebuildCheckboxes();
}

void AnalysisTab::prepareInjectionLive()
{
    m_time.clear();m_columns.clear();
    m_originalTimeTexts.clear();m_originalColumnTexts.clear();m_rawColumnNames.clear();
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
    if(m_viewModeButton->isChecked())m_valuesModel->dataUpdated();
}

void AnalysisTab::onLoadFileClicked()
{
    QString documents=QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if(documents.isEmpty())documents=QDir::homePath();
    const QString logDir=QDir(documents).filePath(QStringLiteral("ECU MEMS Manager/logs"));
    const QString path=QFileDialog::getOpenFileName(this,I18n::text(6448),logDir,I18n::text(6449));
    if(!path.isEmpty())loadFile(path);
}

void AnalysisTab::loadFile(const QString &path){parseCsv(path);}

// Parse normal CSV, quoted fields, French semicolon-separated exports and tabs.
// The historic SESSION logger uses a separate ECU-identification line followed
// by "#time,<channels>"; an ordinary CSV starts directly with its header.
static QChar analysisCsvSeparator(const QString &line)
{
    int comma=0,semicolon=0,tab=0;
    bool quoted=false;
    for(int i=0;i<line.size();++i){
        const QChar c=line.at(i);
        if(c==QLatin1Char('"')){
            if(quoted && i+1<line.size() && line.at(i+1)==QLatin1Char('"')){++i;continue;}
            quoted=!quoted;
        }else if(!quoted){
            if(c==QLatin1Char(','))++comma;
            else if(c==QLatin1Char(';'))++semicolon;
            else if(c==QLatin1Char('\t'))++tab;
        }
    }
    if(semicolon>comma && semicolon>=tab)return QLatin1Char(';');
    if(tab>comma && tab>semicolon)return QLatin1Char('\t');
    return QLatin1Char(',');
}

static QStringList analysisCsvFields(const QString &line,QChar delimiter)
{
    QStringList fields;
    QString current;
    bool quoted=false;
    for(int i=0;i<line.size();++i){
        const QChar c=line.at(i);
        if(c==QLatin1Char('"')){
            if(quoted && i+1<line.size() && line.at(i+1)==QLatin1Char('"')){
                current.append(QLatin1Char('"'));++i;
            }else quoted=!quoted;
        }else if(c==delimiter && !quoted){
            fields.append(current.trimmed());current.clear();
        }else current.append(c);
    }
    fields.append(current.trimmed());
    return fields;
}

void AnalysisTab::parseCsv(const QString &path)
{
    QFile file(path);
    if(!file.open(QFile::ReadOnly|QFile::Text)){
        QMessageBox::warning(this,I18n::text(6450),I18n::text(6451).arg(path));
        return;
    }
    QTextStream stream(&file);
    QStringList headers;
    QChar separator=QLatin1Char(',');
    // Ignore optional ECU identifier / comments but never discard the actual
    // first CSV header. The old reader unconditionally skipped one line.
    while(!stream.atEnd()){
        QString line=stream.readLine().trimmed();
        if(!line.isEmpty() && line.at(0)==QChar::ByteOrderMark)
            line.remove(0,1);
        if(line.isEmpty())continue;
        if(line.startsWith(QLatin1Char('#'))){
            const QString prefix=line.mid(1).trimmed().toLower();
            if(!prefix.startsWith(QStringLiteral("time")) &&
               !prefix.startsWith(QStringLiteral("temps")) &&
               !prefix.startsWith(QStringLiteral("heure")) &&
               !prefix.startsWith(QStringLiteral("date")))
                continue;
        }
        separator=analysisCsvSeparator(line);
        headers=analysisCsvFields(line,separator);
        if(headers.size()>=2)break;
        headers.clear();
    }
    if(headers.size()<2){
        QMessageBox::warning(this,I18n::text(6454),I18n::text(6455));return;
    }
    if(!headers.isEmpty() && headers[0].startsWith(QLatin1Char('#')))
        headers[0].remove(0,1);
    const int count=headers.size()-1;
    QStringList names,originalNames,timeTexts;
    for(int i=1;i<headers.size();++i){
        const QString raw=headers.at(i).trimmed();
        originalNames.append(raw);
        names.append(friendlyColumnName(raw.isEmpty()
            ?QStringLiteral("Col %1").arg(i):raw));
    }
    QVector<double> times;
    QVector<QVector<double>> values(count);
    QVector<QStringList> originals(count);
    QTime firstTime;
    int lines=0;
    while(!stream.atEnd()){
        const QString line=stream.readLine();
        if(line.trimmed().isEmpty() || line.trimmed().startsWith(QLatin1Char('#')))
            continue;
        const QStringList fields=analysisCsvFields(line,separator);
        if(fields.size()<2)continue;
        const QString first=fields.at(0).trimmed();
        QTime t=QTime::fromString(first,QStringLiteral("hh:mm:ss"));
        if(!t.isValid())t=QTime::fromString(first,QStringLiteral("hh:mm:ss.zzz"));
        double sec=lines;
        if(t.isValid()){
            if(!firstTime.isValid())firstTime=t;
            sec=firstTime.msecsTo(t)/1000.0;
            if(sec<0)sec+=86400.0;
        }
        times.append(sec);
        timeTexts.append(first);
        for(int col=0;col<count;++col){
            const QString original=col+1<fields.size()?fields.at(col+1).trimmed():QString();
            originals[col].append(original);
            bool numericOk=false;
            double numeric=original.toDouble(&numericOk);
            if(!numericOk && original.contains(QLatin1Char(','))){
                QString normalized=original;normalized.replace(QLatin1Char(','),QLatin1Char('.'));
                numeric=normalized.toDouble(&numericOk);
            }
            values[col].append(numericOk?numeric:0.0);
        }
        ++lines;
    }
    file.close();
    if(!lines){
        QMessageBox::warning(this,I18n::text(6456),I18n::text(6457));return;
    }
    m_time=std::move(times);
    m_columns=std::move(values);
    m_columnNames=names;
    m_rawColumnNames=originalNames;
    m_originalTimeTexts=timeTexts;
    m_originalColumnTexts=std::move(originals);
    m_liveSource=FileSource;
    setProperty("analysisSourceFile",QFileInfo(path).fileName());
    setLiveButtons(false,false);
    emit liveSourceRequested(false);
    m_fileLabel->setText(QFileInfo(path).fileName()+
        QStringLiteral(" (%1 points)").arg(lines));
    m_overlayChart->setData(m_time,m_columns,m_columnNames,m_colors);
    rebuildCheckboxes();
    m_valuesModel->dataUpdated();
}

void AnalysisTab::rebuildCheckboxes()
{
    for(QCheckBox *cb:m_checkboxes){m_checkboxLayout->removeWidget(cb);delete cb;}m_checkboxes.clear();
    qDeleteAll(m_stackedCharts);m_stackedCharts.clear();
    QLayoutItem *stretch=nullptr;if(m_checkboxLayout->count()>0)stretch=m_checkboxLayout->takeAt(m_checkboxLayout->count()-1);
    for(int i=0;i<m_columnNames.size();++i){
        QCheckBox *cb=new QCheckBox(m_columnNames[i],m_checkboxContainer);const QColor c=m_colors[i%m_colors.size()];
        // Full name and original CSV identifier are readable on hover,
        // without modifying the existing right-side layout or checkbox text.
        const QString raw=i<m_rawColumnNames.size()?m_rawColumnNames[i]:QString();
        cb->setToolTip(raw.isEmpty() || raw==m_columnNames[i]
            ?m_columnNames[i]:(m_columnNames[i]+QStringLiteral("\n[")+raw+QStringLiteral("]")));
        cb->setAccessibleName(m_columnNames[i]);
        cb->setStyleSheet(QString((qApp && qApp->property("ecuDarkTheme").toBool()) ? "QCheckBox{padding:3px 0;}QCheckBox::indicator{width:13px;height:13px;border:1px solid %1;border-radius:2px;background:#0b1015;}QCheckBox::indicator:checked{background:%1;}" : "QCheckBox{padding:3px 0;}QCheckBox::indicator{width:13px;height:13px;border:1px solid %1;border-radius:2px;background:#ffffff;}QCheckBox::indicator:checked{background:%1;}").arg(c.name()));
        bool checked=false;
        if(m_liveSource==InjectionLiveSource)checked=(i<3);
        else checked=m_columnNames[i].contains(I18n::text(6458),Qt::CaseInsensitive)||m_columnNames[i].contains(I18n::text(6459),Qt::CaseInsensitive)||m_columnNames[i].contains(I18n::text(6460),Qt::CaseInsensitive);