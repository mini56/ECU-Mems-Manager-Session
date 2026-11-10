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
#include <QSizePolicy>
#include <QFontMetrics>
#include <QPainterPath>
#include <QPainter>
#include <QPushButton>
#include <QTabWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include "i18n.h"
#include "mainwindow.h"
#include "ecupages.h"

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

        const QRectF outer=QRectF(rect()).adjusted(.5,.5,-.5,-.5);
        p.setPen(QPen(QColor("#34414a"),1.0));
        p.setBrush(QColor("#0c1217"));
        p.drawRoundedRect(outer,5,5);

        QFont f=p.font();
        f.setBold(true);
        f.setPointSizeF(8.2);
        p.setFont(f);
        p.setPen(QColor("#edf2f4"));
        p.drawText(QRectF(6,5,qMax(0,width()-12),18),Qt::AlignCenter,I18n::text(7149));

        p.setPen(QPen(QColor("#27333b"),1.0));
        p.drawLine(QPointF(8,25),QPointF(qMax(8,width()-8),25));

        // Same source, rule, colours and translated states as the Overview card.
        const bool fault=m_source?m_source->property("checked").toBool():false;
        const bool disconnected=m_window&&m_window->isEcuDisconnected();
        const QColor state=disconnected?QColor("#ff9828"):(fault?QColor("#ff4b3b"):QColor("#6bdd45"));

        const QPointF c(width()/2.0,height()*0.48);
        const qreal rr=qMin(width()*0.19,height()*0.16);
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
        stateFont.setPointSizeF(7.8);
        p.setFont(stateFont);
        p.setPen(state);
        p.drawText(QRectF(8,height()*0.67,qMax(0,width()-16),24),
                   Qt::AlignCenter,disconnected?I18n::disconnectedStatus():(fault?I18n::text(7144):I18n::text(7143)));

        QFont sub=p.font();
        sub.setBold(false);
        sub.setPointSizeF(6.2);
        p.setFont(sub);
        p.setPen(QColor("#8d99a3"));
        p.drawText(QRectF(8,qMax(30,height()-38),qMax(0,width()-16),30),
                   Qt::AlignCenter|Qt::TextWordWrap,I18n::text(7145));
    }

private:
    MainWindow *m_window=nullptr;
    QObject *m_source=nullptr;
};

static void compactStoredPanel(QMainWindow *w,QFrame *top)
{
    if(!w || !top) return;
    QPushButton *clear=w->findChild<QPushButton*>(QStringLiteral("m_clearFaultsButton"));
    putButtonInTitleBar(top,clear);

    if(QLabel *duplicate=w->findChild<QLabel*>(QStringLiteral("m_faultCodesLabel")))
        duplicate->hide();

    QWidget *stored=w->findChild<QWidget*>(QStringLiteral("layoutWidget_3"));
    if(stored) {
        stored->setMinimumSize(0,0);
        stored->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
        if(QGridLayout *g=qobject_cast<QGridLayout*>(stored->layout())) {
            g->setContentsMargins(0,0,0,0);
            g->setHorizontalSpacing(10);
            g->setVerticalSpacing(2);
        }
    }
}

static void addLed(QGridLayout *grid,QWidget *led,QWidget *parent,int row,int col)
{
    if(!grid || !led || !parent) return;
    moveWidget(led,parent);
    led->setFixedSize(22,22);
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
    if(topIndex>=0) root->setStretch(topIndex,5);
    if(bottomIndex>=0) root->setStretch(bottomIndex,6);
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
    QWidget *page=pageFor(window,QStringLiteral("actuators"));if(!page||page->property("majorPageRebuilt").toBool())return;page->setProperty("majorPageRebuilt",true);
    QWidget *list=window->findChild<QWidget*>(QStringLiteral("layoutWidget_4"));
    QWidget *pos=window->findChild<QWidget*>(QStringLiteral("layoutWidget_5"));
    QWidget *slider=window->findChild<QWidget*>(QStringLiteral("layoutWidget_6"));
    QLabel *info=page->findChild<QLabel*>(QStringLiteral("label"));
    QLabel *o2Label=page->findChild<QLabel*>(QStringLiteral("m_O2Heater"));
    QPushButton *o2On=page->findChild<QPushButton*>(QStringLiteral("m_O2Heater_OnButton"));
    QPushButton *move=page->findChild<QPushButton*>(QStringLiteral("m_moveIACButton"));
    QPushButton *minus=page->findChild<QPushButton*>(QStringLiteral("m_IACMinusButton"));
    QPushButton *plus=page->findChild<QPushButton*>(QStringLiteral("m_IACPlusButton"));
    if(list)list->setParent(page);if(pos)pos->setParent(page);if(slider)slider->setParent(page);if(info)info->setParent(page);if(move)move->setParent(page);if(minus)minus->setParent(page);if(plus)plus->setParent(page);

    if(o2Label&&o2On){
        const QString tip=o2On->toolTip();
        if(!tip.isEmpty()){
            o2On->setToolTip(QString());
            o2Label->setProperty("helpBubbleLeft",true);
            o2Label->setToolTip(tip);
            o2Label->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
            o2Label->setSizePolicy(QSizePolicy::Maximum,QSizePolicy::Preferred);
            if(list&&list->layout())list->layout()->setAlignment(o2Label,Qt::AlignRight|Qt::AlignVCenter);
        }
    }

    QVBoxLayout *root=nullptr;beginPage(page,root);
    QHBoxLayout *body=new QHBoxLayout;body->setSpacing(10);
    QFrame *left=card(page,QStringLiteral("actuatorListCard"),I18n::text(4001).toUpper());
    if(list){
        prepWidget(list);
        list->setParent(left);
        // Do not cap the black actuator list at 490px: its left labels were
        // clipped while the existing left card had unused horizontal space.
        list->setMaximumWidth(QWIDGETSIZE_MAX);
        list->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
        QHBoxLayout *centered=new QHBoxLayout;
        centered->setContentsMargins(0,0,0,0);
        centered->setSpacing(0);
        centered->addStretch(1);
        centered->addWidget(list,1);
        centered->addStretch(1);
        static_cast<QVBoxLayout*>(left->layout())->addLayout(centered,1);
    }
    body->addWidget(left,1);
    QFrame *right=card(page,QStringLiteral("actuatorIacCard"),I18n::text(7138));QVBoxLayout *rv=static_cast<QVBoxLayout*>(right->layout());
    if(info){info->setParent(right);info->setWordWrap(true);info->show();rv->addWidget(info);}if(pos){prepWidget(pos);pos->setParent(right);rv->addWidget(pos);}if(slider){prepWidget(slider);slider->setParent(right);rv->addWidget(slider);}
    QHBoxLayout *buttons=new QHBoxLayout;if(minus){minus->setParent(right);minus->show();buttons->addWidget(minus);}if(move){move->setParent(right);move->show();buttons->addWidget(move,1);}if(plus){plus->setParent(right);plus->show();buttons->addWidget(plus);}rv->addLayout(buttons);rv->addStretch(1);body->addWidget(right,1);
    root->addLayout(body,1);
}


} // namespace

void buildEcuControlPages(QMainWindow *window)
{
    if (!window)
        return;
    // Create both original control pages in their actual owner module.
    // No late global event filter or repeated 240/900 ms corrective passes.
    composeErrors(window);
    composeActuators(window);
    applyErrorsVisualPhaseOne(window);
}
