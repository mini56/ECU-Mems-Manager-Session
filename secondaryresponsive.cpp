#include <QApplication>
#include <QColorDialog>
#include <QCoreApplication>
#include <QEvent>
#include <QFileDialog>
#include <QFont>
#include <QFontDialog>
#include <QLayout>
#include <QMainWindow>
#include <QMessageBox>
#include <QPointer>
#include <QRect>
#include <QSize>
#include <QSplashScreen>
#include <QTimer>
#include <QVariant>
#include <QWidget>
#include <QtMath>

namespace {

static const qreal kGlobalMinScale = 0.62;
static const qreal kGlobalMaxScale = 1.16;

static qreal globalScaleFromMainWindow()
{
    const QList<QWidget*> topLevels = QApplication::topLevelWidgets();
    for (QWidget *w : topLevels) {
        QMainWindow *mw = qobject_cast<QMainWindow*>(w);
        if (!mw || mw->objectName() != QStringLiteral("MainWindow"))
            continue;
        const QVariant v = mw->property("globalUiScale");
        if (v.isValid())
            return qBound<qreal>(kGlobalMinScale, v.toDouble(), kGlobalMaxScale);
    }
    return 1.0;
}

static bool isNativeOrTransientSystemDialog(QWidget *w)
{
    return qobject_cast<QMessageBox*>(w) ||
           qobject_cast<QFileDialog*>(w) ||
           qobject_cast<QColorDialog*>(w) ||
           qobject_cast<QFontDialog*>(w) ||
           qobject_cast<QSplashScreen*>(w);
}

static void captureAbsoluteChildren(QWidget *window)
{
    const QList<QWidget*> children = window->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
    for (QWidget *child : children) {
        if (!child) continue;
        if (!child->property("secondaryResponsiveBaseGeometry").isValid())
            child->setProperty("secondaryResponsiveBaseGeometry", child->geometry());
        if (!child->property("secondaryResponsiveBaseFont").isValid()) {
            const qreal ps = child->font().pointSizeF();
            if (ps > 0.0)
                child->setProperty("secondaryResponsiveBaseFont", ps);
        }
    }
}

static void scaleFontsRecursively(QWidget *root, qreal scale)
{
    const QList<QWidget*> children = root->findChildren<QWidget*>();
    for (QWidget *child : children) {
        if (!child) continue;
        if (!child->property("secondaryResponsiveBaseFont").isValid()) {
            const qreal ps = child->font().pointSizeF();
            if (ps > 0.0)
                child->setProperty("secondaryResponsiveBaseFont", ps);
        }
        const QVariant fv = child->property("secondaryResponsiveBaseFont");
        if (!fv.isValid()) continue;
        QFont f = child->font();
        f.setPointSizeF(qMax<qreal>(6.5, fv.toDouble() * scale));
        child->setFont(f);
    }
}

static void applyScale(QWidget *window)
{
    if (!window || isNativeOrTransientSystemDialog(window)) return;
    if (window->objectName() == QStringLiteral("MainWindow")) return;

    const qreal scale = globalScaleFromMainWindow();

    if (!window->property("secondaryResponsiveBaseSize").isValid()) {
        const QSize base = window->size().expandedTo(window->sizeHint());
        window->setProperty("secondaryResponsiveBaseSize", base);
        const qreal ps = window->font().pointSizeF();
        if (ps > 0.0)
            window->setProperty("secondaryResponsiveBaseFont", ps);
        if (!window->layout())
            captureAbsoluteChildren(window);
    }

    const QSize base = window->property("secondaryResponsiveBaseSize").toSize();
    if (!base.isValid()) return;

    window->setMinimumSize(0, 0);
    window->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);

    const QSize target(qMax(240, qRound(base.width() * scale)),
                       qMax(160, qRound(base.height() * scale)));
    window->resize(target);

    const QVariant rootFont = window->property("secondaryResponsiveBaseFont");
    if (rootFont.isValid()) {
        QFont f = window->font();
        f.setPointSizeF(qMax<qreal>(6.5, rootFont.toDouble() * scale));
        window->setFont(f);
    }

    if (!window->layout()) {
        const QList<QWidget*> children = window->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
        for (QWidget *child : children) {
            const QVariant gv = child->property("secondaryResponsiveBaseGeometry");
            if (!gv.isValid()) continue;
            const QRect baseRect = gv.toRect();
            child->setGeometry(qRound(baseRect.x() * scale),
                               qRound(baseRect.y() * scale),
                               qMax(1, qRound(baseRect.width() * scale)),
                               qMax(1, qRound(baseRect.height() * scale)));
        }
    }

    scaleFontsRecursively(window, scale);
    window->setProperty("secondaryResponsiveScale", scale);
}

class SecondaryResponsiveInstaller : public QObject
{
public:
    explicit SecondaryResponsiveInstaller(QObject *parent = nullptr) : QObject(parent) {}

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        QWidget *window = qobject_cast<QWidget*>(watched);
        if (!window) return QObject::eventFilter(watched, event);

        if (event->type() == QEvent::Show && window->isWindow() &&
            window->objectName() != QStringLiteral("MainWindow") &&
            !isNativeOrTransientSystemDialog(window)) {
            if (!window->property("secondaryResponsiveInstalled").toBool()) {
                window->setProperty("secondaryResponsiveInstalled", true);
                QPointer<QWidget> safe(window);
                QTimer::singleShot(0, window, [safe]() {
                    if (safe) applyScale(safe);
                });
            }
        }
        return QObject::eventFilter(watched, event);
    }
};

void installSecondaryResponsiveHook()
{
    QApplication *app = qobject_cast<QApplication*>(QCoreApplication::instance());
    if (!app) return;
    SecondaryResponsiveInstaller *installer = new SecondaryResponsiveInstaller(app);
    app->installEventFilter(installer);
}

}

Q_COREAPP_STARTUP_FUNCTION(installSecondaryResponsiveHook)
