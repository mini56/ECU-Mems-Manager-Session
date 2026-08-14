#include <QApplication>
#include <QCoreApplication>
#include <QEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QMenuBar>
#include <QPixmap>
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
                QTimer::singleShot(0, window, [window](){ install(window); });
            }
        }
        return QObject::eventFilter(watched,event);
    }

private:
    static QString infoStyle(int pad=8)
    {
        return QStringLiteral(
            "QLabel{color:#cfd5da;background:#121820;border:1px solid #303943;"
            "border-radius:3px;padding:3px %1px;font-weight:600;}"
        ).arg(pad);
    }

    static QLabel *makeInfoChip(const QString &text, QWidget *parent)
    {
        QLabel *label=new QLabel(text,parent);
        label->setAlignment(Qt::AlignCenter);
        label->setStyleSheet(infoStyle());
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

        QPushButton *connectButton=legacy->findChild<QPushButton*>(QStringLiteral("m_connectButton"));
        QPushButton *disconnectButton=legacy->findChild<QPushButton*>(QStringLiteral("m_disconnectButton"));
        QLabel *ecuLabel=legacy->findChild<QLabel*>(QStringLiteral("m_ecuIdLabel"));
        QLabel *commLabel=legacy->findChild<QLabel*>(QStringLiteral("m_communicationsStatusLabel"));
        QWidget *goodLed=legacy->findChild<QWidget*>(QStringLiteral("m_commsGoodLed"));
        QWidget *badLed=legacy->findChild<QWidget*>(QStringLiteral("m_commsBadLed"));
        if (!connectButton || !disconnectButton || !ecuLabel || !commLabel) return;

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
        layout->setSpacing(7);

        QWidget *brandBox=new QWidget(bar);
        QHBoxLayout *brandLayout=new QHBoxLayout(brandBox);
        brandLayout->setContentsMargins(0,0,0,0);
        brandLayout->setSpacing(5);

        QLabel *logo=new QLabel(brandBox);
        logo->setObjectName(QStringLiteral("mockupLogoLabel"));
        logo->setAlignment(Qt::AlignCenter);
        const QPixmap logoSource(QStringLiteral(":/icons/key.png"));
        if (!logoSource.isNull()) logo->setPixmap(logoSource.scaled(24,24,Qt::KeepAspectRatio,Qt::SmoothTransformation));
        logo->setFixedSize(26,26);
        brandLayout->addWidget(logo);

        QLabel *brand=new QLabel(QStringLiteral("ECU MEMS MANAGER"),brandBox);
        brand->setStyleSheet("color:#f3f5f7;font-weight:700;letter-spacing:.5px;background:transparent;");
        QLabel *version=new QLabel(QStringLiteral("v%1.%2.%3").arg(VER_MAJOR).arg(VER_MINOR).arg(VER_PATCH),brandBox);
        version->setStyleSheet("color:#7f8992;font-weight:600;background:transparent;");
        brandLayout->addWidget(brand);
        brandLayout->addWidget(version);
        layout->addWidget(brandBox,0,Qt::AlignVCenter);

        ecuLabel->setParent(bar);
        ecuLabel->setAlignment(Qt::AlignCenter);
        ecuLabel->setStyleSheet(infoStyle());
        layout->addWidget(ecuLabel,0,Qt::AlignVCenter);

        QLabel *portChip=makeInfoChip(QStringLiteral("Port : %1").arg(configuredSerialPort()),bar);
        portChip->setObjectName(QStringLiteral("mockupPortLabel"));
        layout->addWidget(portChip,0,Qt::AlignVCenter);

        QLabel *frequencyChip=makeInfoChip(QStringLiteral("Fréquence : auto"),bar);
        frequencyChip->setObjectName(QStringLiteral("mockupFrequencyLabel"));
        layout->addWidget(frequencyChip,0,Qt::AlignVCenter);

        QWidget *commBox=new QWidget(bar);
        commBox->setObjectName(QStringLiteral("mockupCommunicationBox"));
        commBox->setStyleSheet("#mockupCommunicationBox{background:#121820;border:1px solid #303943;border-radius:3px;}");
        QHBoxLayout *commLayout=new QHBoxLayout(commBox);
        commLayout->setContentsMargins(7,2,6,2);
        commLayout->setSpacing(4);
        commLabel->setParent(commBox);
        commLabel->setStyleSheet("color:#cfd5da;background:transparent;border:0;font-weight:600;");
        commLayout->addWidget(commLabel);
        if (goodLed) { goodLed->setParent(commBox); commLayout->addWidget(goodLed); goodLed->show(); }
        if (badLed) { badLed->setParent(commBox); commLayout->addWidget(badLed); badLed->show(); }
        layout->addWidget(commBox,0,Qt::AlignVCenter);

        layout->addStretch(1);

        connectButton->setParent(bar);
        disconnectButton->setParent(bar);
        connectButton->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Preferred);
        disconnectButton->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Preferred);
        layout->addWidget(connectButton,0,Qt::AlignVCenter);
        layout->addWidget(disconnectButton,0,Qt::AlignVCenter);

        legacy->hide();
        legacy->setMaximumSize(0,0);

        root->insertWidget(0,bar);
        bar->show();

        const auto syncHeader = [=](){
            const qreal scale=window->property("globalUiScale").isValid()
                ? window->property("globalUiScale").toDouble() : 1.0;
            const int headerH=qBound(32,qRound(40.0*scale),46);
            if (bar->height()!=headerH) bar->setFixedHeight(headerH);

            const int margin=qBound(5,qRound(9.0*scale),10);
            const int spacing=qBound(4,qRound(7.0*scale),8);
            layout->setContentsMargins(margin,qMax(3,qRound(4.0*scale)),margin,qMax(3,qRound(4.0*scale)));
            layout->setSpacing(spacing);

            const int logoSize=qBound(18,qRound(24.0*scale),28);
            logo->setFixedSize(logoSize+2,logoSize+2);
            if (!logoSource.isNull()) logo->setPixmap(logoSource.scaled(logoSize,logoSize,Qt::KeepAspectRatio,Qt::SmoothTransformation));

            QFont bf=brand->font();
            bf.setPointSizeF(qBound<qreal>(7.0,9.0*scale,10.2));
            brand->setFont(bf);
            QFont vf=version->font();
            vf.setPointSizeF(qBound<qreal>(6.2,7.3*scale,8.2));
            version->setFont(vf);

            const int chipPad=qBound(4,qRound(8.0*scale),9);
            const QString chipCss=infoStyle(chipPad);
            ecuLabel->setStyleSheet(chipCss);
            portChip->setStyleSheet(chipCss);
            frequencyChip->setStyleSheet(chipCss);

            const int h=qBound(22,qRound(27.0*scale),31);
            const int pad=qBound(14,qRound(18.0*scale),22);
            connectButton->setFixedHeight(h);
            disconnectButton->setFixedHeight(h);
            connectButton->setFixedWidth(qMax(62,connectButton->fontMetrics().horizontalAdvance(connectButton->text())+pad));
            disconnectButton->setFixedWidth(qMax(62,disconnectButton->fontMetrics().horizontalAdvance(disconnectButton->text())+pad));

            portChip->setText(QStringLiteral("Port : %1").arg(configuredSerialPort()));
        };

        // Apply the final geometry before the first periodic synchronization tick.
        syncHeader();

        QTimer *sync=new QTimer(bar);
        sync->setInterval(200);
        QObject::connect(sync,&QTimer::timeout,bar,syncHeader);
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
