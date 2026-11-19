#include <QCheckBox>
#include <QApplication>
#include <QVariant>
#include "applicationvisualstyle.h"
#include "uishell.h"
#include "summarytab.h"
#include "rawpage.h"
#include "settingspage.h"
#include "styles.h"
#include <QComboBox>
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
#include <QPalette>
#include <QScrollBar>
#include <QStatusBar>
#include <QMenu>
#include <QMenuBar>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSizePolicy>
#include <QTabWidget>
#include <QTableView>
#include <QTableWidget>
#include <QTextEdit>
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
        "QScrollBar::handle:vertical{background:#ff8a22;border-radius:4px;min-height:28px;}"
        "QScrollBar::handle:vertical:hover{background:#ffb05a;}"
        "QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical{height:0;}"
        "QScrollBar::add-page:vertical,QScrollBar::sub-page:vertical{background:transparent;}"
        "QScrollBar:horizontal{background:#070c11;height:9px;margin:0;border:0;}"
        "QScrollBar::handle:horizontal{background:#ff8a22;border-radius:4px;min-width:28px;}"
        "QScrollBar::handle:horizontal:hover{background:#ffb05a;}"
        "QScrollBar::add-line:horizontal,QScrollBar::sub-line:horizontal{width:0;}"
        "QScrollBar::add-page:horizontal,QScrollBar::sub-page:horizontal{background:transparent;}" );
    // Conserver les styles explicites des deux ascenseurs d'Analyse.
    // Sinon ce style global remplace leurs poignées orange à chaque redessin.
    const QScrollArea *channels=w->findChild<QScrollArea*>(
        QStringLiteral("analysisChannelsScroll"));
    const QScrollArea *plots=w->findChild<QScrollArea*>(
        QStringLiteral("analysisStackScroll"));
    const QScrollBar *channelsBar=channels?channels->verticalScrollBar():nullptr;
    const QScrollBar *plotsBar=plots?plots->verticalScrollBar():nullptr;
    for(QScrollBar *bar:w->findChildren<QScrollBar*>()) {
        if(bar==channelsBar || bar==plotsBar) continue;
        bar->setStyleSheet(qss);
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
        // SummaryTab is the native owner of the three raw-data tables.
        bool isSummary=false;
        for (QWidget *parent=t->parentWidget(); parent; parent=parent->parentWidget())
          if (parent->inherits("SummaryTab")) { isSummary=true; break; }
        if (isSummary) continue;
        t->setStyleSheet(qss);t->setAlternatingRowColors(true);t->setShowGrid(false);t->setMinimumSize(0,0);t->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
        if(t->verticalHeader()) t->verticalHeader()->setVisible(false);
        if(t->horizontalHeader()){t->horizontalHeader()->setStretchLastSection(true);t->horizontalHeader()->setMinimumHeight(qMax(28,t->fontMetrics().height()+13));}
    }
    makeTablesReadable(w);
}

static void styleControls(QMainWindow *w)
{
    const QString button=QStringLiteral("QPushButton{background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #1b252c,stop:1 #111920);color:#eaf0f3;border:1px solid #35434e;border-radius:5px;padding:5px 10px;font-weight:650;}QPushButton:hover{border-color:#ff7a00;background:#202a31;color:#fff;}QPushButton:pressed{background:#0b1116;}QPushButton:checked{background:#3b2918;border-color:#ff8a1c;color:#ffad5c;}QPushButton:disabled{background:#141b20;color:#63707a;border-color:#27313a;}");
    for(QPushButton *b:w->findChildren<QPushButton*>()){
        if(b->objectName()==QStringLiteral("m_connectButton")||b->objectName()==QStringLiteral("m_disconnectButton")) continue;
        // These two Erreurs controls have their own user-approved
        // black/orange style, which must not be replaced by global styling.
        if(b->objectName()==QStringLiteral("m_clearFaultsButton") ||
           b->objectName()==QStringLiteral("m_clearRecordedAnomalies")) continue;
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

static void apply(QMainWindow *w)
{
    if(!w) return;
    reflowOverview(w);
    stylePages(w);
    styleTables(w);
    styleControls(w);
    styleAnalysis(w);
    styleChrome(w);
    styleScrollBars(w);
}

} // namespace

// The historical dark-only shell installs widget-local stylesheets.
// A global STYLE_LIGHT used to be overridden by those styles, so selecting
// "Clair" never changed the actual user interface. This is now the single
// theme owner: capture the native dark styles once, let the light palette
// style all surface/controls, and restore the untouched originals on dark.
// Gauge paint routines are untouched and all 14 layouts remain identical.
namespace {
constexpr const char *kNativeWidgetStyle="ecuOriginalNativeStyle158";

static QString lightShellStyle()
{
    return QStringLiteral(R"STYLE(
QMainWindow,QWidget#uiRebuildWorkspace,QWidget#overview_tab,QWidget#database_tab,
QWidget#raw,QWidget#ia_mems_tab,QWidget#ECU,
QTabWidget#Tab_main,QTabWidget::pane,
QFrame#darkSidebar,QFrame#darkSidebarUtilities,QFrame#uiRebuildHeader,
QFrame#uiRebuildStatus,QFrame#overviewHeading,QFrame#memsDatabaseFooter,
QFrame#memsDatabaseBrowser,QFrame#uiRebuildWorkspace{
    background:#ffffff;color:#202936;border-color:#d4dce4;
}
QFrame#darkSidebar{background:#f5f7fa;border:0;border-right:1px solid #d5dee7;}
QFrame#darkSidebarUtilities{background:#f1f4f8;border:0;border-top:1px solid #d4dce4;}
QFrame#uiRebuildHeader{background:#ffffff;border:0;border-bottom:1px solid #d6dee6;}
QFrame#uiRebuildStatus{background:#f5f7fa;border:0;border-top:1px solid #d6dee6;}
QFrame#overviewHeading{background:#f6f8fb;border:1px solid #dce3eb;border-left:3px solid #f07800;border-radius:5px;}
QWidget#analysisTab,QWidget#analysisPlotsContent,QWidget#analysisChannelsContent,
QWidget#injection_tab,QFrame#injectionRawPanel,
QScrollArea#analysisStackScroll,QScrollArea#analysisChannelsScroll{
    background:#ffffff;color:#202936;border-color:#d4dce4;
}
QWidget#analysisTopBar,QFrame#injectionHeading{
    background:#f7f9fb;color:#202936;border:1px solid #d5dee6;border-left:3px solid #f07800;border-radius:5px;
}
QWidget#analysisRightPanel{
    background:#f8fafc;color:#202936;border:1px solid #d5dee6;border-radius:5px;
}
QLabel#analysisEmptyState{
    background:#ffffff;color:#475564;border:1px dashed #c3cfd9;border-radius:8px;padding:28px;
}
QFrame#uiRebuildHero_0,QFrame[uiHero="true"]{background:#f7f9fb;border:1px solid #d7e0e9;border-left:3px solid #f07800;}
QLabel,QCheckBox,QRadioButton{color:#202936;background:transparent;}
QLabel#darkUtilitiesTitle,QLabel#memsHelpHeading{color:#b35d00;}
QLabel#uiRebuildBrand{color:#202936;font-weight:700;}
QLabel#memsDatabaseUpdateTitle{color:#a75700;font-weight:700;}
QListWidget#uiRebuildNav{
    background:#f5f7fa;color:#202936;border:0;padding:2px 0;outline:0;
}
QListWidget#uiRebuildNav::item{
    background:transparent;color:#263342;
    padding:3px 11px;border:0;border-left:3px solid transparent;
    border-bottom:1px solid #e5ebf1;
}
QListWidget#uiRebuildNav::item:hover{background:#e8eef5;color:#1d2937;}
QListWidget#uiRebuildNav::item:selected{
    background:#fff0df;color:#ae5200;border-left:3px solid #ef7c00;font-weight:700;
}
QMenuBar{background:#ffffff;color:#202936;border-bottom:1px solid #d7e0e9;}
QMenuBar::item:selected{background:#e7edf5;color:#202936;}
QMenu{background:#ffffff;color:#202936;border:1px solid #cbd6e0;}
QMenu::item:selected{background:#ffead4;color:#9a4b00;}
QStatusBar{background:#f5f7fa;color:#263342;border-top:1px solid #d4dee7;}
QStatusBar QLabel{color:#263342;background:transparent;}
QGroupBox{background:#f7f9fb;color:#202936;border:1px solid #d9e1e9;border-radius:6px;margin-top:15px;font-weight:700;}
QGroupBox::title{subcontrol-origin:margin;left:12px;padding:0 6px;color:#a95800;}
QFrame#settingsMetrics,QFrame#settingsStates,QFrame#settingsAdjust,
QFrame#errorsStored,QFrame#errorsLive,QFrame#actuatorListCard,
QFrame#actuatorIacCard{
    background:#f8fafc;border:1px solid #d8e0e8;border-radius:6px;
}
QTableView,QTableWidget,QTreeWidget,QListView,QTextEdit,QPlainTextEdit,QTextBrowser{
    background:#ffffff;alternate-background-color:#f5f7fa;color:#202936;
    border:1px solid #cad5df;border-radius:4px;
}
QHeaderView::section{
    background:#edf2f6;color:#283748;border:0;border-bottom:1px solid #cbd5df;
    padding:5px 7px;font-weight:700;
}
QLineEdit,QComboBox,QSpinBox,QDoubleSpinBox{
    background:#ffffff;color:#202936;border:1px solid #c6d1db;
    border-radius:4px;padding:4px 7px;
}
QComboBox QAbstractItemView{background:#ffffff;color:#202936;selection-background-color:#fbdabd;}
QPushButton,QToolButton{
    background:#f3f6f9;color:#263442;border:1px solid #bfcbd7;
    border-radius:5px;padding:5px 10px;font-weight:650;
}
QPushButton:hover,QToolButton:hover{background:#e6edf5;border-color:#e88019;}
QPushButton:disabled,QToolButton:disabled{background:#f3f5f8;color:#9ca8b5;border-color:#d6dce2;}
QPushButton#m_connectButton{background:#f07800;color:#ffffff;border:1px solid #cd6500;font-weight:800;}
QPushButton#m_disconnectButton{background:#f4f5f7;color:#374452;border:1px solid #c8d3de;font-weight:800;}
QPushButton#m_clearFaultsButton,QPushButton#m_clearRecordedAnomalies{
    background:#ffffff;color:#c26000;border:1px solid #ff8800;
}
QPushButton#m_clearFaultsButton:hover:enabled,QPushButton#m_clearRecordedAnomalies:hover:enabled{
    background:#fff0e1;color:#a64d00;
}
QPushButton#m_clearFaultsButton:disabled,QPushButton#m_clearRecordedAnomalies:disabled{
    background:#ffffff;color:#c26000;border:1px solid #ff8800;
}
QScrollBar:vertical{background:#edf1f5;width:10px;margin:0;border:0;}
QScrollBar::handle:vertical{background:#aab9c8;border-radius:4px;min-height:28px;}
QScrollBar:horizontal{background:#edf1f5;height:10px;margin:0;border:0;}
QScrollBar::handle:horizontal{background:#aab9c8;border-radius:4px;min-width:28px;}
QScrollBar::add-line,QScrollBar::sub-line{width:0;height:0;}
)STYLE");
}

static void restoreDarkNativeStyles(QMainWindow *window)
{
    QList<QWidget*> widgets=window->findChildren<QWidget*>();
    widgets.prepend(window);
    for(QWidget *widget:widgets){
        if(!widget)continue;
        const QVariant saved=widget->property(kNativeWidgetStyle);
        if(saved.isValid()){
            widget->setStyleSheet(saved.toString());
            widget->setProperty(kNativeWidgetStyle,QVariant());
        }
    }
}

static void applyLightVisualStyle(QMainWindow *window)
{
    // All local dark styles are snapshots from the actual source widgets,
    // not guessed replacements. Restoring dark does not recreate pages,
    // change gauge geometry or interrupt an ECU connection.
    QList<QWidget*> widgets=window->findChildren<QWidget*>();
    widgets.prepend(window);
    for(QWidget *widget:widgets){
        if(!widget)continue;
        if(widget->inherits("QMenu") || widget->inherits("QToolTip"))
            continue;
        if(!widget->property(kNativeWidgetStyle).isValid())
            widget->setProperty(kNativeWidgetStyle,widget->styleSheet());
        if(!widget->styleSheet().isEmpty())
            widget->setStyleSheet(QString());
    }
    if(qApp)
        qApp->setStyleSheet(STYLE_LIGHT + lightShellStyle());
}
}

void applyApplicationVisualStyle(QMainWindow *window)
{
    if(!window)return;
    const bool dark=qApp && qApp->property("ecuDarkTheme").toBool();
    if(dark){
        // Remove the previous application's light stylesheet before restoring
        // the original dark widget styles. The switch is immediate; no restart.
        if(qApp) qApp->setStyleSheet(STYLE_DARK);
        restoreDarkNativeStyles(window);
        apply(window);
    }else{
        reflowOverview(window);
        applyLightVisualStyle(window);
    }
    // Gauge container colours must be updated with the theme. The same size
    // routine is already used by the native page: no new overlay or widget.
    fitSettingsPage(window);
    refreshUiShellTheme(window);
    if(SummaryTab *summary=window->findChild<SummaryTab*>())
        summary->applyVisualTheme(dark);
    applyRawDataVisualTheme(window->findChild<QWidget*>(QStringLiteral("raw")),dark);
    // Custom QPainter dials and graphs read ecuDarkTheme when they repaint.
    // Refresh their existing widgets; do not rebuild pages or interrupt ECU I/O.
    for(QWidget *widget:window->findChildren<QWidget*>())widget->update();
}

void resizeApplicationVisualStyle(QMainWindow *window)
{
    if (!window)
        return;
    reflowOverview(window);
    if(qApp && qApp->property("ecuDarkTheme").toBool())
        styleAnalysis(window);
}
