#ifndef NAVIGATIONLIGHTMENU_H
#define NAVIGATIONLIGHTMENU_H

#include <QListWidget>
#include <QScrollArea>
#include <QString>
#include <QTabWidget>
#include <QWidget>

// LIGHT is a navigation-only presentation choice. Never remove or delete
// the original pages: their tab indices and internal implementations stay put.
inline bool lightMenuHidesTab(QWidget *tab)
{
    if (!tab) return false;
    QWidget *page = tab;
    if (QScrollArea *scroll = qobject_cast<QScrollArea*>(tab))
        page = scroll->widget();
    if (!page) return false;

    const QString identity = page->objectName();
    return identity == QStringLiteral("ia_mems_tab")
        || identity == QStringLiteral("ecu_rosco_tab")
        || identity == QStringLiteral("mems19_test_tab");
}

inline int applyLightMenuVisibility(QTabWidget *tabs, QListWidget *nav)
{
    if (!tabs || !nav || nav->count() != tabs->count())
        return -1; // Fail explicitly if menu and tab indices are not aligned.

    int visible = 0;
    for (int i = 0; i < tabs->count(); ++i)
    {
        const bool hidden = lightMenuHidesTab(tabs->widget(i));
        nav->item(i)->setHidden(hidden);
        if (!hidden) ++visible;
    }

    // Do not leave the sidebar selection on an entry that is hidden.
    const int selected = tabs->currentIndex();
    if (selected >= 0 && selected < nav->count()
        && lightMenuHidesTab(tabs->widget(selected)))
        nav->setCurrentRow(0);
    return visible;
}

#endif // NAVIGATIONLIGHTMENU_H
