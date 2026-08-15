#include <QApplication>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEvent>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QMainWindow>
#include <QPainter>
#include <QPainterPath>
#include <QRadialGradient>
#include <QSizePolicy>
#include <QTimer>
#include <QVariant>
#include <QVector>
#include <QVBoxLayout>
#include <QWidget>
#include <QtMath>
#include "i18n.h"

namespace {

class RebuildGaugeCard : public QWidget
{
public:
    RebuildGaugeCard(QObject *source,const QString &title,const QString &unit,QWidget *parent=nullptr)
        : QWidget(parent),m_source(source),m_title(title),m_unit(unit)
    {
        setMinimumSize(88,122);
        setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
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

        const qreal baseW=188.0;
        const qreal baseH=226.0;
        const qreal s=qMin(width()/baseW,height()/baseH);
        const qreal x0=(width()-baseW*s)/2.0;
        const qreal y0=(height()-baseH*s)/2.0;
        p.translate(x0,y0);
        p.scale(s,s);

        QRectF card(0,0,baseW,baseH);
        p.setPen(QPen(QColor("#303a43"),0.9));
        p.setBrush(QColor("#0c1217"));
        p.drawRoundedRect(card.adjusted(.5,.5,-.5,-.5),3.5,3.5);

        QFont title=p.font();
        title.setBold(true);
        title.setPointSizeF(7.0);
        p.setFont(title);
        p.setPen(QColor("#dfe5e9"));
        p.drawText(QRectF(6,5,176,17),Qt::AlignCenter,m_title.toUpper());
        p.setPen(QPen(QColor("#222c34"),0.8));
        p.drawLine(QPointF(8,24),QPointF(180,24));

        double value=m_source?m_source->property("value").toDouble():0.0;
        double minv=m_source?m_source->property("minimum").toDouble():0.0;
        double maxv=m_source?m_source->property("maximum").toDouble():100.0;
        if (!qIsFinite(minv)||!qIsFinite(maxv)||qFuzzyCompare(minv,maxv)) { minv=0; maxv=100; }

        const QPointF c(94,92);
        const qreal r=63;
        QRadialGradient bezel(c-QPointF(13,15),r*1.20);
        bezel.setColorAt(0,QColor("#4a5258"));
        bezel.setColorAt(.18,QColor("#1c2329"));
        bezel.setColorAt(.72,QColor("#090e12"));
        bezel.setColorAt(1,QColor("#020406"));
        p.setPen(QPen(QColor("#5a646c"),1.1));
        p.setBrush(bezel);
        p.drawEllipse(c,r,r);
        p.setPen(QPen(QColor("#11171c"),5.0));
        p.setBrush(QColor("#080d11"));
        p.drawEllipse(c,r-5,r-5);
        p.setPen(QPen(QColor("#39454e"),0.9));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(c,r-9,r-9);
        p.setPen(QPen(QColor("#151c21"),1.0));
        p.drawEllipse(c,r-13,r-13);

        const qreal start=-140.0;
        const qreal sweep=280.0;

        p.save();
        p.translate(c);
        for(int i=0;i<=70;i++) {
            const bool major=i%10==0;
            const bool medium=!major && i%5==0;
            const qreal a=qDegreesToRadians(start+sweep*i/70.0);
            const qreal ro=r-8;
            const qreal len=major?10.5:(medium?6.8:3.6);
            QPointF po(qCos(a)*ro,qSin(a)*ro);
            QPointF pi(qCos(a)*(ro-len),qSin(a)*(ro-len));
            const QColor col=major?QColor("#ecf0f2"):QColor("#828d95");
            p.setPen(QPen(col,major?1.25:(medium?.9:.55),Qt::SolidLine,Qt::FlatCap));
            p.drawLine(pi,po);
        }
        p.restore();

        QFont sf=p.font(); sf.setBold(false); sf.setPointSizeF(5.2); p.setFont(sf);
        p.setPen(QColor("#c8d0d5"));
        for(int i=0;i<=6;i++) {
            double fv=minv+(maxv-minv)*i/6.0;
            qreal a=qDegreesToRadians(start+sweep*i/6.0);
            qreal rr=r-22;
            QPointF pos=c+QPointF(qCos(a)*rr,qSin(a)*rr);
            QString txt=qAbs(maxv-minv)<=25?QString::number(fv,'f',1):QString::number(fv,'f',0);
            p.drawText(QRectF(pos.x()-17,pos.y()-5,34,10),Qt::AlignCenter,txt);
        }

        double n=(value-minv)/(maxv-minv);
        n=qBound(0.0,n,1.0);
        qreal ang=qDegreesToRadians(start+sweep*n);
        QPointF d(qCos(ang),qSin(ang));
        QPointF normal(-d.y(),d.x());
        QPointF tip=c+d*(r-15);
        QPointF rear=c-d*8;
        QPainterPath needle;
        needle.moveTo(tip);
        needle.lineTo(c+normal*2.1);
        needle.lineTo(rear);
        needle.lineTo(c-normal*2.1);
        needle.closeSubpath();
        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#ff7417"));
        p.drawPath(needle);
        p.setPen(QPen(QColor("#646e75"),0.8));
        p.setBrush(QColor("#252d33"));
        p.drawEllipse(c,5.2,5.2);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#505960"));
        p.drawEllipse(c,2.2,2.2);

        QFont vf=p.font(); vf.setBold(true); vf.setPointSizeF(13.0); p.setFont(vf);
        p.setPen(QColor("#f8fafb"));
        QString val=qAbs(value)<10 && qAbs(maxv-minv)<=40?QString::number(value,'f',1):QString::number(value,'f',0);
        p.drawText(QRectF(42,111,104,22),Qt::AlignCenter,val);
        QFont uf=p.font(); uf.setBold(false); uf.setPointSizeF(5.7); p.setFont(uf);
        p.setPen(QColor("#adb7bd"));
        p.drawText(QRectF(42,131,104,11),Qt::AlignCenter,m_unit);

        QRectF tr(8,151,172,66);
        p.setPen(QPen(QColor("#2a343d"),0.8));
        p.setBrush(QColor("#070c10"));
        p.drawRoundedRect(tr,2.5,2.5);
        for(int i=1;i<4;i++) {
            qreal y=tr.top()+tr.height()*i/4.;
            p.setPen(QPen(QColor("#1b242b"),0.55));
            p.drawLine(tr.left()+4,y,tr.right()-4,y);
        }
        for(int i=1;i<6;i++) {
            qreal x=tr.left()+tr.width()*i/6.;
            p.setPen(QPen(QColor("#182128"),0.5));
            p.drawLine(x,tr.top()+4,x,tr.bottom()-4);
        }
        if (m_history.size()>1) {
            double hmin=m_history.first().second,hmax=hmin;
            for(const auto &pt:m_history){hmin=qMin(hmin,pt.second);hmax=qMax(hmax,pt.second);}
            if(qAbs(hmax-hmin)<.001){hmin-=1;hmax+=1;}
            const qint64 tmax=m_history.last().first;
            const qint64 tmin=qMax<qint64>(0,tmax-120000);
            QPainterPath path; bool first=true;
            for(const auto &pt:m_history){
                if(pt.first<tmin) continue;
                qreal x=tr.left()+4+((pt.first-tmin)/120000.0)*(tr.width()-8);
                qreal y=tr.bottom()-4-((pt.second-hmin)/(hmax-hmin))*(tr.height()-8);
                if(first){path.moveTo(x,y);first=false;}else path.lineTo(x,y);
            }
            p.setPen(QPen(QColor(255,122,0,46),3.2,Qt::SolidLine,Qt::RoundCap)); p.drawPath(path);
            p.setPen(QPen(QColor("#ff7a00"),1.25,Qt::SolidLine,Qt::RoundCap)); p.drawPath(path);
        }
        QFont tf=p.font(); tf.setPointSizeF(4.4); tf.setBold(false); p.setFont(tf);
        p.setPen(QColor("#68747c"));
        p.drawText(QRectF(tr.left()+4,tr.top()+2,42,9),Qt::AlignLeft,QStringLiteral("2 min"));
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
        setMinimumSize(88,122);
        setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
        setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    }
protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this); p.setRenderHint(QPainter::Antialiasing,true);
        const qreal baseW=188.0,baseH=226.0;
        const qreal s=qMin(width()/baseW,height()/baseH);
        p.translate((width()-baseW*s)/2.,(height()-baseH*s)/2.); p.scale(s,s);
        QRectF card(0,0,baseW,baseH);
        p.setPen(QPen(QColor("#303a43"),.9)); p.setBrush(QColor("#0c1217")); p.drawRoundedRect(card.adjusted(.5,.5,-.5,-.5),3.5,3.5);
        QFont f=p.font(); f.setBold(true); f.setPointSizeF(7.0); p.setFont(f); p.setPen(QColor("#dfe5e9"));
        p.drawText(QRectF(6,6,176,17),Qt::AlignCenter,I18n::text(7149));
        p.setPen(QPen(QColor("#222c34"),.8)); p.drawLine(QPointF(8,24),QPointF(180,24));
        const bool fault=m_source?m_source->property("checked").toBool():false;
        QPointF c(94,97); qreal rr=44;
        QPainterPath shield; shield.moveTo(c.x(),c.y()-rr); shield.lineTo(c.x()+rr*.75,c.y()-rr*.65); shield.lineTo(c.x()+rr*.62,c.y()+rr*.28); shield.quadTo(c.x(),c.y()+rr,c.x()-rr*.62,c.y()+rr*.28); shield.lineTo(c.x()-rr*.75,c.y()-rr*.65); shield.closeSubpath();
        p.setPen(QPen(fault?QColor("#ff4b3b"):QColor("#6bdd45"),2.3)); p.setBrush(QColor(0,0,0,0)); p.drawPath(shield);
        p.setPen(QPen(fault?QColor("#ff4b3b"):QColor("#6bdd45"),3,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));
        if(!fault){p.drawLine(QPointF(c.x()-18,c.y()+2),QPointF(c.x()-5,c.y()+15));p.drawLine(QPointF(c.x()-5,c.y()+15),QPointF(c.x()+23,c.y()-18));}
        QFont st=p.font(); st.setBold(true); st.setPointSizeF(8.2); p.setFont(st); p.setPen(fault?QColor("#ff5141"):QColor("#6bdd45"));
        p.drawText(QRectF(10,157,168,28),Qt::AlignCenter,fault?I18n::text(7144):I18n::text(7143));
        QFont sub=p.font(); sub.setBold(false); sub.setPointSizeF(5.6); p.setFont(sub); p.setPen(QColor("#8d99a3"));
        p.drawText(QRectF(14,190,160,24),Qt::AlignCenter|Qt::TextWordWrap,I18n::text(7145));
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

        overview->setMinimumSize(0,0);
        overview->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
        overview->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
        overview->setAttribute(Qt::WA_StyledBackground,true);
        overview->setStyleSheet(QStringLiteral("#overview_tab{background:#090e13;}") );

        QVBoxLayout *root=new QVBoxLayout(overview);
        root->setContentsMargins(8,6,8,6);
        root->setSpacing(5);
        root->setSizeConstraint(QLayout::SetDefaultConstraint);

        QFrame *heading=new QFrame(overview);
        heading->setObjectName(QStringLiteral("overviewHeading"));
        heading->setAttribute(Qt::WA_StyledBackground,true);
        heading->setStyleSheet(QStringLiteral("#overviewHeading{background:#10161c;border:1px solid #29343e;border-radius:4px;}"));
        heading->setMinimumHeight(34);
        heading->setMaximumHeight(44);
        heading->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
        QVBoxLayout *hv=new QVBoxLayout(heading); hv->setContentsMargins(12,4,12,4); hv->setSpacing(0);
        QLabel *title=new QLabel(I18n::text(1001).toUpper(),heading); QFont tf=title->font(); tf.setBold(true); tf.setPointSizeF(9.5); title->setFont(tf); title->setStyleSheet(QStringLiteral("color:#ff9828;background:transparent;"));
        QLabel *sub=new QLabel(I18n::text(7146),heading); QFont sf=sub->font(); sf.setPointSizeF(6.8); sub->setFont(sf); sub->setStyleSheet(QStringLiteral("color:#83909a;background:transparent;"));
        hv->addWidget(title); hv->addWidget(sub); root->addWidget(heading,0);

        QGridLayout *grid=new QGridLayout;
        grid->setContentsMargins(0,0,0,0);
        grid->setHorizontalSpacing(5);
        grid->setVerticalSpacing(5);
        grid->setSizeConstraint(QLayout::SetDefaultConstraint);
        for(int c=0;c<6;c++) grid->setColumnStretch(c,1);
        grid->setRowStretch(0,1); grid->setRowStretch(1,1);

        struct G{const char*name;int titleKey;const char*unit;int row;int col;};
        const G gauges[]={
            {"m_revCounter",1011,"tr/min",0,0},
            {"m_waterTempGauge",1017,"°C",0,1},
            {"m_mapGauge",1012,"kPa",0,2},
            {"m_throttle_pos",1003,"%",0,3},
            {"m_battery",1006,"V",0,4},
            {"m_short_term_correction",7140,"%",0,5},
            {"m_lambda_voltage",7142,"mV",1,0},
            {"m_injector_time",7147,"ms",1,1},
            {"m_airTempGauge",1018,"°C",1,2},
            {"m_idle_position",7148,"%",1,3},
            {"m_ignition_advance",7141,"°",1,4}
        };

        QVector<RebuildGaugeCard*> cards;
        for(const G &g:gauges){
            QObject *src=w->findChild<QObject*>(QString::fromLatin1(g.name));
            RebuildGaugeCard *card=new RebuildGaugeCard(src,I18n::text(g.titleKey),QString::fromUtf8(g.unit),overview);
            grid->addWidget(card,g.row,g.col);
            cards.append(card);
        }
        QObject *sys=w->findChild<QObject*>(QStringLiteral("m_engine_error"));
        grid->addWidget(new SystemStateCard(sys,overview),1,5);
        root->addLayout(grid,1);

        QTimer *timer=new QTimer(overview);
        timer->setInterval(500);
        QObject::connect(timer,&QTimer::timeout,overview,[cards](){for(RebuildGaugeCard *c:cards)c->sample();});
        timer->start();
    }
};

void installOverviewRebuild(){QApplication *app=qobject_cast<QApplication*>(QCoreApplication::instance());if(!app)return;OverviewRebuildInstaller *i=new OverviewRebuildInstaller(app);app->installEventFilter(i);} 

}

Q_COREAPP_STARTUP_FUNCTION(installOverviewRebuild)