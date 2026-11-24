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
    if(failures)return 1;
    puts("AUTO_CONNECT_SELFTEST_PASS: protocol order, ECU confirmation, fallback, cleanup, cancellation, identity and capabilities");
    return 0;
}
