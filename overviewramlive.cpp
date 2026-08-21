#include "mainwindow.h"
#include "memsinterface.h"
#include "ecuidentification.h"
#include "i18n.h"

#include <QApplication>
#include <QCoreApplication>
#include <QEvent>
#include <QLabel>
#include <QMetaObject>
#include <QTabWidget>
#include <QWidget>

// Historical compatibility slot. Aperçu must never enter Mode 4 anymore.
void MEMSInterface::onOverviewLiveModeRequested(bool enabled)
{
    Q_UNUSED(enabled);
    setProperty("overviewLiveRequested", false);
    setProperty("overviewLiveModeActive", false);
}

namespace {

class OverviewLiveInstaller : public QObject
{
public:
    explicit OverviewLiveInstaller(QObject *parent=nullptr):QObject(parent){}

protected:
    bool eventFilter(QObject *watched,QEvent *event) override
    {
        if(!watched||(event->type()!=QEvent::Show&&event->type()!=QEvent::Polish))
            return QObject::eventFilter(watched,event);

        MainWindow *window=qobject_cast<MainWindow*>(watched);
        if(!window||!window->memsInterface())
            return QObject::eventFilter(watched,event);

        // main.cpp still carries the historical injector range. Reassert the
        // normal-frame dwell range on every MainWindow Show/Polish so Aperçu
        // cannot silently fall back to the old 0-20 ms injection scale.
        if(window->property("overviewLiveInstalled").toBool()){
            if(QWidget *dwell=window->findChild<QWidget*>(QStringLiteral("m_injector_time"))){
                dwell->setProperty("minimum",0.0);dwell->setProperty("maximum",5.0);
            }
            return QObject::eventFilter(watched,event);
        }

        QTabWidget *tabs=window->findChild<QTabWidget*>(QStringLiteral("Tab_main"));
        if(!tabs)return QObject::eventFilter(watched,event);

        QWidget *dwellSource=window->findChild<QWidget*>(QStringLiteral("m_injector_time"));
        if(!dwellSource){dwellSource=new QWidget(window);dwellSource->setObjectName(QStringLiteral("m_injector_time"));dwellSource->hide();}
        dwellSource->setProperty("minimum",0.0);dwellSource->setProperty("maximum",5.0);dwellSource->setProperty("value",QStringLiteral("--"));

        window->setProperty("overviewLiveInstalled",true);
        window->setProperty("ecuExtendedIdentifier",QString());
        MEMSInterface *mems=window->memsInterface();

        // Dwell is part of the normal 0x80 diagnostic frame. This updates the
        // existing Aperçu card and its 2-minute trace without any RAM/Mode-4 read.
        QObject::connect(mems,&MEMSInterface::dataReady,window,[mems,dwellSource](){
            if(!mems||!dwellSource)return;const mems_data *data=mems->getData();if(!data)return;
            dwellSource->setProperty("value",double(data->coil_time)*0.002);
        });

        // Read D1 once after connection and cache the firmware identifier.
        QObject::connect(mems,&MEMSInterface::connected,window,[window,dwellSource](){
            window->setProperty("ecuExtendedIdentifier",QString());dwellSource->setProperty("value",QStringLiteral("--"));
            emit window->requestProtocolCommand(quint8(0xD1));
        });

        QObject::connect(mems,&MEMSInterface::protocolResponse,window,[window](quint8 command,const QByteArray &response){
            if(command!=quint8(0xD1))return;
            const QString identifier=EcuIdentification::extendedIdentifierFromReply(response);
            if(identifier.isEmpty()){window->setProperty("ecuExtendedIdentifier",QString());return;}
            window->setProperty("ecuExtendedIdentifier",identifier);
            if(QLabel *label=window->findChild<QLabel*>(QStringLiteral("m_ecuIdLabel"))){
                QString text=I18n::text(7050);if(!text.endsWith(QLatin1Char(' ')))text+=QLatin1Char(' ');text+=identifier;
                const QString reference=EcuIdentification::referenceForExtendedIdentifier(identifier);if(!reference.isEmpty())text+=QString::fromUtf8(" — ")+reference;label->setText(text);
            }
        });

        QObject::connect(mems,&MEMSInterface::disconnected,window,[window,dwellSource](){
            window->setProperty("ecuExtendedIdentifier",QString());dwellSource->setProperty("value",QStringLiteral("--"));
        });

        return QObject::eventFilter(watched,event);
    }
};

static void installOverviewLiveInstaller()
{
    QApplication *app=qobject_cast<QApplication*>(QCoreApplication::instance());
    if(app)app->installEventFilter(new OverviewLiveInstaller(app));
}

} // namespace

Q_COREAPP_STARTUP_FUNCTION(installOverviewLiveInstaller)
