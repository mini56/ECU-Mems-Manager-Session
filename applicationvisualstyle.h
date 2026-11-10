#ifndef APPLICATIONVISUALSTYLE_H
#define APPLICATIONVISUALSTYLE_H
#include <QFont>
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
// Native UI owns visual style application and responsive card arrangement.
void applyApplicationVisualStyle(QMainWindow *window);
void resizeApplicationVisualStyle(QMainWindow *window);
#endif
