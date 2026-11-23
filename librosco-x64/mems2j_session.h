#ifndef MEMS_MANAGER_MEMS2J_SESSION_H
#define MEMS_MANAGER_MEMS2J_SESSION_H

/*
 * Native Windows MEMS 2J diagnostic transport/session.
 * Reference: immutable Session 205 mems2jprotocol.h (commit
 * b5cd9b61bbaed6251f17559885481b1bd03f4380).
 *
 * Completely separate from historical ROSCO mems_info and UI. No Qt.
 * This API is append-only; exports 1..105 and their ABI are unchanged.
 * ECU operation truth is based on real 2J frame replies, not USB detection.
 * The hardware/cable-specific timing remains to be confirmed on real ECU.
 */
#include "rosco.h"
#include "protocol_catalogue.h"
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mems_manager_mems2j_session mems_manager_mems2j_session;

typedef enum mems_manager_mems2j_result {
    MEMS_MANAGER_2J_OK = 0,
    MEMS_MANAGER_2J_INVALID_ARGUMENT = 1,
    MEMS_MANAGER_2J_PORT_FAILURE = 2,
    MEMS_MANAGER_2J_WAKE_FAILURE = 3,
    MEMS_MANAGER_2J_TIMEOUT = 4,
    MEMS_MANAGER_2J_NEGATIVE_RESPONSE = 5,
    MEMS_MANAGER_2J_CANCELLED = 6,
    MEMS_MANAGER_2J_WRITE_FAILURE = 7,
    MEMS_MANAGER_2J_PROTOCOL_FAILURE = 8,
    MEMS_MANAGER_2J_NOT_CONNECTED = 9
} mems_manager_mems2j_result;

typedef bool (*mems_manager_mems2j_cancel_fn)(void *user);

/* Fired SYNCHRONOUSLY when actual bytes pass the COM port. 'bytes' is only
 * valid for the duration of the call. Caller owns any persistent log.
 * A response callback does NOT imply a successful ECU command.
 * direction: 1=TX, 2=RX; tick_ms is monotonic GetTickCount64.
 */
typedef void (*mems_manager_mems2j_trace_fn)(
    void *user, int direction, uint64_t tick_ms,
    const uint8_t *bytes, size_t length);

/* Open 10400 8N1, 25ms BREAK, unframed 81 13 F7 81 0C, start 10 A0,
 * seed request 27 01, calculated key 27 02, and verify positive ECU
 * responses. 3E keepalive/21 09 optional after authenticated session.
 *
 * No mutation of ECU user settings, no DTC clear on Connect.
 * Returns session only after the actual seed/key diagnostic handshake.
 * All I/O and sleeps occur on the calling thread.
 */
ROSCO_API bool mems_manager_mems2j_connect(
    const char *device_path, mems_manager_mems2j_session **out_session,
    mems_manager_mems2j_cancel_fn cancelled,
    mems_manager_mems2j_trace_fn trace, void *user);

/* Positive 61 ID reply stored losslessly; codec measurement is OPTIONAL.
 * Some known IDs, including 0x00 and 0x06, remain raw-only (not invented).
 * Set 'metrics' to NULL if not wanted; returned count=0 for raw-only ids.
 */
ROSCO_API bool mems_manager_mems2j_read_measurement(
    mems_manager_mems2j_session *session, uint8_t id,
    uint8_t *raw_payload, size_t capacity, size_t *raw_size,
    mems_codec_measurements *metrics);

/* Read raw 61 19 fault response; Mini MPi bit decoding remains in
 * documentation/Manager, not hard-coded as universal 2J terminology.
 */
ROSCO_API bool mems_manager_mems2j_read_faults(
    mems_manager_mems2j_session *session, uint8_t *raw_payload,
    size_t capacity, size_t *raw_size);

/* Explicit diagnostic request ONLY: 31 CB + 15x00, positive 71 CB.
 * No clearing on connection, and a serial WriteFile success is not an
 * acknowledged ECU DTC clear.
 */
ROSCO_API bool mems_manager_mems2j_clear_faults(
    mems_manager_mems2j_session *session);

/* Optional 3E keepalive (7E positive response), explicit call. */
ROSCO_API bool mems_manager_mems2j_keepalive(
    mems_manager_mems2j_session *session);

/* Session 205 schedule: 09 + 19 + 4 rotating from the 9 'primary' IDs
 * + 1 rotating from 11 'extra' IDs. Calls to this function are explicit;
 * rejected sensor IDs do not blacklist a whole ECU session. The caller
 * supplies per-response callback for raw bytes (or NULL to skip results).
 * 'success_count' counts valid positive replies, including raw-only.
 */
typedef void (*mems_manager_mems2j_cycle_fn)(
    void *user, uint8_t id, bool success,
    const uint8_t *payload, size_t payload_length);
ROSCO_API bool mems_manager_mems2j_read_cycle(
    mems_manager_mems2j_session *session,
    mems_manager_mems2j_cycle_fn on_result, void *result_user,
    unsigned int *success_count);

/* Returns last ECU transaction result; for a real ECU negative response,
 * response_code is the third byte of 7F service code. 0 if unavailable.
 * The monotonic response time in milliseconds is retained for diagnostics.
 * Every parameter except 'session' may be NULL.
 */
ROSCO_API bool mems_manager_mems2j_last_transaction(
    const mems_manager_mems2j_session *session,
    mems_manager_mems2j_result *result,
    uint8_t *negative_response_code, uint32_t *duration_ms);

ROSCO_API bool mems_manager_mems2j_is_open(
    const mems_manager_mems2j_session *session);
ROSCO_API void mems_manager_mems2j_close(
    mems_manager_mems2j_session *session);

#ifdef __cplusplus
}
#endif
#endif
