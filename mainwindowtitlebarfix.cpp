#include <QApplication>
#include <QCoreApplication>
#include <QEvent>
#include <QMainWindow>
#include <QTimer>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace {

static void applyMainWindowTitleBar(QMainWindow *window)
{
#ifdef Q_OS_WIN
    if (!window)
        return;

    const BOOL useDark = (qApp && qApp->property("ecuDarkTheme").toBool()) ? TRUE : FALSE;
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
#endif
}

class MainWindowTitleBarFix : public QObject
{
public:
    explicit MainWindowTitleBarFix(QObject *parent = nullptr) : QObject(parent) {}

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if (event->type() == QEvent::Show || event->type() == QEvent::Polish)
        {
            if (QMainWindow *window = qobject_cast<QMainWindow*>(watched))
                QTimer::singleShot(0, window, [window]() { applyMainWindowTitleBar(window); });
        }
        return QObject::eventFilter(watched, event);
    }
};

void installMainWindowTitleBarFix()
{
    QApplication *app = qobject_cast<QApplication*>(QCoreApplication::instance());
    if (app)
        app->installEventFilter(new MainWindowTitleBarFix(app));
}

}

Q_COREAPP_STARTUP_FUNCTION(installMainWindowTitleBarFix)
