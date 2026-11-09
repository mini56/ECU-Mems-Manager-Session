#include "navigationorderpatch.h"

#include <QApplication>
#include <QDebug>
#include <QEvent>
#include <QGridLayout>
#include <QGroupBox>
#include <QIcon>
#include <QLabel>
#include <QListWidget>
#include <QMainWindow>
#include <QPainter>
#include <QPixmap>
#include <QPolygonF>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QTabWidget>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>

#include "analysistab.h"
#include "database/MemsDatabaseBrowser.h"
#include "diagnosticpanel.h"
#include "i18n.h"
#include "iamemstab.h"
#include "mainwindow.h"
#include "mems19testtab.h"
#include "summarytab.h"

namespace {

static const int kFinalTabCount = 14;

struct TabSpec
{
    const char *identity;
    int translationKey;
};

static const TabSpec kTabs[kFinalTabCount] = {
    {"overview",     1001},
    {"injection",    7900},
    {"settings",     2001},
    {"actuators",    4001},
    {"errors",       3001},
    {"diagnostic",   7013},
    {"ia_mems",      7940},
    {"analysis",     7018},
    {"measurements", 7017},
    {"ecu_rosco",    7012},
    {"raw",          5001},
    {"database",     7152},
    {"interactive",  6002},
    {"mems19_test", 19000}
};

static QWidget *realPage(QWidget *tab)
{
    if (!tab)
        return nullptr;
    if (QScrollArea *scroll = qobject_cast<QScrollArea*>(tab))
        return scroll->widget();
    return tab;
}

static QString translatedTitle(int rank)
{
    if (rank < 0 || rank >= kFinalTabCount)
        return QString();

    QString title = I18n::text(kTabs[rank].translationKey).trimmed();
    if (rank == 6 && (title.isEmpty() || title.startsWith(QLatin1Char('['))))
        title = QStringLiteral("IA MEMS");
    if (rank == 13 && (title.isEmpty() || title.startsWith(QLatin1Char('['))))
        title = QStringLiteral("Test ECU 1.9");
    return title;
}

static QIcon iconForRank(int rank)
{
    QPixmap pm(22, 22);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(QPen(QColor(QStringLiteral("#ff8a1c")), 1.7,
                  Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.setBrush(Qt::NoBrush);

    switch (rank)
    {
    case 0:
        p.drawRoundedRect(QRectF(3,4,7,6),1.2,1.2);
        p.drawRoundedRect(QRectF(12,4,7,6),1.2,1.2);
        p.drawRoundedRect(QRectF(3,12,7,6),1.2,1.2);
        p.drawRoundedRect(QRectF(12,12,7,6),1.2,1.2);
        break;
    case 1:
        p.drawLine(5,17,16,6); p.drawLine(7,19,18,8);
        p.drawLine(13,5,19,11); p.drawLine(4,14,9,19);
        p.drawLine(4,18,2.5,19.5);
        break;
    case 2:
        p.drawLine(4,6,18,6); p.drawLine(4,11,18,11); p.drawLine(4,16,18,16);
        p.drawEllipse(QPointF(9,6),2,2); p.drawEllipse(QPointF(14,11),2,2); p.drawEllipse(QPointF(7,16),2,2);
        break;
    case 3:
        p.drawEllipse(QRectF(6,6,10,10)); p.drawEllipse(QPointF(11,11),2.4,2.4);
        p.drawLine(11,3,11,6); p.drawLine(11,16,11,19); p.drawLine(3,11,6,11); p.drawLine(16,11,19,11);
        break;
    case 4: {
        QPolygonF q; q << QPointF(11,3.5) << QPointF(19,18) << QPointF(3,18);
        p.drawPolygon(q); p.drawLine(11,8,11,13); p.drawPoint(QPointF(11,16));
        break;
    }
    case 5:
        p.drawEllipse(QRectF(4,4,11,11)); p.drawLine(14,14,19,19);
        p.drawLine(7,10,9.5,12.5); p.drawLine(9.5,12.5,13,8);
        break;
    case 6:
        p.drawRoundedRect(QRectF(3,4,16,11),3,3);
        p.drawLine(QPointF(8,15),QPointF(6,19)); p.drawLine(QPointF(8,15),QPointF(12,15));
        p.drawEllipse(QPointF(8,9.5),0.7,0.7); p.drawEllipse(QPointF(11,9.5),0.7,0.7); p.drawEllipse(QPointF(14,9.5),0.7,0.7);
        break;
    case 7: {
        p.drawLine(4,18,4,5); p.drawLine(4,18,19,18);
        QPolygonF q; q << QPointF(5,15) << QPointF(9,10) << QPointF(12,12) << QPointF(18,6);
        p.drawPolyline(q);
        break;
    }
    case 8:
        p.drawRoundedRect(QRectF(4,12,3,6),.8,.8); p.drawRoundedRect(QRectF(9.5,8,3,10),.8,.8);
        p.drawRoundedRect(QRectF(15,4,3,14),.8,.8); p.drawLine(3,18.5,19,18.5);
        break;
    case 9:
        p.drawRoundedRect(QRectF(3,7,9,8),4,4); p.drawRoundedRect(QRectF(10,7,9,8),4,4); p.drawLine(8,11,14,11);
        break;
    case 10:
        p.drawRoundedRect(QRectF(3.5,3.5,15,15),1.5,1.5);
        p.drawLine(8.5,4,8.5,18); p.drawLine(13.5,4,13.5,18); p.drawLine(4,8.5,18,8.5); p.drawLine(4,13.5,18,13.5);
        break;
    case 11:
        p.drawEllipse(QRectF(4,4,14,5)); p.drawLine(4,6.5,4,16); p.drawLine(18,6.5,18,16);
        p.drawArc(QRectF(4,13.5,14,5),180*16,180*16); p.drawArc(QRectF(4,9,14,5),180*16,180*16);
        break;
    case 12:
        p.drawRoundedRect(QRectF(3,4,16,14),2,2); p.drawLine(6,8,9,11); p.drawLine(9,11,6,14); p.drawLine(11.5,14,16,14);
        break;
    case 13:
        p.drawRoundedRect(QRectF(3,5,10,12),2,2); p.drawLine(13,9,18,9); p.drawLine(13,13,18,13); p.drawLine(18,7,18,15);
        p.drawLine(5,8,10,8); p.drawLine(5,11,10,11); p.drawLine(5,14,10,14);
        break;
    default:
        p.drawEllipse(QPointF(11,11),7,7);
        break;
    }
    return QIcon(pm);
}

static bool isReadOnlyProtocolPage(QWidget *page)
{
    if (!page || !page->objectName().isEmpty())
        return false;
    if (QString::fromLatin1(page->metaObject()->className()) != QStringLiteral("QWidget"))
        return false;
    if (!page->findChild<QGroupBox*>())
        return false;
    const QList<QTextEdit*> edits = page->findChildren<QTextEdit*>();
    for (QTextEdit *edit : edits)
        if (edit && edit->isReadOnly())
            return true;
    return false;
}

static int rankForTab(QWidget *tab)
{
    QWidget *page = realPage(tab);
    if (!page)
        return -1;

    QString name = page->objectName();
    if (name.isEmpty() && isReadOnlyProtocolPage(page))
    {
        page->setObjectName(QStringLiteral("ecu_rosco_tab"));
        name = page->objectName();
    }

    if (name == QStringLiteral("overview_tab")) return 0;
    if (name == QStringLiteral("injection_tab")) return 1;
    if (name == QStringLiteral("emission_tab")) return 2;
    if (name == QStringLiteral("actuators")) return 3;
    if (name == QStringLiteral("errors")) return 4;
    if (name == QStringLiteral("ia_mems_tab")) return 6;
    if (name == QStringLiteral("ecu_rosco_tab")) return 9;
    if (name == QStringLiteral("raw")) return 10;
    if (name == QStringLiteral("database_tab")) return 11;
    if (name == QStringLiteral("ECU")) return 12;
    if (name == QStringLiteral("mems19_test_tab")) return 13;

    if (qobject_cast<DiagnosticPanel*>(page)) return 5;
    if (qobject_cast<AnalysisTab*>(page)) return 7;
    if (qobject_cast<SummaryTab*>(page)) return 8;
    return -1;
}

static QWidget *findTabByRank(QTabWidget *tabs, int rank)
{
    if (!tabs)
        return nullptr;
    for (int i = 0; i < tabs->count(); ++i)
        if (rankForTab(tabs->widget(i)) == rank)
            return tabs->widget(i);
    return nullptr;
}

static void buildDatabasePage(QWidget *database)
{
    if (!database)
        return;
    if (database->findChild<MemsDatabaseBrowser*>(QStringLiteral("memsDatabaseBrowser"), Qt::FindDirectChildrenOnly))
        return;

    database->setObjectName(QStringLiteral("database_tab"));
    database->setMinimumSize(0,0);
    database->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
    database->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    database->setAttribute(Qt::WA_StyledBackground,true);
    database->setStyleSheet(QStringLiteral("#database_tab{background:#090e13;}"));

    QVBoxLayout *root = qobject_cast<QVBoxLayout*>(database->layout());
    if (!root)
    {
        root = new QVBoxLayout(database);
        root->setContentsMargins(0,0,0,0);
        root->setSpacing(0);
    }

    MemsDatabaseBrowser *browser = new MemsDatabaseBrowser(database);
    browser->setObjectName(QStringLiteral("memsDatabaseBrowser"));
    browser->setMinimumSize(0,0);
    browser->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
    browser->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    root->addWidget(browser,1);
}

static void ensureDynamicPages(MainWindow *window, QTabWidget *tabs)
{
    if (!window || !tabs)
        return;

    if (!findTabByRank(tabs, 6) && window->memsInterface())
    {
        IaMemsTab *ia = new IaMemsTab(window, tabs);
        ia->setObjectName(QStringLiteral("ia_mems_tab"));
        tabs->addTab(ia, translatedTitle(6));
    }

    if (!findTabByRank(tabs, 11))
    {
        QWidget *database = new QWidget(tabs);
        buildDatabasePage(database);
        tabs->addTab(database, translatedTitle(11));
    }
    else
    {
        buildDatabasePage(realPage(findTabByRank(tabs, 11)));
    }

    if (!findTabByRank(tabs, 13) && window->memsInterface())
    {
        Mems19TestTab *test = new Mems19TestTab(window->memsInterface(), tabs);
        test->setObjectName(QStringLiteral("mems19_test_tab"));
        tabs->addTab(test, translatedTitle(13));
    }
}

static void fitNavigationRows(QListWidget *nav)
{
    if (!nav || nav->count() <= 0)
        return;

    const int viewportHeight = nav->viewport() ? nav->viewport()->height() : nav->height();
    if (viewportHeight <= 0)
        return;

    // On smaller screens the original 14 rows can exceed the viewport by only
    // a few pixels. Compress the row height just enough to keep every official
    // tab visible; never solve this by scrolling the first tab out of view.
    const int rowHeight = qBound(21, viewportHeight / kFinalTabCount, 26);
    nav->setIconSize(QSize(qMin(20, rowHeight - 2), qMin(20, rowHeight - 2)));
    nav->setSpacing(0);
    nav->setUniformItemSizes(true);
    nav->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    nav->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    for (int row = 0; row < nav->count(); ++row) {
        if (QListWidgetItem *item = nav->item(row))
            item->setSizeHint(QSize(0, rowHeight));
    }
    nav->scrollToTop();
}

static void fitRawSpacing(QMainWindow *window)
{
    if (!window)
        return;
    QTabWidget *tabs = window->findChild<QTabWidget*>(QStringLiteral("Tab_main"));
    if (!tabs)
        return;

    QWidget *page = realPage(findTabByRank(tabs, 10));
    if (!page)
        return;
    QWidget *left = page->findChild<QWidget*>(QStringLiteral("raw_1"));
    QWidget *right = page->findChild<QWidget*>(QStringLiteral("raw_2"));
    if (!left || !right)
        return;

    QGridLayout *leftGrid = qobject_cast<QGridLayout*>(left->layout());
    QGridLayout *rightGrid = qobject_cast<QGridLayout*>(right->layout());
    const int rows = qMax(leftGrid ? leftGrid->rowCount() : 1,
                          rightGrid ? rightGrid->rowCount() : 1);
    if (rows <= 0)
        return;

    const int pageHeight = qMax(page->height(), tabs->height());
    const bool wrapped = left->parentWidget() && left->parentWidget() != page;
    const int reserve = wrapped ? 48 : 20;
    const int usableHeight = qMax(320, pageHeight - reserve);
    const int rowHeight = qBound(13, usableHeight / rows, 18);
    const int blockHeight = rowHeight * rows;

    const auto fitBlock = [page,rowHeight,blockHeight](QWidget *block,QGridLayout *grid) {
        if (!block || !grid)
            return;
        grid->setContentsMargins(0,0,0,0);
        grid->setHorizontalSpacing(8);
        grid->setVerticalSpacing(0);
        grid->setSizeConstraint(QLayout::SetDefaultConstraint);
        for (int r = 0; r < grid->rowCount(); ++r)
            grid->setRowMinimumHeight(r,rowHeight);
        block->setMinimumHeight(blockHeight);
        block->setMaximumHeight(blockHeight);
        block->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
        QWidget *container = block->parentWidget();
        if (container && container != page)
        {
            container->setMinimumHeight(blockHeight + 42);
            container->setMaximumHeight(QWIDGETSIZE_MAX);
            container->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
        }
        else if (container == page)
        {
            const int y = qMax(6,block->y());
            block->setGeometry(block->x(),y,block->width(),blockHeight);
        }
        for (QLabel *label : block->findChildren<QLabel*>())
        {
            label->setWordWrap(false);
            label->setMinimumHeight(rowHeight);
            label->setMaximumHeight(rowHeight);
            label->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Fixed);
        }
    };

    fitBlock(left,leftGrid);
    fitBlock(right,rightGrid);
}

static void installMappedConnections(QMainWindow *window, QTabWidget *tabs, QListWidget *nav)
{
    if (!window || !tabs || !nav || nav->property("deterministicNavigationMapped").toBool())
        return;

    QObject::disconnect(nav, &QListWidget::currentRowChanged,
                        tabs, &QTabWidget::setCurrentIndex);
    QObject::disconnect(tabs, nullptr, nav, nullptr);

    QObject::connect(nav, &QListWidget::currentRowChanged, window,
                     [tabs](int row) {
        if (row >= 0 && row < tabs->count() && tabs->currentIndex() != row)
            tabs->setCurrentIndex(row);
    });

    QObject::connect(tabs, &QTabWidget::currentChanged, window,
                     [nav](int index) {
        if (index < 0 || index >= nav->count())
            return;
        const QSignalBlocker blocker(nav);
        nav->setCurrentRow(index);
        fitNavigationRows(nav);
    });

    nav->setProperty("deterministicNavigationMapped", true);
}

static bool collectOfficialTabs(QTabWidget *tabs, QWidget *official[kFinalTabCount])
{
    for (int rank = 0; rank < kFinalTabCount; ++rank)
        official[rank] = nullptr;

    bool duplicate = false;
    for (int i = 0; i < tabs->count(); ++i)
    {
        QWidget *tab = tabs->widget(i);
        const int rank = rankForTab(tab);
        if (rank < 0 || rank >= kFinalTabCount)
            continue;
        if (official[rank] && official[rank] != tab)
        {
            qWarning() << "MEMSX64 navigation duplicate identity" << kTabs[rank].identity;
            duplicate = true;
            continue;
        }
        official[rank] = tab;
    }

    if (duplicate)
        return false;

    for (int rank = 0; rank < kFinalTabCount; ++rank)
    {
        if (!official[rank])
        {
            qWarning() << "MEMSX64 navigation missing identity" << kTabs[rank].identity;
            return false;
        }
    }
    return true;
}

static bool applyDeterministicNavigation(MainWindow *window)
{
    if (!window)
        return false;
    QTabWidget *tabs = window->findChild<QTabWidget*>(QStringLiteral("Tab_main"));
    QListWidget *nav = window->findChild<QListWidget*>(QStringLiteral("uiRebuildNav"));
    if (!tabs || !nav)
        return false;

    ensureDynamicPages(window,tabs);

    QWidget *official[kFinalTabCount];
    if (!collectOfficialTabs(tabs, official))
        return false;

    QWidget *current = tabs->currentWidget();
    {
        const QSignalBlocker blocker(tabs);
        for (int rank = 0; rank < kFinalTabCount; ++rank)
        {
            const int oldIndex = tabs->indexOf(official[rank]);
            if (oldIndex >= 0)
                tabs->removeTab(oldIndex);
        }
        for (int rank = 0; rank < kFinalTabCount; ++rank)
            tabs->insertTab(rank, official[rank], iconForRank(rank), translatedTitle(rank));

        if (current && tabs->indexOf(current) >= 0)
            tabs->setCurrentWidget(current);
        else
            tabs->setCurrentIndex(0);
    }

    {
        const QSignalBlocker blocker(nav);
        nav->clear();
        for (int rank = 0; rank < kFinalTabCount; ++rank)
        {
            const QString title = translatedTitle(rank);
            QListWidgetItem *item = new QListWidgetItem(iconForRank(rank), title, nav);
            item->setToolTip(title);
        }
        nav->setCurrentRow(qBound(0, tabs->currentIndex(), kFinalTabCount - 1));
        fitNavigationRows(nav);
    }

    installMappedConnections(window,tabs,nav);
    fitNavigationRows(nav);
    fitRawSpacing(window);
    window->setProperty("deterministicNavigationApplied", true);
    window->setProperty("deterministicNavigationCount", kFinalTabCount);
    return true;
}

class DeterministicNavigationFilter final : public QObject
{
public:
    explicit DeterministicNavigationFilter(MainWindow *window)
        : QObject(window), m_window(window) {}

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if (watched == m_window && m_window && event)
        {
            if (event->type() == QEvent::LanguageChange)
                applyDeterministicNavigation(m_window);
            else if (event->type() == QEvent::Resize &&
                     m_window->property("deterministicNavigationApplied").toBool()) {
                fitRawSpacing(m_window);
                if (QListWidget *nav = m_window->findChild<QListWidget*>(QStringLiteral("uiRebuildNav")))
                    fitNavigationRows(nav);
            }
        }
        return QObject::eventFilter(watched,event);
    }

private:
    MainWindow *m_window = nullptr;
};

} // namespace

void installFinalNavigation(QApplication *app, QMainWindow *window)
{
    MainWindow *mainWindow = qobject_cast<MainWindow*>(window);
    if (!app || !mainWindow || mainWindow->property("deterministicNavigationInstalled").toBool())
        return;

    app->processEvents();

    mainWindow->setProperty("deterministicNavigationInstalled", true);
    mainWindow->installEventFilter(new DeterministicNavigationFilter(mainWindow));
    if (!applyDeterministicNavigation(mainWindow))
        qWarning() << "MEMSX64 deterministic navigation could not resolve all 14 official tabs";
}
