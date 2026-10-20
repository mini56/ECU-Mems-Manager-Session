#include "navigationlightmenu.h"

#include <QApplication>
#include <QListWidget>
#include <QTabWidget>
#include <QWidget>
#include <QDebug>

static bool check(bool pass, const QString &message)
{
    if (!pass) qCritical().noquote() << "LIGHT_NAVIGATION_SELFTEST_FAIL:" << message;
    return pass;
}

static bool testNavigation(const QStringList &names)
{
    QTabWidget tabs;
    QListWidget menu;
    int hiddenCount = 0;
    for (const QString &name : names) {
        QWidget *page = new QWidget(&tabs);
        page->setObjectName(name);
        tabs.addTab(page, name);
        menu.addItem(name);
    }
    if (!check(tabs.count() == 14 && menu.count() == 14,
               QStringLiteral("all fourteen original pages must remain present")))
        return false;
    const int visible = applyLightMenuVisibility(&tabs, &menu);
    if (!check(visible == 11, QStringLiteral("expected eleven visible links")))
        return false;
    for (int i=0; i<tabs.count(); ++i) {
        const bool expected = lightMenuHidesTab(tabs.widget(i));
        if (!check(menu.item(i)->isHidden() == expected,
                   QStringLiteral("incorrect visibility for page %1").arg(names.at(i))))
            return false;
        if (expected) ++hiddenCount;
    }
    if (!check(hiddenCount == 3, QStringLiteral("exactly three hidden links")))
        return false;
    for (int i=0; i<tabs.count(); ++i)
        if (!check(tabs.widget(i) != nullptr,
                   QStringLiteral("page widget was destroyed"))) return false;
    menu.clear();
    for (int i=0; i<tabs.count(); ++i)
        menu.addItem(tabs.tabText(i));
    return check(applyLightMenuVisibility(&tabs,&menu) == 11,
                 QStringLiteral("menu rebuild must not resurrect hidden links"));
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    const QStringList official = {
        "overview_tab", "injection_tab", "emission_tab", "actuators",
        "errors", "diagnostic", "ia_mems_tab", "analysis",
        "measurements", "ecu_rosco_tab", "raw", "database_tab",
        "ECU", "mems19_test_tab"
    };
    if (!testNavigation(official)) return 1;

    // Recreating the sidebar in a different tab order must still hide by
    // stable page identity, never by fragile translated label or row index.
    QStringList shuffled = official;
    shuffled.swapItemsAt(1,6);
    shuffled.swapItemsAt(3,9);
    shuffled.swapItemsAt(4,13);
    if (!testNavigation(shuffled)) return 1;
    qInfo() << "LIGHT_NAVIGATION_SELFTEST_PASS"
            << "visible_links=11 hidden_links=3 preserved_pages=14";
    return 0;
}
