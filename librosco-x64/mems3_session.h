#ifndef MEMS_MANAGER_MEMS3_SESSION_H
#define MEMS_MANAGER_MEMS3_SESSION_H

/* Native Windows MEMS 3 diagnostic session. Reference: immutable Session 205
 * mems3protocol.h, commit b5cd9b61bbaed6251f17559885481b1bd03f4380.
 * Independent of Manager, MEMS 2J and historical ROSCO sessions.
 * Exports 115..123, no change to existing ABI. Hardware validation pending.
 */
#include "rosco.h"
#include "protocol_catalogue.h"
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mems_manager_mems3_session mems_manager_mems3_session;

typedef enum mems_manager_mems3_result {
    MEMS_MANAGER_3_OK = 0,
    MEMS_MANAGER_3_INVALID_ARGUMENT = 1,
    MEMS_MANAGER_3_PORT_FAILURE = 2,
    MEMS_MANAGER_3_WAKE_FAILURE = 3,
    MEMS_MANAGER_3_TIMEOUT = 4,
    MEMS_MANAGER_3_NEGATIVE_RESPONSE = 5,
    MEMS_MANAGER_3_CANCELLED = 6,
    MEMS_MANAGER_3_WRITE_FAILURE = 7,
    MEMS_MANAGER_3_PROTOCOL_FAILURE = 8,
    MEMS_MANAGER_3_NOT_CONNECTED = 9
} mems_manager_mems3_result;

typedef bool (*mems_manager_mems3_cancel_fn)(void *user);

/* Fired SYNCHRONOUSLY when actual bytes pass the COM port. 'bytes' is only
 * valid for the duration of the call. Caller owns any persistent log.
 * A response callback does NOT imply a successful ECU command.
 * direction: 1=TX, 2=RX; tick_ms is monotonic GetTickCount64.
 */
typedef void (*mems_manager_mems3_trace_fn)(
    void *user, int direction, uint64_t tick_ms,
    const uint8_t *bytes, size_t length);

/* Open 9600 8E1, framed wake 1A 9A, start 10 A0, seed/key (zero seed
 * means no key required), optional ping. No automatic DTC clear. */
ROSCO_API bool mems_manager_mems3_connect(
    const char *device_path, mems_manager_mems3_session **out_session,
    mems_manager_mems3_cancel_fn cancelled,
    mems_manager_mems3_trace_fn trace, void *user,
    mems_manager_mems3_result *failure_reason);

/* Positive 61 ID reply preserved raw; optional MEMS 3-specific metrics. */
ROSCO_API bool mems_manager_mems3_read_measurement(
    mems_manager_mems3_session *session, uint8_t id,
    uint8_t *raw_payload, size_t capacity, size_t *raw_size,
    mems_codec_measurements *metrics);

/* Read 18 00 00 00 / 58; raw payload for existing DTC codec. */
ROSCO_API bool mems_manager_mems3_read_faults(
    mems_manager_mems3_session *session, uint8_t *raw_payload,
    size_t capacity, size_t *raw_size);

/* Explicit 14 00 00, positive 54 00 00; never called during connect. */
ROSCO_API bool mems_manager_mems3_clear_faults(
    mems_manager_mems3_session *session);

/* Optional 3E keepalive (7E positive response), explicit call. */
ROSCO_API bool mems_manager_mems3_keepalive(
    mems_manager_mems3_session *session);

/* Fixed cycle: DTC (callback ID FF), measurements 00,06,0A,0B,21.
 * Callback success refers to a valid positive raw reply; conversion is separate. */
typedef void (*mems_manager_mems3_cycle_fn)(
    void *user, uint8_t id, bool success,
    const uint8_t *payload, size_t payload_length);
ROSCO_API bool mems_manager_mems3_read_cycle(
    mems_manager_mems3_session *session,
    mems_manager_mems3_cycle_fn on_result, void *result_user,
    unsigned int *success_count);

/* Returns last ECU transaction result; for a real ECU negative response,
 * response_code is the third byte of 7F service code. 0 if unavailable.
 * The monotonic response time in milliseconds is retained for diagnostics.
 * Every parameter except 'session' may be NULL.
 */
ROSCO_API bool mems_manager_mems3_last_transaction(
    const mems_manager_mems3_session *session,
    mems_manager_mems3_result *result,
    uint8_t *negative_response_code, uint32_t *duration_ms);

ROSCO_API bool mems_manager_mems3_is_open(
    const mems_manager_mems3_session *session);
ROSCO_API void mems_manager_mems3_close(
    mems_manager_mems3_session *session);

#ifdef __cplusplus
}
#endif
#endif
