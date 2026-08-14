#include <QApplication>
#include <QCoreApplication>
#include <QEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QMenuBar>
#include <QPushButton>
#include <QSettings>
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
    static QLabel *makeInfoChip(const QString &text, QWidget *parent)
    {
        QLabel *label=new QLabel(text,parent);
        label->setAlignment(Qt::AlignCenter);
        label->setStyleSheet(
            "QLabel{color:#cfd5da;background:#121820;border:1px solid #303943;"
            "border-radius:3px;padding:3px 8px;font-weight:600;}"
        );
        return label;
    }

    static QString configuredSerialPort()
    {
        QSettings settings(QSettings::IniFormat,QSettings::UserScope,PROJECTNAME);
        settings.beginGroup(QStringLiteral("Settings"));
        const QString port=settings.value(QStringLiteral("SerialDevice"),QString()).toString().trimmed();
        settings.endGroup();
        return port.isEmpty() ? QStringLiteral("--") : port;
    }

    static void install(QMainWindow *window)
    {
        QWidget *central=window->centralWidget();
        QVBoxLayout *root=central ? qobject_cast<QVBoxLayout*>(central->layout()) : nullptr;
        if (!central || !root) return;

        if (window->menuBar()) window->menuBar()->hide();

        QWidget *legacy=central->findChild<QWidget*>(QStringLiteral("layoutWidget_7"));
        if (!legacy) return;

        QFrame *bar=new QFrame(central);
        bar->setObjectName(QStringLiteral("mockupTopHeader"));
        bar->setStyleSheet(
            "#mockupTopHeader{background:#0b1015;border-bottom:1px solid #29313a;}"
            "#mockupTopHeader QLabel{color:#dce2e7;background:transparent;border:0;}"
            "#mockupTopHeader QPushButton{background:#1769d2;color:#ffffff;border:1px solid #2d7ee8;"
            "border-radius:3px;padding:3px 8px;font-weight:600;}"
            "#mockupTopHeader QPushButton:hover{background:#2378e6;border-color:#4c91ef;}"
            "#mockupTopHeader QPushButton:pressed{background:#1257b0;}"
            "#mockupTopHeader QPushButton:disabled{background:#252b31;color:#707983;border-color:#333b43;}"
        );

        QHBoxLayout *layout=new QHBoxLayout(bar);
        layout->setContentsMargins(9,4,9,4);
        layout->setSpacing(8);

        QWidget *brandBox=new QWidget(bar);
        QHBoxLayout *brandLayout=new QHBoxLayout(brandBox);
        brandLayout->setContentsMargins(0,0,0,0);
        brandLayout->setSpacing(6);

        QLabel *brand=new QLabel(QStringLiteral("ECU MEMS MANAGER"),brandBox);
        brand->setObjectName(QStringLiteral("mockupBrandLabel"));
        brand->setStyleSheet("color:#f3f5f7;font-weight:700;letter-spacing:.5px;background:transparent;");
        brandLayout->addWidget(brand);

        QLabel *version=new QLabel(QStringLiteral("v%1.%2.%3").arg(VER_MAJOR).arg(VER_MINOR).arg(VER_PATCH),brandBox);
        version->setObjectName(QStringLiteral("mockupVersionLabel"));
        version->setStyleSheet("color:#7f8992;font-weight:600;background:transparent;");
        brandLayout->addWidget(version);
        layout->addWidget(brandBox,0,Qt::AlignVCenter);

        QLabel *portChip=makeInfoChip(QStringLiteral("Port : %1").arg(configuredSerialPort()),bar);
        portChip->setObjectName(QStringLiteral("mockupPortLabel"));
        layout->addWidget(portChip,0,Qt::AlignVCenter);

        // MEMS polling runs continuously without a fixed sleep in the service loop;
        // therefore no fabricated numeric Hz value is shown.
        QLabel *frequencyChip=makeInfoChip(QStringLiteral("Fréquence : auto"),bar);
        frequencyChip->setObjectName(QStringLiteral("mockupFrequencyLabel"));
        layout->addWidget(frequencyChip,0,Qt::AlignVCenter);

        legacy->setParent(bar);
        legacy->setMinimumSize(0,0);
        legacy->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
        legacy->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
        legacy->setStyleSheet(
            "QWidget{background:transparent;color:#dce2e7;}"
            "QLabel{color:#cfd5da;background:transparent;border:0;}"
            "QPushButton{background:#1769d2;color:#fff;border:1px solid #2d7ee8;border-radius:3px;"
            "padding:3px 8px;font-weight:600;}"
            "QPushButton:hover{background:#2378e6;}"
            "QPushButton:disabled{background:#252b31;color:#707983;border-color:#333b43;}"
        );

        const QList<QPushButton*> realButtons=legacy->findChildren<QPushButton*>();
        for (QPushButton *button : realButtons) {
            button->setMinimumWidth(0);
            button->setMaximumWidth(QWIDGETSIZE_MAX);
            button->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Preferred);
        }
        if (QLayout *legacyLayout=legacy->layout()) {
            legacyLayout->setContentsMargins(0,0,0,0);
            legacyLayout->setSpacing(7);
        }

        layout->addWidget(legacy,1);
        root->insertWidget(0,bar);
        legacy->show();

        QTimer *sync=new QTimer(bar);
        sync->setInterval(200);
        QObject::connect(sync,&QTimer::timeout,bar,[=](){
            const qreal scale=window->property("globalUiScale").isValid()
                ? window->property("globalUiScale").toDouble() : 1.0;
            const int headerH=qBound(32,qRound(40.0*scale),46);
            if (bar->height()!=headerH) bar->setFixedHeight(headerH);

            const int margin=qBound(5,qRound(9.0*scale),10);
            const int spacing=qBound(4,qRound(8.0*scale),9);
            layout->setContentsMargins(margin,qMax(3,qRound(4.0*scale)),margin,qMax(3,qRound(4.0*scale)));
            layout->setSpacing(spacing);

            QFont bf=brand->font();
            bf.setPointSizeF(qBound<qreal>(7.0,9.0*scale,10.2));
            brand->setFont(bf);
            QFont vf=version->font();
            vf.setPointSizeF(qBound<qreal>(6.2,7.3*scale,8.2));
            version->setFont(vf);

            const int chipPad=qBound(4,qRound(8.0*scale),9);
            const QString chipStyle=QStringLiteral(
                "QLabel{color:#cfd5da;background:#121820;border:1px solid #303943;"
                "border-radius:3px;padding:3px %1px;font-weight:600;}"
            ).arg(chipPad);
            portChip->setStyleSheet(chipStyle);
            frequencyChip->setStyleSheet(chipStyle);

            for (QPushButton *button : realButtons) {
                const int h=qBound(22,qRound(27.0*scale),31);
                const int pad=qBound(14,qRound(18.0*scale),22);
                const int w=qMax(62,button->fontMetrics().horizontalAdvance(button->text())+pad);
                button->setFixedHeight(h);
                button->setFixedWidth(w);
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
