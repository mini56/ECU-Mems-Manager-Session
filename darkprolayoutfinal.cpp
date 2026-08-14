#include <QApplication>
#include <QCoreApplication>
#include <QEvent>
#include <QFontMetrics>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMainWindow>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QStyle>
#include <QTabWidget>
#include <QTableView>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

namespace {

static QString className(QObject *o)
{
    return o ? QString::fromLatin1(o->metaObject()->className()) : QString();
}

static QWidget *realPage(QWidget *tab)
{
    if (!tab) return nullptr;
    if (QScrollArea *s=qobject_cast<QScrollArea*>(tab)) return s->widget();
    return tab;
}

static QIcon lineIcon(const QString &kind)
{
    QPixmap pm(22,22);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing,true);
    p.setPen(QPen(QColor("#ff8a1c"),1.8,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));
    p.setBrush(Qt::NoBrush);
    const QRectF r(3.5,3.5,15,15);
    if (kind==QStringLiteral("overview")) {
        p.drawRoundedRect(QRectF(3,4,7,6),1.5,1.5); p.drawRoundedRect(QRectF(12,4,7,6),1.5,1.5);
        p.drawRoundedRect(QRectF(3,12,7,6),1.5,1.5); p.drawRoundedRect(QRectF(12,12,7,6),1.5,1.5);
    } else if (kind==QStringLiteral("settings")) {
        p.drawLine(4,7,18,7); p.drawLine(4,15,18,15); p.drawEllipse(QPointF(9,7),2.2,2.2); p.drawEllipse(QPointF(14,15),2.2,2.2);
    } else if (kind==QStringLiteral("error")) {
        QPolygonF q; q << QPointF(11,3.5) << QPointF(19,18) << QPointF(3,18); p.drawPolygon(q); p.drawLine(11,8,11,13); p.drawPoint(QPointF(11,16));
    } else if (kind==QStringLiteral("actuator")) {
        p.drawEllipse(r); p.drawEllipse(QPointF(11,11),3,3); p.drawLine(11,3,11,6); p.drawLine(11,16,11,19); p.drawLine(3,11,6,11); p.drawLine(16,11,19,11);
    } else if (kind==QStringLiteral("chart")) {
        p.drawLine(4,18,4,5); p.drawLine(4,18,19,18); QPolygonF q; q<<QPointF(5,14)<<QPointF(9,10)<<QPointF(12,12)<<QPointF(18,6); p.drawPolyline(q);
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
    const QString c=className(page).toLower();
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

static void polishNavigation(QMainWindow *w)
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
        item->setIcon(lineIcon(iconKind(realPage(tabs->widget(i)),text)));
        item->setSizeHint(QSize(0,qMax(36,fm.height()+18)));
        item->setToolTip(text);
        longest=qMax(longest,fm.horizontalAdvance(text));
    }
    nav->setCurrentRow(tabs->currentIndex());
    const int maxWidth=qBound(225,w->width()/5,310);
    nav->setFixedWidth(qBound(178,longest+iconSide+58,maxWidth));
    nav->setSpacing(1);
    nav->setStyleSheet(QStringLiteral(
        "#uiRebuildNav{background:qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #080e13,stop:1 #0d151b);color:#cdd5da;border:0;border-right:1px solid #26323b;padding:8px 0;}"
        "#uiRebuildNav::item{padding:6px 12px;border-left:3px solid transparent;border-bottom:1px solid #111a20;}"
        "#uiRebuildNav::item:hover{background:#141e25;color:#fff;}"
        "#uiRebuildNav::item:selected{background:qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #2b2118,stop:1 #171c1d);color:#ffad5c;border-left:3px solid #ff7a00;font-weight:700;}"));
}

static QGridLayout *gridContaining(QWidget *root,const QList<QWidget*> &widgets)
{
    if(!root||widgets.isEmpty()) return nullptr;
    QGridLayout *best=nullptr; int bestCount=0;
    const QList<QGridLayout*> grids=root->findChildren<QGridLayout*>();
    for(QGridLayout *g:grids){
        int count=0;
        for(QWidget *w:widgets) if(g->indexOf(w)>=0) ++count;
        if(count>bestCount){best=g;bestCount=count;}
    }
    return best;
}

static void reflowOverview(QMainWindow *w)
{
    QWidget *page=w?w->findChild<QWidget*>(QStringLiteral("overview_tab")):nullptr;
    if(!page) return;
    QList<QWidget*> cards;
    for(QWidget *child:page->findChildren<QWidget*>(QString(),Qt::FindDirectChildrenOnly)){
        const QString c=className(child);
        if(c==QStringLiteral("RebuildGaugeCard")||c==QStringLiteral("SystemStateCard")) cards<<child;
    }
    if(cards.size()<6) return;
    QGridLayout *grid=gridContaining(page,cards);
    if(!grid) return;

    const int width=qMax(1,page->width());
    int cols=2;
    if(width>=1580) cols=6;
    else if(width>=1300) cols=5;
    else if(width>=960) cols=4;
    else if(width>=690) cols=3;

    if(page->property("darkProOverviewCols").toInt()!=cols){
        for(QWidget *card:cards) grid->removeWidget(card);
        for(int c=0;c<6;++c) grid->setColumnStretch(c,c<cols?1:0);
        const int rows=(cards.size()+cols-1)/cols;
        for(int r=0;r<6;++r) grid->setRowStretch(r,r<rows?1:0);
        for(int i=0;i<cards.size();++i) grid->addWidget(cards.at(i),i/cols,i%cols);
        page->setProperty("darkProOverviewCols",cols);
    }
    grid->setHorizontalSpacing(qBound(6,page->fontMetrics().height()/2,10));
    grid->setVerticalSpacing(qBound(6,page->fontMetrics().height()/2,10));
    const int rows=(cards.size()+cols-1)/cols;
    const int usable=qMax(330,page->height()-58);
    const int preferred=qBound(118,(usable-qMax(0,rows-1)*8)/qMax(1,rows),238);
    for(QWidget *card:cards){
        card->setMinimumSize(92,qMin(preferred,150));
        card->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
        card->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    }
    if(QFrame *heading=page->findChild<QFrame*>(QStringLiteral("overviewHeading"))){
        heading->setStyleSheet(QStringLiteral("#overviewHeading{background:qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #182129,stop:.65 #11191f,stop:1 #0c1318);border:1px solid #2c3943;border-left:3px solid #ff7a00;border-radius:6px;}#overviewHeading QLabel{background:transparent;border:0;}"));
    }
}

static QList<QWidget*> compactGauges(QFrame *metrics)
{
    QList<QWidget*> out;
    if(!metrics) return out;
    for(QWidget *w:metrics->findChildren<QWidget*>(QString(),Qt::FindDirectChildrenOnly))
        if(className(w)==QStringLiteral("CompactGauge")) out<<w;
    return out;
}

static void reflowSettings(QMainWindow *w)
{
    QWidget *page=w?w->findChild<QWidget*>(QStringLiteral("emission_tab")):nullptr;
    QFrame *metrics=w?w->findChild<QFrame*>(QStringLiteral("settingsMetrics")):nullptr;
    QFrame *states=w?w->findChild<QFrame*>(QStringLiteral("settingsStates")):nullptr;
    QFrame *adjust=w?w->findChild<QFrame*>(QStringLiteral("settingsAdjust")):nullptr;
    if(!page||!metrics||!states||!adjust) return;

    QList<QWidget*> gauges=compactGauges(metrics);
    if(gauges.size()==5){
        QGridLayout *grid=gridContaining(metrics,gauges);
        if(grid){
            const int mw=qMax(1,metrics->width());
            int cols=mw>=980?5:(mw>=620?3:(mw>=420?2:1));
            if(metrics->property("darkProGaugeCols").toInt()!=cols){
                for(QWidget *g:gauges) grid->removeWidget(g);
                for(int c=0;c<5;++c) grid->setColumnStretch(c,c<cols?1:0);
                const int rows=(gauges.size()+cols-1)/cols;
                for(int r=0;r<5;++r) grid->setRowStretch(r,r<rows?1:0);
                for(int i=0;i<gauges.size();++i) grid->addWidget(gauges.at(i),i/cols,i%cols);
                metrics->setProperty("darkProGaugeCols",cols);
            }
            grid->setHorizontalSpacing(8); grid->setVerticalSpacing(8);
        }
    }

    QList<QWidget*> panels; panels<<metrics<<states<<adjust;
    QGridLayout *body=gridContaining(page,panels);
    if(body){
        const bool narrow=page->width()<880;
        if(page->property("darkProSettingsNarrow").toBool()!=narrow || !page->property("darkProSettingsLayoutDone").toBool()){
            for(QWidget *p:panels) body->removeWidget(p);
            if(narrow){
                body->addWidget(metrics,0,0);
                body->addWidget(states,1,0);
                body->addWidget(adjust,2,0);
                body->setColumnStretch(0,1); body->setColumnStretch(1,0);
                body->setRowStretch(0,3); body->setRowStretch(1,1); body->setRowStretch(2,2);
            }else{
                body->addWidget(metrics,0,0,1,2);
                body->addWidget(states,1,0);
                body->addWidget(adjust,1,1);
                body->setColumnStretch(0,2); body->setColumnStretch(1,3);
                body->setRowStretch(0,3); body->setRowStretch(1,2); body->setRowStretch(2,0);
            }
            page->setProperty("darkProSettingsNarrow",narrow);
            page->setProperty("darkProSettingsLayoutDone",true);
        }
        body->setHorizontalSpacing(10); body->setVerticalSpacing(10);
    }

    for(QFrame *f:{metrics,states,adjust}){
        f->setMinimumSize(0,0); f->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX); f->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
        f->setStyleSheet(QStringLiteral("QFrame{background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #131c22,stop:1 #0b1217);border:1px solid #2d3943;border-radius:7px;}QLabel{background:transparent;border:0;}"));
    }
}

static void polishMajorCards(QMainWindow *w)
{
    if(!w) return;
    const QString cardStyle=QStringLiteral("QFrame{background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #131c22,stop:1 #0a1116);border:1px solid #2c3943;border-radius:7px;}QLabel{background:transparent;border:0;}");
    for(const char *name:{"errorsStored","errorsLive","actuatorListCard","actuatorIacCard"})
        if(QFrame *f=w->findChild<QFrame*>(QString::fromLatin1(name))){f->setStyleSheet(cardStyle);f->setMinimumSize(0,0);f->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);f->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);}

    if(QWidget *errors=w->findChild<QWidget*>(QStringLiteral("errors")))
        if(QVBoxLayout *v=qobject_cast<QVBoxLayout*>(errors->layout())){
            v->setContentsMargins(10,9,10,9);v->setSpacing(9);
            if(QFrame *a=w->findChild<QFrame*>(QStringLiteral("errorsStored"))) if(v->indexOf(a)>=0) v->setStretch(v->indexOf(a),1);
            if(QFrame *b=w->findChild<QFrame*>(QStringLiteral("errorsLive"))) if(v->indexOf(b)>=0) v->setStretch(v->indexOf(b),1);
        }
}

static void polishTables(QMainWindow *w)
{
    if(!w) return;
    const QString qss=QStringLiteral(
        "QTableView,QTableWidget{background:#071016;color:#dfe6ea;alternate-background-color:#0d171e;border:1px solid #2b3842;border-radius:6px;gridline-color:#1d282f;outline:0;}"
        "QTableView::item,QTableWidget::item{padding:5px 8px;border:0;border-bottom:1px solid #172129;}"
        "QTableView::item:hover,QTableWidget::item:hover{background:#131e25;}"
        "QTableView::item:selected,QTableWidget::item:selected{background:#352518;color:#fff;}"
        "QHeaderView::section{background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #1b252c,stop:1 #11191f);color:#f0f3f5;border:0;border-right:1px solid #2b3741;border-bottom:2px solid #8a4a18;padding:6px 8px;font-weight:700;}" );
    for(QTableView *t:w->findChildren<QTableView*>()){
        t->setStyleSheet(qss);t->setAlternatingRowColors(true);t->setShowGrid(false);t->setMinimumSize(0,0);t->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
        if(t->verticalHeader()) t->verticalHeader()->setVisible(false);
        if(t->horizontalHeader()){t->horizontalHeader()->setStretchLastSection(true);t->horizontalHeader()->setMinimumHeight(qMax(28,t->fontMetrics().height()+13));}
    }
}

static void polishHeroesAndGroups(QMainWindow *w)
{
    if(!w) return;
    const QString heroStyle=QStringLiteral("QFrame{background:qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #172129,stop:.68 #10181e,stop:1 #0a1116);border:1px solid #2d3943;border-left:3px solid #ff7a00;border-radius:7px;}QLabel{background:transparent;border:0;}");
    for(QFrame *f:w->findChildren<QFrame*>()){
        const QString n=f->objectName();
        if(n.startsWith(QStringLiteral("strictHero_"))||n.startsWith(QStringLiteral("uiRebuildHero_"))) f->setStyleSheet(heroStyle);
    }
    for(QGroupBox *g:w->findChildren<QGroupBox*>()){
        g->setStyleSheet(QStringLiteral("QGroupBox{background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #131c22,stop:1 #0a1116);color:#e7edf0;border:1px solid #2c3943;border-radius:7px;margin-top:15px;font-weight:700;}QGroupBox::title{subcontrol-origin:margin;left:12px;padding:0 6px;color:#ffa149;}"));
    }
}

static void polishAnalysis(QMainWindow *w)
{
    QWidget *analysis=nullptr;
    if(w) for(QWidget *p:w->findChildren<QWidget*>()) if(className(p)==QStringLiteral("AnalysisTab")){analysis=p;break;}
    if(!analysis) return;
    if(QWidget *top=analysis->findChild<QWidget*>(QStringLiteral("analysisTopBar"))){top->setAttribute(Qt::WA_StyledBackground,true);top->setStyleSheet(QStringLiteral("#analysisTopBar{background:qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #172129,stop:1 #0c1419);border:1px solid #2d3943;border-left:3px solid #ff7a00;border-radius:7px;}"));}
    if(QWidget *right=analysis->findChild<QWidget*>(QStringLiteral("analysisRightPanel"))){
        const int target=analysis->width()>=1450?285:(analysis->width()>=1050?245:(analysis->width()>=800?215:180));
        right->setMinimumWidth(target);right->setMaximumWidth(target);right->setAttribute(Qt::WA_StyledBackground,true);
        right->setStyleSheet(QStringLiteral("#analysisRightPanel{background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #131c22,stop:1 #091116);border:1px solid #2d3943;border-radius:7px;}"));
    }
    for(QWidget *c:analysis->findChildren<QWidget*>()){
        const QString cls=className(c);
        if(cls==QStringLiteral("SingleChartWidget")||cls==QStringLiteral("ChartWidget")){
            c->setMinimumWidth(0);c->setMaximumWidth(QWIDGETSIZE_MAX);c->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
        }
    }
}

static void polishHeader(QMainWindow *w)
{
    QFrame *header=w?w->findChild<QFrame*>(QStringLiteral("uiRebuildHeader")):nullptr;
    if(!header) return;
    header->setStyleSheet(QStringLiteral(
        "#uiRebuildHeader{background:qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #080e13,stop:.55 #0b1217,stop:1 #10161b);border:0;border-bottom:1px solid #2b3741;}"
        "#uiRebuildHeader QLabel{background:transparent;border:0;color:#dbe2e7;}"
        "#m_ecuIdLabel{background:#0e171d;border:1px solid #2d3943;border-radius:5px;padding:5px 10px;color:#f3f6f7;font-weight:700;}"
        "#m_communicationsStatusLabel{color:#dce4e8;font-weight:700;padding:0 5px;}"
        "#m_connectButton{background:#ff7a00;color:#101419;border:1px solid #ff9a32;border-radius:5px;padding:6px 14px;font-weight:800;}"
        "#m_connectButton:hover{background:#ff8d1f;}"
        "#m_disconnectButton{background:#151d23;color:#c7d0d6;border:1px solid #35414b;border-radius:5px;padding:6px 14px;font-weight:800;}"
        "#m_disconnectButton:hover{border-color:#c95b4f;color:#fff;}"
        "#m_connectButton:disabled,#m_disconnectButton:disabled{background:#151c21;color:#5f6a72;border-color:#263139;}"));
    if(QHBoxLayout *h=qobject_cast<QHBoxLayout*>(header->layout())){h->setContentsMargins(12,6,10,6);h->setSpacing(8);}
    for(QLabel *l:header->findChildren<QLabel*>(QString(),Qt::FindDirectChildrenOnly)){
        if(l->text().startsWith(QStringLiteral("Port\n"))){l->setMinimumWidth(74);l->setStyleSheet(QStringLiteral("background:#0e171d;border:1px solid #2b3741;border-radius:5px;padding:3px 9px;color:#cbd5db;font-weight:650;"));}
    }
}

static void polishStatus(QMainWindow *w)
{
    QFrame *status=w?w->findChild<QFrame*>(QStringLiteral("uiRebuildStatus")):nullptr;
    if(!status) return;
    status->setStyleSheet(QStringLiteral(
        "#uiRebuildStatus{background:qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #080e13,stop:1 #0c1318);border-top:1px solid #293640;}"
        "#uiRebuildStatus QLabel{background:transparent;color:#cbd4da;border-right:1px solid #25313a;padding:0 8px;}"
        "#uiRebuildStatus QLineEdit{background:#081015;color:#dfe6ea;border:1px solid #30404a;border-radius:5px;padding:3px 7px;}"
        "#uiRebuildStatus QPushButton{background:#111b22;color:#e4ebef;border:1px solid #33506a;border-radius:5px;padding:3px 9px;font-weight:700;}"
        "#uiRebuildStatus QPushButton:hover{border-color:#ff7a00;color:#fff;}"));
}

static void apply(QMainWindow *w)
{
    if(!w) return;
    polishNavigation(w);
    reflowOverview(w);
    reflowSettings(w);
    polishMajorCards(w);
    polishTables(w);
    polishHeroesAndGroups(w);
    polishAnalysis(w);
    polishHeader(w);
    polishStatus(w);
}

class DarkProLayoutFinalInstaller:public QObject
{
public: explicit DarkProLayoutFinalInstaller(QObject *p=nullptr):QObject(p){}
protected:
    bool eventFilter(QObject *watched,QEvent *event) override
    {
        QMainWindow *w=qobject_cast<QMainWindow*>(watched);
        if(!w||w->objectName()!=QStringLiteral("MainWindow")) return QObject::eventFilter(watched,event);
        if((event->type()==QEvent::Show||event->type()==QEvent::Polish)&&!w->property("darkProLayoutFinalScheduled").toBool()){
            w->setProperty("darkProLayoutFinalScheduled",true);
            QTimer::singleShot(1080,w,[w](){apply(w);});
            QTimer::singleShot(1280,w,[w](){apply(w);});
        }else if(event->type()==QEvent::Resize&&w->property("darkProLayoutFinalScheduled").toBool()){
            QTimer::singleShot(80,w,[w](){apply(w);});
        }
        return QObject::eventFilter(watched,event);
    }
};

void installDarkProLayoutFinal(){QApplication *app=qobject_cast<QApplication*>(QCoreApplication::instance());if(app)app->installEventFilter(new DarkProLayoutFinalInstaller(app));}

}

Q_COREAPP_STARTUP_FUNCTION(installDarkProLayoutFinal)
