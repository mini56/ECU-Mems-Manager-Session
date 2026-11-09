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
#include "i18n.h"

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

class MajorPagesInstaller:public QObject
{
public:explicit MajorPagesInstaller(QObject*p=nullptr):QObject(p){}
protected:bool eventFilter(QObject*watched,QEvent*event)override
    {
        if((event->type()!=QEvent::Show&&event->type()!=QEvent::Polish)||!watched)return QObject::eventFilter(watched,event);
        QMainWindow *w=qobject_cast<QMainWindow*>(watched);if(!w||w->objectName()!=QStringLiteral("MainWindow")||w->property("majorPagesInstaller").toBool())return QObject::eventFilter(watched,event);
        w->setProperty("majorPagesInstaller",true);QTimer::singleShot(60,w,[w](){composeErrors(w);composeActuators(w);});return QObject::eventFilter(watched,event);
    }
};

void installMajorPages(){QApplication *a=qobject_cast<QApplication*>(QCoreApplication::instance());if(!a)return;a->installEventFilter(new MajorPagesInstaller(a));}

}

Q_COREAPP_STARTUP_FUNCTION(installMajorPages)
