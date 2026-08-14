#include <QApplication>
#include <QCoreApplication>
#include <QEvent>
#include <QFontMetrics>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMainWindow>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QTabWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include <QtMath>

namespace {

static QWidget *pageOf(QWidget *tab)
{
    if (!tab) return nullptr;
    if (QScrollArea *scroll = qobject_cast<QScrollArea*>(tab)) return scroll->widget();
    return tab;
}

static int pageKind(QWidget *page, const QString &title)
{
    const QString name = page ? page->objectName().toLower() : QString();
    const QString cls = page ? QString::fromLatin1(page->metaObject()->className()).toLower() : QString();
    const QString t = title.toLower();
    if (name == QStringLiteral("overview_tab") || t.contains(QStringLiteral("aper"))) return 0;
    if (name == QStringLiteral("emission_tab") || t.contains(QStringLiteral("régl")) || t.contains(QStringLiteral("regl"))) return 1;
    if (name == QStringLiteral("errors") || t.contains(QStringLiteral("erreur"))) return 2;
    if (name == QStringLiteral("actuators") || t.contains(QStringLiteral("actionneur"))) return 3;
    if (t.contains(QStringLiteral("mesure"))) return 4;
    if (name == QStringLiteral("raw") || t.contains(QStringLiteral("donnée")) || t.contains(QStringLiteral("donnee"))) return 5;
    if (name == QStringLiteral("ecu") || t.contains(QStringLiteral("interactif"))) return 6;
    if (cls.contains(QStringLiteral("analysis")) || t.contains(QStringLiteral("analyse"))) return 7;
    if (t.contains(QStringLiteral("rosco"))) return 8;
    if (cls.contains(QStringLiteral("diagnostic")) || t.contains(QStringLiteral("diagnostic"))) return 9;
    return 10;
}

static QIcon makeNavIcon(int kind)
{
    QPixmap px(22,22);
    px.fill(Qt::transparent);
    QPainter p(&px);
    p.setRenderHint(QPainter::Antialiasing,true);
    QPen pen(QColor("#ff8a1c"),1.7,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);

    switch(kind) {
    case 0:
        p.drawRoundedRect(QRectF(3,3,6,6),1,1); p.drawRoundedRect(QRectF(13,3,6,6),1,1);
        p.drawRoundedRect(QRectF(3,13,6,6),1,1); p.drawRoundedRect(QRectF(13,13,6,6),1,1); break;
    case 1:
        p.drawLine(4,5,18,5); p.drawEllipse(QPointF(9,5),2,2);
        p.drawLine(4,11,18,11); p.drawEllipse(QPointF(15,11),2,2);
        p.drawLine(4,17,18,17); p.drawEllipse(QPointF(7,17),2,2); break;
    case 2:
        p.drawPolygon(QPolygonF()<<QPointF(11,3)<<QPointF(19,18)<<QPointF(3,18));
        p.drawLine(11,8,11,13); p.drawPoint(QPointF(11,16)); break;
    case 3:
        p.drawEllipse(QPointF(11,11),5.5,5.5); p.drawEllipse(QPointF(11,11),2,2);
        for(int i=0;i<8;i++){ const qreal a=i*M_PI/4.0; p.drawLine(QPointF(11+qCos(a)*7,11+qSin(a)*7),QPointF(11+qCos(a)*9,11+qSin(a)*9)); } break;
    case 4:
        p.drawArc(QRectF(3,5,16,14),20*16,140*16); p.drawLine(11,12,16,8); p.drawEllipse(QPointF(11,12),1.5,1.5); break;
    case 5:
        p.drawLine(4,5,18,5); p.drawLine(4,11,18,11); p.drawLine(4,17,18,17);
        p.drawEllipse(QPointF(6,5),1,1); p.drawEllipse(QPointF(12,11),1,1); p.drawEllipse(QPointF(16,17),1,1); break;
    case 6:
        p.drawRoundedRect(QRectF(3,4,16,14),2,2); p.drawLine(6,8,9,11); p.drawLine(9,11,6,14); p.drawLine(11,14,16,14); break;
    case 7:
        p.drawLine(3,18,3,4); p.drawLine(3,18,19,18); p.drawPolyline(QPolygonF()<<QPointF(5,15)<<QPointF(9,10)<<QPointF(12,12)<<QPointF(17,5)); break;
    case 8:
        p.drawEllipse(QRectF(3,7,7,7)); p.drawEllipse(QRectF(12,7,7,7)); p.drawLine(8,10,14,10); p.drawLine(8,12,14,12); break;
    case 9:
        p.drawRoundedRect(QRectF(4,4,14,14),3,3); p.drawLine(7,11,10,14); p.drawLine(10,14,16,7); break;
    default:
        p.drawRoundedRect(QRectF(4,4,14,14),2,2); p.drawLine(7,8,15,8); p.drawLine(7,12,15,12); p.drawLine(7,16,12,16); break;
    }
    return QIcon(px);
}

static QString tabTitleForPage(QMainWindow *window, QWidget *page)
{
    QTabWidget *tabs = window ? window->findChild<QTabWidget*>(QStringLiteral("Tab_main")) : nullptr;
    if (!tabs || !page) return QString();
    for (int i=0;i<tabs->count();++i) if (pageOf(tabs->widget(i)) == page) return tabs->tabText(i).trimmed();
    return QString();
}

static QFrame *ensurePageHeading(QMainWindow *window, QWidget *page)
{
    if (!window || !page) return nullptr;
    QVBoxLayout *root = qobject_cast<QVBoxLayout*>(page->layout());
    if (!root) return nullptr;
    const QString name = QStringLiteral("runtimePageHeading_%1").arg(page->objectName());
    if (QFrame *existing = page->findChild<QFrame*>(name,Qt::FindDirectChildrenOnly)) return existing;

    const QString title = tabTitleForPage(window,page);
    if (title.isEmpty()) return nullptr;
    QFrame *heading = new QFrame(page);
    heading->setObjectName(name);
    heading->setAttribute(Qt::WA_StyledBackground,true);
    heading->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
    heading->setStyleSheet(QStringLiteral(
        "#%1{background:qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #151d23,stop:0.72 #0f161b,stop:1 #0c1217);"
        "border:1px solid #2c3842;border-left:3px solid #ff7a00;border-radius:5px;}"
        "#%1 QLabel{background:transparent;border:0;color:#f0f4f6;}").arg(name));
    QHBoxLayout *h = new QHBoxLayout(heading);
    h->setContentsMargins(12,5,12,5);
    QLabel *label = new QLabel(title.toUpper(),heading);
    QFont f=label->font(); f.setBold(true); f.setPointSizeF(qMax<qreal>(9.0,window->font().pointSizeF()+0.5)); label->setFont(f);
    h->addWidget(label); h->addStretch(1);
    heading->setFixedHeight(qMax(34,label->fontMetrics().height()+14));
    root->insertWidget(0,heading);
    return heading;
}

static void polishNavigation(QMainWindow *window)
{
    QTabWidget *tabs = window ? window->findChild<QTabWidget*>(QStringLiteral("Tab_main")) : nullptr;
    QListWidget *nav = window ? window->findChild<QListWidget*>(QStringLiteral("uiRebuildNav")) : nullptr;
    if (!tabs || !nav) return;

    const QSignalBlocker blocker(nav);
    if (nav->count() != tabs->count()) nav->clear();

    QFont f = nav->font();
    f.setPointSizeF(qMax<qreal>(8.5, window->font().pointSizeF()));
    nav->setFont(f);
    const QFontMetrics fm(f);
    int longest = 0;

    for (int i = 0; i < tabs->count(); ++i) {
        const QString text = tabs->tabText(i).trimmed();
        QListWidgetItem *item = (i < nav->count()) ? nav->item(i) : nullptr;
        if (!item) item = new QListWidgetItem(nav);
        item->setText(text);
        item->setIcon(makeNavIcon(pageKind(pageOf(tabs->widget(i)),text)));
        item->setSizeHint(QSize(0, qMax(35, fm.height() + 18)));
        longest = qMax(longest, fm.horizontalAdvance(text));
    }
    nav->setCurrentRow(tabs->currentIndex());
    nav->setIconSize(QSize(qMax(17, fm.height()+1), qMax(17, fm.height()+1)));
    nav->setFixedWidth(qBound(178, longest + nav->iconSize().width() + 60, 270));
    nav->setSpacing(1);
    nav->setStyleSheet(QStringLiteral(
        "#uiRebuildNav{background:#090f14;color:#cbd4da;border:0;border-right:1px solid #26313a;padding:7px 0;}"
        "#uiRebuildNav::item{padding:5px 12px;border-left:3px solid transparent;border-bottom:1px solid #121a20;}"
        "#uiRebuildNav::item:hover{background:#151d24;color:#ffffff;}"
        "#uiRebuildNav::item:selected{background:qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #282117,stop:1 #171b1d);color:#ffa03c;border-left:3px solid #ff7a00;font-weight:700;}"));
}

static QGridLayout *gridContaining(const QList<QGridLayout*> &grids,const QList<QWidget*> &widgets)
{
    for (QGridLayout *grid : grids) {
        if (!grid) continue;
        int found=0;
        for (QWidget *w:widgets) if (w && grid->indexOf(w)>=0) ++found;
        if (found>=qMin(2,widgets.size())) return grid;
    }
    return nullptr;
}

static void polishOverview(QMainWindow *window)
{
    QWidget *page = window ? window->findChild<QWidget*>(QStringLiteral("overview_tab")) : nullptr;
    if (!page || !page->layout()) return;

    QList<QWidget*> cards;
    for (QWidget *w:page->findChildren<QWidget*>(QString(),Qt::FindDirectChildrenOnly)) {
        const QString cls=QString::fromLatin1(w->metaObject()->className());
        if (cls==QStringLiteral("RebuildGaugeCard") || cls==QStringLiteral("SystemStateCard")) cards<<w;
    }
    if (cards.size()<6) return;

    QGridLayout *grid=gridContaining(page->findChildren<QGridLayout*>(),cards);
    if (!grid) return;
    for (QWidget *card:cards) grid->removeWidget(card);

    const int gap=7;
    const int usableW=qMax(1,page->width()-16);
    int headingH=0;
    if (QFrame *heading=page->findChild<QFrame*>(QStringLiteral("overviewHeading"),Qt::FindDirectChildrenOnly)) headingH=heading->height()+8;
    const int usableH=qMax(1,page->height()-headingH-12);
    int bestCols=6;
    qreal bestScore=-1.0;
    for (int cols=2; cols<=6; ++cols) {
        const int rows=(cards.size()+cols-1)/cols;
        const qreal cw=(usableW-gap*(cols-1))/qreal(cols);
        const qreal ch=(usableH-gap*(rows-1))/qreal(rows);
        const qreal score=qMin(cw/188.0,ch/226.0);
        if (score>bestScore) { bestScore=score; bestCols=cols; }
    }

    for(int c=0;c<6;c++) grid->setColumnStretch(c,c<bestCols?1:0);
    for(int r=0;r<6;r++) grid->setRowStretch(r,r<((cards.size()+bestCols-1)/bestCols)?1:0);
    grid->setHorizontalSpacing(gap); grid->setVerticalSpacing(gap);
    for(int i=0;i<cards.size();++i) {
        QWidget *card=cards.at(i);
        card->setMinimumSize(0,0); card->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
        card->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
        grid->addWidget(card,i/bestCols,i%bestCols);
    }
}

static void polishSettings(QMainWindow *window)
{
    QWidget *page = window ? window->findChild<QWidget*>(QStringLiteral("emission_tab")) : nullptr;
    QFrame *metrics = window ? window->findChild<QFrame*>(QStringLiteral("settingsMetrics")) : nullptr;
    QFrame *states = window ? window->findChild<QFrame*>(QStringLiteral("settingsStates")) : nullptr;
    QFrame *adjust = window ? window->findChild<QFrame*>(QStringLiteral("settingsAdjust")) : nullptr;
    if (!page || !metrics || !states || !adjust) return;
    ensurePageHeading(window,page);

    QVBoxLayout *root = qobject_cast<QVBoxLayout*>(page->layout());
    QGridLayout *body = nullptr;
    if (root) {
        root->setContentsMargins(10,8,10,9); root->setSpacing(8);
        for (int i = 0; i < root->count(); ++i) {
            QGridLayout *candidate = qobject_cast<QGridLayout*>(root->itemAt(i)->layout());
            if (candidate && candidate->indexOf(metrics) >= 0) { body = candidate; break; }
        }
    }

    if (body) {
        body->removeWidget(metrics); body->removeWidget(states); body->removeWidget(adjust);
        body->addWidget(metrics,0,0,1,2);
        body->addWidget(states,1,0);
        body->addWidget(adjust,1,1);
        body->setColumnStretch(0,2); body->setColumnStretch(1,5);
        body->setRowStretch(0,3); body->setRowStretch(1,2);
        body->setHorizontalSpacing(9); body->setVerticalSpacing(9);
    }

    const QString panelQss=QStringLiteral("background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #121a20,stop:1 #0b1217);border:1px solid #2b3741;border-radius:6px;");
    metrics->setStyleSheet(QStringLiteral("#settingsMetrics{%1}").arg(panelQss));
    states->setStyleSheet(QStringLiteral("#settingsStates{%1}").arg(panelQss));
    adjust->setStyleSheet(QStringLiteral("#settingsAdjust{%1}").arg(panelQss));
    metrics->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    states->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    adjust->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    states->setMinimumHeight(0); adjust->setMinimumHeight(0);

    QList<QWidget*> gauges;
    for (QWidget *w : metrics->findChildren<QWidget*>(QString(),Qt::FindDirectChildrenOnly))
        if (w && QString::fromLatin1(w->metaObject()->className())==QStringLiteral("CompactGauge")) gauges<<w;
    if (gauges.size()!=5) return;
    QGridLayout *grid=gridContaining(metrics->findChildren<QGridLayout*>(),gauges);
    if (!grid) return;
    for(QWidget *g:gauges) grid->removeWidget(g);

    const int ideal=qMax(138,QFontMetrics(window->font()).horizontalAdvance(QStringLiteral("CORRECTION CARBURANT"))+30);
    const int cols=qBound(2,qMax(1,metrics->width()/ideal),5);
    const int rows=(gauges.size()+cols-1)/cols;
    for(int c=0;c<5;c++) grid->setColumnStretch(c,c<cols?1:0);
    for(int r=0;r<3;r++) grid->setRowStretch(r,r<rows?1:0);
    grid->setHorizontalSpacing(7); grid->setVerticalSpacing(7);
    for(int i=0;i<gauges.size();++i) {
        QWidget *g=gauges.at(i); g->setMinimumSize(70,88); g->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
        g->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding); grid->addWidget(g,i/cols,i%cols);
    }
}

static void polishErrors(QMainWindow *window)
{
    QWidget *page = window ? window->findChild<QWidget*>(QStringLiteral("errors")) : nullptr;
    QFrame *stored = window ? window->findChild<QFrame*>(QStringLiteral("errorsStored")) : nullptr;
    QFrame *live = window ? window->findChild<QFrame*>(QStringLiteral("errorsLive")) : nullptr;
    if (!page || !stored || !live) return;
    ensurePageHeading(window,page);

    stored->setStyleSheet(QStringLiteral("#errorsStored{background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #131b21,stop:1 #0b1217);border:1px solid #2c3842;border-radius:6px;}"));
    live->setStyleSheet(QStringLiteral("#errorsLive{background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #121a20,stop:1 #091016);border:1px solid #2c3842;border-radius:6px;}"));
    if (QVBoxLayout *root=qobject_cast<QVBoxLayout*>(page->layout())) {
        root->setContentsMargins(10,8,10,9); root->setSpacing(8);
        const int a=root->indexOf(stored),b=root->indexOf(live);
        if(a>=0)root->setStretch(a,1); if(b>=0)root->setStretch(b,1);
    }
    for(QLabel *l:page->findChildren<QLabel*>()) {
        if(l->parentWidget()==page) continue;
        QFont f=l->font(); if(f.pointSizeF()<8.0) f.setPointSizeF(8.0); l->setFont(f);
    }
}

static void polishActuators(QMainWindow *window)
{
    QWidget *page=window?window->findChild<QWidget*>(QStringLiteral("actuators")):nullptr;
    QFrame *left=window?window->findChild<QFrame*>(QStringLiteral("actuatorListCard")):nullptr;
    QFrame *right=window?window->findChild<QFrame*>(QStringLiteral("actuatorIacCard")):nullptr;
    if(!page||!left||!right) return;
    ensurePageHeading(window,page);
    const QString panel=QStringLiteral("background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #131b21,stop:1 #0b1217);border:1px solid #2c3842;border-radius:6px;");
    left->setStyleSheet(QStringLiteral("#actuatorListCard{%1}").arg(panel));
    right->setStyleSheet(QStringLiteral("#actuatorIacCard{%1}").arg(panel));
    if(QVBoxLayout *root=qobject_cast<QVBoxLayout*>(page->layout())){root->setContentsMargins(10,8,10,9);root->setSpacing(8);}
    for(QPushButton *b:page->findChildren<QPushButton*>()){
        b->setMinimumHeight(qMax(30,b->fontMetrics().height()+14));
        b->setStyleSheet(QStringLiteral("QPushButton{background:#151d24;color:#eef2f5;border:1px solid #35414b;border-radius:5px;padding:5px 10px;font-weight:700;}QPushButton:hover{background:#202830;border-color:#ff7a00;}QPushButton:pressed{background:#0d1419;}QPushButton:disabled{color:#64717a;background:#141b20;border-color:#27313a;}"));
    }
}

static void polishChrome(QMainWindow *window)
{
    if (!window) return;
    const QFontMetrics fm(window->font());
    if (QFrame *header=window->findChild<QFrame*>(QStringLiteral("uiRebuildHeader"))) {
        header->setFixedHeight(qBound(48,fm.height()*3+10,64));
        header->setStyleSheet(QStringLiteral("#uiRebuildHeader{background:qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #070c10,stop:.55 #0b1116,stop:1 #080d12);border:0;border-bottom:1px solid #2a343d;}#uiRebuildHeader QLabel{background:transparent;border:0;}#m_connectButton{background:#ff7a00;color:#11161a;border:1px solid #ffa043;border-radius:6px;font-weight:800;padding:5px 15px;}#m_disconnectButton{background:#171f26;color:#d6dde2;border:1px solid #33404a;border-radius:6px;font-weight:800;padding:5px 15px;}#m_disconnectButton:enabled{background:#7b2525;color:white;border-color:#a63a3a;}#m_connectButton:disabled,#m_disconnectButton:disabled{background:#171e24;color:#5f6972;border-color:#29323a;}"));
    }
    if (QFrame *status=window->findChild<QFrame*>(QStringLiteral("uiRebuildStatus"))) {
        status->setFixedHeight(qBound(31,fm.height()+17,41));
        status->setStyleSheet(QStringLiteral("#uiRebuildStatus{background:#070c10;border-top:1px solid #26313a;}#uiRebuildStatus QLabel{background:transparent;border-right:1px solid #202a31;color:#c8d1d7;padding:0 9px;}#uiRebuildStatus QLineEdit{background:#0c1318;color:#dce4e9;border:1px solid #34414b;border-radius:5px;padding:3px 7px;}#uiRebuildStatus QPushButton{background:#111920;color:#e5ebef;border:1px solid #34414b;border-radius:5px;padding:3px 9px;font-weight:700;}#uiRebuildStatus QPushButton:hover{border-color:#ff7a00;background:#182129;}#m_startLoggingButton{border-color:#a75d1d;color:#ff9b3d;}#m_stopLoggingButton{border-color:#713333;color:#f08b8b;}"));
    }
}

static void applyPolish(QMainWindow *window)
{
    if (!window) return;
    polishNavigation(window);
    polishOverview(window);
    polishSettings(window);
    polishErrors(window);
    polishActuators(window);
    polishChrome(window);
}

class RuntimeVisualPolishInstaller : public QObject
{
public: explicit RuntimeVisualPolishInstaller(QObject *parent=nullptr):QObject(parent){}
protected:
    bool eventFilter(QObject *watched,QEvent *event) override
    {
        QMainWindow *window=qobject_cast<QMainWindow*>(watched);
        if(!window||window->objectName()!=QStringLiteral("MainWindow")) return QObject::eventFilter(watched,event);
        if((event->type()==QEvent::Show||event->type()==QEvent::Polish)&&!window->property("runtimeVisualPolishScheduled").toBool()){
            window->setProperty("runtimeVisualPolishScheduled",true);
            QTimer::singleShot(760,window,[window](){applyPolish(window);});
            QTimer::singleShot(980,window,[window](){applyPolish(window);});
        } else if(event->type()==QEvent::Resize&&window->property("runtimeVisualPolishScheduled").toBool()){
            QTimer::singleShot(30,window,[window](){applyPolish(window);});
        }
        return QObject::eventFilter(watched,event);
    }
};

void installRuntimeVisualPolish(){QApplication *app=qobject_cast<QApplication*>(QCoreApplication::instance());if(app)app->installEventFilter(new RuntimeVisualPolishInstaller(app));}

}

Q_COREAPP_STARTUP_FUNCTION(installRuntimeVisualPolish)
