#include "rawpage.h"
#include <QFont>
#include <QGridLayout>
#include <QLabel>
#include <QLayout>
#include <QSizePolicy>
#include <QWidget>
#include <QtMath>

// This module owns the responsive layout of the original raw-data page.
// It only adjusts its two data grids and never modifies other test pages.
void fitRawDataPage(QWidget *page)
{
    if (!page) return;
    QWidget *left=page->findChild<QWidget*>(QStringLiteral("raw_1"));
    QWidget *right=page->findChild<QWidget*>(QStringLiteral("raw_2"));
    if (!left || !right) return;

    page->setMinimumSize(0,0);
    page->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);

    QWidget *viewport=page->parentWidget();
    int visibleHeight=(viewport && viewport->height()>0)?viewport->height():page->height();
    visibleHeight=qMax(360,visibleHeight);

    auto gridFor=[](QWidget *block)->QGridLayout* {
        return block?qobject_cast<QGridLayout*>(block->layout()):nullptr;
    };
    QGridLayout *leftGrid=gridFor(left);
    QGridLayout *rightGrid=gridFor(right);
    const int rows=qMax(leftGrid?leftGrid->rowCount():1,rightGrid?rightGrid->rowCount():1);

    for (QWidget *block:{left,right}) {
        block->setMinimumSize(0,0);
        block->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
        block->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
        QWidget *parent=block->parentWidget();
        QLayout *parentLayout=parent?parent->layout():nullptr;
        if (!parentLayout || parentLayout->indexOf(block)<0) {
            const int top=qMax(0,block->y());
            block->resize(block->width(),qMax(300,visibleHeight-top-6));
        }
    }

    const int usableHeight=qMax(300,visibleHeight-14);
    const int rowHeight=qMax(10,qMin(21,usableHeight/qMax(1,rows)));
    const qreal fontSize=rowHeight<=10?6.7:(rowHeight<=11?6.9:(rowHeight<=12?7.1:(rowHeight<=13?7.4:(rowHeight<=14?7.6:(rowHeight<=15?7.8:(rowHeight<=16?8.0:8.2))))));

    auto fitBlock=[&](QWidget *block,QGridLayout *grid) {
        if (!block) return;
        if (grid) {
            grid->setContentsMargins(0,0,0,0);
            grid->setHorizontalSpacing(8);
            grid->setVerticalSpacing(0);
            grid->setSizeConstraint(QLayout::SetDefaultConstraint);
            for (int r=0;r<grid->rowCount();++r) grid->setRowMinimumHeight(r,rowHeight);
        }
        for (QLabel *label:block->findChildren<QLabel*>()) {
            const QString name=label->objectName();
            const bool header=name.startsWith(QStringLiteral("header_")) || name.startsWith(QStringLiteral("Aheader_"));
            QFont f=label->font();
            f.setPointSizeF(fontSize);
            f.setBold(header);
            label->setFont(f);
            label->setWordWrap(false);
            label->setMinimumHeight(rowHeight);
            label->setMaximumHeight(rowHeight);
            label->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Fixed);
            label->setStyleSheet(header
                ? QStringLiteral("color:#ff9828;background:transparent;border:0;font-weight:700;")
                : QStringLiteral("color:#ffffff;background:transparent;border:0;"));
        }
    };

    fitBlock(left,leftGrid);
    fitBlock(right,rightGrid);
}

