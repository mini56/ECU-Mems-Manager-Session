#include <QAbstractScrollArea>
#include <QApplication>
#include <QCoreApplication>
#include <QEvent>
#include <QPointer>
#include <QScrollBar>
#include <QTableWidget>
#include <QTextBrowser>
#include <QTimer>
#include <QWidget>

namespace {

void styleVerticalBar(QScrollBar *bar)
{
    if(!bar) return;
    bar->setFixedWidth(16);
    bar->setStyleSheet(QStringLiteral(
        "QScrollBar:vertical{background:#111a21;width:16px;margin:0;border-left:1px solid #53616c;}"
        "QScrollBar::handle:vertical{background:#7c8b96;min-height:38px;border:1px solid #a0abb3;border-radius:5px;margin:2px;}"
        "QScrollBar::handle:vertical:hover{background:#ff8a22;border-color:#ffad5c;}"
        "QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical{height:0;}"
        "QScrollBar::add-page:vertical,QScrollBar::sub-page:vertical{background:#111a21;}"));
    bar->show();
}

void styleResultScroll(QWidget *browser)
{
    if(!browser) return;
    if(QTableWidget *table=browser->findChild<QTableWidget*>()){
        table->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        styleVerticalBar(table->verticalScrollBar());
    }
    const QList<QTextBrowser*> documents=browser->findChildren<QTextBrowser*>();
    for(QTextBrowser *view:documents){
        if(!view) continue;
        view->setSizeAdjustPolicy(QAbstractScrollArea::AdjustIgnored);
        view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        view->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        view->setFocusPolicy(Qt::StrongFocus);
        QScrollBar *bar=view->verticalScrollBar();
        if(!bar) continue;
        bar->setSingleStep(28);
        const int page=view->viewport()?view->viewport()->height()-40:0;
        bar->setPageStep(page>64?page:64);
        styleVerticalBar(bar);
    }
}

class ResultScrollInstaller : public QObject
{
public:
    explicit ResultScrollInstaller(QObject *parent=nullptr):QObject(parent){}
    bool eventFilter(QObject *watched,QEvent *event) override
    {
        QWidget *widget=qobject_cast<QWidget*>(watched);
        if(widget && event && (event->type()==QEvent::Show || event->type()==QEvent::Polish) &&
           widget->objectName()==QStringLiteral("memsDatabaseBrowser")){
            QPointer<QWidget> guarded(widget);
            QTimer::singleShot(100,this,[guarded](){if(guarded) styleResultScroll(guarded);});
        }
        return QObject::eventFilter(watched,event);
    }
};

void installResultScroll()
{
    QCoreApplication *core=QCoreApplication::instance();
    if(!core) return;
    ResultScrollInstaller *installer=new ResultScrollInstaller(core);
    core->installEventFilter(installer);
    QTimer::singleShot(300,installer,[installer](){
        for(QWidget *widget:QApplication::allWidgets())
            if(widget && widget->objectName()==QStringLiteral("memsDatabaseBrowser")) styleResultScroll(widget);
    });
}

}

Q_COREAPP_STARTUP_FUNCTION(installResultScroll)
