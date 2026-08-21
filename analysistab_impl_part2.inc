    const int n=qMin(m_time.size(),m_values.size());
    for(int i=0;i<n;++i){
        const double xf=(m_time[i]-tMin)/tSpan;
        const double yf=(m_values[i]-vMin)/vSpan;
        poly << QPointF(plot.left()+xf*plot.width(),plot.bottom()-yf*plot.height());
    }
    p.setPen(QPen(m_color,1.8)); p.drawPolyline(poly);

    p.setPen(QColor("#8a93a6"));
    for(int i=0;i<=5;++i){
        const double t=tMin+tSpan*i/5.0;
        const int x=plot.left()+plot.width()*i/5;
        p.drawText(QRect(x-34,plot.bottom()+3,68,17),Qt::AlignCenter,
                   QString("%1 s").arg(t-tMax,0,'f',0));
    }

    if(m_hasCursor && m_cursorX>=plot.left() && m_cursorX<=plot.right()){
        p.setPen(QPen(QColor("#d6dce1"),1,Qt::DashLine));
        p.drawLine(m_cursorX,plot.top(),m_cursorX,plot.bottom());
        const double tAt=tMin+double(m_cursorX-plot.left())/qMax(1,plot.width())*tSpan;
        int idx=0; double best=1e99;
        for(int i=0;i<m_time.size();++i){ double d=qAbs(m_time[i]-tAt); if(d<best){best=d;idx=i;} }
        if(idx<m_values.size()){
            const QString label=QString::number(m_values[idx],'f',2);
            QFontMetrics fm(axis); const int tw=fm.horizontalAdvance(label)+10;
            int bx=m_cursorX+6; if(bx+tw>plot.right()) bx=m_cursorX-6-tw;
            p.setPen(Qt::NoPen); p.setBrush(QColor(14,20,32,225)); p.drawRect(bx,plot.top()+4,tw,17);
            p.setPen(m_color); p.drawText(QRect(bx+5,plot.top()+4,tw-5,17),Qt::AlignVCenter,label);
        }
    }
}

ChartWidget::ChartWidget(QWidget *parent)
    : QWidget(parent), m_hasCursor(false), m_cursorX(0)
{
    setMinimumHeight(150);
    setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    setMouseTracking(true);
}

void ChartWidget::setData(const QVector<double> &time,
                          const QVector<QVector<double> > &series,
                          const QStringList &names,
                          const QVector<QColor> &colors)
{
    m_time=time; m_series=series; m_names=names; m_colors=colors;
    m_visible=QVector<bool>(names.count(),false);
    update();
}

void ChartWidget::setVisible(int index,bool visible)
{
    if(index>=0 && index<m_visible.size()){m_visible[index]=visible;update();}
}

void ChartWidget::clearData()
{
    m_time.clear();m_series.clear();m_names.clear();m_colors.clear();m_visible.clear();update();
}

void ChartWidget::mouseMoveEvent(QMouseEvent *event){m_hasCursor=true;m_cursorX=event->pos().x();update();}
void ChartWidget::leaveEvent(QEvent *event){Q_UNUSED(event);m_hasCursor=false;update();}

void ChartWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter p(this); p.setRenderHint(QPainter::Antialiasing); p.fillRect(rect(),QColor("#0d1218"));
    const QRect plot(45,12,qMax(1,width()-58),qMax(1,height()-38));
    p.setPen(QPen(QColor("#233047"),1));
    for(int i=0;i<=4;++i){int y=plot.top()+plot.height()*i/4;p.drawLine(plot.left(),y,plot.right(),y);}
    for(int i=0;i<=10;++i){int x=plot.left()+plot.width()*i/10;p.drawLine(x,plot.top(),x,plot.bottom());}
    if(m_time.size()<2){drawEmptyMessage(p,plot,I18n::text(6468));return;}

    const double tMin=m_time.first(),tMax=m_time.last(),tSpan=qMax(0.001,tMax-tMin);
    int visibleCount=0;
    for(int s=0;s<m_series.size();++s){
        if(s>=m_visible.size()||!m_visible[s]||m_series[s].isEmpty())continue;
        ++visibleCount;
        const QVector<double> &vals=m_series[s];
        double vMin=vals.first(),vMax=vals.first();for(double x:vals){vMin=qMin(vMin,x);vMax=qMax(vMax,x);}
        const double vSpan=qMax(0.001,vMax-vMin);
        QPolygonF poly; const int n=qMin(m_time.size(),vals.size());
        for(int i=0;i<n;++i){const double xf=(m_time[i]-tMin)/tSpan;const double yf=(vals[i]-vMin)/vSpan;poly<<QPointF(plot.left()+xf*plot.width(),plot.bottom()-yf*plot.height());}
        p.setPen(QPen(m_colors[s%m_colors.size()],1.8));p.drawPolyline(poly);
    }
    if(!visibleCount)drawEmptyMessage(p,plot,I18n::text(6441));
    p.setPen(QColor("#8a93a6"));
    for(int i=0;i<=5;++i){int x=plot.left()+plot.width()*i/5;double t=tMin+tSpan*i/5.0;p.drawText(QRect(x-32,plot.bottom()+4,64,18),Qt::AlignCenter,QString("%1 s").arg(t-tMax,0,'f',0));}

    if(m_hasCursor&&visibleCount&&m_cursorX>=plot.left()&&m_cursorX<=plot.right()){
        p.setPen(QPen(QColor("#d6dce1"),1,Qt::DashLine));p.drawLine(m_cursorX,plot.top(),m_cursorX,plot.bottom());
        const double tAt=tMin+double(m_cursorX-plot.left())/qMax(1,plot.width())*tSpan;
        int idx=0;double best=1e99;for(int i=0;i<m_time.size();++i){double d=qAbs(m_time[i]-tAt);if(d<best){best=d;idx=i;}}
        int y=plot.top()+5;
        for(int s=0;s<m_series.size();++s){
            if(s>=m_visible.size()||!m_visible[s]||idx>=m_series[s].size())continue;
            QString txt=QString("%1 : %2").arg(m_names[s]).arg(m_series[s][idx],0,'f',2);
            p.setPen(m_colors[s%m_colors.size()]);p.drawText(QRect(plot.left()+8,y,plot.width()-16,17),Qt::AlignLeft|Qt::AlignVCenter,txt);y+=18;
        }
    }
}

AnalysisTab::AnalysisTab(QWidget *parent)
    : QWidget(parent), m_overlayMode(false), m_liveSource(FileSource)
{
    setObjectName(QStringLiteral("analysisTab"));
    m_colors << QColor("#ff7a00") << QColor("#4fc3f7") << QColor("#66bb6a")
             << QColor("#ffca28") << QColor("#ba68c8") << QColor("#26c6da")
             << QColor("#ef5350") << QColor("#9ccc65") << QColor("#ec407a")
             << QColor("#7e57c2") << QColor("#8d6e63") << QColor("#5c6bc0");

    QVBoxLayout *root=new QVBoxLayout(this);
    root->setContentsMargins(6,6,6,6);root->setSpacing(6);

    QWidget *topBar=new QWidget(this);topBar->setObjectName(QStringLiteral("analysisTopBar"));
    QGridLayout *top=new QGridLayout(topBar);top->setContentsMargins(7,5,7,5);top->setHorizontalSpacing(6);top->setVerticalSpacing(5);
    QWidget *spacer1=new QWidget(topBar);spacer1->setObjectName(QStringLiteral("analysisTopSpacer1"));
    QWidget *spacer2=new QWidget(topBar);spacer2->setObjectName(QStringLiteral("analysisTopSpacer2"));
    spacer1->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Preferred);spacer2->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Preferred);

    // Keep the four historical buttons created first: some older visual patches
    // still locate them by creation order.
    m_loadButton=new QPushButton(I18n::text(6442),topBar);m_loadButton->setObjectName(QStringLiteral("analysisLoadButton"));
    m_selectAllButton=new QPushButton(I18n::text(6444),this);m_selectAllButton->setObjectName(QStringLiteral("analysisSelectAllButton"));
    m_selectNoneButton=new QPushButton(I18n::text(6445),this);m_selectNoneButton->setObjectName(QStringLiteral("analysisSelectNoneButton"));
    m_overlayButton=new QPushButton(I18n::text(6446),this);m_overlayButton->setObjectName(QStringLiteral("analysisOverlayButton"));m_overlayButton->setCheckable(true);
    m_fileLabel=new QLabel(I18n::text(6443),topBar);m_fileLabel->setObjectName(QStringLiteral("analysisFileLabel"));m_fileLabel->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);

    m_diagnosticLiveButton=new QPushButton(I18n::text(7207),topBar);m_diagnosticLiveButton->setObjectName(QStringLiteral("analysisDiagnosticLiveButton"));m_diagnosticLiveButton->setCheckable(true);
    m_injectionLiveButton=new QPushButton(I18n::text(7208),topBar);m_injectionLiveButton->setObjectName(QStringLiteral("analysisInjectionLiveButton"));m_injectionLiveButton->setCheckable(true);
    m_liveButtonGroup=new QButtonGroup(this);m_liveButtonGroup->setExclusive(true);m_liveButtonGroup->addButton(m_diagnosticLiveButton);m_liveButtonGroup->addButton(m_injectionLiveButton);

    top->addWidget(spacer1,0,0);top->addWidget(m_loadButton,0,1);top->addWidget(m_fileLabel,0,2,1,2);
    top->addWidget(spacer2,1,0);top->addWidget(m_diagnosticLiveButton,1,1);top->addWidget(m_injectionLiveButton,1,2);top->setColumnStretch(3,1);
    root->addWidget(topBar);

    QHBoxLayout *mainRow=new QHBoxLayout();mainRow->setContentsMargins(0,0,0,0);mainRow->setSpacing(6);root->addLayout(mainRow,1);

    m_stackScrollArea=new QScrollArea(this);m_stackScrollArea->setObjectName(QStringLiteral("analysisStackScroll"));m_stackScrollArea->setWidgetResizable(true);
    m_stackContainer=new QWidget();m_stackLayout=new QVBoxLayout(m_stackContainer);m_stackLayout->setContentsMargins(4,4,4,4);m_stackLayout->setSpacing(6);m_stackLayout->addStretch();m_stackScrollArea->setWidget(m_stackContainer);
