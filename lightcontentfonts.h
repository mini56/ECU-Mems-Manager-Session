#ifndef LIGHTCONTENTFONTS_H
#define LIGHTCONTENTFONTS_H

#include <QAbstractButton>
#include <QAbstractItemView>
#include <QAbstractSpinBox>
#include <QComboBox>
#include <QFont>
#include <QFontInfo>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QScrollArea>
#include <QTabWidget>
#include <QTextEdit>
#include <QWidget>

// LIGHT readability policy: operator-approved "All measurements" and
// "All data" pages stay untouched. Settings and Actuators intentionally use
// 10pt instead of 11pt so buttons and field labels fit without clipping.
// All other pages retain the previously validated minimum of 11pt.
inline bool lightContentFontPageIsProtected(const QWidget *page)
{
    if (!page) return true;
    return page->objectName() == QStringLiteral("raw")
        || QString::fromLatin1(page->metaObject()->className()) == QStringLiteral("SummaryTab");
}

inline bool lightContentFontTarget(const QWidget *widget)
{
    return qobject_cast<const QLabel*>(widget)
        || qobject_cast<const QAbstractButton*>(widget)
        || qobject_cast<const QAbstractItemView*>(widget)
        || qobject_cast<const QLineEdit*>(widget)
        || qobject_cast<const QTextEdit*>(widget)
        || qobject_cast<const QPlainTextEdit*>(widget)
        || qobject_cast<const QComboBox*>(widget)
        || qobject_cast<const QAbstractSpinBox*>(widget)
        || qobject_cast<const QGroupBox*>(widget);
}

inline void applyLightReadableContentFonts(QTabWidget *tabs)
{
    if (!tabs) return;
    constexpr qreal minimumPointSize = 11.0;
    for (int i=0; i<tabs->count(); ++i) {
        QWidget *page=tabs->widget(i);
        if (QScrollArea *area=qobject_cast<QScrollArea*>(page))
            page=area->widget();
        if (!page || lightContentFontPageIsProtected(page)) continue;
        const bool compactControls = page->objectName()==QStringLiteral("emission_tab")
                                  || page->objectName()==QStringLiteral("actuators");
        const qreal targetSize=compactControls ? 10.0 : minimumPointSize;

        // Page content only: the sidebar/chrome, measured gauges, and the two
        // previously approved pages are not changed by this adjustment.
        QList<QWidget*> children=page->findChildren<QWidget*>();
        children.prepend(page);
        for(QWidget *widget : children) {
            if (!widget || !lightContentFontTarget(widget)) continue;
            const qreal points=QFontInfo(widget->font()).pointSizeF();
            // On Settings/Actuators, reduce 11pt controls by only 1pt.
            // Elsewhere preserve larger user/editor fonts.
            if (compactControls ? qAbs(points-targetSize)<0.1
                                : points>=targetSize) continue;
            QFont corrected=widget->font();
            corrected.setPointSizeF(targetSize);
            widget->setFont(corrected);
            widget->updateGeometry();
        }
    }
}

#endif // LIGHTCONTENTFONTS_H
