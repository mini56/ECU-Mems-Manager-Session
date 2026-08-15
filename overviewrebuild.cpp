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
        setMinimumSize(118,150);
        setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
        setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
        setAttribute(Qt::WA_TransparentForMouseEvents,true);
        m_clock.start();
    }

    void sample()
    {
        if (!m_source) return;
        const QVariant v=m_source->property("value");
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
        p.setRenderHint(QPainter::TextAntialiasing,true);

        const qreal baseW=188.0;
        const qreal baseH=226.0;
        const qreal s=qMin(width()/baseW,height()/baseH);
        p.translate((width()-baseW*s)/2.0,(height()-baseH*s)/2.0);
        p.scale(s,s);

        const QRectF card(0,0,baseW,baseH);
        p.setPen(QPen(QColor("#34414a"),1.0));
        p.setBrush(QColor("#0c1217"));
        p.drawRoundedRect(card.adjusted(.5,.5,-.5,-.5),5.0,5.0);

        QFont title=p.font();
        title.setBold(true);
        title.setPointSizeF(8.2);
        p.setFont(title);
        p.setPen(QColor("#edf2f4"));
        p.drawText(QRectF(6,5,176,18),Qt::AlignCenter,m_title.toUpper());
        p.setPen(QPen(QColor("#27333b"),1.0));
        p.drawLine(QPointF(8,25),QPointF(180,25));

        double value=m_source?m_source->property("value").toDouble():0.0;
        double minv=m_source?m_source->property("minimum").toDouble():0.0;
        double maxv=m_source?m_source->property("maximum").toDouble():100.0;
        if (!qIsFinite(minv)||!qIsFinite(maxv)||qFuzzyCompare(minv,maxv)) {
            minv=0.0;
            maxv=100.0;
        }

        const QPointF c(94,92);
        const qreal r=67.0;
        const qreal start=-140.0;
        const qreal sweep=280.0;

        QRadialGradient bezel(c-QPointF(15,18),r*1.22);
        bezel.setColorAt(0,QColor("#56616a"));
        bezel.setColorAt(.17,QColor("#252e35"));
        bezel.setColorAt(.55,QColor("#10171c"));
        bezel.setColorAt(.83,QColor("#060a0d"));
        bezel.setColorAt(1,QColor("#020304"));
        p.setPen(QPen(QColor("#66727a"),1.25));
        p.setBrush(bezel);
        p.drawEllipse(c,r,r);

        p.setPen(QPen(QColor("#151d22"),5.5));
        p.setBrush(QColor("#05090c"));
        p.drawEllipse(c,r-5.5,r-5.5);
        p.setPen(QPen(QColor("#3d4a53"),1.15));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(c,r-10.0,r-10.0);

        const QRectF arcRect(c.x()-r+11,c.y()-r+11,(r-11)*2,(r-11)*2);
        p.setPen(QPen(QColor("#303b43"),2.0,Qt::SolidLine,Qt::RoundCap));
        p.drawArc(arcRect,qRound(-start*16.0),qRound(-sweep*16.0));

        double n=(value-minv)/(maxv-minv);
        n=qBound(0.0,n,1.0);
        const qreal currentDeg=start+sweep*n;
        p.setPen(QPen(QColor("#ff7a00"),2.2,Qt::SolidLine,Qt::RoundCap));
        p.drawArc(arcRect,qRound(-start*16.0),qRound(-(currentDeg-start)*16.0));

        p.save();
        p.translate(c);
        const int tickCount=40;
        for(int i=0;i<=tickCount;i++) {
            const bool major=i%10==0;
            const bool medium=!major && i%5==0;
            const qreal a=qDegreesToRadians(start+sweep*i/qreal(tickCount));
            const qreal ro=r-8.0;
            const qreal len=major?13.0:(medium?8.0:4.5);
            const QPointF po(qCos(a)*ro,qSin(a)*ro);
            const QPointF pi(qCos(a)*(ro-len),qSin(a)*(ro-len));
            const QColor col=major?QColor("#f4f6f7"):(medium?QColor("#b4bec5"):QColor("#707d85"));
            p.setPen(QPen(col,major?1.55:(medium?1.05:.7),Qt::SolidLine,Qt::FlatCap));
            p.drawLine(pi,po);
        }
        p.restore();

        QFont scaleFont=p.font();
        scaleFont.setBold(true);
        scaleFont.setPointSizeF(7.0);
        p.setFont(scaleFont);
        p.setPen(QColor("#e4eaed"));
        const int labelCount=4;
        for(int i=0;i<=labelCount;i++) {
            const double fv=minv+(maxv-minv)*i/qreal(labelCount);
            const qreal a=qDegreesToRadians(start+sweep*i/qreal(labelCount));
            const qreal rr=r-24.5;
            const QPointF pos=c+QPointF(qCos(a)*rr,qSin(a)*rr);
            const QString txt=qAbs(maxv-minv)<=25.0
                ? QString::number(fv,'f',1)
                : QString::number(fv,'f',0);
            p.drawText(QRectF(pos.x()-20,pos.y()-7,40,14),Qt::AlignCenter,txt);
        }

        const qreal ang=qDegreesToRadians(currentDeg);
        const QPointF d(qCos(ang),qSin(ang));
        const QPointF normal(-d.y(),d.x());
        const QPointF tip=c+d*(r-14);
        const QPointF rear=c-d*10;
        QPainterPath needle;
        needle.moveTo(tip);
        needle.lineTo(c+normal*3.0);
        needle.lineTo(rear);
        needle.lineTo(c-normal*3.0);
        needle.closeSubpath();
        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#ff7417"));
        p.drawPath(needle);

        p.setPen(QPen(QColor("#77838a"),1.0));
        p.setBrush(QColor("#293239"));
        p.drawEllipse(c,6.1,6.1);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#59636a"));
        p.drawEllipse(c,2.5,2.5);

        QFont valueFont=p.font();
        valueFont.setBold(true);
        valueFont.setPointSizeF(19.0);
        p.setFont(valueFont);
        p.setPen(QColor("#ffffff"));
        const QString val=(qAbs(value)<10.0 && qAbs(maxv-minv)<=40.0)
            ? QString::number(value,'f',1)
            : QString::number(value,'f',0);
        p.drawText(QRectF(37,110,114,28),Qt::AlignCenter,val);

        QFont unitFont=p.font();
        unitFont.setBold(true);
        unitFont.setPointSizeF(7.2);
        p.setFont(unitFont);
        p.setPen(QColor("#c3cdd3"));
        p.drawText(QRectF(40,136,108,13),Qt::AlignCenter,m_unit);

        const QRectF tr(8,158,172,59);
        p.setPen(QPen(QColor("#2d3942"),.9));
        p.setBrush(QColor("#070c10"));
        p.drawRoundedRect(tr,3.0,3.0);

        p.setPen(QPen(QColor("#192229"),.6));
        for(int i=1;i<3;i++) {
            const qreal y=tr.top()+tr.height()*i/3.0;
            p.drawLine(tr.left()+4,y,tr.right()-4,y);
        }
        for(int i=1;i<6;i++) {
            const qreal x=tr.left()+tr.width()*i/6.0;
            p.drawLine(x,tr.top()+4,x,tr.bottom()-4);
        }

        if (m_history.size()>1) {
            double hmin=m_history.first().second,hmax=hmin;
            for(const auto &pt:m_history) {
                hmin=qMin(hmin,pt.second);
                hmax=qMax(hmax,pt.second);
            }
            if(qAbs(hmax-hmin)<.001) {
                hmin-=1.0;
                hmax+=1.0;
            }
            const qint64 tmax=m_history.last().first;
            const qint64 tmin=qMax<qint64>(0,tmax-120000);
            QPainterPath path;
            bool first=true;
            for(const auto &pt:m_history) {
                if(pt.first<tmin) continue;
                const qreal x=tr.left()+5+((pt.first-tmin)/120000.0)*(tr.width()-10);
                const qreal y=tr.bottom()-5-((pt.second-hmin)/(hmax-hmin))*(tr.height()-10);
                if(first) {
                    path.moveTo(x,y);
                    first=false;
                } else {
                    path.lineTo(x,y);
                }
            }
            p.setPen(QPen(QColor(255,122,0,42),3.6,Qt::SolidLine,Qt::RoundCap));
            p.drawPath(path);
            p.setPen(QPen(QColor("#ff7a00"),1.45,Qt::SolidLine,Qt::RoundCap));
            p.drawPath(path);
        }
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
        setMinimumSize(118,150);
        setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
        setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing,true);
        p.setRenderHint(QPainter::TextAntialiasing,true);

        const qreal baseW=188.0,baseH=226.0;
        const qreal s=qMin(width()/baseW,height()/baseH);
        p.translate((width()-baseW*s)/2.0,(height()-baseH*s)/2.0);
        p.scale(s,s);

        const QRectF card(0,0,baseW,baseH);
        p.setPen(QPen(QColor("#34414a"),1.0));
        p.setBrush(QColor("#0c1217"));
        p.drawRoundedRect(card.adjusted(.5,.5,-.5,-.5),5.0,5.0);

        QFont f=p.font();
        f.setBold(true);
        f.setPointSizeF(8.2);
        p.setFont(f);
        p.setPen(QColor("#edf2f4"));
        p.drawText(QRectF(6,5,176,18),Qt::AlignCenter,I18n::text(7149));
        p.setPen(QPen(QColor("#27333b"),1.0));
        p.drawLine(QPointF(8,25),QPointF(180,25));

        const bool fault=m_source?m_source->property("checked").toBool():false;
        const QPointF c(94,97);
        const qreal rr=45;
        QPainterPath shield;
        shield.moveTo(c.x(),c.y()-rr);
        shield.lineTo(c.x()+rr*.75,c.y()-rr*.65);
        shield.lineTo(c.x()+rr*.62,c.y()+rr*.28);
        shield.quadTo(c.x(),c.y()+rr,c.x()-rr*.62,c.y()+rr*.28);
        shield.lineTo(c.x()-rr*.75,c.y()-rr*.65);
        shield.closeSubpath();

        const QColor state=fault?QColor("#ff4b3b"):QColor("#6bdd45");
        p.setPen(QPen(state,2.6));
        p.setBrush(Qt::NoBrush);
        p.drawPath(shield);
        p.setPen(QPen(state,3.3,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));
        if(!fault) {
            p.drawLine(QPointF(c.x()-18,c.y()+2),QPointF(c.x()-5,c.y()+15));
            p.drawLine(QPointF(c.x()-5,c.y()+15),QPointF(c.x()+23,c.y()-18));
        }

        QFont st=p.font();
        st.setBold(true);
        st.setPointSizeF(9.2);
        p.setFont(st);
        p.setPen(state);
        p.drawText(QRectF(10,157,168,28),Qt::AlignCenter,fault?I18n::text(7144):I18n::text(7143));

        QFont sub=p.font();
        sub.setBold(false);
        sub.setPointSizeF(6.0);
        p.setFont(sub);
        p.setPen(QColor("#8d99a3"));
        p.drawText(QRectF(14,190,160,24),Qt::AlignCenter|Qt::TextWordWrap,I18n::text(7145));
    }

private:
    QObject *m_source=nullptr;
};

class OverviewRebuildInstaller : public QObject
{
public:
    explicit OverviewRebuildInstaller(QObject *p=nullptr):QObject(p){}

protected:
    bool eventFilter(QObject *watched,QEvent *event) override
    {
        if((event->type()!=QEvent::Show&&event->type()!=QEvent::Polish)||!watched)
            return QObject::eventFilter(watched,event);

        QMainWindow *w=qobject_cast<QMainWindow*>(watched);
        if(!w||w->objectName()!=QStringLiteral("MainWindow")||w->property("overviewRebuildInstalled").toBool())
            return QObject::eventFilter(watched,event);

        QWidget *overview=w->findChild<QWidget*>(QStringLiteral("overview_tab"));
        if(!overview)
            return QObject::eventFilter(watched,event);

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
        overview->setStyleSheet(QStringLiteral("#overview_tab{background:#090e13;}"));

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

        QVBoxLayout *hv=new QVBoxLayout(heading);
        hv->setContentsMargins(12,4,12,4);
        hv->setSpacing(0);

        QLabel *title=new QLabel(I18n::text(1001).toUpper(),heading);
        QFont tf=title->font();
        tf.setBold(true);
        tf.setPointSizeF(9.5);
        title->setFont(tf);
        title->setStyleSheet(QStringLiteral("color:#ff9828;background:transparent;"));

        QLabel *sub=new QLabel(I18n::text(7146),heading);
        QFont sf=sub->font();
        sf.setPointSizeF(6.8);
        sub->setFont(sf);
        sub->setStyleSheet(QStringLiteral("color:#83909a;background:transparent;"));

        hv->addWidget(title);
        hv->addWidget(sub);
        root->addWidget(heading,0);

        QGridLayout *grid=new QGridLayout;
        grid->setContentsMargins(0,0,0,0);
        grid->setHorizontalSpacing(5);
        grid->setVerticalSpacing(5);
        grid->setSizeConstraint(QLayout::SetDefaultConstraint);
        for(int c=0;c<6;c++) grid->setColumnStretch(c,1);
        grid->setRowStretch(0,1);
        grid->setRowStretch(1,1);

        struct G { const char *name; int titleKey; const char *unit; int row; int col; };
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
        for(const G &g:gauges) {
            QObject *src=w->findChild<QObject*>(QString::fromLatin1(g.name));
            RebuildGaugeCard *gaugeCard=new RebuildGaugeCard(src,I18n::text(g.titleKey),QString::fromUtf8(g.unit),overview);
            grid->addWidget(gaugeCard,g.row,g.col);
            cards.append(gaugeCard);
        }

        QObject *sys=w->findChild<QObject*>(QStringLiteral("m_engine_error"));
        grid->addWidget(new SystemStateCard(sys,overview),1,5);
        root->addLayout(grid,1);

        QTimer *timer=new QTimer(overview);
        timer->setInterval(500);
        QObject::connect(timer,&QTimer::timeout,overview,[cards](){
            for(RebuildGaugeCard *c:cards) c->sample();
        });
        timer->start();
    }
};

void installOverviewRebuild()
{
    QApplication *app=qobject_cast<QApplication*>(QCoreApplication::instance());
    if(!app) return;
    OverviewRebuildInstaller *installer=new OverviewRebuildInstaller(app);
    app->installEventFilter(installer);
}

}

Q_COREAPP_STARTUP_FUNCTION(installOverviewRebuild)
