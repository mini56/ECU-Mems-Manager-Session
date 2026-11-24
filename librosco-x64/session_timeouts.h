#ifndef MEMS_MANAGER_SESSION_TIMEOUTS_H
#define MEMS_MANAGER_SESSION_TIMEOUTS_H
#include <stdint.h>
/* Exact Win32 COMMTIMEOUTS fields, plus optional framed protocol deadline.
 * 0 response_ms retains each command's historical deadline.
 * Legacy MEMS has no independent framed deadline: response_ms must be 0.
 * All fields in ms; multiplier fields ms per byte. */
typedef struct mems_manager_session_timeouts {
    uint32_t read_interval_ms,read_multiplier_ms,read_constant_ms;
    uint32_t write_multiplier_ms,write_constant_ms,response_ms;
} mems_manager_session_timeouts;
#endif
