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
        if(QTableWidgetItem *helpHeader=table->horizontalHeaderItem(1))
            helpHeader->setText(QString());

        // Keep the existing translated label, simply display its two words
        // on separate lines when possible so the fixed column can stay narrow.
        if(QTableWidgetItem *receivedHeader=table->horizontalHeaderItem(2)) {
            QString text=receivedHeader->text();
            if(!text.contains(QLatin1Char('\n'))) {
                const int split=text.indexOf(QLatin1Char(' '));
                if(split>0)
                    text[split]=QLatin1Char('\n');
            }
            receivedHeader->setText(text);
            receivedHeader->setTextAlignment(Qt::AlignCenter);
        }
        if(QTableWidgetItem *interpretedHeader=table->horizontalHeaderItem(3))
            interpretedHeader->setTextAlignment(Qt::AlignCenter);

        table->setIconSize(QSize(24,18));
        QHeaderView *header=table->horizontalHeader();
        header->setStretchLastSection(false);
        header->setSectionResizeMode(0,QHeaderView::Stretch);
        header->setSectionResizeMode(1,QHeaderView::Fixed);
        header->setSectionResizeMode(2,QHeaderView::Fixed);
        header->setSectionResizeMode(3,QHeaderView::Fixed);
        table->setColumnWidth(1,28);
        table->setColumnWidth(2,54);
        table->setColumnWidth(3,68);

        header->setStyleSheet(QStringLiteral(
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
            if(QTableWidgetItem *received=table->item(row,2))
                received->setTextAlignment(Qt::AlignCenter);
            if(QTableWidgetItem *interpreted=table->item(row,3))
                interpreted->setTextAlignment(Qt::AlignCenter);
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
