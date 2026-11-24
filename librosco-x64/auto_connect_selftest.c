/* Deterministic dispatcher tests. All hardware-facing calls replaced. */
#include "auto_connect.h"
#include "mems19_wakeup.h"
#include "mems2j_session.h"
#include "mems3_session.h"
#include <stdio.h>
#include <string.h>
static int failures;
#define CHECK(x) do{if(!(x)){fprintf(stderr,"AUTO_CONNECT_FAIL %d %s\n",__LINE__,#x);++failures;}}while(0)
static struct {
    unsigned count,logs[32],n,live,closed,cancel_after;
    unsigned success_family,success_port;
    bool kline,enumeration_error;
} g;
static void reset(void){memset(&g,0,sizeof(g));g.count=1;}
static unsigned port_no(const char *p){return strstr(p,"COM2")?2:1;}
static bool log_attempt(unsigned f,const char *p){
    unsigned port=port_no(p);CHECK(g.live==0);g.logs[g.n++]=port*100+f;
    return g.success_family==f&&g.success_port==port;
}
static bool fake_list(const char *preferred,mems_manager_serial_adapter *out,size_t cap,size_t *total){
    (void)preferred;if(g.enumeration_error)return false;*total=g.count;
    for(size_t i=0;i<g.count&&i<cap;++i){
        memset(out+i,0,sizeof(*out));snprintf(out[i].port,sizeof(out[i].port),"COM%u",(unsigned)i+1);
        snprintf(out[i].device_path,sizeof(out[i].device_path),"\\\\.\\COM%u",(unsigned)i+1);
        out[i].prefers_kline_first=(uint8_t)g.kline;
    }return true;
}
static void fake_init(mems_info *i){i->mutex=(HANDLE)(uintptr_t)1;i->sd=INVALID_HANDLE_VALUE;}
static bool legacy_reply;
static bool fake_connect(mems_info *i,const char *p){
    legacy_reply=log_attempt(1,p);i->sd=(HANDLE)(uintptr_t)2;++g.live;return true;
}
static bool fake_link(mems_info *i,uint8_t *id){(void)i;memset(id,0x42,4);return legacy_reply;}
static void fake_cleanup(mems_info *i){
    if(i->sd!=INVALID_HANDLE_VALUE){--g.live;++g.closed;}i->sd=INVALID_HANDLE_VALUE;i->mutex=NULL;
}
static bool fake19(mems_info *i,const char *p,uint8_t *id,bool *echo,
    mems_manager_mems19_wakeup_result *r,mems_manager_mems19_cancel_fn cb,void *user){
    (void)r;(void)cb;(void)user;if(!log_attempt(19,p))return false;
    i->sd=(HANDLE)(uintptr_t)2;++g.live;memset(id,0x19,4);*echo=true;return true;
}
static bool fake2(const char *p,mems_manager_mems2j_session **s,
    mems_manager_mems2j_cancel_fn cb,mems_manager_mems2j_trace_fn tr,void *u,mems_manager_mems2j_result *r){
    (void)cb;(void)tr;(void)u;(void)r;if(!log_attempt(20,p))return false;
    *s=(mems_manager_mems2j_session*)(uintptr_t)3;++g.live;return true;
}
static bool fake3(const char *p,mems_manager_mems3_session **s,
    mems_manager_mems3_cancel_fn cb,mems_manager_mems3_trace_fn tr,void *u,mems_manager_mems3_result *r){
    (void)cb;(void)tr;(void)u;(void)r;if(!log_attempt(30,p))return false;
    *s=(mems_manager_mems3_session*)(uintptr_t)4;++g.live;return true;
}
static mems_manager_auto_session *active;
static bool cancel_in_read,fail_read;
static bool fake_legacy_read(mems_info *i,mems_data *data){
    (void)i;memset(data,0,sizeof(*data));data->engine_rpm=1234;
    if(cancel_in_read)CHECK(mems_manager_cancel_current(active));
    return !fail_read;
}
static bool fake19_read(mems_info *i,mems_data *data,bool echo){
    CHECK(echo);return fake_legacy_read(i,data);
}
static bool fake_heartbeat(mems_info *i){(void)i;return true;}
static bool fake19_cmd(mems_info *i,uint8_t cmd,bool echo,uint8_t *reply){
    (void)i;(void)reply;CHECK(echo&&cmd==MEMS_Heartbeat);return true;
}
static bool fake_measure(uint8_t id,uint8_t *raw,size_t cap,size_t *n,mems_codec_measurements *m){
    CHECK(cap>=4);raw[0]=0x61;raw[1]=id;raw[2]=0x01;raw[3]=0x02;*n=4;
    if(m){memset(m,0,sizeof(*m));m->id=id;}
    if(cancel_in_read)CHECK(mems_manager_cancel_current(active));
    return !fail_read;
}
static bool fake2_measure(mems_manager_mems2j_session *s,uint8_t id,uint8_t *raw,size_t cap,size_t *n,mems_codec_measurements *m){
    (void)s;return fake_measure(id,raw,cap,n,m);
}
static bool fake3_measure(mems_manager_mems3_session *s,uint8_t id,uint8_t *raw,size_t cap,size_t *n,mems_codec_measurements *m){
    (void)s;return fake_measure(id,raw,cap,n,m);
}
static bool fake2_fault(mems_manager_mems2j_session *s,uint8_t *raw,size_t cap,size_t *n){
    return fake2_measure(s,0x19,raw,cap,n,NULL);
}
static bool fake3_fault(mems_manager_mems3_session *s,uint8_t *raw,size_t cap,size_t *n){
    (void)s;CHECK(cap>=2);raw[0]=0x58;raw[1]=0;*n=2;return !fail_read;
}
static bool fake2_keep(mems_manager_mems2j_session *s){(void)s;return true;}
static bool fake3_keep(mems_manager_mems3_session *s){(void)s;return true;}
static bool fake2_last(const mems_manager_mems2j_session *s,mems_manager_mems2j_result *r,uint8_t *neg,uint32_t *ms){
    (void)s;(void)ms;*r=fail_read?MEMS_MANAGER_2J_NEGATIVE_RESPONSE:MEMS_MANAGER_2J_OK;*neg=fail_read?0x31:0;return true;
}
static bool fake3_last(const mems_manager_mems3_session *s,mems_manager_mems3_result *r,uint8_t *neg,uint32_t *ms){
    (void)s;(void)ms;*r=fail_read?MEMS_MANAGER_3_NEGATIVE_RESPONSE:MEMS_MANAGER_3_OK;*neg=fail_read?0x31:0;return true;
}
static void close2(mems_manager_mems2j_session *s){CHECK(s!=NULL);--g.live;++g.closed;}
static void close3(mems_manager_mems3_session *s){CHECK(s!=NULL);--g.live;++g.closed;}
static bool cancel(void *user){(void)user;return g.cancel_after&&g.n>=g.cancel_after;}
#define mems_manager_list_serial_adapters fake_list
#define mems_init fake_init
#define mems_connect fake_connect
#define mems_init_link fake_link
#define mems_cleanup fake_cleanup
#define mems_manager_mems19_connect_identify fake19
#define mems_manager_mems2j_connect fake2
#define mems_manager_mems3_connect fake3
#define mems_manager_mems2j_close close2
#define mems_manager_mems3_close close3
#define mems_read fake_legacy_read
#define mems_read_kline19 fake19_read
#define mems_heartbeat fake_heartbeat
#define mems_command_kline19 fake19_cmd
#define mems_manager_mems2j_read_measurement fake2_measure
#define mems_manager_mems3_read_measurement fake3_measure
#define mems_manager_mems2j_read_faults fake2_fault
#define mems_manager_mems3_read_faults fake3_fault
#define mems_manager_mems2j_keepalive fake2_keep
#define mems_manager_mems3_keepalive fake3_keep
#define mems_manager_mems2j_last_transaction fake2_last
#define mems_manager_mems3_last_transaction fake3_last
static COMMTIMEOUTS fake_timeouts={100,0,100,0,400};
static uint32_t fake_response_timeout;
static BOOL WINAPI fake_get_timeouts(HANDLE h,LPCOMMTIMEOUTS p){(void)h;*p=fake_timeouts;return TRUE;}
static BOOL WINAPI fake_set_timeouts(HANDLE h,LPCOMMTIMEOUTS p){(void)h;fake_timeouts=*p;return TRUE;}
static bool fake_native_get(mems_manager_session_timeouts *p){
    p->read_interval_ms=fake_timeouts.ReadIntervalTimeout;p->read_multiplier_ms=fake_timeouts.ReadTotalTimeoutMultiplier;
    p->read_constant_ms=fake_timeouts.ReadTotalTimeoutConstant;p->write_multiplier_ms=fake_timeouts.WriteTotalTimeoutMultiplier;
    p->write_constant_ms=fake_timeouts.WriteTotalTimeoutConstant;p->response_ms=fake_response_timeout;return true;
}
static bool fake_native_set(const mems_manager_session_timeouts *p){
    fake_timeouts.ReadIntervalTimeout=p->read_interval_ms;fake_timeouts.ReadTotalTimeoutMultiplier=p->read_multiplier_ms;
    fake_timeouts.ReadTotalTimeoutConstant=p->read_constant_ms;fake_timeouts.WriteTotalTimeoutMultiplier=p->write_multiplier_ms;
    fake_timeouts.WriteTotalTimeoutConstant=p->write_constant_ms;fake_response_timeout=p->response_ms;return true;
}
static bool fake2_get(const mems_manager_mems2j_session *s,mems_manager_session_timeouts *p){(void)s;return fake_native_get(p);}
static bool fake2_set(mems_manager_mems2j_session *s,const mems_manager_session_timeouts *p){(void)s;return fake_native_set(p);}
static bool fake3_get(const mems_manager_mems3_session *s,mems_manager_session_timeouts *p){(void)s;return fake_native_get(p);}
static bool fake3_set(mems_manager_mems3_session *s,const mems_manager_session_timeouts *p){(void)s;return fake_native_set(p);}
#define GetCommTimeouts fake_get_timeouts
#define SetCommTimeouts fake_set_timeouts
#define mems_internal_2j_get_timeouts fake2_get
#define mems_internal_2j_set_timeouts fake2_set
#define mems_internal_3_get_timeouts fake3_get
#define mems_internal_3_set_timeouts fake3_set
#include "auto_connect.c"
int main(void){
    mems_manager_auto_session *s=NULL;mems_manager_auto_identity id;mems_manager_auto_result r;
    reset();g.success_family=1;g.success_port=1;
    CHECK(mems_manager_auto_connect(NULL,&s,cancel,NULL,NULL,&r)&&r==MEMS_MANAGER_AUTO_OK);
    CHECK(g.n==1&&g.logs[0]==101&&g.live==1);
    CHECK(mems_manager_get_session_family(s)==MEMS_MANAGER_AUTO_ROSCO_1X);
    CHECK(mems_manager_get_session_identity(s,&id)&&id.has_d0_id&&id.d0_id[0]==0x42);
    CHECK(mems_manager_get_session_capabilities(s)&MEMS_MANAGER_AUTO_CAP_LEGACY_READ);
    mems_manager_disconnect_auto(s);s=NULL;CHECK(!g.live&&g.closed==1);
    reset();g.kline=true;g.success_family=19;g.success_port=1;
    CHECK(mems_manager_auto_connect(NULL,&s,cancel,NULL,NULL,&r));
    CHECK(g.n==3&&g.logs[0]==120&&g.logs[1]==130&&g.logs[2]==119);
    CHECK(mems_manager_get_session_identity(s,&id)&&id.has_d0_id&&id.local_echo&&id.d0_id[0]==0x19);
    mems_manager_disconnect_auto(s);s=NULL;CHECK(!g.live);
    reset();g.count=2;g.success_family=30;g.success_port=2;
    CHECK(mems_manager_auto_connect(NULL,&s,cancel,NULL,NULL,&r));
    CHECK(g.n==7&&g.logs[0]==101&&g.logs[1]==120&&g.logs[2]==130&&g.logs[3]==119&&g.logs[4]==201&&g.logs[5]==220&&g.logs[6]==230);
    CHECK(mems_manager_get_session_identity(s,&id)&&!id.has_d0_id&&id.family==30);
    CHECK(mems_manager_get_session_capabilities(s)&MEMS_MANAGER_AUTO_CAP_WIRE_TRACE);
    mems_manager_disconnect_auto(s);s=NULL;CHECK(!g.live);
    reset();g.kline=true;g.success_family=20;g.success_port=1;
    CHECK(mems_manager_auto_connect(NULL,&s,cancel,NULL,NULL,&r)&&g.n==1&&g.logs[0]==120);
    mems_manager_disconnect_auto(s);s=NULL;CHECK(!g.live);
    reset();CHECK(!mems_manager_auto_connect(NULL,&s,cancel,NULL,NULL,&r)&&r==MEMS_MANAGER_AUTO_NO_ECU&&g.n==4&&!g.live&&!s);
    reset();g.cancel_after=1;
    CHECK(!mems_manager_auto_connect(NULL,&s,cancel,NULL,NULL,&r)&&r==MEMS_MANAGER_AUTO_CANCELLED&&g.n==1&&!g.live&&!s);
    reset();g.count=0;CHECK(!mems_manager_auto_connect(NULL,&s,cancel,NULL,NULL,&r)&&r==MEMS_MANAGER_AUTO_NO_PORT);
    reset();g.enumeration_error=true;CHECK(!mems_manager_auto_connect(NULL,&s,cancel,NULL,NULL,&r)&&r==MEMS_MANAGER_AUTO_ENUMERATION_FAILED);
    CHECK(!mems_manager_auto_connect(NULL,NULL,NULL,NULL,NULL,&r)&&r==MEMS_MANAGER_AUTO_INVALID);
    CHECK(!mems_manager_get_session_identity(NULL,&id)&&!mems_manager_get_session_capabilities(NULL));
    mems_manager_disconnect_auto(NULL);
    {
        mems_data data;mems_manager_transport_error error;uint32_t duration;
        uint8_t raw[96],b[2]={0x61,0x09};size_t n;mems_codec_measurements metrics;
        mems_manager_trace_event ev;
        reset();g.success_family=1;g.success_port=1;
        CHECK(mems_manager_auto_connect(NULL,&s,cancel,NULL,NULL,&r));
        active=s;CHECK(!mems_manager_cancel_current(s));
        {
            mems_manager_session_timeouts tm,back;
            CHECK(mems_manager_get_session_timeouts(s,&tm)&&tm.read_constant_ms==100&&tm.response_ms==0);
            tm.read_constant_ms=150;
            CHECK(mems_manager_set_session_timeouts(s,&tm));
            CHECK(mems_manager_get_session_timeouts(s,&back)&&back.read_constant_ms==150);
            tm.response_ms=600;CHECK(!mems_manager_set_session_timeouts(s,&tm));
        }
        CHECK(!mems_manager_get_transport_error(s,&error));
        CHECK(mems_manager_auto_read_legacy(s,&data)&&data.engine_rpm==1234);
        CHECK(mems_manager_get_response_timing(s,&duration));
        CHECK(mems_manager_auto_keepalive(s));
        CHECK(!mems_manager_auto_read_measurement(s,9,raw,sizeof(raw),&n,&metrics));
        CHECK(mems_manager_get_transport_error(s,&error)&&error.status==MEMS_MANAGER_TRANSPORT_UNSUPPORTED&&!error.has_timing);
        cancel_in_read=true;data.engine_rpm=999;
        CHECK(!mems_manager_auto_read_legacy(s,&data)&&data.engine_rpm==999);
        CHECK(mems_manager_get_transport_error(s,&error)&&error.status==MEMS_MANAGER_TRANSPORT_CANCELLED);
        cancel_in_read=false;
        CHECK(mems_manager_auto_read_legacy(s,&data)&&data.engine_rpm==1234);
        mems_manager_disconnect_auto(s);s=NULL;
        reset();g.kline=true;g.success_family=20;g.success_port=1;
        CHECK(mems_manager_auto_connect(NULL,&s,cancel,NULL,NULL,&r));active=s;
        {
            mems_manager_session_timeouts tm,back;
            CHECK(mems_manager_get_session_timeouts(s,&tm));
            tm.response_ms=700;
            CHECK(mems_manager_set_session_timeouts(s,&tm));
            CHECK(mems_manager_get_session_timeouts(s,&back)&&back.response_ms==700);
            tm.response_ms=0;CHECK(mems_manager_set_session_timeouts(s,&tm));
        }
        CHECK(mems_manager_auto_read_measurement(s,9,raw,sizeof(raw),&n,&metrics)&&n==4&&raw[1]==9&&metrics.id==9);
        CHECK(mems_manager_auto_read_faults(s,raw,sizeof(raw),&n)&&raw[1]==0x19);
        CHECK(mems_manager_auto_keepalive(s));
        fail_read=true;n=999;
        CHECK(!mems_manager_auto_read_measurement(s,9,raw,sizeof(raw),&n,NULL)&&n==999);
        CHECK(mems_manager_get_transport_error(s,&error)&&error.status==MEMS_MANAGER_TRANSPORT_NATIVE_FAILURE&&error.has_native_status&&error.negative_response_code==0x31);
        fail_read=false;cancel_in_read=true;
        CHECK(!mems_manager_auto_read_measurement(s,9,raw,sizeof(raw),&n,NULL)&&n==999);
        cancel_in_read=false;
        CHECK(mems_manager_auto_read_measurement(s,9,raw,sizeof(raw),&n,NULL)&&n==4);
        mems_manager_clear_trace(s);
        for(unsigned i=0;i<MEMS_MANAGER_TRACE_EVENTS+3;++i)native_trace(s,2,i,b,sizeof(b));
        CHECK(mems_manager_get_next_trace_event(s,&ev)&&ev.dropped_total==3&&ev.family==20&&ev.length==2&&ev.bytes[1]==9&&strcmp(ev.port,"COM1")==0);
        CHECK(ev.tick_ms==3);
        mems_manager_clear_trace(s);CHECK(!mems_manager_get_next_trace_event(s,&ev));
        mems_manager_disconnect_auto(s);s=NULL;CHECK(!g.live);
    }
    if(failures)return 1;
    puts("AUTO_CONNECT_SELFTEST_PASS: protocol order, ECU confirmation, fallback, cleanup, cancellation, identity and capabilities");
    return 0;
}
