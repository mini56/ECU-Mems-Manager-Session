#include "nativewindowstyle.h"
#include <QWidget>
#include <QtGlobal>
#ifdef Q_OS_WIN
#include <windows.h>
#endif

void applyNativeTitleBar(QWidget *window, bool dark)
{
#ifdef Q_OS_WIN
    if (!window)
        return;

    const BOOL useDark = dark ? TRUE : FALSE;
    HWND hwnd = reinterpret_cast<HWND>(window->winId());
    HMODULE dwmApi = LoadLibraryW(L"dwmapi.dll");
    if (!dwmApi)
        return;

    typedef HRESULT (WINAPI *DwmSetWindowAttributeFn)(HWND, DWORD, LPCVOID, DWORD);
    DwmSetWindowAttributeFn setAttribute = reinterpret_cast<DwmSetWindowAttributeFn>(
        GetProcAddress(dwmApi, "DwmSetWindowAttribute"));
    if (setAttribute)
    {
        const DWORD attributes[] = {20, 19};
        for (DWORD attribute : attributes)
        {
            if (SUCCEEDED(setAttribute(hwnd, attribute, &useDark, sizeof(useDark))))
                break;
        }
    }

    FreeLibrary(dwmApi);
#else
    Q_UNUSED(window);
    Q_UNUSED(dark);
#endif
}
