#include <QApplication>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEvent>
#include <QGridLayout>
#include <QLabel>
#include <QMainWindow>
#include <QPainter>
#include <QPainterPath>
#include <QRadialGradient>
#include <QTimer>
#include <QVariant>
#include <QVector>
#include <QVBoxLayout>
#include <QWidget>
#include <QtMath>

namespace {

class RebuildGaugeCard : public QWidget
{
public:
    RebuildGaugeCard(QObject *source,const QString &title,const QString &unit,QWidget *parent=nullptr)
        : QWidget(parent),m_source(source),m_title(title),m_unit(unit)
    {
        setMinimumSize(155,220);
        setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
        setAttribute(Qt::WA_TransparentForMouseEvents,true);
        m_clock.start();
    }

    void sample()
    {
        if (!m_source) return;
        QVariant v=m_source->property("value");
        if (!v.isValid()) return;
        const qint64 now=m_clock.elapsed();
        m_history.append(qMakePair(now,v.toDouble()));
        const qint64 cutoff=now-120000;
        while (!m_history.isEmpty() && m_history.first().first<cutoff) m_history.removeFirst();
        if (m_history.size()>600) m_history.remove(0,m_history.size()-600);
        update();
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing,true);
        const qreal s=qMin(width()/188.0,height()/260.0);
        const qreal x0=(width()-188*s)/2.0;
        const qreal y0=(height()-260*s)/2.0;
        p.translate(x0,y0);

        QRectF card(0,0,188*s,260*s);
        p.setPen(QPen(QColor("#2a343d"),qMax<qreal>(0.8,s)));
        p.setBrush(QColor("#0c1217"));
        p.drawRoundedRect(card.adjusted(.6*s,.6*s,-.6*s,-.6*s),4*s,4*s);

        QFont title=p.font();
        title.setBold(true);
        title.setPointSizeF(qBound<qreal>(5.5,6.4*s,8.2));
        p.setFont(title);
        p.setPen(QColor("#dbe2e7"));
        p.drawText(QRectF(6*s,5*s,176*s,17*s),Qt::AlignCenter,m_title.toUpper());
        p.setPen(QPen(QColor("#222c34"),qMax<qreal>(.6,.75*s)));
        p.drawLine(QPointF(8*s,24*s),QPointF(180*s,24*s));

        double value=m_source?m_source->property("value").toDouble():0.0;
        double minv=m_source?m_source->property("minimum").toDouble():0.0;
        double maxv=m_source?m_source->property("maximum").toDouble():100.0;
        if (!qIsFinite(minv)||!qIsFinite(maxv)||qFuzzyCompare(minv,maxv)) { minv=0; maxv=100; }

        const QPointF c(94*s,103*s);
        const qreal r=73*s;
        QRadialGradient face(c-QPointF(10*s,13*s),r*1.15);
        face.setColorAt(0,QColor("#1a2026"));
        face.setColorAt(.58,QColor("#0d1318"));
        face.setColorAt(1,QColor("#05090c"));
        p.setPen(QPen(QColor("#58636d"),1.1*s));
        p.setBrush(face);
        p.drawEllipse(c,r,r);
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(QColor("#151d23"),4*s));
        p.drawEllipse(c,r-4*s,r-4*s);
        p.setPen(QPen(QColor("#35424c"),.75*s));
        p.drawEllipse(c,r-8*s,r-8*s);

        const qreal start=-140.0;
        const qreal sweep=280.0;
        p.save();
        p.translate(c);
        for(int i=0;i<=70;i++) {
            const bool major=i%10==0;
            const bool medium=!major && i%5==0;
            const qreal a=qDegreesToRadians(start+sweep*i/70.0);
            const qreal ro=r-9*s;
            const qreal len=(major?10.5:(medium?7.0:3.7))*s;
            QPointF po(qCos(a)*ro,qSin(a)*ro);
            QPointF pi(qCos(a)*(ro-len),qSin(a)*(ro-len));
            QColor col=major?QColor("#e8ecef"):QColor("#7f8991");
            if (i>=64) col=QColor("#f04432");
            else if (i>=59) col=QColor("#ff7918");
            p.setPen(QPen(col,(major?1.15:(medium?.85:.55))*s,Qt::SolidLine,Qt::FlatCap));
            p.drawLine(pi,po);
        }
        p.restore();

        QFont sf=p.font(); sf.setBold(false); sf.setPointSizeF(qBound<qreal>(4.2,4.8*s,6.3)); p.setFont(sf);
        p.setPen(QColor("#c7cdd2"));
        for(int i=0;i<=6;i++) {
            double fv=minv+(maxv-minv)*i/6.0;
            qreal a=qDegreesToRadians(start+sweep*i/6.0);
            qreal rr=r-22*s;
            QPointF pos=c+QPointF(qCos(a)*rr,qSin(a)*rr);
            QString txt=qAbs(maxv-minv)<=25?QString::number(fv,'f',1):QString::number(fv,'f',0);
            p.drawText(QRectF(pos.x()-17*s,pos.y()-5*s,34*s,10*s),Qt::AlignCenter,txt);
        }

        double n=(value-minv)/(maxv-minv);
        const bool ok=value>=minv && value<=maxv;
        n=qBound(0.0,n,1.0);
        qreal ang=qDegreesToRadians(start+sweep*n);
        QPointF d(qCos(ang),qSin(ang));
        QPointF normal(-d.y(),d.x());
        QPointF tip=c+d*(r-16*s);
        QPointF rear=c-d*(9*s);
        QPainterPath needle;
        needle.moveTo(tip);
        needle.lineTo(c+normal*2.3*s);
        needle.lineTo(rear);
        needle.lineTo(c-normal*2.3*s);
        needle.closeSubpath();
        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#ff7417"));
        p.drawPath(needle);
        p.setPen(QPen(QColor("#59636b"),.7*s));
        p.setBrush(QColor("#20272d"));
        p.drawEllipse(c,5*s,5*s);
        p.setPen(Qt::NoPen); p.setBrush(QColor("#3a434b")); p.drawEllipse(c,2.2*s,2.2*s);

        QFont vf=p.font(); vf.setBold(true); vf.setPointSizeF(qBound<qreal>(10.0,13.0*s,17.0)); p.setFont(vf);
        p.setPen(QColor("#f6f8f9"));
        QString val=qAbs(value)<10 && qAbs(maxv-minv)<=40?QString::number(value,'f',1):QString::number(value,'f',0);
        p.drawText(QRectF(42*s,126*s,104*s,23*s),Qt::AlignCenter,val);
        QFont uf=p.font(); uf.setBold(false); uf.setPointSizeF(qBound<qreal>(4.8,5.5*s,6.8)); p.setFont(uf);
        p.setPen(QColor("#adb6bd"));
        p.drawText(QRectF(42*s,148*s,104*s,11*s),Qt::AlignCenter,m_unit);
        QFont st=p.font(); st.setBold(true); st.setPointSizeF(qBound<qreal>(4.4,5.0*s,6.2)); p.setFont(st);
        p.setPen(ok?QColor("#68df72"):QColor("#ff5141"));
        p.drawText(QRectF(8*s,164*s,172*s,11*s),Qt::AlignCenter,ok?QStringLiteral("NORMAL"):QStringLiteral("HORS PLAGE"));

        QRectF tr(8*s,184*s,172*s,66*s);
        p.setPen(QPen(QColor("#28333c"),.8*s));
        p.setBrush(QColor("#080d11"));
        p.drawRoundedRect(tr,2.5*s,2.5*s);
        for(int i=1;i<4;i++) { qreal y=tr.top()+tr.height()*i/4.; p.setPen(QPen(QColor("#182128"),.55*s)); p.drawLine(tr.left()+4*s,y,tr.right()-4*s,y); }
        for(int i=1;i<6;i++) { qreal x=tr.left()+tr.width()*i/6.; p.setPen(QPen(QColor("#151e24"),.5*s)); p.drawLine(x,tr.top()+4*s,x,tr.bottom()-4*s); }
        if (m_history.size()>1) {
            double hmin=m_history.first().second,hmax=hmin;
            for(const auto &pt:m_history){hmin=qMin(hmin,pt.second);hmax=qMax(hmax,pt.second);}
            if(qAbs(hmax-hmin)<.001){hmin-=1;hmax+=1;}
            const qint64 tmax=m_history.last().first;
            const qint64 tmin=qMax<qint64>(0,tmax-120000);
            QPainterPath path; bool first=true;
            for(const auto &pt:m_history){
                if(pt.first<tmin) continue;
                qreal x=tr.left()+4*s+((pt.first-tmin)/120000.0)*(tr.width()-8*s);
                qreal y=tr.bottom()-4*s-((pt.second-hmin)/(hmax-hmin))*(tr.height()-8*s);
                if(first){path.moveTo(x,y);first=false;}else path.lineTo(x,y);
            }
            p.setPen(QPen(QColor(255,122,0,50),3.2*s,Qt::SolidLine,Qt::RoundCap)); p.drawPath(path);
            p.setPen(QPen(QColor("#ff7a00"),1.15*s,Qt::SolidLine,Qt::RoundCap)); p.drawPath(path);
        }
        QFont tf=p.font(); tf.setPointSizeF(qBound<qreal>(3.8,4.2*s,5.2)); tf.setBold(false); p.setFont(tf);
        p.setPen(QColor("#58636c"));
        p.drawText(QRectF(tr.left()+4*s,tr.top()+2*s,36*s,9*s),Qt::AlignLeft,QStringLiteral("2 min"));
    }

private:
    QObject *m_source=nullptr;
    QString m_title;
    QString m_unit;
    QElapsedTimer m_clock;
    QVector<QPair<qint64,double>> m_history;
};

class SystemStateCard : public QWidget
{
public:
    explicit SystemStateCard(QObject *source,QWidget *parent=nullptr):QWidget(parent),m_source(source)
    {
        setMinimumSize(155,220);
        setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    }
protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this); p.setRenderHint(QPainter::Antialiasing,true);
        const qreal s=qMin(width()/188.0,height()/260.0); const qreal x=(width()-188*s)/2.,y=(height()-260*s)/2.; p.translate(x,y);
        QRectF card(0,0,188*s,260*s); p.setPen(QPen(QColor("#2a343d"),.9*s)); p.setBrush(QColor("#0c1217")); p.drawRoundedRect(card,4*s,4*s);
        QFont f=p.font(); f.setBold(true); f.setPointSizeF(qBound<qreal>(5.5,6.4*s,8.2)); p.setFont(f); p.setPen(QColor("#dbe2e7"));
        p.drawText(QRectF(6*s,7*s,176*s,17*s),Qt::AlignCenter,QStringLiteral("ÉTAT SYSTÈME"));
        const bool fault=m_source?m_source->property("checked").toBool():false;
        QPointF c(94*s,112*s); qreal rr=50*s;
        QPainterPath shield; shield.moveTo(c.x(),c.y()-rr); shield.lineTo(c.x()+rr*.75,c.y()-rr*.65); shield.lineTo(c.x()+rr*.62,c.y()+rr*.28); shield.quadTo(c.x(),c.y()+rr,c.x()-rr*.62,c.y()+rr*.28); shield.lineTo(c.x()-rr*.75,c.y()-rr*.65); shield.closeSubpath();
        p.setPen(QPen(fault?QColor("#ff4b3b"):QColor("#6bdd45"),2.3*s)); p.setBrush(QColor(0,0,0,0)); p.drawPath(shield);
        p.setPen(QPen(fault?QColor("#ff4b3b"):QColor("#6bdd45"),3*s,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));
        if(!fault){p.drawLine(QPointF(c.x()-18*s,c.y()+2*s),QPointF(c.x()-5*s,c.y()+15*s));p.drawLine(QPointF(c.x()-5*s,c.y()+15*s),QPointF(c.x()+23*s,c.y()-18*s));}
        QFont st=p.font(); st.setBold(true); st.setPointSizeF(qBound<qreal>(7.0,8.5*s,10.0)); p.setFont(st); p.setPen(fault?QColor("#ff5141"):QColor("#6bdd45"));
        p.drawText(QRectF(10*s,190*s,168*s,28*s),Qt::AlignCenter,fault?QStringLiteral("DÉFAUT DÉTECTÉ"):QStringLiteral("AUCUN DÉFAUT"));
    }
private: QObject *m_source=nullptr;
};

class OverviewRebuildInstaller : public QObject
{
public: explicit OverviewRebuildInstaller(QObject *p=nullptr):QObject(p){}
protected:
    bool eventFilter(QObject *watched,QEvent *event) override
    {
        if((event->type()!=QEvent::Show&&event->type()!=QEvent::Polish)||!watched) return QObject::eventFilter(watched,event);
        QMainWindow *w=qobject_cast<QMainWindow*>(watched);
        if(!w||w->objectName()!=QStringLiteral("MainWindow")||w->property("overviewRebuildInstalled").toBool()) return QObject::eventFilter(watched,event);
        QWidget *overview=w->findChild<QWidget*>(QStringLiteral("overview_tab"));
        if(!overview) return QObject::eventFilter(watched,event);
        w->setProperty("overviewRebuildInstalled",true);
        QTimer::singleShot(80,w,[w,overview](){install(w,overview);});
        return QObject::eventFilter(watched,event);
    }
private:
    static void install(QMainWindow *w,QWidget *overview)
    {
        if(overview->layout()) return;
        const QList<QWidget*> old=overview->findChildren<QWidget*>(QString(),Qt::FindDirectChildrenOnly);
        for(QWidget *child:old) child->hide();
        overview->setMinimumSize(0,0); overview->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX); overview->setAttribute(Qt::WA_StyledBackground,true);
        overview->setStyleSheet(QStringLiteral("#overview_tab{background:#090e13;}") );
        QVBoxLayout *root=new QVBoxLayout(overview); root->setContentsMargins(12,10,12,10); root->setSpacing(9);
        QGridLayout *grid=new QGridLayout; grid->setContentsMargins(0,0,0,0); grid->setHorizontalSpacing(7); grid->setVerticalSpacing(7);
        for(int c=0;c<6;c++) grid->setColumnStretch(c,1);
        grid->setRowStretch(0,1); grid->setRowStretch(1,1);

        struct G{const char*name;const char*title;const char*unit;int row;int col;};
        const G gauges[]={
            {"m_revCounter","Régime moteur","tr/min",0,0},
            {"m_waterTempGauge","Temp. eau","°C",0,1},
            {"m_mapGauge","Pression MAP","kPa",0,2},
            {"m_throttle_pos","Position papillon","%",0,3},
            {"m_battery","Tension batterie","V",0,4},
            {"m_short_term_correction","Correction carburant","%",0,5},
            {"m_lambda_voltage","Sonde lambda","mV",1,0},
            {"m_injector_time","Temps injection","ms",1,1},
            {"m_airTempGauge","Temp. air","°C",1,2},
            {"m_idle_position","Position ralenti","%",1,3},
            {"m_ignition_advance","Avance allumage","°",1,4}
        };
        QVector<RebuildGaugeCard*> cards;
        for(const G &g:gauges){
            QObject *src=w->findChild<QObject*>(QString::fromLatin1(g.name));
            RebuildGaugeCard *card=new RebuildGaugeCard(src,QString::fromUtf8(g.title),QString::fromUtf8(g.unit),overview);
            grid->addWidget(card,g.row,g.col); cards.append(card);
        }
        QObject *sys=w->findChild<QObject*>(QStringLiteral("m_engine_error"));
        grid->addWidget(new SystemStateCard(sys,overview),1,5);
        root->addLayout(grid,1);
        QTimer *timer=new QTimer(overview); timer->setInterval(500); QObject::connect(timer,&QTimer::timeout,overview,[cards](){for(RebuildGaugeCard *c:cards)c->sample();}); timer->start();
    }
};

void installOverviewRebuild(){QApplication *app=qobject_cast<QApplication*>(QCoreApplication::instance());if(!app)return;OverviewRebuildInstaller *i=new OverviewRebuildInstaller(app);app->installEventFilter(i);} 

}

Q_COREAPP_STARTUP_FUNCTION(installOverviewRebuild)
