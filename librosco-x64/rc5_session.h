#ifndef MEMS_MANAGER_RC5_SESSION_H
#define MEMS_MANAGER_RC5_SESSION_H
/* RC5/EC5 SRS diagnostic session; native Windows, separate from engine.
 * Caller supplies an electrically appropriate interface and a Win32 COM path.
 * Synchronous single-owner API. No deployment, coding or crash-data reset.
 * Source: Session 205 rc5protocol.h, b5cd9b61bbaed6251f17559885481b1bd03f4380.
 * Physical ECU verification pending.
 */
#include "rosco.h"
#include "protocol_catalogue.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct mems_manager_rc5_session mems_manager_rc5_session;
typedef bool (*mems_manager_rc5_cancel_fn)(void *user);
/* direction 1=TX, 2=RX; monotonic milliseconds, bytes borrowed until return. */
typedef void (*mems_manager_rc5_trace_fn)(void *user,int direction,
    uint64_t tick_ms,const uint8_t *bytes,size_t length);
/* Open exclusive 2400 8N1 transport only. Does NOT imply ECU connection.
 * Caller must perform wake then ping. Close on failure before retrying. */
ROSCO_API bool mems_manager_rc5_open_transport(const char *device_path,
    mems_manager_rc5_session **out_session,mems_manager_rc5_cancel_fn cancel,
    mems_manager_rc5_trace_fn trace,void *user);
/* Documented BREAK sequence + actual wake signature. No DTC mutation. */
ROSCO_API bool mems_manager_rc5_wakeup(mems_manager_rc5_session *s);
/* Require exact positive ECU ping; enables diagnostic transactions. */
ROSCO_API bool mems_manager_rc5_ping(mems_manager_rc5_session *s);
/* Return validated raw fault packet, including zero-fault packet. */
ROSCO_API bool mems_manager_rc5_read_faults(mems_manager_rc5_session *s,
    uint8_t *packet,size_t capacity,size_t *length);
/* Existing fault codec plus the immutable Manager 205 description key.
 * Unknown codes retained; historical flag does not prove current status. */
ROSCO_API bool mems_manager_rc5_decode_fault_detail(const uint8_t *packet,
    size_t length,size_t index,uint16_t *code,bool *historical,
    bool *known,int *description_key);
/* Standard diagnostic DTC clear only, explicit caller confirmation;
 * acknowledgement is not proof of repair or permanent fault removal.
 * Never invoked from open, wake, ping or read. No crash-data manipulation. */
ROSCO_API bool mems_manager_rc5_clear_faults(mems_manager_rc5_session *s,
    bool explicitly_confirmed);
ROSCO_API void mems_manager_rc5_close_session(mems_manager_rc5_session *s);
#ifdef __cplusplus
}
#endif
#endif
