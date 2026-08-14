#include <QApplication>
#include <QCoreApplication>
#include <QEvent>
#include <QFontMetrics>
#include <QFrame>
#include <QGridLayout>
#include <QListWidget>
#include <QMainWindow>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QStyle>
#include <QTabWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

namespace {

static QWidget *pageOf(QWidget *tab)
{
    if (!tab) return nullptr;
    return tab;
}

static QIcon iconForPage(QMainWindow *window, QWidget *page, const QString &title)
{
    if (!window) return QIcon();
    QStyle *style = window->style();
    if (!style) return QIcon();

    const QString name = page ? page->objectName().toLower() : QString();
    const QString cls = page ? QString::fromLatin1(page->metaObject()->className()).toLower() : QString();
    const QString t = title.toLower();

    if (name == QStringLiteral("overview_tab") || t.contains(QStringLiteral("aper")))
        return style->standardIcon(QStyle::SP_DesktopIcon);
    if (name == QStringLiteral("emission_tab") || t.contains(QStringLiteral("régl")) || t.contains(QStringLiteral("regl")))
        return style->standardIcon(QStyle::SP_FileDialogDetailedView);
    if (name == QStringLiteral("errors") || t.contains(QStringLiteral("erreur")))
        return style->standardIcon(QStyle::SP_MessageBoxWarning);
    if (name == QStringLiteral("actuators") || t.contains(QStringLiteral("actionneur")))
        return style->standardIcon(QStyle::SP_BrowserReload);
    if (name == QStringLiteral("raw") || t.contains(QStringLiteral("donnée")) || t.contains(QStringLiteral("donnee")))
        return style->standardIcon(QStyle::SP_FileIcon);
    if (name == QStringLiteral("ecu") || t.contains(QStringLiteral("interactif")))
        return style->standardIcon(QStyle::SP_CommandLink);
    if (cls.contains(QStringLiteral("analysis")) || t.contains(QStringLiteral("analyse")))
        return style->standardIcon(QStyle::SP_FileDialogInfoView);
    if (cls.contains(QStringLiteral("diagnostic")) || t.contains(QStringLiteral("diagnostic")))
        return style->standardIcon(QStyle::SP_DialogApplyButton);
    if (t.contains(QStringLiteral("rosco")))
        return style->standardIcon(QStyle::SP_DriveNetIcon);
    if (t.contains(QStringLiteral("mesure")))
        return style->standardIcon(QStyle::SP_FileDialogContentsView);
    return style->standardIcon(QStyle::SP_ArrowRight);
}

static void polishNavigation(QMainWindow *window)
{
    QTabWidget *tabs = window ? window->findChild<QTabWidget*>(QStringLiteral("Tab_main")) : nullptr;
    QListWidget *nav = window ? window->findChild<QListWidget*>(QStringLiteral("uiRebuildNav")) : nullptr;
    if (!tabs || !nav) return;

    const QSignalBlocker blocker(nav);
    if (nav->count() != tabs->count()) nav->clear();

    QFont f = nav->font();
    f.setPointSizeF(qMax<qreal>(8.5, window->font().pointSizeF()));
    nav->setFont(f);
    const QFontMetrics fm(f);
    int longest = 0;

    for (int i = 0; i < tabs->count(); ++i) {
        const QString text = tabs->tabText(i).trimmed();
        QListWidgetItem *item = (i < nav->count()) ? nav->item(i) : nullptr;
        if (!item) {
            item = new QListWidgetItem(nav);
        }
        item->setText(text);
        QIcon icon = tabs->tabIcon(i);
        if (icon.isNull()) icon = iconForPage(window, pageOf(tabs->widget(i)), text);
        item->setIcon(icon);
        item->setSizeHint(QSize(0, qMax(34, fm.height() + 17)));
        longest = qMax(longest, fm.horizontalAdvance(text));
    }
    nav->setCurrentRow(tabs->currentIndex());
    nav->setIconSize(QSize(qMax(16, fm.height()), qMax(16, fm.height())));
    nav->setFixedWidth(qBound(172, longest + nav->iconSize().width() + 54, 258));
    nav->setSpacing(1);
    nav->setStyleSheet(QStringLiteral(
        "#uiRebuildNav{background:#0a1015;color:#cbd4da;border:0;border-right:1px solid #26313a;padding:7px 0;}"
        "#uiRebuildNav::item{padding:5px 12px;border-left:3px solid transparent;border-bottom:1px solid #11191f;}"
        "#uiRebuildNav::item:hover{background:#141d24;color:#ffffff;}"
        "#uiRebuildNav::item:selected{background:#1c211f;color:#ff9a32;border-left:3px solid #ff7a00;font-weight:700;}"));
}

static QGridLayout *gaugeGrid(QFrame *metrics, const QList<QWidget*> &gauges)
{
    if (!metrics || gauges.isEmpty()) return nullptr;
    const QList<QGridLayout*> grids = metrics->findChildren<QGridLayout*>();
    for (QGridLayout *grid : grids) {
        if (!grid) continue;
        int found = 0;
        for (QWidget *g : gauges) if (grid->indexOf(g) >= 0) ++found;
        if (found >= qMin(3, gauges.size())) return grid;
    }
    return nullptr;
}

static void polishSettings(QMainWindow *window)
{
    QWidget *page = window ? window->findChild<QWidget*>(QStringLiteral("emission_tab")) : nullptr;
    QFrame *metrics = window ? window->findChild<QFrame*>(QStringLiteral("settingsMetrics")) : nullptr;
    QFrame *states = window ? window->findChild<QFrame*>(QStringLiteral("settingsStates")) : nullptr;
    QFrame *adjust = window ? window->findChild<QFrame*>(QStringLiteral("settingsAdjust")) : nullptr;
    if (!page || !metrics || !states || !adjust) return;

    QVBoxLayout *root = qobject_cast<QVBoxLayout*>(page->layout());
    QGridLayout *body = nullptr;
    if (root) {
        for (int i = 0; i < root->count(); ++i) {
            QGridLayout *candidate = qobject_cast<QGridLayout*>(root->itemAt(i)->layout());
            if (candidate && candidate->indexOf(metrics) >= 0) { body = candidate; break; }
        }
    }

    if (body) {
        body->removeWidget(metrics);
        body->removeWidget(states);
        body->removeWidget(adjust);
        body->addWidget(metrics, 0, 0, 1, 2);
        body->addWidget(states, 1, 0);
        body->addWidget(adjust, 1, 1);
        body->setColumnStretch(0, 1);
        body->setColumnStretch(1, 2);
        body->setRowStretch(0, 3);
        body->setRowStretch(1, 2);
        body->setHorizontalSpacing(10);
        body->setVerticalSpacing(10);
    }

    metrics->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    states->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    adjust->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    states->setMinimumHeight(0);
    adjust->setMinimumHeight(0);

    QList<QWidget*> gauges;
    const QList<QWidget*> all = metrics->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
    for (QWidget *w : all) {
        if (w && QString::fromLatin1(w->metaObject()->className()) == QStringLiteral("CompactGauge")) gauges << w;
    }
    if (gauges.size() != 5) return;

    QGridLayout *grid = gaugeGrid(metrics, gauges);
    if (!grid) return;
    for (QWidget *g : gauges) grid->removeWidget(g);
    for (int c = 0; c < 5; ++c) grid->setColumnStretch(c, 1);
    grid->setRowStretch(0, 1);
    grid->setRowStretch(1, 0);
    grid->setHorizontalSpacing(7);
    grid->setVerticalSpacing(0);
    for (int i = 0; i < gauges.size(); ++i) {
        gauges.at(i)->setMinimumSize(72, 94);
        gauges.at(i)->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
        gauges.at(i)->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        grid->addWidget(gauges.at(i), 0, i);
    }
}

static void polishErrors(QMainWindow *window)
{
    QWidget *page = window ? window->findChild<QWidget*>(QStringLiteral("errors")) : nullptr;
    QFrame *stored = window ? window->findChild<QFrame*>(QStringLiteral("errorsStored")) : nullptr;
    QFrame *live = window ? window->findChild<QFrame*>(QStringLiteral("errorsLive")) : nullptr;
    if (!page || !stored || !live) return;

    stored->setStyleSheet(QStringLiteral(
        "#errorsStored{background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #121a20,stop:1 #0c1318);border:1px solid #2c3842;border-radius:6px;}"));
    live->setStyleSheet(QStringLiteral(
        "#errorsLive{background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #11191f,stop:1 #0a1116);border:1px solid #2c3842;border-radius:6px;}"));

    if (QVBoxLayout *root = qobject_cast<QVBoxLayout*>(page->layout())) {
        root->setContentsMargins(9, 8, 9, 8);
        root->setSpacing(8);
        const int a = root->indexOf(stored);
        const int b = root->indexOf(live);
        if (a >= 0) root->setStretch(a, 1);
        if (b >= 0) root->setStretch(b, 1);
    }
}

static void polishChrome(QMainWindow *window)
{
    if (!window) return;
    const QFontMetrics fm(window->font());
    if (QFrame *header = window->findChild<QFrame*>(QStringLiteral("uiRebuildHeader")))
        header->setFixedHeight(qBound(46, fm.height() * 3 + 10, 62));
    if (QFrame *status = window->findChild<QFrame*>(QStringLiteral("uiRebuildStatus")))
        status->setFixedHeight(qBound(30, fm.height() + 16, 40));
}

static void applyPolish(QMainWindow *window)
{
    if (!window) return;
    polishNavigation(window);
    polishSettings(window);
    polishErrors(window);
    polishChrome(window);
}

class RuntimeVisualPolishInstaller : public QObject
{
public:
    explicit RuntimeVisualPolishInstaller(QObject *parent = nullptr) : QObject(parent) {}
protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        QMainWindow *window = qobject_cast<QMainWindow*>(watched);
        if (!window || window->objectName() != QStringLiteral("MainWindow"))
            return QObject::eventFilter(watched, event);

        if ((event->type() == QEvent::Show || event->type() == QEvent::Polish) &&
            !window->property("runtimeVisualPolishScheduled").toBool()) {
            window->setProperty("runtimeVisualPolishScheduled", true);
            QTimer::singleShot(760, window, [window](){ applyPolish(window); });
            QTimer::singleShot(980, window, [window](){ applyPolish(window); });
        } else if (event->type() == QEvent::Resize &&
                   window->property("runtimeVisualPolishScheduled").toBool()) {
            QTimer::singleShot(25, window, [window](){ applyPolish(window); });
        }
        return QObject::eventFilter(watched, event);
    }
};

void installRuntimeVisualPolish()
{
    QApplication *app = qobject_cast<QApplication*>(QCoreApplication::instance());
    if (app) app->installEventFilter(new RuntimeVisualPolishInstaller(app));
}

}

Q_COREAPP_STARTUP_FUNCTION(installRuntimeVisualPolish)
