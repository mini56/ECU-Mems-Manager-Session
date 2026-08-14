#include <QApplication>
#include <QComboBox>
#include <QCoreApplication>
#include <QEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QPushButton>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

#include "i18n.h"

namespace {

static QPushButton *makeBlueButton(const QString &text, QWidget *parent)
{
    QPushButton *button = new QPushButton(text, parent);
    button->setCursor(Qt::PointingHandCursor);
    button->setMinimumHeight(28);
    button->setStyleSheet(
        "QPushButton{background:#1769d2;color:#ffffff;border:1px solid #2d7ee8;"
        "border-radius:4px;padding:4px 13px;font-weight:600;}"
        "QPushButton:hover{background:#2378e6;}"
        "QPushButton:pressed{background:#1257b0;}"
        "QPushButton:disabled{background:#20262d;color:#65707a;border-color:#343c45;}"
    );
    return button;
}

static QFrame *makeStatusChip(QWidget *parent)
{
    QFrame *chip = new QFrame(parent);
    chip->setStyleSheet(
        "QFrame{background:#151a20;border:1px solid #313943;border-radius:4px;}"
        "QLabel{color:#dce2e7;background:transparent;border:0;}"
    );
    return chip;
}

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

        // Keep the classic application menu available; no extra hamburger menu is added.
        QWidget *legacy=central->findChild<QWidget*>(QStringLiteral("layoutWidget_7"));
        if (legacy) legacy->hide();

        QFrame *oldModeBar=central->findChild<QFrame*>(QStringLiteral("modernModeBar"));
        QComboBox *modeBox=oldModeBar ? oldModeBar->findChild<QComboBox*>() : nullptr;
        if (oldModeBar) oldModeBar->hide();

        QFrame *bar=new QFrame(central);
        bar->setObjectName(QStringLiteral("mockupTopHeader"));
        bar->setFixedHeight(42);
        bar->setStyleSheet(
            "#mockupTopHeader{background:#0e1318;border-bottom:1px solid #29313a;}"
            "QToolButton{color:#dce2e7;background:#171c22;border:1px solid #303944;"
            "border-radius:4px;padding:3px 7px;font-weight:600;}"
            "QToolButton:hover{border-color:#ff8a1c;color:#ffffff;}"
            "QComboBox{background:#151a20;color:#ffffff;border:1px solid #38424c;"
            "border-radius:4px;padding:4px 10px;min-width:92px;}"
        );
        QHBoxLayout *layout=new QHBoxLayout(bar);
        layout->setContentsMargins(10,6,10,6);
        layout->setSpacing(8);

        QLabel *brand=new QLabel(QStringLiteral("ECU MEMS MANAGER"),bar);
        brand->setStyleSheet("color:#f2f5f7;font-weight:700;letter-spacing:.4px;");
        layout->addWidget(brand);
        layout->addSpacing(10);

        QFrame *commChip=makeStatusChip(bar);
        QHBoxLayout *commLayout=new QHBoxLayout(commChip);
        commLayout->setContentsMargins(9,2,7,2);
        commLayout->setSpacing(5);
        QLabel *commText=new QLabel(I18n::text(102).remove(':'),commChip);
        QLabel *commDot=new QLabel(QString::fromUtf8("●"),commChip);
        commDot->setObjectName(QStringLiteral("modernCommDot"));
        commDot->setStyleSheet("color:#31553d;font-size:15px;border:0;background:transparent;");
        commLayout->addWidget(commText);
        commLayout->addWidget(commDot);
        layout->addWidget(commChip);

        QFrame *errorChip=makeStatusChip(bar);
        QHBoxLayout *errLayout=new QHBoxLayout(errorChip);
        errLayout->setContentsMargins(9,2,7,2);
        errLayout->setSpacing(5);
        QLabel *errText=new QLabel(I18n::text(105).remove(':'),errorChip);
        QLabel *errDot=new QLabel(QString::fromUtf8("●"),errorChip);
        errDot->setObjectName(QStringLiteral("modernErrorDot"));
        errDot->setStyleSheet("color:#702126;font-size:15px;border:0;background:transparent;");
        errLayout->addWidget(errText);
        errLayout->addWidget(errDot);
        layout->addWidget(errorChip);

        layout->addStretch(1);

        QLabel *ecuLabel=window->findChild<QLabel*>(QStringLiteral("m_ecuIdLabel"));
        QLabel *ecuMirror=new QLabel(ecuLabel ? ecuLabel->text() : I18n::text(104),bar);
        ecuMirror->setObjectName(QStringLiteral("modernEcuIdLabel"));
        ecuMirror->setStyleSheet("color:#d8dde2;background:#151a20;border:1px solid #313943;border-radius:4px;padding:5px 12px;");
        layout->addWidget(ecuMirror);

        QLabel *modeLabel=new QLabel(I18n::text(7100),bar);
        modeLabel->setStyleSheet("color:#aeb6bd;");
        layout->addWidget(modeLabel);
        if (modeBox) {
            modeBox->setParent(bar);
            layout->addWidget(modeBox);
            modeBox->show();
        }

        QPushButton *connect=makeBlueButton(I18n::text(101),bar);
        QPushButton *disconnect=makeBlueButton(I18n::text(103),bar);
        QPushButton *snapshot=makeBlueButton(I18n::text(7015),bar);
        QPushButton *captures=makeBlueButton(I18n::text(7016),bar);

        QObject::connect(connect,&QPushButton::clicked,window,[window](){QMetaObject::invokeMethod(window,"onConnectClicked",Qt::QueuedConnection);});
        QObject::connect(disconnect,&QPushButton::clicked,window,[window](){QMetaObject::invokeMethod(window,"onDisconnectClicked",Qt::QueuedConnection);});
        QObject::connect(snapshot,&QPushButton::clicked,window,[window](){QMetaObject::invokeMethod(window,"onSnapshotClicked",Qt::QueuedConnection);});
        QObject::connect(captures,&QPushButton::clicked,window,[window](){QMetaObject::invokeMethod(window,"onViewCapturesClicked",Qt::QueuedConnection);});

        layout->addWidget(connect);
        layout->addWidget(disconnect);
        layout->addWidget(snapshot);
        layout->addWidget(captures);

        QToolButton *gear=new QToolButton(bar);
        gear->setText(QString::fromUtf8("⚙"));
        gear->setFixedSize(32,28);
        QObject::connect(gear,&QToolButton::clicked,window,[window](){QMetaObject::invokeMethod(window,"onEditOptionsClicked",Qt::QueuedConnection);});
        layout->addWidget(gear);

        root->insertWidget(0,bar);

        QPushButton *legacyConnect=window->findChild<QPushButton*>(QStringLiteral("m_connectButton"));
        QPushButton *legacyDisconnect=window->findChild<QPushButton*>(QStringLiteral("m_disconnectButton"));
        QObject *goodLed=window->findChild<QObject*>(QStringLiteral("m_commsGoodLed"));
        QObject *errorLed=window->findChild<QObject*>(QStringLiteral("m_engine_error"));

        QTimer *sync=new QTimer(bar);
        sync->setInterval(250);
        QObject::connect(sync,&QTimer::timeout,bar,[=](){
            if (ecuLabel) ecuMirror->setText(ecuLabel->text());
            const bool connected=legacyDisconnect && legacyDisconnect->isEnabled();
            const bool canConnect=legacyConnect ? legacyConnect->isEnabled() : !connected;
            connect->setEnabled(canConnect);
            disconnect->setEnabled(connected);
            commDot->setStyleSheet(QStringLiteral("color:%1;font-size:15px;border:0;background:transparent;").arg(connected?QStringLiteral("#1ed760"):QStringLiteral("#31553d")));
            if (goodLed && goodLed->property("checked").isValid() && goodLed->property("checked").toBool())
                commDot->setStyleSheet("color:#1ed760;font-size:15px;border:0;background:transparent;");
            const bool errorOn=errorLed && errorLed->property("checked").toBool();
            errDot->setStyleSheet(QStringLiteral("color:%1;font-size:15px;border:0;background:transparent;").arg(errorOn?QStringLiteral("#d83b43"):QStringLiteral("#702126")));
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
