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
    QPainter p(this); p.setRenderHint(QPainter::Antialiasing);
    const bool darkTheme=qApp && qApp->property("ecuDarkTheme").toBool();
    p.fillRect(rect(),darkTheme?QColor("#0d1218"):QColor("#ffffff"));
    const QRect plot(45,12,qMax(1,width()-58),qMax(1,height()-38));
    p.setPen(QPen((darkTheme?QColor("#233047"):QColor("#dae2e9")),1));
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
    p.setPen((darkTheme?QColor("#8a93a6"):QColor("#425160")));
    for(int i=0;i<=5;++i){int x=plot.left()+plot.width()*i/5;double t=tMin+tSpan*i/5.0;p.drawText(QRect(x-32,plot.bottom()+4,64,18),Qt::AlignCenter,QString("%1 s").arg(t-tMax,0,'f',0));}

    if(m_hasCursor&&visibleCount&&m_cursorX>=plot.left()&&m_cursorX<=plot.right()){
        p.setPen(QPen((darkTheme?QColor("#d6dce1"):QColor("#53616f")),1,Qt::DashLine));p.drawLine(m_cursorX,plot.top(),m_cursorX,plot.bottom());
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

// Native QWidget lifecycle replaces a global application event filter.
void AnalysisTab::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    if (property("analysisModernized").toBool()) return;
    const qreal pt=font().pointSizeF()>0?font().pointSizeF():9.0;
    setProperty("analysisModernized",true);
    setProperty("analysisBaseFontPointSize",pt);
    QTimer::singleShot(0,this,[this](){
        applyAnalysisModernStyle(this);
        configureNativeAnalysis(this);
    });
}

void AnalysisTab::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (property("analysisModernized").toBool())
        QTimer::singleShot(0,this,[this](){
            fitAnalysisModernSizing(this);
            configureNativeAnalysis(this);
        });
}

// Lightweight read-only view over the SAME measurement arrays as the curves.
// No duplicated time-series calculation and no per-cell widget allocations.
class AnalysisValuesTableModel final : public QAbstractTableModel
{
public:
    AnalysisValuesTableModel(const QVector<double> *time,
                             const QVector<QVector<double>> *values,
                             const QStringList *names,
                             const QStringList *originalTime,
                             const QVector<QStringList> *originalValues,
                             QObject *parent)
        : QAbstractTableModel(parent),m_time(time),m_values(values),m_names(names),
          m_originalTime(originalTime),m_originalValues(originalValues) {}

    int rowCount(const QModelIndex &parent=QModelIndex()) const override
    {
        return parent.isValid() || !m_time ? 0 : m_time->size();
    }
    int columnCount(const QModelIndex &parent=QModelIndex()) const override
    {
        return parent.isValid() ? 0 : 1+m_selected.size();
    }
    QVariant headerData(int section,Qt::Orientation orientation,int role) const override
    {
        if(orientation==Qt::Vertical && role==Qt::DisplayRole) return section+1;
        if(orientation!=Qt::Horizontal || (role!=Qt::DisplayRole && role!=Qt::ToolTipRole))
            return QVariant();
        if(section==0) return I18n::text(18022);
        const int selected=section-1;
        if(selected<0 || selected>=m_selected.size() || !m_names)return QVariant();
        const int source=m_selected.at(selected);
        return source>=0 && source<m_names->size()?m_names->at(source):QVariant();
    }
    QVariant data(const QModelIndex &index,int role=Qt::DisplayRole) const override
    {
        if(!index.isValid() || !m_time || index.row()<0 ||
           index.row()>=m_time->size())return QVariant();
        if(role==Qt::TextAlignmentRole)
            return int(Qt::AlignRight|Qt::AlignVCenter);
        if(role!=Qt::DisplayRole && role!=Qt::ToolTipRole)return QVariant();
        if(index.column()==0){
            if(m_originalTime && index.row()<m_originalTime->size() &&
               !m_originalTime->at(index.row()).isEmpty())
                return m_originalTime->at(index.row());
            return QString::number(m_time->at(index.row()),'f',2);
        }
        const int selected=index.column()-1;
        if(selected<0 || selected>=m_selected.size())return QVariant();
        const int source=m_selected.at(selected);
        if(m_originalValues && source>=0 && source<m_originalValues->size() &&
           index.row()<m_originalValues->at(source).size())
            return m_originalValues->at(source).at(index.row());
        if(!m_values || source<0 || source>=m_values->size() ||
           index.row()>=m_values->at(source).size())return QVariant();
        return QString::number(m_values->at(source).at(index.row()),'g',12);
    }
    void setSelectedChannels(const QVector<int> &selected)
    {
        if(m_selected==selected)return;
        beginResetModel();m_selected=selected;endResetModel();
    }
    void dataUpdated()
    {
        beginResetModel();endResetModel();
    }
private:
    const QVector<double> *m_time;
    const QVector<QVector<double>> *m_values;
    const QStringList *m_names;
    const QStringList *m_originalTime;
    const QVector<QStringList> *m_originalValues;
    QVector<int> m_selected;
};

AnalysisTab::AnalysisTab(QWidget *parent)
    : QWidget(parent), m_overlayMode(false), m_liveSource(FileSource)
{
    setObjectName(QStringLiteral("analysisTab"));
    setProperty("analysisSourceFile",QString());
    m_colors << QColor("#ff7a00") << QColor("#4fc3f7") << QColor("#66bb6a")
             << QColor("#ffca28") << QColor("#ba68c8") << QColor("#26c6da")
             << QColor("#ef5350") << QColor("#9ccc65") << QColor("#ec407a")
             << QColor("#7e57c2") << QColor("#8d6e63") << QColor("#5c6bc0");

    QVBoxLayout *root=new QVBoxLayout(this);
    root->setContentsMargins(6,6,6,6);root->setSpacing(6);

    m_loadButton=new QPushButton(I18n::text(6442),this);m_loadButton->setObjectName(QStringLiteral("analysisLoadButton"));
    m_selectAllButton=new QPushButton(I18n::text(6444),this);m_selectAllButton->setObjectName(QStringLiteral("analysisSelectAllButton"));
    m_selectNoneButton=new QPushButton(I18n::text(6445),this);m_selectNoneButton->setObjectName(QStringLiteral("analysisSelectNoneButton"));
    m_overlayButton=new QPushButton(I18n::text(6446),this);m_overlayButton->setObjectName(QStringLiteral("analysisOverlayButton"));m_overlayButton->setCheckable(true);
    m_fileLabel=new QLabel(I18n::text(6443),this);m_fileLabel->setObjectName(QStringLiteral("analysisFileLabel"));m_fileLabel->setWordWrap(true);m_fileLabel->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);

    m_diagnosticLiveButton=new QPushButton(I18n::text(6876),this);m_diagnosticLiveButton->setObjectName(QStringLiteral("analysisDiagnosticLiveButton"));m_diagnosticLiveButton->setCheckable(true);
    m_injectionLiveButton=new QPushButton(I18n::text(7900),this);m_injectionLiveButton->setObjectName(QStringLiteral("analysisInjectionLiveButton"));m_injectionLiveButton->setCheckable(true);
    for(QPushButton *button:{m_loadButton,m_diagnosticLiveButton,m_injectionLiveButton})button->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
    m_liveButtonGroup=new QButtonGroup(this);m_liveButtonGroup->setExclusive(true);m_liveButtonGroup->addButton(m_diagnosticLiveButton);m_liveButtonGroup->addButton(m_injectionLiveButton);

    QHBoxLayout *mainRow=new QHBoxLayout();mainRow->setContentsMargins(0,0,0,0);mainRow->setSpacing(6);root->addLayout(mainRow,1);

    // The plots must scroll INSIDE the page when many signals are selected.
    // A resizable viewport with a content-constrained layout preserves the
    // charts' minimum heights rather than cutting the last chart off-screen.
    m_stackScrollArea=new QScrollArea(this);
    m_stackScrollArea->setObjectName(QStringLiteral("analysisStackScroll"));
    m_stackScrollArea->setFrameShape(QFrame::NoFrame);
    m_stackScrollArea->setWidgetResizable(true);
    m_stackScrollArea->setMinimumSize(0,0);
    m_stackScrollArea->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Ignored);
    configureAnalysisVisibleScroll(m_stackScrollArea);
    m_stackContainer=new QWidget();
    m_stackContainer->setObjectName(QStringLiteral("analysisPlotsContent"));
    m_stackLayout=new QVBoxLayout(m_stackContainer);
    m_stackLayout->setContentsMargins(4,4,4,4);
    m_stackLayout->setSpacing(6);
    m_stackLayout->setSizeConstraint(QLayout::SetMinAndMaxSize);
    m_stackLayout->addStretch();
    m_stackScrollArea->setWidget(m_stackContainer);