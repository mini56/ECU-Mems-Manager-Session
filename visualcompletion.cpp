#include <QApplication>
#include <QCoreApplication>
#include <QEvent>
#include <QFrame>
#include <QGridLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QMainWindow>
#include <QMenuBar>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
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
    QWidget *workspace=w->findChild<QWidget*>(QStringLiteral("uiRebuildWorkspace"));
    const QSize a=(workspace && workspace->width()>0 && workspace->height()>0)
        ? workspace->size()
        : (w->centralWidget()?w->centralWidget()->size():w->size());
    return qBound<qreal>(0.64,qMin(a.width()/1450.0,a.height()/720.0),1.30);
}

static QPushButton *buttonByText(QMainWindow *w,const QString &text,QWidget *exclude=nullptr)
{
    if (!w) return nullptr;
    for (QPushButton *b:w->findChildren<QPushButton*>())
        if (b && b->text()==text && b->parentWidget()!=exclude) return b;
    return nullptr;
}

static void styleMenuBar(QMainWindow *w)
{
    if (!w || !w->menuBar()) return;
    w->menuBar()->setStyleSheet(QStringLiteral(
        "QMenuBar{background:#080d12;color:#d9e0e5;border:0;border-bottom:1px solid #29343e;padding:1px 4px;}"
        "QMenuBar::item{background:transparent;padding:4px 9px;}"
        "QMenuBar::item:selected{background:#182027;color:#ff9828;}"
        "QMenu{background:#0d141a;color:#dce3e8;border:1px solid #313d47;padding:4px;}"
        "QMenu::item{padding:5px 26px 5px 10px;}"
        "QMenu::item:selected{background:#2d2117;color:#ff9828;}"
        "QMenu::separator{height:1px;background:#29343e;margin:3px 6px;}"));
}

static void preserveRealUtilityControls(QMainWindow *w,qreal s)
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
        file->setMinimumWidth(qBound(68,qRound(105*s),120));
        file->setMaximumWidth(qBound(105,qRound(150*s),175));
        file->setMinimumHeight(23); file->setMaximumHeight(29);
        start->setMinimumWidth(qBound(54,qRound(68*s),82)); start->setMaximumWidth(qBound(66,qRound(88*s),98));
        stop->setMinimumWidth(qBound(50,qRound(62*s),76)); stop->setMaximumWidth(qBound(60,qRound(80*s),90));
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
        snapshot->setMaximumWidth(qBound(86,qRound(122*s),145));
        if (bar->indexOf(snapshot)<0) bar->addWidget(snapshot,0);
        snapshot->show();
    }
    if (view) {
        view->setParent(status); view->setMinimumHeight(23); view->setMaximumHeight(29);
        view->setMaximumWidth(qBound(86,qRound(122*s),145));
        if (bar->indexOf(view)<0) bar->addWidget(view,0);
        view->show();
    }
}

static void removeInventedRecorder(QMainWindow *w)
{
    QTabWidget *tabs=w?w->findChild<QTabWidget*>(QStringLiteral("Tab_main")):nullptr;
    if (!tabs) return;
    for (int i=tabs->count()-1;i>=0;--i) {
        QWidget *p=realPage(tabs->widget(i));
        if (p && p->objectName()==QStringLiteral("recorder_tab")) {
            QWidget *victim=tabs->widget(i);
            tabs->removeTab(i);
            if (victim) victim->deleteLater();
        }
    }
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
        const int target=qBound(168,qRound(tab->width()*0.18),285);
        right->setMinimumWidth(target); right->setMaximumWidth(target);
        right->setMinimumHeight(0); right->setMaximumHeight(QWIDGETSIZE_MAX);
    }
    if (QWidget *top=tab->findChild<QWidget*>(QStringLiteral("analysisTopBar")))
        top->setFixedHeight(qBound(29,qRound(36*s),42));

    QList<QWidget*> charts;
    for (QWidget *c:tab->findChildren<QWidget*>()) {
        const QString cls=QString::fromLatin1(c->metaObject()->className());
        if (cls==QStringLiteral("SingleChartWidget") || cls==QStringLiteral("ChartWidget")) charts.append(c);
    }
    if (!charts.isEmpty()) {
        const int available=qMax(225,tab->height()-72);
        const int each=qBound(80,(available-qMax(0,charts.size()-1)*5)/qMax(1,charts.size()),215);
        for (QWidget *c:charts) {
            c->setMinimumHeight(each); c->setMaximumHeight(QWIDGETSIZE_MAX);
            c->setMinimumWidth(0); c->setMaximumWidth(QWIDGETSIZE_MAX);
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
                c->setMinimumSize(64,88); c->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
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
                c->setMinimumSize(60,80); c->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
                c->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding); c->show();
            }
    }
}

static void fitRawDataPage(QWidget *page)
{
    if (!page) return;
    QWidget *left=page->findChild<QWidget*>(QStringLiteral("raw_1"));
    QWidget *right=page->findChild<QWidget*>(QStringLiteral("raw_2"));
    if (!left || !right) return;

    page->setMinimumSize(0,0);
    page->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);

    QWidget *viewport=page->parentWidget();
    int visibleHeight=(viewport && viewport->height()>0)?viewport->height():page->height();
    visibleHeight=qMax(360,visibleHeight);

    auto gridFor=[](QWidget *block)->QGridLayout* {
        return block?qobject_cast<QGridLayout*>(block->layout()):nullptr;
    };
    QGridLayout *leftGrid=gridFor(left);
    QGridLayout *rightGrid=gridFor(right);
    const int rows=qMax(leftGrid?leftGrid->rowCount():1,rightGrid?rightGrid->rowCount():1);

    for (QWidget *block:{left,right}) {
        block->setMinimumSize(0,0);
        block->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
        block->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
        QWidget *parent=block->parentWidget();
        QLayout *parentLayout=parent?parent->layout():nullptr;
        if (!parentLayout || parentLayout->indexOf(block)<0) {
            const int top=qMax(0,block->y());
            block->resize(block->width(),qMax(300,visibleHeight-top-6));
        }
    }

    const int usableHeight=qMax(300,visibleHeight-14);
    const int rowHeight=qMax(10,qMin(21,usableHeight/qMax(1,rows)));
    const qreal fontSize=rowHeight<=10?6.7:(rowHeight<=11?6.9:(rowHeight<=12?7.1:(rowHeight<=13?7.4:(rowHeight<=14?7.6:(rowHeight<=15?7.8:(rowHeight<=16?8.0:8.2))))));

    auto fitBlock=[&](QWidget *block,QGridLayout *grid) {
        if (!block) return;
        if (grid) {
            grid->setContentsMargins(0,0,0,0);
            grid->setHorizontalSpacing(8);
            grid->setVerticalSpacing(0);
            grid->setSizeConstraint(QLayout::SetDefaultConstraint);
            for (int r=0;r<grid->rowCount();++r) grid->setRowMinimumHeight(r,rowHeight);
        }
        for (QLabel *label:block->findChildren<QLabel*>()) {
            const QString name=label->objectName();
            const bool header=name.startsWith(QStringLiteral("header_")) || name.startsWith(QStringLiteral("Aheader_"));
            QFont f=label->font();
            f.setPointSizeF(fontSize);
            f.setBold(header);
            label->setFont(f);
            label->setWordWrap(false);
            label->setMinimumHeight(rowHeight);
            label->setMaximumHeight(rowHeight);
            label->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Fixed);
            label->setStyleSheet(header
                ? QStringLiteral("color:#ff9828;background:transparent;border:0;font-weight:700;")
                : QStringLiteral("color:#dce3e8;background:transparent;border:0;"));
        }
    };

    fitBlock(left,leftGrid);
    fitBlock(right,rightGrid);
}

static void fitDedicatedPages(QMainWindow *w,qreal s)
{
    QTabWidget *tabs=w?w->findChild<QTabWidget*>(QStringLiteral("Tab_main")):nullptr;
    if (!tabs) return;
    const int margin=qBound(3,qRound(7*s),10),gap=qBound(3,qRound(6*s),8);
    const int row=qBound(19,qRound(24*s),28);
    for (int i=0;i<tabs->count();++i) {
        QWidget *p=realPage(tabs->widget(i)); if (!p) continue;

        fitRawDataPage(p);

        const bool dedicated=p->property("strictSummaryBuilt").toBool() || p->property("strictRawBuilt").toBool() ||
            p->property("strictInteractiveBuilt").toBool() || p->property("strictRoscoBuilt").toBool() || p->property("strictDiagnosticBuilt").toBool();
        if (!dedicated) continue;
        p->setMinimumSize(0,0); p->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
        if (QLayout *l=p->layout()) { l->setContentsMargins(margin,margin,margin,margin); l->setSpacing(gap); }
        for (QFrame *f:p->findChildren<QFrame*>())
            if (f->objectName().startsWith(QStringLiteral("strictHero_"))) f->setMaximumHeight(qBound(36,qRound(44*s),48));
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
    QWidget *workspace=w->findChild<QWidget*>(QStringLiteral("uiRebuildWorkspace"));
    const QSize a=(workspace && workspace->width()>0)?workspace->size():(w->centralWidget()?w->centralWidget()->size():w->size());
    if (QListWidget *nav=w->findChild<QListWidget*>(QStringLiteral("uiRebuildNav"))) {
        nav->setFixedWidth(qBound(142,qRound(a.width()*0.12),214));
        nav->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    }
    if (QFrame *header=w->findChild<QFrame*>(QStringLiteral("uiRebuildHeader")))
        header->setFixedHeight(qBound(41,qRound(52*s),62));
    if (QFrame *status=w->findChild<QFrame*>(QStringLiteral("uiRebuildStatus")))
        status->setFixedHeight(qBound(29,qRound(35*s),40));
}

static void apply(QMainWindow *w)
{
    if (!w) return;
    const qreal s=scaleFor(w);
    styleMenuBar(w);
    fitChrome(w,s);
    preserveRealUtilityControls(w,s);
    removeInventedRecorder(w);
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
            QTimer::singleShot(120,w,[w](){apply(w);});
            QTimer::singleShot(450,w,[w](){apply(w);});
            QTimer::singleShot(900,w,[w](){apply(w);});
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