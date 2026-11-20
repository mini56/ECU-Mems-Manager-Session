#include "tooltipbubblemanager.h"

#include <QApplication>
#include <QEvent>
#include <QPainter>
#include <QPainterPath>
#include <QFont>
#include <QLabel>
#include <QMargins>
#include <QPixmap>
#include <QVariant>
#include <QWidget>

namespace {
const char *kBubbleObjectName = "_ecuHelpBubble";
}

// Use the original icon for dark mode. In the light theme, create the same
// small speech-bubble shape in orange with WHITE lettering. This is a local
// Qt drawing, so no extra graphic asset, web access or theme-wide replacement.
QPixmap TooltipBubbleManager::helpBubblePixmap(bool darkTheme)
{
    if(darkTheme)
        return QPixmap(QStringLiteral(":/icons/helpbubble.png"));

    QPixmap pixmap(48,36);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing,true);

    QPainterPath bubble;
    bubble.addRoundedRect(QRectF(2,2,43,27),5,5);
    bubble.moveTo(32,27);
    bubble.lineTo(39,34);
    bubble.lineTo(38,27);
    bubble.closeSubpath();
    painter.setPen(Qt::NoPen);
    painter.fillPath(bubble,QColor(QStringLiteral("#ff8a1c")));

    QFont textFont=painter.font();
    textFont.setFamily(QStringLiteral("Segoe UI"));
    textFont.setBold(true);
    textFont.setPixelSize(16);
    painter.setFont(textFont);
    painter.setPen(Qt::white);
    painter.drawText(QRect(2,3,43,25),Qt::AlignCenter,QStringLiteral("HELP"));
    painter.end();
    return pixmap;
}

TooltipBubbleManager::TooltipBubbleManager(QWidget *root) : QObject(root)
{
    registerWidget(root);
    const QList<QWidget*> widgets = root->findChildren<QWidget*>();
    for (QWidget *widget : widgets)
        registerWidget(widget);
}

void TooltipBubbleManager::install(QWidget *root)
{
    if (!root || root->findChild<TooltipBubbleManager*>())
        return;
    new TooltipBubbleManager(root);
}

void TooltipBubbleManager::registerWidget(QWidget *widget)
{
    if (!widget)
        return;
    widget->installEventFilter(this);
    ensureBubble(widget);
}

void TooltipBubbleManager::ensureBubble(QWidget *widget)
{
    if (!widget || widget->objectName() == kBubbleObjectName)
        return;

    QLabel *bubble = widget->findChild<QLabel*>(kBubbleObjectName, Qt::FindDirectChildrenOnly);
    // The sidebar collapse control is explicitly labelled; it must never
    // get the generic speech bubble. Own this rule in the tooltip source.
    if (widget->objectName() == QStringLiteral("darkSidebarToggle"))
    {
        if (!widget->toolTip().isEmpty()) widget->setToolTip(QString());
        if (bubble) bubble->hide();
        return;
    }
    // The header's new-version button is already self-explanatory. An icon
    // overlaid at its right edge obscured the version number on compact screens.
    // Keep its normal hover help, but never paint a speech bubble on the button.
    if (widget->objectName() == QStringLiteral("uiProgramUpdateButton"))
    {
        if (bubble) bubble->hide();
        return;
    }
    // The Analysis selector owns a standalone help icon outside both buttons.
    // Never add a second icon or reserve margins inside this dedicated icon.
    if (widget->objectName() == QStringLiteral("analysisViewModeHelp"))
    {
        if (bubble) bubble->hide();
        return;
    }
    const bool bubbleLeft = widget->property("helpBubbleLeft").toBool();

    // Labels explicitly marked helpBubbleLeft always reserve the HELP column,
    // even when that particular row has no tooltip/bubble.  This keeps the
    // text aligned with neighbouring rows without creating a fake bubble.
    if (bubbleLeft)
    {
        if (QLabel *textLabel = qobject_cast<QLabel*>(widget))
        {
            const QMargins m = textLabel->contentsMargins();
            if (m.left() < 30 || m.right() != 0)
                textLabel->setContentsMargins(qMax(m.left(), 30), m.top(), 0, m.bottom());
        }
    }

    if (widget->toolTip().isEmpty())
    {
        if (bubble) bubble->hide();
        return;
    }

    // All other tooltip labels preserve the existing right-side behaviour.
    if (!bubbleLeft)
    {
        if (QLabel *textLabel = qobject_cast<QLabel*>(widget))
        {
            const QMargins m = textLabel->contentsMargins();
            if (m.right() < 30)
                textLabel->setContentsMargins(m.left(), m.top(), 30, m.bottom());
        }
    }

    if (!bubble)
    {
        bubble = new QLabel(widget);
        bubble->setObjectName(kBubbleObjectName);
        bubble->setFixedSize(24, 18);
        bubble->setScaledContents(true);
        bubble->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        bubble->setStyleSheet("background: transparent;");
    }

    bubble->setPixmap(helpBubblePixmap(
        qApp && qApp->property("ecuDarkTheme").toBool()));
    const int y = qMax(0, (widget->height() - bubble->height()) / 2);
    if (bubbleLeft)
        bubble->move(2, y);
    else
        bubble->move(qMax(0, widget->width() - bubble->width() - 2), y);
    bubble->raise();
    bubble->show();
}

bool TooltipBubbleManager::eventFilter(QObject *watched, QEvent *event)
{
    QWidget *widget = qobject_cast<QWidget*>(watched);
    if (!widget)
        return QObject::eventFilter(watched, event);

    if (event->type() == QEvent::ToolTipChange ||
        event->type() == QEvent::Resize ||
        event->type() == QEvent::Show ||
        event->type() == QEvent::StyleChange ||
        event->type() == QEvent::DynamicPropertyChange)
    {
        ensureBubble(widget);
    }
    else if (event->type() == QEvent::ChildAdded)
    {
        QChildEvent *ce = static_cast<QChildEvent*>(event);
        if (QWidget *child = qobject_cast<QWidget*>(ce->child()))
            registerWidget(child);
    }

    return QObject::eventFilter(watched, event);
}
