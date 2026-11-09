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
#include <QScrollBar>
#include <QSizePolicy>
#include <QStatusBar>
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
    const QString disabledQss=QStringLiteral(
        "QTableView:disabled,QTableWidget:disabled{color:#91a1ab;background:#081016;}"
        "QTableView::item:disabled,QTableWidget::item:disabled{color:#7f909b;background:#081016;}"
        "QHeaderView::section{color:#edf2f4;}" );
    for(QTableView *t:w->findChildren<QTableView*>()) {
        QPalette pal=t->palette();
        pal.setColor(QPalette::Disabled,QPalette::Text,disabledText);
        pal.setColor(QPalette::Disabled,QPalette::WindowText,disabledText);
        pal.setColor(QPalette::Disabled,QPalette::Base,QColor("#081016"));
        t->setPalette(pal);
        if(!t->property("darkReadableStyled").toBool()) {
            t->setStyleSheet(t->styleSheet()+disabledQss);
            t->setProperty("darkReadableStyled",true);
        }
    }
    for(QTableWidget *t:w->findChildren<QTableWidget*>()) {
        QPalette pal=t->palette();
        pal.setColor(QPalette::Disabled,QPalette::Text,disabledText);
        pal.setColor(QPalette::Disabled,QPalette::WindowText,disabledText);
        pal.setColor(QPalette::Disabled,QPalette::Base,QColor("#081016"));
        t->setPalette(pal);
        if(!t->property("darkReadableStyled").toBool()) {
            t->setStyleSheet(t->styleSheet()+disabledQss);
            t->setProperty("darkReadableStyled",true);
        }
    }
}

static void styleScrollBars(QMainWindow *w)
{
    if(!w) return;
    const QString qss=QStringLiteral(
        "QScrollBar:vertical{background:#070c11;width:9px;margin:0;border:0;}"
        "QScrollBar::handle:vertical{background:#35434d;border-radius:4px;min-height:28px;}"
        "QScrollBar::handle:vertical:hover{background:#52616b;}"
        "QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical{height:0;}"
        "QScrollBar::add-page:vertical,QScrollBar::sub-page:vertical{background:transparent;}"
        "QScrollBar:horizontal{background:#070c11;height:9px;margin:0;border:0;}"
        "QScrollBar::handle:horizontal{background:#35434d;border-radius:4px;min-width:28px;}"
        "QScrollBar::handle:horizontal:hover{background:#52616b;}"
        "QScrollBar::add-line:horizontal,QScrollBar::sub-line:horizontal{width:0;}"
        "QScrollBar::add-page:horizontal,QScrollBar::sub-page:horizontal{background:transparent;}" );
    for(QScrollBar *bar:w->findChildren<QScrollBar*>()) bar->setStyleSheet(qss);
}

static void cleanBottomBar(QMainWindow *w)
{
    QFrame *status=w?w->findChild<QFrame*>(QStringLiteral("uiRebuildStatus")):nullptr;
    if(status) {
        status->setMinimumHeight(29); status->setMaximumHeight(34);
        status->setStyleSheet(QStringLiteral("#uiRebuildStatus{background:#070c11;border-top:1px solid #293640;}#uiRebuildStatus QLabel{background:transparent;color:#cbd4da;border-right:1px solid #25313a;padding:0 9px;}"));
    }
    if(QStatusBar *native=w?w->statusBar():nullptr) {
        native->setSizeGripEnabled(false);
        native->setStyleSheet(QStringLiteral("QStatusBar{background:#070c11;color:#cbd4da;border-top:1px solid #293640;padding:1px 6px;}QStatusBar::item{border:0;}"));
        native->setMaximumHeight(22);
    }
}

static void applyFinalFixes(QMainWindow *w)
{
    if(!w) return;
    fixAnalysis(w);
    fixDiagnostic(w);
    makeTablesReadable(w);
    styleScrollBars(w);
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