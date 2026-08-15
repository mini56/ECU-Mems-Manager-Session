#include <QApplication>
#include <QCoreApplication>
#include <QEvent>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayout>
#include <QMainWindow>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QSizePolicy>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include "i18n.h"

namespace {

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
    button->setMinimumHeight(23);
    button->setMaximumHeight(25);
    button->setMinimumWidth(150);
    button->setMaximumWidth(230);
    button->setSizePolicy(QSizePolicy::Maximum,QSizePolicy::Fixed);
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
    explicit SystemStatePreview(QWidget *parent=nullptr):QWidget(parent)
    {
        setMinimumWidth(150);
        setMaximumWidth(210);
        setMinimumHeight(135);
        setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Expanding);
        setAttribute(Qt::WA_TransparentForMouseEvents,true);
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

        const QPointF c(width()/2.0,height()*0.54);
        const qreal rr=qMin(width()*0.19,height()*0.19);
        QPainterPath shield;
        shield.moveTo(c.x(),c.y()-rr);
        shield.lineTo(c.x()+rr*.75,c.y()-rr*.65);
        shield.lineTo(c.x()+rr*.62,c.y()+rr*.28);
        shield.quadTo(c.x(),c.y()+rr,c.x()-rr*.62,c.y()+rr*.28);
        shield.lineTo(c.x()-rr*.75,c.y()-rr*.65);
        shield.closeSubpath();
        p.setPen(QPen(QColor("#64727c"),2.0));
        p.setBrush(Qt::NoBrush);
        p.drawPath(shield);

        QFont sub=p.font();
        sub.setBold(false);
        sub.setPointSizeF(6.2);
        p.setFont(sub);
        p.setPen(QColor("#8d99a3"));
        p.drawText(QRectF(8,qMax(30,height()-38),qMax(0,width()-16),30),
                   Qt::AlignCenter|Qt::TextWordWrap,I18n::text(7145));
    }
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

    SystemStatePreview *preview=new SystemStatePreview(bottom);

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

class ErrorsVisualInstaller : public QObject
{
public:
    explicit ErrorsVisualInstaller(QObject *parent=nullptr):QObject(parent){}
protected:
    bool eventFilter(QObject *watched,QEvent *event) override
    {
        QMainWindow *w=qobject_cast<QMainWindow*>(watched);
        if(!w || w->objectName()!=QStringLiteral("MainWindow"))
            return QObject::eventFilter(watched,event);

        if((event->type()==QEvent::Show || event->type()==QEvent::Polish) &&
           !w->property("errorsVisualScheduled").toBool()) {
            w->setProperty("errorsVisualScheduled",true);
            QTimer::singleShot(240,w,[w](){applyErrorsVisualPhaseOne(w);});
            QTimer::singleShot(900,w,[w](){applyErrorsVisualPhaseOne(w);});
        }
        return QObject::eventFilter(watched,event);
    }
};

void installErrorsVisualPhaseOne()
{
    QApplication *app=qobject_cast<QApplication*>(QCoreApplication::instance());
    if(app) app->installEventFilter(new ErrorsVisualInstaller(app));
}

}

Q_COREAPP_STARTUP_FUNCTION(installErrorsVisualPhaseOne)
