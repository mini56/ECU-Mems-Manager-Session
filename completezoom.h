#ifndef COMPLETE_ZOOM_H
#define COMPLETE_ZOOM_H

#include <QApplication>
#include <QByteArray>
#include <QSize>
#include <QtGlobal>
#include "completezoommath.h"

#if defined(Q_OS_WIN)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace CompleteZoom {

inline QSize referenceSize() { return QSize(kReferenceWidth, kReferenceHeight); }

inline QSize physicalPrimaryWorkArea()
{
#if defined(Q_OS_WIN)
    // Ask Windows for native pixels before creating Qt's screen abstraction.
    // This does not change the display resolution or the user's DPI setting.
    ::SetProcessDPIAware();
    MONITORINFO info = {};
    info.cbSize = sizeof(info);
    POINT origin = {0, 0};
    HMONITOR monitor = ::MonitorFromPoint(origin, MONITOR_DEFAULTTOPRIMARY);
    if (monitor && ::GetMonitorInfoW(monitor, &info)) {
        const int w = info.rcWork.right - info.rcWork.left;
        const int h = info.rcWork.bottom - info.rcWork.top;
        if (w > 0 && h > 0) return QSize(w, h);
    }
#endif
    return QSize(kReferenceWidth, kReferenceHeight + kNativeCaptionAllowance);
}

// Qt 5 processes QT_SCALE_FACTOR at QApplication construction time.
// Disable its additional automatic DPI scaling to prevent double zoom at
// 125%/150% Windows text scaling; apply one uniformly calculated scale.
inline void configureBeforeApplication()
{
    QApplication::setAttribute(Qt::AA_DisableHighDpiScaling, true);
    qputenv("QT_AUTO_SCREEN_SCALE_FACTOR", "0");
    const QSize work = physicalPrimaryWorkArea();
    const double zoom = factorForWorkArea(work.width(), work.height());
    qputenv("QT_SCALE_FACTOR", QByteArray::number(zoom, 'f', 4));
}

}
#endif
