#include "tooltipbubblemanager.h"

#include <QEvent>
#include <QLabel>
#include <QMargins>
#include <QPixmap>
#include <QVariant>
#include <QWidget>

namespace {
const char *kBubbleObjectName = "_ecuHelpBubble";
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
    if (widget->toolTip().isEmpty())
    {
        if (bubble) bubble->hide();
        return;
    }

    const bool bubbleLeft = widget->property("helpBubbleLeft").toBool();

    // Reserve space for the embedded help bubble in text labels.
    // Labels explicitly marked helpBubbleLeft keep the icon before the text;
    // all other labels preserve the existing right-side behaviour.
    if (QLabel *textLabel = qobject_cast<QLabel*>(widget))
    {
        const QMargins m = textLabel->contentsMargins();
        if (bubbleLeft)
        {
            if (m.left() < 30 || m.right() != 0)
                textLabel->setContentsMargins(qMax(m.left(), 30), m.top(), 0, m.bottom());
        }
        else if (m.right() < 30)
        {
            textLabel->setContentsMargins(m.left(), m.top(), 30, m.bottom());
        }
    }

    if (!bubble)
    {
        bubble = new QLabel(widget);
        bubble->setObjectName(kBubbleObjectName);
        bubble->setPixmap(QPixmap(":/icons/helpbubble.png"));
        bubble->setFixedSize(24, 18);
        bubble->setScaledContents(true);
        bubble->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        bubble->setStyleSheet("background: transparent;");
    }

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
