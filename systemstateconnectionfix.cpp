#include <QAbstractButton>
#include <QApplication>
#include <QCoreApplication>
#include <QEvent>
#include <QFrame>
#include <QLabel>
#include <QLayout>
#include <QMainWindow>
#include <QPainter>
#include <QPainterPath>
#include <QPointer>
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

        QFont stateFont = p.font();
        stateFont.setBold(true);
        stateFont.setPointSizeF(9.2);
        p.setFont(stateFont);
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

class ErrorsDisconnectedSystemOverlay : public QWidget
{
public:
    ErrorsDisconnectedSystemOverlay(QMainWindow *window, QWidget *parent)
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

        const QRectF outer = QRectF(rect()).adjusted(.5, .5, -.5, -.5);
        p.setPen(QPen(QColor("#34414a"), 1.0));
        p.setBrush(QColor("#0c1217"));
        p.drawRoundedRect(outer, 5, 5);

        QFont f = p.font();
        f.setBold(true);
        f.setPointSizeF(8.2);
        p.setFont(f);
        p.setPen(QColor("#edf2f4"));
        p.drawText(QRectF(6, 5, qMax(0, width() - 12), 18), Qt::AlignCenter, I18n::text(7149));
        p.setPen(QPen(QColor("#27333b"), 1.0));
        p.drawLine(QPointF(8, 25), QPointF(qMax(8, width() - 8), 25));

        const QColor state("#ff9828");
        const QPointF c(width() / 2.0, height() * 0.48);
        const qreal rr = qMin(width() * 0.19, height() * 0.16);
        QPainterPath shield;
        shield.moveTo(c.x(), c.y() - rr);
        shield.lineTo(c.x() + rr * .75, c.y() - rr * .65);
        shield.lineTo(c.x() + rr * .62, c.y() + rr * .28);
        shield.quadTo(c.x(), c.y() + rr, c.x() - rr * .62, c.y() + rr * .28);
        shield.lineTo(c.x() - rr * .75, c.y() - rr * .65);
        shield.closeSubpath();
        p.setPen(QPen(state, 2.4));
        p.setBrush(Qt::NoBrush);
        p.drawPath(shield);

        p.setPen(QPen(state, 2.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.drawLine(QPointF(c.x() - rr * .40, c.y() + rr * .04),
                   QPointF(c.x() - rr * .12, c.y() + rr * .32));
        p.drawLine(QPointF(c.x() - rr * .12, c.y() + rr * .32),
                   QPointF(c.x() + rr * .50, c.y() - rr * .40));

        QFont stateFont = p.font();
        stateFont.setBold(true);
        stateFont.setPointSizeF(7.8);
        p.setFont(stateFont);
        p.setPen(state);
        p.drawText(QRectF(8, height() * 0.67, qMax(0, width() - 16), 24),
                   Qt::AlignCenter, disconnectedText());

        QFont sub = p.font();
        sub.setBold(false);
        sub.setPointSizeF(6.2);
        p.setFont(sub);
        p.setPen(QColor("#8d99a3"));
        p.drawText(QRectF(8, qMax(30, height() - 38), qMax(0, width() - 16), 30),
                   Qt::AlignCenter | Qt::TextWordWrap, I18n::text(7145));
    }

private:
    QMainWindow *m_window = nullptr;
};

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

class BottomDisconnectedValueOverlay : public QLabel
{
public:
    BottomDisconnectedValueOverlay(QMainWindow *window, QLabel *source, const QString &text)
        : QLabel(source ? source->parentWidget() : nullptr), m_window(window), m_source(source), m_text(text)
    {
        setAttribute(Qt::WA_TransparentForMouseEvents, true);
        if (m_source)
        {
            setAlignment(m_source->alignment());
            setFont(m_source->font());
        }
        hide();
        syncState();
    }

    void syncState()
    {
        if (!m_window || !m_source || !parentWidget())
            return;

        setGeometry(m_source->geometry());
        const bool showDisconnected = isDisconnected(m_window) && m_source->isVisible();
        setVisible(showDisconnected);
        if (showDisconnected)
        {
            setText(m_text);
            setStyleSheet(QStringLiteral(
                "background:#080d12;color:#c9d1d7;border-right:1px solid #29343e;padding:0 8px;"));
            raise();
        }
    }

private:
    QMainWindow *m_window = nullptr;
    QLabel *m_source = nullptr;
    QString m_text;
};

static QList<QLabel*> bottomStatusLabels(QMainWindow *window)
{
    QList<QLabel*> labels;
    QFrame *bar = window ? window->findChild<QFrame*>(QStringLiteral("uiRebuildStatus")) : nullptr;
    QLayout *layout = bar ? bar->layout() : nullptr;
    if (!layout)
        return labels;

    for (int i = 0; i < layout->count(); ++i)
    {
        QLayoutItem *item = layout->itemAt(i);
        QLabel *label = item ? qobject_cast<QLabel*>(item->widget()) : nullptr;
        if (label)
            labels.append(label);
    }
    return labels;
}

static QWidget *findOverviewSystemStateCard(QMainWindow *window)
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

static QWidget *findErrorsSystemStateCard(QMainWindow *window)
{
    QFrame *errorsLive = window ? window->findChild<QFrame*>(QStringLiteral("errorsLive")) : nullptr;
    if (!errorsLive)
        return nullptr;

    const QList<QWidget*> children = errorsLive->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
    for (QWidget *child : children)
    {
        if (!child || child->objectName() == QStringLiteral("errorsCompactSignals"))
            continue;
        if (!child->testAttribute(Qt::WA_TransparentForMouseEvents))
            continue;
        if (child->minimumWidth() >= 140 && child->maximumWidth() <= 220)
            return child;
    }
    return nullptr;
}

class SystemStateConnectionController : public QObject
{
public:
    explicit SystemStateConnectionController(QMainWindow *window)
        : QObject(window), m_window(window)
    {
        QTimer *timer = new QTimer(this);
        timer->setInterval(200);
        QObject::connect(timer, &QTimer::timeout, this, [this]() { syncAll(); });
        timer->start();
        syncAll();
    }

private:
    void ensureOverviewOverlay()
    {
        if (!m_window || m_overviewOverlay)
            return;

        QWidget *card = findOverviewSystemStateCard(m_window);
        if (!card)
            return;

        m_overviewOverlay = new DisconnectedSystemOverlay(m_window, card);
        m_overviewOverlay->setObjectName(QStringLiteral("disconnectedSystemOverlay"));
        m_window->setProperty("systemStateConnectionFixInstalled", true);
    }

    void ensureErrorsOverlay()
    {
        if (!m_window || m_errorsOverlay)
            return;

        QWidget *card = findErrorsSystemStateCard(m_window);
        if (!card)
            return;

        m_errorsOverlay = new ErrorsDisconnectedSystemOverlay(m_window, card);
        m_errorsOverlay->setObjectName(QStringLiteral("errorsDisconnectedSystemOverlay"));
        m_window->setProperty("errorsSystemStateConnectionFixInstalled", true);
    }

    void ensureBottomOverlays()
    {
        if (!m_window || m_systemOverlay)
            return;

        const QList<QLabel*> labels = bottomStatusLabels(m_window);
        if (labels.size() < 6)
            return;

        QLabel *loopLabel = labels.at(1);
        QLabel *lambdaLabel = labels.at(2);
        QLabel *systemLabel = labels.at(3);
        QLabel *injectLabel = labels.at(4);
        QLabel *airLabel = labels.at(5);
        systemLabel->setObjectName(QStringLiteral("uiRebuildSystemStatus"));

        m_systemOverlay = new BottomSystemStatusOverlay(m_window, systemLabel);
        m_loopOverlay = new BottomDisconnectedValueOverlay(
            m_window, loopLabel, I18n::text(7116).arg(QStringLiteral("--")));
        m_lambdaOverlay = new BottomDisconnectedValueOverlay(
            m_window, lambdaLabel, I18n::text(7119).arg(QStringLiteral("--")));
        m_injectOverlay = new BottomDisconnectedValueOverlay(
            m_window, injectLabel, I18n::text(7122).arg(QStringLiteral("--")));
        m_airOverlay = new BottomDisconnectedValueOverlay(
            m_window, airLabel, I18n::text(7123).arg(QStringLiteral("--")));
        m_window->setProperty("bottomStatusConnectionFixInstalled", true);
    }

    void syncAll()
    {
        if (!m_window)
            return;

        ensureOverviewOverlay();
        ensureErrorsOverlay();
        ensureBottomOverlays();

        if (m_overviewOverlay)
            m_overviewOverlay->syncState();
        if (m_errorsOverlay)
            m_errorsOverlay->syncState();
        if (m_systemOverlay)
            m_systemOverlay->syncState();
        if (m_loopOverlay)
            m_loopOverlay->syncState();
        if (m_lambdaOverlay)
            m_lambdaOverlay->syncState();
        if (m_injectOverlay)
            m_injectOverlay->syncState();
        if (m_airOverlay)
            m_airOverlay->syncState();
    }

    QPointer<QMainWindow> m_window;
    QPointer<DisconnectedSystemOverlay> m_overviewOverlay;
    QPointer<ErrorsDisconnectedSystemOverlay> m_errorsOverlay;
    QPointer<BottomSystemStatusOverlay> m_systemOverlay;
    QPointer<BottomDisconnectedValueOverlay> m_loopOverlay;
    QPointer<BottomDisconnectedValueOverlay> m_lambdaOverlay;
    QPointer<BottomDisconnectedValueOverlay> m_injectOverlay;
    QPointer<BottomDisconnectedValueOverlay> m_airOverlay;
};

class SystemStateConnectionFixInstaller : public QObject
{
public:
    explicit SystemStateConnectionFixInstaller(QApplication *app)
        : QObject(app), m_app(app)
    {
    }

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if (!event || (event->type() != QEvent::Show && event->type() != QEvent::Polish))
            return QObject::eventFilter(watched, event);

        QMainWindow *window = qobject_cast<QMainWindow*>(watched);
        if (!window || QString::fromLatin1(window->metaObject()->className()) != QStringLiteral("MainWindow"))
            return QObject::eventFilter(watched, event);

        QPointer<QMainWindow> guardedWindow(window);
        QTimer::singleShot(300, window, [guardedWindow]() {
            if (!guardedWindow || guardedWindow->findChild<SystemStateConnectionController*>())
                return;
            SystemStateConnectionController *controller = new SystemStateConnectionController(guardedWindow);
            controller->setObjectName(QStringLiteral("systemStateConnectionController"));
        });

        // After locating MainWindow, no application-wide event filtering is
        // required. The controller owns one local 200 ms timer and retries the
        // late-created visual targets itself.
        if (m_app)
            m_app->removeEventFilter(this);
        deleteLater();

        return QObject::eventFilter(watched, event);
    }

private:
    QApplication *m_app;
};

void installSystemStateConnectionFixInstaller()
{
    QApplication *app = qobject_cast<QApplication*>(QCoreApplication::instance());
    if (app)
        app->installEventFilter(new SystemStateConnectionFixInstaller(app));
}

}

Q_COREAPP_STARTUP_FUNCTION(installSystemStateConnectionFixInstaller)
