#include <QApplication>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEvent>
#include <QLabel>
#include <QMainWindow>
#include <QPainter>
#include <QPainterPath>
#include <QTimer>
#include <QVariant>
#include <QVector>
#include <QWidget>
#include <QtMath>

namespace {

class ModernGaugeCard : public QWidget
{
public:
    ModernGaugeCard(const QRect &geometry, QObject *source, const QString &title, QWidget *parent)
        : QWidget(parent), m_source(source), m_title(title)
    {
        setGeometry(geometry);
        setProperty("screenfitBaseGeometry", geometry);
        setAttribute(Qt::WA_TransparentForMouseEvents, true);
        m_clock.start();
        show();
    }

    void sample()
    {
        if (!m_source) return;
        const QVariant vv = m_source->property("value");
        if (!vv.isValid()) return;
        const qint64 now = m_clock.elapsed();
        m_history.append(qMakePair(now, vv.toDouble()));
        const qint64 cutoff = now - 120000;
        while (!m_history.isEmpty() && m_history.first().first < cutoff)
            m_history.removeFirst();
        if (m_history.size() > 1000)
            m_history.remove(0, m_history.size() - 1000);
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        const QRectF card = rect().adjusted(1,1,-1,-1);
        QLinearGradient cardGrad(card.topLeft(), card.bottomRight());
        cardGrad.setColorAt(0.0, QColor("#1b2026"));
        cardGrad.setColorAt(1.0, QColor("#11151a"));
        p.setPen(QPen(QColor("#343b43"), 1));
        p.setBrush(cardGrad);
        p.drawRoundedRect(card, 10, 10);

        p.setPen(QColor("#e9edf0"));
        QFont tf = p.font(); tf.setPointSize(8); tf.setBold(true); p.setFont(tf);
        p.drawText(QRectF(8,6,width()-16,18), Qt::AlignCenter, m_title);

        const double value = m_source ? m_source->property("value").toDouble() : 0.0;
        double minv = m_source ? m_source->property("minimum").toDouble() : 0.0;
        double maxv = m_source ? m_source->property("maximum").toDouble() : 100.0;
        if (qFuzzyCompare(minv,maxv)) { minv=0.0; maxv=100.0; }
        const QString suffix = m_source ? m_source->property("suffix").toString().trimmed() : QString();

        const qreal gaugeTop = 26.0;
        const qreal trendH = 48.0;
        const qreal trendBottom = height()-9.0;
        const QRectF trendRect(10, trendBottom-trendH, width()-20, trendH);
        const qreal gaugeAvailableH = trendRect.top()-gaugeTop-4.0;
        const qreal diameter = qMin<qreal>(width()-28.0, gaugeAvailableH);
        const QPointF center(width()/2.0, gaugeTop + diameter/2.0 + 1.0);
        const qreal radius = diameter/2.0;

        QRadialGradient face(center, radius, center-QPointF(radius*0.25,radius*0.30));
        face.setColorAt(0.0, QColor("#20262d"));
        face.setColorAt(0.70, QColor("#11161c"));
        face.setColorAt(1.0, QColor("#070a0e"));
        p.setPen(QPen(QColor("#4b535d"), 2));
        p.setBrush(face);
        p.drawEllipse(center, radius, radius);
        p.setPen(QPen(QColor("#242a31"), 5));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(center, radius-5, radius-5);

        const qreal startDeg = -135.0;
        const qreal sweepDeg = 270.0;
        p.save();
        p.translate(center);
        for (int i=0;i<=40;++i) {
            const bool major=(i%5)==0;
            const qreal a=qDegreesToRadians(startDeg + sweepDeg*i/40.0);
            const qreal outer=radius-10;
            const qreal inner=outer-(major?9:5);
            QPointF po(qCos(a)*outer,qSin(a)*outer);
            QPointF pi(qCos(a)*inner,qSin(a)*inner);
            p.setPen(QPen(major?QColor("#e5e8eb"):QColor("#7f8994"), major?1.5:0.8));
            p.drawLine(pi,po);
        }
        p.restore();

        QFont sf=p.font(); sf.setPointSize(7); sf.setBold(false); p.setFont(sf);
        p.setPen(QColor("#d8dde1"));
        for (int i=0;i<=4;++i) {
            const double fv=minv+(maxv-minv)*i/4.0;
            const qreal a=qDegreesToRadians(startDeg+sweepDeg*i/4.0);
            const qreal rr=radius-27;
            const QPointF pos=center+QPointF(qCos(a)*rr,qSin(a)*rr);
            QString txt;
            const double span=qAbs(maxv-minv);
            if (span<=20.0) txt=QString::number(fv,'f',1);
            else txt=QString::number(fv,'f',0);
            QRectF tr(pos.x()-24,pos.y()-8,48,16);
            p.drawText(tr,Qt::AlignCenter,txt);
        }

        double norm=(value-minv)/(maxv-minv);
        norm=qBound(0.0,norm,1.0);
        const qreal needleDeg=startDeg+sweepDeg*norm;
        const qreal na=qDegreesToRadians(needleDeg);
        const QPointF tip=center+QPointF(qCos(na)*(radius-22),qSin(na)*(radius-22));
        const QPointF tail=center-QPointF(qCos(na)*12,qSin(na)*12);

        QPen glow(QColor(255,123,0,70),7,Qt::SolidLine,Qt::RoundCap);
        p.setPen(glow); p.drawLine(tail,tip);
        p.setPen(QPen(QColor("#ff8a1c"),3,Qt::SolidLine,Qt::RoundCap));
        p.drawLine(tail,tip);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#ff9b32")); p.drawEllipse(center,5.5,5.5);
        p.setBrush(QColor("#242a31")); p.drawEllipse(center,2.4,2.4);

        QFont vf=p.font(); vf.setPointSize(16); vf.setBold(false); p.setFont(vf);
        p.setPen(QColor("#ffffff"));
        QString valueText;
        if (qAbs(value) < 10.0 && qAbs(maxv-minv)<=40.0) valueText=QString::number(value,'f',1);
        else valueText=QString::number(value,'f',0);
        p.drawText(QRectF(center.x()-62,center.y()+18,124,25),Qt::AlignCenter,valueText);
        if (!suffix.isEmpty()) {
            QFont uf=p.font(); uf.setPointSize(7); uf.setBold(false); p.setFont(uf);
            p.setPen(QColor("#aeb6bd"));
            QString clean=suffix; clean.remove('['); clean.remove(']');
            p.drawText(QRectF(center.x()-58,center.y()+41,116,14),Qt::AlignCenter,clean.trimmed());
        }

        p.setPen(QPen(QColor("#303740"),1));
        p.setBrush(QColor("#0d1116"));
        p.drawRoundedRect(trendRect,4,4);
        for (int i=1;i<4;++i) {
            const qreal y=trendRect.top()+trendRect.height()*i/4.0;
            p.setPen(QPen(QColor("#232930"),0.8));
            p.drawLine(trendRect.left()+4,y,trendRect.right()-4,y);
        }
        if (m_history.size()>1) {
            double hmin=m_history.first().second,hmax=hmin;
            for (const auto &pt:m_history) { hmin=qMin(hmin,pt.second); hmax=qMax(hmax,pt.second); }
            if (qAbs(hmax-hmin)<0.001) { hmin-=1.0; hmax+=1.0; }
            const qint64 tmax=m_history.last().first;
            const qint64 tmin=qMax<qint64>(0,tmax-120000);
            QPainterPath path; bool first=true;
            for (const auto &pt:m_history) {
                if (pt.first<tmin) continue;
                const qreal x=trendRect.left()+4+((pt.first-tmin)/120000.0)*(trendRect.width()-8);
                const qreal y=trendRect.bottom()-4-((pt.second-hmin)/(hmax-hmin))*(trendRect.height()-8);
                if (first) { path.moveTo(x,y); first=false; } else path.lineTo(x,y);
            }
            p.setPen(QPen(QColor(255,138,28,65),4,Qt::SolidLine,Qt::RoundCap)); p.drawPath(path);
            p.setPen(QPen(QColor("#ff8a1c"),1.5)); p.drawPath(path);
        }
    }

private:
    QObject *m_source=nullptr;
    QString m_title;
    QElapsedTimer m_clock;
    QVector<QPair<qint64,double>> m_history;
};

class OverviewCardsInstaller : public QObject
{
public:
    explicit OverviewCardsInstaller(QObject *parent=nullptr) : QObject(parent) {}

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if ((event->type()==QEvent::Show || event->type()==QEvent::Polish) && watched) {
            QMainWindow *window=qobject_cast<QMainWindow*>(watched);
            if (window && window->objectName()==QStringLiteral("MainWindow") &&
                !window->property("overviewCardsInstalled").toBool()) {
                QWidget *overview=window->findChild<QWidget*>(QStringLiteral("overview_tab"));
                if (overview) {
                    window->setProperty("overviewCardsInstalled", true);
                    QTimer::singleShot(180, window, [window,overview](){
                        struct Item { const char *source; const char *label; };
                        const Item items[10] = {
                            {"m_revCounter","m_revCounterLabel"},
                            {"m_mapGauge","m_mapGaugeLabel"},
                            {"m_throttle_pos","m_throttle_pos_label"},
                            {"m_idle_position","m_idlebypass_pos_label"},
                            {"m_battery","m_battery_voltage_label"},
                            {"m_waterTempGauge","m_waterTempLabel"},
                            {"m_airTempGauge","m_airTempLabel"},
                            {"m_lambda_voltage","m_lambda_label"},
                            {"m_short_term_correction","m_short_term_label"},
                            {"m_ignition_advance","m_ignition_advance_label"}
                        };
                        const int xs[5]={28,278,528,778,1028};
                        QVector<ModernGaugeCard*> cards;
                        for (int i=0;i<10;++i) {
                            QWidget *source=window->findChild<QWidget*>(QString::fromLatin1(items[i].source));
                            QLabel *label=window->findChild<QLabel*>(QString::fromLatin1(items[i].label));
                            if (!source) continue;
                            const QString title=label?label->text():QString();
                            ModernGaugeCard *card=new ModernGaugeCard(QRect(xs[i%5],i<5?8:272,238,252),source,title,overview);
                            card->setObjectName(QStringLiteral("modernGaugeCard_%1").arg(i));
                            card->raise();
                            cards.append(card);
                            source->hide();
                            if (label) label->hide();
                        }
                        QTimer *timer=new QTimer(overview);
                        timer->setInterval(500);
                        QObject::connect(timer,&QTimer::timeout,overview,[cards](){
                            for (ModernGaugeCard *card:cards) if (card) card->sample();
                        });
                        timer->start();
                    });
                }
            }
        }
        return QObject::eventFilter(watched,event);
    }
};

void installOverviewCardsHook()
{
    QApplication *app=qobject_cast<QApplication*>(QCoreApplication::instance());
    if (!app) return;
    OverviewCardsInstaller *installer=new OverviewCardsInstaller(app);
    app->installEventFilter(installer);
}

}

Q_COREAPP_STARTUP_FUNCTION(installOverviewCardsHook)
