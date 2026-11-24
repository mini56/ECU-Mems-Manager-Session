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
};
static bool cancelled_now(const mems_manager_auto_session *s)
{return s->cancelled&&s->cancelled(s->user);}
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
                    id,&echo,NULL,s->cancelled,s->user);
            }
        }
    }else if(family==MEMS_MANAGER_AUTO_2J){
        ok=mems_manager_mems2j_connect(a->device_path,&s->j2,
            s->cancelled,s->trace,s->user,NULL);
    }else if(family==MEMS_MANAGER_AUTO_3){
        ok=mems_manager_mems3_connect(a->device_path,&s->m3,
            s->cancelled,s->trace,s->user,NULL);
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
