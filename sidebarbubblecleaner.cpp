#include <QApplication>
#include <QCoreApplication>
#include <QEvent>
#include <QHeaderView>
#include <QLabel>
#include <QMainWindow>
#include <QSize>
#include <QTableWidget>
#include <QTimer>
#include <QToolButton>

#include "summarytab.h"

namespace {

static void removeSidebarBubble(QMainWindow *w)
{
    if(!w) return;
    QToolButton *toggle=w->findChild<QToolButton*>(QStringLiteral("darkSidebarToggle"));
    if(!toggle) return;

    // No tooltip here: the control is already explicitly labelled when the
    // sidebar is expanded, so the generic help-bubble manager must not add
    // its speech-bubble icon beside it.
    toggle->setToolTip(QString());
    if(QLabel *bubble=toggle->findChild<QLabel*>(QStringLiteral("_ecuHelpBubble"),Qt::FindDirectChildrenOnly))
        bubble->hide();
}

static void styleSummaryTables(QMainWindow *w)
{
    if(!w) return;
    SummaryTab *summary=w->findChild<SummaryTab*>();
    if(!summary) return;

    const QList<QTableWidget*> tables=summary->findChildren<QTableWidget*>();
    for(QTableWidget *table:tables) {
        if(!table || table->columnCount()<4) continue;

        // Keep the help-icon column but leave its heading empty.
        if(QTableWidgetItem *header=table->horizontalHeaderItem(1))
            header->setText(QString());

        table->setIconSize(QSize(24,18));
        table->horizontalHeader()->setSectionResizeMode(0,QHeaderView::Stretch);
        table->horizontalHeader()->setSectionResizeMode(1,QHeaderView::Fixed);
        table->horizontalHeader()->setSectionResizeMode(2,QHeaderView::Fixed);
        table->horizontalHeader()->setSectionResizeMode(3,QHeaderView::Fixed);
        table->setColumnWidth(1,28);
        table->setColumnWidth(2,72);
        table->setColumnWidth(3,86);

        table->horizontalHeader()->setStyleSheet(QStringLiteral(
            "QHeaderView::section{"
            "background:#141c23;"
            "color:#ff9828;"
            "border:0;"
            "border-right:1px solid #29343e;"
            "border-bottom:2px solid #ff7a00;"
            "padding:4px 5px;"
            "font-weight:700;"
            "}"));

        for(int row=0;row<table->rowCount();++row) {
            if(QTableWidgetItem *bubble=table->item(row,1))
                bubble->setTextAlignment(Qt::AlignCenter);
        }
    }
}

static void applyVisualOnlyPatches(QMainWindow *w)
{
    removeSidebarBubble(w);
    styleSummaryTables(w);
}

class SidebarBubbleCleaner : public QObject
{
public:
    explicit SidebarBubbleCleaner(QObject *parent=nullptr):QObject(parent){}
protected:
    bool eventFilter(QObject *watched,QEvent *event) override
    {
        QMainWindow *w=qobject_cast<QMainWindow*>(watched);
        if(!w || w->objectName()!=QStringLiteral("MainWindow"))
            return QObject::eventFilter(watched,event);

        if(event->type()==QEvent::Show || event->type()==QEvent::Polish) {
            QTimer::singleShot(1900,w,[w](){applyVisualOnlyPatches(w);});
            QTimer::singleShot(2400,w,[w](){applyVisualOnlyPatches(w);});
        } else if(event->type()==QEvent::Resize) {
            QTimer::singleShot(0,w,[w](){applyVisualOnlyPatches(w);});
        }
        return QObject::eventFilter(watched,event);
    }
};

void installSidebarBubbleCleaner()
{
    QApplication *app=qobject_cast<QApplication*>(QCoreApplication::instance());
    if(app) app->installEventFilter(new SidebarBubbleCleaner(app));
}

}

Q_COREAPP_STARTUP_FUNCTION(installSidebarBubbleCleaner)
