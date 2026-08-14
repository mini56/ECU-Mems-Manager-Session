#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QEvent>
#include <QFontMetrics>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QPainter>
#include <QPixmap>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QTabWidget>
#include <QTableView>
#include <QTextEdit>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

namespace {

static QString cls(QObject *o)
{
    return o ? QString::fromLatin1(o->metaObject()->className()) : QString();
}

static QWidget *pageOf(QWidget *tab)
{
    if (!tab) return nullptr;
    if (QScrollArea *s = qobject_cast<QScrollArea*>(tab)) return s->widget();
    return tab;
}

static QIcon navIcon(const QString &kind)
{
    QPixmap pm(22,22);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing,true);
    p.setPen(QPen(QColor("#ff8a1c"),1.7,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));
    p.setBrush(Qt::NoBrush);
    const QRectF r(3.5,3.5,15,15);

    if (kind==QStringLiteral("overview")) {
        p.drawRoundedRect(QRectF(3,4,7,6),1.3,1.3); p.drawRoundedRect(QRectF(12,4,7,6),1.3,1.3);
        p.drawRoundedRect(QRectF(3,12,7,6),1.3,1.3); p.drawRoundedRect(QRectF(12,12,7,6),1.3,1.3);
    } else if (kind==QStringLiteral("settings")) {
        p.drawLine(4,6,18,6); p.drawLine(4,11,18,11); p.drawLine(4,16,18,16);
        p.drawEllipse(QPointF(9,6),2,2); p.drawEllipse(QPointF(14,11),2,2); p.drawEllipse(QPointF(7,16),2,2);
    } else if (kind==QStringLiteral("error")) {
        QPolygonF q; q << QPointF(11,3.5) << QPointF(19,18) << QPointF(3,18); p.drawPolygon(q);
        p.drawLine(11,8,11,13); p.drawPoint(QPointF(11,16));
    } else if (kind==QStringLiteral("actuator")) {
        p.drawEllipse(r); p.drawEllipse(QPointF(11,11),3,3);
        p.drawLine(11,3,11,6); p.drawLine(11,16,11,19); p.drawLine(3,11,6,11); p.drawLine(16,11,19,11);
    } else if (kind==QStringLiteral("chart")) {
        p.drawLine(4,18,4,5); p.drawLine(4,18,19,18);
        QPolygonF q; q << QPointF(5,15) << QPointF(9,10) << QPointF(12,12) << QPointF(18,6); p.drawPolyline(q);
    } else if (kind==QStringLiteral("data")) {
        p.drawRoundedRect(r,2,2); p.drawLine(7,4,7,19); p.drawLine(12,4,12,19); p.drawLine(4,9,19,9); p.drawLine(4,14,19,14);
    } else if (kind==QStringLiteral("diag")) {
        p.drawEllipse(r); p.drawLine(7,11,10,14); p.drawLine(10,14,16,7);
    } else if (kind==QStringLiteral("link")) {
        p.drawRoundedRect(QRectF(3,7,9,8),4,4); p.drawRoundedRect(QRectF(10,7,9,8),4,4); p.drawLine(8,11,14,11);
    } else {
        p.drawRoundedRect(r,2,2); p.drawLine(7,8,15,8); p.drawLine(7,12,15,12); p.drawLine(7,16,13,16);
    }
    return QIcon(pm);
}

static QString iconKind(QWidget *page,const QString &title)
{
    const QString n=page?page->objectName().toLower():QString();
    const QString c=cls(page).toLower();
    const QString t=title.toLower();
    if(n==QStringLiteral("overview_tab")||t.contains(QStringLiteral("aper"))) return QStringLiteral("overview");
    if(n==QStringLiteral("emission_tab")||t.contains(QStringLiteral("régl"))||t.contains(QStringLiteral("regl"))) return QStringLiteral("settings");
    if(n==QStringLiteral("errors")||t.contains(QStringLiteral("erreur"))) return QStringLiteral("error");
    if(n==QStringLiteral("actuators")||t.contains(QStringLiteral("actionneur"))) return QStringLiteral("actuator");
    if(c.contains(QStringLiteral("analysis"))||t.contains(QStringLiteral("analyse"))) return QStringLiteral("chart");
    if(c.contains(QStringLiteral("diagnostic"))||t.contains(QStringLiteral("diagnostic"))) return QStringLiteral("diag");
    if(t.contains(QStringLiteral("rosco"))||n==QStringLiteral("ecu")) return QStringLiteral("link");
    if(t.contains(QStringLiteral("mesure"))||t.contains(QStringLiteral("donnée"))||t.contains(QStringLiteral("donnee"))||n==QStringLiteral("raw")) return QStringLiteral("data");
    return QStringLiteral("page");
}

static void styleNavigation(QMainWindow *w)
{
    QTabWidget *tabs=w?w->findChild<QTabWidget*>(QStringLiteral("Tab_main")):nullptr;
    QListWidget *nav=w?w->findChild<QListWidget*>(QStringLiteral("uiRebuildNav")):nullptr;
    if(!tabs||!nav) return;

    const QSignalBlocker blocker(nav);
    if(nav->count()!=tabs->count()) nav->clear();

    QFont f=nav->font();
    const qreal base=w->font().pointSizeF()>0?w->font().pointSizeF():9.0;
    f.setPointSizeF(qBound<qreal>(8.5,base,10.5));
    nav->setFont(f);
    const QFontMetrics fm(f);
    const int iconSide=qBound(18,fm.height()+2,22);
    nav->setIconSize(QSize(iconSide,iconSide));
    int longest=0;

    for(int i=0;i<tabs->count();++i){
        const QString text=tabs->tabText(i).trimmed();
        QListWidgetItem *item=i<nav->count()?nav->item(i):new QListWidgetItem(nav);
        item->setText(text);
        item->setIcon(navIcon(iconKind(pageOf(tabs->widget(i)),text)));
        item->setSizeHint(QSize(0,qMax(37,fm.height()+19)));
        item->setToolTip(text);
        longest=qMax(longest,fm.horizontalAdvance(text));
    }

    nav->setCurrentRow(tabs->currentIndex());
    const int available=qMax(1,w->centralWidget()?w->centralWidget()->width():w->width());
    const int maxWidth=qBound(220,available/4,305);
    nav->setFixedWidth(qBound(178,longest+iconSide+62,maxWidth));
    nav->setSpacing(1);
    nav->setStyleSheet(QStringLiteral(
        "#uiRebuildNav{background:qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #070c11,stop:1 #0c141a);color:#cbd4da;border:0;border-right:1px solid #2a3540;padding:8px 0;}"
        "#uiRebuildNav::item{padding:6px 12px;border-left:3px solid transparent;border-bottom:1px solid #121a20;}"
        "#uiRebuildNav::item:hover{background:#151f26;color:#fff;}"
        "#uiRebuildNav::item:selected{background:qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #322417,stop:1 #171d20);color:#ffad5c;border-left:3px solid #ff7a00;font-weight:700;}"));
}

static QGridLayout *gridContaining(QWidget *root,const QList<QWidget*> &widgets)
{
    if(!root||widgets.isEmpty()) return nullptr;
    QGridLayout *best=nullptr; int bestCount=0;
    for(QGridLayout *g:root->findChildren<QGridLayout*>()){
        int count=0; for(QWidget *x:widgets) if(x&&g->indexOf(x)>=0) ++count;
        if(count>bestCount){best=g;bestCount=count;}
    }
    return best;
}

static void reflowOverview(QMainWindow *w)
{
    QWidget *page=w?w->findChild<QWidget*>(QStringLiteral("overview_tab")):nullptr;
    if(!page) return;
    QList<QWidget*> cards;
    for(QWidget *x:page->findChildren<QWidget*>(QString(),Qt::FindDirectChildrenOnly)){
        const QString c=cls(x);
        if(c==QStringLiteral("RebuildGaugeCard")||c==QStringLiteral("SystemStateCard")) cards<<x;
    }
    if(cards.size()<6) return;
    QGridLayout *grid=gridContaining(page,cards); if(!grid) return;

    const int gap=8;
    const int usableW=qMax(1,page->width()-16);
    const int usableH=qMax(1,page->height()-52);
    int bestCols=2; qreal bestScore=-1.0;
    for(int cols=2;cols<=6;++cols){
        const int rows=(cards.size()+cols-1)/cols;
        const qreal cw=(usableW-gap*(cols-1))/qreal(cols);
        const qreal ch=(usableH-gap*(rows-1))/qreal(rows);
        const qreal score=qMin(cw/180.0,ch/205.0);
        if(score>bestScore){bestScore=score;bestCols=cols;}
    }

    if(page->property("darkCols").toInt()!=bestCols){
        for(QWidget *x:cards) grid->removeWidget(x);
        for(int c=0;c<6;++c) grid->setColumnStretch(c,c<bestCols?1:0);
        const int rows=(cards.size()+bestCols-1)/bestCols;
        for(int r=0;r<6;++r) grid->setRowStretch(r,r<rows?1:0);
        for(int i=0;i<cards.size();++i) grid->addWidget(cards.at(i),i/bestCols,i%bestCols);
        page->setProperty("darkCols",bestCols);
    }
    grid->setHorizontalSpacing(gap); grid->setVerticalSpacing(gap);
    for(QWidget *x:cards){x->setMinimumSize(0,0);x->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);x->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);}
}

static void reflowSettings(QMainWindow *w)
{
    QWidget *page=w?w->findChild<QWidget*>(QStringLiteral("emission_tab")):nullptr;
    QFrame *metrics=w?w->findChild<QFrame*>(QStringLiteral("settingsMetrics")):nullptr;
    QFrame *states=w?w->findChild<QFrame*>(QStringLiteral("settingsStates")):nullptr;
    QFrame *adjust=w?w->findChild<QFrame*>(QStringLiteral("settingsAdjust")):nullptr;
    if(!page||!metrics||!states||!adjust) return;

    QList<QWidget*> gauges;
    for(QWidget *x:metrics->findChildren<QWidget*>()) if(cls(x)==QStringLiteral("CompactGauge")) gauges<<x;
    if(gauges.size()==5){
        QGridLayout *g=gridContaining(metrics,gauges);
        if(g){
            const int mw=qMax(1,metrics->width());
            const int cols=mw>=900?5:(mw>=560?3:(mw>=360?2:1));
            if(metrics->property("darkGaugeCols").toInt()!=cols){
                for(QWidget *x:gauges) g->removeWidget(x);
                for(int c=0;c<5;++c) g->setColumnStretch(c,c<cols?1:0);
                const int rows=(gauges.size()+cols-1)/cols;
                for(int r=0;r<5;++r) g->setRowStretch(r,r<rows?1:0);
                for(int i=0;i<gauges.size();++i) g->addWidget(gauges.at(i),i/cols,i%cols);
                metrics->setProperty("darkGaugeCols",cols);
            }
            g->setHorizontalSpacing(8);g->setVerticalSpacing(8);
        }
    }

    QList<QWidget*> panels; panels<<metrics<<states<<adjust;
    QGridLayout *body=gridContaining(page,panels); if(!body) return;
    const bool narrow=page->width()<860;
    if(page->property("darkSettingsNarrow").toBool()!=narrow||!page->property("darkSettingsDone").toBool()){
        for(QWidget *x:panels) body->removeWidget(x);
        if(narrow){
            body->addWidget(metrics,0,0); body->addWidget(states,1,0); body->addWidget(adjust,2,0);
            body->setColumnStretch(0,1);body->setColumnStretch(1,0);
            body->setRowStretch(0,3);body->setRowStretch(1,1);body->setRowStretch(2,2);
        }else{
            body->addWidget(metrics,0,0,1,2); body->addWidget(states,1,0); body->addWidget(adjust,1,1);
            body->setColumnStretch(0,2);body->setColumnStretch(1,3);
            body->setRowStretch(0,3);body->setRowStretch(1,2);body->setRowStretch(2,0);
        }
        page->setProperty("darkSettingsNarrow",narrow);page->setProperty("darkSettingsDone",true);
    }
    body->setHorizontalSpacing(10);body->setVerticalSpacing(10);
}

static void stylePages(QMainWindow *w)
{
    QTabWidget *tabs=w?w->findChild<QTabWidget*>(QStringLiteral("Tab_main")):nullptr;
    if(!tabs) return;
    const QString card=QStringLiteral("QFrame{background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #141d24,stop:1 #0a1116);border:1px solid #2d3943;border-radius:7px;}QLabel{background:transparent;border:0;}");
    for(int i=0;i<tabs->count();++i){
        QWidget *page=pageOf(tabs->widget(i)); if(!page) continue;
        page->setMinimumSize(0,0);page->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);page->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
        if(QLayout *l=page->layout()){l->setSizeConstraint(QLayout::SetDefaultConstraint);}
        for(QFrame *f:page->findChildren<QFrame*>()){
            const QString n=f->objectName();
            if(n.startsWith(QStringLiteral("strictHero_"))||n.startsWith(QStringLiteral("uiRebuildHero_"))||n==QStringLiteral("overviewHeading"))
                f->setStyleSheet(QStringLiteral("QFrame{background:qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #19232b,stop:.68 #11191f,stop:1 #0a1116);border:1px solid #303d47;border-left:3px solid #ff7a00;border-radius:7px;}QLabel{background:transparent;border:0;color:#f0f4f6;}"));
            else if(n==QStringLiteral("settingsMetrics")||n==QStringLiteral("settingsStates")||n==QStringLiteral("settingsAdjust")||n==QStringLiteral("errorsStored")||n==QStringLiteral("errorsLive")||n==QStringLiteral("actuatorListCard")||n==QStringLiteral("actuatorIacCard"))
                f->setStyleSheet(card);
        }
        for(QGroupBox *g:page->findChildren<QGroupBox*>()) g->setStyleSheet(QStringLiteral("QGroupBox{background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #141d24,stop:1 #0a1116);color:#e7edf0;border:1px solid #2d3943;border-radius:7px;margin-top:15px;font-weight:700;}QGroupBox::title{subcontrol-origin:margin;left:12px;padding:0 6px;color:#ffa149;}"));
    }
}

static void styleTables(QMainWindow *w)
{
    const QString qss=QStringLiteral(
        "QTableView,QTableWidget{background:#071016;color:#e1e8ec;alternate-background-color:#0d171e;border:1px solid #2b3842;border-radius:6px;gridline-color:#1d282f;outline:0;}"
        "QTableView::item,QTableWidget::item{padding:5px 8px;border:0;border-bottom:1px solid #172129;}"
        "QTableView::item:hover,QTableWidget::item:hover{background:#131e25;}"
        "QTableView::item:selected,QTableWidget::item:selected{background:#352518;color:#fff;}"
        "QHeaderView::section{background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #1b252c,stop:1 #11191f);color:#f0f3f5;border:0;border-right:1px solid #2b3741;border-bottom:2px solid #8a4a18;padding:6px 8px;font-weight:700;}");
    for(QTableView *t:w->findChildren<QTableView*>()){
        t->setStyleSheet(qss);t->setAlternatingRowColors(true);t->setShowGrid(false);t->setMinimumSize(0,0);t->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
        if(t->verticalHeader()) t->verticalHeader()->setVisible(false);
        if(t->horizontalHeader()){t->horizontalHeader()->setStretchLastSection(true);t->horizontalHeader()->setMinimumHeight(qMax(28,t->fontMetrics().height()+13));}
    }
}

static void styleControls(QMainWindow *w)
{
    const QString button=QStringLiteral("QPushButton{background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #1b252c,stop:1 #111920);color:#eaf0f3;border:1px solid #35434e;border-radius:5px;padding:5px 10px;font-weight:650;}QPushButton:hover{border-color:#ff7a00;background:#202a31;color:#fff;}QPushButton:pressed{background:#0b1116;}QPushButton:checked{background:#3b2918;border-color:#ff8a1c;color:#ffad5c;}QPushButton:disabled{background:#141b20;color:#63707a;border-color:#27313a;}");
    for(QPushButton *b:w->findChildren<QPushButton*>()){
        if(b->objectName()==QStringLiteral("m_connectButton")||b->objectName()==QStringLiteral("m_disconnectButton")) continue;
        b->setStyleSheet(button);b->setMinimumHeight(qMax(27,b->fontMetrics().height()+12));
    }
    const QString edit=QStringLiteral("background:#071016;color:#e2e9ed;border:1px solid #34434e;border-radius:5px;padding:4px 7px;selection-background-color:#754016;");
    for(QLineEdit *e:w->findChildren<QLineEdit*>()) e->setStyleSheet(edit);
    for(QComboBox *c:w->findChildren<QComboBox*>()) c->setStyleSheet(QStringLiteral("QComboBox{%1;padding-right:20px;}QComboBox QAbstractItemView{background:#0b1217;color:#e4ebef;border:1px solid #34434e;selection-background-color:#3a2918;}").arg(edit));
    for(QPlainTextEdit *e:w->findChildren<QPlainTextEdit*>()) e->setStyleSheet(QStringLiteral("background:#071016;color:#dfe7eb;border:1px solid #2d3943;border-radius:6px;padding:7px;selection-background-color:#754016;"));
    for(QTextEdit *e:w->findChildren<QTextEdit*>()) e->setStyleSheet(QStringLiteral("background:#071016;color:#dfe7eb;border:1px solid #2d3943;border-radius:6px;padding:7px;selection-background-color:#754016;"));
    for(QCheckBox *c:w->findChildren<QCheckBox*>()) c->setStyleSheet(QStringLiteral("QCheckBox{color:#dce4e8;spacing:6px;background:transparent;}QCheckBox::indicator{width:13px;height:13px;border:1px solid #53616c;background:#081015;}QCheckBox::indicator:checked{background:#ff7a00;border-color:#ff9a32;}"));
}

static void styleAnalysis(QMainWindow *w)
{
    QWidget *analysis=nullptr;
    for(QWidget *x:w->findChildren<QWidget*>()) if(cls(x)==QStringLiteral("AnalysisTab")){analysis=x;break;}
    if(!analysis) return;
    if(QWidget *top=analysis->findChild<QWidget*>(QStringLiteral("analysisTopBar"))) top->setStyleSheet(QStringLiteral("#analysisTopBar{background:qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #182129,stop:1 #0c1419);border:1px solid #2d3943;border-left:3px solid #ff7a00;border-radius:7px;}"));
    if(QWidget *right=analysis->findChild<QWidget*>(QStringLiteral("analysisRightPanel"))){
        const int target=qBound(180,qRound(analysis->width()*0.19),285);
        right->setMinimumWidth(target);right->setMaximumWidth(target);
        right->setStyleSheet(QStringLiteral("#analysisRightPanel{background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #141d24,stop:1 #091116);border:1px solid #2d3943;border-radius:7px;}"));
    }
}

static void styleChrome(QMainWindow *w)
{
    if(QMenuBar *bar=w->menuBar()) bar->setStyleSheet(QStringLiteral("QMenuBar{background:#080d12;color:#cfd7dc;border-bottom:1px solid #27323b;padding:2px;}QMenuBar::item{background:transparent;padding:4px 8px;}QMenuBar::item:selected{background:#182129;color:#fff;}"));
    for(QMenu *m:w->findChildren<QMenu*>()) m->setStyleSheet(QStringLiteral("QMenu{background:#0d141a;color:#dce4e8;border:1px solid #33404a;padding:4px;}QMenu::item{padding:6px 22px 6px 10px;}QMenu::item:selected{background:#302217;color:#ffad5c;}QMenu::separator{height:1px;background:#2b3741;margin:4px 7px;}"));

    QFrame *header=w->findChild<QFrame*>(QStringLiteral("uiRebuildHeader"));
    if(header){
        header->setStyleSheet(QStringLiteral("#uiRebuildHeader{background:qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #070c11,stop:.55 #0b1217,stop:1 #11171c);border:0;border-bottom:1px solid #2c3842;}#uiRebuildHeader QLabel{background:transparent;border:0;color:#dbe3e7;}#m_ecuIdLabel{background:#0e171d;border:1px solid #30404b;border-radius:5px;padding:5px 10px;color:#f3f6f7;font-weight:700;}#m_communicationsStatusLabel{color:#dce4e8;font-weight:700;padding:0 5px;}#m_connectButton{background:#ff7a00;color:#101419;border:1px solid #ff9a32;border-radius:5px;padding:6px 14px;font-weight:800;}#m_connectButton:hover{background:#ff8d1f;}#m_disconnectButton{background:#151d23;color:#c7d0d6;border:1px solid #35414b;border-radius:5px;padding:6px 14px;font-weight:800;}#m_disconnectButton:hover{border-color:#c95b4f;color:#fff;}#m_connectButton:disabled,#m_disconnectButton:disabled{background:#151c21;color:#5f6a72;border-color:#263139;}"));
        if(QHBoxLayout *h=qobject_cast<QHBoxLayout*>(header->layout())){h->setContentsMargins(12,6,10,6);h->setSpacing(8);}
    }

    QFrame *status=w->findChild<QFrame*>(QStringLiteral("uiRebuildStatus"));
    if(status) status->setStyleSheet(QStringLiteral("#uiRebuildStatus{background:qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #070c11,stop:1 #0c1318);border-top:1px solid #293640;}#uiRebuildStatus QLabel{background:transparent;color:#cbd4da;border-right:1px solid #25313a;padding:0 8px;}#uiRebuildStatus QLineEdit{background:#081015;color:#dfe6ea;border:1px solid #30404a;border-radius:5px;padding:3px 7px;}#uiRebuildStatus QPushButton{background:#111b22;color:#e4ebef;border:1px solid #33506a;border-radius:5px;padding:3px 9px;font-weight:700;}#uiRebuildStatus QPushButton:hover{border-color:#ff7a00;color:#fff;}"));
}

static void apply(QMainWindow *w)
{
    if(!w) return;
    styleNavigation(w);
    reflowOverview(w);
    reflowSettings(w);
    stylePages(w);
    styleTables(w);
    styleControls(w);
    styleAnalysis(w);
    styleChrome(w);
}

class DarkStyleInstaller : public QObject
{
public:
    explicit DarkStyleInstaller(QObject *parent=nullptr):QObject(parent){}
protected:
    bool eventFilter(QObject *watched,QEvent *event) override
    {
        QMainWindow *w=qobject_cast<QMainWindow*>(watched);
        if(!w||w->objectName()!=QStringLiteral("MainWindow")) return QObject::eventFilter(watched,event);
        if((event->type()==QEvent::Show||event->type()==QEvent::Polish)&&!w->property("darkStyleScheduled").toBool()){
            w->setProperty("darkStyleScheduled",true);
            QTimer::singleShot(1050,w,[w](){apply(w);});
            QTimer::singleShot(1320,w,[w](){apply(w);});
        }else if(event->type()==QEvent::Resize&&w->property("darkStyleScheduled").toBool()){
            QTimer::singleShot(70,w,[w](){apply(w);});
        }
        return QObject::eventFilter(watched,event);
    }
};

void installDarkStyle()
{
    QApplication *app=qobject_cast<QApplication*>(QCoreApplication::instance());
    if(app) app->installEventFilter(new DarkStyleInstaller(app));
}

}

Q_COREAPP_STARTUP_FUNCTION(installDarkStyle)
