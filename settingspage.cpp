#include <QFrame>
#include <QFontMetrics>
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
#include "settingspage.h"
#include "applicationvisualstyle.h"

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

    SettingsGauge(QObject *source,const QString &title,const QString &unit,Kind kind,
                  qreal titleSize,QWidget *parent=nullptr)
        : QWidget(parent),m_source(source),m_title(title),m_unit(unit),m_kind(kind),
          m_titleSize(titleSize)
    {
        setMinimumSize(kind==EngineRpm?260:145,kind==EngineRpm?260:145);
        setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
        setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
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
        // Keep the RPM caption in its own bottom strip below the dial.
        const qreal dialHeight=qMax<qreal>(1,height()-(large?30:0));
        const qreal scale=qMin(width()/bw,dialHeight/bh);
        p.translate((width()-bw*scale)/2.0,(dialHeight-bh*scale)/2.0);
        p.scale(scale,scale);

        const QString gaugeTitle=m_title.toUpper();
        const QFont titleFont=GaugeHeading::styledFont(p.font(),m_titleSize);
        p.setFont(titleFont);
        p.setPen(kText);
        p.drawText(QRectF(4,0,bw-8,20),Qt::AlignCenter,gaugeTitle);

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
        scaleFont.setPointSizeF(large?9.2:7.3);
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
        valueFont.setPointSizeF(large?23.0:14.5);
        p.setFont(valueFont);
        p.setPen(QColor("#ffffff"));
        const QString valueText=(qAbs(maxv-minv)<=50.0)?QString::number(value,'f',1):QString::number(value,'f',0);
        p.drawText(QRectF(c.x()-(large?62:40),c.y()+(large?58:26),large?124:80,large?32:22),Qt::AlignCenter,valueText);

        QFont unitFont=p.font();
        unitFont.setBold(true);
        unitFont.setPointSizeF(large?9.2:7.6);
        p.setFont(unitFont);
        p.setPen(QColor("#d4dbe0"));
        p.drawText(QRectF(c.x()-(large?55:36),c.y()+(large?83:45),large?110:72,large?17:13),Qt::AlignCenter,m_unit);
    }

private:
    QObject *m_source=nullptr;
    QString m_title;
    QString m_unit;
    Kind m_kind;
    qreal m_titleSize=8.0;
};

static void styleAdjustButton(QPushButton *button)
{
    if(!button) return;
    // Allow the full button label to fit above and below the text baseline.
    button->setMinimumHeight(36);
    button->setMaximumHeight(42);
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

static void installPositiveLcdSign(QLCDNumber *lcd)
{
    if(!lcd || lcd->findChild<QWidget*>(QStringLiteral("ignitionAdjustmentPositiveSign"),Qt::FindDirectChildrenOnly)) return;

    QWidget *sign=new QWidget(lcd);
    sign->setObjectName(QStringLiteral("ignitionAdjustmentPositiveSign"));
    sign->setAttribute(Qt::WA_TransparentForMouseEvents,true);
    sign->setGeometry(5,1,14,22);
    sign->setStyleSheet(QStringLiteral("background:transparent;border:0;"));

    QFrame *horizontal=new QFrame(sign);
    horizontal->setGeometry(1,10,12,2);
    horizontal->setStyleSheet(QStringLiteral("background:#ff8a00;border:0;"));
    QFrame *vertical=new QFrame(sign);
    vertical->setGeometry(6,5,2,12);
    vertical->setStyleSheet(QStringLiteral("background:#ff8a00;border:0;"));

    sign->hide();
    QTimer *timer=new QTimer(sign);
    timer->setInterval(100);
    QObject::connect(timer,&QTimer::timeout,sign,[lcd,sign](){
        const bool positive=lcd->value()>0.0001;
        sign->setVisible(positive);
        if(positive) sign->raise();
    });
    timer->start();
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

    const qreal titleSize=GaugeHeading::sharedPointSize(page->font(),
        QStringList{I18n::text(7139),I18n::text(1011),I18n::text(7140),
                    I18n::text(7141),I18n::text(7142)},164.0);
    SettingsGauge *idle=new SettingsGauge(idleSrc,I18n::text(7139),QStringLiteral("tr/min"),SettingsGauge::IdleError,titleSize,metrics);
    SettingsGauge *rpm=new SettingsGauge(rpmSrc,I18n::text(1011),QStringLiteral("tr/min"),SettingsGauge::EngineRpm,titleSize,metrics);
    SettingsGauge *trim=new SettingsGauge(trimSrc,I18n::text(7140),QStringLiteral("%"),SettingsGauge::FuelTrim,titleSize,metrics);
    SettingsGauge *ign=new SettingsGauge(ignSrc,I18n::text(7141),QString::fromUtf8("°"),SettingsGauge::IgnitionAdvance,titleSize,metrics);
    SettingsGauge *lambda=new SettingsGauge(lambdaSrc,I18n::text(7142),QStringLiteral("mV"),SettingsGauge::Lambda,titleSize,metrics);

    idle->setObjectName(QStringLiteral("darkTuneGaugeIdle"));
    rpm->setObjectName(QStringLiteral("darkTuneGaugeRpm"));
    trim->setObjectName(QStringLiteral("darkTuneGaugeTrim"));
    ign->setObjectName(QStringLiteral("darkTuneGaugeIgnition"));
    lambda->setObjectName(QStringLiteral("darkTuneGaugeLambda"));

    QGridLayout *grid=new QGridLayout;
    grid->setContentsMargins(2,0,2,0);
    grid->setHorizontalSpacing(18);
    grid->setVerticalSpacing(0);
    // Let the grid allocate the full cell to each gauge.  Center alignment
    // constrains widgets to sizeHint/minimumSize and caused the five dials
    // to shrink despite having plenty of available space.
    grid->addWidget(idle,0,0);
    grid->addWidget(ign,1,0);
    grid->addWidget(rpm,0,1,2,1);
    grid->addWidget(trim,0,2);
    grid->addWidget(lambda,1,2);
    grid->setColumnStretch(0,10);
    grid->setColumnStretch(1,18);
    grid->setColumnStretch(2,10);
    grid->setRowStretch(0,1);
    grid->setRowStretch(1,1);
    layout->addLayout(grid,1);
}

static void rebuildAdjustArea(QWidget *page,QFrame *adjust)
{
    if(!page || !adjust) return;

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

    QVBoxLayout *layout=qobject_cast<QVBoxLayout*>(adjust->layout());
    if(!layout) return;
    clearLayout(layout);
    for(QWidget *child:adjust->findChildren<QWidget*>(QString(),Qt::FindDirectChildrenOnly)) child->hide();
    layout->setContentsMargins(14,8,14,9);
    layout->setSpacing(5);

    // The title belongs directly to the panel layout.  A nested QWidget
    // inherited a solid dark background and appeared as an unwanted band.
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
    header->addStretch(1);
    layout->addLayout(header);

    QFrame *line=new QFrame(adjust);
    line->setFrameShape(QFrame::HLine);
    line->setFixedHeight(1);
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
            if(rows[row].lcd==ignLcd) installPositiveLcdSign(ignLcd);
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
        resetAdj->setMinimumHeight(36);
        resetAdj->setMaximumHeight(42);
        resetAdj->show();
        resets->addWidget(resetAdj);
    }
    if(resetEcu) {
        resetEcu->setParent(adjust);
        resetEcu->setMinimumHeight(36);
        resetEcu->setMaximumHeight(42);
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

static void prepareSettingsGauge(QWidget *gauge,bool precision)
{
    if(!gauge)return;
    // The grid owns gauge geometry on every resize: no stale pixel caps.
    gauge->setMinimumSize(precision?270:150,precision?280:145);
    gauge->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
    gauge->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    gauge->setAttribute(Qt::WA_StyledBackground,true);
    gauge->setStyleSheet(QStringLiteral(
        "background:#080d11;border:1px solid #34414a;border-radius:5px;"));
    gauge->show();
}

static void applySettingsSizing(QWidget *page,QFrame *metrics,QFrame *adjust)
{
    if(!page || !metrics || !adjust) return;

    // The central precision RPM card spans both rows and fills its cell.
    // Free space is allocated by Qt's current layout, not startup geometry.
    for(const char *name:{"darkTuneGaugeIdle","darkTuneGaugeTrim",
                          "darkTuneGaugeIgnition","darkTuneGaugeLambda"})
        prepareSettingsGauge(page->findChild<QWidget*>(QString::fromLatin1(name)),false);
    prepareSettingsGauge(page->findChild<QWidget*>(QStringLiteral("darkTuneGaugeRpm")),true);

    metrics->setMinimumHeight(280);
    metrics->setMaximumHeight(QWIDGETSIZE_MAX);
    metrics->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);

    // Four control rows remain compact; spare height belongs to the dials.
    const int controlsHeight=qMax(210,adjust->minimumSizeHint().height());
    adjust->setMinimumHeight(controlsHeight);
    adjust->setMaximumHeight(controlsHeight);
    adjust->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
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

static void compactAdjustForm(QWidget *page,QFrame *adjust)
{
    if(!page || !adjust) return;

    QWidget *form=page->findChild<QWidget*>(QStringLiteral("darkSettingsAdjustForm"));
    if(form) {
        form->setMaximumWidth(760);
        form->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Fixed);
        form->setStyleSheet(QStringLiteral("background:transparent;border:0;"));

        if(QGridLayout *grid=qobject_cast<QGridLayout*>(form->layout())) {
            grid->setContentsMargins(0,0,0,0);
            grid->setVerticalSpacing(4);
        }

        const QList<QPushButton*> buttons=form->findChildren<QPushButton*>();
        for(QPushButton *button:buttons) {
            button->setMinimumWidth(108);
            button->setMaximumWidth(142);
            button->setMinimumHeight(36);
            button->setMaximumHeight(42);
        }

        const QList<QLCDNumber*> lcds=form->findChildren<QLCDNumber*>();
        for(QLCDNumber *lcd:lcds) {
            lcd->setMinimumHeight(32);
            lcd->setMaximumHeight(32);
        }

        const QList<QLabel*> labels=form->findChildren<QLabel*>();
        for(QLabel *label:labels) {
            label->setMinimumHeight(30);
            label->setMaximumHeight(42);
            if(label->minimumWidth()>=200) {
                label->setMinimumWidth(185);
                label->setMaximumWidth(215);
            }
        }
    }

    const QList<QPushButton*> resetButtons=adjust->findChildren<QPushButton*>(QString(),Qt::FindDirectChildrenOnly);
    for(QPushButton *button:resetButtons) {
        button->setMinimumHeight(36);
        button->setMaximumHeight(42);
    }

    if(QLineEdit *note=page->findChild<QLineEdit*>(QStringLiteral("lineEdit_3"))) {
        note->setMinimumWidth(0);
        note->setMaximumWidth(QWIDGETSIZE_MAX);
        note->setMinimumHeight(24);
        note->setMaximumHeight(27);
        note->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Fixed);
        note->setAlignment(Qt::AlignCenter);
        QFont font=note->font();
        font.setPointSizeF(8.0);
        note->setFont(font);
        note->setStyleSheet(QStringLiteral(
            "QLineEdit{background:transparent;color:#b8c1c7;"
            "border:0;padding:0 2px;}"));
    }
}

static void embedNoteInRpmGauge(QWidget *page,QFrame *metrics,QFrame *adjust)
{
    if(!page || !metrics || !adjust) return;

    QLineEdit *note=page->findChild<QLineEdit*>(QStringLiteral("lineEdit_3"));
    QWidget *rpm=metrics->findChild<QWidget*>(QStringLiteral("darkTuneGaugeRpm"));
    if(!note || !rpm) return;

    // This is part of the gauge's layout, not a late geometry correction.
    // The bottom-aligned line always follows gauge resizing.
    for(QHBoxLayout *row:adjust->findChildren<QHBoxLayout*>())
        if(row->indexOf(note)>=0) row->removeWidget(note);

    note->setParent(rpm);
    note->setReadOnly(true);
    note->setMinimumHeight(24);
    note->setMaximumHeight(27);
    note->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);

    QVBoxLayout *layout=qobject_cast<QVBoxLayout*>(rpm->layout());
    if(!layout) {
        layout=new QVBoxLayout(rpm);
        layout->setContentsMargins(8,0,8,6);
        layout->setSpacing(0);
        layout->addStretch(1);
    }
    if(layout->indexOf(note)<0)
        layout->addWidget(note);
    note->show();
}

static void arrangeBottomPanels(QWidget *page,QFrame *metrics,QFrame *states,QFrame *adjust)
{
    if(!page || !metrics || !states || !adjust) return;

    QGridLayout *body=nullptr;
    for(QGridLayout *grid:page->findChildren<QGridLayout*>()) {
        if(grid->indexOf(metrics)>=0 && grid->indexOf(adjust)>=0) {
            body=grid;
            break;
        }
    }
    if(!body) return;

    // The metrics frame spans both body columns so the gauge group stays
    // centered while the two lower panels share the full available width.
    body->removeWidget(metrics);
    body->removeWidget(states);
    body->removeWidget(adjust);
    body->addWidget(metrics,0,0,1,2);
    body->addWidget(states,1,0);
    states->show();
    body->addWidget(adjust,1,1);
    body->setColumnStretch(0,1);
    body->setColumnStretch(1,4);
    // The precision dial owns any spare vertical room; controls keep their
    // natural height rather than expanding into an oversized lower band.
    body->setRowStretch(0,1);
    body->setRowStretch(1,0);
    body->setHorizontalSpacing(8);
    body->setVerticalSpacing(4);

    states->setMinimumWidth(205);
    states->setMaximumWidth(275);
    states->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Expanding);
    adjust->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
}

static void shortenSettingsSeparator(QFrame *adjust)
{
    if(!adjust) return;
    QVBoxLayout *layout=qobject_cast<QVBoxLayout*>(adjust->layout());
    if(!layout) return;

    for(QFrame *line:adjust->findChildren<QFrame*>(QString(),Qt::FindDirectChildrenOnly)) {
        if(line->frameShape()!=QFrame::HLine) continue;
        line->setMinimumWidth(0);
        line->setMaximumWidth(650);
        line->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Fixed);
        layout->setAlignment(line,Qt::AlignLeft);
        break;
    }
}

// The Settings page is owned here, including its initial BUILD117 scaffold.
// No other page module is permitted to rebuild its layouts or its gauges.
static QFrame *settingsScaffoldCard(QWidget *parent,const QString &name,const QString &title)
{
    QFrame *c=new QFrame(parent);
    c->setObjectName(name);
    c->setAttribute(Qt::WA_StyledBackground,true);
    c->setStyleSheet(QStringLiteral("#%1{background:#10161c;border:1px solid #29343e;border-radius:5px;}").arg(name));
    QVBoxLayout *v=new QVBoxLayout(c);
    v->setContentsMargins(14,12,14,14);
    v->setSpacing(8);
    QLabel *t=new QLabel(title,c);
    QFont font=t->font();font.setBold(true);font.setPointSizeF(9.5);t->setFont(font);
    t->setStyleSheet(QStringLiteral("color:#ff9828;background:transparent;border:0;"));
    v->addWidget(t);
    QFrame *line=new QFrame(c);line->setFrameShape(QFrame::HLine);
    line->setStyleSheet(QStringLiteral("background:#29343e;border:0;max-height:1px;"));
    v->addWidget(line);
    return c;
}
static void beginSettingsScaffold(QWidget *page,QVBoxLayout *&root)
{
    QFrame *header=nullptr;
    for(QFrame *f:page->findChildren<QFrame*>(QString(),Qt::FindDirectChildrenOnly)) {
        if(f->objectName().startsWith(QStringLiteral("uiRebuildHero_"))){header=f;break;}
    }
    QLayout *old=page->layout();
    if(old){clearLayout(old);delete old;}
    root=new QVBoxLayout(page);
    root->setContentsMargins(15,12,15,12);
    root->setSpacing(10);
    if(header){header->show();root->addWidget(header);}
    for(QWidget *w:page->findChildren<QWidget*>(QString(),Qt::FindDirectChildrenOnly))
        if(w!=header)w->hide();
}

static void buildSettingsScaffold(QMainWindow *window)
{
    QWidget *page=window?window->findChild<QWidget*>(QStringLiteral("emission_tab")):nullptr;
    if(!page||page->property("majorPageRebuilt").toBool())return;
    page->setProperty("majorPageRebuilt",true);

    QWidget *rpm=page->findChild<QWidget*>(QStringLiteral("m_revCounter_exhaust"));
    QWidget *idleErr=page->findChild<QWidget*>(QStringLiteral("e_idle_error"));
    QWidget *advance=page->findChild<QWidget*>(QStringLiteral("e_ignition_advance"));
    QWidget *trim=page->findChild<QWidget*>(QStringLiteral("e_short_term_fuel_trim"));
    QWidget *lambda=page->findChild<QWidget*>(QStringLiteral("e_lambda"));
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
    QLineEdit *note=page->findChild<QLineEdit*>(QStringLiteral("lineEdit_3"));

    QVBoxLayout *root=nullptr;beginSettingsScaffold(page,root);
    QGridLayout *body=new QGridLayout;body->setHorizontalSpacing(10);body->setVerticalSpacing(10);body->setColumnStretch(0,3);body->setColumnStretch(1,2);

    QFrame *metrics=settingsScaffoldCard(page,QStringLiteral("settingsMetrics"),I18n::text(7133));
    QGridLayout *mg=new QGridLayout;mg->setSpacing(8);
    QWidget *gauges[]={idleErr,rpm,trim,advance,lambda};
    const int titleKeys[]={7139,1011,7140,7141,7142};
    for(int i=0;i<5;i++){
        QFrame *box=new QFrame(metrics);box->setStyleSheet(QStringLiteral("background:#0b1116;border:1px solid #27323b;border-radius:4px;"));
        QVBoxLayout *bv=new QVBoxLayout(box);bv->setContentsMargins(6,6,6,6);bv->setSpacing(3);
        QLabel *tl=new QLabel(I18n::text(titleKeys[i]),box);QFont tf=tl->font();tf.setBold(true);tf.setPointSizeF(7.2);tl->setFont(tf);tl->setAlignment(Qt::AlignCenter);tl->setStyleSheet(QStringLiteral("color:#cfd6dc;background:transparent;border:0;"));bv->addWidget(tl);
        if(gauges[i]){gauges[i]->setParent(box);gauges[i]->setMinimumHeight(80);gauges[i]->setMaximumHeight(130);gauges[i]->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);gauges[i]->show();bv->addWidget(gauges[i],1);}
        mg->addWidget(box,i<3?0:1,i<3?i:i-3);
    }
    static_cast<QVBoxLayout*>(metrics->layout())->addLayout(mg,1);
    body->addWidget(metrics,0,0,2,1);

    QFrame *states=settingsScaffoldCard(page,QStringLiteral("settingsStates"),I18n::text(7134));
    QGridLayout *sg=new QGridLayout;sg->setHorizontalSpacing(12);sg->setVerticalSpacing(10);
    QLabel *idleTxt=new QLabel(I18n::text(2015),states);QLabel *loopTxt=new QLabel(I18n::text(2014),states);
    if(idleLed){idleLed->setParent(states);idleLed->setFixedSize(26,26);idleLed->show();sg->addWidget(idleLed,0,0);}sg->addWidget(idleTxt,0,1);
    if(loopLed){loopLed->setParent(states);loopLed->setFixedSize(26,26);loopLed->show();sg->addWidget(loopLed,1,0);}sg->addWidget(loopTxt,1,1);sg->setColumnStretch(2,1);
    static_cast<QVBoxLayout*>(states->layout())->addLayout(sg);static_cast<QVBoxLayout*>(states->layout())->addStretch(1);
    body->addWidget(states,0,1);

    QFrame *adjust=settingsScaffoldCard(page,QStringLiteral("settingsAdjust"),I18n::text(7135));
    QGridLayout *ag=new QGridLayout;ag->setHorizontalSpacing(8);ag->setVerticalSpacing(7);ag->setColumnStretch(1,1);
    struct Row{QLabel*l;QPushButton*minus;QLCDNumber*lcd;QPushButton*plus;};
    Row rows[]={{fuelLabel,fuelMinus,fuelLcd,fuelPlus},{hotLabel,hotMinus,hotLcd,hotPlus},{rpmLabel,rpmMinus,rpmLcd,rpmPlus},{ignLabel,ignMinus,ignLcd,ignPlus}};
    for(int r=0;r<4;r++){
        if(rows[r].l){rows[r].l->setParent(adjust);rows[r].l->show();ag->addWidget(rows[r].l,r,0);}
        if(rows[r].minus){rows[r].minus->setParent(adjust);rows[r].minus->show();ag->addWidget(rows[r].minus,r,1);}
        if(rows[r].lcd){rows[r].lcd->setParent(adjust);rows[r].lcd->setSegmentStyle(QLCDNumber::Flat);rows[r].lcd->setStyleSheet(QStringLiteral("background:#0a1117;color:#ff9828;border:1px solid #303c46;"));rows[r].lcd->setMinimumWidth(64);rows[r].lcd->show();ag->addWidget(rows[r].lcd,r,2);}
        if(rows[r].plus){rows[r].plus->setParent(adjust);rows[r].plus->show();ag->addWidget(rows[r].plus,r,3);}
    }
    if(note){note->setParent(adjust);note->setReadOnly(true);note->show();ag->addWidget(note,4,0,1,4);}
    QHBoxLayout *resets=new QHBoxLayout;if(resetAdj){resetAdj->setParent(adjust);resetAdj->show();resets->addWidget(resetAdj);}if(resetEcu){resetEcu->setParent(adjust);resetEcu->show();resets->addWidget(resetEcu);}ag->addLayout(resets,5,0,1,4);
    static_cast<QVBoxLayout*>(adjust->layout())->addLayout(ag,1);body->addWidget(adjust,1,1);
    root->addLayout(body,1);
}

static void applySettingsLayout(QMainWindow *window)
{
    if(!window) return;
    buildSettingsScaffold(window);
    QWidget *page=window->findChild<QWidget*>(QStringLiteral("emission_tab"));
    QFrame *metrics=window->findChild<QFrame*>(QStringLiteral("settingsMetrics"));
    QFrame *states=window->findChild<QFrame*>(QStringLiteral("settingsStates"));
    QFrame *adjust=window->findChild<QFrame*>(QStringLiteral("settingsAdjust"));
    if(!page || !metrics || !states || !adjust) return;

    if(!page->property("settingsDark410Built").toBool()) {
        page->setProperty("settingsDark410Built",true);
        rebuildGaugeArea(page,metrics);
        rebuildAdjustArea(page,adjust);
        rebuildPageLayout(page,metrics,states,adjust);
    }

    // One authoritative source for the Settings page.
    // Integrate the previously separate late visual pass once, here.
    if(!page->property("settingsLayoutIntegrated").toBool()) {
        compactAdjustForm(page,adjust);
        arrangeBottomPanels(page,metrics,states,adjust);
        shortenSettingsSeparator(adjust);
        page->setProperty("settingsLayoutIntegrated",true);
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
    // Let the same layout size the status panel; never restore a 154px cap.
    states->setMinimumHeight(adjust->minimumHeight());
    states->setMaximumHeight(adjust->maximumHeight());
    states->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Fixed);
    embedNoteInRpmGauge(page,metrics,adjust);
}


} // namespace

void buildSettingsPage(QMainWindow *window)
{
    // The Settings layout is built once as part of the real UI, not corrected
    // again at arbitrary delays after the first window show.
    applySettingsLayout(window);
}

void fitSettingsPage(QMainWindow *window)
{
    if (!window)
        return;
    QWidget *page=window->findChild<QWidget*>(QStringLiteral("emission_tab"));
    QFrame *metrics=window->findChild<QFrame*>(QStringLiteral("settingsMetrics"));
    QFrame *adjust=window->findChild<QFrame*>(QStringLiteral("settingsAdjust"));
    applySettingsSizing(page,metrics,adjust);
}
