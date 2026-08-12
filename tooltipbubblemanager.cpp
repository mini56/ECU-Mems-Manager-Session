#include "tooltipbubblemanager.h"

#include <QEvent>
#include <QLabel>
#include <QMargins>
#include <QPixmap>
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

    // Reserve space for the embedded help bubble in text labels.
    // This prevents long translations from running underneath the icon.
    if (QLabel *textLabel = qobject_cast<QLabel*>(widget))
    {
        const QMargins m = textLabel->contentsMargins();
        if (m.right() < 24)
            textLabel->setContentsMargins(m.left(), m.top(), 24, m.bottom());
    }

    if (!bubble)
    {
        bubble = new QLabel(widget);
        bubble->setObjectName(kBubbleObjectName);
        bubble->setPixmap(QPixmap(":/icons/helpbubble.png"));
        bubble->setFixedSize(18, 18);
        bubble->setScaledContents(true);
        bubble->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        bubble->setStyleSheet("background: transparent;");
    }

    bubble->move(qMax(0, widget->width() - bubble->width() - 2), 2);
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
        event->type() == QEvent::Show)
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
