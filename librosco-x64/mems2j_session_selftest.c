/*
 * Offline native MEMS2J full handshake/transport regression.
 * Production mems2j_session.c included verbatim, every Win32 serial,
 * clock and Sleep function intercepted ONLY inside this test executable.
 * No Windows COM port or physical ECU opened.
 */
#include "mems2j_session.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

typedef struct {
    ULONGLONG clock_ms;
    uint8_t queue[4096];
    size_t total, cursor;
    unsigned int opens, closes, break_on, break_off;
    unsigned int init_count, start_count, seed_count, key_count;
    unsigned int keepalives, read_count, clear_count, total_writes;
    unsigned int trace_tx, trace_rx;
    unsigned int callback_results, callback_positive, callback_raw_only;
    unsigned int ports_10400;
    bool echo;
    bool no_wake;
    bool no_key;
    bool fail_port;
    bool fail_break;
    bool fail_write;
    bool pending_first;
    bool pending_used;
    bool negative_faults;
    bool negative_rpm;
    bool negative_clear;
    bool wrong_key;
    bool reject_everything;
    unsigned int cancel_after;
    uint8_t readings[48];
    size_t reading_count;
    uint8_t key_hi,key_lo;
} mock_2j;
static mock_2j g;
static int failures;
#define CHECK(p) do { if (!(p)) { \
 fprintf(stderr,"MEMS2J_SESSION_SELFTEST_FAIL line %d %s\n",__LINE__,#p); \
 failures++; \
} } while(0)

static void reset_fake(void)
{
    memset(&g,0,sizeof(g));
    g.echo=true;
}
static bool push_bytes(const uint8_t* b,size_t count)
{
    if (!b || g.total+count>sizeof(g.queue))return false;
    memcpy(g.queue+g.total,b,count);
    g.total+=count;
    return true;
}
static bool push_reply(const uint8_t* payload,size_t count)
{
    uint8_t frame[128];
    size_t n=0;
    return mems2j_frame_encode(payload,count,frame,sizeof(frame),&n) &&
           push_bytes(frame,n);
}
static HANDLE WINAPI FakeCreateFileA(LPCSTR path,DWORD access,DWORD sharing,
                                      LPSECURITY_ATTRIBUTES security,
                                      DWORD mode,DWORD attr,HANDLE template_file)
{
    (void)access;(void)sharing;(void)security;(void)mode;
    (void)attr;(void)template_file;
    g.opens++;
    if (g.fail_port || !path || strcmp(path,"INVALID")==0)
        return INVALID_HANDLE_VALUE;
    return (HANDLE)(uintptr_t)0xC0FFEE;
}
static BOOL WINAPI FakeCloseHandle(HANDLE h)
{
    (void)h;
    g.closes++;
    return TRUE;
}
static BOOL WINAPI FakeGetCommState(HANDLE h,LPDCB dcb)
{
    (void)h;
    return dcb && dcb->DCBlength==sizeof(*dcb);
}
static BOOL WINAPI FakeSetCommState(HANDLE h,LPDCB dcb)
{
    (void)h;
    if(!dcb)return FALSE;
    if(dcb->BaudRate==10400 && dcb->ByteSize==8 &&
       dcb->Parity==NOPARITY && dcb->StopBits==ONESTOPBIT)
        g.ports_10400++;
    else return FALSE;
    return TRUE;
}
static BOOL WINAPI FakeGetCommTimeouts(HANDLE h,LPCOMMTIMEOUTS t)
{
    (void)h;
    if(!t)return FALSE;
    memset(t,0,sizeof(*t));
    return TRUE;
}
static BOOL WINAPI FakeSetCommTimeouts(HANDLE h,LPCOMMTIMEOUTS t)
{
    (void)h;
    return t && t->ReadTotalTimeoutConstant==40 &&
           t->WriteTotalTimeoutConstant==400;
}
static BOOL WINAPI FakePurgeComm(HANDLE h,DWORD mask)
{
    (void)h;
    if(mask & PURGE_RXCLEAR)g.total=g.cursor=0;
    return TRUE;
}
static BOOL WINAPI FakeSetCommBreak(HANDLE h)
{
    (void)h;
    g.break_on++;
    return !g.fail_break;
}
static BOOL WINAPI FakeClearCommBreak(HANDLE h)
{
    (void)h;
    g.break_off++;
    return !g.fail_break;
}
static ULONGLONG WINAPI FakeGetTickCount64(void){return g.clock_ms;}
static VOID WINAPI FakeSleep(DWORD n) {g.clock_ms+=(n?n:1);}
static BOOL WINAPI FakeFlushFileBuffers(HANDLE h)
{
    (void)h;
    return TRUE;
}
static BOOL WINAPI FakeWriteFile(HANDLE h,LPCVOID bytes,DWORD n,
                                 LPDWORD written,LPOVERLAPPED o)
{
    const uint8_t *wire=(const uint8_t*)bytes;
    uint8_t cmd[96];
    size_t cmd_n=0;
    (void)h;(void)o;
    if (!wire || !written || g.fail_write || g.reject_everything)return FALSE;
    *written=n;
    g.total_writes++;
    /* Some KKL devices echo every byte. Echo may arrive fragmented. */
    if (g.echo && !push_bytes(wire,n))return FALSE;
    if (n==5 && memcmp(wire,"\x81\x13\xf7\x81\x0c",5)==0) {
        const uint8_t ack[]={0xC1,0xD5,0x8F};
        g.init_count++;
        if (!g.no_wake && !push_reply(ack,sizeof(ack)))return FALSE;
        return TRUE;
    }
    if(!mems2j_frame_decode(wire,n,cmd,sizeof(cmd),&cmd_n))
        return FALSE;
    if (cmd_n>=2 && cmd[0]==0x10 && cmd[1]==0xA0) {
        const uint8_t ack[]={0x50};
        g.start_count++;
        return push_reply(ack,sizeof(ack));
    }
    if (cmd_n>=2 && cmd[0]==0x27 && cmd[1]==0x01) {
        const uint8_t ack[]={0x67,0x01,0x12,0x34};
        g.seed_count++;
        if(g.no_key)return TRUE;
        return push_reply(ack,sizeof(ack));
    }
    if (cmd_n>=4 && cmd[0]==0x27 && cmd[1]==0x02) {
        const uint8_t ack[]={0x67,0x02};
        g.key_count++;
        g.key_hi=cmd[2]; g.key_lo=cmd[3];
        if (g.wrong_key ||
            ((uint16_t)cmd[2]<<8 | cmd[3])!=0x8247)
            return TRUE;
        return push_reply(ack,sizeof(ack));
    }
    if (cmd_n && cmd[0]==0x3E) {
        const uint8_t ack[]={0x7E};
        g.keepalives++;
        return push_reply(ack,sizeof(ack));
    }
    if (cmd_n>=2 && cmd[0]==0x21) {
        uint8_t reply[]={0x61,0,0x0B,0xB8};
        const uint8_t pending[]={0x7F,0x21,0x78};
        const uint8_t denied[]={0x7F,0x21,0x31};
        g.read_count++;
        if(g.reading_count<sizeof(g.readings))
            g.readings[g.reading_count++]=cmd[1];
        if(g.pending_first && !g.pending_used) {
            g.pending_used=true;
            if(!push_reply(pending,sizeof(pending)))return FALSE;
        }
        if((g.negative_faults && cmd[1]==0x19) ||
           (g.negative_rpm && cmd[1]==0x09))
            return push_reply(denied,sizeof(denied));
        reply[1]=cmd[1];
        if(cmd[1]==0x19) {
            const uint8_t raw_faults[]={0x61,0x19,0x00,0x00,0x12};
            return push_reply(raw_faults,sizeof(raw_faults));
        }
        return push_reply(reply,sizeof(reply));
    }
    if (cmd_n>=2 && cmd[0]==0x31 && cmd[1]==0xCB) {
        const uint8_t positive[]={0x71,0xCB};
        const uint8_t negative[]={0x7F,0x31,0x22};
        g.clear_count++;
        CHECK(cmd_n==17);
        return g.negative_clear ?
            push_reply(negative,sizeof(negative)):
            push_reply(positive,sizeof(positive));
    }
    return FALSE;
}
static BOOL WINAPI FakeReadFile(HANDLE h,LPVOID out,DWORD max,LPDWORD got,
                                LPOVERLAPPED o)
{
    size_t n;
    (void)h;(void)o;
    if (!out || !got)return FALSE;
    n=g.total-g.cursor;
    if(n>max)n=max;
    if(n>3)n=3; /* Split initial command, echo, pending, data packets. */
    if(n)memcpy(out,g.queue+g.cursor,n);
    g.cursor+=n;
    *got=(DWORD)n;
    return TRUE;
}

/* This fake input/test compile contains the EXACT production module with
 * every external Win32 call intercepted by deterministic fake functions. */
#define CreateFileA FakeCreateFileA
#define CloseHandle FakeCloseHandle
#define GetCommState FakeGetCommState
#define SetCommState FakeSetCommState
#define GetCommTimeouts FakeGetCommTimeouts
#define SetCommTimeouts FakeSetCommTimeouts
#define PurgeComm FakePurgeComm
#define SetCommBreak FakeSetCommBreak
#define ClearCommBreak FakeClearCommBreak
#define GetTickCount64 FakeGetTickCount64
#define Sleep FakeSleep
#define FlushFileBuffers FakeFlushFileBuffers
#define WriteFile FakeWriteFile
#define ReadFile FakeReadFile
#include "mems2j_session.c"
#undef CreateFileA
#undef CloseHandle
#undef GetCommState
#undef SetCommState
#undef GetCommTimeouts
#undef SetCommTimeouts
#undef PurgeComm
#undef SetCommBreak
#undef ClearCommBreak
#undef GetTickCount64
#undef Sleep
#undef FlushFileBuffers
#undef WriteFile
#undef ReadFile

static bool fake_cancel(void* ctx)
{
    (void)ctx;
    return g.cancel_after!=0 && g.clock_ms>=g.cancel_after;
}
static void collect_trace(void* user,int direction,uint64_t at,
                          const uint8_t* bytes,size_t n)
{
    (void)user;
    CHECK(n>0 && bytes!=NULL && at==g.clock_ms);
    if (direction==1)g.trace_tx++;
    else if(direction==2)g.trace_rx++;
    else CHECK(0);
}
static void collect_cycle(void* user,uint8_t id,bool ok,
                          const uint8_t* bytes,size_t n)
{
    (void)user;(void)id;
    g.callback_results++;
    if(ok) {
        CHECK(bytes!=NULL && n>=2 && bytes[0]==0x61 && bytes[1]==id);
        g.callback_positive++;
        if(id==0x00)g.callback_raw_only++;
    }else CHECK(n==0 && bytes==NULL);
}

int main(void)
{
    mems_manager_mems2j_session *s=NULL;
    mems_manager_mems2j_result result;
    uint8_t neg=0,raw[96];
    uint32_t timing=0;
    size_t length=0;
    unsigned int success=0;
    mems_codec_measurements decoded;

    reset_fake();
    CHECK(mems_manager_mems2j_connect("COM99",&s,NULL,collect_trace,NULL,&result));
    CHECK(s!=NULL && result==MEMS_MANAGER_2J_OK);
    CHECK(mems_manager_mems2j_is_open(s));
    CHECK(g.opens==1 && g.closes==0 && g.ports_10400==1);
    CHECK(g.break_on==1 && g.break_off>=2);
    CHECK(g.init_count==1 && g.start_count==1 && g.seed_count==1 && g.key_count==1);
    CHECK(g.key_hi==0x82 && g.key_lo==0x47);
    CHECK(g.keepalives==1 && g.read_count==1 && g.clear_count==0);
    CHECK(g.trace_tx>=6 && g.trace_rx>=6);
    CHECK(mems_manager_mems2j_read_measurement(
        s,0x09,raw,sizeof(raw),&length,&decoded));
    CHECK(length==4 && raw[0]==0x61 && raw[1]==0x09);
    CHECK(decoded.count==1 && decoded.metrics[0].value==3000);
    CHECK(mems_manager_mems2j_read_faults(s,raw,sizeof(raw),&length));
    CHECK(length==5 && raw[1]==0x19 && raw[4]==0x12);
    CHECK(mems_manager_mems2j_clear_faults(s) && g.clear_count==1);
    CHECK(mems_manager_mems2j_keepalive(s));
    CHECK(mems_manager_mems2j_last_transaction(s,&result,&neg,&timing));
    CHECK(result==MEMS_MANAGER_2J_OK && neg==0 && timing<=350);
    CHECK(mems_manager_mems2j_read_cycle(s,collect_cycle,NULL,&success));
    CHECK(success==7 && g.callback_results==7 && g.callback_positive==7);
    CHECK(g.reading_count>=7 && g.readings[g.reading_count-7]==0x09);
    CHECK(g.readings[g.reading_count-6]==0x19);
    CHECK(g.callback_raw_only==1); /* first 'extra' = 0x00 */
    mems_manager_mems2j_close(s);s=NULL;
    CHECK(g.closes==1);

    /* No echo is a legitimate cable mode; sensor 09 may be rejected
     * during optional Connect probe without invalidating the session. */
    reset_fake();
    g.echo=false; g.negative_rpm=true;
    CHECK(mems_manager_mems2j_connect("COM99",&s,NULL,collect_trace,NULL,&result));
    CHECK(mems_manager_mems2j_is_open(s) && g.clear_count==0);
    g.negative_rpm=false;
    g.negative_faults=true;
    length=1984;
    CHECK(!mems_manager_mems2j_read_faults(s,raw,sizeof(raw),&length));
    CHECK(length==1984);
    CHECK(mems_manager_mems2j_last_transaction(s,&result,&neg,&timing));
    CHECK(result==MEMS_MANAGER_2J_NEGATIVE_RESPONSE && neg==0x31);
    g.negative_faults=false;
    g.pending_first=true;
    CHECK(mems_manager_mems2j_read_faults(s,raw,sizeof(raw),&length));
    CHECK(length==5 && result==MEMS_MANAGER_2J_NEGATIVE_RESPONSE);
    g.negative_clear=true;
    CHECK(!mems_manager_mems2j_clear_faults(s));
    CHECK(mems_manager_mems2j_last_transaction(s,&result,&neg,NULL));
    CHECK(result==MEMS_MANAGER_2J_NEGATIVE_RESPONSE && neg==0x22);
    mems_manager_mems2j_close(s);s=NULL;

    reset_fake();
    g.no_wake=true;
    CHECK(!mems_manager_mems2j_connect("COM99",&s,NULL,NULL,NULL,&result));
    CHECK(s==NULL && result==MEMS_MANAGER_2J_TIMEOUT && g.closes==1);
    CHECK(g.start_count==0 && g.clear_count==0);

    reset_fake();
    g.no_key=true;
    CHECK(!mems_manager_mems2j_connect("COM99",&s,NULL,NULL,NULL,&result));
    CHECK(s==NULL && result==MEMS_MANAGER_2J_TIMEOUT && g.closes==1);
    CHECK(g.key_count==0);

    reset_fake();
    g.wrong_key=true;
    CHECK(!mems_manager_mems2j_connect("COM99",&s,NULL,NULL,NULL,&result));
    CHECK(s==NULL && result==MEMS_MANAGER_2J_TIMEOUT && g.closes==1);
    CHECK(g.clear_count==0);

    reset_fake();
    g.fail_port=true;
    CHECK(!mems_manager_mems2j_connect("COM99",&s,NULL,NULL,NULL,&result));
    CHECK(s==NULL && result==MEMS_MANAGER_2J_PORT_FAILURE);

    reset_fake();
    g.fail_break=true;
    CHECK(!mems_manager_mems2j_connect("COM99",&s,NULL,NULL,NULL,&result));
    CHECK(s==NULL && result==MEMS_MANAGER_2J_PORT_FAILURE && g.closes==1);

    reset_fake();
    g.fail_write=true;
    CHECK(!mems_manager_mems2j_connect("COM99",&s,NULL,NULL,NULL,&result));
    CHECK(s==NULL && result==MEMS_MANAGER_2J_WRITE_FAILURE && g.closes==1);

    reset_fake();
    g.cancel_after=220;
    CHECK(!mems_manager_mems2j_connect("COM99",&s,fake_cancel,NULL,NULL,&result));
    CHECK(s==NULL && result==MEMS_MANAGER_2J_CANCELLED);
    CHECK(g.clear_count==0 && g.closes==1);

    reset_fake();
    CHECK(!mems_manager_mems2j_connect(NULL,&s,NULL,NULL,NULL,&result));
    CHECK(result==MEMS_MANAGER_2J_INVALID_ARGUMENT);
    CHECK(!mems_manager_mems2j_connect("COM99",NULL,NULL,NULL,NULL,&result));
    CHECK(!mems_manager_mems2j_is_open(NULL));
    CHECK(!mems_manager_mems2j_keepalive(NULL));
    CHECK(!mems_manager_mems2j_clear_faults(NULL));
    mems_manager_mems2j_close(NULL);

    if(failures){
        fprintf(stderr,"MEMS2J_SESSION_SELFTEST_FAIL %d\n",failures);
        return 1;
    }
    puts("MEMS2J_SESSION_SELFTEST_PASS: 10400/8N1 25ms BREAK, full auth, echo, 21xx, DTC, 31CB, pending, errors and trace");
    return 0;
}
