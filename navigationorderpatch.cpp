#include "navigationorderpatch.h"

#include <QApplication>
#include <QEvent>
#include <QFrame>
#include <QIcon>
#include <QListWidget>
#include <QMainWindow>
#include <QPainter>
#include <QPixmap>
#include <QPolygonF>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QTabWidget>
#include <QTimer>
#include <QVariant>
#include <QWidget>

#include "i18n.h"

namespace {

static const int kTextRole = Qt::UserRole + 42;
static const int kTabPtrRole = Qt::UserRole + 60;

static QWidget *realPage(QWidget *tab)
{
    if (!tab) return nullptr;
    if (QScrollArea *scroll = qobject_cast<QScrollArea*>(tab)) return scroll->widget();
    return tab;
}

static int rankFromTitle(const QString &title)
{
    const QString t = title.trimmed();
    if (t == I18n::text(1001).trimmed()) return 0;  // Aperçu
    if (t == I18n::text(2001).trimmed()) return 1;  // Réglages
    if (t == I18n::text(4001).trimmed()) return 2;  // Actionneurs
    if (t == I18n::text(3001).trimmed()) return 3;  // Erreurs
    if (t == I18n::text(7013).trimmed()) return 4;  // Diagnostic automatique
    if (t == I18n::text(7018).trimmed()) return 5;  // Analyse
    if (t == I18n::text(7017).trimmed()) return 6;  // Toutes les mesures
    if (t == I18n::text(7012).trimmed()) return 7;  // ECU / ROSCO
    if (t == I18n::text(5001).trimmed()) return 8;  // Toutes les données
    if (t == I18n::text(7152).trimmed()) return 9;  // Base de données
    if (t == I18n::text(6002).trimmed() || t == I18n::text(6003).trimmed()) return 10; // Mode interactif
    return -1;
}

static int fallbackRank(QWidget *tab)
{
    QWidget *page = realPage(tab);
    if (!page) return -1;
    const QString name = page->objectName().toLower();
    const QString cls = QString::fromLatin1(page->metaObject()->className()).toLower();

    if (name == QStringLiteral("overview_tab")) return 0;
    if (name == QStringLiteral("emission_tab")) return 1;
    if (name == QStringLiteral("actuators")) return 2;
    if (name == QStringLiteral("errors")) return 3;
    if (cls.contains(QStringLiteral("diagnostic"))) return 4;
    if (cls.contains(QStringLiteral("analysis"))) return 5;
    if (cls.contains(QStringLiteral("summary"))) return 6;
    if (name == QStringLiteral("raw")) return 8;
    if (name == QStringLiteral("database_tab")) return 9;
    if (name == QStringLiteral("ecu")) return 10;
    return -1;
}

static int rankForTab(QTabWidget *tabs, int index)
{
    if (!tabs || index < 0 || index >= tabs->count()) return -1;
    const int byTitle = rankFromTitle(tabs->tabText(index));
    return byTitle >= 0 ? byTitle : fallbackRank(tabs->widget(index));
}

static QIcon iconForRank(int rank)
{
    QPixmap pm(22, 22);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(QPen(QColor(QStringLiteral("#ff8a1c")), 1.7, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.setBrush(Qt::NoBrush);

    switch (rank)
    {
    case 0: // Aperçu : 4 cadrans
        p.drawRoundedRect(QRectF(3,4,7,6),1.2,1.2); p.drawRoundedRect(QRectF(12,4,7,6),1.2,1.2);
        p.drawRoundedRect(QRectF(3,12,7,6),1.2,1.2); p.drawRoundedRect(QRectF(12,12,7,6),1.2,1.2);
        break;
    case 1: // Réglages : curseurs
        p.drawLine(4,6,18,6); p.drawLine(4,11,18,11); p.drawLine(4,16,18,16);
        p.drawEllipse(QPointF(9,6),2,2); p.drawEllipse(QPointF(14,11),2,2); p.drawEllipse(QPointF(7,16),2,2);
        break;
    case 2: // Actionneurs : engrenage
        p.drawEllipse(QRectF(6,6,10,10)); p.drawEllipse(QPointF(11,11),2.4,2.4);
        p.drawLine(11,3,11,6); p.drawLine(11,16,11,19); p.drawLine(3,11,6,11); p.drawLine(16,11,19,11);
        p.drawLine(5.3,5.3,7.2,7.2); p.drawLine(14.8,14.8,16.7,16.7);
        break;
    case 3: { // Erreurs : triangle
        QPolygonF q; q << QPointF(11,3.5) << QPointF(19,18) << QPointF(3,18);
        p.drawPolygon(q); p.drawLine(11,8,11,13); p.drawPoint(QPointF(11,16));
        break;
    }
    case 4: // Diagnostic : loupe + coche
        p.drawEllipse(QRectF(4,4,11,11)); p.drawLine(14,14,19,19);
        p.drawLine(7,10,9.5,12.5); p.drawLine(9.5,12.5,13,8);
        break;
    case 5: { // Analyse : courbe
        p.drawLine(4,18,4,5); p.drawLine(4,18,19,18);
        QPolygonF q; q << QPointF(5,15) << QPointF(9,10) << QPointF(12,12) << QPointF(18,6);
        p.drawPolyline(q);
        break;
    }
    case 6: // Toutes les mesures : histogramme
        p.drawRoundedRect(QRectF(4,12,3,6),.8,.8); p.drawRoundedRect(QRectF(9.5,8,3,10),.8,.8);
        p.drawRoundedRect(QRectF(15,4,3,14),.8,.8); p.drawLine(3,18.5,19,18.5);
        break;
    case 7: // ECU / ROSCO : liaison
        p.drawRoundedRect(QRectF(3,7,9,8),4,4); p.drawRoundedRect(QRectF(10,7,9,8),4,4); p.drawLine(8,11,14,11);
        break;
    case 8: // Toutes les données : matrice
        p.drawRoundedRect(QRectF(3.5,3.5,15,15),1.5,1.5);
        p.drawLine(8.5,4,8.5,18); p.drawLine(13.5,4,13.5,18);
        p.drawLine(4,8.5,18,8.5); p.drawLine(4,13.5,18,13.5);
        p.drawPoint(QPointF(6,6)); p.drawPoint(QPointF(11,11)); p.drawPoint(QPointF(16,16));
        break;
    case 9: // Base de données : cylindre
        p.drawEllipse(QRectF(4,4,14,5)); p.drawLine(4,6.5,4,16); p.drawLine(18,6.5,18,16);
        p.drawArc(QRectF(4,13.5,14,5),180*16,180*16); p.drawArc(QRectF(4,9,14,5),180*16,180*16);
        break;
    case 10: // Mode interactif : terminal
        p.drawRoundedRect(QRectF(3,4,16,14),2,2);
        p.drawLine(6,8,9,11); p.drawLine(9,11,6,14); p.drawLine(11.5,14,16,14);
        break;
    default:
        p.drawEllipse(QPointF(11,11),7,7);
        break;
    }
    return QIcon(pm);
}

static QWidget *tabFromItem(QListWidgetItem *item)
{
    if (!item) return nullptr;
    const qulonglong raw = item->data(kTabPtrRole).toULongLong();
    return reinterpret_cast<QWidget*>(static_cast<quintptr>(raw));
}

static void installMappedConnections(QMainWindow *window, QTabWidget *tabs, QListWidget *nav)
{
    if (!window || !tabs || !nav || nav->property("finalNavigationMapped").toBool()) return;

    QObject::disconnect(nav, &QListWidget::currentRowChanged, tabs, &QTabWidget::setCurrentIndex);
    QObject::disconnect(tabs, nullptr, nav, nullptr);

    QObject::connect(nav, &QListWidget::currentRowChanged, window, [nav, tabs](int row) {
        if (row < 0 || row >= nav->count()) return;
        QWidget *target = tabFromItem(nav->item(row));
        if (!target) return;
        const int index = tabs->indexOf(target);
        if (index >= 0 && tabs->currentIndex() != index) tabs->setCurrentIndex(index);
    });

    QObject::connect(tabs, &QTabWidget::currentChanged, window, [nav, tabs](int index) {
        if (index < 0 || index >= tabs->count()) return;
        QWidget *current = tabs->widget(index);
        for (int row = 0; row < nav->count(); ++row)
        {
            if (tabFromItem(nav->item(row)) == current)
            {
                const QSignalBlocker blocker(nav);
                nav->setCurrentRow(row);
                break;
            }
        }
    });

    nav->setProperty("finalNavigationMapped", true);
}

static void applyFinalMenu(QMainWindow *window)
{
    if (!window) return;
    QTabWidget *tabs = window->findChild<QTabWidget*>(QStringLiteral("Tab_main"));
    QListWidget *nav = window->findChild<QListWidget*>(QStringLiteral("uiRebuildNav"));
    if (!tabs || !nav || tabs->count() < 11) return;

    QWidget *tabsByRank[11] = {nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr};
    QString titlesByRank[11];

    for (int i = 0; i < tabs->count(); ++i)
    {
        const int rank = rankForTab(tabs, i);
        if (rank < 0 || rank >= 11 || tabsByRank[rank]) continue;
        tabsByRank[rank] = tabs->widget(i);
        titlesByRank[rank] = tabs->tabText(i).trimmed();
    }

    for (int rank = 0; rank < 11; ++rank)
        if (!tabsByRank[rank]) return;

    bool collapsed = false;
    if (QFrame *sidebar = window->findChild<QFrame*>(QStringLiteral("darkSidebar")))
        collapsed = sidebar->property("collapsed").toBool();

    QWidget *current = tabs->currentWidget();
    int currentRow = -1;
    {
        const QSignalBlocker blocker(nav);
        nav->clear();
        nav->setIconSize(QSize(20,20));
        for (int rank = 0; rank < 11; ++rank)
        {
            QListWidgetItem *item = new QListWidgetItem(iconForRank(rank), collapsed ? QString() : titlesByRank[rank], nav);
            item->setData(kTextRole, titlesByRank[rank]);
            item->setData(kTabPtrRole, static_cast<qulonglong>(reinterpret_cast<quintptr>(tabsByRank[rank])));
            item->setToolTip(titlesByRank[rank]);
            if (tabsByRank[rank] == current) currentRow = rank;
        }
        if (currentRow >= 0) nav->setCurrentRow(currentRow);
    }

    installMappedConnections(window, tabs, nav);
    window->setProperty("finalNavigationApplied", true);
}

class FinalNavigationFilter : public QObject
{
public:
    explicit FinalNavigationFilter(QMainWindow *window) : QObject(window), m_window(window) {}

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if (watched == m_window && m_window)
        {
            if (event->type() == QEvent::Resize && m_window->property("finalNavigationApplied").toBool())
                QTimer::singleShot(180, m_window, [this]() { if (m_window) applyFinalMenu(m_window); });
            else if (event->type() == QEvent::LanguageChange)
                QTimer::singleShot(300, m_window, [this]() { if (m_window) applyFinalMenu(m_window); });
        }
        return QObject::eventFilter(watched, event);
    }

private:
    QMainWindow *m_window;
};

}

void installFinalNavigation(QApplication *app, QMainWindow *window)
{
    if (!app || !window || window->property("finalNavigationInstalled").toBool()) return;
    window->setProperty("finalNavigationInstalled", true);
    window->installEventFilter(new FinalNavigationFilter(window));

    // Les anciens modules reconstruisent encore la barre pendant les premières secondes.
    // Ces passages finis, ce gestionnaire devient l'unique source de l'ordre du menu.
    const int delays[] = {80, 1550, 2550, 3750, 4700};
    for (int delay : delays)
        QTimer::singleShot(delay, window, [window]() { applyFinalMenu(window); });
}
