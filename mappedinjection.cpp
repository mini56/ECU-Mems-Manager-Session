#include "mainwindow.h"
#include "memsinterface.h"
#include "ecuidentification.h"
#include "analysistab.h"
#include "i18n.h"

#include <QApplication>
#include <QByteArray>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEvent>
#include <QFrame>
#include <QGridLayout>
#include <QHash>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QMetaObject>
#include <QPainter>
#include <QPainterPath>
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
#include "mappedinjection_profiles_mems13.inc"
}

#include "mappedinjection_runtime.inc"
