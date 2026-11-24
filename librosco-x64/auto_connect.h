#ifndef MEMS_MANAGER_AUTO_CONNECT_H
#define MEMS_MANAGER_AUTO_CONNECT_H
#include "serial_adapters.h"
#include "protocol_catalogue.h"
#include "session_timeouts.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Separate engine diagnostic session. Same worker thread must open, use and
 * close it. Cancellation predicate may observe an atomic flag from the caller.
 * Callbacks are synchronous and must not reenter or destroy this session. */
typedef struct mems_manager_auto_session mems_manager_auto_session;
typedef bool (*mems_manager_auto_cancel_fn)(void *user);
typedef void (*mems_manager_auto_trace_fn)(void *user,int direction,
    uint64_t tick_ms,const uint8_t *bytes,size_t length);
/* Legacy family deliberately groups 1.2/1.3/1.6 until subtype is proven. */
typedef enum mems_manager_auto_family {
    MEMS_MANAGER_AUTO_UNKNOWN=0, MEMS_MANAGER_AUTO_ROSCO_1X=1,
    MEMS_MANAGER_AUTO_19=19, MEMS_MANAGER_AUTO_2J=20,
    MEMS_MANAGER_AUTO_3=30
} mems_manager_auto_family;
typedef enum mems_manager_auto_result {
    MEMS_MANAGER_AUTO_OK=0, MEMS_MANAGER_AUTO_INVALID=1,
    MEMS_MANAGER_AUTO_ENUMERATION_FAILED=2, MEMS_MANAGER_AUTO_NO_PORT=3,
    MEMS_MANAGER_AUTO_NO_ECU=4, MEMS_MANAGER_AUTO_CANCELLED=5,
    MEMS_MANAGER_AUTO_RESOURCE_FAILED=6
} mems_manager_auto_result;
/* Available protocol implementations, not proof of ECU-specific permissions
 * or sensor support. No operations are executed by this query. */
enum {
    MEMS_MANAGER_AUTO_CAP_LEGACY_READ=1u,
    MEMS_MANAGER_AUTO_CAP_FRAMED_READ=2u,
    MEMS_MANAGER_AUTO_CAP_DTC=4u,
    MEMS_MANAGER_AUTO_CAP_KEEPALIVE=8u,
    MEMS_MANAGER_AUTO_CAP_WIRE_TRACE=16u
};
typedef struct mems_manager_auto_identity {
    mems_manager_serial_adapter adapter;
    uint32_t family;
    uint8_t d0_id[4];
    uint8_t has_d0_id;
    uint8_t local_echo;
    uint8_t reserved[2];
} mems_manager_auto_identity;
/* Enumerate and try the four engine protocols in Session 205 order.
 * On failure *out_session is NULL and every attempted port is released.
 * Never auto-probes RC5 or Lucas 5AS, never clears DTC, never tunes ECU.
 * Byte trace covers auto-session engine protocols, including legacy wake bytes.
 * Electrical BREAK transitions are not byte events.
 * Legacy blocking calls can only check cancellation between operations. */
ROSCO_API bool mems_manager_auto_connect(const char *preferred_device,
    mems_manager_auto_session **out_session,mems_manager_auto_cancel_fn cancelled,
    mems_manager_auto_trace_fn trace,void *user,mems_manager_auto_result *result);
ROSCO_API bool mems_manager_get_session_identity(
    const mems_manager_auto_session *session,mems_manager_auto_identity *identity);
ROSCO_API mems_manager_auto_family mems_manager_get_session_family(
    const mems_manager_auto_session *session);
ROSCO_API uint32_t mems_manager_get_session_capabilities(
    const mems_manager_auto_session *session);
ROSCO_API void mems_manager_disconnect_auto(mems_manager_auto_session *session);
/* Operations on the same opaque session, without reopening the port.
 * All except cancel_current run on its owner thread. Output is only
 * committed on success. Legacy output uses existing ROSCO ABI/units.
 * DTC for 1.x/1.9 is carried in mems_data; framed DTC uses raw payload.
 * Framed calls require capacity >= MEMS_CODEC_MAX_PAYLOAD (96 bytes).
 */
ROSCO_API bool mems_manager_auto_read_legacy(mems_manager_auto_session *s,mems_data *data);
ROSCO_API bool mems_manager_auto_read_measurement(mems_manager_auto_session *s,uint8_t id,
    uint8_t *raw,size_t capacity,size_t *length,mems_codec_measurements *metrics);
ROSCO_API bool mems_manager_auto_read_faults(mems_manager_auto_session *s,
    uint8_t *raw,size_t capacity,size_t *length);
ROSCO_API bool mems_manager_auto_keepalive(mems_manager_auto_session *s);
/* Cross-thread request only while an operation is active; the caller must
 * keep s alive. No forced COM close. Legacy operations finish their blocking
 * call, then report cancellation; native framed calls check the predicate.
 * Connect uses the predicate supplied to auto_connect (s not yet published).
 */
ROSCO_API bool mems_manager_cancel_current(mems_manager_auto_session *s);
typedef enum mems_manager_transport_status {
    MEMS_MANAGER_TRANSPORT_OK=0, MEMS_MANAGER_TRANSPORT_INVALID=1,
    MEMS_MANAGER_TRANSPORT_UNSUPPORTED=2, MEMS_MANAGER_TRANSPORT_CANCELLED=3,
    MEMS_MANAGER_TRANSPORT_LEGACY_FAILURE=4, MEMS_MANAGER_TRANSPORT_NATIVE_FAILURE=5
} mems_manager_transport_status;
typedef struct mems_manager_transport_error {
    uint32_t status,family,native_status;
    uint8_t has_native_status,negative_response_code,has_timing,reserved;
    uint32_t duration_ms;
} mems_manager_transport_error;
/* Last completed generic operation, including failure. Native status is
 * the original 2J/3 enum when the underlying transaction ran.
 * Legacy failure has no invented negative response or OS error.
 * Duration is elapsed host operation time (not isolated ECU latency).
 */
ROSCO_API bool mems_manager_get_transport_error(const mems_manager_auto_session *s,
    mems_manager_transport_error *out);
ROSCO_API bool mems_manager_get_response_timing(const mems_manager_auto_session *s,
    uint32_t *duration_ms);
#define MEMS_MANAGER_TRACE_BYTES 512u
#define MEMS_MANAGER_TRACE_EVENTS 256u
typedef struct mems_manager_trace_event {
    uint64_t sequence,tick_ms,dropped_total;
    uint32_t family;
    char port[16];
    uint16_t length;
    uint8_t direction,reserved;
    uint8_t bytes[MEMS_MANAGER_TRACE_BYTES];
} mems_manager_trace_event;
/* Consume oldest retained TX/RX event. Bounded FIFO drops oldest on overflow;
 * sequence and dropped_total expose gaps. Failed probe events retain their
 * own family/port. All engine protocols through this auto-session API.
 * Caller persists events to file; DLL writes no log files.
 * Clear discards queued events and resets loss counter, not sequence.
 */
ROSCO_API bool mems_manager_get_next_trace_event(mems_manager_auto_session *s,
    mems_manager_trace_event *out);
ROSCO_API void mems_manager_clear_trace(mems_manager_auto_session *s);
/* Configures only the already-connected port/session, owner thread only.
 * No handshake timing changes and no changes to legacy Manager APIs.
 * Driver may reject unsupported Win32 values. 2J/3 response override is
 * per request, not a hard bound on OS I/O. Legacy response_ms must be 0.
 * Preserve defaults by reading settings before changing individual fields. */
ROSCO_API bool mems_manager_set_session_timeouts(mems_manager_auto_session *s,
    const mems_manager_session_timeouts *timeouts);
ROSCO_API bool mems_manager_get_session_timeouts(mems_manager_auto_session *s,
    mems_manager_session_timeouts *timeouts);
#ifdef __cplusplus
}
#endif
#endif
