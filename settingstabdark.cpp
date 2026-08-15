#include <QApplication>
#include <QCoreApplication>
#include <QEvent>
#include <QFrame>
#include <QGridLayout>
#include <QMainWindow>
#include <QSizePolicy>
#include <QTimer>
#include <QWidget>

namespace {

static QGridLayout *gridContaining(QWidget *root,const QList<QWidget*> &widgets)
{
    if(!root || widgets.isEmpty()) return nullptr;
    QGridLayout *best=nullptr;
    int bestCount=0;
    for(QGridLayout *grid:root->findChildren<QGridLayout*>()) {
        int count=0;
        for(QWidget *widget:widgets)
            if(widget && grid->indexOf(widget)>=0) ++count;
        if(count>bestCount) {
            best=grid;
            bestCount=count;
        }
    }
    return best;
}

static void sizeGauge(QWidget *gauge,int width,int height)
{
    if(!gauge) return;
    gauge->setMinimumSize(width,height);
    gauge->setMaximumSize(width,height);
    gauge->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
    gauge->show();
}

static void applySettingsLayout(QMainWindow *window)
{
    if(!window) return;

    QWidget *page=window->findChild<QWidget*>(QStringLiteral("emission_tab"));
    QFrame *metrics=window->findChild<QFrame*>(QStringLiteral("settingsMetrics"));
    QFrame *states=window->findChild<QFrame*>(QStringLiteral("settingsStates"));
    QFrame *adjust=window->findChild<QFrame*>(QStringLiteral("settingsAdjust"));
    if(!page || !metrics || !states || !adjust) return;

    QWidget *stage=metrics->findChild<QWidget*>(QStringLiteral("darkSettingsGaugeStage"),Qt::FindDirectChildrenOnly);
    if(!stage) return;
    QGridLayout *grid=qobject_cast<QGridLayout*>(stage->layout());
    if(!grid) return;

    QWidget *idle=stage->findChild<QWidget*>(QStringLiteral("darkGaugeIdleError"));
    QWidget *rpm=stage->findChild<QWidget*>(QStringLiteral("darkGaugeRpm"));
    QWidget *trim=stage->findChild<QWidget*>(QStringLiteral("darkGaugeFuelTrim"));
    QWidget *ignition=stage->findChild<QWidget*>(QStringLiteral("darkGaugeIgnition"));
    QWidget *lambda=stage->findChild<QWidget*>(QStringLiteral("darkGaugeLambda"));
    if(!idle || !rpm || !trim || !ignition || !lambda) return;

    const int pageWidth=qMax(1,page->width());
    const int pageHeight=qMax(1,page->height());
    const bool compact=pageWidth<900;

    QList<QWidget*> gauges;
    gauges<<idle<<ignition<<rpm<<lambda<<trim;
    for(QWidget *gauge:gauges) grid->removeWidget(gauge);

    grid->setContentsMargins(8,4,8,4);
    grid->setHorizontalSpacing(compact?7:12);
    grid->setVerticalSpacing(8);
    for(int column=0;column<5;++column) grid->setColumnStretch(column,0);
    for(int row=0;row<3;++row) grid->setRowStretch(row,0);

    if(!compact) {
        const int usable=qMax(760,pageWidth-70);
        const int centerWidth=qBound(215,qRound(usable*0.215),275);
        const int outerWidth=qBound(150,(usable-centerWidth-48)/4,210);
        const int centerHeight=qBound(220,qRound(pageHeight*0.43),275);
        const int outerHeight=qBound(175,qRound(pageHeight*0.335),215);

        sizeGauge(idle,outerWidth,outerHeight);
        sizeGauge(ignition,outerWidth,outerHeight);
        sizeGauge(rpm,centerWidth,centerHeight);
        sizeGauge(lambda,outerWidth,outerHeight);
        sizeGauge(trim,outerWidth,outerHeight);

        grid->addWidget(idle,0,0,Qt::AlignCenter);
        grid->addWidget(ignition,0,1,Qt::AlignCenter);
        grid->addWidget(rpm,0,2,Qt::AlignCenter);
        grid->addWidget(lambda,0,3,Qt::AlignCenter);
        grid->addWidget(trim,0,4,Qt::AlignCenter);
        grid->setColumnStretch(0,10);
        grid->setColumnStretch(1,10);
        grid->setColumnStretch(2,13);
        grid->setColumnStretch(3,10);
        grid->setColumnStretch(4,10);
        grid->setRowStretch(0,1);

        const int metricsHeight=qBound(285,qRound(pageHeight*0.50),345);
        metrics->setMinimumHeight(metricsHeight);
        stage->setMinimumHeight(qMax(220,metricsHeight-48));
    } else {
        const int centerWidth=qBound(190,qRound(pageWidth*0.34),235);
        const int outerWidth=qBound(135,qRound(pageWidth*0.26),180);
        const int centerHeight=qBound(205,qRound(pageHeight*0.36),245);
        const int outerHeight=qBound(155,qRound(pageHeight*0.27),190);

        sizeGauge(idle,outerWidth,outerHeight);
        sizeGauge(trim,outerWidth,outerHeight);
        sizeGauge(ignition,outerWidth,outerHeight);
        sizeGauge(lambda,outerWidth,outerHeight);
        sizeGauge(rpm,centerWidth,centerHeight);

        grid->addWidget(idle,0,0,Qt::AlignCenter);
        grid->addWidget(rpm,0,1,2,1,Qt::AlignCenter);
        grid->addWidget(trim,0,2,Qt::AlignCenter);
        grid->addWidget(ignition,1,0,Qt::AlignCenter);
        grid->addWidget(lambda,1,2,Qt::AlignCenter);
        grid->setColumnStretch(0,10);
        grid->setColumnStretch(1,14);
        grid->setColumnStretch(2,10);
        grid->setRowStretch(0,1);
        grid->setRowStretch(1,1);

        metrics->setMinimumHeight(qBound(350,qRound(pageHeight*0.62),430));
        stage->setMinimumHeight(qBound(300,qRound(pageHeight*0.54),380));
    }

    metrics->setMaximumHeight(QWIDGETSIZE_MAX);
    stage->setMaximumHeight(QWIDGETSIZE_MAX);
    metrics->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    stage->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);

    QList<QWidget*> panels;
    panels<<metrics<<states<<adjust;
    QGridLayout *body=gridContaining(page,panels);
    if(body && !compact) {
        for(QWidget *panel:panels) body->removeWidget(panel);
        body->addWidget(metrics,0,0,1,2);
        body->addWidget(states,1,0);
        body->addWidget(adjust,1,1);
        body->setColumnStretch(0,2);
        body->setColumnStretch(1,3);
        body->setRowStretch(0,5);
        body->setRowStretch(1,3);
        body->setHorizontalSpacing(10);
        body->setVerticalSpacing(10);
        states->setMinimumHeight(0);
        adjust->setMinimumHeight(0);
        states->setMaximumHeight(QWIDGETSIZE_MAX);
        adjust->setMaximumHeight(QWIDGETSIZE_MAX);
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
            QTimer::singleShot(180,window,[window](){applySettingsLayout(window);});
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
