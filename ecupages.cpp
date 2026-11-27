#include <QApplication>
#include "applicationvisualstyle.h"
#include "settingspage.h"
#include <QCoreApplication>
#include <QEvent>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QPlainTextEdit>
#include <QSettings>
#include <QLabel>
#include <QLCDNumber>
#include <QLineEdit>
#include <QMainWindow>
#include <QSizePolicy>
#include <QFontMetrics>
#include <QPainterPath>
#include <QPainter>
#include <QtMath>
#include <QPushButton>
#include <QTabWidget>
#include <QTimer>
#include <QTextDocument>
#include <QVBoxLayout>
#include <QWidget>
#include "i18n.h"
#include "mainwindow.h"
#include "memsinterface.h"
#include <QScrollArea>
#include "ecupages.h"
#include "mems218_tests.h"
#include "memsextendeddtc_presentation.h"
#include <qledindicator/qledindicator.h>

namespace {

static QFrame *card(QWidget *parent,const QString &name,const QString &title)
{
    QFrame *c=new QFrame(parent);
    c->setObjectName(name);
    c->setAttribute(Qt::WA_StyledBackground,true);
    c->setStyleSheet(QStringLiteral("#%1{background:#10161c;border:1px solid #29343e;border-radius:5px;}").arg(name));
    QVBoxLayout *v=new QVBoxLayout(c);
    v->setContentsMargins(14,12,14,14);
    v->setSpacing(8);
    QLabel *t=new QLabel(title,c);
    QFont f=t->font();f.setBold(true);f.setPointSizeF(9.5);t->setFont(f);
    t->setStyleSheet(QStringLiteral("color:#ff9828;background:transparent;border:0;"));
    v->addWidget(t);
    QFrame *line=new QFrame(c);line->setFrameShape(QFrame::HLine);line->setStyleSheet(QStringLiteral("background:#29343e;border:0;max-height:1px;"));v->addWidget(line);
    return c;
}

static QFrame *existingHeader(QWidget *page)
{
    if(!page)return nullptr;
    for(QFrame *f:page->findChildren<QFrame*>(QString(),Qt::FindDirectChildrenOnly))
        if(f->objectName().startsWith(QStringLiteral("uiRebuildHero_"))) return f;
    return nullptr;
}

static void clearLayout(QLayout *layout)
{
    if(!layout)return;
    while(QLayoutItem *it=layout->takeAt(0)){
        if(it->layout()) clearLayout(it->layout());
        delete it;
    }
}

static void beginPage(QWidget *page,QVBoxLayout *&root)
{
    QFrame *header=existingHeader(page);
    QLayout *old=page->layout();
    if(old){clearLayout(old);delete old;}
    root=new QVBoxLayout(page);
    root->setContentsMargins(15,12,15,12);
    root->setSpacing(10);
    if(header){header->show();root->addWidget(header);}
    for(QWidget *w:page->findChildren<QWidget*>(QString(),Qt::FindDirectChildrenOnly)){
        if(w!=header)w->hide();
    }
}

static QWidget *pageFor(QWidget *window,const QString &name)
{
    return window?window->findChild<QWidget*>(name):nullptr;
}

static void prepWidget(QWidget *w)
{
    if(!w)return;
    w->setMinimumSize(0,0);
    w->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
    w->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
    w->show();
}

static void removeFromCurrentLayout(QWidget *w)
{
    if(!w) return;
    QWidget *p=w->parentWidget();
    if(p && p->layout()) p->layout()->removeWidget(w);
}

static void moveWidget(QWidget *w,QWidget *parent)
{
    if(!w || !parent) return;
    removeFromCurrentLayout(w);
    w->setParent(parent);
    w->show();
}

static QLabel *cardTitle(QFrame *card)
{
    if(!card) return nullptr;
    const QList<QLabel*> labels=card->findChildren<QLabel*>(QString(),Qt::FindDirectChildrenOnly);
    for(QLabel *l:labels)
        if(l && l->styleSheet().contains(QStringLiteral("#ff9828")))
            return l;
    return labels.isEmpty()?nullptr:labels.first();
}

static void putButtonInTitleBar(QFrame *card,QPushButton *button)
{
    if(!card || !button) return;
    QVBoxLayout *v=qobject_cast<QVBoxLayout*>(card->layout());
    QLabel *title=cardTitle(card);
    if(!v || !title) return;

    v->removeWidget(title);
    removeFromCurrentLayout(button);
    button->setParent(card);
    // Both existing clear buttons only: avoid clipping after the BUILD132
    // readability font increase, and use the program's orange on black.
    QFont buttonFont=button->font();
    if(buttonFont.pointSizeF()>0 && buttonFont.pointSizeF()<9.8)
        buttonFont.setPointSizeF(9.8);
    buttonFont.setBold(true);
    button->setFont(buttonFont);
    const int fullTextWidth=QFontMetrics(buttonFont).horizontalAdvance(button->text())+34;
    button->setMinimumWidth(qBound(220,fullTextWidth,400));
    button->setMaximumWidth(qBound(220,fullTextWidth,400));
    button->setMinimumHeight(36);
    button->setMaximumHeight(36);
    button->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
    button->setStyleSheet(QStringLiteral(
        "QPushButton{background:#090e13;color:#ff9828;"
        "border:1px solid #ff9828;border-radius:4px;"
        "padding:4px 10px;font-weight:700;}"
        "QPushButton:disabled{background:#090e13;color:#ff9828;"
        "border:1px solid #ff9828;}"
        "QPushButton:hover:enabled{background:#19130a;}"));
    button->show();

    QGridLayout *head=new QGridLayout;
    head->setContentsMargins(0,0,0,0);
    head->setHorizontalSpacing(8);
    head->setColumnStretch(0,1);
    head->setColumnStretch(1,0);
    head->setColumnStretch(2,1);
    head->addWidget(title,0,0,Qt::AlignLeft|Qt::AlignVCenter);
    head->addWidget(button,0,1,Qt::AlignCenter);
    v->insertLayout(0,head);
}

class SystemStatePreview : public QWidget
{
public:
    explicit SystemStatePreview(MainWindow *window,QObject *source,QWidget *parent=nullptr)
        : QWidget(parent),m_window(window),m_source(source)
    {
        setMinimumWidth(150);
        setMaximumWidth(210);
        setMinimumHeight(135);
        setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Expanding);
        setAttribute(Qt::WA_TransparentForMouseEvents,true);

        QTimer *refresh=new QTimer(this);
        refresh->setInterval(500);
        QObject::connect(refresh,&QTimer::timeout,this,[this](){update();});
        refresh->start();
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing,true);
        p.setRenderHint(QPainter::TextAntialiasing,true);
        const bool darkTheme=qApp && qApp->property("ecuDarkTheme").toBool();

        const QRectF outer=QRectF(rect()).adjusted(.5,.5,-.5,-.5);
        p.setPen(QPen(darkTheme?QColor("#34414a"):QColor("#c4cdd5"),1.0));
        p.setBrush(darkTheme?QColor("#0c1217"):QColor("#ffffff"));
        p.drawRoundedRect(outer,5,5);

        QFont f=p.font();
        f.setBold(true);
        f.setPointSizeF(8.2);
        p.setFont(f);
        p.setPen(darkTheme?QColor("#edf2f4"):QColor("#101820"));
        p.drawText(QRectF(6,5,qMax(0,width()-12),18),Qt::AlignCenter,I18n::text(7149));

        p.setPen(QPen(darkTheme?QColor("#27333b"):QColor("#cbd5de"),1.0));
        p.drawLine(QPointF(8,25),QPointF(qMax(8,width()-8),25));

        // Same source, rule, colours and translated states as the Overview card.
        const bool fault=m_source?m_source->property("checked").toBool():false;
        const bool disconnected=m_window&&m_window->isEcuDisconnected();
        const QColor state=disconnected?QColor("#ff9828"):(fault?QColor("#ff4b3b"):QColor("#6bdd45"));

        const bool compact=objectName()==QStringLiteral("actuatorSystemState");
        const QPointF c(compact?width()*0.22:width()/2.0,compact?height()*0.64:height()*0.48);
        const qreal rr=compact?qMin(27.0,height()*0.26):qMin(width()*0.19,height()*0.16);
        QPainterPath shield;
        shield.moveTo(c.x(),c.y()-rr);
        shield.lineTo(c.x()+rr*.75,c.y()-rr*.65);
        shield.lineTo(c.x()+rr*.62,c.y()+rr*.28);
        shield.quadTo(c.x(),c.y()+rr,c.x()-rr*.62,c.y()+rr*.28);
        shield.lineTo(c.x()-rr*.75,c.y()-rr*.65);
        shield.closeSubpath();
        p.setPen(QPen(state,2.4));
        p.setBrush(Qt::NoBrush);
        p.drawPath(shield);

        if(!fault||disconnected) {
            p.setPen(QPen(state,2.8,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));
            p.drawLine(QPointF(c.x()-rr*.40,c.y()+rr*.04),
                       QPointF(c.x()-rr*.12,c.y()+rr*.32));
            p.drawLine(QPointF(c.x()-rr*.12,c.y()+rr*.32),
                       QPointF(c.x()+rr*.50,c.y()-rr*.40));
        }

        QFont stateFont=p.font();
        stateFont.setBold(true);
        stateFont.setPointSizeF(compact?10.0:7.8);
        p.setFont(stateFont);
        p.setPen(state);
        p.drawText(compact?QRectF(width()*0.39,30,width()*0.59-8,25):QRectF(8,height()*0.67,qMax(0,width()-16),24),
                   Qt::AlignCenter,disconnected?I18n::disconnectedStatus():(fault?I18n::text(7144):I18n::text(7143)));

        QFont sub=p.font();
        sub.setBold(false);
        sub.setPointSizeF(compact?9.0:6.2);
        p.setFont(sub);
        p.setPen(darkTheme?QColor("#8d99a3"):QColor("#485767"));
        p.drawText(compact?QRectF(width()*0.39,55,width()*0.59-8,32):QRectF(8,qMax(30,height()-38),qMax(0,width()-16),30),
                   Qt::AlignCenter|Qt::TextWordWrap,I18n::text(7145));
    }

private:
    MainWindow *m_window=nullptr;
    QObject *m_source=nullptr;
};

static constexpr int kErrorLedSize=22;

static QLedIndicator *additionalLed(QWidget *parent,const QString &name,bool state=false)
{
    auto *led=new QLedIndicator(parent);
    led->setObjectName(name);led->setCheckable(true);led->setChecked(false);
    led->setAttribute(Qt::WA_TransparentForMouseEvents,true);led->setFocusPolicy(Qt::NoFocus);
    led->setFixedSize(kErrorLedSize,kErrorLedSize);
    led->setOnColor1(state?QColor("#6bdd45"):QColor(255,0,0));
    led->setOnColor2(state?QColor("#348a22"):QColor(176,0,2));
    led->setOffColor1(QColor(20,0,0));
    led->setOffColor2(QColor(90,0,2));
    return led;
}
static void compactStoredPanel(QMainWindow *w,QFrame *top)
{
    if(!w || !top) return;
    QPushButton *clear=w->findChild<QPushButton*>(QStringLiteral("m_clearFaultsButton"));
    putButtonInTitleBar(top,clear);

    if(QLabel *duplicate=w->findChild<QLabel*>(QStringLiteral("m_faultCodesLabel")))
        duplicate->hide();

    QWidget *stored=w->findChild<QWidget*>(QStringLiteral("layoutWidget_3"));
    if(!stored)return;
    auto *g=qobject_cast<QGridLayout*>(stored->layout());
    if(!g)return;
    // Retain the original 24 widgets and all their ECU signal connections.
    // Every stored fault is a row of ONE grid, not a second compact panel.
    QWidget *leds[30]={};
    QLabel *labels[30]={};
    for(int i=0;i<24;++i){
        const int row=1+i%8,col=1+3*(i/8);
        auto *ledItem=g->itemAtPosition(row,col);
        auto *labelItem=g->itemAtPosition(row,col+1);
        leds[i]=ledItem?ledItem->widget():nullptr;
        labels[i]=labelItem?qobject_cast<QLabel*>(labelItem->widget()):nullptr;
        if(!leds[i]||!labels[i])return;
    }
    for(int i=0;i<6;++i){
        const unsigned bit=2u<<i;
        leds[24+i]=additionalLed(stored,QStringLiteral("extraDtc5Led%1").arg(bit));
        labels[24+i]=new QLabel(QStringLiteral("%1 - ").arg(25+i)+I18n::text(29245+i),stored);
        labels[24+i]->setObjectName(QStringLiteral("extraDtc5Label%1").arg(bit));
        labels[24+i]->setFont(labels[0]->font());
    }
    while(auto *item=g->takeAt(0))delete item;
    for(int row=0;row<g->rowCount();++row){g->setRowStretch(row,0);g->setRowMinimumHeight(row,0);}
    for(int col=0;col<g->columnCount();++col){g->setColumnStretch(col,0);g->setColumnMinimumWidth(col,0);}
    g->setContentsMargins(0,0,0,0);
    g->setHorizontalSpacing(8);g->setVerticalSpacing(3);
    for(int i=0;i<30;++i){
        const int row=i%10,col=2*(i/10);
        leds[i]->setFixedSize(kErrorLedSize,kErrorLedSize);
        leds[i]->setAttribute(Qt::WA_TransparentForMouseEvents,true);
        labels[i]->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
        labels[i]->setWordWrap(true);
        labels[i]->setMinimumWidth(0);
        labels[i]->setMaximumWidth(QWIDGETSIZE_MAX);
        labels[i]->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);
        g->addWidget(leds[i],row,col,Qt::AlignCenter);
        g->addWidget(labels[i],row,col+1);
        g->setRowStretch(row,1);g->setRowMinimumHeight(row,kErrorLedSize);
        g->setColumnStretch(col+1,1);
        leds[i]->show();labels[i]->show();
    }

}

static void addLed(QGridLayout *grid,QWidget *led,QWidget *parent,int row,int col)
{
    if(!grid || !led || !parent) return;
    moveWidget(led,parent);
    led->setFixedSize(kErrorLedSize,kErrorLedSize);
    grid->addWidget(led,row,col,Qt::AlignHCenter|Qt::AlignVCenter);
}

static void addLeftHelpLabel(QGridLayout *grid,QLabel *label,QWidget *parent,int row,int col)
{
    if(!grid || !label || !parent) return;
    moveWidget(label,parent);
    label->setMinimumWidth(0);
    label->setMaximumWidth(QWIDGETSIZE_MAX);
    label->setMinimumHeight(24);
    label->setMaximumHeight(QWIDGETSIZE_MAX);
    label->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
    label->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
    label->setProperty("helpBubbleLeft",true);
    grid->setRowMinimumHeight(row,24);
    grid->addWidget(label,row,col);
}

static void rebuildLivePanel(QMainWindow *w,QFrame *bottom)
{
    if(!w || !bottom) return;

    QPushButton *clear=w->findChild<QPushButton*>(QStringLiteral("m_clearRecordedAnomalies"));
    putButtonInTitleBar(bottom,clear);

    QWidget *oldLive=w->findChild<QWidget*>(QStringLiteral("layoutWidget_9"));
    QVBoxLayout *bv=qobject_cast<QVBoxLayout*>(bottom->layout());
    if(!bv) return;
    if(oldLive) {
        bv->removeWidget(oldLive);
        oldLive->hide();
    }

    QFrame *signalPanel=new QFrame(bottom);
    signalPanel->setObjectName(QStringLiteral("errorsCompactSignals"));
    signalPanel->setAttribute(Qt::WA_StyledBackground,true);
    signalPanel->setStyleSheet(QStringLiteral("#errorsCompactSignals{background:#0b1116;border:1px solid #27323b;border-radius:4px;}"));
    signalPanel->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    QVBoxLayout *sv=new QVBoxLayout(signalPanel);
    sv->setContentsMargins(10,7,10,8);
    sv->setSpacing(2);

    QGridLayout *grid=new QGridLayout;
    grid->setContentsMargins(0,0,0,0);
    grid->setHorizontalSpacing(10);
    grid->setVerticalSpacing(3);
    grid->setColumnMinimumWidth(0,82);
    grid->setColumnMinimumWidth(1,96);
    grid->setColumnStretch(0,0);
    grid->setColumnStretch(1,0);
    grid->setColumnStretch(2,1);

    // Row 0: RPM/crankshaft signal.  It deliberately shares the exact same
    // columns as the anomaly rows so its HELP bubble aligns with all others.
    addLed(grid,w->findChild<QWidget*>(QStringLiteral("m_RPMSensor")),signalPanel,0,0);
    addLeftHelpLabel(grid,w->findChild<QLabel*>(QStringLiteral("m_RPMSensorLabel")),signalPanel,0,2);

    // Row 1: column headings.
    QLabel *liveHead=w->findChild<QLabel*>(QStringLiteral("m_faultCodesLabel2"));
    QLabel *recordedHead=w->findChild<QLabel*>(QStringLiteral("m_faultCodesLabel3"));
    QLabel *anomHead=w->findChild<QLabel*>(QStringLiteral("m_faultCodesLabel4"));
    if(liveHead) {
        moveWidget(liveHead,signalPanel);
        liveHead->setAlignment(Qt::AlignCenter);
        grid->addWidget(liveHead,1,0,Qt::AlignCenter);
    }
    if(recordedHead) {
        moveWidget(recordedHead,signalPanel);
        recordedHead->setText(I18n::text(1015));
        recordedHead->setAlignment(Qt::AlignCenter);
        grid->addWidget(recordedHead,1,1,Qt::AlignCenter);
    }
    addLeftHelpLabel(grid,anomHead,signalPanel,1,2);

    struct LiveRow { const char *live; const char *recorded; const char *label; };
    const LiveRow rows[]={
        {"m_LambdaSensor",nullptr,"m_LambdaSensorLabel"},
        {"m_LambdaMax","m_LambdaMaxOnce","m_LambdaMaxLabel"},
        {"m_LambdaMin","m_LambdaMinOnce","m_LambdaMinLabel"},
        {"m_IACMin","m_IACMinOnce","m_IACMinLabel"},
        {"m_JackCount",nullptr,"m_JackCountLabel"}
    };

    for(int i=0;i<5;i++) {
        const int row=i+2;
        addLed(grid,w->findChild<QWidget*>(QString::fromLatin1(rows[i].live)),signalPanel,row,0);
        if(rows[i].recorded)
            addLed(grid,w->findChild<QWidget*>(QString::fromLatin1(rows[i].recorded)),signalPanel,row,1);
        addLeftHelpLabel(grid,w->findChild<QLabel*>(QString::fromLatin1(rows[i].label)),signalPanel,row,2);
    }
    auto *syncLed=additionalLed(signalPanel,QStringLiteral("extraDtc3TeethLed"),true);
    auto *syncLabel=new QLabel(I18n::text(29240),signalPanel);
    syncLabel->setObjectName(QStringLiteral("extraDtc3TeethLabel"));
    syncLabel->setWordWrap(true);
    if(auto *reference=w->findChild<QLabel*>(QStringLiteral("m_RPMSensorLabel")))
        syncLabel->setFont(reference->font());
    // Same grid, same dimensions, same reserved help column as the existing
    // live indicators. No extra offset when the help icon is absent.
    syncLabel->setContentsMargins(30,0,0,0);
    addLed(grid,syncLed,signalPanel,7,0);
    addLeftHelpLabel(grid,syncLabel,signalPanel,7,2);
    // Preserve the original counter indicator and its position. A confirmed
    // alert condition is required before activating it.
    sv->addLayout(grid,1);

    // The lower signal panel must not contain a separator below the RPM row.
    // The title-bar separator belongs to errorsLive (the parent card), so it
    // is not affected here.
    const QList<QFrame*> panelFrames=signalPanel->findChildren<QFrame*>(QString(),Qt::FindDirectChildrenOnly);
    for(QFrame *line:panelFrames)
        if(line && line!=signalPanel && line->frameShape()==QFrame::HLine)
            line->hide();

    QObject *sys=w->findChild<QObject*>(QStringLiteral("m_engine_error"));
    SystemStatePreview *preview=new SystemStatePreview(qobject_cast<MainWindow*>(w),sys,bottom);
    preview->setMinimumWidth(190);preview->setMaximumWidth(220);

    QHBoxLayout *body=new QHBoxLayout;
    body->setContentsMargins(0,0,0,0);
    body->setSpacing(8);
    body->addWidget(signalPanel,1);
    body->addWidget(preview,0);
    bv->addLayout(body,1);
}

static void rebalanceErrorsHeight(QFrame *top,QFrame *bottom)
{
    if(!top || !bottom) return;
    QWidget *page=top->parentWidget();
    QVBoxLayout *root=page?qobject_cast<QVBoxLayout*>(page->layout()):nullptr;
    if(!root) return;
    const int topIndex=root->indexOf(top);
    const int bottomIndex=root->indexOf(bottom);
    if(topIndex>=0) root->setStretch(topIndex,7);
    if(bottomIndex>=0) root->setStretch(bottomIndex,5);
}

static void applyErrorsVisualPhaseOne(QMainWindow *w)
{
    if(!w || w->property("errorsVisualPhaseOne").toBool()) return;
    QFrame *top=w->findChild<QFrame*>(QStringLiteral("errorsStored"));
    QFrame *bottom=w->findChild<QFrame*>(QStringLiteral("errorsLive"));
    if(!top || !bottom) return;

    w->setProperty("errorsVisualPhaseOne",true);
    compactStoredPanel(w,top);
    rebuildLivePanel(w,bottom);
    rebalanceErrorsHeight(top,bottom);
}

static void composeErrors(QMainWindow *window)
{
    QWidget *page=pageFor(window,QStringLiteral("errors"));if(!page||page->property("majorPageRebuilt").toBool())return;page->setProperty("majorPageRebuilt",true);
    QWidget *stored=window->findChild<QWidget*>(QStringLiteral("layoutWidget_3"));
    QWidget *live=window->findChild<QWidget*>(QStringLiteral("layoutWidget_9"));
    if(stored)stored->setParent(page);if(live)live->setParent(page);
    QVBoxLayout *root=nullptr;beginPage(page,root);
    QFrame *top=card(page,QStringLiteral("errorsStored"),I18n::text(7136));
    if(stored){prepWidget(stored);stored->setParent(top);static_cast<QVBoxLayout*>(top->layout())->addWidget(stored,1);}root->addWidget(top,3);
    QFrame *bottom=card(page,QStringLiteral("errorsLive"),I18n::text(7137));
    if(live){prepWidget(live);live->setParent(bottom);static_cast<QVBoxLayout*>(bottom->layout())->addWidget(live,1);}root->addWidget(bottom,2);
}

static void composeActuators(QMainWindow *window)
{
    QWidget *page=pageFor(window,QStringLiteral("actuators"));
    if(!page || page->property("majorPageRebuilt").toBool())return;
    page->setProperty("majorPageRebuilt",true);

    QWidget *list=window->findChild<QWidget*>(QStringLiteral("layoutWidget_4"));
    QWidget *pos=window->findChild<QWidget*>(QStringLiteral("layoutWidget_5"));
    QWidget *slider=window->findChild<QWidget*>(QStringLiteral("layoutWidget_6"));
    QLabel *info=page->findChild<QLabel*>(QStringLiteral("label"));
    QLabel *o2Label=page->findChild<QLabel*>(QStringLiteral("m_O2Heater"));
    QPushButton *o2On=page->findChild<QPushButton*>(QStringLiteral("m_O2Heater_OnButton"));
    QPushButton *move=page->findChild<QPushButton*>(QStringLiteral("m_moveIACButton"));
    QPushButton *minus=page->findChild<QPushButton*>(QStringLiteral("m_IACMinusButton"));
    QPushButton *plus=page->findChild<QPushButton*>(QStringLiteral("m_IACPlusButton"));

    const QList<QWidget*> controls={list,pos,slider,info,move,minus,plus};
    for(QWidget *widget:controls)
        if(widget)widget->setParent(page);

    QVBoxLayout *root=nullptr;
    beginPage(page,root);
    root->setContentsMargins(6,5,6,5);
    root->setSpacing(4);
    QHBoxLayout *body=new QHBoxLayout;
    body->setSpacing(8);

    QFrame *left=card(page,QStringLiteral("actuatorListCard"),I18n::text(4001).toUpper());
    left->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Expanding);
    left->setMinimumWidth(410);
    static_cast<QVBoxLayout*>(left->layout())->setContentsMargins(8,6,8,6);
    static_cast<QVBoxLayout*>(left->layout())->setSpacing(4);
    if(list){
        prepWidget(list);
        list->setParent(left);
        // Centre the complete existing grid, keeping all actuator signals
        // and button sizes intact.  On narrower windows it can still shrink.
        list->setMinimumWidth(0);
        list->setMaximumWidth(650);
        list->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Expanding);

        if(QGridLayout *grid=qobject_cast<QGridLayout*>(list->layout())){
            grid->setContentsMargins(6,4,6,4);
            grid->setHorizontalSpacing(6);
            grid->setVerticalSpacing(3);
            for(int row=0;row<grid->rowCount();++row)grid->setRowStretch(row,0);
            for(int row=2;row<=10;++row)grid->setRowStretch(row,1);
            for(int row:{12,13,15})grid->setRowStretch(row,1);

            // The functional names belong to the left edge of their column,
            // not flush against the first button. Keep the group title centred.
            // Width belongs to the grid, not the longest translated label.
            for(int row:{2,3,4,5,6,7,8,9,10,12,13,15}){
                QLayoutItem *cell=grid->itemAtPosition(row,0);
                QLabel *label=cell?qobject_cast<QLabel*>(cell->widget()):nullptr;
                if(!label)continue;
                label->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
                label->setMinimumSize(0,28);
                label->setWordWrap(true);
                label->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);
            }
            grid->setColumnStretch(0,2);
            grid->setColumnMinimumWidth(0,150);
            for(int row:{0,11,14})grid->setRowMinimumHeight(row,0);
            for(int col=1;col<=3;++col){grid->setColumnStretch(col,1);grid->setColumnMinimumWidth(col,65);}

            for(QPushButton *button:list->findChildren<QPushButton*>()){
                button->setMinimumSize(0,28);
                button->setProperty("actuatorCompactButton",true);
                button->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Fixed);
                button->setMinimumWidth(0);
            }

            // Transfer the lambda warning to the label's RIGHT help bubble.
            // A left-side property reserved 30px before the text, reversing
            // the requested reading order.
            if(o2Label && o2On){
                const QString tip=o2On->toolTip();
                o2Label->setProperty("helpBubbleLeft",false);
                if(!tip.isEmpty()){
                    o2On->setToolTip(QString());
                    o2Label->setToolTip(tip);
                }
                // Allow two lines and retain the existing right-hand help icon.
                o2Label->setWordWrap(true);
                const QMargins margins=o2Label->contentsMargins();
                o2Label->setContentsMargins(0,margins.top(),30,margins.bottom());
                o2Label->setMinimumWidth(0);
                o2Label->setMaximumWidth(QWIDGETSIZE_MAX);
                o2Label->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);
                grid->setAlignment(o2Label,Qt::AlignVCenter);
            }
            // Show the actual received result next to each existing test button.
            for(int row:{12,13}){
                auto *result=new QLabel(QString(),list);
                result->setObjectName(row==12?QStringLiteral("injectorTestReply"):QStringLiteral("coilTestReply"));
                result->setWordWrap(true);result->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);
                grid->addWidget(result,row,2,1,2);
            }
        }

        QHBoxLayout *centered=new QHBoxLayout;
        centered->setContentsMargins(0,0,0,0);
        centered->setSpacing(0);
        if(auto *reset=list->findChild<QPushButton*>(QStringLiteral("m_AllActuatorsOffButton")))reset->setText(I18n::text(2972418));
        centered->addWidget(list,1);
        for(const char *name:{"m_empty1","m_empty2"})if(auto *empty=list->findChild<QLabel*>(QString::fromLatin1(name)))empty->hide();
        static_cast<QVBoxLayout*>(left->layout())->addLayout(centered,1);
    }
    body->addWidget(left,5);

    QFrame *right=card(page,QStringLiteral("actuatorIacCard"),I18n::text(7138));
    right->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Expanding);
    right->setMinimumWidth(460);
    QVBoxLayout *rv=static_cast<QVBoxLayout*>(right->layout());
    rv->setContentsMargins(8,6,8,6);
    rv->setSpacing(3);
    if(auto *title=cardTitle(right)){
        title->setObjectName(QStringLiteral("actuatorStepperTitle"));
        title->setFixedHeight(22);title->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
    }
    // A single owned section allows the instrument test to exclude the existing
    // stepper controls without changing their individual enabled states.
    auto *iacSection=new QWidget(right);
    iacSection->setObjectName(QStringLiteral("actuatorStepperControls"));
    iacSection->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);
    auto *iacLayout=new QVBoxLayout(iacSection);
    iacLayout->setContentsMargins(0,0,0,0);iacLayout->setSpacing(3);
    if(info){
        info->setParent(right);
        info->setWordWrap(true);
        info->setTextFormat(Qt::PlainText);
        info->setText(I18n::text(2972415));
        info->setMinimumSize(0,30);
        info->setMaximumHeight(56);
        info->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);
        info->show();
        iacLayout->addWidget(info);
    }
    if(pos){
        prepWidget(pos);
        pos->setParent(right);
        pos->setMinimumHeight(24);
        pos->setMaximumHeight(28);
        pos->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
        if(auto *caption=pos->findChild<QLabel*>(QStringLiteral("m_idleBypassLabel_2"))){
            caption->setText(I18n::text(2972416));
            caption->setMinimumWidth(0);
            caption->setWordWrap(true);
            caption->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);
            caption->setMinimumWidth(140);
        }
        if(pos->layout())pos->layout()->setContentsMargins(0,0,0,0);
        pos->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Fixed);
        iacLayout->addWidget(pos);
    }
    if(slider){
        prepWidget(slider);
        slider->setParent(right);
        slider->setMinimumHeight(25);
        slider->setMaximumHeight(30);
        slider->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
        iacLayout->addWidget(slider);
    }

    QHBoxLayout *buttons=new QHBoxLayout;
    buttons->setSpacing(8);
    for(QPushButton *button:{minus,move,plus}){
        if(!button)continue;
        button->setParent(right);
        button->setMinimumSize(0,30);
        button->setProperty("actuatorCompactButton",true);
        button->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Fixed);
        if(button==move)button->setText(I18n::text(2972417));
        else button->setFixedWidth(36);
        button->show();
        buttons->addWidget(button,button==move?1:0);
    }
    iacLayout->addLayout(buttons);
    rv->addWidget(iacSection);

    auto *main=qobject_cast<MainWindow*>(window);
    auto *worker=main?main->memsInterface():nullptr;
    auto *instruments=new QHBoxLayout;
    instruments->setSpacing(6);
    auto makeInstrument=[&](bool tachometer){
        QFrame *panel=card(right,tachometer?QStringLiteral("actuatorRevCounterTestCard"):
            QStringLiteral("actuatorGaugeTestCard"),I18n::text(tachometer?2972404:2972409));
        auto *v=static_cast<QVBoxLayout*>(panel->layout());
        v->setContentsMargins(6,5,6,5);v->setSpacing(3);
        panel->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
        if(auto *title=cardTitle(panel)){
            title->setWordWrap(true);
            title->setFixedHeight(2*QFontMetrics(title->font()).lineSpacing()+2);
            title->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Fixed);
        }
        QWidget *dial=tachometer?createActuatorRevCounterGauge(panel):createActuatorTemperatureGauge(panel);
        v->addWidget(dial,1);
        auto addButton=[&](int key,const char *name){
            auto *button=new QPushButton(I18n::text(key),panel);
            button->setObjectName(QString::fromLatin1(name));
            button->setProperty("actuatorCompactButton",true);
            button->setFixedHeight(28);button->setMinimumWidth(0);
            button->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Fixed);
            button->setProperty("helpBubbleNoIcon",true);
            button->setEnabled(false);
            if(worker){
                const int action=key==2971802?1:key==2971803?2:key==2972406?3:0;
                QObject::connect(button,&QPushButton::clicked,window,[window,worker,action](){
                    if(action==0)worker->cancelInstrumentTest();
                    else {
                        const bool accepted=worker->requestInstrumentTest(action);
                        if(auto *result=window->findChild<QLabel*>(QStringLiteral("instrumentTestResult")))
                            result->setText(I18n::text(accepted?2972420:2972423));
                    }
                    Mems16Na::SessionEvidence snapshot;
                    worker->getData(&snapshot);
                    updateGaugeTestAvailability(window,worker->isConnected(),&snapshot);
                });
            }
            v->addWidget(button);
        };
        if(tachometer){
            addButton(2972406,"revCounterTestStart");
            v->addSpacing(31);
            addButton(2971804,"revCounterTestStop");
        }else{
            addButton(2971802,"gaugeTestMinimum");
            addButton(2971803,"gaugeTestMaximum");
            addButton(2971804,"gaugeTestStop");
        }
        auto addNote=[&](int key,const char *name,int lines,bool orange){
            auto *label=new QLabel(I18n::text(key),panel);
            label->setObjectName(QString::fromLatin1(name));
            label->setWordWrap(true);
            label->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
            // Identical rows in both cards keep all buttons aligned. The
            // permanent safety instruction has room for three wrapped lines;
            // the compact availability label has room for two.
            label->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Fixed);
            label->setFixedHeight(lines==2?48:36);
            if(orange){
                const bool dark=qApp&&qApp->property("ecuDarkTheme").toBool();
                label->setStyleSheet(dark?QStringLiteral("color:#ff9828;background:transparent;border:0;"):
                    QStringLiteral("color:#a04a00;background:transparent;border:0;"));
            }
            v->addWidget(label);
        };
        addNote(2972405,tachometer?"revCounterTestSafety":"gaugeTestSafety",2,true);
        addNote(2972428,tachometer?"revCounterTestReason":"gaugeTestReason",3,true);
        // The full observation guidance remains accessible without consuming
        // two duplicate permanent text blocks. Safety and availability stay visible.
        panel->setToolTip(I18n::text(tachometer?2972408:2971811));
        panel->setProperty("helpBubbleNoIcon",true);
        instruments->addWidget(panel,1);
    };
    makeInstrument(false);
    makeInstrument(true);
    rv->addLayout(instruments,1);
    // Both instruments share the same visible observation instruction.
    auto *observe=new QLabel(I18n::text(2972414),right);
    observe->setObjectName(QStringLiteral("instrumentTestResult"));
    observe->setWordWrap(true);
    observe->setAlignment(Qt::AlignCenter);
    QSizePolicy observePolicy(QSizePolicy::Ignored,QSizePolicy::Minimum);
    observePolicy.setHeightForWidth(true);
    observe->setSizePolicy(observePolicy);
    rv->addWidget(observe);
    if(worker){
        QObject::connect(worker,&MEMSInterface::instrumentTestResult,window,[window,worker](int,int result){
            if(auto *label=window->findChild<QLabel*>(QStringLiteral("instrumentTestResult"))){
                const int key=result==1?2972421:result==2?2972422:result==3?2972423:result==4?2972424:2972425;
                label->setText(I18n::text(key));
                label->setToolTip(label->text());
            }
            Mems16Na::SessionEvidence snapshot;
            worker->getData(&snapshot);
            updateGaugeTestAvailability(window,worker->isConnected(),&snapshot);
        });
        if(auto *tabs=window->findChild<QTabWidget*>(QStringLiteral("Tab_main")))
            QObject::connect(tabs,&QTabWidget::currentChanged,window,[tabs,page,worker](int index){
                QWidget *visible=tabs->widget(index);
                if(auto *scroll=qobject_cast<QScrollArea*>(visible))visible=scroll->widget();
                if(visible!=page)worker->cancelInstrumentTest();
            });
    }
    // Reuse the actual system status, with a shorter horizontal arrangement.
    QObject *systemSource=window->findChild<QObject*>(QStringLiteral("m_engine_error"));
    SystemStatePreview *state=new SystemStatePreview(
        qobject_cast<MainWindow*>(window),systemSource,right);
    state->setObjectName(QStringLiteral("actuatorSystemState"));
    state->setMinimumSize(200,76);
    state->setMaximumSize(QWIDGETSIZE_MAX,76);
    state->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
    rv->addWidget(state);

    body->addWidget(right,6);
    root->addLayout(body,1);
    page->setMinimumSize(0,0);
    page->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
}

} // namespace

// Native interactive ECU page: legacy .ui uses absolute coordinates,
 // which clipped the command and warning labels.  Own one responsive
 // layout here, preserving the existing widgets and their ECU signals.
static void composeInteractive(QMainWindow *window)
{
    QWidget *page=pageFor(window,QStringLiteral("ECU"));
    if(!page || page->property("interactivePageLayoutBuilt").toBool())return;
    page->setProperty("interactivePageLayoutBuilt",true);
    page->setMinimumSize(0,0);
    page->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
    page->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);

    if(QWidget *empty=page->findChild<QWidget*>(QStringLiteral("label_9")))
        empty->hide();

    QVBoxLayout *layout=qobject_cast<QVBoxLayout*>(page->layout());
    if(!layout){
        layout=new QVBoxLayout(page);
        layout->setContentsMargins(32,22,32,20);
        layout->setSpacing(12);
    }

    const char *texts[]={
        "label_23", // Readmems prerequisite
        "label_24", // Open a command prompt
        "label_48", // Dir command
        "label_49", // Interactive command
        "label_50", // Serial port
        "ecuResetSettingsWarningLabel", // 0xFA warning
    };
    for(const char *name:texts){
        QLabel *label=page->findChild<QLabel*>(QString::fromLatin1(name));
        if(!label)continue;
        // Translate and display the source wording, with long lines wrapped.
        // The old HTML markup is not needed for these instructional labels.
        if(Qt::mightBeRichText(label->text())){
            QTextDocument document;
            document.setHtml(label->text());
            label->setTextFormat(Qt::PlainText);
            label->setText(document.toPlainText().trimmed());
        }
        label->setFont(DataPageTypography::body(label->font()));
        label->setStyleSheet(QStringLiteral(
            "color:#ffffff;background:transparent;border:0;font-weight:400;"));
        label->setWordWrap(true);
        label->setMinimumSize(0,0);
        label->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
        label->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
        label->show();
        layout->addWidget(label);
    }

    // Preserve the original text and purpose of the 0x0F field.
    if(QLineEdit *note=page->findChild<QLineEdit*>(QStringLiteral("lineEdit_2"))){
        note->setFont(DataPageTypography::body(note->font()));
        note->setStyleSheet(QStringLiteral(
            "QLineEdit{font-weight:400;color:#ffffff;background:transparent;"
            "border:0;padding:0 2px;}"));
        note->setMinimumWidth(0);
        note->setMaximumWidth(QWIDGETSIZE_MAX);
        note->setMinimumHeight(30);
        note->setMaximumHeight(32);
        note->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
        note->show();
        layout->addWidget(note);
    }

    if(QLabel *warning=page->findChild<QLabel*>(QStringLiteral("label_22"))){
        // Inline legacy HTML forces 12pt red text.  Use the shared 10pt
        // body typeface while retaining red as a meaningful safety warning.
        if(Qt::mightBeRichText(warning->text())){
            QTextDocument document;
            document.setHtml(warning->text());
            warning->setTextFormat(Qt::PlainText);
            warning->setText(document.toPlainText().trimmed());
        }
        warning->setFont(DataPageTypography::body(warning->font()));
        warning->setStyleSheet(QStringLiteral(
            "color:#ff3535;background:transparent;border:0;font-weight:400;"));
        warning->setWordWrap(true);
        warning->setMinimumSize(0,0);
        warning->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
        warning->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
        warning->show();
        layout->addWidget(warning);
    }

    if(QPushButton *button=page->findChild<QPushButton*>(QStringLiteral("interactive_push_button"))){
        button->setFont(DataPageTypography::body(button->font()));
        button->setMinimumWidth(160);
        button->setMinimumHeight(34);
        button->setMaximumWidth(QWIDGETSIZE_MAX);
        button->setMaximumHeight(38);
        button->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Fixed);
        button->show();
        layout->addWidget(button,0,Qt::AlignLeft);
    }
    // Keep the original interactive command instructions and button intact.
    // Add the editable report-style notepad below them, in their own page.
    QGroupBox *notesBox = new QGroupBox(I18n::text(18120), page);
    notesBox->setObjectName(QStringLiteral("interactiveNotesBox"));
    notesBox->setMinimumHeight(170);
    notesBox->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    QVBoxLayout *notesLayout = new QVBoxLayout(notesBox);
    QPlainTextEdit *notesEditor = new QPlainTextEdit(notesBox);
    notesEditor->setObjectName(QStringLiteral("interactiveNotesEditor"));
    notesEditor->setReadOnly(false);
    // Preserve any text entered in the misplaced 195 notepad, without
    // changing or deleting its previous storage key.
    QSettings notesSettings(QSettings::IniFormat,QSettings::UserScope,PROJECTNAME);
    const QString currentNotesKey = QStringLiteral("Interactive/Notes");
    const QString previousNotesKey = QStringLiteral("Rosco/Notes");
    notesEditor->setPlainText(notesSettings.value(
        notesSettings.contains(currentNotesKey) ? currentNotesKey : previousNotesKey).toString());
    notesLayout->addWidget(notesEditor);
    QObject::connect(notesEditor,&QPlainTextEdit::textChanged,page,[notesEditor]() {
        QSettings settings(QSettings::IniFormat,QSettings::UserScope,PROJECTNAME);
        settings.setValue(QStringLiteral("Interactive/Notes"),notesEditor->toPlainText());
    });
    layout->addStretch(1);
    layout->addWidget(notesBox,2);
}

unsigned updateAdditionalErrors(QMainWindow *window,bool connected,const Mems16Na::SessionEvidence *evidence)
{
    if(!window)return 0;
    const char *f=evidence?evidence->firmwareName().c_str():"";
    const auto d5=evidence?evidence->extendedDtc(5):MemsExtendedDtc::Result{MemsExtendedDtc::Status::Missing,0,0,0,0};
    const auto d3=evidence?evidence->extendedDtc(3):MemsExtendedDtc::Result{MemsExtendedDtc::Status::Missing,0,0,0,0};
    const unsigned supported=MemsExtendedDtc::extraFaultMask(f);
    for(int i=0;i<6;++i){
        const unsigned bit=2u<<i;
        const bool available=connected&&(supported&bit)&&d5.status!=MemsExtendedDtc::Status::Missing&&d5.raw!=255;
        if(auto *led=window->findChild<QLedIndicator*>(QStringLiteral("extraDtc5Led%1").arg(bit)))led->setChecked(available&&(d5.faults&bit));
        if(auto *label=window->findChild<QLabel*>(QStringLiteral("extraDtc5Label%1").arg(bit))){
            const QString name=I18n::text(29245+i);
            const QString display=QStringLiteral("%1 - ").arg(25+i)+(!connected||(supported&bit)?name:I18n::text(2972201));
            if(label->text()!=display)label->setText(display);
            const QString detail=!connected?QString():(supported&bit)?(available?I18n::text(29021):I18n::text(29048)):name;
            if(label->toolTip()!=detail)label->setToolTip(detail);
        }
    }
    const bool stateSupported=MemsExtendedDtc::sync19(f);
    const bool stateAvailable=connected&&stateSupported&&d3.status==MemsExtendedDtc::Status::Decoded;
    if(auto *led=window->findChild<QLedIndicator*>(QStringLiteral("extraDtc3TeethLed"))){
        const bool detected=stateAvailable&&(d3.states&4);
        led->setOnColor1(detected?QColor("#6bdd45"):QColor(255,0,0));
        led->setOnColor2(detected?QColor("#348a22"):QColor(176,0,2));
        // Off (dark red) means no valid state; illuminated red means no recent teeth.
        led->setChecked(stateAvailable);
        led->update();
    }
    if(auto *label=window->findChild<QLabel*>(QStringLiteral("extraDtc3TeethLabel"))){
        label->setText(!connected||stateSupported?I18n::text(29240):I18n::text(2972201));
        label->setToolTip(connected&&stateSupported&&!stateAvailable?I18n::text(29048):QString());
    }
    return connected?d5.faults&supported:0u;
}

void updateGaugeTestAvailability(QMainWindow *window,bool connected,const Mems16Na::SessionEvidence *evidence)
{
    if(!window)return;
    auto *main=qobject_cast<MainWindow*>(window);
    auto *worker=main?main->memsInterface():nullptr;
    const int request=connected&&worker?worker->instrumentRequest():0;
    const bool normal=connected&&worker&&worker->instrumentNormalMode();
    const bool dirty=connected&&worker&&worker->instrumentOtherForced();
    for(const char *name:{"actuatorListCard","actuatorStepperControls"})
        if(auto *section=window->findChild<QWidget*>(QString::fromLatin1(name)))section->setEnabled(request==0);
    const bool dark=qApp&&qApp->property("ecuDarkTheme").toBool();
    const QString style=dark?QStringLiteral("color:#ff9828;background:transparent;border:0;"):
                             QStringLiteral("color:#a04a00;background:transparent;border:0;");
    auto update=[&](bool tach){
        const auto action=tach?Mems218::InstrumentAction::Tachometer:Mems218::InstrumentAction::GaugeMin;
        const bool ownRequest=tach?request==3:request==1||request==2;
        int key=2971805;
        bool ready=false;
        if(connected){
            if(!evidence||evidence->firmwareName().empty())key=2971806;
            else if(!Mems218::instrumentSupported(action,*evidence))key=tach?2972407:2971807;
            else if(ownRequest)key=2972420;
            else if(request!=0||!normal)key=2972426;
            else if(dirty)key=2972427;
            else if(!Mems217::present(Mems217::frame80(*evidence),25)||
                    !Mems217::present(Mems217::frame7d(*evidence),1))key=2971808;
            else if(!Mems218::instrumentSafeSample(action,*evidence))key=2971809;
            else{ready=true;key=2972419;}
        }
        const QString message=I18n::text(key);
        int shortKey=key;
        switch(key){
        case 2971805:shortKey=2972428;break;
        case 2971806:case 2971807:case 2972407:shortKey=2972429;break;
        case 2971808:shortKey=2972430;break;
        case 2971809:shortKey=2972431;break;
        case 2972419:shortKey=2972432;break;
        case 2972426:shortKey=2972433;break;
        case 2972427:shortKey=2972434;break;
        default:break;
        }
        const QString shortMessage=I18n::text(shortKey);
        if(auto *reason=window->findChild<QLabel*>(tach?QStringLiteral("revCounterTestReason"):QStringLiteral("gaugeTestReason"))){
            if(reason->text()!=shortMessage)reason->setText(shortMessage);
            reason->setToolTip(message);
            reason->setProperty("helpBubbleNoIcon",true);
            if(reason->styleSheet()!=style)reason->setStyleSheet(style);
        }
        if(auto *safety=window->findChild<QLabel*>(tach?QStringLiteral("revCounterTestSafety"):QStringLiteral("gaugeTestSafety")))
            if(safety->styleSheet()!=style)safety->setStyleSheet(style);
        const QList<const char*> starts=tach?QList<const char*>{"revCounterTestStart"}:QList<const char*>{"gaugeTestMinimum","gaugeTestMaximum"};
        for(const char *name:starts)if(auto *button=window->findChild<QPushButton*>(QString::fromLatin1(name))){
            button->setEnabled(ready);button->setToolTip(message);
        }
        if(auto *stop=window->findChild<QPushButton*>(tach?QStringLiteral("revCounterTestStop"):QStringLiteral("gaugeTestStop"))){
            stop->setEnabled(connected&&ownRequest);stop->setToolTip(message);
        }
    };
    update(false);update(true);
}

void buildEcuControlPages(QMainWindow *window)
{
    if (!window)
        return;
    // Create both original control pages in their actual owner module.
    // No late global event filter or repeated 240/900 ms corrective passes.
    composeErrors(window);
    composeActuators(window);
    updateGaugeTestAvailability(window,false,nullptr);
    composeInteractive(window);
    applyErrorsVisualPhaseOne(window);
}
