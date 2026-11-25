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
#include <QAbstractItemView>
#include <QAbstractItemModel>
#include <QHelpEvent>
#include <QToolTip>
#include <QScreen>
#include <QTextDocument>
#include <QAbstractButton>
#include <QGuiApplication>

namespace {
const char *kBubbleObjectName = "_ecuHelpBubble";

// One presenter for widget and item-view help, including dialogs created later.
// Native Qt tooltip owns its lifetime, palette and screen-edge repositioning.
class HelpPresenter final : public QObject
{
public:
    explicit HelpPresenter(QObject *parent) : QObject(parent) {}
protected:
    bool eventFilter(QObject *watched,QEvent *event) override
    {
        if(event->type()==QEvent::ApplicationDeactivate ||
           (watched==qApp && event->type()==QEvent::DynamicPropertyChange)) {
            QToolTip::hideText();
        }
        if(event->type()!=QEvent::ToolTip)return false;
        QWidget *widget=qobject_cast<QWidget*>(watched);
        if(!widget)return false;
        auto *help=static_cast<QHelpEvent*>(event);
        QString detail=widget->toolTip(),title;
        QRect region=widget->rect();
        if(auto *view=qobject_cast<QAbstractItemView*>(widget->parentWidget())) {
            if(view->viewport()==widget) {
                const QModelIndex index=view->indexAt(help->pos());
                if(index.isValid()) {
                    const QString itemHelp=index.data(Qt::ToolTipRole).toString();
                    if(!itemHelp.isEmpty()) {
                        detail=itemHelp;
                        region=view->visualRect(index);
                        title=index.sibling(index.row(),0).data(Qt::DisplayRole).toString();
                    }
                }
            }
        }
        if(auto *icon=widget->findChild<QLabel*>(kBubbleObjectName,Qt::FindDirectChildrenOnly)) {
            if(!icon->isHidden() && qobject_cast<QLabel*>(widget)) {
                region=icon->geometry().intersected(widget->rect());
                if(!region.contains(help->pos())) {
                    QToolTip::hideText();event->accept();return true;
                }
            }
        }
        if(detail.trimmed().isEmpty())return false;
        if(title.isEmpty()) {
            if(auto *label=qobject_cast<QLabel*>(widget))title=label->text();
            else if(auto *button=qobject_cast<QAbstractButton*>(widget))title=button->text();
        }
        // Convert existing rich help to text, then escape all content exactly once.
        auto plain=[](const QString &s) {
            if(!Qt::mightBeRichText(s))return s;
            QTextDocument document;document.setHtml(s);return document.toPlainText();
        };
        title=plain(title).trimmed();
        if(title.size()>100)title.clear(); // do not turn long explanatory labels into headings
        QStringList lines;
        for(const QString &line:plain(detail).split(QLatin1Char('\n'))) {
            const QString clean=line.trimmed();
            if(!clean.isEmpty() && clean!=title && !lines.contains(clean))lines<<clean;
        }
        if(lines.isEmpty() && title.isEmpty())return false;
        QScreen *screen=QGuiApplication::screenAt(help->globalPos());
        if(!screen)screen=QGuiApplication::primaryScreen();
        const int width=screen?qBound(200,screen->availableGeometry().width()/3,450):420;
        QString body=lines.join(QLatin1Char('\n')).toHtmlEscaped();
        body.replace(QLatin1Char('\n'),QStringLiteral("<br/>"));
        if(!title.isEmpty())body=QStringLiteral("<b>")+title.toHtmlEscaped()+QStringLiteral("</b><br/>")+body;
        const QString html=QStringLiteral("<table width='%1' cellspacing='0' cellpadding='0'><tr><td>%2</td></tr></table>").arg(width).arg(body);
        QPoint anchor=widget->mapToGlobal(region.bottomRight());
        // Prefer the adjacent side; Qt clamps the tooltip to the active screen.
        if(screen && anchor.x()+width+30>screen->availableGeometry().right())
            anchor.setX(qMax(screen->availableGeometry().left(),widget->mapToGlobal(region.topLeft()).x()-width-24));
        QToolTip::showText(anchor,html,widget,region);
        event->accept();return true;
    }
};
void installHelpPresenter()
{
    if(!qApp || qApp->property("_ecuHelpPresenterInstalled").toBool())return;
    qApp->setProperty("_ecuHelpPresenterInstalled",true);
    qApp->installEventFilter(new HelpPresenter(qApp));
}
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
    // A thin white contour ensures the icon remains visibly white/orange
    // even when the host does not have the preferred UI font installed.
    painter.setPen(QPen(Qt::white,1.6));
    painter.drawPath(bubble);

    QFont textFont=painter.font();
    textFont.setFamily(QStringLiteral("Segoe UI"));
    textFont.setBold(true);
    textFont.setPixelSize(14);
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
    installHelpPresenter();
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
