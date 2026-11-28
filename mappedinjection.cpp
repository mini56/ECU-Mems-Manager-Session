#include "mainwindow.h"
#include "memsinterface.h"
#include "ecuidentification.h"
#include "analysistab.h"
#include "i18n.h"
#include "applicationvisualstyle.h"
#include "injectionrapidplan.h"

#include <QApplication>
#include <QByteArray>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEvent>
#include <QFrame>
#include <QFontMetrics>
#include <QGridLayout>
#include <QHash>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QMetaObject>
#include <QPainter>
#include <QPainterPath>
#include <QRadialGradient>
#include <QPushButton>
#include <QScrollArea>
#include <QSettings>
#include <QTabWidget>
#include <QThread>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include <QtMath>

#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/select.h>
#endif

namespace {
#include "mappedinjection_maps.inc"
#include "mappedinjection_helpers.inc"
#define validatedInjectionProfileForFirmware validatedInjectionProfileForFirmwareExisting
#include "mappedinjection_profiles.inc"
#undef validatedInjectionProfileForFirmware
#define validatedInjectionProfileForFirmware validatedInjectionProfileForFirmwareNa
#include "mappedinjection_profiles_extra.inc"
#undef validatedInjectionProfileForFirmware
#define validatedInjectionProfileForFirmware validatedInjectionProfileForFirmware16
#include "mappedinjection_profiles_turbo.inc"
#undef validatedInjectionProfileForFirmware
#define validatedInjectionProfileForFirmware validatedInjectionProfileForFirmware13
#include "mappedinjection_profiles_mems13.inc"
#undef validatedInjectionProfileForFirmware
#define validatedInjectionProfileForFirmware validatedInjectionProfileForFirmwareMems19B4
#include "mappedinjection_profiles_mems19.inc"
#undef validatedInjectionProfileForFirmware
#define validatedInjectionProfileForFirmware validatedInjectionProfileForFirmwareMems19B5
#include "mappedinjection_profiles_mems19_batch5.inc"
#undef validatedInjectionProfileForFirmware
#define validatedInjectionProfileForFirmware validatedInjectionProfileForFirmwareMems19B6
#include "mappedinjection_profiles_mems19_batch6.inc"
#undef validatedInjectionProfileForFirmware
#define validatedInjectionProfileForFirmware validatedInjectionProfileForFirmwareMems19B7
#include "mappedinjection_profiles_mems19_batch7.inc"
#undef validatedInjectionProfileForFirmware
#include "mappedinjection_profiles_mems19_tbp7f015.inc"
}

bool injectionRapidPlanForNa(const QString &firmware,InjectionRapidPlan *out)
{
    if(!out)return false;
    *out=InjectionRapidPlan();
    if(!Mems16Na::byFirmware(firmware.toLatin1().constData()))return false;
    InjectionCalculationProfile p;
    if(!validatedInjectionProfileForFirmwareNa(firmware,&p))return false;
    if(!p.map.baseAddress||!p.map.transientSourceAddress||!p.map.transientCounterAddress||
       (p.map.baseAddress&1u)||(p.map.transientSourceAddress&1u)||
       p.kind==InjectionCalculationKind::Unverified)return false;
    out->base=p.map.baseAddress;out->source=p.map.transientSourceAddress;
    out->counter=p.map.transientCounterAddress;
    out->incremental=p.counterPolicy==InjectionCounterPolicy::IncrementEvent;
    out->additive=p.kind==InjectionCalculationKind::AdditiveC001;
    out->baseScale=p.baseMsPerRaw;out->transientScale=p.transientMsPerRaw;
    return true;
}

#include "mappedinjection_runtime.inc"
