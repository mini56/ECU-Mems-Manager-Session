/* Engine auto-dispatch, source of truth Session 205
 * b5cd9b61bbaed6251f17559885481b1bd03f4380, memsinterface_dispatch.cpp.
 * Native existing transports reused; no historical function changed. */
#include "auto_connect.h"
#include "mems19_wakeup.h"
#include "mems2j_session.h"
#include "mems3_session.h"
#include <stdlib.h>
#include <string.h>
struct mems_manager_auto_session {
    mems_manager_auto_identity identity;
    mems_info legacy;
    bool legacy_initialized;
    mems_manager_mems2j_session *j2;
    mems_manager_mems3_session *m3;
    mems_manager_auto_cancel_fn cancelled;
    mems_manager_auto_trace_fn trace;
    void *user;
    volatile LONG operation_state;
    mems_manager_transport_error last;
    bool has_last;
    uint32_t probe_family;
    char probe_port[16];
    mems_manager_trace_event events[MEMS_MANAGER_TRACE_EVENTS];
    size_t trace_head,trace_count;
    uint64_t trace_sequence,trace_dropped;
};
static bool cancelled_now(const mems_manager_auto_session *s)
{return InterlockedCompareExchange((volatile LONG*)&s->operation_state,0,0)==2 ||
    (s->cancelled&&s->cancelled(s->user));}
static bool native_cancel(void *user)
{return cancelled_now((const mems_manager_auto_session*)user);}
static void native_trace(void *user,int direction,uint64_t tick,
    const uint8_t *bytes,size_t length)
{
    mems_manager_auto_session *s=(mems_manager_auto_session*)user;
    size_t offset=0;
    if(!bytes||!length)return;
    while(offset<length){
        size_t n=length-offset,index;
        mems_manager_trace_event *event;
        if(n>MEMS_MANAGER_TRACE_BYTES)n=MEMS_MANAGER_TRACE_BYTES;
        if(s->trace_count==MEMS_MANAGER_TRACE_EVENTS){
            s->trace_head=(s->trace_head+1)%MEMS_MANAGER_TRACE_EVENTS;
            --s->trace_count;++s->trace_dropped;
        }
        index=(s->trace_head+s->trace_count)%MEMS_MANAGER_TRACE_EVENTS;
        event=&s->events[index];memset(event,0,sizeof(*event));
        event->sequence=++s->trace_sequence;event->tick_ms=tick;
        event->dropped_total=s->trace_dropped;event->family=s->probe_family;
        memcpy(event->port,s->probe_port,sizeof(event->port));
        event->direction=(uint8_t)direction;event->length=(uint16_t)n;
        memcpy(event->bytes,bytes+offset,n);
        ++s->trace_count;offset+=n;
    }
    if(s->trace)s->trace(s->user,direction,tick,bytes,length);
}
static void release_transport(mems_manager_auto_session *s)
{
    if(s->j2){mems_manager_mems2j_close(s->j2);s->j2=NULL;}
    if(s->m3){mems_manager_mems3_close(s->m3);s->m3=NULL;}
    if(s->legacy_initialized){mems_cleanup(&s->legacy);s->legacy_initialized=false;}
    memset(&s->identity,0,sizeof(s->identity));
}
static bool try_protocol(mems_manager_auto_session *s,
    const mems_manager_serial_adapter *a,mems_manager_auto_family family)
{
    bool ok=false,echo=false;uint8_t id[4]={0};
    if(cancelled_now(s))return false;
    s->probe_family=(uint32_t)family;
    memcpy(s->probe_port,a->port,sizeof(s->probe_port));
    if(family==MEMS_MANAGER_AUTO_ROSCO_1X||family==MEMS_MANAGER_AUTO_19){
        memset(&s->legacy,0,sizeof(s->legacy));
        mems_init(&s->legacy);s->legacy_initialized=true;
        if(s->legacy.mutex){
            if(family==MEMS_MANAGER_AUTO_ROSCO_1X){
                /* Opening COM alone is never a successful ECU connection. */
                ok=mems_connect(&s->legacy,a->device_path)&&!cancelled_now(s)&&
                    mems_init_link(&s->legacy,id);
            }else{
                ok=mems_manager_mems19_connect_identify(&s->legacy,a->device_path,
                    id,&echo,NULL,native_cancel,s);
            }
        }
    }else if(family==MEMS_MANAGER_AUTO_2J){
        ok=mems_manager_mems2j_connect(a->device_path,&s->j2,
            native_cancel,native_trace,s,NULL);
    }else if(family==MEMS_MANAGER_AUTO_3){
        ok=mems_manager_mems3_connect(a->device_path,&s->m3,
            native_cancel,native_trace,s,NULL);
    }
    if(!ok||cancelled_now(s)){release_transport(s);return false;}
    s->identity.adapter=*a;s->identity.family=(uint32_t)family;
    if(family==MEMS_MANAGER_AUTO_ROSCO_1X||family==MEMS_MANAGER_AUTO_19){
        memcpy(s->identity.d0_id,id,sizeof(id));s->identity.has_d0_id=1;
        s->identity.local_echo=(uint8_t)echo;
    }
    return true;
}
bool mems_manager_auto_connect(const char *preferred_device,
    mems_manager_auto_session **out_session,mems_manager_auto_cancel_fn cancelled,
    mems_manager_auto_trace_fn trace,void *user,mems_manager_auto_result *result)
{
    static const mems_manager_auto_family rosco_first[]={MEMS_MANAGER_AUTO_ROSCO_1X,
        MEMS_MANAGER_AUTO_2J,MEMS_MANAGER_AUTO_3,MEMS_MANAGER_AUTO_19};
    static const mems_manager_auto_family kline_first[]={MEMS_MANAGER_AUTO_2J,
        MEMS_MANAGER_AUTO_3,MEMS_MANAGER_AUTO_19,MEMS_MANAGER_AUTO_ROSCO_1X};
    mems_manager_serial_adapter *adapters=NULL;size_t count=0,capacity=0;
    mems_manager_auto_session *s=NULL;
    mems_manager_auto_result status=MEMS_MANAGER_AUTO_INVALID;
    unsigned retry;size_t i,j;
    if(result)*result=status;
    if(!out_session)return false;
    *out_session=NULL;
    if(cancelled&&cancelled(user)){status=MEMS_MANAGER_AUTO_CANCELLED;goto done;}
    if(!mems_manager_list_serial_adapters(preferred_device,NULL,0,&count)){
        status=MEMS_MANAGER_AUTO_ENUMERATION_FAILED;goto done;
    }
    for(retry=0;retry<3&&count>capacity;++retry){
        mems_manager_serial_adapter *next;
        if(count>SIZE_MAX/sizeof(*next)){status=MEMS_MANAGER_AUTO_RESOURCE_FAILED;goto done;}
        next=(mems_manager_serial_adapter*)realloc(adapters,count*sizeof(*next));
        if(!next){status=MEMS_MANAGER_AUTO_RESOURCE_FAILED;goto done;}
        adapters=next;capacity=count;
        if(!mems_manager_list_serial_adapters(preferred_device,adapters,capacity,&count)){
            status=MEMS_MANAGER_AUTO_ENUMERATION_FAILED;goto done;
        }
    }
    if(count>capacity){status=MEMS_MANAGER_AUTO_ENUMERATION_FAILED;goto done;}
    if(!count){status=MEMS_MANAGER_AUTO_NO_PORT;goto done;}
    s=(mems_manager_auto_session*)calloc(1,sizeof(*s));
    if(!s){status=MEMS_MANAGER_AUTO_RESOURCE_FAILED;goto done;}
    s->cancelled=cancelled;s->trace=trace;s->user=user;
    status=MEMS_MANAGER_AUTO_NO_ECU;
    for(i=0;i<count;++i){
        const mems_manager_auto_family *order=adapters[i].prefers_kline_first?kline_first:rosco_first;
        for(j=0;j<4;++j){
            if(cancelled_now(s)){status=MEMS_MANAGER_AUTO_CANCELLED;goto done;}
            if(try_protocol(s,&adapters[i],order[j])){
                *out_session=s;s=NULL;status=MEMS_MANAGER_AUTO_OK;goto done;
            }
        }
    }
    if(cancelled_now(s))status=MEMS_MANAGER_AUTO_CANCELLED;
done:
    if(s){release_transport(s);free(s);}
    free(adapters);
    if(result)*result=status;
    return status==MEMS_MANAGER_AUTO_OK;
}
mems_manager_auto_family mems_manager_get_session_family(const mems_manager_auto_session *s)
{return s?(mems_manager_auto_family)s->identity.family:MEMS_MANAGER_AUTO_UNKNOWN;}
bool mems_manager_get_session_identity(const mems_manager_auto_session *s,mems_manager_auto_identity *id)
{
    if(!s||!id||s->identity.family==MEMS_MANAGER_AUTO_UNKNOWN)return false;
    *id=s->identity;return true;
}
uint32_t mems_manager_get_session_capabilities(const mems_manager_auto_session *s)
{
    switch(mems_manager_get_session_family(s)){
    case MEMS_MANAGER_AUTO_ROSCO_1X:case MEMS_MANAGER_AUTO_19:
        return MEMS_MANAGER_AUTO_CAP_LEGACY_READ|MEMS_MANAGER_AUTO_CAP_DTC|
            MEMS_MANAGER_AUTO_CAP_KEEPALIVE;
    case MEMS_MANAGER_AUTO_2J:case MEMS_MANAGER_AUTO_3:
        return MEMS_MANAGER_AUTO_CAP_FRAMED_READ|MEMS_MANAGER_AUTO_CAP_DTC|
            MEMS_MANAGER_AUTO_CAP_KEEPALIVE|MEMS_MANAGER_AUTO_CAP_WIRE_TRACE;
    default:return 0;
    }
}
void mems_manager_disconnect_auto(mems_manager_auto_session *s)
{if(s){release_transport(s);free(s);}}

static void record_status(mems_manager_auto_session *s,mems_manager_transport_status status)
{
    memset(&s->last,0,sizeof(s->last));
    s->last.family=s->identity.family;s->last.status=(uint32_t)status;s->has_last=true;
}
static bool begin_operation(mems_manager_auto_session *s,bool supported,bool valid)
{
    if(!s)return false;
    if(InterlockedCompareExchange(&s->operation_state,1,0)!=0)return false;
    if(!valid||!supported||cancelled_now(s)){
        record_status(s,!valid?MEMS_MANAGER_TRANSPORT_INVALID:
            !supported?MEMS_MANAGER_TRANSPORT_UNSUPPORTED:MEMS_MANAGER_TRANSPORT_CANCELLED);
        InterlockedExchange(&s->operation_state,0);return false;
    }
    return true;
}
static bool finish_operation(mems_manager_auto_session *s,bool ok,bool native,ULONGLONG start)
{
    uint64_t elapsed=GetTickCount64()-start;bool was_cancelled=cancelled_now(s);
    /* Atomically close the cancellation window, including a late request. */
    if(InterlockedExchange(&s->operation_state,0)==2)was_cancelled=true;
    record_status(s,was_cancelled?MEMS_MANAGER_TRANSPORT_CANCELLED:
        ok?MEMS_MANAGER_TRANSPORT_OK:native?MEMS_MANAGER_TRANSPORT_NATIVE_FAILURE:
        MEMS_MANAGER_TRANSPORT_LEGACY_FAILURE);
    s->last.has_timing=1;s->last.duration_ms=elapsed>UINT32_MAX?UINT32_MAX:(uint32_t)elapsed;
    if(native){
        uint8_t neg=0;bool got=false;
        if(s->j2){
            mems_manager_mems2j_result r;
            got=mems_manager_mems2j_last_transaction(s->j2,&r,&neg,NULL);
            if(got)s->last.native_status=(uint32_t)r;
        }else if(s->m3){
            mems_manager_mems3_result r;
            got=mems_manager_mems3_last_transaction(s->m3,&r,&neg,NULL);
            if(got)s->last.native_status=(uint32_t)r;
        }
        s->last.has_native_status=(uint8_t)got;
        s->last.negative_response_code=got?neg:0;
    }
    return ok&&!was_cancelled;
}
bool mems_manager_cancel_current(mems_manager_auto_session *s)
{
    LONG old;
    if(!s)return false;
    old=InterlockedCompareExchange(&s->operation_state,2,1);
    return old==1||old==2;
}
bool mems_manager_get_transport_error(const mems_manager_auto_session *s,mems_manager_transport_error *out)
{
    if(!s||!out||!s->has_last)return false;
    *out=s->last;return true;
}
bool mems_manager_get_response_timing(const mems_manager_auto_session *s,uint32_t *ms)
{
    if(!s||!ms||!s->has_last||!s->last.has_timing)return false;
    *ms=s->last.duration_ms;return true;
}
bool mems_manager_get_next_trace_event(mems_manager_auto_session *s,mems_manager_trace_event *out)
{
    if(!s||!out||!s->trace_count)return false;
    *out=s->events[s->trace_head];
    /* Report losses so far, even if oldest retained event predates overflow. */
    out->dropped_total=s->trace_dropped;
    s->trace_head=(s->trace_head+1)%MEMS_MANAGER_TRACE_EVENTS;--s->trace_count;
    return true;
}
void mems_manager_clear_trace(mems_manager_auto_session *s)
{if(s){s->trace_head=s->trace_count=0;s->trace_dropped=0;}}
bool mems_manager_auto_read_legacy(mems_manager_auto_session *s,mems_data *data)
{
    mems_data tmp;bool ok;ULONGLONG start;
    if(!begin_operation(s,s&&s->legacy_initialized,data!=NULL))return false;
    start=GetTickCount64();
    ok=s->identity.family==MEMS_MANAGER_AUTO_19?
        mems_read_kline19(&s->legacy,&tmp,s->identity.local_echo!=0):
        mems_read(&s->legacy,&tmp);
    if(!finish_operation(s,ok,false,start))return false;
    *data=tmp;return true;
}
bool mems_manager_auto_read_measurement(mems_manager_auto_session *s,uint8_t id,
    uint8_t *raw,size_t capacity,size_t *length,mems_codec_measurements *metrics)
{
    uint8_t tmp[MEMS_CODEC_MAX_PAYLOAD];size_t n=0;mems_codec_measurements m;
    bool ok;ULONGLONG start;
    if(!begin_operation(s,s&&(s->j2||s->m3),raw&&length&&capacity>=sizeof(tmp)))return false;
    start=GetTickCount64();
    ok=s->j2?mems_manager_mems2j_read_measurement(s->j2,id,tmp,sizeof(tmp),&n,metrics?&m:NULL):
        mems_manager_mems3_read_measurement(s->m3,id,tmp,sizeof(tmp),&n,metrics?&m:NULL);
    if(!finish_operation(s,ok,true,start))return false;
    if(n>sizeof(tmp)){record_status(s,MEMS_MANAGER_TRANSPORT_NATIVE_FAILURE);return false;}
    memcpy(raw,tmp,n);*length=n;if(metrics)*metrics=m;return true;
}
bool mems_manager_auto_read_faults(mems_manager_auto_session *s,uint8_t *raw,size_t capacity,size_t *length)
{
    uint8_t tmp[MEMS_CODEC_MAX_PAYLOAD];size_t n=0;bool ok;ULONGLONG start;
    if(!begin_operation(s,s&&(s->j2||s->m3),raw&&length&&capacity>=sizeof(tmp)))return false;
    start=GetTickCount64();
    ok=s->j2?mems_manager_mems2j_read_faults(s->j2,tmp,sizeof(tmp),&n):
        mems_manager_mems3_read_faults(s->m3,tmp,sizeof(tmp),&n);
    if(!finish_operation(s,ok,true,start))return false;
    if(n>sizeof(tmp)){record_status(s,MEMS_MANAGER_TRANSPORT_NATIVE_FAILURE);return false;}
    memcpy(raw,tmp,n);*length=n;return true;
}
bool mems_manager_auto_keepalive(mems_manager_auto_session *s)
{
    bool ok,native;ULONGLONG start;
    if(!begin_operation(s,s&&(s->legacy_initialized||s->j2||s->m3),true))return false;
    start=GetTickCount64();native=s->j2||s->m3;
    if(s->j2)ok=mems_manager_mems2j_keepalive(s->j2);
    else if(s->m3)ok=mems_manager_mems3_keepalive(s->m3);
    else if(s->identity.family==MEMS_MANAGER_AUTO_19)
        ok=mems_command_kline19(&s->legacy,MEMS_Heartbeat,s->identity.local_echo!=0,NULL);
    else ok=mems_heartbeat(&s->legacy);
    return finish_operation(s,ok,native,start);
}
