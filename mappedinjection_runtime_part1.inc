namespace {

static QString hexAddress(quint16 address)
{
    return QStringLiteral("%1").arg(address,4,16,QLatin1Char('0')).toUpper();
}

class InjectionGaugeCard : public QWidget
{
public:
    InjectionGaugeCard(const QString &title,const QString &unit,double minimum,double maximum,QWidget *parent=nullptr)
        : QWidget(parent),m_title(title),m_unit(unit),m_min(minimum),m_max(maximum),m_value(0.0)
    {
        setMinimumSize(118,150);setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    }
    void setTitle(const QString &title){m_title=title;update();}
    void setValue(double value){m_value=value;update();}
    void setRange(double minimum,double maximum){m_min=minimum;m_max=maximum;update();}
protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);p.setRenderHint(QPainter::Antialiasing,true);p.setRenderHint(QPainter::TextAntialiasing,true);
        const qreal baseW=188.0,baseH=210.0,s=qMin(width()/baseW,height()/baseH);
        p.translate((width()-baseW*s)/2.0,(height()-baseH*s)/2.0);p.scale(s,s);
        p.setPen(QPen(QColor("#34414a"),1));p.setBrush(QColor("#0c1217"));p.drawRoundedRect(QRectF(.5,.5,187,209),5,5);
        QFont tf=p.font();tf.setBold(true);tf.setPointSizeF(8.0);p.setFont(tf);p.setPen(QColor("#edf2f4"));
        p.drawText(QRectF(6,5,176,20),Qt::AlignCenter,m_title.toUpper());p.setPen(QColor("#27333b"));p.drawLine(QPointF(8,26),QPointF(180,26));
        const QPointF c(94,105);const qreal r=70,startDeg=135,sweepDeg=270;
        p.setPen(QPen(QColor("#66727a"),1.2));p.setBrush(QColor("#060a0d"));p.drawEllipse(c,r,r);
        p.setPen(QPen(QColor("#38434a"),2));p.setBrush(Qt::NoBrush);p.drawArc(QRectF(c.x()-58,c.y()-58,116,116),(-45)*16,270*16);
        const int ticks=40;for(int i=0;i<=ticks;++i){const bool major=i%10==0;const qreal a=qDegreesToRadians(startDeg+sweepDeg*i/qreal(ticks));const QPointF o=c+QPointF(qCos(a)*(r-8),qSin(a)*(r-8));const QPointF in=c+QPointF(qCos(a)*(r-(major?20:14)),qSin(a)*(r-(major?20:14)));p.setPen(QPen(major?QColor("#f1f4f6"):QColor("#74818a"),major?1.4:.7));p.drawLine(in,o);}
        QFont sf=p.font();sf.setBold(true);sf.setPointSizeF(6.1);p.setFont(sf);p.setPen(QColor("#eef2f4"));
        for(int i=0;i<=4;++i){const double value=m_min+(m_max-m_min)*i/4.0;const qreal la=qDegreesToRadians(startDeg+sweepDeg*i/4.0);const QPointF pos=c+QPointF(qCos(la)*(r-31),qSin(la)*(r-31));QString txt;if(m_max<=6.0)txt=QString::number(value,'f',(qAbs(value-qRound(value))<.01)?0:1);else txt=QString::number(value,'f',0);p.drawText(QRectF(pos.x()-18,pos.y()-6,36,12),Qt::AlignCenter,txt);}
        const double span=qMax(0.0001,m_max-m_min);const double n=qBound(0.0,(m_value-m_min)/span,1.0);const qreal a=qDegreesToRadians(startDeg+sweepDeg*n);const QPointF d(qCos(a),qSin(a));const QPointF normal(-d.y(),d.x());
        QPainterPath needle;needle.moveTo(c+d*(r-14));needle.lineTo(c+normal*3.2);needle.lineTo(c-d*10);needle.lineTo(c-normal*3.2);needle.closeSubpath();p.setPen(Qt::NoPen);p.setBrush(QColor("#ff7417"));p.drawPath(needle);
        p.setBrush(QColor("#293239"));p.drawEllipse(c,6,6);
        QFont vf=p.font();vf.setBold(true);vf.setPointSizeF(18);p.setFont(vf);p.setPen(Qt::white);
        p.drawText(QRectF(40,130,108,28),Qt::AlignCenter,QString::number(m_value,'f',m_unit==QStringLiteral("ms")?2:0));
        QFont uf=p.font();uf.setBold(true);uf.setPointSizeF(8);p.setFont(uf);p.setPen(QColor("#d0d8dc"));p.drawText(QRectF(45,157,98,16),Qt::AlignCenter,m_unit);
    }
private:
    QString m_title,m_unit;double m_min,m_max,m_value;
};

class InjectionHistoryWidget : public QWidget
{
public:
    explicit InjectionHistoryWidget(QWidget *parent=nullptr):QWidget(parent){setMinimumHeight(170);setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);}
    void setData(const QVector<double> &time,const QVector<double> &values){m_time=time;m_values=values;update();}
protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);p.setRenderHint(QPainter::Antialiasing,true);p.fillRect(rect(),QColor("#0c1217"));
        QFont f=p.font();f.setBold(true);f.setPointSize(9);p.setFont(f);p.setPen(QColor("#edf2f4"));p.drawText(QRect(12,6,width()-24,20),Qt::AlignLeft|Qt::AlignVCenter,I18n::text(7905));
        QRect plot(48,34,qMax(1,width()-62),qMax(1,height()-58));p.setPen(QPen(QColor("#26313b"),1));p.setBrush(QColor("#090e13"));p.drawRect(plot);
        for(int i=0;i<=4;++i){int y=plot.top()+plot.height()*i/4;p.setPen(QColor("#233047"));p.drawLine(plot.left(),y,plot.right(),y);}
        for(int i=0;i<=6;++i){int x=plot.left()+plot.width()*i/6;p.drawLine(x,plot.top(),x,plot.bottom());}
        if(m_time.size()<2||m_values.size()<2){p.setPen(QColor("#8a93a6"));p.drawText(plot,Qt::AlignCenter,I18n::text(6440));return;}
        const double tMin=m_time.first(),tMax=m_time.last(),tSpan=qMax(.001,tMax-tMin);double vMin=m_values.first(),vMax=vMin;for(double v:m_values){vMin=qMin(vMin,v);vMax=qMax(vMax,v);}if(qAbs(vMax-vMin)<.01){vMin-=.5;vMax+=.5;}const double vm=(vMax-vMin)*.1;vMin=qMax(0.0,vMin-vm);vMax+=vm;const double vSpan=vMax-vMin;
        // Never invent a continuous slope across a period where no Injection
        // samples were received. Mode-4 normally samples far faster than this;
        // a gap above 0.5 s is therefore rendered as a genuine break.
        const double maxContinuousGapSeconds=0.5;
        QPainterPath historyPath;const int n=qMin(m_time.size(),m_values.size());for(int i=0;i<n;++i){const QPointF point(plot.left()+(m_time[i]-tMin)/tSpan*plot.width(),plot.bottom()-(m_values[i]-vMin)/vSpan*plot.height());if(i==0||(m_time[i]-m_time[i-1])>maxContinuousGapSeconds)historyPath.moveTo(point);else historyPath.lineTo(point);}
        p.setPen(QPen(QColor("#ff7a00"),1.8));p.drawPath(historyPath);
        QFont af=p.font();af.setBold(false);af.setPointSize(7);p.setFont(af);p.setPen(QColor("#9aa4ad"));
        for(int i=0;i<=4;++i){double v=vMax-vSpan*i/4.0;int y=plot.top()+plot.height()*i/4;p.drawText(QRect(0,y-8,43,16),Qt::AlignRight|Qt::AlignVCenter,QString::number(v,'f',2));}
        for(int i=0;i<=6;++i){int x=plot.left()+plot.width()*i/6;double t=tMin+tSpan*i/6.0;p.drawText(QRect(x-28,plot.bottom()+3,56,17),Qt::AlignCenter,QString("%1 s").arg(t-tMax,0,'f',0));}
    }
private:
    QVector<double> m_time,m_values;
};

class InjectionDashboard : public QWidget
{
public:
    explicit InjectionDashboard(QWidget *parent=nullptr):QWidget(parent)
    {
        setObjectName(QStringLiteral("injection_tab"));setAttribute(Qt::WA_StyledBackground,true);
        setStyleSheet(QStringLiteral("#injection_tab{background:#090e13;color:#e7ecef;}QLabel{background:transparent;color:#dce2e7;}"));
        QVBoxLayout *root=new QVBoxLayout(this);root->setContentsMargins(8,6,8,6);root->setSpacing(6);
        QFrame *head=new QFrame(this);head->setObjectName(QStringLiteral("injectionHeading"));head->setStyleSheet(QStringLiteral("#injectionHeading{background:#10161c;border:1px solid #29343e;border-radius:4px;}"));
        QVBoxLayout *hv=new QVBoxLayout(head);hv->setContentsMargins(12,5,12,5);hv->setSpacing(0);
        QLabel *title=new QLabel(I18n::text(7900).toUpper(),head);QFont tf=title->font();tf.setBold(true);tf.setPointSizeF(9.5);title->setFont(tf);title->setStyleSheet(QStringLiteral("color:#ff9828;"));hv->addWidget(title);
        m_status=new QLabel(I18n::text(7922),head);m_status->setStyleSheet(QStringLiteral("color:#89959e;"));hv->addWidget(m_status);root->addWidget(head);

        QGridLayout *grid=new QGridLayout;grid->setContentsMargins(0,0,0,0);grid->setSpacing(6);for(int c=0;c<4;++c)grid->setColumnStretch(c,1);
        m_final=new InjectionGaugeCard(I18n::text(7901),QStringLiteral("ms"),0,20,this);
        m_base=new InjectionGaugeCard(I18n::text(7902),QStringLiteral("ms"),0,20,this);
        m_transient=new InjectionGaugeCard(I18n::text(7903),QStringLiteral("ms"),0,5,this);
        m_counter=new InjectionGaugeCard(I18n::text(7904),QString(),0,255,this);
        grid->addWidget(m_final,0,0);grid->addWidget(m_base,0,1);grid->addWidget(m_transient,0,2);grid->addWidget(m_counter,0,3);root->addLayout(grid,1);

        QHBoxLayout *bottom=new QHBoxLayout;bottom->setContentsMargins(0,0,0,0);bottom->setSpacing(6);
        m_history=new InjectionHistoryWidget(this);bottom->addWidget(m_history,3);
        QFrame *raw=new QFrame(this);raw->setObjectName(QStringLiteral("injectionRawPanel"));raw->setStyleSheet(QStringLiteral("#injectionRawPanel{background:#0c1217;border:1px solid #29343e;border-radius:4px;}"));QVBoxLayout *rv=new QVBoxLayout(raw);rv->setContentsMargins(10,8,10,8);rv->setSpacing(4);
        QLabel *rt=new QLabel(I18n::text(7906).toUpper(),raw);QFont rtf=rt->font();rtf.setBold(true);rt->setFont(rtf);rt->setStyleSheet(QStringLiteral("color:#ff9828;"));rv->addWidget(rt);
