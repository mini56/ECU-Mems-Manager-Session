#include <QAbstractButton>
#include <QApplication>
#include <QCoreApplication>
#include <QEvent>
#include <QFrame>
#include <QLabel>
#include <QMainWindow>
#include <QPainter>
#include <QPainterPath>
#include <QTimer>
#include <QWidget>

#include "i18n.h"

namespace {

static QString disconnectedText()
{
    const QString language = I18n::language().toLower();
    if (language == QStringLiteral("en")) return QStringLiteral("DISCONNECTED");
    if (language == QStringLiteral("es")) return QStringLiteral("DESCONECTADO");
    if (language == QStringLiteral("it")) return QStringLiteral("DISCONNESSO");
    if (language == QStringLiteral("pt")) return QStringLiteral("DESCONECTADO");
    if (language == QStringLiteral("de")) return QStringLiteral("GETRENNT");
    return QString::fromUtf8("DÉCONNECTÉ");
}

static QString disconnectedSystemText()
{
    const QString language = I18n::language().toLower();
    if (language == QStringLiteral("en")) return QStringLiteral("System: disconnected");
    if (language == QStringLiteral("es")) return QStringLiteral("Sistema: desconectado");
    if (language == QStringLiteral("it")) return QStringLiteral("Sistema: disconnesso");
    if (language == QStringLiteral("pt")) return QStringLiteral("Sistema: desconectado");
    if (language == QStringLiteral("de")) return QStringLiteral("System: getrennt");
    return QString::fromUtf8("Système : déconnecté");
}

static bool isDisconnected(QMainWindow *window)
{
    if (!window)
        return false;

    QAbstractButton *connectButton = window->findChild<QAbstractButton*>(QStringLiteral("m_connectButton"));
    QAbstractButton *disconnectButton = window->findChild<QAbstractButton*>(QStringLiteral("m_disconnectButton"));
    return connectButton && disconnectButton && connectButton->isEnabled() && !disconnectButton->isEnabled();
}

class DisconnectedSystemOverlay : public QWidget
{
public:
    DisconnectedSystemOverlay(QMainWindow *window, QWidget *parent)
        : QWidget(parent), m_window(window)
    {
        setAttribute(Qt::WA_TransparentForMouseEvents, true);
        setAutoFillBackground(false);
        hide();
    }

    void syncState()
    {
        if (!parentWidget())
            return;

        setGeometry(parentWidget()->rect());
        const bool showDisconnected = isDisconnected(m_window) && parentWidget()->isVisible();
        setVisible(showDisconnected);
        if (showDisconnected)
        {
            raise();
            update();
        }
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setRenderHint(QPainter::TextAntialiasing, true);

        const qreal baseW = 188.0;
        const qreal baseH = 226.0;
        const qreal s = qMin(width() / baseW, height() / baseH);
        p.translate((width() - baseW * s) / 2.0, (height() - baseH * s) / 2.0);
        p.scale(s, s);

        const QRectF card(0, 0, baseW, baseH);
        p.setPen(QPen(QColor("#34414a"), 1.0));
        p.setBrush(QColor("#0c1217"));
        p.drawRoundedRect(card.adjusted(.5, .5, -.5, -.5), 5.0, 5.0);

        QFont f = p.font();
        f.setBold(true);
        f.setPointSizeF(8.2);
        p.setFont(f);
        p.setPen(QColor("#edf2f4"));
        p.drawText(QRectF(6, 5, 176, 18), Qt::AlignCenter, I18n::text(7149));
        p.setPen(QPen(QColor("#27333b"), 1.0));
        p.drawLine(QPointF(8, 25), QPointF(180, 25));

        const QPointF c(94, 97);
        const qreal rr = 45;
        QPainterPath shield;
        shield.moveTo(c.x(), c.y() - rr);
        shield.lineTo(c.x() + rr * .75, c.y() - rr * .65);
        shield.lineTo(c.x() + rr * .62, c.y() + rr * .28);
        shield.quadTo(c.x(), c.y() + rr, c.x() - rr * .62, c.y() + rr * .28);
        shield.lineTo(c.x() - rr * .75, c.y() - rr * .65);
        shield.closeSubpath();

        const QColor state("#ff9828");
        p.setPen(QPen(state, 2.6));
        p.setBrush(Qt::NoBrush);
        p.drawPath(shield);
        p.setPen(QPen(state, 3.3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.drawLine(QPointF(c.x() - 18, c.y() + 2), QPointF(c.x() - 5, c.y() + 15));
        p.drawLine(QPointF(c.x() - 5, c.y() + 15), QPointF(c.x() + 23, c.y() - 18));

        QFont st = p.font();
        st.setBold(true);
        st.setPointSizeF(9.2);
        p.setFont(st);
        p.setPen(state);
        p.drawText(QRectF(10, 157, 168, 28), Qt::AlignCenter, disconnectedText());

        QFont sub = p.font();
        sub.setBold(false);
        sub.setPointSizeF(6.0);
        p.setFont(sub);
        p.setPen(QColor("#8d99a3"));
        p.drawText(QRectF(14, 190, 160, 24), Qt::AlignCenter | Qt::TextWordWrap, I18n::text(7145));
    }

private:
    QMainWindow *m_window = nullptr;
};

static QLabel *findBottomSystemLabel(QMainWindow *window)
{
    QFrame *bar = window ? window->findChild<QFrame*>(QStringLiteral("uiRebuildStatus")) : nullptr;
    if (!bar)
        return nullptr;

    const auto labels = bar->findChildren<QLabel*>(QString(), Qt::FindDirectChildrenOnly);
    for (QLabel *label : labels)
    {
        if (!label)
            continue;
        if (label->objectName() == QStringLiteral("uiRebuildSystemStatus"))
            return label;
        if (label->text() == I18n::text(7120) || label->text() == I18n::text(7121) ||
            label->text() == disconnectedSystemText())
        {
            label->setObjectName(QStringLiteral("uiRebuildSystemStatus"));
            return label;
        }
    }
    return nullptr;
}

class BottomSystemStatusOverlay : public QLabel
{
public:
    BottomSystemStatusOverlay(QMainWindow *window, QLabel *source)
        : QLabel(source ? source->parentWidget() : nullptr), m_window(window), m_source(source)
    {
        setObjectName(QStringLiteral("uiRebuildSystemStatusOverlay"));
        setAttribute(Qt::WA_TransparentForMouseEvents, true);
        if (m_source)
        {
            setAlignment(m_source->alignment());
            setFont(m_source->font());
        }
        syncState();
    }

    void syncState()
    {
        if (!m_window || !m_source || !parentWidget())
            return;

        setGeometry(m_source->geometry());

        QString text;
        QString color;
        if (isDisconnected(m_window))
        {
            text = disconnectedSystemText();
            color = QStringLiteral("#ff9828");
        }
        else
        {
            QWidget *engineError = m_window->findChild<QWidget*>(QStringLiteral("m_engine_error"));
            const bool fault = engineError && engineError->property("checked").toBool();
            text = fault ? I18n::text(7120) : I18n::text(7121);
            color = fault ? QStringLiteral("#ff4b3b") : QStringLiteral("#65db79");
        }

        setText(text);
        setStyleSheet(QStringLiteral(
            "background:#080d12;color:%1;border-right:1px solid #29343e;padding:0 8px;").arg(color));
        show();
        raise();
    }

private:
    QMainWindow *m_window = nullptr;
    QLabel *m_source = nullptr;
};

static QWidget *findSystemStateCard(QMainWindow *window)
{
    QWidget *overview = window ? window->findChild<QWidget*>(QStringLiteral("overview_tab")) : nullptr;
    if (!overview || !overview->layout())
        return nullptr;

    const QList<QWidget*> children = overview->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
    for (QWidget *child : children)
    {
        if (!child || !child->isVisible())
            continue;
        if (qobject_cast<QFrame*>(child))
            continue;
        if (child->testAttribute(Qt::WA_TransparentForMouseEvents))
            continue;
        return child;
    }
    return nullptr;
}

static void installSystemStateConnectionFix(QMainWindow *window)
{
    if (!window || window->property("systemStateConnectionFixInstalled").toBool())
        return;

    QWidget *card = findSystemStateCard(window);
    if (!card)
        return;

    window->setProperty("systemStateConnectionFixInstalled", true);
    DisconnectedSystemOverlay *overlay = new DisconnectedSystemOverlay(window, card);
    overlay->setObjectName(QStringLiteral("disconnectedSystemOverlay"));

    BottomSystemStatusOverlay *bottomOverlay = nullptr;
    if (QLabel *systemLabel = findBottomSystemLabel(window))
        bottomOverlay = new BottomSystemStatusOverlay(window, systemLabel);

    QTimer *timer = new QTimer(overlay);
    timer->setInterval(200);
    QObject::connect(timer, &QTimer::timeout, overlay, [overlay, bottomOverlay]() {
        overlay->syncState();
        if (bottomOverlay)
            bottomOverlay->syncState();
    });
    timer->start();
    overlay->syncState();
    if (bottomOverlay)
        bottomOverlay->syncState();
}

class SystemStateConnectionFixInstaller : public QObject
{
public:
    explicit SystemStateConnectionFixInstaller(QObject *parent = nullptr) : QObject(parent) {}

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        QMainWindow *window = qobject_cast<QMainWindow*>(watched);
        if (!window || QString::fromLatin1(window->metaObject()->className()) != QStringLiteral("MainWindow"))
            return QObject::eventFilter(watched, event);

        if (event->type() == QEvent::Show || event->type() == QEvent::Polish)
        {
            QTimer::singleShot(300, window, [window]() { installSystemStateConnectionFix(window); });
            QTimer::singleShot(1000, window, [window]() { installSystemStateConnectionFix(window); });
        }

        return QObject::eventFilter(watched, event);
    }
};

void installSystemStateConnectionFixInstaller()
{
    QApplication *app = qobject_cast<QApplication*>(QCoreApplication::instance());
    if (app)
        app->installEventFilter(new SystemStateConnectionFixInstaller(app));
}

}

Q_COREAPP_STARTUP_FUNCTION(installSystemStateConnectionFixInstaller)
