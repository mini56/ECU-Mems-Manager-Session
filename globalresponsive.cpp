#include <QApplication>
#include <QCoreApplication>
#include <QEvent>
#include <QFont>
#include <QGuiApplication>
#include <QLayout>
#include <QMainWindow>
#include <QPointer>
#include <QRect>
#include <QScreen>
#include <QScrollArea>
#include <QTabWidget>
#include <QTimer>
#include <QVariant>
#include <QWidget>
#include <QtMath>

namespace {

static const qreal kMinReadableScale = 0.58;
static const qreal kMaxScale = 1.16;
static const int kWindowReferenceWidth = 1300;
static const int kWindowReferenceHeight = 690;
static const int kPageReferenceWidth = 1280;
static const int kPageReferenceHeight = 620;

static QWidget *realTabPage(QWidget *tabPage, QScrollArea **scrollOut = nullptr)
{
    if (scrollOut) *scrollOut = nullptr;
    if (!tabPage) return nullptr;
    if (QScrollArea *scroll = qobject_cast<QScrollArea*>(tabPage)) {
        if (scrollOut) *scrollOut = scroll;
        return scroll->widget();
    }
    return tabPage;
}

static void prepareScrollArea(QScrollArea *scroll)
{
    if (!scroll) return;
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    if (QWidget *page = scroll->widget()) {
        page->setMinimumSize(0, 0);
        page->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    }
}

static qreal calculateGlobalScale(QMainWindow *window)
{
    if (!window) return 1.0;
    QScreen *screen = window->screen();
    if (!screen) screen = QGuiApplication::primaryScreen();

    int availableWidth = window->width();
    int availableHeight = window->height();
    if (screen) {
        const QRect available = screen->availableGeometry();
        availableWidth = qMin(availableWidth, available.width() - 12);
        availableHeight = qMin(availableHeight, available.height() - 12);
    }

    if (availableWidth < 700 || availableHeight < 430) return 1.0;

    const qreal sx = qreal(availableWidth) / qreal(kWindowReferenceWidth);
    const qreal sy = qreal(availableHeight) / qreal(kWindowReferenceHeight);
    return qBound<qreal>(kMinReadableScale, qMin(sx, sy), kMaxScale);
}

class LegacyPageFitter : public QObject
{
public:
    LegacyPageFitter(QWidget *page, QScrollArea *scroll)
        : QObject(page), m_page(page), m_scroll(scroll)
    {
        if (!m_page) return;
        if (m_scroll) {
            prepareScrollArea(m_scroll);
            if (m_scroll->viewport()) m_scroll->viewport()->installEventFilter(this);
        }
        if (m_page->layout()) {
            m_layoutDriven = true;
            m_page->setMinimumSize(0, 0);
            m_page->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
            softenLayoutTree(m_page);
        } else {
            captureReference();
        }
        m_page->installEventFilter(this);
        QTimer::singleShot(0, this, [this]() { fit(); });
    }

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        const bool relevant = watched == m_page || (m_scroll && watched == m_scroll->viewport());
        if (relevant && (event->type() == QEvent::Resize || event->type() == QEvent::Show)) {
            if (!m_pending) {
                m_pending = true;
                QTimer::singleShot(0, this, [this]() { m_pending = false; fit(); });
            }
        }
        return QObject::eventFilter(watched, event);
    }

private:
    static void softenLayoutTree(QWidget *root)
    {
        if (!root) return;
        const QList<QWidget*> children = root->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
        for (QWidget *child : children) {
            if (!child) continue;
            if (child->minimumWidth() > 480) child->setMinimumWidth(0);
            if (child->minimumHeight() > 320) child->setMinimumHeight(0);
            if (child->maximumWidth() < QWIDGETSIZE_MAX && child->maximumWidth() > 480) child->setMaximumWidth(QWIDGETSIZE_MAX);
            if (child->maximumHeight() < QWIDGETSIZE_MAX && child->maximumHeight() > 320) child->setMaximumHeight(QWIDGETSIZE_MAX);
            if (child->layout()) softenLayoutTree(child);
        }
    }

    void captureReference()
    {
        if (!m_page) return;
        QRect bounds;
        const QList<QWidget*> children = m_page->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
        for (QWidget *child : children) {
            if (!child) continue;
            if (!child->property("globalResponsiveBaseGeometry").isValid())
                child->setProperty("globalResponsiveBaseGeometry", child->geometry());
            if (!child->property("globalResponsiveBaseFont").isValid()) {
                const qreal ps = child->font().pointSizeF();
                if (ps > 0.0) child->setProperty("globalResponsiveBaseFont", ps);
            }
            bounds = bounds.united(child->geometry());
        }
        m_baseWidth = qMax(kPageReferenceWidth, bounds.right() + 16);
        m_baseHeight = qMax(kPageReferenceHeight, qMin(bounds.bottom() + 16, 900));
        m_page->setMinimumSize(0, 0);
        m_page->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    }

    qreal effectiveScale() const
    {
        if (!m_page) return 1.0;
        QWidget *top = m_page->window();
        qreal scale = (top && top->property("globalUiScale").isValid())
            ? top->property("globalUiScale").toDouble() : 1.0;

        if (m_scroll && m_scroll->viewport()) {
            const QSize viewport = m_scroll->viewport()->size();
            if (viewport.width() > 100 && m_baseWidth > 0)
                scale = qMin(scale, qreal(viewport.width() - 8) / qreal(m_baseWidth));
            if (!m_layoutDriven && viewport.height() > 100 && m_baseHeight > 0)
                scale = qMin(scale, qreal(viewport.height() - 8) / qreal(m_baseHeight));
        }
        return qBound<qreal>(kMinReadableScale, scale, kMaxScale);
    }

    void fit()
    {
        if (!m_page) return;
        if (!m_layoutDriven && (m_baseWidth <= 0 || m_baseHeight <= 0)) captureReference();

        const qreal scale = effectiveScale();
        m_page->setProperty("globalResponsiveScale", scale);

        if (m_scroll && m_scroll->viewport()) {
            const QSize viewport = m_scroll->viewport()->size();
            m_page->setMinimumSize(0, 0);
            m_page->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
            m_page->resize(viewport);
        }

        if (m_layoutDriven) {
            softenLayoutTree(m_page);
            QFont f = m_page->font();
            const qreal base = m_page->property("globalResponsiveBasePageFont").isValid()
                ? m_page->property("globalResponsiveBasePageFont").toDouble()
                : (m_page->font().pointSizeF() > 0 ? m_page->font().pointSizeF() : 9.0);
            if (!m_page->property("globalResponsiveBasePageFont").isValid())
                m_page->setProperty("globalResponsiveBasePageFont", base);
            f.setPointSizeF(qMax<qreal>(6.2, base * scale));
            m_page->setFont(f);
            return;
        }

        if (m_baseWidth <= 0 || m_baseHeight <= 0) return;

        const int scaledW = qRound(m_baseWidth * scale);
        const int scaledH = qRound(m_baseHeight * scale);
        const int offsetX = qMax(0, (m_page->width() - scaledW) / 2);
        const int offsetY = qMax(0, (m_page->height() - scaledH) / 2);

        const QList<QWidget*> children = m_page->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
        for (QWidget *child : children) {
            if (!child) continue;
            const QVariant gv = child->property("globalResponsiveBaseGeometry");
            if (!gv.isValid()) continue;
            const QRect base = gv.toRect();
            child->setGeometry(offsetX + qRound(base.x() * scale), offsetY + qRound(base.y() * scale),
                               qMax(1, qRound(base.width() * scale)), qMax(1, qRound(base.height() * scale)));
            const QVariant fv = child->property("globalResponsiveBaseFont");
            if (fv.isValid()) {
                QFont font = child->font();
                font.setPointSizeF(qMax<qreal>(6.2, fv.toDouble() * scale));
                child->setFont(font);
            }
        }
        m_page->update();
    }

    QPointer<QWidget> m_page;
    QPointer<QScrollArea> m_scroll;
    bool m_layoutDriven = false;
    bool m_pending = false;
    int m_baseWidth = 0;
    int m_baseHeight = 0;
};

class GlobalResponsiveController : public QObject
{
public:
    explicit GlobalResponsiveController(QMainWindow *window)
        : QObject(window), m_window(window)
    {
        if (!m_window) return;
        m_window->installEventFilter(this);
        applyScale();
    }

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if (watched == m_window && (event->type() == QEvent::Show || event->type() == QEvent::Resize || event->type() == QEvent::WindowStateChange)) {
            if (!m_pending) {
                m_pending = true;
                QTimer::singleShot(0, this, [this]() { m_pending = false; applyScale(); });
            }
        }
        return QObject::eventFilter(watched, event);
    }

private:
    void applyScale()
    {
        if (!m_window) return;
        const qreal scale = calculateGlobalScale(m_window);
        m_window->setProperty("globalUiScale", scale);

        if (QTabWidget *tabs = m_window->findChild<QTabWidget*>(QStringLiteral("Tab_main"))) {
            tabs->setProperty("globalUiScale", scale);
            for (int i = 0; i < tabs->count(); ++i) {
                QScrollArea *scroll = nullptr;
                QWidget *page = realTabPage(tabs->widget(i), &scroll);
                if (scroll) prepareScrollArea(scroll);
                if (page) {
                    page->setProperty("globalUiScale", scale);
                    QEvent resizeEvent(QEvent::Resize);
                    QCoreApplication::sendEvent(page, &resizeEvent);
                }
            }
        }
        m_window->setProperty("globalUiScaleChanged", !m_window->property("globalUiScaleChanged").toBool());
    }

    QPointer<QMainWindow> m_window;
    bool m_pending = false;
};

class GlobalResponsiveInstaller : public QObject
{
public:
    explicit GlobalResponsiveInstaller(QObject *parent = nullptr) : QObject(parent) {}

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if ((event->type() == QEvent::Show || event->type() == QEvent::Polish) && watched) {
            QMainWindow *window = qobject_cast<QMainWindow*>(watched);
            if (window && window->objectName() == QStringLiteral("MainWindow") && !window->property("globalResponsiveInstalled").toBool()) {
                window->setProperty("globalResponsiveInstalled", true);
                QTimer::singleShot(0, window, [window]() { install(window); });
            }
        }
        return QObject::eventFilter(watched, event);
    }

private:
    static void install(QMainWindow *window)
    {
        if (!window) return;
        new GlobalResponsiveController(window);
        QTabWidget *tabs = window->findChild<QTabWidget*>(QStringLiteral("Tab_main"));
        if (!tabs) return;

        for (int i = 0; i < tabs->count(); ++i) {
            QScrollArea *scroll = nullptr;
            QWidget *page = realTabPage(tabs->widget(i), &scroll);
            if (!page) continue;
            if (scroll) prepareScrollArea(scroll);
            if (page->objectName() == QStringLiteral("overview_tab")) continue;
            if (QString::fromLatin1(page->metaObject()->className()) == QStringLiteral("AnalysisTab")) continue;
            if (!page->property("legacyResponsiveInstalled").toBool()) {
                page->setProperty("legacyResponsiveInstalled", true);
                new LegacyPageFitter(page, scroll);
            }
        }
    }
};

void installGlobalResponsiveHook()
{
    QApplication *app = qobject_cast<QApplication*>(QCoreApplication::instance());
    if (!app) return;
    GlobalResponsiveInstaller *installer = new GlobalResponsiveInstaller(app);
    app->installEventFilter(installer);
}

}

Q_COREAPP_STARTUP_FUNCTION(installGlobalResponsiveHook)
