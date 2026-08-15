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

static const QColor kPanel("#10161c");
static const QColor kBorder("#29343e");
static const QColor kText("#edf2f4");
static const QColor kMuted("#8d99a3");
static const QColor kOrange("#ff7a00");
static const QColor kOrange2("#ff9828");
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

static void clearLayout(QLayout *layout,int keep=0)
{
    if(!layout) return;
    while(layout->count()>keep) disposeLayoutItem(layout->takeAt(keep));
}

class SettingsGauge : public QWidget
{
public:
    enum Kind { IdleError, EngineRpm, FuelTrim, IgnitionAdvance, Lambda };

    SettingsGauge(QObject *source,const QString &title,const QString &unit,Kind kind,QWidget *parent=nullptr)
        : QWidget(parent),m_source(source),m_title(title),m_unit(unit),m_kind(kind)
    {
        setMinimumSize(kind==EngineRpm?230:130,kind==EngineRpm?230:135);
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

        auto pointFor=[&](double f,qreal radius){
            const qreal a=qDegreesToRadians(startDeg+sweepDeg*f);
            return c+QPointF(qCos(a)*radius,qSin(a)*radius);
        };
        auto drawRange=[&](double lo,double hi,const QColor &color,qreal width){
            if(hi<=lo || hi<=minv || lo>=maxv) return;
            lo=qMax(lo,minv); hi=qMin(hi,maxv);
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
            drawRange(redStart,maxv,kRed,large?7.0:4.0);
        } else if(m_kind==IdleError) {
            const double redStart=(maxv>=180.0)?100.0:(minv+(maxv-minv)*.60);
            drawRange(redStart,maxv,kRed,large?6.0:4.5);
        }

        const int majorCount=(m_kind==EngineRpm || m_kind==IdleError || m_kind==FuelTrim || m_kind==IgnitionAdvance)?8:6;
        const int tickCount=majorCount*5;
        p.save();
        p.translate(c);
        for(int i=0;i<=tickCount;i++) {
            const bool major=(i%5==0);
            const qreal a=qDegreesToRadians(startDeg+sweepDeg*i/qreal(tickCount));
            const qreal ro=r-(large?9.0:6.5);
            const qreal len=major?(large?15.0:9.0):(large?7.0:4.0);
            const QPointF po(qCos(a)*ro,qSin(a)*ro);
            const QPointF pi(qCos(a)*(ro-len),qSin(a)*(ro-len));
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
            QString txt;
            if(qAbs(maxv-minv)<=50.0) txt=QString::number(fv,'f',0);
            else txt=QString::number(qRound(fv));
            p.drawText(QRectF(pos.x()-(large?28:18),pos.y()-(large?9:7),large?56:36,large?18:14),Qt::AlignCenter,txt);
        }

        const double n=qBound(0.0,(value-minv)/(maxv-minv),1.0);
        const qreal needleDeg=startDeg+sweepDeg*n;
        const qreal a=qDegreesToRadians(needleDeg);
        const QPointF d(qCos(a),qSin(a));
        const QPointF normal(-d.y(),d.x());
        QPainterPath needle;
        needle.moveTo(c+d*(r-(large?24.0:14.0)));
        needle.lineTo(c+normal*(large?4.2:2.8));
        needle.lineTo(c-d*(large?14.0:8.0));
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
        p.drawText(QRectF(c.x()-(large?62:40),c.y()+(large?72:39),large?124:80,large?32:22),Qt::AlignCenter,valueText);

        QFont unitFont=p.font();
        unitFont.setBold(true);
        unitFont.setPointSizeF(large?8.5:6.3);
        p.setFont(unitFont);
        p.setPen(QColor("#d4dbe0"));
        p.drawText(QRectF(c.x()-(large?55:36),c.y()+(large?101:58),large?110:72,large?17:13),Qt::AlignCenter,m_unit);
    }

private:
    QObject *m_source=nullptr;
    QString m_title;
    QString m_unit;
    Kind m_kind;
};

static void prepareGaugeSource(QWidget *page,const char *name)
{
    if(!page) return;
    if(QWidget *source=page->findChild<QWidget*>(QString::fromLatin1(name))) source->hide();
}

static void styleLed(QWidget *led)
{
    if(!led) return;
    led->setFixedSize(24,24);
    led->setMinimumSize(24,24);
    led->setMaximumSize(24,24);
    led->show();
}

static void styleAdjustButton(QPushButton *button)
{
    if(!button) return;
    button->setMinimumHeight(31);
    button->setMaximumHeight(36);
    button->setMinimumWidth(102);
    button->setMaximumWidth(QWIDGETSIZE_MAX);
    button->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Fixed);
    button->show();
}

static void styleLcd(QLCDNumber *lcd)
{
    if(!lcd) return;
    lcd->setSegmentStyle(QLCDNumber::Flat);
    lcd->setDigitCount(4);
    lcd->setSmallDecimalPoint(true);
    lcd->setMinimumSize(82,31);
    lcd->setMaximumHeight(36);
    lcd->setStyleSheet(QStringLiteral("QLCDNumber{background:#080e13;color:#ff8a00;border:1px solid #34414b;border-radius:3px;}"));
    lcd->show();
}

static void rebuildGaugeArea(QMainWindow *window,QWidget *page,QFrame *metrics,QLineEdit *note)
{
    if(!window || !page || !metrics) return;

    QVBoxLayout *layout=qobject_cast<QVBoxLayout*>(metrics->layout());
    if(!layout) return;

    for(QWidget *w:metrics->findChildren<QWidget*>()) {
        if(QString::fromLatin1(w->metaObject()->className())==QStringLiteral("CompactGauge")) w->hide();
    }
    clearLayout(layout,2);

    prepareGaugeSource(page,"e_idle_error");
    prepareGaugeSource(page,"m_revCounter_exhaust");
    prepareGaugeSource(page,"e_short_term_fuel_trim");
    prepareGaugeSource(page,"e_ignition_advance");
    prepareGaugeSource(page,"e_lambda");

    QObject *idleSrc=page->findChild<QObject*>(QStringLiteral("e_idle_error"));
    QObject *rpmSrc=page->findChild<QObject*>(QStringLiteral("m_revCounter_exhaust"));
    QObject *trimSrc=page->findChild<QObject*>(QStringLiteral("e_short_term_fuel_trim"));
    QObject *ignSrc=page->findChild<QObject*>(QStringLiteral("e_ignition_advance"));
    QObject *lambdaSrc=page->findChild<QObject*>(QStringLiteral("e_lambda"));

    SettingsGauge *idle=new SettingsGauge(idleSrc,I18n::text(7139),QStringLiteral("tr/min"),SettingsGauge::IdleError,metrics);
    SettingsGauge *rpm=new SettingsGauge(rpmSrc,I18n::text(1011),QStringLiteral("tr/min"),SettingsGauge::EngineRpm,metrics);
    SettingsGauge *trim=new SettingsGauge(trimSrc,I18n::text(7140),QStringLiteral("%"),SettingsGauge::FuelTrim,metrics);
    SettingsGauge *ign=new SettingsGauge(ignSrc,I18n::text(7141),QStringLiteral("°"),SettingsGauge::IgnitionAdvance,metrics);
    SettingsGauge *lambda=new SettingsGauge(lambdaSrc,I18n::text(7142),QStringLiteral("mV"),SettingsGauge::Lambda,metrics);

    idle->setObjectName(QStringLiteral("darkTuneGaugeIdle"));
    rpm->setObjectName(QStringLiteral("darkTuneGaugeRpm"));
    trim->setObjectName(QStringLiteral("darkTuneGaugeTrim"));
    ign->setObjectName(QStringLiteral("darkTuneGaugeIgnition"));
    lambda->setObjectName(QStringLiteral("darkTuneGaugeLambda"));

    QGridLayout *grid=new QGridLayout;
    grid->setContentsMargins(4,2,4,2);
    grid->setHorizontalSpacing(14);
    grid->setVerticalSpacing(4);
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

    if(note) {
        note->setParent(metrics);
        note->setReadOnly(true);
        note->setAlignment(Qt::AlignCenter);
        note->setMinimumHeight(25);
        note->setMaximumHeight(29);
        note->setMaximumWidth(620);
        note->setStyleSheet(QStringLiteral("QLineEdit{background:#0b1116;color:#9ca8b0;border:1px solid #27333b;border-radius:3px;padding:2px 8px;}"));
        note->show();
        layout->addWidget(note,0,Qt::AlignHCenter);
    }

    metrics->setMinimumHeight(350);
    metrics->setMaximumHeight(QWIDGETSIZE_MAX);
    metrics->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
}

static void rebuildAdjustArea(QMainWindow *window,QWidget *page,QFrame *states,QFrame *adjust)
{
    if(!window || !page || !adjust) return;

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
    clearLayout(layout,0);
    for(QWidget *child:adjust->findChildren<QWidget*>(QString(),Qt::FindDirectChildrenOnly)) child->hide();

    QHBoxLayout *header=new QHBoxLayout;
    header->setContentsMargins(0,0,0,0);
    header->setSpacing(10);
    QLabel *title=new QLabel(I18n::text(7135),adjust);
    QFont titleFont=title->font();
    titleFont.setBold(true);
    titleFont.setPointSizeF(9.0);
    title->setFont(titleFont);
    title->setStyleSheet(QStringLiteral("color:#ff9828;background:transparent;border:0;"));
    title->show();
    header->addWidget(title);
    header->addSpacing(18);

    if(loopLed) {
        loopLed->setParent(adjust); styleLed(loopLed); header->addWidget(loopLed);
        QLabel *loopText=new QLabel(I18n::text(2014),adjust); loopText->setStyleSheet(QStringLiteral("color:#dce3e8;background:transparent;border:0;")); header->addWidget(loopText);
    }
    header->addSpacing(18);
    if(idleLed) {
        idleLed->setParent(adjust); styleLed(idleLed); header->addWidget(idleLed);
        QLabel *idleText=new QLabel(I18n::text(2015),adjust); idleText->setStyleSheet(QStringLiteral("color:#dce3e8;background:transparent;border:0;")); header->addWidget(idleText);
    }
    header->addStretch(1);
    layout->addLayout(header);

    QFrame *line=new QFrame(adjust);
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet(QStringLiteral("background:#29343e;border:0;max-height:1px;"));
    line->show();
    layout->addWidget(line);

    QGridLayout *grid=new QGridLayout;
    grid->setContentsMargins(0,1,0,0);
    grid->setHorizontalSpacing(9);
    grid->setVerticalSpacing(6);
    grid->setColumnStretch(0,2);
    grid->setColumnStretch(1,1);
    grid->setColumnStretch(2,0);
    grid->setColumnStretch(3,0);
    grid->setColumnStretch(4,1);

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

    for(int r=0;r<4;r++) {
        if(rows[r].label) {
            rows[r].label->setParent(adjust);
            rows[r].label->setMinimumWidth(190);
            rows[r].label->setStyleSheet(QStringLiteral("color:#e6ecef;background:transparent;border:0;"));
            rows[r].label->show();
            grid->addWidget(rows[r].label,r,0);
        }
        if(rows[r].minus) {
            rows[r].minus->setParent(adjust); styleAdjustButton(rows[r].minus); grid->addWidget(rows[r].minus,r,1);
        }
        if(rows[r].lcd) {
            rows[r].lcd->setParent(adjust); styleLcd(rows[r].lcd); grid->addWidget(rows[r].lcd,r,2);
        }
        QLabel *unitLabel=new QLabel(QString::fromUtf8(rows[r].unit),adjust);
        QFont uf=unitLabel->font(); uf.setBold(true); uf.setPointSizeF(8.5); unitLabel->setFont(uf);
        unitLabel->setMinimumWidth(34);
        unitLabel->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
        unitLabel->setStyleSheet(QStringLiteral("color:#d8dfe3;background:transparent;border:0;"));
        grid->addWidget(unitLabel,r,3);
        if(rows[r].plus) {
            rows[r].plus->setParent(adjust); styleAdjustButton(rows[r].plus); grid->addWidget(rows[r].plus,r,4);
        }
    }

    layout->addLayout(grid,1);

    QHBoxLayout *resets=new QHBoxLayout;
    resets->setContentsMargins(0,2,0,0);
    resets->addStretch(1);
    if(resetAdj) {
        resetAdj->setParent(adjust); resetAdj->setMinimumHeight(31); resetAdj->setMaximumHeight(36); resetAdj->show(); resets->addWidget(resetAdj);
    }
    if(resetEcu) {
        resetEcu->setParent(adjust); resetEcu->setMinimumHeight(31); resetEcu->setMaximumHeight(36); resetEcu->show(); resets->addWidget(resetEcu);
    }
    layout->addLayout(resets);

    adjust->setMinimumHeight(215);
    adjust->setMaximumHeight(QWIDGETSIZE_MAX);
    adjust->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
    adjust->show();
}

static void rebuildPageLayout(QWidget *page,QFrame *metrics,QFrame *states,QFrame *adjust)
{
    if(!page || !metrics || !adjust) return;
    QLayout *root=page->layout();
    if(!root) return;

    QGridLayout *body=nullptr;
    for(QGridLayout *grid:page->findChildren<QGridLayout*>()) {
        if(grid->indexOf(metrics)>=0 || grid->indexOf(adjust)>=0) { body=grid; break; }
    }
    if(!body) return;

    body->removeWidget(metrics);
    if(states) body->removeWidget(states);
    body->removeWidget(adjust);
    body->addWidget(metrics,0,0,1,2);
    body->addWidget(adjust,1,0,1,2);
    body->setColumnStretch(0,1);
    body->setColumnStretch(1,1);
    body->setRowStretch(0,5);
    body->setRowStretch(1,3);
    body->setHorizontalSpacing(0);
    body->setVerticalSpacing(9);
}

static void applySettingsLayout(QMainWindow *window)
{
    if(!window) return;
    QWidget *page=window->findChild<QWidget*>(QStringLiteral("emission_tab"));
    QFrame *metrics=window->findChild<QFrame*>(QStringLiteral("settingsMetrics"));
    QFrame *states=window->findChild<QFrame*>(QStringLiteral("settingsStates"));
    QFrame *adjust=window->findChild<QFrame*>(QStringLiteral("settingsAdjust"));
    if(!page || !metrics || !adjust) return;

    if(!page->property("settingsDark409Built").toBool()) {
        page->setProperty("settingsDark409Built",true);
        QLineEdit *note=page->findChild<QLineEdit*>(QStringLiteral("lineEdit_3"));
        rebuildGaugeArea(window,page,metrics,note);
        rebuildAdjustArea(window,page,states,adjust);
        rebuildPageLayout(page,metrics,states,adjust);
    }

    page->setMinimumSize(0,0);
    page->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
    page->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    if(QLayout *root=page->layout()) {
        root->setContentsMargins(8,7,8,7);
        root->setSpacing(8);
        root->setSizeConstraint(QLayout::SetDefaultConstraint);
    }

    metrics->setMinimumHeight(qBound(335,qRound(page->height()*.56),405));
    adjust->setMinimumHeight(qBound(205,qRound(page->height()*.34),245));

    if(SettingsGauge *rpm=page->findChild<SettingsGauge*>(QStringLiteral("darkTuneGaugeRpm"))) {
        rpm->setMinimumSize(qBound(225,qRound(page->width()*.23),300),qBound(225,qRound(page->height()*.40),305));
    }
    for(const char *name:{"darkTuneGaugeIdle","darkTuneGaugeTrim","darkTuneGaugeIgnition","darkTuneGaugeLambda"}) {
        if(SettingsGauge *g=page->findChild<SettingsGauge*>(QString::fromLatin1(name)))
            g->setMinimumSize(qBound(125,qRound(page->width()*.135),180),qBound(125,qRound(page->height()*.22),170));
    }
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
            QTimer::singleShot(220,window,[window](){applySettingsLayout(window);});
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
