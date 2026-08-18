#include "aboutbox.h"
#include <QMessageBox>
#include <QObject>
#include <QString>
#include <QIcon>
#ifdef Q_OS_WIN
#include <windows.h>
#endif
#include "i18n.h"
#define tr I18n::text

namespace {

static void applyDarkNativeTitleBar(QWidget *widget)
{
#ifdef Q_OS_WIN
    if (!widget)
        return;

    HWND hwnd = reinterpret_cast<HWND>(widget->winId());
    HMODULE dwmApi = LoadLibraryW(L"dwmapi.dll");
    if (!dwmApi)
        return;

    typedef HRESULT (WINAPI *DwmSetWindowAttributeFn)(HWND, DWORD, LPCVOID, DWORD);
    DwmSetWindowAttributeFn setAttribute = reinterpret_cast<DwmSetWindowAttributeFn>(
        GetProcAddress(dwmApi, "DwmSetWindowAttribute"));
    if (setAttribute)
    {
        const BOOL dark = TRUE;
        const DWORD attributes[] = {20, 19};
        for (DWORD attribute : attributes)
        {
            if (SUCCEEDED(setAttribute(hwnd, attribute, &dark, sizeof(dark))))
                break;
        }
    }

    FreeLibrary(dwmApi);
#else
    Q_UNUSED(widget);
#endif
}

}

void AboutBox::showAbout(QWidget* parent)
{
    const QString text =
        QStringLiteral("<h2>ECU Mems Manager</h2>")
        + QStringLiteral("<p>Version ") + QStringLiteral(APP_VERSION) + QStringLiteral("</p>")
        + QStringLiteral("<p><b>") + I18n::text(6200) /* EN: Compatible with: */ + QStringLiteral("</b></p>")
        + QStringLiteral("<ul>")
        + QStringLiteral("<li>Rover MEMS 1.2</li>")
        + QStringLiteral("<li>Rover MEMS 1.3</li>")
        + QStringLiteral("<li>Rover MEMS 1.6</li>")
        + QStringLiteral("</ul>")
        + QStringLiteral("<p><b>") + I18n::text(6201) /* EN: Protocol: */ + QStringLiteral("</b> ROSCO</p>")
        + QStringLiteral("<p><b>") + I18n::text(6202) /* EN: Development: */ + QStringLiteral("</b> Claude Lespagnol</p>");

    QMessageBox box(parent);
    box.setWindowTitle(I18n::text(6203) /* EN: About ECU Mems Manager */);
    box.setTextFormat(Qt::RichText);
    box.setText(text);
    box.setStandardButtons(QMessageBox::Ok);
    if (parent && !parent->windowIcon().isNull())
        box.setIconPixmap(parent->windowIcon().pixmap(64, 64));
    box.setStyleSheet(QStringLiteral(
        "QMessageBox QPushButton{background:#ff7a00;color:#101419;border:1px solid #ff9828;border-radius:5px;padding:6px 16px;font-weight:700;min-width:70px;}"
        "QMessageBox QPushButton:hover{background:#ff9828;}"
        "QMessageBox QPushButton:pressed{background:#d96500;}"));
    applyDarkNativeTitleBar(&box);
    box.exec();
}
