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

        // Reference card: same composition at every resolution. ScreenFit only
        // changes this single proportional factor; no alternate layout is used.
        const qreal sx = width() / 238.0;
        const qreal sy = height() / 252.0;
        const qreal s = qBound<qreal>(0.62, qMin(sx, sy), 1.35);
        const qreal line = qMax<qreal>(0.8, s);

        const QRectF card = rect().adjusted(line, line, -line, -line);
        p.setPen(QPen(QColor("#27313a"), line));
        p.setBrush(QColor("#0d1217"));
        p.drawRoundedRect(card, 3.0*s, 3.0*s);

        // Header strip exactly follows the compact DARK PRO language of the mock-up.
        const QRectF titleRect(7*s, 5*s, width()-14*s, 17*s);
        p.setPen(QColor("#cfd6dc"));
        QFont titleFont=p.font();
        titleFont.setPointSizeF(qBound<qreal>(5.8, 6.8*s, 8.5));
        titleFont.setBold(true);
        p.setFont(titleFont);
        p.drawText(titleRect, Qt::AlignHCenter|Qt::AlignVCenter, m_title.toUpper());
        p.setPen(QPen(QColor("#202a32"), line));
        p.drawLine(QPointF(7*s,24*s),QPointF(width()-7*s,24*s));

        const double value = m_source ? m_source->property("value").toDouble() : 0.0;
        double minv = m_source ? m_source->property("minimum").toDouble() : 0.0;
        double maxv = m_source ? m_source->property("maximum").toDouble() : 100.0;
        if (qFuzzyCompare(minv,maxv)) { minv=0.0; maxv=100.0; }
        const QString suffix = m_source ? m_source->property("suffix").toString().trimmed() : QString();

        // Gauge occupies the upper card, history trace remains immediately below it.
        const QRectF trendRect(8*s, 204*s, width()-16*s, 39*s);
        const QPointF center(width()/2.0, 104*s);
        const qreal radius = qMin<qreal>(88*s, (width()-30*s)/2.0);

        // Crisp layered dial, deliberately flatter than the previous gradient card.
        p.setPen(QPen(QColor("#303b45"), 1.2*s));
        p.setBrush(QColor("#080c10"));
        p.drawEllipse(center, radius, radius);
        p.setPen(QPen(QColor("#131b22"), 4.0*s));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(center, radius-4.5*s, radius-4.5*s);
        p.setPen(QPen(QColor("#3a4651"), 0.9*s));
        p.drawEllipse(center, radius-9*s, radius-9*s);

        const qreal startDeg=-135.0;
        const qreal sweepDeg=270.0;
        p.save();
        p.translate(center);
        for (int i=0;i<=60;++i) {
            const bool major=(i%10)==0;
            const bool medium=(!major && i%5==0);
            const qreal a=qDegreesToRadians(startDeg+sweepDeg*i/60.0);
            const qreal outer=radius-10*s;
            const qreal len=(major?10.5:(medium?7.5:4.5))*s;
            const QPointF po(qCos(a)*outer,qSin(a)*outer);
            const QPointF pi(qCos(a)*(outer-len),qSin(a)*(outer-len));

            QColor tick("#8c959e");
            if (major) tick=QColor("#d8dde1");
            // The approved reference has an orange/red high-end warning sector.
            if (i>=54) tick=QColor("#f04b32");
            else if (i>=49) tick=QColor("#ff8a1c");
            p.setPen(QPen(tick,(major?1.25:(medium?1.0:0.7))*s,Qt::SolidLine,Qt::FlatCap));
            p.drawLine(pi,po);
        }
        p.restore();

        // Six clean numeric marks, kept small like the visual reference.
        QFont scaleFont=p.font();
        scaleFont.setPointSizeF(qBound<qreal>(4.8,5.8*s,7.2));
        scaleFont.setBold(false);
        p.setFont(scaleFont);
        p.setPen(QColor("#aeb6bd"));
        for (int i=0;i<=5;++i) {
            const double fv=minv+(maxv-minv)*i/5.0;
            const qreal a=qDegreesToRadians(startDeg+sweepDeg*i/5.0);
            const qreal rr=radius-26*s;
            const QPointF pos=center+QPointF(qCos(a)*rr,qSin(a)*rr);
            const double span=qAbs(maxv-minv);
            const QString txt=span<=20.0 ? QString::number(fv,'f',1) : QString::number(fv,'f',0);
            p.drawText(QRectF(pos.x()-20*s,pos.y()-6*s,40*s,12*s),Qt::AlignCenter,txt);
        }

        double norm=(value-minv)/(maxv-minv);
        const bool inRange=(value>=minv && value<=maxv);
        norm=qBound(0.0,norm,1.0);
        const qreal needleDeg=startDeg+sweepDeg*norm;
        const qreal na=qDegreesToRadians(needleDeg);
        const QPointF tip=center+QPointF(qCos(na)*(radius-23*s),qSin(na)*(radius-23*s));
        const QPointF tail=center-QPointF(qCos(na)*9*s,qSin(na)*9*s);

        // Thin orange needle and small metallic hub: no oversized glow.
        p.setPen(QPen(QColor(255,104,0,75),4.2*s,Qt::SolidLine,Qt::RoundCap));
        p.drawLine(tail,tip);
        p.setPen(QPen(QColor("#ff7200"),1.8*s,Qt::SolidLine,Qt::RoundCap));
        p.drawLine(tail,tip);
        p.setPen(QPen(QColor("#56616b"),0.8*s));
        p.setBrush(QColor("#1c242b"));
        p.drawEllipse(center,5.0*s,5.0*s);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#ff7200"));
        p.drawEllipse(center,2.1*s,2.1*s);

        // Large central value and smaller unit, matching the hierarchy of the mock-up.
        QFont valueFont=p.font();
        valueFont.setPointSizeF(qBound<qreal>(10.0,14.5*s,18.0));
        valueFont.setBold(true);
        p.setFont(valueFont);
        p.setPen(QColor("#f4f6f7"));
        QString valueText;
        if (qAbs(value)<10.0 && qAbs(maxv-minv)<=40.0) valueText=QString::number(value,'f',1);
        else valueText=QString::number(value,'f',0);
        p.drawText(QRectF(center.x()-58*s,center.y()+14*s,116*s,24*s),Qt::AlignCenter,valueText);

        QString cleanSuffix=suffix;
        cleanSuffix.remove('['); cleanSuffix.remove(']');
        if (!cleanSuffix.trimmed().isEmpty()) {
            QFont unitFont=p.font();
            unitFont.setPointSizeF(qBound<qreal>(4.8,5.9*s,7.2));
            unitFont.setBold(false);
            p.setFont(unitFont);
            p.setPen(QColor("#a3adb5"));
            p.drawText(QRectF(center.x()-52*s,center.y()+35*s,104*s,12*s),Qt::AlignCenter,cleanSuffix.trimmed());
        }

        // Real state: NORMAL means the current value is inside the declared gauge range.
        QFont statusFont=p.font();
        statusFont.setPointSizeF(qBound<qreal>(4.8,5.6*s,6.8));
        statusFont.setBold(true);
        p.setFont(statusFont);
        p.setPen(inRange ? QColor("#63d66f") : QColor("#ff5a42"));
        p.drawText(QRectF(8*s,181*s,width()-16*s,14*s),Qt::AlignCenter,
                   inRange ? QStringLiteral("NORMAL") : QStringLiteral("HORS PLAGE"));

        // 2-minute trace integrated immediately below each dial.
        p.setPen(QPen(QColor("#253039"),line));
        p.setBrush(QColor("#090e13"));
        p.drawRoundedRect(trendRect,2.0*s,2.0*s);
        for (int i=1;i<4;++i) {
            const qreal y=trendRect.top()+trendRect.height()*i/4.0;
            p.setPen(QPen(QColor("#192229"),qMax<qreal>(0.6,0.7*s)));
            p.drawLine(trendRect.left()+3*s,y,trendRect.right()-3*s,y);
        }
        for (int i=1;i<6;++i) {
            const qreal x=trendRect.left()+trendRect.width()*i/6.0;
            p.setPen(QPen(QColor("#141c23"),qMax<qreal>(0.5,0.6*s)));
            p.drawLine(x,trendRect.top()+3*s,x,trendRect.bottom()-3*s);
        }

        if (m_history.size()>1) {
            double hmin=m_history.first().second,hmax=hmin;
            for (const auto &pt:m_history) { hmin=qMin(hmin,pt.second); hmax=qMax(hmax,pt.second); }
            if (qAbs(hmax-hmin)<0.001) { hmin-=1.0; hmax+=1.0; }
            const qint64 tmax=m_history.last().first;
            const qint64 tmin=qMax<qint64>(0,tmax-120000);
            QPainterPath path;
            bool first=true;
            for (const auto &pt:m_history) {
                if (pt.first<tmin) continue;
                const qreal x=trendRect.left()+3*s+((pt.first-tmin)/120000.0)*(trendRect.width()-6*s);
                const qreal y=trendRect.bottom()-3*s-((pt.second-hmin)/(hmax-hmin))*(trendRect.height()-6*s);
                if (first) { path.moveTo(x,y); first=false; }
                else path.lineTo(x,y);
            }
            p.setPen(QPen(QColor(255,114,0,55),3.0*s,Qt::SolidLine,Qt::RoundCap));
            p.drawPath(path);
            p.setPen(QPen(QColor("#ff7200"),qMax<qreal>(0.9,1.25*s),Qt::SolidLine,Qt::RoundCap));
            p.drawPath(path);
        }

        QFont traceFont=p.font();
        traceFont.setPointSizeF(qBound<qreal>(4.2,4.8*s,5.8));
        traceFont.setBold(false);
        p.setFont(traceFont);
        p.setPen(QColor("#59636c"));
        p.drawText(QRectF(trendRect.left()+4*s,trendRect.top()+2*s,35*s,9*s),Qt::AlignLeft,QStringLiteral("-2 min"));
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
                    QTimer::singleShot(0, window, [window,overview](){
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
