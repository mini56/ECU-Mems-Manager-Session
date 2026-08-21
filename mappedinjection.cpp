#include "mainwindow.h"
#include "memsinterface.h"
#include "ecuidentification.h"

#include <QApplication>
#include <QByteArray>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEvent>
#include <QHash>
#include <QMetaObject>
#include <QTabWidget>
#include <QString>
#include <QtGlobal>

#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/select.h>
#endif

namespace {
#include "mappedinjection_maps.inc"
#include "mappedinjection_helpers.inc"
} // namespace

#include "mappedinjection_runtime.inc"
