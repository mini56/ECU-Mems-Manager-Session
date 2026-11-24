#include "mems19_wakeup.h"
#include "legacy_trace.h"
#include <string.h>

/* Physical 5-baud ISO 9141 wake extracted from Session 205
 * MEMSInterface::performMems19Wakeup() native K-Line path.
 * Win32 only. The historical ROSCO/Manager APIs stay unchanged.
 *
 * IMPORTANT: the same mems_info->sd HANDLE MUST stay open between E9
 * and CA/75/F4/D0; reopening can reset KKL/ECU diagnostic state.
 */
static bool cancelled_now(mems_manager_mems19_cancel_fn callback, void* data)
{
    return callback && callback(data);
}

/* Single clock and short waits prevent cumulative per-bit drift and
 * allow the caller to cancel while waiting for the ECU. */
static bool wait_until(ULONGLONG deadline,
                       mems_manager_mems19_cancel_fn callback, void* data)
{
    for (;;) {
        ULONGLONG now;
        ULONGLONG remaining;
        if (cancelled_now(callback, data))
            return false;
        now = GetTickCount64();
        if (now >= deadline)
            return true;
        remaining = deadline - now;
        Sleep((DWORD)(remaining < 20u ? remaining : 20u));
    }
}

static bool wait_ms(DWORD milliseconds,
                    mems_manager_mems19_cancel_fn callback, void* data)
{
    return wait_until(GetTickCount64() + milliseconds, callback, data);
}

/* Exactly one byte per read, as in Session 205's native wakeup.
 * Win32 serial read timeouts are set by the unchanged mems_openserial().
 * Buffer is capped; 0x55 need not be at offset zero. */
static bool receive_sync_keys(HANDLE handle, uint8_t* inverted_key,
                              mems_manager_mems19_cancel_fn callback,
                              void* data)
{
    uint8_t stream[64] = {0};
    size_t length = 0;
    ULONGLONG start = GetTickCount64();
    while (GetTickCount64() - start < 2000u &&
           !cancelled_now(callback, data)) {
        uint8_t byte = 0;
        DWORD got = 0;
        if (!ReadFile(handle, &byte, 1u, &got, NULL))
            return false;
        if (got == 1u) {
            mems_internal_trace_bytes(handle,2,(uint64_t)GetTickCount64(),&byte,1);
            if (length == sizeof(stream)) {
                memmove(stream, stream + 1, sizeof(stream) - 1u);
                --length;
            }
            stream[length++] = byte;
            if (mems_manager_mems19_key_complement(
                    stream, length, inverted_key))
                return true;
        } else {
            if (!wait_ms(10u, callback, data))
                return false;
        }
    }
    return false;
}

static bool receive_address_confirmation(HANDLE handle, uint8_t key_ack,
                                         bool* local_echo,
                                         mems_manager_mems19_cancel_fn callback,
                                         void* data)
{
    uint8_t stream[64] = {0};
    size_t length = 0;
    bool echo_seen = false;
    ULONGLONG start = GetTickCount64();
    while (GetTickCount64() - start < 1600u &&
           !cancelled_now(callback, data)) {
        uint8_t byte = 0;
        DWORD got = 0;
        if (!ReadFile(handle, &byte, 1u, &got, NULL))
            return false;
        if (got == 1u) {
            mems_internal_trace_bytes(handle,2,(uint64_t)GetTickCount64(),&byte,1);
            if (length == sizeof(stream)) {
                memmove(stream, stream + 1, sizeof(stream) - 1u);
                --length;
            }
            stream[length++] = byte;
            if (byte == key_ack)
                echo_seen = true;
            if (mems_manager_mems19_has_address_confirmation(stream, length)) {
                *local_echo = echo_seen;
                return true;
            }
        } else {
            if (!wait_ms(10u, callback, data))
                return false;
        }
    }
    return false;
}

bool mems_manager_mems19_wakeup(
    mems_info* info, bool* local_echo,
    mems_manager_mems19_wakeup_result* result,
    mems_manager_mems19_cancel_fn callback, void* data)
{
    HANDLE handle;
    bool echo = false;
    uint8_t complemented_key = 0;
    DWORD written = 0;
    int bit;
    ULONGLONG slow_start;
    mems_manager_mems19_wakeup_result status =
        MEMS_MANAGER_MEMS19_WAKE_INVALID_ARGUMENT;

    if (result) *result = status;
    if (local_echo) *local_echo = false;
    if (!info || !local_echo)
        return false;
    if (cancelled_now(callback, data)) {
        if (result) *result = MEMS_MANAGER_MEMS19_WAKE_CANCELLED;
        return false;
    }
    if (!mems_is_connected(info)) {
        if (result) *result = MEMS_MANAGER_MEMS19_WAKE_NO_OPEN_PORT;
        return false;
    }

    handle = info->sd;
    if (!PurgeComm(handle, PURGE_RXCLEAR | PURGE_TXCLEAR) ||
        !ClearCommBreak(handle)) {
        status = MEMS_MANAGER_MEMS19_WAKE_BREAK_FAILED;
        goto fail;
    }

    /* Same native Session 205 process: 2000 ms idle then 10-bit 0x16
     * address at 5 baud. Do not inherit old QSerialPort fallback's
     * different 300 ms idle and separate COM handle behavior. */
    if (!wait_ms(2000u, callback, data)) {
        status = MEMS_MANAGER_MEMS19_WAKE_CANCELLED;
        goto fail;
    }
    slow_start = GetTickCount64();
    for (bit = 0; bit < 10; ++bit) {
        bool high = false;
        if (cancelled_now(callback, data)) {
            status = MEMS_MANAGER_MEMS19_WAKE_CANCELLED;
            goto fail;
        }
        if (!mems_manager_mems19_slow_bit_high(bit, &high)) {
            status = MEMS_MANAGER_MEMS19_WAKE_INVALID_ARGUMENT;
            goto fail;
        }
        if (!(high ? ClearCommBreak(handle) : SetCommBreak(handle))) {
            status = MEMS_MANAGER_MEMS19_WAKE_BREAK_FAILED;
            goto fail;
        }
        if (!wait_until(slow_start + (ULONGLONG)(bit + 1) * 200u,
                        callback, data)) {
            status = MEMS_MANAGER_MEMS19_WAKE_CANCELLED;
            goto fail;
        }
    }
    if (!ClearCommBreak(handle)) {
        status = MEMS_MANAGER_MEMS19_WAKE_BREAK_FAILED;
        goto fail;
    }
    if (!receive_sync_keys(handle, &complemented_key, callback, data)) {
        status = cancelled_now(callback, data)
            ? MEMS_MANAGER_MEMS19_WAKE_CANCELLED
            : MEMS_MANAGER_MEMS19_WAKE_NO_KEY_BYTES;
        goto fail;
    }
    if (!wait_ms(50u, callback, data)) {
        status = MEMS_MANAGER_MEMS19_WAKE_CANCELLED;
        goto fail;
    }
    if (!WriteFile(handle, &complemented_key, 1u, &written, NULL) ||
        written != 1u) {
        status = MEMS_MANAGER_MEMS19_WAKE_ACK_FAILED;
        goto fail;
    }
    mems_internal_trace_bytes(handle,1,(uint64_t)GetTickCount64(),&complemented_key,1);
    if (!receive_address_confirmation(handle, complemented_key, &echo,
                                      callback, data)) {
        status = cancelled_now(callback, data)
            ? MEMS_MANAGER_MEMS19_WAKE_CANCELLED
            : MEMS_MANAGER_MEMS19_WAKE_NO_ADDRESS_CONFIRMATION;
        goto fail;
    }

    *local_echo = echo;
    if (result) *result = MEMS_MANAGER_MEMS19_WAKE_OK;
    return true;

fail:
    /* Never close/reopen the serial handle ourselves. The owning caller
     * can choose to disconnect on failure. Always release BREAK. */
    ClearCommBreak(handle);
    if (result) *result = status;
    return false;
}

bool mems_manager_mems19_connect_identify(
    mems_info* info, const char* device_path, uint8_t* four_byte_ecu_id,
    bool* local_echo, mems_manager_mems19_wakeup_result* result,
    mems_manager_mems19_cancel_fn callback, void* data)
{
    bool echo = false;
    uint8_t id[4] = {0};
    mems_manager_mems19_wakeup_result status =
        MEMS_MANAGER_MEMS19_WAKE_INVALID_ARGUMENT;

    if (result) *result = status;
    if (local_echo) *local_echo = false;
    if (!info || !device_path || !device_path[0] ||
        !four_byte_ecu_id || !local_echo)
        return false;
    if (cancelled_now(callback, data)) {
        if (result) *result = MEMS_MANAGER_MEMS19_WAKE_CANCELLED;
        return false;
    }
    /* Do not seize a port already owned by another ECU session. */
    if (mems_is_connected(info)) {
        if (result) *result = MEMS_MANAGER_MEMS19_WAKE_NO_OPEN_PORT;
        return false;
    }
    if (!mems_connect(info, device_path)) {
        if (result) *result = MEMS_MANAGER_MEMS19_WAKE_OPEN_FAILED;
        return false;
    }
    if (!mems_manager_mems19_wakeup(info, &echo, &status, callback, data))
        goto fail;
    if (cancelled_now(callback, data)) {
        status = MEMS_MANAGER_MEMS19_WAKE_CANCELLED;
        goto fail;
    }
    if (!mems_init_link_kline19(info, id, echo)) {
        status = MEMS_MANAGER_MEMS19_WAKE_LINK_INIT_FAILED;
        goto fail;
    }
    if (cancelled_now(callback, data)) {
        status = MEMS_MANAGER_MEMS19_WAKE_CANCELLED;
        goto fail;
    }
    memcpy(four_byte_ecu_id, id, sizeof(id));
    *local_echo = echo;
    if (result) *result = MEMS_MANAGER_MEMS19_WAKE_OK;
    return true;

fail:
    mems_disconnect(info);
    if (result) *result = status;
    return false;
}
