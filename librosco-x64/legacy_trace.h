#ifndef MEMS_INTERNAL_LEGACY_TRACE_H
#define MEMS_INTERNAL_LEGACY_TRACE_H
#include "rosco.h"
#include <stddef.h>
typedef void (*mems_internal_trace_fn)(void*,int,uint64_t,const uint8_t*,size_t);
typedef struct mems_internal_trace_scope {
    mems_info *info;
    mems_internal_trace_fn callback;
    void *user;
    struct mems_internal_trace_scope *previous;
} mems_internal_trace_scope;
/* Internal only; no public export or modification of mems_info layout.
 * Scope lives on caller's stack; matching handle only, thread-local. */
void mems_internal_trace_begin(mems_internal_trace_scope*,mems_info*,mems_internal_trace_fn,void*);
void mems_internal_trace_end(mems_internal_trace_scope*);
void mems_internal_trace_bytes(HANDLE,int,uint64_t,const uint8_t*,size_t);
#endif
