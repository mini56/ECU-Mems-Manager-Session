#include <QApplication>
#include <QCoreApplication>
#include <QEvent>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLCDNumber>
#include <QLayout>
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

static const QColor kBorder("#34414a");
static const QColor kPanel("#10161c");
static const QColor kGaugeBg("#080d11");
static const QColor kText("#edf2f4");
static const QColor kMuted("#8d99a3");
static const QColor kOrange("#ff7a00");
static const QColor kRed("#ff3d32");

static void clearLayout(QLayout *layout)
{
    if(!layout) return;
    while(QLayoutItem *item=layout->takeAt(0)) {
        if(QLayout *sub=item->layout()) {
            clearLayout(sub);
            delete sub;
        }
        delete item;
    }
}

static void hideDirectChildren(QWidget *parent)
{
    if(!parent) return;
    const QList<QWidget*> children=parent->findChildren<QWidget*>(QString(),Qt::FindDirectChildrenOnly);
    for(QWidget *child:children) child->hide();
}

class SettingsGauge : public QWidget
{
public:
    enum Kind { IdleError, EngineRpm, FuelTrim, IgnitionAdvance, Lambda };

    SettingsGauge(QObject *source,const QString &title,const QString &unit,Kind kind,QWidget *parent=nullptr)
        : QWidget(parent),m_source(source),m_title(title),m_unit(unit),m_kind(kind)
    {
        setMinimumSize(kind==EngineRpm?300:170,kind==EngineRpm?300:155);
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
        const qreal bw=large?390.0:220.0;
        const qreal bh=large?350.0:190.0;
        const qreal s=qMin(width()/bw,height()/bh);
        p.translate((width()-bw*s)/2.0,(height()-bh*s)/2.0);
        p.scale(s,s);

        // Individual Dark card restored around every gauge.
        p.setPen(QPen(kBorder,1.0));
        p.setBrush(kGaugeBg);
        p.drawRoundedRect(QRectF(.5,.5,bw-1.0,bh-1.0),5.0,5.0);

        QFont titleFont=p.font();
        titleFont.setBold(true);
        titleFont.setPointSizeF(large?9.5:7.6);
        p.setFont(titleFont);
        p.setPen(kText);
        p.drawText(QRectF(8,6,bw-16,19),Qt::AlignCenter,m_title.toUpper());
        p.setPen(QPen(QColor("#28343c"),1.0));
        p.drawLine(QPointF(10,28),QPointF(bw-10,28));

        const QPointF c(bw/2.0,large?180.0:103.0);
        const qreal r=large?145.0:75.0;
        const qreal startDeg=135.0;
        const qreal sweepDeg=270.0;
        const qreal arcR=r-(large?12.0:8.5);

        QRadialGradient bezel(c-QPointF(r*.18,r*.22),r*1.18);
        bezel.setColorAt(0,QColor("#5a646b"));
        bezel.setColorAt(.18,QColor("#2b343a"));
        bezel.setColorAt(.58,QColor("#11181d"));
        bezel.setColorAt(.86,QColor("#070b0e"));
        bezel.setColorAt(1,QColor("#020304"));
        p.setPen(QPen(QColor("#68747b"),large?2.0:1.35));
        p.setBrush(bezel);
        p.drawEllipse(c,r,r);
        p.setPen(QPen(QColor("#121a1f"),large?8.0:5.0));
        p.setBrush(QColor("#05090c"));
        p.drawEllipse(c,r-6.0,r-6.0);
        p.setPen(QPen(QColor("#3d4a53"),large?1.5:1.0));
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

        auto pointFor=[&](qreal f,qreal radius){
            const qreal a=qDegreesToRadians(startDeg+sweepDeg*f);
            return c+QPointF(qCos(a)*radius,qSin(a)*radius);
        };
        auto drawRange=[&](double lo,double hi,const QColor &color,qreal width){
            if(hi<=lo || hi<=minv || lo>=maxv) return;
            lo=qMax(lo,minv); hi=qMin(hi,maxv);
            const qreal f0=(lo-minv)/(maxv-minv);
            const qreal f1=(hi-minv)/(maxv-minv);
            QPainterPath path;
            const int steps=40;
            for(int i=0;i<=steps;i++) {
                const qreal f=f0+(f1-f0)*i/qreal(steps);
                const QPointF pt=pointFor(f,arcR);
                if(i==0) path.moveTo(pt); else path.lineTo(pt);
            }
            p.setPen(QPen(color,width,Qt::SolidLine,Qt::RoundCap));
            p.drawPath(path);
        };

        QPainterPath baseArc;
        for(int i=0;i<=100;i++) {
            const QPointF pt=pointFor(i/100.0,arcR);
            if(i==0) baseArc.moveTo(pt); else baseArc.lineTo(pt);
        }
        p.setPen(QPen(QColor("#39464e"),large?2.8:1.8,Qt::SolidLine,Qt::RoundCap));
        p.drawPath(baseArc);

        if(m_kind==EngineRpm) {
            drawRange(qMax(minv,maxv*.875),maxv,kRed,large?7.0:4.0);
        } else if(m_kind==IdleError) {
            drawRange(minv+(maxv-minv)*.50,maxv,kRed,4.6);
        } else if(m_kind==FuelTrim) {
            drawRange(minv+(maxv-minv)*.875,maxv,kRed,4.4);
        } else if(m_kind==Lambda) {
            drawRange(minv+(maxv-minv)*.86,maxv,kRed,4.2);
        }

        const int majorCount=(m_kind==Lambda)?6:8;
        const int tickCount=majorCount*5;
        p.save();
        p.translate(c);
        for(int i=0;i<=tickCount;i++) {
            const bool major=(i%5==0);
            const qreal a=qDegreesToRadians(startDeg+sweepDeg*i/qreal(tickCount));
            const qreal ro=r-(large?10.0:7.0);
            const qreal len=major?(large?16.0:10.0):(large?7.0:4.5);
            const QPointF po(qCos(a)*ro,qSin(a)*ro);
            const QPointF pi(qCos(a)*(ro-len),qSin(a)*(ro-len));
            p.setPen(QPen(major?QColor("#f1f4f5"):QColor("#829099"),major?(large?2.0:1.2):(large?1.0:.65),Qt::SolidLine,Qt::FlatCap));
            p.drawLine(pi,po);
        }
        p.restore();

        QFont scaleFont=p.font();
        scaleFont.setBold(false);
        scaleFont.setPointSizeF(large?8.4:6.2);
        p.setFont(scaleFont);
        p.setPen(QColor("#eef2f4"));
        const qreal labelR=r-(large?42.0:28.0);
        for(int i=0;i<=majorCount;i++) {
            const double fv=minv+(maxv-minv)*i/qreal(majorCount);
            const QPointF pos=pointFor(i/qreal(majorCount),labelR);
            QString txt;
            if(qAbs(maxv-minv)<=50.0) txt=QString::number(fv,'f',0);
            else txt=QString::number(qRound(fv));
            p.drawText(QRectF(pos.x()-(large?30:19),pos.y()-(large?9:7),large?60:38,large?18:14),Qt::AlignCenter,txt);
        }

        const qreal n=qBound<qreal>(0.0,(value-minv)/(maxv-minv),1.0);
        const qreal a=qDegreesToRadians(startDeg+sweepDeg*n);
        const QPointF d(qCos(a),qSin(a));
        const QPointF normal(-d.y(),d.x());
        QPainterPath needle;
        needle.moveTo(c+d*(r-(large?24.0:15.0)));
        needle.lineTo(c+normal*(large?4.5:2.8));
        needle.lineTo(c-d*(large?15.0:8.0));
        needle.lineTo(c-normal*(large?4.5:2.8));
        needle.closeSubpath();
        p.setPen(Qt::NoPen);
        p.setBrush(kOrange);
        p.drawPath(needle);
        p.setPen(QPen(QColor("#768188"),large?1.4:1.0));
        p.setBrush(QColor("#273038"));
        p.drawEllipse(c,large?10.5:6.0,large?10.5:6.0);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#11171b"));
        p.drawEllipse(c,large?6.0:3.5,large?6.0:3.5);

        QFont valueFont=p.font();
        valueFont.setBold(true);
        valueFont.setPointSizeF(large?25.0:14.0);
        p.setFont(valueFont);
        p.setPen(Qt::white);
        const QString valueText=(qAbs(maxv-minv)<=50.0)?QString::number(value,'f',1):QString::number(value,'f',0);
        p.drawText(QRectF(c.x()-(large?66:42),c.y()+(large?78:42),large?132:84,large?34:23),Qt::AlignCenter,valueText);

        QFont unitFont=p.font();
        unitFont.setBold(true);
        unitFont.setPointSizeF(large?8.6:6.4);
        p.setFont(unitFont);
        p.setPen(QColor("#d3dbe0"));
        p.drawText(QRectF(c.x()-(large?58:38),c.y()+(large?109:61),large?116:76,large?17:13),Qt::AlignCenter,m_unit);
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
    button->setMinimumSize(135,31);
    button->setMaximumHeight(35);
    button->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Fixed);
    button->show();
}

static void styleLcd(QLCDNumber *lcd)
{
    if(!lcd) return;
    lcd->setSegmentStyle(QLCDNumber::Flat);
    lcd->setDigitCount(4);
    lcd->setSmallDecimalPoint(true);
    lcd->setFixedSize(86,33);
    lcd->setStyleSheet(QStringLiteral("QLCDNumber{background:#080e13;color:#ff8a00;border:1px solid #34414b;border-radius:3px;}"));
    lcd->show();
}

static void detachGaugeSources(QWidget *page)
{
    if(!page) return;
    const char *names[]={"e_idle_error","m_revCounter_exhaust","e_short_term_fuel_trim","e_ignition_advance","e_lambda"};
    for(const char *name:names) {
        if(QWidget *source=page->findChild<QWidget*>(QString::fromLatin1(name))) {
            source->setParent(page);
            source->hide();
        }
    }
}

static void removeOldGaugeLayers(QWidget *page,QFrame *metrics)
{
    if(!page || !metrics) return;
    for(QWidget *w:page->findChildren<QWidget*>()) {
        const QString cls=QString::fromLatin1(w->metaObject()->className());
        if(cls==QStringLiteral("CompactGauge")) {
            w->hide();
            w->deleteLater();
        }
    }
    hideDirectChildren(metrics);
}

static void buildGaugeArea(QWidget *page,QFrame *metrics)
{
    if(!page || !metrics) return;
    detachGaugeSources(page);
    removeOldGaugeLayers(page,metrics);

    QVBoxLayout *outer=qobject_cast<QVBoxLayout*>(metrics->layout());
    if(!outer) return;
    clearLayout(outer);
    outer->setContentsMargins(10,8,10,10);
    outer->setSpacing(5);

    metrics->setStyleSheet(QStringLiteral("#settingsMetrics{background:#10161c;border:1px solid #34414a;border-radius:6px;}"));

    QLabel *sectionTitle=new QLabel(I18n::text(7133),metrics);
    QFont sf=sectionTitle->font(); sf.setBold(true); sf.setPointSizeF(9.1); sectionTitle->setFont(sf);
    sectionTitle->setStyleSheet(QStringLiteral("color:#ff9828;background:transparent;border:0;"));
    outer->addWidget(sectionTitle);
    QFrame *line=new QFrame(metrics);
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet(QStringLiteral("background:#29343e;border:0;max-height:1px;"));
    outer->addWidget(line);

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
    grid->setContentsMargins(2,1,2,1);
    grid->setHorizontalSpacing(10);
    grid->setVerticalSpacing(6);
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
    outer->addLayout(grid,1);
}

static void buildAdjustArea(QWidget *page,QFrame *states,QFrame *adjust,QLineEdit *note)
{
    if(!page || !adjust) return;
    if(states) states->hide();

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

    QVBoxLayout *outer=qobject_cast<QVBoxLayout*>(adjust->layout());
    if(!outer) return;
    clearLayout(outer);
    hideDirectChildren(adjust);
    outer->setContentsMargins(12,8,12,9);
    outer->setSpacing(5);
    adjust->setStyleSheet(QStringLiteral("#settingsAdjust{background:#10161c;border:1px solid #34414a;border-radius:6px;}"));

    QHBoxLayout *header=new QHBoxLayout;
    header->setContentsMargins(0,0,0,0);
    header->setSpacing(9);
    QLabel *title=new QLabel(I18n::text(7135),adjust);
    QFont tf=title->font(); tf.setBold(true); tf.setPointSizeF(9.0); title->setFont(tf);
    title->setStyleSheet(QStringLiteral("color:#ff9828;background:transparent;border:0;"));
    header->addWidget(title);
    header->addSpacing(12);

    if(loopLed) {
        loopLed->setParent(adjust); styleLed(loopLed); header->addWidget(loopLed);
        QLabel *txt=new QLabel(I18n::text(2014),adjust); txt->setStyleSheet(QStringLiteral("color:#dce3e8;background:transparent;border:0;")); header->addWidget(txt);
    }
    header->addSpacing(10);
    if(idleLed) {
        idleLed->setParent(adjust); styleLed(idleLed); header->addWidget(idleLed);
        QLabel *txt=new QLabel(I18n::text(2015),adjust); txt->setStyleSheet(QStringLiteral("color:#dce3e8;background:transparent;border:0;")); header->addWidget(txt);
    }
    header->addStretch(1);

    if(note) {
        const QString noteText=note->text();
        note->hide();
        QLabel *info=new QLabel(noteText,adjust);
        QFont nf=info->font(); nf.setPointSizeF(7.5); info->setFont(nf);
        info->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
        info->setMinimumWidth(350);
        info->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
        info->setStyleSheet(QStringLiteral("color:#b5c0c7;background:transparent;border:0;"));
        header->addWidget(info,2);
    }
    outer->addLayout(header);

    QFrame *line=new QFrame(adjust);
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet(QStringLiteral("background:#29343e;border:0;max-height:1px;"));
    outer->addWidget(line);

    QWidget *controls=new QWidget(adjust);
    controls->setMaximumWidth(780);
    controls->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Preferred);
    QGridLayout *grid=new QGridLayout(controls);
    grid->setContentsMargins(0,0,0,0);
    grid->setHorizontalSpacing(9);
    grid->setVerticalSpacing(5);

    struct Row { QLabel *label; QPushButton *minus; QLCDNumber *lcd; const char *unit; QPushButton *plus; };
    Row rows[]={
        {fuelLabel,fuelMinus,fuelLcd,"%",fuelPlus},
        {hotLabel,hotMinus,hotLcd,"pas",hotPlus},
        {rpmLabel,rpmMinus,rpmLcd,"pas",rpmPlus},
        {ignLabel,ignMinus,ignLcd,"°",ignPlus}
    };

    for(int r=0;r<4;r++) {
        if(rows[r].label) {
            rows[r].label->setParent(controls);
            rows[r].label->setMinimumWidth(205);
            rows[r].label->setStyleSheet(QStringLiteral("color:#e6ecef;background:transparent;border:0;"));
            rows[r].label->show();
            grid->addWidget(rows[r].label,r,0);
        }
        if(rows[r].minus) { rows[r].minus->setParent(controls); styleAdjustButton(rows[r].minus); grid->addWidget(rows[r].minus,r,1); }
        if(rows[r].lcd) { rows[r].lcd->setParent(controls); styleLcd(rows[r].lcd); grid->addWidget(rows[r].lcd,r,2); }
        QLabel *unit=new QLabel(QString::fromUtf8(rows[r].unit),controls);
        QFont uf=unit->font(); uf.setBold(true); uf.setPointSizeF(8.4); unit->setFont(uf);
        unit->setFixedWidth(36);
        unit->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
        unit->setStyleSheet(QStringLiteral("color:#d8dfe3;background:transparent;border:0;"));
        grid->addWidget(unit,r,3);
        if(rows[r].plus) { rows[r].plus->setParent(controls); styleAdjustButton(rows[r].plus); grid->addWidget(rows[r].plus,r,4); }
    }

    QHBoxLayout *centerControls=new QHBoxLayout;
    centerControls->setContentsMargins(0,0,0,0);
    centerControls->addStretch(1);
    centerControls->addWidget(controls);
    centerControls->addStretch(1);
    outer->addLayout(centerControls);

    QHBoxLayout *resets=new QHBoxLayout;
    resets->setContentsMargins(0,1,0,0);
    resets->addStretch(1);
    if(resetAdj) { resetAdj->setParent(adjust); resetAdj->setMinimumHeight(30); resetAdj->setMaximumHeight(34); resetAdj->show(); resets->addWidget(resetAdj); }
    if(resetEcu) { resetEcu->setParent(adjust); resetEcu->setMinimumHeight(30); resetEcu->setMaximumHeight(34); resetEcu->show(); resets->addWidget(resetEcu); }
    resets->addStretch(1);
    outer->addLayout(resets);
}

static void rebuildPage(QWidget *page,QFrame *metrics,QFrame *states,QFrame *adjust)
{
    if(!page || !metrics || !adjust) return;
    QLayout *old=page->layout();
    if(old) {
        clearLayout(old);
        delete old;
    }
    metrics->setParent(page);
    adjust->setParent(page);
    if(states) states->setParent(page);

    QVBoxLayout *root=new QVBoxLayout(page);
    root->setContentsMargins(7,5,7,6);
    root->setSpacing(8);
    root->setSizeConstraint(QLayout::SetDefaultConstraint);
    root->addWidget(metrics,0);
    root->addWidget(adjust,0);
    root->addStretch(1);

    metrics->show();
    adjust->show();
    if(states) states->hide();
}

static void applySizing(QWidget *page,QFrame *metrics,QFrame *adjust)
{
    if(!page || !metrics || !adjust) return;
    page->setMinimumSize(0,0);
    page->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
    page->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);

    const int h=qMax(520,page->height());
    const int metricH=qBound(360,qRound(h*.66),405);
    const int adjustH=qBound(205,qRound(h*.36),230);
    metrics->setMinimumHeight(metricH);
    metrics->setMaximumHeight(metricH);
    adjust->setMinimumHeight(adjustH);
    adjust->setMaximumHeight(adjustH);
    metrics->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
    adjust->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
}

static void applySettingsLayout(QMainWindow *window)
{
    if(!window) return;
    QWidget *page=window->findChild<QWidget*>(QStringLiteral("emission_tab"));
    QFrame *metrics=window->findChild<QFrame*>(QStringLiteral("settingsMetrics"));
    QFrame *states=window->findChild<QFrame*>(QStringLiteral("settingsStates"));
    QFrame *adjust=window->findChild<QFrame*>(QStringLiteral("settingsAdjust"));
    if(!page || !metrics || !adjust) return;

    if(!page->property("settingsDark411Built").toBool()) {
        page->setProperty("settingsDark411Built",true);
        QLineEdit *note=page->findChild<QLineEdit*>(QStringLiteral("lineEdit_3"));
        buildGaugeArea(page,metrics);
        buildAdjustArea(page,states,adjust,note);
        rebuildPage(page,metrics,states,adjust);
    }
    applySizing(page,metrics,adjust);
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
