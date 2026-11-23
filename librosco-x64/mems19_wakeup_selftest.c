/* Offline, same-handle MEMS 1.9 native wake, testing production
 * mems19_wakeup.c against fully simulated Win32 serial/clock functions.
 * No real COM port is ever opened. No vehicle commands are transmitted.
 */
#include "mems19_wakeup.h"
#include <string.h>
#include <stdio.h>
#include <stdint.h>

typedef struct {
    ULONGLONG tick;
    bool connected;
    bool echo;
    bool no_keys;
    bool bad_key;
    bool no_confirm;
    bool failed_ack;
    bool failed_break;
    bool no_port;
    bool failed_init;
    ULONGLONG cancel_after;
    bool keys_offered;
    int break_cells;
    bool bit_levels[10];
    int opens;
    int closes;
    int purge_calls;
    int init_calls;
    bool init_echo;
    uint8_t received[64];
    size_t rsize;
    size_t ri;
    uint8_t sent[32];
    size_t nsent;
} fake_mems19;
static fake_mems19 g;
static int failures;
#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr,"MEMS19_WAKE_SELFTEST_FAIL line %d: %s\n",__LINE__,#expr); \
    failures++; \
} } while(0)

static void reset_fake(void) { memset(&g,0,sizeof(g)); g.connected=true; }
static bool enqueue(const uint8_t *src,size_t n)
{
    if(g.rsize+n>sizeof(g.received))return false;
    memcpy(g.received+g.rsize,src,n);
    g.rsize+=n;
    return true;
}

static ULONGLONG WINAPI MockTick(void) {return g.tick;}
static VOID WINAPI MockSleep(DWORD n) {g.tick += (n ? n : 1);}
static BOOL WINAPI MockPurge(HANDLE h,DWORD flags)
{
    (void)h; (void)flags;
    g.purge_calls++;
    g.rsize=g.ri=0;
    return TRUE;
}
static BOOL WINAPI MockBreakOn(HANDLE h)
{
    (void)h;
    if(g.failed_break)return FALSE;
    if(g.break_cells<10)g.bit_levels[g.break_cells++]=false;
    return TRUE;
}
static BOOL WINAPI MockBreakOff(HANDLE h)
{
    (void)h;
    if(g.failed_break)return FALSE;
    /* Ignore the initial line-idle clear and the trailing release,
     * but count the actual 10 slow-cell transitions by their timing. */
    if(g.tick>=2000 && g.tick<4000 && g.break_cells<10)
        g.bit_levels[g.break_cells++]=true;
    return TRUE;
}
static BOOL WINAPI MockRead(HANDLE h,LPVOID buffer,DWORD count,
                             LPDWORD got,LPOVERLAPPED ov)
{
    (void)h;(void)ov;
    if(!got||!buffer||count!=1)return FALSE;
    if(!g.keys_offered && g.tick>=4000 && !g.no_keys) {
        const uint8_t key_ok[]={0x55,0x76,0x83};
        const uint8_t key_wrong[]={0x00,0x76,0x83};
        if(!enqueue(g.bad_key?key_wrong:key_ok,3))return FALSE;
        g.keys_offered=true;
    }
    if(g.ri<g.rsize) {
        ((uint8_t*)buffer)[0]=g.received[g.ri++];
        *got=1;
        return TRUE;
    }
    *got=0;
    return TRUE;
}
static BOOL WINAPI MockWrite(HANDLE h,LPCVOID buffer,DWORD count,
                              LPDWORD sent,LPOVERLAPPED ov)
{
    const uint8_t* bytes=(const uint8_t*)buffer;
    (void)h;(void)ov;
    if(!bytes||!sent||count!=1||g.nsent+count>sizeof(g.sent)||g.failed_ack)
        return FALSE;
    g.sent[g.nsent++]=bytes[0];
    *sent=1;
    if(!g.no_confirm){
        uint8_t response[]={0x7C,0xE9};
        if(g.echo && !enqueue(response,2))return FALSE;
        if(!g.echo && !enqueue(response+1,1))return FALSE;
    }
    return TRUE;
}

bool mems_is_connected(mems_info* info)
{
    return info!=NULL && g.connected;
}
bool mems_connect(mems_info* info,const char* path)
{
    if(!info||!path||g.no_port)return false;
    g.opens++;
    g.connected=true;
    info->sd=(HANDLE)(uintptr_t)0xCAFE;
    return true;
}
void mems_disconnect(mems_info* info)
{
    if(info) info->sd=INVALID_HANDLE_VALUE;
    g.connected=false;
    g.closes++;
}
bool mems_init_link_kline19(mems_info* info,uint8_t* id,bool echo)
{
    static const uint8_t ecu[]={0x11,0x22,0x33,0x44};
    (void)info;
    g.init_calls++;
    g.init_echo=echo;
    if(g.failed_init)return false;
    memcpy(id,ecu,4);
    return true;
}

/* Only this FAKE test binary has Win32 APIs mapped to simulators. */
#define GetTickCount64 MockTick
#define Sleep MockSleep
#define PurgeComm MockPurge
#define SetCommBreak MockBreakOn
#define ClearCommBreak MockBreakOff
#define ReadFile MockRead
#define WriteFile MockWrite
#include "mems19_wakeup.c"
#undef GetTickCount64
#undef Sleep
#undef PurgeComm
#undef SetCommBreak
#undef ClearCommBreak
#undef ReadFile
#undef WriteFile

static bool simulated_cancel(void *ctx)
{
    (void)ctx;
    return g.cancel_after != 0 && g.tick>=g.cancel_after;
}
static void check_bits(void)
{
    static const bool expected[]={false,false,true,true,false,true,
                                   false,false,false,true};
    int i;
    CHECK(g.break_cells==10);
    for(i=0;i<10;++i)CHECK(g.bit_levels[i]==expected[i]);
}
int main(void)
{
    mems_info info;
    uint8_t id[]={0xAA,0xAA,0xAA,0xAA};
    bool echo=false;
    mems_manager_mems19_wakeup_result status;
    memset(&info,0,sizeof(info));
    info.sd=(HANDLE)(uintptr_t)0xCAFE;
    info.mutex=(HANDLE)(uintptr_t)0xBEEF;

    reset_fake();
    g.echo=true;
    CHECK(mems_manager_mems19_wakeup(&info,&echo,&status,NULL,NULL));
    CHECK(status==MEMS_MANAGER_MEMS19_WAKE_OK && echo);
    CHECK(g.tick>=4050 && g.tick<5000);
    CHECK(g.purge_calls==1 && g.nsent==1 && g.sent[0]==0x7C);
    CHECK(g.opens==0 && g.closes==0);
    check_bits();

    reset_fake();
    echo=true;
    CHECK(mems_manager_mems19_wakeup(&info,&echo,&status,NULL,NULL));
    CHECK(status==MEMS_MANAGER_MEMS19_WAKE_OK && !echo);
    CHECK(g.nsent==1 && g.sent[0]==0x7C);
    check_bits();

    reset_fake();
    g.no_keys=true;
    echo=true;
    CHECK(!mems_manager_mems19_wakeup(&info,&echo,&status,NULL,NULL));
    CHECK(status==MEMS_MANAGER_MEMS19_WAKE_NO_KEY_BYTES && !echo && g.nsent==0);

    reset_fake();
    g.bad_key=true;
    CHECK(!mems_manager_mems19_wakeup(&info,&echo,&status,NULL,NULL));
    CHECK(status==MEMS_MANAGER_MEMS19_WAKE_NO_KEY_BYTES && g.nsent==0);

    reset_fake();
    g.no_confirm=true;
    CHECK(!mems_manager_mems19_wakeup(&info,&echo,&status,NULL,NULL));
    CHECK(status==MEMS_MANAGER_MEMS19_WAKE_NO_ADDRESS_CONFIRMATION && g.nsent==1);

    reset_fake();
    g.failed_ack=true;
    CHECK(!mems_manager_mems19_wakeup(&info,&echo,&status,NULL,NULL));
    CHECK(status==MEMS_MANAGER_MEMS19_WAKE_ACK_FAILED && g.nsent==0);

    reset_fake();
    g.failed_break=true;
    CHECK(!mems_manager_mems19_wakeup(&info,&echo,&status,NULL,NULL));
    CHECK(status==MEMS_MANAGER_MEMS19_WAKE_BREAK_FAILED);

    reset_fake();
    g.cancel_after=2250;
    CHECK(!mems_manager_mems19_wakeup(&info,&echo,&status,
                                     simulated_cancel,NULL));
    CHECK(status==MEMS_MANAGER_MEMS19_WAKE_CANCELLED && g.nsent==0);

    reset_fake();
    g.connected=false;
    CHECK(!mems_manager_mems19_wakeup(&info,&echo,&status,NULL,NULL));
    CHECK(status==MEMS_MANAGER_MEMS19_WAKE_NO_OPEN_PORT);

    /* Integrated open+wake+ID: same connected session, no reopen. */
    reset_fake();
    g.connected=false;
    g.echo=true;
    CHECK(mems_manager_mems19_connect_identify(&info,"COM99",id,&echo,&status,
                                                NULL,NULL));
    CHECK(status==MEMS_MANAGER_MEMS19_WAKE_OK && echo);
    CHECK(g.opens==1 && g.closes==0 && g.init_calls==1 && g.init_echo);
    CHECK(id[0]==0x11 && id[3]==0x44 && g.nsent==1 && g.sent[0]==0x7C);
    mems_disconnect(&info);

    reset_fake();
    g.connected=false;
    g.failed_init=true;
    id[0]=0xAA;
    CHECK(!mems_manager_mems19_connect_identify(&info,"COM99",id,&echo,&status,
                                                 NULL,NULL));
    CHECK(status==MEMS_MANAGER_MEMS19_WAKE_LINK_INIT_FAILED);
    CHECK(g.opens==1 && g.closes==1 && g.init_calls==1);
    CHECK(id[0]==0xAA); /* Do not leak partial D0 on failure. */

    reset_fake();
    g.connected=false;
    g.no_port=true;
    CHECK(!mems_manager_mems19_connect_identify(&info,"COM99",id,&echo,&status,
                                                 NULL,NULL));
    CHECK(status==MEMS_MANAGER_MEMS19_WAKE_OPEN_FAILED);

    reset_fake();
    CHECK(!mems_manager_mems19_connect_identify(&info,"COM99",id,&echo,&status,
                                                 NULL,NULL));
    CHECK(status==MEMS_MANAGER_MEMS19_WAKE_NO_OPEN_PORT);
    CHECK(g.opens==0); /* No implicit takeover of live ECU session. */

    if(failures)return 1;
    puts("MEMS19_WAKE_SELFTEST_PASS: full same-handle 5baud, keys, echo, D0, cancellation and errors");
    return 0;
}
