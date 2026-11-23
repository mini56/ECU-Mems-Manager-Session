        cb->setChecked(checked);connect(cb,&QCheckBox::toggled,this,&AnalysisTab::onCheckboxToggled);m_checkboxLayout->addWidget(cb);m_checkboxes.append(cb);
    }
    if(stretch)m_checkboxLayout->addItem(stretch);else m_checkboxLayout->addStretch();
    // Notify QScrollArea of the new minimum content height after a CSV load
    // or a switch between the diagnostic and injection channel lists.
    m_checkboxLayout->invalidate();
    m_checkboxContainer->updateGeometry();
    m_checkboxScrollArea->verticalScrollBar()->setValue(0);
    updateChartVisibility();
    syncTableColumns();
}

void AnalysisTab::onCheckboxToggled(bool checked)
{
    QCheckBox *cb=qobject_cast<QCheckBox*>(sender());if(!cb)return;const int i=m_checkboxes.indexOf(cb);if(i<0)return;
    if(m_overlayMode)m_overlayChart->setVisible(i,checked);else if(checked)addStackedChart(i);else removeStackedChart(i);
    syncTableColumns();
}

void AnalysisTab::addStackedChart(int index)
{
    if(index<0||index>=m_columnNames.size()||m_stackedCharts.contains(index))return;
    SingleChartWidget *chart=new SingleChartWidget(m_columnNames[index],m_colors[index%m_colors.size()],m_stackContainer);
    if(index<m_columns.size())chart->setData(m_time,m_columns[index]);
    const int stretchIndex=qMax(0,m_stackLayout->count()-1);m_stackLayout->insertWidget(stretchIndex,chart);m_stackedCharts.insert(index,chart);
}

void AnalysisTab::removeStackedChart(int index)
{
    SingleChartWidget *chart=m_stackedCharts.take(index);if(!chart)return;m_stackLayout->removeWidget(chart);chart->deleteLater();
}

void AnalysisTab::updateChartVisibility()
{
    if(m_overlayMode){
        for(int i=0;i<m_checkboxes.size();++i)m_overlayChart->setVisible(i,m_checkboxes[i]->isChecked());
        return;
    }
    for(int i=0;i<m_checkboxes.size();++i){if(m_checkboxes[i]->isChecked())addStackedChart(i);else removeStackedChart(i);}
}

void AnalysisTab::onSelectAllClicked(){for(QCheckBox *cb:m_checkboxes)cb->setChecked(true);}
void AnalysisTab::onSelectNoneClicked(){for(QCheckBox *cb:m_checkboxes)cb->setChecked(false);}

void AnalysisTab::onOverlayToggled(bool checked)
{
    m_overlayMode=checked;m_stackScrollArea->setVisible(!checked);m_overlayChart->QWidget::setVisible(checked);
    m_overlayButton->setText(checked?I18n::text(6461):I18n::text(6462));
    if(checked)rebuildOverlayChart();else updateChartVisibility();
}

void AnalysisTab::rebuildOverlayChart()
{
    m_overlayChart->setData(m_time,m_columns,m_columnNames,m_colors);updateChartVisibility();
}


void AnalysisTab::syncTableColumns()
{
    if(!m_valuesModel)return;
    QVector<int> selected;
    for(int i=0;i<m_checkboxes.size();++i)
        if(m_checkboxes.at(i)->isChecked())selected.append(i);
    m_valuesModel->setSelectedChannels(selected);
}

void AnalysisTab::onViewModeToggled(bool columns)
{
    Q_UNUSED(columns);
    if(!m_leftStack || !m_viewModeButton)return;
    const bool communications=m_communicationsModeButton && m_communicationsModeButton->isChecked();
    // CSV/TXT measurements belong only to Curves and Columns. Hide their
    // loader while browsing raw ECU communication logs: one open button per mode.
    if(QWidget *fileToolbar=findChild<QWidget*>(QStringLiteral("analysisFileToolbar")))
        fileToolbar->setVisible(!communications);
    const bool table=!communications && m_viewModeButton->isChecked();
    m_leftStack->setCurrentIndex(communications?2:(table?1:0));
    if(table){
        syncTableColumns();
        m_valuesModel->dataUpdated();
    }
}

void AnalysisTab::refreshCommunicationFiles()
{
    if(!m_communicationFiles)return;
    QString docs=QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if(docs.isEmpty())docs=QDir::homePath();
    const QDir dir(QDir(docs).filePath(QStringLiteral("ECU MEMS Manager/logs")));
    const QString oldPath=m_communicationFiles->currentData().toString();
    const QStringList files=dir.entryList(
        QStringList()<<QStringLiteral("*_Communications_ECU*.txt"),
        QDir::Files,QDir::Time);
    QSignalBlocker blocker(m_communicationFiles);
    m_communicationFiles->clear();
    for(const QString &name:files)
        m_communicationFiles->addItem(name,dir.filePath(name));
    int index=m_communicationFiles->findData(oldPath);
    if(index<0)index=files.isEmpty()?-1:0;
    if(index>=0){
        m_communicationFiles->setCurrentIndex(index);
        showCommunicationFile(m_communicationFiles->itemData(index).toString());
    }else m_communicationText->setPlainText(I18n::text(18028));
}

void AnalysisTab::showCommunicationFile(const QString &path)
{
    QFile file(path);
    if(!file.open(QIODevice::ReadOnly|QIODevice::Text)){
        m_communicationText->setPlainText(I18n::text(6451).arg(path));
        return;
    }
    m_communicationText->setPlainText(QString::fromUtf8(file.readAll()));
    m_communicationText->moveCursor(QTextCursor::Start);
}
