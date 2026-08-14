#include <QApplication>
#include <QCoreApplication>
#include <QEvent>
#include <QFrame>
#include <QGroupBox>
#include <QHeaderView>
#include <QLayout>
#include <QMainWindow>
#include <QPushButton>
#include <QSizePolicy>
#include <QTabWidget>
#include <QTableView>
#include <QTableWidget>
#include <QTextEdit>
#include <QPlainTextEdit>
#include <QTimer>
#include <QWidget>

namespace {

static QWidget *realPage(QWidget *tab)
{
    return tab;
}

static void styleTable(QTableView *table)
{
    if(!table) return;
    table->setAlternatingRowColors(true);
    table->setShowGrid(false);
    table->setStyleSheet(QStringLiteral(
        "QTableView,QTableWidget{background:#081015;color:#e2e8ec;alternate-background-color:#0d161c;border:1px solid #2b3741;border-radius:5px;gridline-color:#1e282f;outline:0;}"
        "QTableView::item,QTableWidget::item{padding:4px 7px;border:0;border-bottom:1px solid #172129;}"
        "QTableView::item:hover,QTableWidget::item:hover{background:#121c23;}"
        "QTableView::item:selected,QTableWidget::item:selected{background:#332316;color:#fff;}"
        "QHeaderView::section{background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #1a232a,stop:1 #111920);color:#f0f4f6;border:0;border-right:1px solid #29343e;border-bottom:2px solid #7b4315;padding:6px 8px;font-weight:700;}"));
    if(table->verticalHeader()) table->verticalHeader()->setVisible(false);
    if(table->horizontalHeader()) table->horizontalHeader()->setMinimumHeight(qMax(28,table->fontMetrics().height()+13));
}

static void styleHero(QFrame *frame)
{
    if(!frame) return;
    frame->setStyleSheet(QStringLiteral(
        "QFrame{background:qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #151e24,stop:.7 #10171d,stop:1 #0b1217);"
        "border:1px solid #2d3943;border-left:3px solid #ff7a00;border-radius:6px;}"
        "QLabel{background:transparent;border:0;}"));
}

static void styleGroups(QWidget *page)
{
    if(!page) return;
    for(QGroupBox *g:page->findChildren<QGroupBox*>()) {
        g->setStyleSheet(QStringLiteral(
            "QGroupBox{background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #121a20,stop:1 #0b1217);color:#e8edf0;border:1px solid #2c3842;border-radius:6px;margin-top:15px;font-weight:700;}"
            "QGroupBox::title{subcontrol-origin:margin;left:12px;padding:0 6px;color:#ff9932;}"));
        g->setMinimumSize(0,0);
        g->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
    }
}

static void styleButtons(QWidget *page)
{
    if(!page) return;
    for(QPushButton *b:page->findChildren<QPushButton*>()) {
        b->setMinimumHeight(qMax(29,b->fontMetrics().height()+13));
        b->setStyleSheet(QStringLiteral(
            "QPushButton{background:#141d24;color:#edf2f5;border:1px solid #35414b;border-radius:5px;padding:5px 11px;font-weight:700;}"
            "QPushButton:hover{background:#202a31;border-color:#ff7a00;color:white;}"
            "QPushButton:pressed{background:#0c1318;}"
            "QPushButton:checked{background:#3a2918;border-color:#ff8a1c;color:#ffa64d;}"
            "QPushButton:disabled{background:#141b20;color:#63707a;border-color:#27313a;}"));
    }
}

static void styleTextAreas(QWidget *page)
{
    if(!page) return;
    const QString qss=QStringLiteral("background:#070d11;color:#dce5ea;border:1px solid #2c3842;border-radius:5px;padding:7px;selection-background-color:#6f3b12;");
    for(QTextEdit *e:page->findChildren<QTextEdit*>()) e->setStyleSheet(qss);
    for(QPlainTextEdit *e:page->findChildren<QPlainTextEdit*>()) e->setStyleSheet(qss);
}

static void styleAnalysis(QMainWindow *window)
{
    if(!window) return;
    QWidget *analysis=nullptr;
    for(QWidget *p:window->findChildren<QWidget*>())
        if(QString::fromLatin1(p->metaObject()->className())==QStringLiteral("AnalysisTab")){analysis=p;break;}
    if(!analysis) return;
    if(QWidget *top=analysis->findChild<QWidget*>(QStringLiteral("analysisTopBar")))
        top->setStyleSheet(QStringLiteral("#analysisTopBar{background:qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #141d23,stop:1 #0d1419);border:1px solid #2c3842;border-radius:6px;}"));
    if(QWidget *right=analysis->findChild<QWidget*>(QStringLiteral("analysisRightPanel")))
        right->setStyleSheet(QStringLiteral("#analysisRightPanel{background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #121a20,stop:1 #0a1116);border:1px solid #2c3842;border-radius:6px;}"));
    styleGroups(analysis); styleButtons(analysis); styleTextAreas(analysis);
}

static void apply(QMainWindow *window)
{
    if(!window) return;
    QTabWidget *tabs=window->findChild<QTabWidget*>(QStringLiteral("Tab_main"));
    if(tabs) {
        for(int i=0;i<tabs->count();++i) {
            QWidget *page=realPage(tabs->widget(i));
            if(!page) continue;
            const bool dedicated=page->property("strictSummaryBuilt").toBool() || page->property("strictRawBuilt").toBool() ||
                page->property("strictInteractiveBuilt").toBool() || page->property("strictRoscoBuilt").toBool() || page->property("strictDiagnosticBuilt").toBool();
            if(!dedicated) continue;
            for(QFrame *f:page->findChildren<QFrame*>())
                if(f->objectName().startsWith(QStringLiteral("strictHero_"))) styleHero(f);
            for(QTableView *t:page->findChildren<QTableView*>()) styleTable(t);
            styleGroups(page); styleButtons(page); styleTextAreas(page);
        }
    }
    styleAnalysis(window);
}

class DarkProDetailInstaller:public QObject
{
public: explicit DarkProDetailInstaller(QObject *p=nullptr):QObject(p){}
protected:
    bool eventFilter(QObject *watched,QEvent *event) override
    {
        QMainWindow *window=qobject_cast<QMainWindow*>(watched);
        if(!window||window->objectName()!=QStringLiteral("MainWindow")) return QObject::eventFilter(watched,event);
        if((event->type()==QEvent::Show||event->type()==QEvent::Polish)&&!window->property("darkProDetailScheduled").toBool()) {
            window->setProperty("darkProDetailScheduled",true);
            QTimer::singleShot(1020,window,[window](){apply(window);});
        } else if(event->type()==QEvent::Resize&&window->property("darkProDetailScheduled").toBool()) {
            QTimer::singleShot(45,window,[window](){apply(window);});
        }
        return QObject::eventFilter(watched,event);
    }
};

void installDarkProDetail(){QApplication *app=qobject_cast<QApplication*>(QCoreApplication::instance());if(app)app->installEventFilter(new DarkProDetailInstaller(app));}

}

Q_COREAPP_STARTUP_FUNCTION(installDarkProDetail)
