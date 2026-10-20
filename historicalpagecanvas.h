#ifndef HISTORICALPAGECANVAS_H
#define HISTORICALPAGECANVAS_H

#include <QScrollArea>
#include <QWidget>
#include <QtGlobal>

// The original ECU tabs use a fixed canvas set up by
// MainWindow::makeFixedTabsScrollable(). This is NOT a new display scale:
// keep the original design readable and let a smaller viewport scroll.
// Modern non-wrapped pages and embedded controls retain their own policy.
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
    // Various visual polishers can reset minWidth to zero during Show/Resize.
    // Restore exactly the declared original 1300px minimum, not a font scale.
    page->setMinimumWidth(qMax(1300,page->minimumWidth()));
    area->setWidgetResizable(false);
    area->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    area->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
}

#endif
