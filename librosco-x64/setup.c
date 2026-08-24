#include "rosco.h"

void mems_init(mems_info* info)
{
    if (!info)
        return;

    info->sd = INVALID_HANDLE_VALUE;
    /* Keep the historical initialization semantics for the compatibility
     * baseline. ECU MEMS Manager initializes and uses librosco from its worker
     * thread, as the old 32-bit build did. */
    info->mutex = CreateMutexA(NULL, TRUE, NULL);
}

void mems_cleanup(mems_info* info)
{
    if (!info)
        return;

    if (mems_is_connected(info))
    {
        CloseHandle(info->sd);
        info->sd = INVALID_HANDLE_VALUE;
    }

    if (info->mutex)
    {
        CloseHandle(info->mutex);
        info->mutex = NULL;
    }
}

librosco_version mems_get_lib_version(void)
{
    librosco_version ver;
    ver.major = LIBROSCO_VER_MAJOR;
    ver.minor = LIBROSCO_VER_MINOR;
    ver.patch = LIBROSCO_VER_PATCH;
    return ver;
}

void mems_disconnect(mems_info* info)
{
    if (!info || !info->mutex)
        return;

    if (WaitForSingleObject(info->mutex, INFINITE) == WAIT_OBJECT_0)
    {
        if (mems_is_connected(info))
        {
            CloseHandle(info->sd);
            info->sd = INVALID_HANDLE_VALUE;
        }
        ReleaseMutex(info->mutex);
    }
}

bool mems_connect(mems_info* info, const char* devPath)
{
    bool result = false;

    if (!info || !info->mutex || !devPath)
        return false;

    if (WaitForSingleObject(info->mutex, INFINITE) == WAIT_OBJECT_0)
    {
        result = mems_is_connected(info) || mems_openserial(info, devPath);
        ReleaseMutex(info->mutex);
    }

    return result;
}

bool mems_openserial(mems_info* info, const char* devPath)
{
    DCB dcb;
    COMMTIMEOUTS commTimeouts;
    bool retVal = false;

    if (!info || !devPath)
        return false;

    info->sd = CreateFileA(devPath,
                           GENERIC_READ | GENERIC_WRITE,
                           0,
                           NULL,
                           OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL);

    if (info->sd == INVALID_HANDLE_VALUE)
        return false;

    ZeroMemory(&dcb, sizeof(dcb));
    dcb.DCBlength = sizeof(dcb);

    if (GetCommState(info->sd, &dcb) == TRUE)
    {
        dcb.BaudRate = CBR_9600;
        dcb.fBinary = TRUE;
        dcb.fParity = FALSE;
        dcb.fOutxCtsFlow = FALSE;
        dcb.fOutxDsrFlow = FALSE;
        dcb.fDtrControl = DTR_CONTROL_DISABLE;
        dcb.fRtsControl = RTS_CONTROL_DISABLE;
        dcb.fOutX = FALSE;
        dcb.fInX = FALSE;
        dcb.ByteSize = 8;
        dcb.Parity = NOPARITY;
        dcb.StopBits = ONESTOPBIT;

        if (SetCommState(info->sd, &dcb) == TRUE &&
            GetCommTimeouts(info->sd, &commTimeouts) == TRUE)
        {
            commTimeouts.ReadIntervalTimeout = 100;
            commTimeouts.ReadTotalTimeoutMultiplier = 0;
            commTimeouts.ReadTotalTimeoutConstant = 100;

            if (SetCommTimeouts(info->sd, &commTimeouts) == TRUE)
                retVal = true;
        }
    }

    if (!retVal)
    {
        CloseHandle(info->sd);
        info->sd = INVALID_HANDLE_VALUE;
    }

    return retVal;
}

bool mems_is_connected(mems_info* info)
{
    return info && info->sd != INVALID_HANDLE_VALUE;
}
