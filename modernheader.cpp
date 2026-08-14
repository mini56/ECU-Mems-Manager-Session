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
    static QString infoStyle(int pad=6)
    {
        return QStringLiteral(
            "QLabel{color:#cfd5da;background:transparent;border:0;"
            "padding:1px %1px;font-weight:600;}"
        ).arg(pad);
    }

    static QLabel *makeInfoLabel(const QString &text, QWidget *parent)
    {
        QLabel *label=new QLabel(text,parent);
        label->setAlignment(Qt::AlignCenter);
        label->setStyleSheet(infoStyle());
        return label;
    }

    static QFrame *separator(QWidget *parent)
    {
        QFrame *line=new QFrame(parent);
        line->setFrameShape(QFrame::VLine);
        line->setFrameShadow(QFrame::Plain);
        line->setStyleSheet(QStringLiteral("color:#252d35;background:#252d35;"));
        line->setFixedWidth(1);
        return line;
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
            "#mockupTopHeader #m_connectButton{background:#141a20;color:#cfd5da;border:1px solid #3a434c;"
            "border-radius:0px;padding:2px 8px;font-weight:700;}"
            "#mockupTopHeader #m_connectButton:hover{color:#ff9b32;border-color:#ff8a1c;}"
            "#mockupTopHeader #m_connectButton:pressed{background:#1b2229;}"
            "#mockupTopHeader #m_connectButton:disabled{background:#151a1f;color:#535c65;border-color:#252c33;}"
            "#mockupTopHeader #m_disconnectButton{background:#d92f2f;color:#ffffff;border:1px solid #ef4949;"
            "border-radius:0px;padding:2px 8px;font-weight:700;}"
            "#mockupTopHeader #m_disconnectButton:hover{background:#e83b3b;border-color:#ff6262;}"
            "#mockupTopHeader #m_disconnectButton:pressed{background:#b52323;}"
            "#mockupTopHeader #m_disconnectButton:disabled{background:#151a1f;color:#535c65;border-color:#252c33;}"
        );

        QHBoxLayout *layout=new QHBoxLayout(bar);
        layout->setContentsMargins(7,3,7,3);
        layout->setSpacing(4);

        QWidget *brandBox=new QWidget(bar);
        QHBoxLayout *brandLayout=new QHBoxLayout(brandBox);
        brandLayout->setContentsMargins(0,0,0,0);
        brandLayout->setSpacing(4);

        QLabel *logo=new QLabel(brandBox);
        logo->setObjectName(QStringLiteral("mockupLogoLabel"));
        logo->setAlignment(Qt::AlignCenter);
        const QPixmap logoSource(QStringLiteral(":/icons/key.png"));
        if (!logoSource.isNull()) logo->setPixmap(logoSource.scaled(22,22,Qt::KeepAspectRatio,Qt::SmoothTransformation));
        logo->setFixedSize(24,24);
        brandLayout->addWidget(logo);

        QLabel *brand=new QLabel(QStringLiteral("ECU MEMS MANAGER"),brandBox);
        brand->setStyleSheet("color:#f3f5f7;font-weight:700;letter-spacing:.4px;background:transparent;");
        QLabel *version=new QLabel(QStringLiteral("v%1.%2.%3").arg(VER_MAJOR).arg(VER_MINOR).arg(VER_PATCH),brandBox);
        version->setStyleSheet("color:#7f8992;font-weight:600;background:transparent;");
        brandLayout->addWidget(brand);
        brandLayout->addWidget(version);
        layout->addWidget(brandBox,0,Qt::AlignVCenter);
        layout->addWidget(separator(bar));

        ecuLabel->setParent(bar);
        ecuLabel->setAlignment(Qt::AlignCenter);
        ecuLabel->setStyleSheet(infoStyle());
        layout->addWidget(ecuLabel,0,Qt::AlignVCenter);
        layout->addWidget(separator(bar));

        QLabel *portLabel=makeInfoLabel(QStringLiteral("Port\n%1").arg(configuredSerialPort()),bar);
        portLabel->setObjectName(QStringLiteral("mockupPortLabel"));
        layout->addWidget(portLabel,0,Qt::AlignVCenter);
        layout->addWidget(separator(bar));

        QLabel *frequencyLabel=makeInfoLabel(QStringLiteral("Fréquence\nauto"),bar);
        frequencyLabel->setObjectName(QStringLiteral("mockupFrequencyLabel"));
        layout->addWidget(frequencyLabel,0,Qt::AlignVCenter);

        layout->addStretch(1);

        QWidget *commBox=new QWidget(bar);
        commBox->setObjectName(QStringLiteral("mockupCommunicationBox"));
        commBox->setStyleSheet("#mockupCommunicationBox{background:transparent;border:0;}");
        QHBoxLayout *commLayout=new QHBoxLayout(commBox);
        commLayout->setContentsMargins(4,0,4,0);
        commLayout->setSpacing(4);
        commLabel->setParent(commBox);
        commLabel->setStyleSheet("color:#bfc7ce;background:transparent;border:0;font-weight:600;");
        commLayout->addWidget(commLabel);
        if (goodLed) { goodLed->setParent(commBox); commLayout->addWidget(goodLed); goodLed->show(); }
        if (badLed) { badLed->setParent(commBox); commLayout->addWidget(badLed); badLed->show(); }
        layout->addWidget(commBox,0,Qt::AlignVCenter);

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
            const int headerH=qBound(31,qRound(38.0*scale),42);
            if (bar->height()!=headerH) bar->setFixedHeight(headerH);

            const int margin=qBound(4,qRound(7.0*scale),8);
            const int spacing=qBound(2,qRound(4.0*scale),5);
            layout->setContentsMargins(margin,qMax(2,qRound(3.0*scale)),margin,qMax(2,qRound(3.0*scale)));
            layout->setSpacing(spacing);

            const int logoSize=qBound(17,qRound(22.0*scale),25);
            logo->setFixedSize(logoSize+2,logoSize+2);
            if (!logoSource.isNull()) logo->setPixmap(logoSource.scaled(logoSize,logoSize,Qt::KeepAspectRatio,Qt::SmoothTransformation));

            QFont bf=brand->font();
            bf.setPointSizeF(qBound<qreal>(6.8,8.4*scale,9.5));
            brand->setFont(bf);
            QFont vf=version->font();
            vf.setPointSizeF(qBound<qreal>(6.0,6.9*scale,7.8));
            version->setFont(vf);

            const int infoPad=qBound(2,qRound(5.0*scale),6);
            const QString infoCss=infoStyle(infoPad);
            ecuLabel->setStyleSheet(infoCss);
            portLabel->setStyleSheet(infoCss);
            frequencyLabel->setStyleSheet(infoCss);

            const int h=qBound(21,qRound(24.0*scale),27);
            const int pad=qBound(12,qRound(16.0*scale),19);
            connectButton->setFixedHeight(h);
            disconnectButton->setFixedHeight(h);
            connectButton->setFixedWidth(qMax(62,connectButton->fontMetrics().horizontalAdvance(connectButton->text())+pad));
            disconnectButton->setFixedWidth(qMax(70,disconnectButton->fontMetrics().horizontalAdvance(disconnectButton->text())+pad));

            portLabel->setText(QStringLiteral("Port\n%1").arg(configuredSerialPort()));
        };

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
