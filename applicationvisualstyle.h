#ifndef APPLICATIONVISUALSTYLE_H
#define APPLICATIONVISUALSTYLE_H
#include <QFont>
#include <QApplication>
#include <QVariant>
#include <QWidget>
#include <QFontMetricsF>
#include <QStringList>

class QMainWindow;

// Shared dark-theme typography for ECU dial headings. Every dial on a
// page uses the same font size, chosen from the longest translated label.
namespace GaugeHeading {
inline QFont styledFont(const QFont &base,qreal size)
{
    QFont font(base);
    font.setFamily(QStringLiteral("Segoe UI"));
    font.setBold(true);
    font.setLetterSpacing(QFont::AbsoluteSpacing,0.4);
    font.setPointSizeF(size);
    return font;
}
inline qreal sharedPointSize(const QFont &base,const QStringList &labels,
                             qreal width,qreal preferred=8.2)
{
    qreal size=preferred;
    for(;size>=4.0;size-=0.1){
        const QFontMetricsF metrics(styledFont(base,size));
        bool fits=true;
        for(const QString &label:labels){
            if(metrics.horizontalAdvance(label.toUpper())>width){fits=false;break;}
        }
        if(fits)break;
    }
    return qMax<qreal>(3.9,size);
}
}
// Common Segoe UI data typography for the two data views and the ECU
// interactive instructions.  Headers are bold; ordinary content is not.
namespace DataPageTypography {
inline QFont body(const QFont &base)
{
    QFont font(base);
    font.setFamily(QStringLiteral("Segoe UI"));
    font.setPointSizeF(10.0);
    font.setWeight(QFont::Normal);
    return font;
}
inline QFont header(const QFont &base)
{
    QFont font=body(base);
    font.setWeight(QFont::Bold);
    return font;
}
}
// A widget's owner defines its dark appearance; light mode inherits the
// application palette instead. The original dark rule is retained for live
// theme changes, without building a second overlay stylesheet.
namespace ApplicationTheme {
inline bool isDark()
{
    return qApp && qApp->property("ecuDarkTheme").toBool();
}
inline void setDarkStyle(QWidget *widget, const QString &darkStyle)
{
    if (!widget) return;
    widget->setProperty("ecuOwnedDarkStyle", darkStyle);
    widget->setStyleSheet(isDark() ? darkStyle : QString());
}
inline void appendDarkStyle(QWidget *widget, const QString &additionalStyle)
{
    if (!widget) return;
    const QVariant previous = widget->property("ecuOwnedDarkStyle");
    const QString original = previous.isValid() ? previous.toString() : widget->styleSheet();
    setDarkStyle(widget, original + additionalStyle);
}
inline void refreshWidgetStyles(QWidget *root)
{
    if (!root) return;
    const auto refresh = [](QWidget *widget) {
        const QVariant darkStyle = widget->property("ecuOwnedDarkStyle");
        if (darkStyle.isValid())
            widget->setStyleSheet(isDark() ? darkStyle.toString() : QString());
    };
    refresh(root);
    for (QWidget *child : root->findChildren<QWidget*>()) refresh(child);
}
}

// Native UI owns visual style application and responsive card arrangement.
void applyApplicationVisualStyle(QMainWindow *window);
void resizeApplicationVisualStyle(QMainWindow *window);
#endif
