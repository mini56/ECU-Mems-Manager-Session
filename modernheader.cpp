#include <QApplication>
#include <QCoreApplication>
#include <QEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QMenuBar>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include <QtMath>

namespace {

class ModernHeaderInstaller : public QObject
{
public:
    explicit ModernHeaderInstaller(QObject *parent=nullptr) : QObject(parent) {}

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if ((event->type()==QEvent::Show || event->type()==QEvent::Polish) && watched) {
            QMainWindow *window=qobject_cast<QMainWindow*>(watched);
            if (window && window->objectName()==QStringLiteral("MainWindow") &&
                !window->property("modernHeaderInstalled").toBool()) {
                window->setProperty("modernHeaderInstalled", true);
                QTimer::singleShot(420, window, [window](){ install(window); });
            }
        }
        return QObject::eventFilter(watched,event);
    }

private:
    static void install(QMainWindow *window)
    {
        QWidget *central=window->centralWidget();
        QVBoxLayout *root=central ? qobject_cast<QVBoxLayout*>(central->layout()) : nullptr;
        if (!central || !root) return;

        if (window->menuBar()) window->menuBar()->hide();

        // The approved mock-up uses the REAL connection controls. Reuse the
        // existing functional connection bar instead of creating mirror/fake controls.
        QWidget *legacy=central->findChild<QWidget*>(QStringLiteral("layoutWidget_7"));
        if (!legacy) return;

        QFrame *bar=new QFrame(central);
        bar->setObjectName(QStringLiteral("mockupTopHeader"));
        bar->setStyleSheet(
            "#mockupTopHeader{background:#0e1318;border-bottom:1px solid #29313a;}"
            "#mockupTopHeader QLabel{color:#dce2e7;background:transparent;border:0;}"
            "#mockupTopHeader QPushButton{background:#1769d2;color:#ffffff;border:1px solid #2d7ee8;"
            "border-radius:4px;padding:4px 10px;font-weight:600;}"
            "#mockupTopHeader QPushButton:hover{background:#2378e6;border-color:#4c91ef;}"
            "#mockupTopHeader QPushButton:pressed{background:#1257b0;}"
            "#mockupTopHeader QPushButton:disabled{background:#252b31;color:#707983;border-color:#333b43;}"
        );

        QHBoxLayout *layout=new QHBoxLayout(bar);
        layout->setContentsMargins(10,5,10,5);
        layout->setSpacing(12);

        QLabel *brand=new QLabel(QStringLiteral("ECU MEMS MANAGER"),bar);
        brand->setObjectName(QStringLiteral("mockupBrandLabel"));
        brand->setStyleSheet("color:#f3f5f7;font-weight:700;letter-spacing:.5px;background:transparent;");
        layout->addWidget(brand);
        layout->addSpacing(10);

        // Reparent the original functional bar. This preserves every signal,
        // slot, live LED and ECU/connection state exactly as before.
        legacy->setParent(bar);
        legacy->setMinimumSize(0,0);
        legacy->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
        legacy->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
        legacy->setStyleSheet(
            "QWidget{background:transparent;color:#dce2e7;}"
            "QLabel{color:#cfd5da;background:transparent;border:0;}"
            "QPushButton{background:#1769d2;color:#fff;border:1px solid #2d7ee8;border-radius:4px;"
            "padding:4px 10px;font-weight:600;}"
            "QPushButton:hover{background:#2378e6;}"
            "QPushButton:disabled{background:#252b31;color:#707983;border-color:#333b43;}"
        );

        const QList<QPushButton*> realButtons=legacy->findChildren<QPushButton*>();
        for (QPushButton *button : realButtons) {
            button->setMinimumWidth(0);
            button->setMaximumWidth(QWIDGETSIZE_MAX);
            button->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Preferred);
        }

        layout->addWidget(legacy,1);
        root->insertWidget(0,bar);
        legacy->show();

        QTimer *sync=new QTimer(bar);
        sync->setInterval(250);
        QObject::connect(sync,&QTimer::timeout,bar,[=](){
            const qreal scale=window->property("globalUiScale").isValid()
                ? window->property("globalUiScale").toDouble() : 1.0;
            const int headerH=qBound(34,qRound(42.0*scale),49);
            if (bar->height()!=headerH) bar->setFixedHeight(headerH);

            const int margin=qBound(5,qRound(10.0*scale),11);
            const int spacing=qBound(5,qRound(12.0*scale),13);
            layout->setContentsMargins(margin,qMax(3,qRound(5.0*scale)),margin,qMax(3,qRound(5.0*scale)));
            layout->setSpacing(spacing);

            QFont bf=brand->font();
            bf.setPointSizeF(qBound<qreal>(7.0,9.0*scale,10.5));
            brand->setFont(bf);

            for (QPushButton *button : realButtons) {
                const int h=qBound(22,qRound(28.0*scale),32);
                button->setMinimumHeight(h);
                button->setMaximumHeight(h);
            }
        });
        sync->start();
    }
};

void installModernHeaderHook()
{
    QApplication *app=qobject_cast<QApplication*>(QCoreApplication::instance());
    if (!app) return;
    ModernHeaderInstaller *installer=new ModernHeaderInstaller(app);
    app->installEventFilter(installer);
}

}

Q_COREAPP_STARTUP_FUNCTION(installModernHeaderHook)
