#include <QAbstractItemView>
#include <QApplication>
#include <QCoreApplication>
#include <QEvent>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMainWindow>
#include <QPainter>
#include <QPainterPath>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSlider>
#include <QTabWidget>
#include <QTableView>
#include <QTableWidget>
#include <QTextEdit>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include <QtMath>

namespace {

static const QColor BG("#090e13");
static const QColor PANEL("#10161c");
static const QColor BORDER("#29343e");
static const QColor ORANGE("#ff7a00");
static const QColor ORANGE2("#ff9828");
static const QColor TEXT("#e7ecef");
static const QColor MUTED("#8d99a3");

static QWidget *pageOf(QWidget *tab)
{
    if (!tab) return nullptr;
    if (QScrollArea *s=qobject_cast<QScrollArea*>(tab)) return s->widget();
    return tab;
}

static qreal uiScale(QMainWindow *w)
{
    if (!w) return 1.0;
    const QSize a=w->centralWidget()?w->centralWidget()->size():w->size();
    const qreal sx=a.width()/1600.0;
    const qreal sy=a.height()/900.0;
    return qBound<qreal>(0.72,qMin(sx,sy),1.28);
}

class CompactGauge : public QWidget
{
public:
    CompactGauge(QObject *source,const QString &title,const QString &unit,QWidget *parent=nullptr)
        : QWidget(parent),m_source(source),m_title(title),m_unit(unit)
    {
        setMinimumSize(90,110);
        setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
        setAttribute(Qt::WA_TransparentForMouseEvents,true);
    }
protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this); p.setRenderHint(QPainter::Antialiasing,true);
        const qreal bw=180.0,bh=190.0;
        const qreal s=qMin(width()/bw,height()/bh);
        p.translate((width()-bw*s)/2.0,(height()-bh*s)/2.0); p.scale(s,s);

        p.setPen(QPen(BORDER,1)); p.setBrush(QColor("#0b1116"));
        p.drawRoundedRect(QRectF(.5,.5,bw-1,bh-1),4,4);
        QFont f=p.font(); f.setBold(true); f.setPointSizeF(7.4); p.setFont(f); p.setPen(TEXT);
        p.drawText(QRectF(8,7,164,18),Qt::AlignCenter,m_title);
        p.setPen(QPen(QColor("#222c34"),1)); p.drawLine(QPointF(8,28),QPointF(172,28));

        const QPointF c(90,101); const qreal r=58;
        p.setPen(QPen(QColor("#56616a"),1.2)); p.setBrush(QColor("#05090c")); p.drawEllipse(c,r,r);
        p.setPen(QPen(QColor("#172028"),5)); p.drawEllipse(c,r-5,r-5);
        p.setPen(QPen(QColor("#39454e"),1)); p.drawEllipse(c,r-10,r-10);

        double value=m_source?m_source->property("value").toDouble():0.0;
        double minv=m_source?m_source->property("minimum").toDouble():0.0;
        double maxv=m_source?m_source->property("maximum").toDouble():100.0;
        if (!qIsFinite(minv)||!qIsFinite(maxv)||qFuzzyCompare(minv,maxv)){minv=0;maxv=100;}
        const qreal start=-140.0,sweep=280.0;
        p.save(); p.translate(c);
        for(int i=0;i<=50;i++){
            const bool major=i%10==0;
            const qreal a=qDegreesToRadians(start+sweep*i/50.0);
            const qreal ro=r-8,ri=ro-(major?9:4);
            QColor col=i>=45?QColor("#e94336"):(i>=40?ORANGE:QColor("#a5afb6"));
            p.setPen(QPen(col,major?1.2:.6));
            p.drawLine(QPointF(qCos(a)*ri,qSin(a)*ri),QPointF(qCos(a)*ro,qSin(a)*ro));
        }
        p.restore();

        const double n=qBound(0.0,(value-minv)/(maxv-minv),1.0);
        const qreal a=qDegreesToRadians(start+sweep*n);
        const QPointF d(qCos(a),qSin(a)),normal(-d.y(),d.x());
        QPainterPath needle; needle.moveTo(c+d*(r-15)); needle.lineTo(c+normal*2); needle.lineTo(c-d*7); needle.lineTo(c-normal*2); needle.closeSubpath();
        p.setPen(Qt::NoPen); p.setBrush(ORANGE); p.drawPath(needle);
        p.setBrush(QColor("#444e55")); p.drawEllipse(c,4.5,4.5);

        f=p.font(); f.setBold(true); f.setPointSizeF(13); p.setFont(f); p.setPen(QColor("#f8fafb"));
        const QString val=(qAbs(maxv-minv)<40)?QString::number(value,'f',1):QString::number(value,'f',0);
        p.drawText(QRectF(38,128,104,22),Qt::AlignCenter,val);
        f.setBold(false); f.setPointSizeF(6); p.setFont(f); p.setPen(MUTED);
        p.drawText(QRectF(38,149,104,12),Qt::AlignCenter,m_unit);
        f.setBold(true); f.setPointSizeF(5.7); p.setFont(f); p.setPen(QColor("#68de77"));
        p.drawText(QRectF(8,169,164,12),Qt::AlignCenter,QStringLiteral("NORMAL"));
    }
private:
    QObject *m_source=nullptr;
    QString m_title,m_unit;
};

static void removeInventedRecorder(QMainWindow *w)
{
    if(!w) return;
    QTabWidget *tabs=w->findChild<QTabWidget*>(QStringLiteral("Tab_main"));
    if(!tabs) return;

    QWidget *rec=nullptr; int recIndex=-1;
    for(int i=0;i<tabs->count();++i){
        QWidget *p=pageOf(tabs->widget(i));
        if(p && p->objectName()==QStringLiteral("recorder_tab")){rec=tabs->widget(i);recIndex=i;break;}
    }

    QFrame *status=w->findChild<QFrame*>(QStringLiteral("uiRebuildStatus"));
    QHBoxLayout *bar=status?qobject_cast<QHBoxLayout*>(status->layout()):nullptr;
    QLineEdit *file=w->findChild<QLineEdit*>(QStringLiteral("m_logFileNameBox"));
    QPushButton *start=w->findChild<QPushButton*>(QStringLiteral("m_startLoggingButton"));
    QPushButton *stop=w->findChild<QPushButton*>(QStringLiteral("m_stopLoggingButton"));
    QLabel *fileLabel=w->findChild<QLabel*>(QStringLiteral("m_logFileNameLabel"));

    if(bar && file && start && stop && !status->property("realLoggerIntegrated").toBool()){
        status->setProperty("realLoggerIntegrated",true);
        if(fileLabel) fileLabel->hide();
        file->setParent(status); start->setParent(status); stop->setParent(status);
        file->setPlaceholderText(QStringLiteral("Fichier d'enregistrement"));
        file->setMinimumWidth(90); file->setMaximumWidth(190); file->setMinimumHeight(24); file->setMaximumHeight(30);
        start->setMinimumWidth(64); start->setMaximumWidth(86); start->setMinimumHeight(24); start->setMaximumHeight(30);
        stop->setMinimumWidth(58); stop->setMaximumWidth(78); stop->setMinimumHeight(24); stop->setMaximumHeight(30);
        bar->insertWidget(qMax(0,bar->count()-1),file,1);
        bar->insertWidget(qMax(0,bar->count()-1),start,0);
        bar->insertWidget(qMax(0,bar->count()-1),stop,0);
        file->show(); start->show(); stop->show();
    }

    if(recIndex>=0){
        QWidget *victim=tabs->widget(recIndex);
        tabs->removeTab(recIndex);
        if(victim) victim->deleteLater();
    }
}

static void syncNavigation(QMainWindow *w)
{
    QTabWidget *tabs=w?w->findChild<QTabWidget*>(QStringLiteral("Tab_main")):nullptr;
    QListWidget *nav=w?w->findChild<QListWidget*>(QStringLiteral("uiRebuildNav")):nullptr;
    if(!tabs||!nav) return;
    const QSignalBlocker b(nav);
    nav->clear();
    for(int i=0;i<tabs->count();++i) nav->addItem(tabs->tabText(i).trimmed());
    nav->setCurrentRow(tabs->currentIndex());
}

static void releaseLegacySizing(QMainWindow *w)
{
    if(!w) return;
    QTabWidget *tabs=w->findChild<QTabWidget*>(QStringLiteral("Tab_main"));
    if(!tabs) return;
    tabs->setMinimumSize(0,0); tabs->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX); tabs->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    for(int i=0;i<tabs->count();++i){
        QWidget *tab=tabs->widget(i);
        if(QScrollArea *s=qobject_cast<QScrollArea*>(tab)){
            s->setWidgetResizable(true); s->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff); s->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
            s->setMinimumSize(0,0); s->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
        }
        QWidget *p=pageOf(tab); if(!p) continue;
        p->setMinimumSize(0,0); p->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX); p->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
        if(QLayout *l=p->layout()) l->setSizeConstraint(QLayout::SetDefaultConstraint);
    }
}

static void styleTables(QWidget *root,qreal s)
{
    if(!root) return;
    const int row=qBound(21,qRound(27*s),31),head=qBound(24,qRound(30*s),34);
    const QString qss=QStringLiteral(
        "QTableView,QTableWidget{background:#091016;color:#dfe6eb;alternate-background-color:#0e161d;border:1px solid #2b3741;gridline-color:#202a31;outline:0;}"
        "QTableView::item,QTableWidget::item{padding:3px 6px;border:0;border-bottom:1px solid #182129;}"
        "QTableView::item:selected,QTableWidget::item:selected{background:#382717;color:#fff;}"
        "QHeaderView::section{background:#141c23;color:#eef2f4;border:0;border-right:1px solid #29343e;border-bottom:1px solid #29343e;padding:4px 6px;font-weight:700;}");
    for(QTableView *v:root->findChildren<QTableView*>()){
        v->setStyleSheet(qss); v->setMinimumSize(0,0); v->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX); v->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
        v->setAlternatingRowColors(true); v->setShowGrid(false); if(v->verticalHeader()){v->verticalHeader()->setVisible(false);v->verticalHeader()->setDefaultSectionSize(row);} if(v->horizontalHeader()){v->horizontalHeader()->setMinimumHeight(head);v->horizontalHeader()->setStretchLastSection(true);}
    }
    for(QTableWidget *v:root->findChildren<QTableWidget*>()){
        v->setStyleSheet(qss); v->setMinimumSize(0,0); v->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX); v->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
        v->setAlternatingRowColors(true); v->setShowGrid(false); if(v->verticalHeader()){v->verticalHeader()->setVisible(false);v->verticalHeader()->setDefaultSectionSize(row);} if(v->horizontalHeader()){v->horizontalHeader()->setMinimumHeight(head);v->horizontalHeader()->setStretchLastSection(true);}
    }
}

static void stylePage(QWidget *p,qreal s)
{
    if(!p) return;
    p->setStyleSheet(QStringLiteral("background:#090e13;color:#e7ecef;"));
    if(QLayout *l=p->layout()){
        const int m=qBound(6,qRound(12*s),15),gap=qBound(5,qRound(10*s),12);
        l->setContentsMargins(m,m,m,m); l->setSpacing(gap); l->setSizeConstraint(QLayout::SetDefaultConstraint);
    }
    for(QLabel *lab:p->findChildren<QLabel*>()){
        if(lab->objectName().startsWith(QStringLiteral("uiRebuild"))) continue;
        const QString current=lab->styleSheet();
        if(!current.contains(QStringLiteral("#ff9828")) && !current.contains(QStringLiteral("#ff7a00")))
            lab->setStyleSheet(QStringLiteral("color:#dce3e8;background:transparent;border:0;"));
    }
    for(QGroupBox *g:p->findChildren<QGroupBox*>()){
        g->setMinimumSize(0,0); g->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX); g->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
        g->setStyleSheet(QStringLiteral("QGroupBox{background:#10161c;color:#e7ecef;border:1px solid #2b3741;border-radius:5px;margin-top:14px;font-weight:700;}QGroupBox::title{subcontrol-origin:margin;left:11px;padding:0 6px;color:#ff9828;}"));
    }
    for(QPlainTextEdit *e:p->findChildren<QPlainTextEdit*>()){e->setMinimumSize(0,0);e->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);e->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);e->setStyleSheet(QStringLiteral("background:#080e13;color:#dfe6eb;border:1px solid #2c3842;padding:7px;"));}
    for(QTextEdit *e:p->findChildren<QTextEdit*>()){e->setMinimumSize(0,0);e->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);e->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);e->setStyleSheet(QStringLiteral("background:#080e13;color:#dfe6eb;border:1px solid #2c3842;padding:7px;"));}
    styleTables(p,s);
}

static void rebuildSettingsGauges(QMainWindow *w)
{
    QWidget *page=w?w->findChild<QWidget*>(QStringLiteral("emission_tab")):nullptr;
    QFrame *metrics=w?w->findChild<QFrame*>(QStringLiteral("settingsMetrics")):nullptr;
    if(!page||!metrics||metrics->property("premiumGaugeGrid").toBool()) return;
    metrics->setProperty("premiumGaugeGrid",true);

    const char *names[]={"e_idle_error","m_revCounter_exhaust","e_short_term_fuel_trim","e_ignition_advance","e_lambda"};
    const char *titles[]={"ERREUR RALENTI","RÉGIME MOTEUR","CORRECTION CARBURANT","AVANCE ALLUMAGE","SONDE LAMBDA"};
    const char *units[]={"tr/min","tr/min","%","°","mV"};
    for(const char *n:names) if(QWidget *g=page->findChild<QWidget*>(QString::fromLatin1(n))){g->hide(); if(g->parentWidget())g->parentWidget()->hide();}

    QVBoxLayout *v=qobject_cast<QVBoxLayout*>(metrics->layout()); if(!v) return;
    QGridLayout *grid=new QGridLayout; grid->setContentsMargins(0,0,0,0); grid->setHorizontalSpacing(7); grid->setVerticalSpacing(7);
    for(int c=0;c<3;c++) grid->setColumnStretch(c,1); grid->setRowStretch(0,1);grid->setRowStretch(1,1);
    for(int i=0;i<5;i++){
        QObject *src=page->findChild<QObject*>(QString::fromLatin1(names[i]));
        CompactGauge *g=new CompactGauge(src,QString::fromUtf8(titles[i]),QString::fromUtf8(units[i]),metrics);
        grid->addWidget(g,i<3?0:1,i<3?i:i-3);
    }
    v->addLayout(grid,1);
}

static void fitOverview(QMainWindow *w,qreal s)
{
    QWidget *p=w?w->findChild<QWidget*>(QStringLiteral("overview_tab")):nullptr; if(!p) return;
    if(QFrame *h=p->findChild<QFrame*>(QStringLiteral("overviewHeading"),Qt::FindDirectChildrenOnly)) h->setFixedHeight(qBound(34,qRound(42*s),48));
    if(QLayout *l=p->layout()){
        const int m=qBound(5,qRound(9*s),12),gap=qBound(4,qRound(7*s),9); l->setContentsMargins(m,m,m,m); l->setSpacing(gap);
        for(int i=0;i<l->count();++i) if(QLayout *sub=l->itemAt(i)->layout()){
            sub->setContentsMargins(0,0,0,0); sub->setSpacing(gap); sub->setSizeConstraint(QLayout::SetDefaultConstraint);
        }
    }
}

static void fitChrome(QMainWindow *w,qreal s)
{
    if(!w) return; const QSize a=w->centralWidget()?w->centralWidget()->size():w->size();
    if(QFrame *h=w->findChild<QFrame*>(QStringLiteral("uiRebuildHeader"))) h->setFixedHeight(qBound(44,qRound(a.height()*.064),64));
    if(QFrame *b=w->findChild<QFrame*>(QStringLiteral("uiRebuildStatus"))) b->setFixedHeight(qBound(31,qRound(a.height()*.044),42));
    if(QListWidget *nav=w->findChild<QListWidget*>(QStringLiteral("uiRebuildNav"))){
        const int nw=qBound(148,qRound(a.width()*.12),215),rh=qBound(27,qRound(34*s),40),pad=qBound(8,qRound(13*s),16);
        nav->setFixedWidth(nw);
        nav->setStyleSheet(QStringLiteral("#uiRebuildNav{background:#0c1217;color:#cbd3d8;border:0;border-right:1px solid #29343e;padding:5px 0;}#uiRebuildNav::item{min-height:%1px;padding:2px %2px;border-left:3px solid transparent;font-weight:650;}#uiRebuildNav::item:hover{background:#151d23;color:#fff;}#uiRebuildNav::item:selected{background:#1c211f;color:#ff9828;border-left:3px solid #ff7a00;}").arg(rh).arg(pad));
    }
}

static void applyStrict(QMainWindow *w)
{
    if(!w) return;
    removeInventedRecorder(w);
    releaseLegacySizing(w);
    const qreal s=uiScale(w); w->setProperty("globalUiScale",s);
    fitChrome(w,s);
    QTabWidget *tabs=w->findChild<QTabWidget*>(QStringLiteral("Tab_main"));
    if(tabs){for(int i=0;i<tabs->count();++i)stylePage(pageOf(tabs->widget(i)),s);}
    rebuildSettingsGauges(w);
    fitOverview(w,s);
    syncNavigation(w);
}

class StrictVisualInstaller:public QObject
{
public: explicit StrictVisualInstaller(QObject *p=nullptr):QObject(p){}
protected:
    bool eventFilter(QObject *watched,QEvent *event) override
    {
        QMainWindow *w=qobject_cast<QMainWindow*>(watched);
        if(!w||w->objectName()!=QStringLiteral("MainWindow")) return QObject::eventFilter(watched,event);
        if(event->type()==QEvent::Show&&!w->property("strictVisualScheduled").toBool()){
            w->setProperty("strictVisualScheduled",true);
            QTimer::singleShot(1200,w,[w](){applyStrict(w);});
            QTimer::singleShot(2400,w,[w](){applyStrict(w);});
        } else if(event->type()==QEvent::Resize&&w->property("strictVisualScheduled").toBool()){
            QTimer::singleShot(0,w,[w](){applyStrict(w);});
        }
        return QObject::eventFilter(watched,event);
    }
};

void installStrictVisual(){QApplication *a=qobject_cast<QApplication*>(QCoreApplication::instance());if(a)a->installEventFilter(new StrictVisualInstaller(a));}

}

Q_COREAPP_STARTUP_FUNCTION(installStrictVisual)
