#include <QApplication>
#include <QCoreApplication>
#include <QEvent>
#include <QFrame>
#include <QHeaderView>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QMainWindow>
#include <QPushButton>
#include <QScrollArea>
#include <QSizePolicy>
#include <QTabWidget>
#include <QTableView>
#include <QTableWidget>
#include <QTimer>
#include <QWidget>
#include "i18n.h"

namespace {

static QWidget *realPage(QWidget *tab)
{
    if (!tab) return nullptr;
    if (QScrollArea *s=qobject_cast<QScrollArea*>(tab)) return s->widget();
    return tab;
}

static qreal scaleFor(QMainWindow *w)
{
    if (!w) return 1.0;
    const QSize a=w->centralWidget()?w->centralWidget()->size():w->size();
    return qBound<qreal>(0.65,qMin(a.width()/1600.0,a.height()/900.0),1.30);
}

static QPushButton *buttonByText(QMainWindow *w,const QString &text,QWidget *exclude=nullptr)
{
    if (!w) return nullptr;
    for (QPushButton *b:w->findChildren<QPushButton*>())
        if (b && b->text()==text && b->parentWidget()!=exclude) return b;
    return nullptr;
}

static void preserveRealUtilityControls(QMainWindow *w)
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
        file->setMinimumWidth(78); file->setMaximumWidth(175); file->setMinimumHeight(23); file->setMaximumHeight(29);
        start->setMinimumHeight(23); start->setMaximumHeight(29);
        stop->setMinimumHeight(23); stop->setMaximumHeight(29);
        if (bar->indexOf(file)<0) bar->addWidget(file,1);
        if (bar->indexOf(start)<0) bar->addWidget(start,0);
        if (bar->indexOf(stop)<0) bar->addWidget(stop,0);
        file->show(); start->show(); stop->show();
    }

    QPushButton *snapshot=buttonByText(w,I18n::text(7015),status);
    QPushButton *view=buttonByText(w,I18n::text(7016),status);
    if (snapshot) {
        snapshot->setParent(status); snapshot->setMinimumHeight(23); snapshot->setMaximumHeight(29);
        if (bar->indexOf(snapshot)<0) bar->addWidget(snapshot,0);
        snapshot->show();
    }
    if (view) {
        view->setParent(status); view->setMinimumHeight(23); view->setMaximumHeight(29);
        if (bar->indexOf(view)<0) bar->addWidget(view,0);
        view->show();
    }

    for (QPushButton *b:status->findChildren<QPushButton*>(QString(),Qt::FindDirectChildrenOnly))
        if (b && b!=snapshot && b!=view && b->text()==QStringLiteral("Capture écran")) b->hide();
}

static void removeFakeHeaderTelemetry(QMainWindow *w)
{
    if (!w) return;
    QFrame *header=w->findChild<QFrame*>(QStringLiteral("uiRebuildHeader"));
    if (!header) return;
    for (QLabel *l:header->findChildren<QLabel*>(QString(),Qt::FindDirectChildrenOnly))
        if (l && l->text().contains(QStringLiteral("Fréquence"),Qt::CaseInsensitive) && l->text().contains(QStringLiteral("auto"),Qt::CaseInsensitive))
            l->hide();
}

static void fitAnalysis(QMainWindow *w,qreal s)
{
    if (!w) return;
    QWidget *tab=nullptr;
    for (QWidget *p:w->findChildren<QWidget*>())
        if (p && QString::fromLatin1(p->metaObject()->className())==QStringLiteral("AnalysisTab")) { tab=p; break; }
    if (!tab) return;

    tab->setMinimumSize(0,0); tab->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
    if (QLayout *root=tab->layout()) {
        const int m=qBound(4,qRound(7*s),9),gap=qBound(4,qRound(6*s),8);
        root->setContentsMargins(m,m,m,m); root->setSpacing(gap); root->setSizeConstraint(QLayout::SetDefaultConstraint);
    }
    if (QWidget *right=tab->findChild<QWidget*>(QStringLiteral("analysisRightPanel"))) {
        const int target=qBound(176,qRound(tab->width()*0.18),285);
        right->setMinimumWidth(target); right->setMaximumWidth(target);
    }
    if (QWidget *top=tab->findChild<QWidget*>(QStringLiteral("analysisTopBar")))
        top->setFixedHeight(qBound(30,qRound(36*s),42));

    QList<QWidget*> charts;
    for (QWidget *c:tab->findChildren<QWidget*>()) {
        const QString cls=QString::fromLatin1(c->metaObject()->className());
        if (cls==QStringLiteral("SingleChartWidget") || cls==QStringLiteral("ChartWidget")) charts.append(c);
    }
    if (!charts.isEmpty()) {
        const int available=qMax(240,tab->height()-80);
        const int each=qBound(92,available/qMax(1,charts.size()),210);
        for (QWidget *c:charts) {
            c->setMinimumHeight(each); c->setMaximumHeight(QWIDGETSIZE_MAX);
            c->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
        }
    }
}

static void fitOverviewAndSettings(QMainWindow *w,qreal s)
{
    if (!w) return;
    if (QWidget *overview=w->findChild<QWidget*>(QStringLiteral("overview_tab"))) {
        if (QLayout *l=overview->layout()) {
            const int m=qBound(3,qRound(7*s),9),gap=qBound(3,qRound(5*s),7);
            l->setContentsMargins(m,m,m,m); l->setSpacing(gap);
        }
        for (QWidget *c:overview->findChildren<QWidget*>(QString(),Qt::FindDirectChildrenOnly)) {
            const QString cls=QString::fromLatin1(c->metaObject()->className());
            if (cls==QStringLiteral("RebuildGaugeCard") || cls==QStringLiteral("SystemStateCard")) {
                c->setMinimumSize(70,96); c->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
                c->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
            }
        }
    }

    if (QWidget *settings=w->findChild<QWidget*>(QStringLiteral("emission_tab"))) {
        settings->setMinimumSize(0,0); settings->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
        if (QLayout *l=settings->layout()) {
            const int m=qBound(4,qRound(8*s),11),gap=qBound(4,qRound(7*s),9);
            l->setContentsMargins(m,m,m,m); l->setSpacing(gap);
        }
        for (const char *n:{"settingsMetrics","settingsStates","settingsAdjust"})
            if (QWidget *c=settings->findChild<QWidget*>(QString::fromLatin1(n))) {
                c->setMinimumSize(0,0); c->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
                c->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding); c->show();
            }
        for (QWidget *c:settings->findChildren<QWidget*>())
            if (QString::fromLatin1(c->metaObject()->className())==QStringLiteral("CompactGauge")) {
                c->setMinimumSize(66,88); c->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
                c->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding); c->show();
            }
    }
}

static void fitDedicatedPages(QMainWindow *w,qreal s)
{
    QTabWidget *tabs=w?w->findChild<QTabWidget*>(QStringLiteral("Tab_main")):nullptr;
    if (!tabs) return;
    const int margin=qBound(4,qRound(8*s),11),gap=qBound(4,qRound(7*s),9);
    const int row=qBound(20,qRound(25*s),29);
    for (int i=0;i<tabs->count();++i) {
        QWidget *p=realPage(tabs->widget(i)); if (!p) continue;
        const bool dedicated=p->property("strictSummaryBuilt").toBool() || p->property("strictRawBuilt").toBool() ||
            p->property("strictInteractiveBuilt").toBool() || p->property("strictRoscoBuilt").toBool() || p->property("strictDiagnosticBuilt").toBool();
        if (!dedicated) continue;
        p->setMinimumSize(0,0); p->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
        if (QLayout *l=p->layout()) { l->setContentsMargins(margin,margin,margin,margin); l->setSpacing(gap); }
        for (QFrame *f:p->findChildren<QFrame*>())
            if (f->objectName().startsWith(QStringLiteral("strictHero_"))) f->setMaximumHeight(qBound(38,qRound(46*s),50));
        for (QTableView *t:p->findChildren<QTableView*>()) {
            t->setMinimumSize(0,0); t->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
            if (t->verticalHeader()) t->verticalHeader()->setDefaultSectionSize(row);
            if (t->horizontalHeader()) t->horizontalHeader()->setStretchLastSection(true);
        }
        for (QTableWidget *t:p->findChildren<QTableWidget*>()) {
            t->setMinimumSize(0,0); t->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
            if (t->verticalHeader()) t->verticalHeader()->setDefaultSectionSize(row);
            if (t->horizontalHeader()) t->horizontalHeader()->setStretchLastSection(true);
        }
    }
}

static void fitChrome(QMainWindow *w,qreal s)
{
    if (!w) return;
    const QSize a=w->centralWidget()?w->centralWidget()->size():w->size();
    if (QListWidget *nav=w->findChild<QListWidget*>(QStringLiteral("uiRebuildNav"))) {
        nav->setFixedWidth(qBound(148,qRound(a.width()*0.115),218));
        nav->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    }
    if (QFrame *header=w->findChild<QFrame*>(QStringLiteral("uiRebuildHeader")))
        header->setFixedHeight(qBound(43,qRound(54*s),64));
    if (QFrame *status=w->findChild<QFrame*>(QStringLiteral("uiRebuildStatus")))
        status->setFixedHeight(qBound(30,qRound(36*s),42));
}

static void apply(QMainWindow *w)
{
    if (!w) return;
    const qreal s=scaleFor(w);
    fitChrome(w,s);
    removeFakeHeaderTelemetry(w);
    preserveRealUtilityControls(w);
    fitOverviewAndSettings(w,s);
    fitAnalysis(w,s);
    fitDedicatedPages(w,s);
}

class VisualCompletionInstaller : public QObject
{
public:
    explicit VisualCompletionInstaller(QObject *p=nullptr):QObject(p) {}
protected:
    bool eventFilter(QObject *watched,QEvent *event) override
    {
        QMainWindow *w=qobject_cast<QMainWindow*>(watched);
        if (!w || w->objectName()!=QStringLiteral("MainWindow")) return QObject::eventFilter(watched,event);
        if ((event->type()==QEvent::Show || event->type()==QEvent::Polish) && !w->property("visualCompletionScheduled").toBool()) {
            w->setProperty("visualCompletionScheduled",true);
            QTimer::singleShot(600,w,[w](){apply(w);});
            QTimer::singleShot(1500,w,[w](){apply(w);});
            QTimer::singleShot(3800,w,[w](){apply(w);});
        } else if (event->type()==QEvent::Resize && w->property("visualCompletionScheduled").toBool()) {
            QTimer::singleShot(0,w,[w](){apply(w);});
        }
        return QObject::eventFilter(watched,event);
    }
};

void installVisualCompletion()
{
    QApplication *app=qobject_cast<QApplication*>(QCoreApplication::instance());
    if (app) app->installEventFilter(new VisualCompletionInstaller(app));
}

}

Q_COREAPP_STARTUP_FUNCTION(installVisualCompletion)
