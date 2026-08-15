#include <QAbstractItemView>
#include <QApplication>
#include <QBoxLayout>
#include <QCoreApplication>
#include <QEvent>
#include <QFontMetrics>
#include <QFrame>
#include <QGridLayout>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMainWindow>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#include <QPushButton>
#include <QScrollArea>
#include <QSizePolicy>
#include <QTabWidget>
#include <QTableView>
#include <QTableWidget>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>
#include "i18n.h"

namespace {

static QString className(QObject *o)
{
    return o ? QString::fromLatin1(o->metaObject()->className()) : QString();
}

static QIcon utilityIcon(const QString &kind)
{
    QPixmap pm(24,24);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing,true);
    p.setPen(QPen(QColor("#ff8a1c"),1.8,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));
    p.setBrush(Qt::NoBrush);

    if(kind==QStringLiteral("record")) {
        p.setBrush(QColor("#ff8a1c"));
        p.drawEllipse(QPointF(12,12),5.2,5.2);
    } else if(kind==QStringLiteral("stop")) {
        p.setBrush(QColor("#ff8a1c"));
        p.drawRoundedRect(QRectF(7,7,10,10),1.5,1.5);
    } else if(kind==QStringLiteral("camera")) {
        p.drawRoundedRect(QRectF(4,7,16,11),2,2);
        p.drawEllipse(QPointF(12,12.5),3.3,3.3);
        p.drawLine(8,7,10,4.8); p.drawLine(10,4.8,14,4.8); p.drawLine(14,4.8,16,7);
    } else if(kind==QStringLiteral("gallery")) {
        p.drawRoundedRect(QRectF(4,5,16,14),2,2);
        p.drawEllipse(QPointF(15.7,9),1.5,1.5);
        QPolygonF q; q << QPointF(6,17) << QPointF(10.2,12.3) << QPointF(13,15) << QPointF(16,12.2) << QPointF(19,17);
        p.drawPolyline(q);
    }
    return QIcon(pm);
}

static QPushButton *buttonByText(QMainWindow *w,const QString &text)
{
    if(!w) return nullptr;
    for(QPushButton *b:w->findChildren<QPushButton*>())
        if(b && b->text()==text) return b;
    return nullptr;
}

static QGridLayout *gridContaining(QWidget *root,const QList<QWidget*> &widgets)
{
    if(!root || widgets.isEmpty()) return nullptr;
    QGridLayout *best=nullptr;
    int bestCount=0;
    for(QGridLayout *g:root->findChildren<QGridLayout*>()) {
        int count=0;
        for(QWidget *x:widgets) if(x && g->indexOf(x)>=0) ++count;
        if(count>bestCount) { best=g; bestCount=count; }
    }
    return best;
}

static void rememberButtonText(QPushButton *b)
{
    if(!b) return;
    if(!b->property("darkOriginalText").isValid()) b->setProperty("darkOriginalText",b->text());
}

static void styleSidebarButton(QPushButton *b,const QString &kind)
{
    if(!b) return;
    rememberButtonText(b);
    b->setIcon(utilityIcon(kind));
    b->setIconSize(QSize(19,19));
    b->setMinimumHeight(31);
    b->setMaximumHeight(34);
    b->setStyleSheet(QStringLiteral(
        "QPushButton{background:#111920;color:#e7edf0;border:1px solid #33414b;border-radius:5px;padding:4px 8px;text-align:left;font-weight:700;}"
        "QPushButton:hover{background:#1b252c;border-color:#ff7a00;color:#fff;}"
        "QPushButton:pressed{background:#090f14;}"
        "QPushButton:disabled{background:#11181e;color:#5f6c75;border-color:#27323b;}"));
}

static void applySidebarState(QMainWindow *w)
{
    QFrame *sidebar=w?w->findChild<QFrame*>(QStringLiteral("darkSidebar")):nullptr;
    QListWidget *nav=w?w->findChild<QListWidget*>(QStringLiteral("uiRebuildNav")):nullptr;
    if(!sidebar || !nav) return;

    const bool collapsed=sidebar->property("collapsed").toBool();
    QFontMetrics fm(nav->font());
    int longest=0;
    const int textRole=Qt::UserRole+42;
    for(int i=0;i<nav->count();++i) {
        QListWidgetItem *item=nav->item(i);
        if(!item) continue;
        if(!item->data(textRole).isValid()) item->setData(textRole,item->text());
        const QString original=item->data(textRole).toString();
        longest=qMax(longest,fm.horizontalAdvance(original));
        item->setText(collapsed?QString():original);
        item->setTextAlignment(collapsed?Qt::AlignCenter:(Qt::AlignLeft|Qt::AlignVCenter));
        item->setSizeHint(QSize(0,collapsed?43:39));
    }

    const int expanded=qBound(198,longest+78,252);
    const int width=collapsed?58:expanded;
    sidebar->setMinimumWidth(width); sidebar->setMaximumWidth(width);
    nav->setMinimumWidth(width); nav->setMaximumWidth(width);
    nav->setIconSize(QSize(22,22));
    nav->setStyleSheet(QStringLiteral(
        "#uiRebuildNav{background:#080e13;color:#cbd4da;border:0;padding:6px 0;}"
        "#uiRebuildNav::item{padding:5px %1px;border-left:3px solid transparent;border-bottom:1px solid #121a20;}"
        "#uiRebuildNav::item:hover{background:#151f26;color:#fff;}"
        "#uiRebuildNav::item:selected{background:qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #332417,stop:1 #161d21);color:#ffad5c;border-left:3px solid #ff7a00;font-weight:700;}"
    ).arg(collapsed?12:11));

    QToolButton *toggle=sidebar->findChild<QToolButton*>(QStringLiteral("darkSidebarToggle"));
    if(toggle) {
        toggle->setText(collapsed?QStringLiteral("»"):QStringLiteral("«"));
        toggle->setToolTip(collapsed?QStringLiteral("Déployer la barre latérale"):QStringLiteral("Réduire la barre latérale"));
    }

    QFrame *utils=sidebar->findChild<QFrame*>(QStringLiteral("darkSidebarUtilities"));
    if(utils) {
        if(QLabel *title=utils->findChild<QLabel*>(QStringLiteral("darkUtilitiesTitle"))) title->setVisible(!collapsed);
        if(QLineEdit *file=w->findChild<QLineEdit*>(QStringLiteral("m_logFileNameBox"))) file->setVisible(!collapsed);
        for(QPushButton *b:utils->findChildren<QPushButton*>(QString(),Qt::FindDirectChildrenOnly)) {
            rememberButtonText(b);
            const QString original=b->property("darkOriginalText").toString();
            b->setText(collapsed?QString():original);
            if(collapsed) {
                b->setMinimumWidth(38); b->setMaximumWidth(38);
                b->setStyleSheet(b->styleSheet()+QStringLiteral("QPushButton{text-align:center;padding:4px;}"));
            } else {
                b->setMinimumWidth(0); b->setMaximumWidth(QWIDGETSIZE_MAX);
            }
        }
    }
}

static void ensureSidebar(QMainWindow *w)
{
    if(!w) return;
    QWidget *workspace=w->findChild<QWidget*>(QStringLiteral("uiRebuildWorkspace"));
    QListWidget *nav=w->findChild<QListWidget*>(QStringLiteral("uiRebuildNav"));
    QHBoxLayout *workspaceLayout=workspace?qobject_cast<QHBoxLayout*>(workspace->layout()):nullptr;
    if(!workspace || !nav || !workspaceLayout) return;

    QFrame *sidebar=w->findChild<QFrame*>(QStringLiteral("darkSidebar"));
    QVBoxLayout *sideLayout=nullptr;
    if(!sidebar) {
        sidebar=new QFrame(workspace);
        sidebar->setObjectName(QStringLiteral("darkSidebar"));
        sidebar->setProperty("collapsed",false);
        sidebar->setAttribute(Qt::WA_StyledBackground,true);
        sidebar->setStyleSheet(QStringLiteral("#darkSidebar{background:qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #070c11,stop:1 #0c141a);border:0;border-right:1px solid #2a3540;}"));
        sideLayout=new QVBoxLayout(sidebar);
        sideLayout->setContentsMargins(0,5,0,7);
        sideLayout->setSpacing(5);

        QToolButton *toggle=new QToolButton(sidebar);
        toggle->setObjectName(QStringLiteral("darkSidebarToggle"));
        toggle->setAutoRaise(false);
        toggle->setFixedHeight(27);
        toggle->setStyleSheet(QStringLiteral("QToolButton{background:#101820;color:#ff9a32;border:1px solid #2d3943;border-radius:4px;margin:0 8px;font-weight:800;}QToolButton:hover{border-color:#ff7a00;background:#182229;}"));
        sideLayout->addWidget(toggle);

        workspaceLayout->removeWidget(nav);
        nav->setParent(sidebar);
        sideLayout->addWidget(nav,1);

        QFrame *utils=new QFrame(sidebar);
        utils->setObjectName(QStringLiteral("darkSidebarUtilities"));
        utils->setAttribute(Qt::WA_StyledBackground,true);
        utils->setStyleSheet(QStringLiteral("#darkSidebarUtilities{background:#0b1217;border-top:1px solid #27333c;padding-top:4px;}"));
        QVBoxLayout *uv=new QVBoxLayout(utils);
        uv->setContentsMargins(8,6,8,2);
        uv->setSpacing(5);
        QLabel *ut=new QLabel(QStringLiteral("OUTILS"),utils);
        ut->setObjectName(QStringLiteral("darkUtilitiesTitle"));
        QFont uf=ut->font(); uf.setBold(true); uf.setPointSizeF(qMax<qreal>(7.0,uf.pointSizeF()-1.0)); ut->setFont(uf);
        ut->setStyleSheet(QStringLiteral("color:#ff9828;background:transparent;border:0;padding:0 2px 3px 2px;"));
        uv->addWidget(ut);
        sideLayout->addWidget(utils,0);

        QObject::connect(toggle,&QToolButton::clicked,sidebar,[w,sidebar](){
            sidebar->setProperty("collapsed",!sidebar->property("collapsed").toBool());
            applySidebarState(w);
        });

        workspaceLayout->insertWidget(0,sidebar,0);
    } else {
        sideLayout=qobject_cast<QVBoxLayout*>(sidebar->layout());
    }

    QFrame *utils=sidebar->findChild<QFrame*>(QStringLiteral("darkSidebarUtilities"));
    QVBoxLayout *uv=utils?qobject_cast<QVBoxLayout*>(utils->layout()):nullptr;
    if(!utils || !uv) return;

    QLineEdit *file=w->findChild<QLineEdit*>(QStringLiteral("m_logFileNameBox"));
    QPushButton *start=w->findChild<QPushButton*>(QStringLiteral("m_startLoggingButton"));
    QPushButton *stop=w->findChild<QPushButton*>(QStringLiteral("m_stopLoggingButton"));
    QPushButton *snapshot=w->findChild<QPushButton*>(QStringLiteral("darkSnapshotButton"));
    QPushButton *view=w->findChild<QPushButton*>(QStringLiteral("darkViewCapturesButton"));
    if(!snapshot) snapshot=buttonByText(w,I18n::text(7015));
    if(!view) view=buttonByText(w,I18n::text(7016));
    if(snapshot) snapshot->setObjectName(QStringLiteral("darkSnapshotButton"));
    if(view) view->setObjectName(QStringLiteral("darkViewCapturesButton"));

    if(file) {
        file->setParent(utils);
        file->setMinimumHeight(29); file->setMaximumHeight(31);
        file->setMinimumWidth(0); file->setMaximumWidth(QWIDGETSIZE_MAX);
        file->setStyleSheet(QStringLiteral("QLineEdit{background:#071016;color:#e2e9ed;border:1px solid #34434e;border-radius:5px;padding:4px 7px;}QLineEdit:focus{border-color:#ff7a00;}"));
        if(uv->indexOf(file)<0) uv->addWidget(file);
    }
    if(start) { start->setParent(utils); styleSidebarButton(start,QStringLiteral("record")); if(uv->indexOf(start)<0) uv->addWidget(start); }
    if(stop) { stop->setParent(utils); styleSidebarButton(stop,QStringLiteral("stop")); if(uv->indexOf(stop)<0) uv->addWidget(stop); }
    if(snapshot) { snapshot->setParent(utils); styleSidebarButton(snapshot,QStringLiteral("camera")); if(uv->indexOf(snapshot)<0) uv->addWidget(snapshot); }
    if(view) { view->setParent(utils); styleSidebarButton(view,QStringLiteral("gallery")); if(uv->indexOf(view)<0) uv->addWidget(view); }

    if(QLabel *fileLabel=w->findChild<QLabel*>(QStringLiteral("m_logFileNameLabel"))) fileLabel->hide();
    applySidebarState(w);
}

static void fixSettings(QMainWindow *w)
{
    QFrame *metrics=w?w->findChild<QFrame*>(QStringLiteral("settingsMetrics")):nullptr;
    if(!metrics) return;

    QList<QWidget*> gauges;
    const QList<QWidget*> direct=metrics->findChildren<QWidget*>(QString(),Qt::FindDirectChildrenOnly);
    for(QWidget *x:direct) {
        if(!x || !x->isVisible()) continue;
        if(x->testAttribute(Qt::WA_TransparentForMouseEvents) && !qobject_cast<QLabel*>(x)) gauges<<x;
    }
    if(gauges.size()!=5) return;
    QGridLayout *grid=gridContaining(metrics,gauges);
    if(!grid) return;

    const int mw=qMax(1,metrics->width());
    const int cols=mw>=820?5:(mw>=520?3:(mw>=340?2:1));
    if(metrics->property("finalGaugeCols").toInt()!=cols) {
        for(QWidget *g:gauges) grid->removeWidget(g);
        for(int c=0;c<5;++c) grid->setColumnStretch(c,c<cols?1:0);
        const int rows=(gauges.size()+cols-1)/cols;
        for(int r=0;r<5;++r) grid->setRowStretch(r,r<rows?1:0);
        for(int i=0;i<gauges.size();++i) grid->addWidget(gauges.at(i),i/cols,i%cols);
        metrics->setProperty("finalGaugeCols",cols);
    }
    grid->setHorizontalSpacing(9); grid->setVerticalSpacing(9);
    const int minH=cols>=5?155:(cols==3?145:135);
    for(QWidget *g:gauges) {
        g->setMinimumSize(cols>=5?125:105,minH);
        g->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
        g->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    }
    metrics->setMinimumHeight(cols>=5?205:(cols==3?325:430));
    metrics->setMaximumHeight(QWIDGETSIZE_MAX);
}

static QWidget *analysisWidget(QMainWindow *w)
{
    if(!w) return nullptr;
    for(QWidget *x:w->findChildren<QWidget*>())
        if(className(x)==QStringLiteral("AnalysisTab")) return x;
    return nullptr;
}

static void updateAnalysisEmpty(QWidget *analysis,QLabel *fileLabel,QLabel *empty)
{
    if(!analysis || !empty) return;
    const bool noFile=!fileLabel || fileLabel->text()==I18n::text(6443);
    empty->setVisible(noFile);
}

static void fixAnalysis(QMainWindow *w)
{
    QWidget *analysis=analysisWidget(w);
    if(!analysis) return;

    QWidget *topBar=analysis->findChild<QWidget*>(QStringLiteral("analysisTopBar"));
    QWidget *rightPanel=analysis->findChild<QWidget*>(QStringLiteral("analysisRightPanel"));
    QLabel *fileLabel=nullptr;

    if(!topBar || !rightPanel) {
        QWidget *legacy=nullptr;
        const QList<QWidget*> direct=analysis->findChildren<QWidget*>(QString(),Qt::FindDirectChildrenOnly);
        for(QWidget *c:direct) {
            if(!c || qobject_cast<QScrollArea*>(c)) continue;
            if(c->findChildren<QPushButton*>().size()>=4 && c->findChild<QScrollArea*>()) { legacy=c; break; }
        }
        QVBoxLayout *root=qobject_cast<QVBoxLayout*>(analysis->layout());
        QHBoxLayout *mainRow=nullptr;
        if(root) for(int i=0;i<root->count();++i) if(QLayout *l=root->itemAt(i)->layout()) { mainRow=qobject_cast<QHBoxLayout*>(l); if(mainRow) break; }
        if(legacy && root && mainRow) {
            QList<QPushButton*> buttons=legacy->findChildren<QPushButton*>();
            QPushButton *load=nullptr,*all=nullptr,*none=nullptr,*overlay=nullptr;
            for(QPushButton *b:buttons) {
                if(b->isCheckable()) { overlay=b; continue; }
                if(!load && (b->text().contains(QStringLiteral("CSV"),Qt::CaseInsensitive)||b->text().contains(QStringLiteral("TXT"),Qt::CaseInsensitive))) { load=b; continue; }
            }
            if(!load && !buttons.isEmpty()) load=buttons.first();
            for(QPushButton *b:buttons) {
                if(b==load || b==overlay) continue;
                if(!all) all=b; else if(!none) none=b;
            }

            QLabel *channelsLabel=nullptr;
            for(QLabel *l:legacy->findChildren<QLabel*>(QString(),Qt::FindDirectChildrenOnly)) {
                if(l->wordWrap() && !fileLabel) fileLabel=l;
                else if(!channelsLabel) channelsLabel=l;
            }
            QScrollArea *parameters=legacy->findChild<QScrollArea*>();

            topBar=new QWidget(analysis);
            topBar->setObjectName(QStringLiteral("analysisTopBar"));
            topBar->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
            QHBoxLayout *top=new QHBoxLayout(topBar); top->setContentsMargins(8,5,8,5); top->setSpacing(7);
            if(load) { load->setParent(topBar); top->addWidget(load); }
            if(fileLabel) { fileLabel->setParent(topBar); fileLabel->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred); top->addWidget(fileLabel,1); }
            root->insertWidget(0,topBar);

            rightPanel=new QWidget(analysis);
            rightPanel->setObjectName(QStringLiteral("analysisRightPanel"));
            rightPanel->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Expanding);
            QVBoxLayout *right=new QVBoxLayout(rightPanel); right->setContentsMargins(9,9,9,9); right->setSpacing(7);
            if(channelsLabel) {
                channelsLabel->setParent(rightPanel); channelsLabel->setStyleSheet(QStringLiteral("color:#ff9828;font-weight:800;padding:2px 0 5px 0;border-bottom:1px solid #2c3842;")); right->addWidget(channelsLabel);
            } else {
                QLabel *title=new QLabel(I18n::text(6447),rightPanel); title->setStyleSheet(QStringLiteral("color:#ff9828;font-weight:800;padding:2px 0 5px 0;border-bottom:1px solid #2c3842;")); right->addWidget(title);
            }
            if(parameters) {
                parameters->setParent(rightPanel); parameters->setWidgetResizable(true); parameters->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff); right->addWidget(parameters,1);
            }
            QHBoxLayout *select=new QHBoxLayout; select->setSpacing(5);
            if(all) { all->setParent(rightPanel); select->addWidget(all); }
            if(none) { none->setParent(rightPanel); select->addWidget(none); }
            right->addLayout(select);
            if(overlay) { overlay->setParent(rightPanel); overlay->setMinimumWidth(0); overlay->setMaximumWidth(QWIDGETSIZE_MAX); overlay->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred); right->addWidget(overlay); }

            mainRow->removeWidget(legacy); legacy->hide(); legacy->setMaximumWidth(0);
            mainRow->addWidget(rightPanel,0);
            for(int i=0;i<mainRow->count();++i) mainRow->setStretch(i,i==mainRow->count()-1?0:1);
            analysis->setProperty("finalAnalysisLayout",true);
        }
    } else {
        const QList<QLabel*> labels=topBar->findChildren<QLabel*>();
        if(!labels.isEmpty()) fileLabel=labels.first();
    }

    if(!topBar || !rightPanel) return;
    topBar->setStyleSheet(QStringLiteral("#analysisTopBar{background:qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #18232b,stop:1 #0d151b);border:1px solid #2d3943;border-left:3px solid #ff7a00;border-radius:6px;}"));
    rightPanel->setStyleSheet(QStringLiteral("#analysisRightPanel{background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #131c23,stop:1 #091116);border:1px solid #2d3943;border-radius:6px;}"));
    topBar->setFixedHeight(qBound(34,analysis->height()/13,43));
    const int rightW=qBound(205,qRound(analysis->width()*0.21),300);
    rightPanel->setMinimumWidth(rightW); rightPanel->setMaximumWidth(rightW);

    QScrollArea *stack=nullptr;
    for(QScrollArea *s:analysis->findChildren<QScrollArea*>(QString(),Qt::FindDirectChildrenOnly)) { stack=s; break; }
    if(stack && stack->widget()) {
        stack->setFrameShape(QFrame::NoFrame); stack->setWidgetResizable(true); stack->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        QLabel *empty=stack->widget()->findChild<QLabel*>(QStringLiteral("analysisEmptyState"),Qt::FindDirectChildrenOnly);
        if(!empty) {
            empty=new QLabel(I18n::text(6443)+QStringLiteral("\n\n")+I18n::text(6442),stack->widget());
            empty->setObjectName(QStringLiteral("analysisEmptyState"));
            empty->setAlignment(Qt::AlignCenter); empty->setWordWrap(true);
            empty->setStyleSheet(QStringLiteral("color:#8f9da6;background:#0c141a;border:1px dashed #34424d;border-radius:8px;padding:28px;font-size:13px;"));
            if(QVBoxLayout *vl=qobject_cast<QVBoxLayout*>(stack->widget()->layout())) vl->insertWidget(0,empty,1,Qt::AlignCenter);
        }
        updateAnalysisEmpty(analysis,fileLabel,empty);
        if(!analysis->findChild<QTimer*>(QStringLiteral("analysisEmptyTimer"))) {
            QTimer *timer=new QTimer(analysis); timer->setObjectName(QStringLiteral("analysisEmptyTimer")); timer->setInterval(350);
            QObject::connect(timer,&QTimer::timeout,analysis,[analysis,fileLabel,empty](){updateAnalysisEmpty(analysis,fileLabel,empty);}); timer->start();
        }
    }
}

static void fixDiagnostic(QMainWindow *w)
{
    QWidget *diag=nullptr;
    for(QWidget *x:w->findChildren<QWidget*>()) if(className(x)==QStringLiteral("DiagnosticPanel")) { diag=x; break; }
    if(!diag) return;
    QTableWidget *table=diag->findChild<QTableWidget*>();
    if(!table) return;
    table->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    table->setWordWrap(false);
    if(QHeaderView *h=table->horizontalHeader()) {
        h->setMinimumSectionSize(55);
        h->setSectionResizeMode(0,QHeaderView::ResizeToContents);
        h->setSectionResizeMode(1,QHeaderView::ResizeToContents);
        h->setSectionResizeMode(2,QHeaderView::ResizeToContents);
        h->setSectionResizeMode(3,QHeaderView::Stretch);
        h->setStretchLastSection(true);
    }
}

static void makeTablesReadable(QMainWindow *w)
{
    const QColor disabledText("#91a1ab");
    for(QTableView *t:w->findChildren<QTableView*>()) {
        QPalette pal=t->palette();
        pal.setColor(QPalette::Disabled,QPalette::Text,disabledText);
        pal.setColor(QPalette::Disabled,QPalette::WindowText,disabledText);
        pal.setColor(QPalette::Disabled,QPalette::Base,QColor("#081016"));
        t->setPalette(pal);
    }
    for(QTableWidget *t:w->findChildren<QTableWidget*>()) {
        QPalette pal=t->palette();
        pal.setColor(QPalette::Disabled,QPalette::Text,disabledText);
        pal.setColor(QPalette::Disabled,QPalette::WindowText,disabledText);
        pal.setColor(QPalette::Disabled,QPalette::Base,QColor("#081016"));
        t->setPalette(pal);
    }
}

static void cleanBottomBar(QMainWindow *w)
{
    QFrame *status=w?w->findChild<QFrame*>(QStringLiteral("uiRebuildStatus")):nullptr;
    if(!status) return;
    status->setMinimumHeight(29); status->setMaximumHeight(34);
    status->setStyleSheet(QStringLiteral("#uiRebuildStatus{background:#070c11;border-top:1px solid #293640;}#uiRebuildStatus QLabel{background:transparent;color:#cbd4da;border-right:1px solid #25313a;padding:0 9px;}"));
}

static void applyFinalFixes(QMainWindow *w)
{
    if(!w) return;
    ensureSidebar(w);
    fixSettings(w);
    fixAnalysis(w);
    fixDiagnostic(w);
    makeTablesReadable(w);
    cleanBottomBar(w);
}

class DarkFixesInstaller : public QObject
{
public:
    explicit DarkFixesInstaller(QObject *parent=nullptr):QObject(parent){}
protected:
    bool eventFilter(QObject *watched,QEvent *event) override
    {
        QMainWindow *w=qobject_cast<QMainWindow*>(watched);
        if(!w || w->objectName()!=QStringLiteral("MainWindow")) return QObject::eventFilter(watched,event);
        if((event->type()==QEvent::Show || event->type()==QEvent::Polish) && !w->property("darkFixesScheduled").toBool()) {
            w->setProperty("darkFixesScheduled",true);
            QTimer::singleShot(1500,w,[w](){applyFinalFixes(w);});
            QTimer::singleShot(1850,w,[w](){applyFinalFixes(w);});
        } else if(event->type()==QEvent::Resize && w->property("darkFixesScheduled").toBool()) {
            QTimer::singleShot(140,w,[w](){applyFinalFixes(w);});
        }
        return QObject::eventFilter(watched,event);
    }
};

void installDarkFixes()
{
    QApplication *app=qobject_cast<QApplication*>(QCoreApplication::instance());
    if(app) app->installEventFilter(new DarkFixesInstaller(app));
}

}

Q_COREAPP_STARTUP_FUNCTION(installDarkFixes)
