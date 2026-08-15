/***************************************************************************
 *   Copyright (C) 2010 by Tn                                              *
 *   thenobody@poczta.fm                                                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 3 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <QPainter>

#include "qledindicator.h"

const qreal QLedIndicator::scaledSize = 1000; /* Visual Studio static const mess */

QLedIndicator::QLedIndicator(QWidget *parent) : QAbstractButton(parent)
{
    setMinimumSize(20,20);
    setCheckable(true);
    onColor1 =  QColor(0,255,0);
    onColor2 =  QColor(0,192,0);
    offColor1 = QColor(0,28,0);
    offColor2 = QColor(0,128,0);
}

void QLedIndicator::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);
    update();
}

void QLedIndicator::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    const qreal realSize = qMin(width(), height());
    if (realSize <= 0.0)
        return;

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.translate(width() / 2.0, height() / 2.0);
    painter.scale(realSize / scaledSize, realSize / scaledSize);

    /*
     * DARK indicator: the widget geometry is deliberately unchanged.
     * Everything below is painted inside the same 1000 x 1000 logical area,
     * so replacing the old lamp cannot alter any page layout.
     */

    // Very subtle shadow around the lamp, kept inside the widget bounds.
    QRadialGradient shadow(QPointF(0, 0), 495);
    shadow.setColorAt(0.76, QColor(0, 0, 0, 0));
    shadow.setColorAt(1.00, QColor(0, 0, 0, 150));
    painter.setPen(Qt::NoPen);
    painter.setBrush(shadow);
    painter.drawEllipse(QPointF(0, 0), 492, 492);

    // Dark outer bezel.
    QLinearGradient bezel(-350, -420, 350, 420);
    bezel.setColorAt(0.00, QColor(92, 104, 112));
    bezel.setColorAt(0.18, QColor(47, 56, 63));
    bezel.setColorAt(0.52, QColor(17, 23, 28));
    bezel.setColorAt(1.00, QColor(5, 8, 11));
    painter.setPen(QPen(QColor(8, 12, 15), 34));
    painter.setBrush(bezel);
    painter.drawEllipse(QPointF(0, 0), 454, 454);

    // Thin light rim (the selected "liseré"), intentionally restrained.
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(QColor(116, 132, 142), 28));
    painter.drawEllipse(QPointF(0, 0), 411, 411);
    painter.setPen(QPen(QColor(35, 44, 50), 22));
    painter.drawEllipse(QPointF(0, 0), 382, 382);

    const QColor c1 = isChecked() ? onColor1 : offColor1;
    const QColor c2 = isChecked() ? onColor2 : offColor2;

    // LED lens. Existing configured red/green colours are preserved.
    QRadialGradient lens(QPointF(-130, -155), 560, QPointF(-145, -165));
    QColor highlight = c1.lighter(isChecked() ? 145 : 112);
    QColor body = c1;
    QColor edge = c2.darker(isChecked() ? 112 : 138);
    if (!isChecked()) {
        body = body.darker(135);
        highlight = highlight.darker(120);
    }
    lens.setColorAt(0.00, highlight);
    lens.setColorAt(0.30, body);
    lens.setColorAt(0.78, c2);
    lens.setColorAt(1.00, edge);

    painter.setPen(QPen(QColor(4, 7, 9), 24));
    painter.setBrush(lens);
    painter.drawEllipse(QPointF(0, 0), 350, 350);

    // Soft glass reflection, more visible when the indicator is active.
    QRadialGradient reflection(QPointF(-145, -165), 260);
    reflection.setColorAt(0.00, QColor(255, 255, 255, isChecked() ? 110 : 48));
    reflection.setColorAt(0.45, QColor(255, 255, 255, isChecked() ? 28 : 10));
    reflection.setColorAt(1.00, QColor(255, 255, 255, 0));
    painter.setPen(Qt::NoPen);
    painter.setBrush(reflection);
    painter.drawEllipse(QPointF(-105, -120), 205, 150);

    // Fine inner contour keeps the lamp readable on the DARK background.
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(QColor(255, 255, 255, isChecked() ? 34 : 18), 14));
    painter.drawEllipse(QPointF(0, 0), 330, 330);
}
