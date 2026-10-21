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

// LIGHT readability rule: the operator-approved "All measurements" and
// "All data" pages retain exactly their original typography. Everything
// else may use the free space for readable, unscaled text.
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

        // Include the root if it is text-bearing and all descendants,
        // without touching the sidebar, application header or footer.
        QList<QWidget*> children=page->findChildren<QWidget*>();
        children.prepend(page);
        for(QWidget *widget : children) {
            if (!widget || !lightContentFontTarget(widget)) continue;
            const qreal points=QFontInfo(widget->font()).pointSizeF();
            if (points >= minimumPointSize) continue;
            QFont enlarged=widget->font();
            enlarged.setPointSizeF(minimumPointSize);
            widget->setFont(enlarged);
            widget->updateGeometry();
        }
    }
}

#endif // LIGHTCONTENTFONTS_H
