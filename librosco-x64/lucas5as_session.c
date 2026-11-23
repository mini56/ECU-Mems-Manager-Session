#include "lucas5as.h"
#include <stdlib.h>
#include <string.h>

/* Independent read-only Lucas 5AS NORMAL diagnostics transport.
 * Source parity: Session 205 lucas5asreader.h. The session is separate
 * from historical mems_info and NEVER opened by the old Manager API.
 *
 * Prerequisite: physical Lucas 5AS-specific, electrically adapted cable.
 * USB identification is NOT proof of 5AS cable electrical compatibility.
 * No EEPROM, fob learning, output actuation or SRS mutations in this file.
 * Real ECU verification is outstanding; return connected only after a
 * physical 55 83 76 wake AND a complete documented 83 response.
 */
struct mems_manager_lucas5as_session {
    HANDLE serial;
    uint8_t initial_packet[MEMS_MANAGER_LUCAS5AS_MAX_PACKET];
    size_t initial_length;
    bool connected;
};

static bool serial_usable(HANDLE handle)
{
    return handle != NULL && handle != INVALID_HANDLE_VALUE;
}

static bool configure_serial(HANDLE handle)
{
    DCB dcb;
    COMMTIMEOUTS timeouts;
    if (!serial_usable(handle))
        return false;
    memset(&dcb, 0, sizeof(dcb));
    dcb.DCBlength = sizeof(dcb);
    if (!GetCommState(handle, &dcb))
        return false;
    dcb.BaudRate = CBR_9600;
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    dcb.fBinary = TRUE;
    dcb.fParity = FALSE;
    dcb.fOutxCtsFlow = FALSE;
    dcb.fOutxDsrFlow = FALSE;
    dcb.fDtrControl = DTR_CONTROL_DISABLE;
    dcb.fRtsControl = RTS_CONTROL_DISABLE;
    dcb.fOutX = FALSE;
    dcb.fInX = FALSE;
    if (!SetCommState(handle, &dcb) ||
        !GetCommTimeouts(handle, &timeouts))
        return false;
    timeouts.ReadIntervalTimeout = 20;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.ReadTotalTimeoutConstant = 40;
    timeouts.WriteTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutConstant = 500;
    return SetCommTimeouts(handle, &timeouts) == TRUE;
}

static bool pause_until(ULONGLONG target)
{
    /* One clock for all 10 cells avoids accumulating scheduling drift. */
    while (GetTickCount64() < target) {
        const ULONGLONG remaining = target - GetTickCount64();
        Sleep((DWORD)(remaining < 20u ? remaining : 20u));
    }
    return true;
}

static bool transmit_slow_address(HANDLE handle)
{
    const ULONGLONG start = GetTickCount64();
    int index;
    for (index = 0; index < 10; ++index) {
        bool high = false;
        if (!mems_manager_lucas5as_slow_bit_high(index, &high))
            return false;
        if (high) {
            if (!ClearCommBreak(handle))
                return false;
        } else if (!SetCommBreak(handle)) {
            return false;
        }
        pause_until(start + (ULONGLONG)(index + 1) * 200u);
    }
    return ClearCommBreak(handle) == TRUE;
}

static bool wait_normal_wake(HANDLE handle)
{
    uint8_t bytes[96];
    size_t n = 0;
    const ULONGLONG start = GetTickCount64();
    while (GetTickCount64() - start < 2200u) {
        DWORD got = 0;
        if (n >= sizeof(bytes)) {
            memmove(bytes, bytes + n - 48u, 48u);
            n = 48u;
        }
        if (!ReadFile(handle, bytes + n, (DWORD)(sizeof(bytes) - n),
                      &got, NULL))
            return false;
        if (got > sizeof(bytes) - n)
            return false;
        n += (size_t)got;
        if (mems_manager_lucas5as_has_normal_wake(bytes, n))
            return true;
        if (got == 0)
            Sleep(10);
    }
    return false;
}

static bool request_status(mems_manager_lucas5as_session *session,
                           uint8_t *frame, size_t capacity, size_t *written)
{
    uint8_t received[256];
    uint8_t full[MEMS_MANAGER_LUCAS5AS_MAX_PACKET];
    mems_manager_lucas5as_status decoded;
    size_t count = 0;
    size_t output = 0;
    size_t offset = 0;
    DWORD sent = 0;
    const uint8_t request = 0x83u;
    ULONGLONG start;
    if (!session || !serial_usable(session->serial) ||
        !frame || !written || capacity < 2)
        return false;
    /* Like the existing Qt Session205 reader, do not accept stale
     * bytes left in the buffer from the preceding wake or request. */
    if (!PurgeComm(session->serial, PURGE_RXCLEAR))
        return false;
    if (!WriteFile(session->serial, &request, 1, &sent, NULL) ||
        sent != 1)
        return false;

    start = GetTickCount64();
    while (GetTickCount64() - start < 2000u) {
        DWORD got = 0;
        if (count >= sizeof(received))
            return false;
        if (!ReadFile(session->serial, received + count,
                      (DWORD)(sizeof(received) - count), &got, NULL))
            return false;
        if (got > sizeof(received) - count)
            return false;
        count += (size_t)got;
        if (mems_manager_lucas5as_extract_status(
                received, count, full, sizeof(full), &output,
                &offset, &decoded)) {
            if (capacity < output)
                return false;
            memcpy(frame, full, output);
            *written = output;
            return true;
        }
        if (got == 0)
            Sleep(10);
    }
    return false;
}

bool mems_manager_lucas5as_open_normal(
    const char *device_path, mems_manager_lucas5as_session **session)
{
    mems_manager_lucas5as_session *opened;
    DWORD ignored;
    if (!session || !device_path || !device_path[0])
        return false;
    *session = NULL;
    opened = (mems_manager_lucas5as_session*)calloc(1, sizeof(*opened));
    if (!opened)
        return false;
    opened->serial = INVALID_HANDLE_VALUE;
    opened->serial = CreateFileA(device_path, GENERIC_READ | GENERIC_WRITE,
                                 0, NULL, OPEN_EXISTING,
                                 FILE_ATTRIBUTE_NORMAL, NULL);
    if (!serial_usable(opened->serial))
        goto fail;
    if (!configure_serial(opened->serial) ||
        !PurgeComm(opened->serial, PURGE_RXCLEAR | PURGE_TXCLEAR) ||
        !ClearCommBreak(opened->serial))
        goto fail;
    Sleep(300); /* Same pre-wake settling period as Session 205. */
    if (!transmit_slow_address(opened->serial))
        goto fail;
    if (!wait_normal_wake(opened->serial))
        goto fail;
    if (!request_status(opened, opened->initial_packet,
                        sizeof(opened->initial_packet),
                        &opened->initial_length))
        goto fail;
    opened->connected = true;
    *session = opened;
    (void)ignored;
    return true;

fail:
    mems_manager_lucas5as_close(opened);
    return false;
}

bool mems_manager_lucas5as_initial_status(
    const mems_manager_lucas5as_session *session,
    uint8_t *frame, size_t capacity, size_t *written)
{
    if (!session || !session->connected || !frame || !written ||
        !session->initial_length || capacity < session->initial_length)
        return false;
    memcpy(frame, session->initial_packet, session->initial_length);
    *written = session->initial_length;
    return true;
}

bool mems_manager_lucas5as_read_status(
    mems_manager_lucas5as_session *session,
    uint8_t *frame, size_t capacity, size_t *written)
{
    if (!session || !session->connected)
        return false;
    return request_status(session, frame, capacity, written);
}

void mems_manager_lucas5as_close(
    mems_manager_lucas5as_session *session)
{
    if (!session)
        return;
    session->connected = false;
    if (serial_usable(session->serial)) {
        ClearCommBreak(session->serial);
        CloseHandle(session->serial);
        session->serial = INVALID_HANDLE_VALUE;
    }
    free(session);
}
