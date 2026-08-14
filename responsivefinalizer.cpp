#include <QApplication>
#include <QCoreApplication>
#include <QEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QMainWindow>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QTabWidget>
#include <QTimer>
#include <QWidget>
#include "i18n.h"

namespace {

static QWidget *pageOf(QWidget *tab)
{
    if (!tab) return nullptr;
    if (QScrollArea *s=qobject_cast<QScrollArea*>(tab)) return s->widget();
    return tab;
}

static qreal scaleFor(QMainWindow *w)
{
    if (!w) return 1.0;
    const QSize a=w->centralWidget()?w->centralWidget()->size():w->size();
    const qreal sx=a.width()/1600.0;
    const qreal sy=a.height()/900.0;
    return qBound<qreal>(0.68,qMin(sx,sy),1.30);
}

static void normalizePages(QMainWindow *w)
{
    QTabWidget *tabs=w?w->findChild<QTabWidget*>(QStringLiteral("Tab_main")):nullptr;
    if (!tabs) return;
    tabs->setMinimumSize(0,0);
    tabs->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
    tabs->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    for (int i=0;i<tabs->count();++i) {
        QWidget *tab=tabs->widget(i);
        if (QScrollArea *s=qobject_cast<QScrollArea*>(tab)) {
            s->setWidgetResizable(true);
            s->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            s->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
            s->setMinimumSize(0,0);
            s->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
        }
        QWidget *p=pageOf(tab);
        if (!p) continue;
        p->setMinimumSize(0,0);
        p->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
        p->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
        if (QLayout *l=p->layout()) l->setSizeConstraint(QLayout::SetDefaultConstraint);
    }
}

static void fitOverview(QMainWindow *w)
{
    QWidget *p=w?w->findChild<QWidget*>(QStringLiteral("overview_tab")):nullptr;
    if (!p) return;
    const qreal s=scaleFor(w);
    if (QLayout *l=p->layout()) {
        const int m=qBound(4,qRound(8*s),10);
        const int gap=qBound(3,qRound(6*s),8);
        l->setContentsMargins(m,m,m,m);
        l->setSpacing(gap);
    }
    const QList<QWidget*> all=p->findChildren<QWidget*>(QString(),Qt::FindDirectChildrenOnly);
    for (QWidget *c:all) {
        const QString cls=QString::fromLatin1(c->metaObject()->className());
        if (cls==QStringLiteral("RebuildGaugeCard") || cls==QStringLiteral("SystemStateCard")) {
            c->setMinimumSize(qBound(74,qRound(104*s),128),qBound(104,qRound(150*s),184));
            c->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
            c->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
        }
    }
    if (QFrame *h=p->findChild<QFrame*>(QStringLiteral("overviewHeading")))
        h->setFixedHeight(qBound(32,qRound(42*s),48));
}

static void fitSettings(QMainWindow *w)
{
    QWidget *p=w?w->findChild<QWidget*>(QStringLiteral("emission_tab")):nullptr;
    if (!p) return;
    const qreal s=scaleFor(w);
    if (QLayout *l=p->layout()) {
        const int m=qBound(5,qRound(10*s),13);
        const int gap=qBound(4,qRound(8*s),10);
        l->setContentsMargins(m,m,m,m);
        l->setSpacing(gap);
    }
    for (QWidget *c:p->findChildren<QWidget*>()) {
        const QString cls=QString::fromLatin1(c->metaObject()->className());
        if (cls==QStringLiteral("CompactGauge")) {
            c->setMinimumSize(qBound(70,qRound(94*s),112),qBound(92,qRound(126*s),154));
            c->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
            c->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
            c->show();
        }
    }
}

static void syncNavigation(QMainWindow *w)
{
    QTabWidget *tabs=w?w->findChild<QTabWidget*>(QStringLiteral("Tab_main")):nullptr;
    QListWidget *nav=w?w->findChild<QListWidget*>(QStringLiteral("uiRebuildNav")):nullptr;
    if (!tabs || !nav) return;
    const int current=tabs->currentIndex();
    const QSignalBlocker blocker(nav);
    nav->clear();
    for (int i=0;i<tabs->count();++i) nav->addItem(tabs->tabText(i).trimmed());
    nav->setCurrentRow(qBound(0,current,qMax(0,tabs->count()-1)));
}

static QPushButton *realButtonByText(QMainWindow *w,const QString &text,QWidget *excludeParent)
{
    if (!w || text.isEmpty()) return nullptr;
    for (QPushButton *b:w->findChildren<QPushButton*>()) {
        if (b->text()==text && b->parentWidget()!=excludeParent) return b;
    }
    return nullptr;
}

static void integrateRealFunctions(QMainWindow *w)
{
    if (!w) return;
    QFrame *status=w->findChild<QFrame*>(QStringLiteral("uiRebuildStatus"));
    QHBoxLayout *bar=status?qobject_cast<QHBoxLayout*>(status->layout()):nullptr;
    if (!status || !bar) return;

    QLineEdit *file=w->findChild<QLineEdit*>(QStringLiteral("m_logFileNameBox"));
    QPushButton *start=w->findChild<QPushButton*>(QStringLiteral("m_startLoggingButton"));
    QPushButton *stop=w->findChild<QPushButton*>(QStringLiteral("m_stopLoggingButton"));
    QLabel *fileLabel=w->findChild<QLabel*>(QStringLiteral("m_logFileNameLabel"));
    if (file && start && stop) {
        if (fileLabel) fileLabel->hide();
        file->setParent(status); start->setParent(status); stop->setParent(status);
        file->setPlaceholderText(I18n::text(1014));
        file->setMinimumWidth(80); file->setMaximumWidth(180); file->setMinimumHeight(24); file->setMaximumHeight(30);
        start->setMinimumWidth(62); start->setMaximumWidth(88); start->setMinimumHeight(24); start->setMaximumHeight(30);
        stop->setMinimumWidth(54); stop->setMaximumWidth(82); stop->setMinimumHeight(24); stop->setMaximumHeight(30);
        if (bar->indexOf(file)<0) bar->insertWidget(qMax(0,bar->count()-1),file,1);
        if (bar->indexOf(start)<0) bar->insertWidget(qMax(0,bar->count()-1),start,0);
        if (bar->indexOf(stop)<0) bar->insertWidget(qMax(0,bar->count()-1),stop,0);
        file->show(); start->show(); stop->show();
    }

    QPushButton *snapshot=realButtonByText(w,I18n::text(7015),status);
    QPushButton *view=realButtonByText(w,I18n::text(7016),status);
    for (QPushButton *b:status->findChildren<QPushButton*>(QString(),Qt::FindDirectChildrenOnly)) {
        if (b!=snapshot && b!=view && b->text()==QStringLiteral("Capture écran")) b->hide();
    }
    if (snapshot) {
        snapshot->setParent(status); snapshot->setMinimumHeight(24); snapshot->setMaximumHeight(30);
        if (bar->indexOf(snapshot)<0) bar->addWidget(snapshot,0); snapshot->show();
    }
    if (view) {
        view->setParent(status); view->setMinimumHeight(24); view->setMaximumHeight(30);
        if (bar->indexOf(view)<0) bar->addWidget(view,0); view->show();
    }
}

static void apply(QMainWindow *w)
{
    normalizePages(w);
    fitOverview(w);
    fitSettings(w);
    integrateRealFunctions(w);
    syncNavigation(w);
}

class ResponsiveFinalizer : public QObject
{
public:
    explicit ResponsiveFinalizer(QObject *p=nullptr):QObject(p) {}
protected:
    bool eventFilter(QObject *watched,QEvent *event) override
    {
        QMainWindow *w=qobject_cast<QMainWindow*>(watched);
        if (!w || w->objectName()!=QStringLiteral("MainWindow")) return QObject::eventFilter(watched,event);
        if ((event->type()==QEvent::Show || event->type()==QEvent::Polish) && !w->property("responsiveFinalizerScheduled").toBool()) {
            w->setProperty("responsiveFinalizerScheduled",true);
            QTimer::singleShot(450,w,[w](){apply(w);});
            QTimer::singleShot(1400,w,[w](){apply(w);});
            QTimer::singleShot(3200,w,[w](){apply(w);});
        } else if (event->type()==QEvent::Resize && w->property("responsiveFinalizerScheduled").toBool()) {
            QTimer::singleShot(0,w,[w](){apply(w);});
        }
        return QObject::eventFilter(watched,event);
    }
};

void installResponsiveFinalizer()
{
    QApplication *app=qobject_cast<QApplication*>(QCoreApplication::instance());
    if (app) app->installEventFilter(new ResponsiveFinalizer(app));
}

}

Q_COREAPP_STARTUP_FUNCTION(installResponsiveFinalizer)
