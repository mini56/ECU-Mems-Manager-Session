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
    bool terminationFailed = false;

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
                            L"L'instance déjà ouverte va être fermée immédiatement "
                            L"afin de poursuivre ce nouveau démarrage.",
                            L"ECU MEMS Manager déjà ouvert",
                            MB_OK | MB_ICONINFORMATION | MB_TOPMOST | MB_SETFOREGROUND);
                noticeShown = true;
            }

            HANDLE process = OpenProcess(PROCESS_TERMINATE | SYNCHRONIZE,
                                         FALSE, entry.th32ProcessID);
            if (!process)
            {
                terminationFailed = true;
                continue;
            }

            if (!TerminateProcess(process, 0))
            {
                if (WaitForSingleObject(process, 0) != WAIT_OBJECT_0)
                    terminationFailed = true;
            }
            else
            {
                WaitForSingleObject(process, 5000);
            }

            CloseHandle(process);
        }
        while (Process32NextW(snapshot, &entry));
    }

    CloseHandle(snapshot);

    if (terminationFailed)
    {
        MessageBoxW(nullptr,
                    L"Impossible de fermer l'instance déjà ouverte.\n"
                    L"Le nouveau démarrage est annulé.",
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
