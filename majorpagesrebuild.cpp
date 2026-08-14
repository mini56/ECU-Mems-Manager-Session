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
#include <QPushButton>
#include <QTabWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

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

static void composeSettings(QMainWindow *window)
{
    QWidget *page=pageFor(window,QStringLiteral("emission_tab"));
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

    QVBoxLayout *root=nullptr;beginPage(page,root);
    QGridLayout *body=new QGridLayout;body->setHorizontalSpacing(10);body->setVerticalSpacing(10);body->setColumnStretch(0,3);body->setColumnStretch(1,2);

    QFrame *metrics=card(page,QStringLiteral("settingsMetrics"),QStringLiteral("ÉTAT MOTEUR"));
    QGridLayout *mg=new QGridLayout;mg->setSpacing(8);
    QWidget *gauges[]={idleErr,rpm,trim,advance,lambda};
    const char *titles[]={"ERREUR RALENTI","RÉGIME MOTEUR","CORRECTION CARBURANT","AVANCE ALLUMAGE","SONDE LAMBDA"};
    for(int i=0;i<5;i++){
        QFrame *box=new QFrame(metrics);box->setStyleSheet(QStringLiteral("background:#0b1116;border:1px solid #27323b;border-radius:4px;"));
        QVBoxLayout *bv=new QVBoxLayout(box);bv->setContentsMargins(6,6,6,6);bv->setSpacing(3);
        QLabel *tl=new QLabel(QString::fromUtf8(titles[i]),box);QFont tf=tl->font();tf.setBold(true);tf.setPointSizeF(7.2);tl->setFont(tf);tl->setAlignment(Qt::AlignCenter);tl->setStyleSheet(QStringLiteral("color:#cfd6dc;background:transparent;border:0;"));bv->addWidget(tl);
        if(gauges[i]){gauges[i]->setParent(box);gauges[i]->setMinimumHeight(80);gauges[i]->setMaximumHeight(130);gauges[i]->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);gauges[i]->show();bv->addWidget(gauges[i],1);}
        mg->addWidget(box,i<3?0:1,i<3?i:i-3);
    }
    static_cast<QVBoxLayout*>(metrics->layout())->addLayout(mg,1);
    body->addWidget(metrics,0,0,2,1);

    QFrame *states=card(page,QStringLiteral("settingsStates"),QStringLiteral("ÉTATS"));
    QGridLayout *sg=new QGridLayout;sg->setHorizontalSpacing(12);sg->setVerticalSpacing(10);
    QLabel *idleTxt=new QLabel(QStringLiteral("Contact ralenti"),states);QLabel *loopTxt=new QLabel(QStringLiteral("Boucle fermée"),states);
    if(idleLed){idleLed->setParent(states);idleLed->setFixedSize(26,26);idleLed->show();sg->addWidget(idleLed,0,0);}sg->addWidget(idleTxt,0,1);
    if(loopLed){loopLed->setParent(states);loopLed->setFixedSize(26,26);loopLed->show();sg->addWidget(loopLed,1,0);}sg->addWidget(loopTxt,1,1);sg->setColumnStretch(2,1);
    static_cast<QVBoxLayout*>(states->layout())->addLayout(sg);static_cast<QVBoxLayout*>(states->layout())->addStretch(1);
    body->addWidget(states,0,1);

    QFrame *adjust=card(page,QStringLiteral("settingsAdjust"),QStringLiteral("RÉGLAGES ECU"));
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

static void composeErrors(QMainWindow *window)
{
    QWidget *page=pageFor(window,QStringLiteral("errors"));if(!page||page->property("majorPageRebuilt").toBool())return;page->setProperty("majorPageRebuilt",true);
    QWidget *stored=window->findChild<QWidget*>(QStringLiteral("layoutWidget_3"));
    QWidget *live=window->findChild<QWidget*>(QStringLiteral("layoutWidget_9"));
    if(stored)stored->setParent(page);if(live)live->setParent(page);
    QVBoxLayout *root=nullptr;beginPage(page,root);
    QFrame *top=card(page,QStringLiteral("errorsStored"),QStringLiteral("ERREURS ENREGISTRÉES"));
    if(stored){prepWidget(stored);stored->setParent(top);static_cast<QVBoxLayout*>(top->layout())->addWidget(stored,1);}root->addWidget(top,3);
    QFrame *bottom=card(page,QStringLiteral("errorsLive"),QStringLiteral("ANOMALIES ET SIGNAUX EN DIRECT"));
    if(live){prepWidget(live);live->setParent(bottom);static_cast<QVBoxLayout*>(bottom->layout())->addWidget(live,1);}root->addWidget(bottom,2);
}

static void composeActuators(QMainWindow *window)
{
    QWidget *page=pageFor(window,QStringLiteral("actuators"));if(!page||page->property("majorPageRebuilt").toBool())return;page->setProperty("majorPageRebuilt",true);
    QWidget *list=window->findChild<QWidget*>(QStringLiteral("layoutWidget_4"));
    QWidget *pos=window->findChild<QWidget*>(QStringLiteral("layoutWidget_5"));
    QWidget *slider=window->findChild<QWidget*>(QStringLiteral("layoutWidget_6"));
    QLabel *info=page->findChild<QLabel*>(QStringLiteral("label"));
    QPushButton *move=page->findChild<QPushButton*>(QStringLiteral("m_moveIACButton"));
    QPushButton *minus=page->findChild<QPushButton*>(QStringLiteral("m_IACMinusButton"));
    QPushButton *plus=page->findChild<QPushButton*>(QStringLiteral("m_IACPlusButton"));
    if(list)list->setParent(page);if(pos)pos->setParent(page);if(slider)slider->setParent(page);if(info)info->setParent(page);if(move)move->setParent(page);if(minus)minus->setParent(page);if(plus)plus->setParent(page);
    QVBoxLayout *root=nullptr;beginPage(page,root);
    QHBoxLayout *body=new QHBoxLayout;body->setSpacing(10);
    QFrame *left=card(page,QStringLiteral("actuatorListCard"),QStringLiteral("ACTIONNEURS"));if(list){prepWidget(list);list->setParent(left);static_cast<QVBoxLayout*>(left->layout())->addWidget(list,1);}body->addWidget(left,3);
    QFrame *right=card(page,QStringLiteral("actuatorIacCard"),QStringLiteral("MOTEUR PAS-À-PAS DE RALENTI"));QVBoxLayout *rv=static_cast<QVBoxLayout*>(right->layout());
    if(info){info->setParent(right);info->setWordWrap(true);info->show();rv->addWidget(info);}if(pos){prepWidget(pos);pos->setParent(right);rv->addWidget(pos);}if(slider){prepWidget(slider);slider->setParent(right);rv->addWidget(slider);}
    QHBoxLayout *buttons=new QHBoxLayout;if(minus){minus->setParent(right);minus->show();buttons->addWidget(minus);}if(move){move->setParent(right);move->show();buttons->addWidget(move,1);}if(plus){plus->setParent(right);plus->show();buttons->addWidget(plus);}rv->addLayout(buttons);rv->addStretch(1);body->addWidget(right,2);
    root->addLayout(body,1);
}

class MajorPagesInstaller:public QObject
{
public:explicit MajorPagesInstaller(QObject*p=nullptr):QObject(p){}
protected:bool eventFilter(QObject*watched,QEvent*event)override
    {
        if((event->type()!=QEvent::Show&&event->type()!=QEvent::Polish)||!watched)return QObject::eventFilter(watched,event);
        QMainWindow *w=qobject_cast<QMainWindow*>(watched);if(!w||w->objectName()!=QStringLiteral("MainWindow")||w->property("majorPagesInstaller").toBool())return QObject::eventFilter(watched,event);
        w->setProperty("majorPagesInstaller",true);QTimer::singleShot(60,w,[w](){composeSettings(w);composeErrors(w);composeActuators(w);});return QObject::eventFilter(watched,event);
    }
};

void installMajorPages(){QApplication *a=qobject_cast<QApplication*>(QCoreApplication::instance());if(!a)return;a->installEventFilter(new MajorPagesInstaller(a));}

}

Q_COREAPP_STARTUP_FUNCTION(installMajorPages)
