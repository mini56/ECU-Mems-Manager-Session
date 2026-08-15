#include <QApplication>
#include <QCoreApplication>
#include <QEvent>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLCDNumber>
#include <QLineEdit>
#include <QMainWindow>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QRadialGradient>
#include <QSizePolicy>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include <QtMath>
#include "i18n.h"

namespace {

static const QColor kText("#edf2f4");
static const QColor kOrange("#ff7a00");
static const QColor kRed("#ff3d32");

static void disposeLayoutItem(QLayoutItem *item)
{
    if(!item) return;
    if(QLayout *sub=item->layout()) {
        while(QLayoutItem *child=sub->takeAt(0)) disposeLayoutItem(child);
        delete sub;
        return;
    }
    delete item;
}

static void clearLayout(QLayout *layout)
{
    if(!layout) return;
    while(QLayoutItem *item=layout->takeAt(0)) disposeLayoutItem(item);
}

class SettingsGauge : public QWidget
{
public:
    enum Kind { IdleError, EngineRpm, FuelTrim, IgnitionAdvance, Lambda };

    SettingsGauge(QObject *source,const QString &title,const QString &unit,Kind kind,QWidget *parent=nullptr)
        : QWidget(parent),m_source(source),m_title(title),m_unit(unit),m_kind(kind)
    {
        setMinimumSize(kind==EngineRpm?260:145,kind==EngineRpm?260:145);
        setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
        setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
        setAttribute(Qt::WA_TransparentForMouseEvents,true);
        QTimer *timer=new QTimer(this);
        timer->setInterval(250);
        QObject::connect(timer,&QTimer::timeout,this,[this](){update();});
        timer->start();
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing,true);
        p.setRenderHint(QPainter::TextAntialiasing,true);

        const bool large=(m_kind==EngineRpm);
        const qreal bw=large?310.0:176.0;
        const qreal bh=large?300.0:166.0;
        const qreal scale=qMin(width()/bw,height()/bh);
        p.translate((width()-bw*scale)/2.0,(height()-bh*scale)/2.0);
        p.scale(scale,scale);

        QFont titleFont=p.font();
        titleFont.setBold(true);
        titleFont.setPointSizeF(large?9.0:7.2);
        p.setFont(titleFont);
        p.setPen(kText);
        p.drawText(QRectF(4,0,bw-8,20),Qt::AlignCenter,m_title.toUpper());

        const QPointF c(bw/2.0,large?151.0:87.0);
        const qreal r=large?126.0:68.0;
        const qreal startDeg=135.0;
        const qreal sweepDeg=270.0;
        const qreal arcR=r-10.0;

        QRadialGradient bezel(c-QPointF(r*.18,r*.22),r*1.15);
        bezel.setColorAt(0,QColor("#5b646b"));
        bezel.setColorAt(.18,QColor("#2c343a"));
        bezel.setColorAt(.58,QColor("#11181d"));
        bezel.setColorAt(.86,QColor("#070b0e"));
        bezel.setColorAt(1,QColor("#020304"));
        p.setPen(QPen(QColor("#6c7479"),large?2.0:1.35));
        p.setBrush(bezel);
        p.drawEllipse(c,r,r);
        p.setPen(QPen(QColor("#11191e"),large?8.0:5.0));
        p.setBrush(QColor("#05090c"));
        p.drawEllipse(c,r-6.0,r-6.0);
        p.setPen(QPen(QColor("#3b474f"),large?1.5:1.0));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(c,r-12.0,r-12.0);

        double value=m_source?m_source->property("value").toDouble():0.0;
        double minv=m_source?m_source->property("minimum").toDouble():0.0;
        double maxv=m_source?m_source->property("maximum").toDouble():100.0;
        if(!qIsFinite(minv)||!qIsFinite(maxv)||qFuzzyCompare(minv,maxv)) {
            if(m_kind==EngineRpm){minv=0.0;maxv=2000.0;}
            else if(m_kind==IdleError){minv=0.0;maxv=200.0;}
            else if(m_kind==FuelTrim){minv=-20.0;maxv=20.0;}
            else if(m_kind==IgnitionAdvance){minv=0.0;maxv=40.0;}
            else {minv=0.0;maxv=900.0;}
        }

        auto pointFor=[&](double fraction,qreal radius){
            const qreal angle=qDegreesToRadians(startDeg+sweepDeg*fraction);
            return c+QPointF(qCos(angle)*radius,qSin(angle)*radius);
        };
        auto drawRange=[&](double lo,double hi,const QColor &color,qreal width){
            if(hi<=lo || hi<=minv || lo>=maxv) return;
            lo=qMax(lo,minv);
            hi=qMin(hi,maxv);
            const qreal f0=(lo-minv)/(maxv-minv);
            const qreal f1=(hi-minv)/(maxv-minv);
            QPainterPath path;
            const int steps=36;
            for(int i=0;i<=steps;i++) {
                const qreal f=f0+(f1-f0)*i/qreal(steps);
                const QPointF pt=pointFor(f,arcR);
                if(i==0) path.moveTo(pt); else path.lineTo(pt);
            }
            p.setPen(QPen(color,width,Qt::SolidLine,Qt::RoundCap));
            p.drawPath(path);
        };

        QPainterPath baseArc;
        const int arcSteps=100;
        for(int i=0;i<=arcSteps;i++) {
            const QPointF pt=pointFor(i/qreal(arcSteps),arcR);
            if(i==0) baseArc.moveTo(pt); else baseArc.lineTo(pt);
        }
        p.setPen(QPen(QColor("#3a454d"),large?2.8:1.8,Qt::SolidLine,Qt::RoundCap));
        p.drawPath(baseArc);

        if(m_kind==EngineRpm) {
            const double redStart=(maxv>=1900.0)?1750.0:(minv+(maxv-minv)*.875);
            drawRange(redStart,maxv,kRed,7.0);
        } else if(m_kind==IdleError) {
            const double redStart=(maxv>=180.0)?100.0:(minv+(maxv-minv)*.60);
            drawRange(redStart,maxv,kRed,4.5);
        }

        const int majorCount=(m_kind==Lambda)?6:8;
        const int tickCount=majorCount*5;
        p.save();
        p.translate(c);
        for(int i=0;i<=tickCount;i++) {
            const bool major=(i%5==0);
            const qreal angle=qDegreesToRadians(startDeg+sweepDeg*i/qreal(tickCount));
            const qreal outer=r-(large?9.0:6.5);
            const qreal length=major?(large?15.0:9.0):(large?7.0:4.0);
            const QPointF po(qCos(angle)*outer,qSin(angle)*outer);
            const QPointF pi(qCos(angle)*(outer-length),qSin(angle)*(outer-length));
            p.setPen(QPen(major?QColor("#f3f5f6"):QColor("#89949b"),major?(large?2.0:1.2):(large?1.0:.62),Qt::SolidLine,Qt::FlatCap));
            p.drawLine(pi,po);
        }
        p.restore();

        QFont scaleFont=p.font();
        scaleFont.setBold(false);
        scaleFont.setPointSizeF(large?8.5:6.1);
        p.setFont(scaleFont);
        p.setPen(QColor("#eef2f4"));
        const qreal labelR=r-(large?38.0:25.0);
        for(int i=0;i<=majorCount;i++) {
            const double fv=minv+(maxv-minv)*i/qreal(majorCount);
            const QPointF pos=pointFor(i/qreal(majorCount),labelR);
            QString text;
            if(qAbs(maxv-minv)<=50.0) text=QString::number(fv,'f',0);
            else text=QString::number(qRound(fv));
            p.drawText(QRectF(pos.x()-(large?28:18),pos.y()-(large?9:7),large?56:36,large?18:14),Qt::AlignCenter,text);
        }

        const double normalized=qBound(0.0,(value-minv)/(maxv-minv),1.0);
        const qreal needleDeg=startDeg+sweepDeg*normalized;
        const qreal angle=qDegreesToRadians(needleDeg);
        const QPointF direction(qCos(angle),qSin(angle));
        const QPointF normal(-direction.y(),direction.x());
        QPainterPath needle;
        needle.moveTo(c+direction*(r-(large?24.0:14.0)));
        needle.lineTo(c+normal*(large?4.2:2.8));
        needle.lineTo(c-direction*(large?14.0:8.0));
        needle.lineTo(c-normal*(large?4.2:2.8));
        needle.closeSubpath();
        p.setPen(Qt::NoPen);
        p.setBrush(kOrange);
        p.drawPath(needle);
        p.setPen(QPen(QColor("#707980"),large?1.3:1.0));
        p.setBrush(QColor("#273038"));
        p.drawEllipse(c,large?10.0:6.0,large?10.0:6.0);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#11171b"));
        p.drawEllipse(c,large?6.0:3.5,large?6.0:3.5);

        QFont valueFont=p.font();
        valueFont.setBold(true);
        valueFont.setPointSizeF(large?23.0:13.0);
        p.setFont(valueFont);
        p.setPen(QColor("#ffffff"));
        const QString valueText=(qAbs(maxv-minv)<=50.0)?QString::number(value,'f',1):QString::number(value,'f',0);
        p.drawText(QRectF(c.x()-(large?62:40),c.y()+(large?58:26),large?124:80,large?32:22),Qt::AlignCenter,valueText);

        QFont unitFont=p.font();
        unitFont.setBold(true);
        unitFont.setPointSizeF(large?8.5:6.3);
        p.setFont(unitFont);
        p.setPen(QColor("#d4dbe0"));
        p.drawText(QRectF(c.x()-(large?55:36),c.y()+(large?83:45),large?110:72,large?17:13),Qt::AlignCenter,m_unit);
    }

private:
    QObject *m_source=nullptr;
    QString m_title;
    QString m_unit;
    Kind m_kind;
};

static void styleLed(QWidget *led)
{
    if(!led) return;
    led->setFixedSize(24,24);
    led->show();
}

static void styleAdjustButton(QPushButton *button)
{
    if(!button) return;
    button->setMinimumHeight(29);
    button->setMaximumHeight(34);
    button->setMinimumWidth(125);
    button->setMaximumWidth(170);
    button->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Fixed);
    button->show();
}

static void styleLcd(QLCDNumber *lcd)
{
    if(!lcd) return;
    lcd->setSegmentStyle(QLCDNumber::Flat);
    lcd->setDigitCount(4);
    lcd->setSmallDecimalPoint(true);
    lcd->setFixedSize(86,32);
    lcd->setStyleSheet(QStringLiteral("QLCDNumber{background:#080e13;color:#ff8a00;border:1px solid #34414b;border-radius:3px;}"));
    lcd->show();
}

static void deleteLegacyGaugeOverlays(QFrame *metrics)
{
    if(!metrics) return;
    const QList<QWidget*> widgets=metrics->findChildren<QWidget*>();
    for(QWidget *widget:widgets) {
        if(!widget) continue;
        const QString className=QString::fromLatin1(widget->metaObject()->className());
        if(className==QStringLiteral("CompactGauge")) {
            widget->hide();
            widget->deleteLater();
        }
    }
}

static void rebuildGaugeArea(QWidget *page,QFrame *metrics)
{
    if(!page || !metrics) return;
    QVBoxLayout *layout=qobject_cast<QVBoxLayout*>(metrics->layout());
    if(!layout) return;

    deleteLegacyGaugeOverlays(metrics);

    const QList<QWidget*> directChildren=metrics->findChildren<QWidget*>(QString(),Qt::FindDirectChildrenOnly);
    for(QWidget *child:directChildren) child->hide();
    clearLayout(layout);
    layout->setContentsMargins(7,4,7,4);
    layout->setSpacing(0);

    const char *sourceNames[]={"e_idle_error","m_revCounter_exhaust","e_short_term_fuel_trim","e_ignition_advance","e_lambda"};
    for(const char *name:sourceNames) {
        if(QWidget *source=page->findChild<QWidget*>(QString::fromLatin1(name))) {
            source->hide();
            if(QWidget *box=source->parentWidget()) box->hide();
        }
    }

    QObject *idleSrc=page->findChild<QObject*>(QStringLiteral("e_idle_error"));
    QObject *rpmSrc=page->findChild<QObject*>(QStringLiteral("m_revCounter_exhaust"));
    QObject *trimSrc=page->findChild<QObject*>(QStringLiteral("e_short_term_fuel_trim"));
    QObject *ignSrc=page->findChild<QObject*>(QStringLiteral("e_ignition_advance"));
    QObject *lambdaSrc=page->findChild<QObject*>(QStringLiteral("e_lambda"));

    SettingsGauge *idle=new SettingsGauge(idleSrc,I18n::text(7139),QStringLiteral("tr/min"),SettingsGauge::IdleError,metrics);
    SettingsGauge *rpm=new SettingsGauge(rpmSrc,I18n::text(1011),QStringLiteral("tr/min"),SettingsGauge::EngineRpm,metrics);
    SettingsGauge *trim=new SettingsGauge(trimSrc,I18n::text(7140),QStringLiteral("%"),SettingsGauge::FuelTrim,metrics);
    SettingsGauge *ign=new SettingsGauge(ignSrc,I18n::text(7141),QString::fromUtf8("°"),SettingsGauge::IgnitionAdvance,metrics);
    SettingsGauge *lambda=new SettingsGauge(lambdaSrc,I18n::text(7142),QStringLiteral("mV"),SettingsGauge::Lambda,metrics);

    idle->setObjectName(QStringLiteral("darkTuneGaugeIdle"));
    rpm->setObjectName(QStringLiteral("darkTuneGaugeRpm"));
    trim->setObjectName(QStringLiteral("darkTuneGaugeTrim"));
    ign->setObjectName(QStringLiteral("darkTuneGaugeIgnition"));
    lambda->setObjectName(QStringLiteral("darkTuneGaugeLambda"));

    QGridLayout *grid=new QGridLayout;
    grid->setContentsMargins(2,0,2,0);
    grid->setHorizontalSpacing(18);
    grid->setVerticalSpacing(0);
    grid->addWidget(idle,0,0,Qt::AlignCenter);
    grid->addWidget(ign,1,0,Qt::AlignCenter);
    grid->addWidget(rpm,0,1,2,1,Qt::AlignCenter);
    grid->addWidget(trim,0,2,Qt::AlignCenter);
    grid->addWidget(lambda,1,2,Qt::AlignCenter);
    grid->setColumnStretch(0,10);
    grid->setColumnStretch(1,18);
    grid->setColumnStretch(2,10);
    grid->setRowStretch(0,1);
    grid->setRowStretch(1,1);
    layout->addLayout(grid,1);
}

static void rebuildAdjustArea(QWidget *page,QFrame *states,QFrame *adjust,QLineEdit *note)
{
    if(!page || !adjust) return;

    QWidget *idleLed=page->findChild<QWidget*>(QStringLiteral("idleswitch_led"));
    QWidget *loopLed=page->findChild<QWidget*>(QStringLiteral("closedloop_led"));

    QLabel *fuelLabel=page->findChild<QLabel*>(QStringLiteral("emi_shorttermftrim"));
    QLabel *hotLabel=page->findChild<QLabel*>(QStringLiteral("emi_idledecay"));
    QLabel *rpmLabel=page->findChild<QLabel*>(QStringLiteral("emi_rpm"));
    QLabel *ignLabel=page->findChild<QLabel*>(QStringLiteral("emi_ignadvance"));

    QPushButton *fuelMinus=page->findChild<QPushButton*>(QStringLiteral("m_fuel_trim_minusButton"));
    QPushButton *fuelPlus=page->findChild<QPushButton*>(QStringLiteral("m_fuel_trim_plusButton"));
    QPushButton *hotMinus=page->findChild<QPushButton*>(QStringLiteral("m_idle_decay_minusButton"));
    QPushButton *hotPlus=page->findChild<QPushButton*>(QStringLiteral("m_idle_decay_plusButton"));
    QPushButton *rpmMinus=page->findChild<QPushButton*>(QStringLiteral("m_idle_speed_minusButton"));
    QPushButton *rpmPlus=page->findChild<QPushButton*>(QStringLiteral("m_idle_speed_plusButton"));
    QPushButton *ignMinus=page->findChild<QPushButton*>(QStringLiteral("m_ignition_advance_minusButton"));
    QPushButton *ignPlus=page->findChild<QPushButton*>(QStringLiteral("m_ignition_advance_plusButton"));

    QLCDNumber *fuelLcd=page->findChild<QLCDNumber*>(QStringLiteral("e_FuelTrim_lcd"));
    QLCDNumber *hotLcd=page->findChild<QLCDNumber*>(QStringLiteral("e_IdleDecay_lcd"));
    QLCDNumber *rpmLcd=page->findChild<QLCDNumber*>(QStringLiteral("e_IdleSpeed_lcd"));
    QLCDNumber *ignLcd=page->findChild<QLCDNumber*>(QStringLiteral("e_IgnitionAdvance_lcd"));

    QPushButton *resetAdj=page->findChild<QPushButton*>(QStringLiteral("m_resetAdjustmentsButton"));
    QPushButton *resetEcu=page->findChild<QPushButton*>(QStringLiteral("m_resetECUButton"));

    if(states) states->hide();

    QVBoxLayout *layout=qobject_cast<QVBoxLayout*>(adjust->layout());
    if(!layout) return;
    clearLayout(layout);
    for(QWidget *child:adjust->findChildren<QWidget*>(QString(),Qt::FindDirectChildrenOnly)) child->hide();
    layout->setContentsMargins(14,8,14,9);
    layout->setSpacing(5);

    QHBoxLayout *header=new QHBoxLayout;
    header->setContentsMargins(0,0,0,0);
    header->setSpacing(9);

    QLabel *title=new QLabel(I18n::text(7135),adjust);
    QFont titleFont=title->font();
    titleFont.setBold(true);
    titleFont.setPointSizeF(9.0);
    title->setFont(titleFont);
    title->setStyleSheet(QStringLiteral("color:#ff9828;background:transparent;border:0;"));
    header->addWidget(title);
    header->addSpacing(18);

    if(loopLed) {
        loopLed->setParent(adjust);
        styleLed(loopLed);
        header->addWidget(loopLed);
        QLabel *loopText=new QLabel(I18n::text(2014),adjust);
        loopText->setStyleSheet(QStringLiteral("color:#dce3e8;background:transparent;border:0;"));
        header->addWidget(loopText);
    }
    header->addSpacing(14);
    if(idleLed) {
        idleLed->setParent(adjust);
        styleLed(idleLed);
        header->addWidget(idleLed);
        QLabel *idleText=new QLabel(I18n::text(2015),adjust);
        idleText->setStyleSheet(QStringLiteral("color:#dce3e8;background:transparent;border:0;"));
        header->addWidget(idleText);
    }

    header->addStretch(1);
    if(note) {
        note->setParent(adjust);
        note->setReadOnly(true);
        note->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
        note->setMinimumWidth(280);
        note->setMaximumWidth(470);
        note->setMinimumHeight(24);
        note->setMaximumHeight(28);
        note->setStyleSheet(QStringLiteral("QLineEdit{background:transparent;color:#b8c1c7;border:0;padding:1px 2px;}"));
        note->show();
        header->addWidget(note,0);
    }
    layout->addLayout(header);

    QFrame *line=new QFrame(adjust);
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet(QStringLiteral("background:#29343e;border:0;max-height:1px;"));
    layout->addWidget(line);

    QWidget *form=new QWidget(adjust);
    form->setObjectName(QStringLiteral("darkSettingsAdjustForm"));
    form->setMaximumWidth(900);
    form->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Preferred);
    QGridLayout *grid=new QGridLayout(form);
    grid->setContentsMargins(0,0,0,0);
    grid->setHorizontalSpacing(10);
    grid->setVerticalSpacing(4);
    grid->setColumnStretch(0,0);
    grid->setColumnStretch(1,0);
    grid->setColumnStretch(2,0);
    grid->setColumnStretch(3,0);
    grid->setColumnStretch(4,0);

    struct Row {
        QLabel *label;
        QPushButton *minus;
        QLCDNumber *lcd;
        const char *unit;
        QPushButton *plus;
    };
    Row rows[]={
        {fuelLabel,fuelMinus,fuelLcd,"%",fuelPlus},
        {hotLabel,hotMinus,hotLcd,"pas",hotPlus},
        {rpmLabel,rpmMinus,rpmLcd,"pas",rpmPlus},
        {ignLabel,ignMinus,ignLcd,"°",ignPlus}
    };

    for(int row=0;row<4;row++) {
        if(rows[row].label) {
            rows[row].label->setParent(form);
            rows[row].label->setMinimumWidth(210);
            rows[row].label->setMaximumWidth(240);
            rows[row].label->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
            rows[row].label->setStyleSheet(QStringLiteral("color:#e6ecef;background:transparent;border:0;"));
            rows[row].label->show();
            grid->addWidget(rows[row].label,row,0);
        }
        if(rows[row].minus) {
            rows[row].minus->setParent(form);
            styleAdjustButton(rows[row].minus);
            grid->addWidget(rows[row].minus,row,1);
        }
        if(rows[row].lcd) {
            rows[row].lcd->setParent(form);
            styleLcd(rows[row].lcd);
            grid->addWidget(rows[row].lcd,row,2);
        }
        QLabel *unitLabel=new QLabel(QString::fromUtf8(rows[row].unit),form);
        QFont unitFont=unitLabel->font();
        unitFont.setBold(true);
        unitFont.setPointSizeF(8.4);
        unitLabel->setFont(unitFont);
        unitLabel->setFixedWidth(38);
        unitLabel->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
        unitLabel->setStyleSheet(QStringLiteral("color:#d8dfe3;background:transparent;border:0;"));
        grid->addWidget(unitLabel,row,3);
        if(rows[row].plus) {
            rows[row].plus->setParent(form);
            styleAdjustButton(rows[row].plus);
            grid->addWidget(rows[row].plus,row,4);
        }
    }

    QHBoxLayout *formCenter=new QHBoxLayout;
    formCenter->setContentsMargins(0,0,0,0);
    formCenter->addStretch(1);
    formCenter->addWidget(form,0,Qt::AlignHCenter);
    formCenter->addStretch(1);
    layout->addLayout(formCenter,1);

    QHBoxLayout *resets=new QHBoxLayout;
    resets->setContentsMargins(0,0,0,0);
    resets->addStretch(1);
    if(resetAdj) {
        resetAdj->setParent(adjust);
        resetAdj->setMinimumHeight(29);
        resetAdj->setMaximumHeight(34);
        resetAdj->show();
        resets->addWidget(resetAdj);
    }
    if(resetEcu) {
        resetEcu->setParent(adjust);
        resetEcu->setMinimumHeight(29);
        resetEcu->setMaximumHeight(34);
        resetEcu->show();
        resets->addWidget(resetEcu);
    }
    resets->addStretch(1);
    layout->addLayout(resets);

    adjust->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
    adjust->show();
}

static void rebuildPageLayout(QWidget *page,QFrame *metrics,QFrame *states,QFrame *adjust)
{
    if(!page || !metrics || !adjust) return;

    QGridLayout *body=nullptr;
    for(QGridLayout *grid:page->findChildren<QGridLayout*>()) {
        if(grid->indexOf(metrics)>=0 || grid->indexOf(adjust)>=0) {
            body=grid;
            break;
        }
    }
    if(!body) return;

    body->removeWidget(metrics);
    if(states) body->removeWidget(states);
    body->removeWidget(adjust);
    body->addWidget(metrics,0,0,1,2);
    body->addWidget(adjust,1,0,1,2);
    body->setColumnStretch(0,1);
    body->setColumnStretch(1,1);
    body->setRowStretch(0,3);
    body->setRowStretch(1,2);
    body->setHorizontalSpacing(0);
    body->setVerticalSpacing(7);
}

static void sizeGauge(QWidget *gauge,int size)
{
    if(!gauge) return;
    gauge->setMinimumSize(size,size);
    gauge->setMaximumSize(size,size);
    gauge->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
    gauge->show();
}

static void applySettingsSizing(QWidget *page,QFrame *metrics,QFrame *adjust)
{
    if(!page || !metrics || !adjust) return;

    const int width=qMax(700,page->width());
    const int height=qMax(430,page->height());
    const int centerByWidth=qRound(width*.285);
    const int centerByHeight=qRound((height-175)*.91);
    const int center=qBound(270,qMin(centerByWidth,centerByHeight),335);
    const int outer=qBound(155,qRound(center*.57),192);

    sizeGauge(page->findChild<QWidget*>(QStringLiteral("darkTuneGaugeRpm")),center);
    sizeGauge(page->findChild<QWidget*>(QStringLiteral("darkTuneGaugeIdle")),outer);
    sizeGauge(page->findChild<QWidget*>(QStringLiteral("darkTuneGaugeTrim")),outer);
    sizeGauge(page->findChild<QWidget*>(QStringLiteral("darkTuneGaugeIgnition")),outer);
    sizeGauge(page->findChild<QWidget*>(QStringLiteral("darkTuneGaugeLambda")),outer);

    const int metricsHeight=qBound(292,center+12,350);
    metrics->setMinimumHeight(metricsHeight);
    metrics->setMaximumHeight(metricsHeight+6);
    metrics->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);

    const int adjustHeight=qBound(168,qRound(height*.31),205);
    adjust->setMinimumHeight(adjustHeight);
    adjust->setMaximumHeight(adjustHeight+12);
}

static void suppressLegacyOverlays(QFrame *metrics)
{
    deleteLegacyGaugeOverlays(metrics);
    if(!metrics) return;
    for(QWidget *child:metrics->findChildren<QWidget*>(QString(),Qt::FindDirectChildrenOnly)) {
        const QString name=child->objectName();
        if(name.startsWith(QStringLiteral("darkTuneGauge"))) continue;
        if(QString::fromLatin1(child->metaObject()->className())==QStringLiteral("SettingsGauge")) continue;
        child->hide();
    }
}

static void applySettingsLayout(QMainWindow *window)
{
    if(!window) return;
    QWidget *page=window->findChild<QWidget*>(QStringLiteral("emission_tab"));
    QFrame *metrics=window->findChild<QFrame*>(QStringLiteral("settingsMetrics"));
    QFrame *states=window->findChild<QFrame*>(QStringLiteral("settingsStates"));
    QFrame *adjust=window->findChild<QFrame*>(QStringLiteral("settingsAdjust"));
    if(!page || !metrics || !adjust) return;

    if(!page->property("settingsDark410Built").toBool()) {
        page->setProperty("settingsDark410Built",true);
        QLineEdit *note=page->findChild<QLineEdit*>(QStringLiteral("lineEdit_3"));
        rebuildGaugeArea(page,metrics);
        rebuildAdjustArea(page,states,adjust,note);
        rebuildPageLayout(page,metrics,states,adjust);
    }

    suppressLegacyOverlays(metrics);

    page->setMinimumSize(0,0);
    page->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
    page->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    if(QLayout *root=page->layout()) {
        root->setContentsMargins(6,5,6,5);
        root->setSpacing(6);
        root->setSizeConstraint(QLayout::SetDefaultConstraint);
    }

    applySettingsSizing(page,metrics,adjust);
}

class SettingsTabDarkInstaller : public QObject
{
public:
    explicit SettingsTabDarkInstaller(QObject *parent=nullptr):QObject(parent) {}

protected:
    bool eventFilter(QObject *watched,QEvent *event) override
    {
        QMainWindow *window=qobject_cast<QMainWindow*>(watched);
        if(!window || window->objectName()!=QStringLiteral("MainWindow"))
            return QObject::eventFilter(watched,event);

        if((event->type()==QEvent::Show || event->type()==QEvent::Polish) &&
           !window->property("settingsTabDarkScheduled").toBool()) {
            window->setProperty("settingsTabDarkScheduled",true);
            QTimer::singleShot(2050,window,[window](){applySettingsLayout(window);});
            QTimer::singleShot(2450,window,[window](){applySettingsLayout(window);});
        } else if(event->type()==QEvent::Resize && window->property("settingsTabDarkScheduled").toBool()) {
            QTimer::singleShot(240,window,[window](){applySettingsLayout(window);});
        }
        return QObject::eventFilter(watched,event);
    }
};

void installSettingsTabDark()
{
    QApplication *app=qobject_cast<QApplication*>(QCoreApplication::instance());
    if(app) app->installEventFilter(new SettingsTabDarkInstaller(app));
}

}

Q_COREAPP_STARTUP_FUNCTION(installSettingsTabDark)
