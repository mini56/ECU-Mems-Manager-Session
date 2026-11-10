#include "mainwindow.h"
#include "overviewramlive.h"
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

void initializeOverviewLive(MainWindow *window)
{
    if (!window || !window->memsInterface())
        return;

    QWidget *dwellSource=window->findChild<QWidget*>(QStringLiteral("m_injector_time"));
    if (!dwellSource){
        dwellSource=new QWidget(window);
        dwellSource->setObjectName(QStringLiteral("m_injector_time"));
        dwellSource->hide();
    }
    dwellSource->setProperty("minimum",0.0);
    dwellSource->setProperty("maximum",5.0);
    dwellSource->setProperty("value",QStringLiteral("--"));

    window->setProperty("ecuExtendedIdentifier",QString());
    MEMSInterface *mems=window->memsInterface();

    // Data comes from the normal 0x80 diagnostic frame, never Mode 4.
    QObject::connect(mems,&MEMSInterface::dataReady,window,[mems,dwellSource](){
        if(!mems||!dwellSource)return;
        const mems_data *data=mems->getData();
        if(!data)return;
        dwellSource->setProperty("value",double(data->coil_time)*0.002);
    });

    // The extended identifier is read exactly once at connection.
    QObject::connect(mems,&MEMSInterface::connected,window,[window,dwellSource](){
        window->setProperty("ecuExtendedIdentifier",QString());
        dwellSource->setProperty("value",QStringLiteral("--"));
        emit window->requestProtocolCommand(quint8(0xD1));
    });

    QObject::connect(mems,&MEMSInterface::protocolResponse,window,[window](quint8 command,const QByteArray &response){
        if(command!=quint8(0xD1))return;
        const QString identifier=EcuIdentification::extendedIdentifierFromReply(response);
        if(identifier.isEmpty()){
            window->setProperty("ecuExtendedIdentifier",QString());
            return;
        }
        window->setProperty("ecuExtendedIdentifier",identifier);
        if(QLabel *label=window->findChild<QLabel*>(QStringLiteral("m_ecuIdLabel"))){
            QString text=I18n::text(7050);
            if(!text.endsWith(QLatin1Char(' ')))text+=QLatin1Char(' ');
            text+=identifier;
            const QString reference=EcuIdentification::referenceForExtendedIdentifier(identifier);
            if(!reference.isEmpty())text+=QString::fromUtf8(" — ")+reference;
            label->setText(text);
        }
    });

    QObject::connect(mems,&MEMSInterface::disconnected,window,[window,dwellSource](){
        window->setProperty("ecuExtendedIdentifier",QString());
        dwellSource->setProperty("value",QStringLiteral("--"));
    });
}
