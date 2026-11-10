#include "rawpage.h"
#include "applicationvisualstyle.h"
#include <QFont>
#include <QFontMetrics>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayout>
#include <QSizePolicy>
#include <QWidget>
#include <QtMath>

// The native raw-data page owns both 7D and 80 grids.  Their geometry is
// determined by one layout, not by the fixed coordinates in the legacy .ui.
void fitRawDataPage(QWidget *page)
{
    if (!page) return;
    QWidget *left=page->findChild<QWidget*>(QStringLiteral("raw_1"));
    QWidget *right=page->findChild<QWidget*>(QStringLiteral("raw_2"));
    if (!left || !right) return;

    page->setMinimumSize(0,0);
    page->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);

    QHBoxLayout *body=qobject_cast<QHBoxLayout*>(page->layout());
    if(!body && !page->layout()) {
        body=new QHBoxLayout(page);
        body->setContentsMargins(10,8,10,8);
        body->setSpacing(14);
        body->addWidget(left,1);
        body->addWidget(right,1);
    }
    if(!body) return;

    QGridLayout *leftGrid=qobject_cast<QGridLayout*>(left->layout());
    QGridLayout *rightGrid=qobject_cast<QGridLayout*>(right->layout());
    if(!leftGrid || !rightGrid) return;

    // Both native data pages use one 10pt Segoe UI font.  The grid
    // stretches its rows; font size is not recalculated on window resize.

    for (QWidget *block:{left,right}) {
        block->setMinimumSize(0,0);
        block->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
        block->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);

        QGridLayout *grid=qobject_cast<QGridLayout*>(block->layout());
        if(!grid)continue;
        grid->setContentsMargins(4,2,4,2);
        grid->setHorizontalSpacing(10);
        grid->setVerticalSpacing(1);
        grid->setSizeConstraint(QLayout::SetDefaultConstraint);
        grid->setColumnStretch(0,1);  // Hex position
        grid->setColumnStretch(1,6);  // Description
        grid->setColumnStretch(2,2);  // ECU value
        grid->setColumnStretch(3,3);  // Converted value

        const QFont rowFont=DataPageTypography::body(block->font());
        const int textHeight=QFontMetrics(rowFont).height();
        for(int row=0;row<grid->rowCount();++row){
            grid->setRowMinimumHeight(row,textHeight+1);
            grid->setRowStretch(row,1);
        }

        for(QLabel *label:block->findChildren<QLabel*>()) {
            const QString name=label->objectName();
            const bool header=name.startsWith(QStringLiteral("header_")) ||
                              name.startsWith(QStringLiteral("Aheader_"));
            label->setFont(header
                ? DataPageTypography::header(label->font())
                : DataPageTypography::body(label->font()));
            label->setWordWrap(false);
            label->setMinimumHeight(textHeight+1);
            label->setMaximumHeight(QWIDGETSIZE_MAX);
            label->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Expanding);
            label->setStyleSheet(header
                ? QStringLiteral("color:#ff9828;background:transparent;border:0;font-weight:700;")
                : QStringLiteral("color:#ffffff;background:transparent;border:0;font-weight:400;"));
        }
    }
}
