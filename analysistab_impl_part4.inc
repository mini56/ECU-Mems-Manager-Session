        cb->setChecked(checked);connect(cb,&QCheckBox::toggled,this,&AnalysisTab::onCheckboxToggled);m_checkboxLayout->addWidget(cb);m_checkboxes.append(cb);
    }
    if(stretch)m_checkboxLayout->addItem(stretch);else m_checkboxLayout->addStretch();updateChartVisibility();
}

void AnalysisTab::onCheckboxToggled(bool checked)
{
    QCheckBox *cb=qobject_cast<QCheckBox*>(sender());if(!cb)return;const int i=m_checkboxes.indexOf(cb);if(i<0)return;
    if(m_overlayMode)m_overlayChart->setVisible(i,checked);else if(checked)addStackedChart(i);else removeStackedChart(i);
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
