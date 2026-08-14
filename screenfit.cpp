#include <QApplication>
#include <QEvent>
#include <QGuiApplication>
#include <QMainWindow>
#include <QScreen>
#include <QTabWidget>
#include <QTimer>
#include <QVariant>
#include <QWidget>
#include <QtMath>

namespace {

static const int kReferenceWidth = 1280;
static const int kReferenceHeight = 820;

class OverviewScreenFitter : public QObject
{
public:
    explicit OverviewScreenFitter(QMainWindow *window)
        : QObject(window), m_window(window)
    {
        m_overview = window->findChild<QWidget*>(QStringLiteral("overview_tab"));
        m_tabs = window->findChild<QTabWidget*>(QStringLiteral("Tab_main"));
        if (!m_overview || !m_tabs)
            return;

        // The whole program keeps the same visual composition. Only the
        // uniform scale changes according to the actual usable screen area.
        captureReferenceGeometry();
        m_overview->installEventFilter(this);
        m_tabs->installEventFilter(this);
        window->installEventFilter(this);

        QTimer::singleShot(0, this, [this](){ fitToAvailableArea(); });
        QTimer::singleShot(250, this, [this](){ captureNewWidgetsAndFit(); });
    }

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if ((watched == m_window || watched == m_tabs || watched == m_overview) &&
            (event->type() == QEvent::Resize || event->type() == QEvent::Show)) {
            if (!m_pending) {
                m_pending = true;
                QTimer::singleShot(0, this, [this](){
                    m_pending = false;
                    captureNewWidgetsAndFit();
                });
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
                if (pointSize > 0)
                    widget->setProperty("screenfitBaseFontSize", pointSize);
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

        QScreen *screen = m_window->screen();
        if (!screen) screen = QGuiApplication::primaryScreen();
        if (!screen) return;

        const QRect available = screen->availableGeometry();

        // Never allow the top-level window to exceed the usable desktop.
        const int maxWindowW = qMax(640, available.width() - 8);
        const int maxWindowH = qMax(480, available.height() - 8);
        if (m_window->width() > maxWindowW || m_window->height() > maxWindowH)
            m_window->resize(qMin(m_window->width(), maxWindowW), qMin(m_window->height(), maxWindowH));

        // The tab page is the real available viewport after title bar,
        // toolbar and left navigation have taken their space.
        const QSize viewport = m_tabs->size();
        if (viewport.width() < 100 || viewport.height() < 100) return;

        const qreal sx = qreal(viewport.width() - 16) / qreal(kReferenceWidth);
        const qreal sy = qreal(viewport.height() - 16) / qreal(kReferenceHeight);
        qreal scale = qMin(sx, sy);
        scale = qBound<qreal>(0.48, scale, 1.20);

        const int scaledW = qRound(kReferenceWidth * scale);
        const int scaledH = qRound(kReferenceHeight * scale);
        const int offsetX = qMax(0, (viewport.width() - scaledW) / 2);
        const int offsetY = qMax(0, (viewport.height() - scaledH) / 2);

        const QList<QWidget*> children = m_overview->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
        for (QWidget *widget : children) {
            const QVariant stored = widget->property("screenfitBaseGeometry");
            if (!stored.isValid()) continue;
            const QRect base = stored.toRect();
            const QRect fitted(
                offsetX + qRound(base.x() * scale),
                offsetY + qRound(base.y() * scale),
                qMax(1, qRound(base.width() * scale)),
                qMax(1, qRound(base.height() * scale)));
            widget->setGeometry(fitted);

            const QVariant fontStored = widget->property("screenfitBaseFontSize");
            if (fontStored.isValid()) {
                QFont f = widget->font();
                f.setPointSizeF(qMax<qreal>(6.5, fontStored.toDouble() * scale));
                widget->setFont(f);
            }
        }

        m_overview->setMinimumSize(0, 0);
        m_overview->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
        m_overview->setProperty("screenfitScale", scale);
        m_overview->update();
    }

    QMainWindow *m_window = nullptr;
    QWidget *m_overview = nullptr;
    QTabWidget *m_tabs = nullptr;
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
            if (window && window->objectName() == QStringLiteral("MainWindow") &&
                !window->property("screenFitInstalled").toBool()) {
                QTabWidget *tabs = window->findChild<QTabWidget*>(QStringLiteral("Tab_main"));
                if (tabs) {
                    window->setProperty("screenFitInstalled", true);
                    QTimer::singleShot(50, window, [window](){ new OverviewScreenFitter(window); });
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
