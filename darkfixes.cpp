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