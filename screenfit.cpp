#include <QApplication>
#include <QEvent>
#include <QFont>
#include <QMainWindow>
#include <QScrollArea>
#include <QTabWidget>
#include <QTimer>
#include <QVariant>
#include <QWidget>
#include <QtMath>

namespace {

static const int kReferenceWidth = 1280;
static const int kReferenceHeight = 540;

static QScrollArea *containingScrollArea(QWidget *widget)
{
    QWidget *p = widget ? widget->parentWidget() : nullptr;
    while (p) {
        if (QScrollArea *scroll = qobject_cast<QScrollArea*>(p))
            return scroll;
        p = p->parentWidget();
    }
    return nullptr;
}

class OverviewScreenFitter : public QObject
{
public:
    explicit OverviewScreenFitter(QMainWindow *window)
        : QObject(window), m_window(window)
    {
        m_overview = window->findChild<QWidget*>(QStringLiteral("overview_tab"));
        m_tabs = window->findChild<QTabWidget*>(QStringLiteral("Tab_main"));
        if (!m_overview || !m_tabs) return;

        m_scroll = containingScrollArea(m_overview);
        if (m_scroll) {
            m_scroll->setWidgetResizable(true);
            m_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            m_scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
            m_scroll->setAlignment(Qt::AlignLeft | Qt::AlignTop);
            m_overview->setMinimumSize(0, 0);
            m_overview->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
            if (m_scroll->viewport()) m_scroll->viewport()->installEventFilter(this);
        }

        captureReferenceGeometry();
        m_overview->installEventFilter(this);
        m_tabs->installEventFilter(this);
        window->installEventFilter(this);
        QTimer::singleShot(0, this, [this](){ fitToAvailableArea(); });
        QTimer::singleShot(180, this, [this](){ captureNewWidgetsAndFit(); });
    }

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        const bool relevant = watched == m_window || watched == m_tabs || watched == m_overview ||
                              (m_scroll && watched == m_scroll->viewport());
        if (relevant && (event->type() == QEvent::Resize || event->type() == QEvent::Show)) {
            if (!m_pending) {
                m_pending = true;
                QTimer::singleShot(0, this, [this](){ m_pending = false; captureNewWidgetsAndFit(); });
            }
        }
        return QObject::eventFilter(watched, event);
    }

private:
    void captureReferenceGeometry()
    {
        if (!m_overview) return;
        const QList<QWidget*> children = m_overview->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
        for (QWidget *widget : children) {
            if (!widget->property("screenfitBaseGeometry").isValid())
                widget->setProperty("screenfitBaseGeometry", widget->geometry());
            if (!widget->property("screenfitBaseFontSize").isValid()) {
                const qreal pointSize = widget->font().pointSizeF();
                if (pointSize > 0) widget->setProperty("screenfitBaseFontSize", pointSize);
            }
        }
    }

    void captureNewWidgetsAndFit()
    {
        captureReferenceGeometry();
        fitToAvailableArea();
    }

    void fitToAvailableArea()
    {
        if (!m_window || !m_overview || !m_tabs) return;
        const QSize viewport = (m_scroll && m_scroll->viewport()) ? m_scroll->viewport()->size() : m_tabs->size();
        if (viewport.width() < 100 || viewport.height() < 100) return;

        qreal scale = m_window->property("globalUiScale").isValid()
            ? m_window->property("globalUiScale").toDouble()
            : 1.0;

        // The approved composition must fit the real page viewport, not the physical screen.
        // This is what prevents the 1300 px legacy page from creating a horizontal scrollbar.
        scale = qMin(scale, qreal(viewport.width() - 8) / qreal(kReferenceWidth));
        scale = qMin(scale, qreal(viewport.height() - 8) / qreal(kReferenceHeight));
        scale = qBound<qreal>(0.58, scale, 1.16);

        const int scaledW = qRound(kReferenceWidth * scale);
        const int scaledH = qRound(kReferenceHeight * scale);
        const int offsetX = qMax(0, (viewport.width() - scaledW) / 2);
        const int offsetY = qMax(0, (viewport.height() - scaledH) / 2);

        if (m_scroll) {
            m_overview->setMinimumSize(0, 0);
            m_overview->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
            m_overview->resize(viewport);
        }

        const QList<QWidget*> children = m_overview->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
        for (QWidget *widget : children) {
            const QVariant stored = widget->property("screenfitBaseGeometry");
            if (!stored.isValid()) continue;
            const QRect base = stored.toRect();
            widget->setGeometry(offsetX + qRound(base.x() * scale), offsetY + qRound(base.y() * scale),
                                qMax(1, qRound(base.width() * scale)), qMax(1, qRound(base.height() * scale)));
            const QVariant fontStored = widget->property("screenfitBaseFontSize");
            if (fontStored.isValid()) {
                QFont f = widget->font();
                f.setPointSizeF(qMax<qreal>(6.0, fontStored.toDouble() * scale));
                widget->setFont(f);
            }
        }

        m_overview->setProperty("screenfitScale", scale);
        m_overview->update();
    }

    QMainWindow *m_window = nullptr;
    QWidget *m_overview = nullptr;
    QTabWidget *m_tabs = nullptr;
    QScrollArea *m_scroll = nullptr;
    bool m_pending = false;
};

class ScreenFitInstaller : public QObject
{
public:
    explicit ScreenFitInstaller(QObject *parent = nullptr) : QObject(parent) {}
protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if ((event->type() == QEvent::Show || event->type() == QEvent::Polish) && watched) {
            QMainWindow *window = qobject_cast<QMainWindow*>(watched);
            if (window && window->objectName() == QStringLiteral("MainWindow") && !window->property("screenFitInstalled").toBool()) {
                if (window->findChild<QTabWidget*>(QStringLiteral("Tab_main"))) {
                    window->setProperty("screenFitInstalled", true);
                    QTimer::singleShot(0, window, [window](){ new OverviewScreenFitter(window); });
                }
            }
        }
        return QObject::eventFilter(watched, event);
    }
};

void installScreenFitHook()
{
    QApplication *app = qobject_cast<QApplication*>(QCoreApplication::instance());
    if (!app) return;
    ScreenFitInstaller *installer = new ScreenFitInstaller(app);
    app->installEventFilter(installer);
}

}

Q_COREAPP_STARTUP_FUNCTION(installScreenFitHook)
