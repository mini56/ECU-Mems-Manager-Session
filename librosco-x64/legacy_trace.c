#include "legacy_trace.h"
#if defined(_MSC_VER)
static __declspec(thread) mems_internal_trace_scope *current_scope;
#else
static __thread mems_internal_trace_scope *current_scope;
#endif
void mems_internal_trace_begin(mems_internal_trace_scope *scope,mems_info *info,mems_internal_trace_fn cb,void *user)
{
    scope->info=info;scope->callback=cb;scope->user=user;
    scope->previous=current_scope;current_scope=scope;
}
void mems_internal_trace_end(mems_internal_trace_scope *scope)
{
    if(current_scope==scope)current_scope=scope->previous;
}
void mems_internal_trace_bytes(HANDLE handle,int direction,uint64_t tick,const uint8_t *bytes,size_t n)
{
    mems_internal_trace_scope *scope=current_scope;
    if(scope&&scope->info&&scope->info->sd==handle&&scope->callback&&bytes&&n)
        scope->callback(scope->user,direction,tick,bytes,n);
}
