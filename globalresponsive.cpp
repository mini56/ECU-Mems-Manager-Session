#include <QApplication>
#include <QCoreApplication>
#include <QEvent>
#include <QFont>
#include <QLayout>
#include <QMainWindow>
#include <QPointer>
#include <QRect>
#include <QTabWidget>
#include <QTimer>
#include <QVariant>
#include <QWidget>
#include <QtMath>

namespace {

static const qreal kMinReadableScale = 0.62;
static const qreal kMaxScale = 1.16;
static const int kReferenceWidth = 1280;
static const int kReferenceHeight = 620;

class LegacyPageFitter : public QObject
{
public:
    explicit LegacyPageFitter(QWidget *page)
        : QObject(page), m_page(page)
    {
        if (!m_page)
            return;

        // Pages driven by a Qt layout are already naturally responsive. We only
        // relax historical minimum constraints there. Absolute-position pages
        // are captured once and are always resized from that immutable reference.
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
        if (watched == m_page &&
            (event->type() == QEvent::Resize || event->type() == QEvent::Show)) {
            if (!m_pending) {
                m_pending = true;
                QTimer::singleShot(0, this, [this]() {
                    m_pending = false;
                    fit();
                });
            }
        }
        return QObject::eventFilter(watched, event);
    }

private:
    static void softenLayoutTree(QWidget *root)
    {
        if (!root)
            return;

        const QList<QWidget*> children = root->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
        for (QWidget *child : children) {
            if (!child)
                continue;

            // Do not erase intentional fixed control heights; only remove large
            // page-like minimum sizes that can force clipping on small screens.
            if (child->minimumWidth() > 480)
                child->setMinimumWidth(0);
            if (child->minimumHeight() > 320)
                child->setMinimumHeight(0);

            if (child->maximumWidth() < QWIDGETSIZE_MAX && child->maximumWidth() > 480)
                child->setMaximumWidth(QWIDGETSIZE_MAX);
            if (child->maximumHeight() < QWIDGETSIZE_MAX && child->maximumHeight() > 320)
                child->setMaximumHeight(QWIDGETSIZE_MAX);

            if (child->layout())
                softenLayoutTree(child);
        }
    }

    void captureReference()
    {
        if (!m_page)
            return;

        QRect bounds;
        const QList<QWidget*> children = m_page->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
        for (QWidget *child : children) {
            if (!child)
                continue;
            if (!child->property("globalResponsiveBaseGeometry").isValid())
                child->setProperty("globalResponsiveBaseGeometry", child->geometry());
            if (!child->property("globalResponsiveBaseFont").isValid()) {
                const qreal ps = child->font().pointSizeF();
                if (ps > 0.0)
                    child->setProperty("globalResponsiveBaseFont", ps);
            }
            bounds = bounds.united(child->geometry());
        }

        // Some historical pages were authored inside an artificially tall
        // QTabWidget. Use the actual child extent, while keeping the validated
        // 1280-wide composition as the reference width.
        m_baseWidth = qMax(kReferenceWidth, bounds.right() + 16);
        m_baseHeight = qMax(kReferenceHeight, qMin(bounds.bottom() + 16, 900));
        m_page->setProperty("globalResponsiveBaseSize", QSize(m_baseWidth, m_baseHeight));
        m_page->setMinimumSize(0, 0);
        m_page->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    }

    void fit()
    {
        if (!m_page)
            return;

        if (m_layoutDriven) {
            softenLayoutTree(m_page);
            return;
        }

        if (m_baseWidth <= 0 || m_baseHeight <= 0)
            captureReference();
        if (m_baseWidth <= 0 || m_baseHeight <= 0)
            return;

        const int availableW = qMax(320, m_page->width() - 12);
        const int availableH = qMax(240, m_page->height() - 12);
        const qreal sx = qreal(availableW) / qreal(m_baseWidth);
        const qreal sy = qreal(availableH) / qreal(m_baseHeight);
        qreal scale = qMin(sx, sy);
        scale = qBound(kMinReadableScale, scale, kMaxScale);

        const int scaledW = qRound(m_baseWidth * scale);
        const int scaledH = qRound(m_baseHeight * scale);
        const int offsetX = qMax(0, (availableW - scaledW) / 2);
        const int offsetY = qMax(0, (availableH - scaledH) / 2);

        const QList<QWidget*> children = m_page->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
        for (QWidget *child : children) {
            if (!child)
                continue;
            const QVariant gv = child->property("globalResponsiveBaseGeometry");
            if (!gv.isValid())
                continue;

            const QRect base = gv.toRect();
            child->setGeometry(
                offsetX + qRound(base.x() * scale),
                offsetY + qRound(base.y() * scale),
                qMax(1, qRound(base.width() * scale)),
                qMax(1, qRound(base.height() * scale)));

            const QVariant fv = child->property("globalResponsiveBaseFont");
            if (fv.isValid()) {
                QFont font = child->font();
                font.setPointSizeF(qMax<qreal>(6.5, fv.toDouble() * scale));
                child->setFont(font);
            }
        }

        m_page->setProperty("globalResponsiveScale", scale);
        m_page->update();
    }

    QPointer<QWidget> m_page;
    bool m_layoutDriven = false;
    bool m_pending = false;
    int m_baseWidth = 0;
    int m_baseHeight = 0;
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
            if (window && window->objectName() == QStringLiteral("MainWindow") &&
                !window->property("globalResponsiveInstalled").toBool()) {
                window->setProperty("globalResponsiveInstalled", true);
                QTimer::singleShot(700, window, [window]() { install(window); });
            }
        }
        return QObject::eventFilter(watched, event);
    }

private:
    static void install(QMainWindow *window)
    {
        if (!window)
            return;
        QTabWidget *tabs = window->findChild<QTabWidget*>(QStringLiteral("Tab_main"));
        if (!tabs)
            return;

        for (int i = 0; i < tabs->count(); ++i) {
            QWidget *page = tabs->widget(i);
            if (!page)
                continue;

            // These two pages already have dedicated fitters tuned to their
            // validated composition. Every other main page is handled here.
            if (page->objectName() == QStringLiteral("overview_tab"))
                continue;
            if (QString::fromLatin1(page->metaObject()->className()) == QStringLiteral("AnalysisTab"))
                continue;

            if (!page->property("legacyResponsiveInstalled").toBool()) {
                page->setProperty("legacyResponsiveInstalled", true);
                new LegacyPageFitter(page);
            }
        }
    }
};

void installGlobalResponsiveHook()
{
    QApplication *app = qobject_cast<QApplication*>(QCoreApplication::instance());
    if (!app)
        return;
    GlobalResponsiveInstaller *installer = new GlobalResponsiveInstaller(app);
    app->installEventFilter(installer);
}

}

Q_COREAPP_STARTUP_FUNCTION(installGlobalResponsiveHook)
