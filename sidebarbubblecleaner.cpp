#include <QApplication>
#include <QCoreApplication>
#include <QEvent>
#include <QHeaderView>
#include <QLabel>
#include <QListWidget>
#include <QMainWindow>
#include <QPointer>
#include <QSize>
#include <QTableWidget>
#include <QTimer>
#include <QToolButton>

#include "summarytab.h"

namespace {

static void removeSidebarBubble(QMainWindow *window)
{
    if (!window)
        return;

    QToolButton *toggle = window->findChild<QToolButton*>(QStringLiteral("darkSidebarToggle"));
    if (!toggle)
        return;

    // No tooltip here: the control is already explicitly labelled when the
    // sidebar is expanded, so the generic help-bubble manager must not add
    // its speech-bubble icon beside it.
    toggle->setToolTip(QString());
    if (QLabel *bubble = toggle->findChild<QLabel*>(QStringLiteral("_ecuHelpBubble"), Qt::FindDirectChildrenOnly))
        bubble->hide();
}

static void styleSidebarTooltip(QListWidget *nav)
{
    if (!nav || nav->objectName() != QStringLiteral("uiRebuildNav"))
        return;

    const QString tooltipRule = QStringLiteral("QToolTip{color:#ffffff;}");
    if (!nav->styleSheet().contains(tooltipRule))
        nav->setStyleSheet(nav->styleSheet() + tooltipRule);
}

static bool isSummaryTable(QTableWidget *table)
{
    if (!table)
        return false;

    for (QObject *parent = table->parent(); parent; parent = parent->parent())
    {
        if (qobject_cast<SummaryTab*>(parent))
            return true;
    }
    return false;
}

static void sizeSummaryTable(QTableWidget *table)
{
    if (!table || table->columnCount() < 4)
        return;

    const int helpWidth = 28;
    const int receivedWidth = 54;
    const int interpretedWidth = 82;

    QHeaderView *header = table->horizontalHeader();
    header->setStretchLastSection(false);
    header->setSectionResizeMode(0, QHeaderView::Fixed);
    header->setSectionResizeMode(1, QHeaderView::Fixed);
    header->setSectionResizeMode(2, QHeaderView::Fixed);
    header->setSectionResizeMode(3, QHeaderView::Fixed);

    // Only Parameter follows the available table width. The three columns to
    // its right always keep the same width, whatever the language or sidebar.
    const int available = qMax(0, table->viewport()->width() - 2);
    const int parameterWidth = qMax(80, available - helpWidth - receivedWidth - interpretedWidth);

    header->resizeSection(0, parameterWidth);
    header->resizeSection(1, helpWidth);
    header->resizeSection(2, receivedWidth);
    header->resizeSection(3, interpretedWidth);
}

static void styleSummaryTables(QMainWindow *window)
{
    if (!window)
        return;

    SummaryTab *summary = window->findChild<SummaryTab*>();
    if (!summary)
        return;

    const QList<QTableWidget*> tables = summary->findChildren<QTableWidget*>();
    for (QTableWidget *table : tables)
    {
        if (!table || table->columnCount() < 4)
            continue;

        if (QTableWidgetItem *helpHeader = table->horizontalHeaderItem(1))
            helpHeader->setText(QString());

        if (QTableWidgetItem *receivedHeader = table->horizontalHeaderItem(2))
        {
            QString text = receivedHeader->text();
            if (!text.contains(QLatin1Char('\n')))
            {
                const int split = text.indexOf(QLatin1Char(' '));
                if (split > 0)
                    text[split] = QLatin1Char('\n');
            }
            receivedHeader->setText(text);
            receivedHeader->setTextAlignment(Qt::AlignCenter);
        }
        if (QTableWidgetItem *interpretedHeader = table->horizontalHeaderItem(3))
            interpretedHeader->setTextAlignment(Qt::AlignCenter);

        table->setIconSize(QSize(24, 18));
        sizeSummaryTable(table);

        QHeaderView *header = table->horizontalHeader();
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

        for (int row = 0; row < table->rowCount(); ++row)
        {
            if (QTableWidgetItem *bubble = table->item(row, 1))
                bubble->setTextAlignment(Qt::AlignCenter);
            if (QTableWidgetItem *received = table->item(row, 2))
                received->setTextAlignment(Qt::AlignCenter);
            if (QTableWidgetItem *interpreted = table->item(row, 3))
                interpreted->setTextAlignment(Qt::AlignCenter);
        }
    }
}

static void applyVisualOnlyPatches(QMainWindow *window)
{
    removeSidebarBubble(window);
    if (QListWidget *nav = window ? window->findChild<QListWidget*>(QStringLiteral("uiRebuildNav")) : nullptr)
        styleSidebarTooltip(nav);
    styleSummaryTables(window);
}

class SidebarBubbleCleaner : public QObject
{
public:
    explicit SidebarBubbleCleaner(QApplication *app)
        : QObject(app), m_app(app)
    {
    }

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if (!event)
            return QObject::eventFilter(watched, event);

        if (!m_window)
        {
            QMainWindow *candidate = qobject_cast<QMainWindow*>(watched);
            if (!candidate || candidate->objectName() != QStringLiteral("MainWindow"))
                return QObject::eventFilter(watched, event);

            if (event->type() != QEvent::Show && event->type() != QEvent::Polish)
                return QObject::eventFilter(watched, event);

            m_window = candidate;
            candidate->installEventFilter(this);
            if (m_app)
                m_app->removeEventFilter(this);

            scheduleApply(1900);
            scheduleApply(2400);
            return QObject::eventFilter(watched, event);
        }

        if (QListWidget *nav = qobject_cast<QListWidget*>(watched))
        {
            if (nav->objectName() == QStringLiteral("uiRebuildNav") &&
                (event->type() == QEvent::Show || event->type() == QEvent::StyleChange))
            {
                QTimer::singleShot(0, nav, [nav]() { styleSidebarTooltip(nav); });
            }
            return QObject::eventFilter(watched, event);
        }

        if (QTableWidget *table = qobject_cast<QTableWidget*>(watched))
        {
            if (isSummaryTable(table) && event->type() == QEvent::Resize)
                QTimer::singleShot(0, table, [table]() { sizeSummaryTable(table); });
            return QObject::eventFilter(watched, event);
        }

        if (watched == m_window &&
            (event->type() == QEvent::Resize || event->type() == QEvent::LayoutRequest ||
             event->type() == QEvent::Show || event->type() == QEvent::Polish))
        {
            QTimer::singleShot(0, m_window, [this]() {
                if (m_window)
                {
                    applyVisualOnlyPatches(m_window);
                    installChildFilters();
                }
            });
        }

        return QObject::eventFilter(watched, event);
    }

private:
    void scheduleApply(int delay)
    {
        if (!m_window)
            return;

        QTimer::singleShot(delay, m_window, [this]() {
            if (!m_window)
                return;
            applyVisualOnlyPatches(m_window);
            installChildFilters();
        });
    }

    void installChildFilters()
    {
        if (!m_window)
            return;

        if (QListWidget *nav = m_window->findChild<QListWidget*>(QStringLiteral("uiRebuildNav")))
        {
            if (!nav->property("sidebarCleanerFilterInstalled").toBool())
            {
                nav->setProperty("sidebarCleanerFilterInstalled", true);
                nav->installEventFilter(this);
            }
        }

        SummaryTab *summary = m_window->findChild<SummaryTab*>();
        if (!summary)
            return;

        const QList<QTableWidget*> tables = summary->findChildren<QTableWidget*>();
        for (QTableWidget *table : tables)
        {
            if (!table || table->property("sidebarCleanerFilterInstalled").toBool())
                continue;
            table->setProperty("sidebarCleanerFilterInstalled", true);
            table->installEventFilter(this);
        }
    }

    QApplication *m_app;
    QPointer<QMainWindow> m_window;
};

void installSidebarBubbleCleaner()
{
    QApplication *app = qobject_cast<QApplication*>(QCoreApplication::instance());
    if (app)
        app->installEventFilter(new SidebarBubbleCleaner(app));
}

}

Q_COREAPP_STARTUP_FUNCTION(installSidebarBubbleCleaner)
