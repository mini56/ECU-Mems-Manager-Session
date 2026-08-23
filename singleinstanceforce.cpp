#ifdef _WIN32
#include <windows.h>
#include <tlhelp32.h>

namespace {

static const wchar_t *currentExecutableName(wchar_t *buffer, DWORD bufferCount)
{
    if (!buffer || bufferCount == 0)
        return nullptr;

    const DWORD length = GetModuleFileNameW(nullptr, buffer, bufferCount);
    if (length == 0 || length >= bufferCount)
        return nullptr;

    const wchar_t *name = buffer;
    for (const wchar_t *p = buffer; *p; ++p)
    {
        if (*p == L'\\' || *p == L'/')
            name = p + 1;
    }
    return name;
}

struct CloseRequestContext
{
    DWORD processId;
    bool closePosted;
};

static BOOL CALLBACK requestCloseForProcessWindow(HWND hwnd, LPARAM lParam)
{
    CloseRequestContext *context = reinterpret_cast<CloseRequestContext*>(lParam);
    if (!context)
        return TRUE;

    DWORD windowProcessId = 0;
    GetWindowThreadProcessId(hwnd, &windowProcessId);
    if (windowProcessId != context->processId)
        return TRUE;

    if (PostMessageW(hwnd, WM_CLOSE, 0, 0))
        context->closePosted = true;

    return TRUE;
}

static void forceClosePreviousInstances()
{
    wchar_t executablePath[MAX_PATH] = {};
    const wchar_t *executableName = currentExecutableName(executablePath, MAX_PATH);
    if (!executableName || !*executableName)
        return;

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
        return;

    PROCESSENTRY32W entry = {};
    entry.dwSize = sizeof(entry);
    const DWORD currentPid = GetCurrentProcessId();
    bool noticeShown = false;
    bool closeFailed = false;

    if (Process32FirstW(snapshot, &entry))
    {
        do
        {
            if (entry.th32ProcessID == 0 || entry.th32ProcessID == currentPid)
                continue;

            if (lstrcmpiW(entry.szExeFile, executableName) != 0)
                continue;

            if (!noticeShown)
            {
                MessageBoxW(nullptr,
                            L"ECU MEMS Manager est déjà ouvert.\n\n"
                            L"L'instance déjà ouverte va être fermée proprement "
                            L"afin de poursuivre ce nouveau démarrage.",
                            L"ECU MEMS Manager déjà ouvert",
                            MB_OK | MB_ICONINFORMATION | MB_TOPMOST | MB_SETFOREGROUND);
                noticeShown = true;
            }

            HANDLE process = OpenProcess(SYNCHRONIZE, FALSE, entry.th32ProcessID);
            if (!process)
            {
                closeFailed = true;
                continue;
            }

            CloseRequestContext context = { entry.th32ProcessID, false };
            EnumWindows(requestCloseForProcessWindow,
                        reinterpret_cast<LPARAM>(&context));

            if (!context.closePosted || WaitForSingleObject(process, 8000) != WAIT_OBJECT_0)
                closeFailed = true;

            CloseHandle(process);
        }
        while (Process32NextW(snapshot, &entry));
    }

    CloseHandle(snapshot);

    if (closeFailed)
    {
        MessageBoxW(nullptr,
                    L"Impossible de fermer proprement l'instance déjà ouverte.\n"
                    L"Le nouveau démarrage est annulé afin de ne pas interrompre brutalement une session ECU.",
                    L"ECU MEMS Manager",
                    MB_OK | MB_ICONERROR | MB_TOPMOST | MB_SETFOREGROUND);
        ExitProcess(1);
    }
}

struct ForcePreviousInstanceAtProcessStart
{
    ForcePreviousInstanceAtProcessStart()
    {
        forceClosePreviousInstances();
    }
};

static ForcePreviousInstanceAtProcessStart g_forcePreviousInstanceAtProcessStart;

}
#endif
