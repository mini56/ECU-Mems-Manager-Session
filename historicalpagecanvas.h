#ifndef HISTORICALPAGECANVAS_H
#define HISTORICALPAGECANVAS_H

#include <QScrollArea>
#include <QFontInfo>
#include <QWidget>
#include <QTabWidget>
#include <QtGlobal>

// Scroll wrappers remain available for legacy content, but they must never
// impose the 1300px designer width on a real user's screen. Layout-based
// pages fill the viewport; taller content scrolls at its natural height.
inline bool hasHistoricalPageCanvas(const QScrollArea *area)
{
    return area && area->widget()
        && area->widget()->property("wrappedInScrollArea").toBool();
}

inline void preserveHistoricalPageCanvas(QScrollArea *area)
{
    if (!hasHistoricalPageCanvas(area))
        return;

    QWidget *page = area->widget();
    page->setMinimumWidth(0);
    // Do not inherit miniature root fonts from legacy Designer pages.
    if (QFontInfo(page->font()).pointSizeF() < 9.0) {
        QFont readable = page->font();
        readable.setPointSizeF(9.0);
        page->setFont(readable);
    }
    if (page->layout())
        page->setMinimumHeight(0);
    area->setWidgetResizable(true);
    area->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    area->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
}

inline int stabilizeHistoricalTabCanvases(QTabWidget *tabs)
{
    if (!tabs) return 0;
    int preserved = 0;
    for (int i = 0; i < tabs->count(); ++i) {
        QScrollArea *area = qobject_cast<QScrollArea*>(tabs->widget(i));
        if (!hasHistoricalPageCanvas(area)) continue;
        preserveHistoricalPageCanvas(area);
        ++preserved;
    }
    return preserved;
}

#endif
