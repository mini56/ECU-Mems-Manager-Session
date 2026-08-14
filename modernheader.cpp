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
        line->setStyleSheet(QStringLiteral("color:#28323b;background:#28323b;border:0;"));
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
        bar->setAttribute(Qt::WA_StyledBackground,true);
        bar->setStyleSheet(
            "#mockupTopHeader{background:#090e13;border:0;border-bottom:1px solid #29313a;}"
            "#mockupTopHeader QWidget{background:transparent;}"
            "#mockupTopHeader QLabel{color:#dce2e7;background:transparent;border:0;}"
            "#mockupTopHeader #brandBox{background:transparent;border:0;}"
            "#mockupTopHeader #m_connectButton{background:#ff7a00;color:#101419;border:1px solid #ff9828;"
            "border-radius:3px;padding:4px 12px;font-weight:800;}"
            "#mockupTopHeader #m_connectButton:hover{background:#ff8b18;border-color:#ffab4d;}"
            "#mockupTopHeader #m_connectButton:pressed{background:#e56b00;}"
            "#mockupTopHeader #m_connectButton:disabled{background:#171d23;color:#59636c;border-color:#29323a;}"
            "#mockupTopHeader #m_disconnectButton{background:#b82424;color:#ffffff;border:1px solid #e13b3b;"
            "border-radius:3px;padding:4px 12px;font-weight:800;}"
            "#mockupTopHeader #m_disconnectButton:hover{background:#d22e2e;border-color:#f24c4c;}"
            "#mockupTopHeader #m_disconnectButton:pressed{background:#941c1c;}"
            "#mockupTopHeader #m_disconnectButton:disabled{background:#171d23;color:#59636c;border-color:#29323a;}"
        );

        QHBoxLayout *layout=new QHBoxLayout(bar);
        layout->setContentsMargins(10,4,9,4);
        layout->setSpacing(7);

        QWidget *brandBox=new QWidget(bar);
        brandBox->setObjectName(QStringLiteral("brandBox"));
        brandBox->setAttribute(Qt::WA_StyledBackground,true);
        brandBox->setStyleSheet(QStringLiteral("#brandBox{background:transparent;border:0;}"));
        QHBoxLayout *brandLayout=new QHBoxLayout(brandBox);
        brandLayout->setContentsMargins(0,0,0,0);
        brandLayout->setSpacing(7);

        QLabel *logo=new QLabel(brandBox);
        logo->setObjectName(QStringLiteral("mockupLogoLabel"));
        logo->setAlignment(Qt::AlignCenter);
        logo->setStyleSheet(QStringLiteral("background:transparent;border:0;"));
        const QPixmap logoSource(QStringLiteral(":/icons/key.png"));
        if (!logoSource.isNull()) logo->setPixmap(logoSource.scaled(30,30,Qt::KeepAspectRatio,Qt::SmoothTransformation));
        logo->setFixedSize(34,34);
        brandLayout->addWidget(logo);

        QVBoxLayout *brandTextLayout=new QVBoxLayout;
        brandTextLayout->setContentsMargins(0,0,0,0);
        brandTextLayout->setSpacing(0);
        QLabel *brand=new QLabel(QStringLiteral("ECU MEMS MANAGER"),brandBox);
        brand->setStyleSheet(QStringLiteral("color:#f4f6f8;font-weight:800;letter-spacing:.6px;background:transparent;border:0;"));
        QLabel *version=new QLabel(QStringLiteral("v%1.%2.%3").arg(VER_MAJOR).arg(VER_MINOR).arg(VER_PATCH),brandBox);
        version->setStyleSheet(QStringLiteral("color:#ff9828;font-weight:700;background:transparent;border:0;"));
        brandTextLayout->addWidget(brand);
        brandTextLayout->addWidget(version);
        brandLayout->addLayout(brandTextLayout);
        layout->addWidget(brandBox,0,Qt::AlignVCenter);
        layout->addWidget(separator(bar));

        ecuLabel->setParent(bar);
        ecuLabel->setAlignment(Qt::AlignCenter);
        ecuLabel->setStyleSheet(infoStyle());
        layout->addWidget(ecuLabel,0,Qt::AlignVCenter);
        layout->addWidget(separator(bar));

        QLabel *portLabel=makeInfoLabel(QStringLiteral("Port : %1").arg(configuredSerialPort()),bar);
        portLabel->setObjectName(QStringLiteral("mockupPortLabel"));
        layout->addWidget(portLabel,0,Qt::AlignVCenter);
        layout->addWidget(separator(bar));

        QLabel *frequencyLabel=makeInfoLabel(QStringLiteral("Fréquence : auto"),bar);
        frequencyLabel->setObjectName(QStringLiteral("mockupFrequencyLabel"));
        layout->addWidget(frequencyLabel,0,Qt::AlignVCenter);

        layout->addStretch(1);

        QWidget *commBox=new QWidget(bar);
        commBox->setObjectName(QStringLiteral("mockupCommunicationBox"));
        commBox->setStyleSheet(QStringLiteral("#mockupCommunicationBox{background:transparent;border:0;}"));
        QHBoxLayout *commLayout=new QHBoxLayout(commBox);
        commLayout->setContentsMargins(5,0,5,0);
        commLayout->setSpacing(5);
        commLabel->setParent(commBox);
        commLabel->setStyleSheet(QStringLiteral("color:#c6cdd3;background:transparent;border:0;font-weight:700;"));
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
            const int headerH=qBound(42,qRound(48.0*scale),54);
            if (bar->height()!=headerH) bar->setFixedHeight(headerH);

            const int margin=qBound(7,qRound(10.0*scale),12);
            const int spacing=qBound(4,qRound(7.0*scale),8);
            layout->setContentsMargins(margin,qMax(3,qRound(4.0*scale)),margin,qMax(3,qRound(4.0*scale)));
            layout->setSpacing(spacing);

            const int logoSize=qBound(25,qRound(30.0*scale),34);
            logo->setFixedSize(logoSize+4,logoSize+4);
            if (!logoSource.isNull()) logo->setPixmap(logoSource.scaled(logoSize,logoSize,Qt::KeepAspectRatio,Qt::SmoothTransformation));

            QFont bf=brand->font();
            bf.setPointSizeF(qBound<qreal>(8.2,10.0*scale,11.0));
            brand->setFont(bf);
            QFont vf=version->font();
            vf.setPointSizeF(qBound<qreal>(6.4,7.3*scale,8.2));
            version->setFont(vf);

            const int infoPad=qBound(4,qRound(7.0*scale),9);
            const QString infoCss=infoStyle(infoPad);
            ecuLabel->setStyleSheet(infoCss);
            portLabel->setStyleSheet(infoCss);
            frequencyLabel->setStyleSheet(infoCss);

            const int h=qBound(25,qRound(29.0*scale),32);
            const int pad=qBound(18,qRound(24.0*scale),28);
            connectButton->setFixedHeight(h);
            disconnectButton->setFixedHeight(h);
            connectButton->setFixedWidth(qMax(84,connectButton->fontMetrics().horizontalAdvance(connectButton->text())+pad));
            disconnectButton->setFixedWidth(qMax(94,disconnectButton->fontMetrics().horizontalAdvance(disconnectButton->text())+pad));

            portLabel->setText(QStringLiteral("Port : %1").arg(configuredSerialPort()));
        };

        syncHeader();

        QTimer *sync=new QTimer(bar);
        sync->setInterval(250);
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
