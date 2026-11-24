#ifndef MEMS_INTERNAL_SESSION_TIMEOUTS_H
#define MEMS_INTERNAL_SESSION_TIMEOUTS_H
#include "rosco.h"
#include "session_timeouts.h"
#include "mems2j_session.h"
#include "mems3_session.h"
/* Internal, not exports. Read/write the actual driver's timeout values. */
static inline bool mems_internal_port_get_timeouts(HANDLE port,mems_manager_session_timeouts *out,uint32_t response)
{
    COMMTIMEOUTS t;
    if(!out||!GetCommTimeouts(port,&t))return false;
    out->read_interval_ms=t.ReadIntervalTimeout;
    out->read_multiplier_ms=t.ReadTotalTimeoutMultiplier;
    out->read_constant_ms=t.ReadTotalTimeoutConstant;
    out->write_multiplier_ms=t.WriteTotalTimeoutMultiplier;
    out->write_constant_ms=t.WriteTotalTimeoutConstant;
    out->response_ms=response;return true;
}
static inline bool mems_internal_port_set_timeouts(HANDLE port,const mems_manager_session_timeouts *in)
{
    COMMTIMEOUTS t;
    if(!in)return false;
    t.ReadIntervalTimeout=in->read_interval_ms;
    t.ReadTotalTimeoutMultiplier=in->read_multiplier_ms;
    t.ReadTotalTimeoutConstant=in->read_constant_ms;
    t.WriteTotalTimeoutMultiplier=in->write_multiplier_ms;
    t.WriteTotalTimeoutConstant=in->write_constant_ms;
    return SetCommTimeouts(port,&t)==TRUE;
}
bool mems_internal_2j_get_timeouts(const mems_manager_mems2j_session*,mems_manager_session_timeouts*);
bool mems_internal_2j_set_timeouts(mems_manager_mems2j_session*,const mems_manager_session_timeouts*);
bool mems_internal_3_get_timeouts(const mems_manager_mems3_session*,mems_manager_session_timeouts*);
bool mems_internal_3_set_timeouts(mems_manager_mems3_session*,const mems_manager_session_timeouts*);
#endif
