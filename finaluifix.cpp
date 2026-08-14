#include <QAbstractItemView>
#include <QApplication>
#include <QCoreApplication>
#include <QEvent>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QMainWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QTabWidget>
#include <QTableView>
#include <QTableWidget>
#include <QTextEdit>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

namespace {

static QWidget *realPage(QWidget *tab)
{
    if (!tab) return nullptr;
    if (QScrollArea *scroll = qobject_cast<QScrollArea*>(tab)) return scroll->widget();
    return tab;
}

static void syncNavigation(QMainWindow *window)
{
    if (!window) return;
    QTabWidget *tabs = window->findChild<QTabWidget*>(QStringLiteral("Tab_main"));
    QListWidget *nav = window->findChild<QListWidget*>(QStringLiteral("uiRebuildNav"));
    if (!tabs || !nav) return;

    bool rebuild = nav->count() != tabs->count();
    if (!rebuild) {
        for (int i=0; i<tabs->count(); ++i) {
            if (!nav->item(i) || nav->item(i)->text() != tabs->tabText(i).trimmed()) {
                rebuild = true;
                break;
            }
        }
    }

    if (rebuild) {
        const QSignalBlocker blocker(nav);
        nav->clear();
        for (int i=0; i<tabs->count(); ++i)
            nav->addItem(tabs->tabText(i).trimmed());
    }

    {
        const QSignalBlocker blocker(nav);
        nav->setCurrentRow(tabs->currentIndex());
    }
}

static void relaxScrollArea(QScrollArea *scroll)
{
    if (!scroll) return;
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll->setMinimumSize(0,0);
    scroll->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
    scroll->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    if (scroll->widget()) {
        scroll->widget()->setMinimumSize(0,0);
        scroll->widget()->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
        scroll->widget()->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    }
}

static void relaxPage(QWidget *page, int contentHeight)
{
    if (!page) return;
    page->setMinimumSize(0,0);
    page->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
    page->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);

    if (QLayout *layout = page->layout()) {
        const int margin = contentHeight < 620 ? 7 : 10;
        const int spacing = contentHeight < 620 ? 6 : 8;
        layout->setContentsMargins(margin,margin,margin,margin);
        layout->setSpacing(spacing);
    }

    const QList<QGroupBox*> groups = page->findChildren<QGroupBox*>();
    for (QGroupBox *g : groups) {
        g->setMinimumSize(0,0);
        g->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
        g->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    }

    const QList<QTableView*> views = page->findChildren<QTableView*>();
    for (QTableView *v : views) {
        v->setMinimumSize(0,0);
        v->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
        v->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
        if (v->verticalHeader())
            v->verticalHeader()->setDefaultSectionSize(contentHeight < 620 ? 23 : 26);
        if (v->horizontalHeader())
            v->horizontalHeader()->setMinimumHeight(contentHeight < 620 ? 24 : 28);
    }

    const QList<QTableWidget*> tables = page->findChildren<QTableWidget*>();
    for (QTableWidget *v : tables) {
        v->setMinimumSize(0,0);
        v->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
        v->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
        if (v->verticalHeader())
            v->verticalHeader()->setDefaultSectionSize(contentHeight < 620 ? 23 : 26);
        if (v->horizontalHeader())
            v->horizontalHeader()->setMinimumHeight(contentHeight < 620 ? 24 : 28);
    }

    const QList<QPlainTextEdit*> plains = page->findChildren<QPlainTextEdit*>();
    for (QPlainTextEdit *e : plains) {
        e->setMinimumSize(0,0);
        e->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
        e->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    }
    const QList<QTextEdit*> edits = page->findChildren<QTextEdit*>();
    for (QTextEdit *e : edits) {
        e->setMinimumSize(0,0);
        e->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
        e->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    }
}

static void fitOverview(QMainWindow *window, int contentHeight)
{
    QWidget *overview = window ? window->findChild<QWidget*>(QStringLiteral("overview_tab")) : nullptr;
    if (!overview) return;
    relaxPage(overview,contentHeight);

    if (QFrame *heading = overview->findChild<QFrame*>(QStringLiteral("overviewHeading"),Qt::FindDirectChildrenOnly))
        heading->setFixedHeight(contentHeight < 620 ? 36 : 42);

    const QList<QWidget*> direct = overview->findChildren<QWidget*>(QString(),Qt::FindDirectChildrenOnly);
    for (QWidget *w : direct) {
        if (!w || w->objectName()==QStringLiteral("overviewHeading")) continue;
        w->setMinimumSize(0,0);
        w->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
        w->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    }
}

static void fitSettings(QMainWindow *window, int contentHeight)
{
    QWidget *page = window ? window->findChild<QWidget*>(QStringLiteral("emission_tab")) : nullptr;
    if (!page) return;
    relaxPage(page,contentHeight);

    const char *gaugeNames[] = {
        "m_revCounter_exhaust", "e_idle_error", "e_ignition_advance",
        "e_short_term_fuel_trim", "e_lambda"
    };
    for (const char *name : gaugeNames) {
        if (QWidget *g = page->findChild<QWidget*>(QString::fromLatin1(name))) {
            g->setMinimumSize(0,0);
            g->setMaximumHeight(contentHeight < 620 ? 132 : 165);
            g->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
        }
    }

    const char *cardNames[] = {"settingsMetrics","settingsStates","settingsAdjust"};
    for (const char *name : cardNames) {
        if (QWidget *c = page->findChild<QWidget*>(QString::fromLatin1(name))) {
            c->setMinimumSize(0,0);
            c->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
            c->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
        }
    }
}

static void applyRealResponsive(QMainWindow *window)
{
    if (!window) return;
    QTabWidget *tabs = window->findChild<QTabWidget*>(QStringLiteral("Tab_main"));
    if (!tabs) return;

    const int contentHeight = qMax(420,tabs->height());

    tabs->setMinimumSize(0,0);
    tabs->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
    tabs->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);

    for (int i=0; i<tabs->count(); ++i) {
        QWidget *tab = tabs->widget(i);
        if (QScrollArea *scroll = qobject_cast<QScrollArea*>(tab)) relaxScrollArea(scroll);
        relaxPage(realPage(tab),contentHeight);
    }

    const QList<QScrollArea*> allScrolls = window->findChildren<QScrollArea*>();
    for (QScrollArea *scroll : allScrolls) relaxScrollArea(scroll);

    fitOverview(window,contentHeight);
    fitSettings(window,contentHeight);
    syncNavigation(window);
}

class FinalUiFixInstaller : public QObject
{
public:
    explicit FinalUiFixInstaller(QObject *parent=nullptr) : QObject(parent) {}

protected:
    bool eventFilter(QObject *watched,QEvent *event) override
    {
        if (!watched) return QObject::eventFilter(watched,event);
        QMainWindow *window = qobject_cast<QMainWindow*>(watched);
        if (!window || window->objectName()!=QStringLiteral("MainWindow"))
            return QObject::eventFilter(watched,event);

        if (event->type()==QEvent::Show && !window->property("finalUiFixScheduled").toBool()) {
            window->setProperty("finalUiFixScheduled",true);
            QTimer::singleShot(900,window,[window](){ applyRealResponsive(window); });
            QTimer::singleShot(1800,window,[window](){ applyRealResponsive(window); });
            QTimer::singleShot(2800,window,[window](){ applyRealResponsive(window); });
        }
        else if (event->type()==QEvent::Resize && window->property("finalUiFixScheduled").toBool()) {
            QTimer::singleShot(0,window,[window](){ applyRealResponsive(window); });
        }
        return QObject::eventFilter(watched,event);
    }
};

void installFinalUiFix()
{
    QApplication *app = qobject_cast<QApplication*>(QCoreApplication::instance());
    if (!app) return;
    app->installEventFilter(new FinalUiFixInstaller(app));
}

}

Q_COREAPP_STARTUP_FUNCTION(installFinalUiFix)
