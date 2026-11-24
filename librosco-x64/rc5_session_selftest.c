/* Isolated Win32 simulator. Every serial call in production source is mocked. */
#include "rc5_session.h"
#include <stdio.h>
#include <string.h>
static struct {
    ULONGLONG clock;
    uint8_t q[256];size_t n,pos;
    unsigned opens,closes,levels,tx,rx,pings,reads,clears;
    bool no_wake,no_ping,no_clear,empty,echo,bad_checksum,fail_port,cancel;
    bool level_log[16];ULONGLONG time_log[16];
} g;
static int failures;
#define CHECK(x) do {if(!(x)){fprintf(stderr,"RC5_SESSION_SELFTEST_FAIL %d %s\n",__LINE__,#x);++failures;}}while(0)
static void reset(void){memset(&g,0,sizeof(g));g.echo=true;}
static void push(const uint8_t *p,size_t n){if(g.n+n<=sizeof(g.q)){memcpy(g.q+g.n,p,n);g.n+=n;}else CHECK(0);}
static void reply(uint8_t cmd,const uint8_t *p,size_t n){
    uint8_t buf[64];unsigned sum=0;size_t i;
    buf[0]=(uint8_t)(0xC0u+n+2);buf[1]=cmd;
    if(n)memcpy(buf+2,p,n);
    for(i=0;i<n+2;++i)sum+=buf[i];
    buf[n+2]=(uint8_t)(0xFFu-sum);
    if(g.bad_checksum)buf[n+2]^=1;
    push(buf,n+3);
}
static HANDLE WINAPI FakeCreateFileA(LPCSTR p,DWORD a,DWORD b,LPSECURITY_ATTRIBUTES c,DWORD d,DWORD e,HANDLE f)
{(void)a;(void)b;(void)c;(void)d;(void)e;(void)f;++g.opens;return p&&!g.fail_port?(HANDLE)(uintptr_t)123:INVALID_HANDLE_VALUE;}
static BOOL WINAPI FakeCloseHandle(HANDLE h){(void)h;++g.closes;return TRUE;}
static BOOL WINAPI FakeGetCommState(HANDLE h,LPDCB p){(void)h;return p&&p->DCBlength==sizeof(*p);}
static BOOL WINAPI FakeSetCommState(HANDLE h,LPDCB p){(void)h;return p&&p->BaudRate==2400&&p->ByteSize==8&&p->Parity==NOPARITY&&p->StopBits==ONESTOPBIT&&!p->fParity;}
static BOOL WINAPI FakeGetCommTimeouts(HANDLE h,LPCOMMTIMEOUTS p){(void)h;memset(p,0,sizeof(*p));return TRUE;}
static BOOL WINAPI FakeSetCommTimeouts(HANDLE h,LPCOMMTIMEOUTS p){(void)h;return p&&p->ReadTotalTimeoutConstant==40;}
static BOOL WINAPI FakePurgeComm(HANDLE h,DWORD mask){(void)h;if(mask&PURGE_RXCLEAR)g.n=g.pos=0;return TRUE;}
static BOOL record_level(bool b){
    const uint8_t wake[]={0x55,0x06,0x3B};
    if(g.levels<16){g.level_log[g.levels]=b;g.time_log[g.levels]=g.clock;}
    ++g.levels;
    if(g.levels==7&&!g.no_wake)push(wake,sizeof(wake));
    return TRUE;
}
static BOOL WINAPI FakeSetCommBreak(HANDLE h){(void)h;return record_level(true);}
static BOOL WINAPI FakeClearCommBreak(HANDLE h){(void)h;return record_level(false);}
static ULONGLONG WINAPI FakeGetTickCount64(void){return g.clock;}
static VOID WINAPI FakeSleep(DWORD n){g.clock+=n?n:1;}
static BOOL WINAPI FakeFlushFileBuffers(HANDLE h){(void)h;return TRUE;}
static BOOL WINAPI FakeWriteFile(HANDLE h,LPCVOID p,DWORD n,LPDWORD done,LPOVERLAPPED o){
    const uint8_t *b=(const uint8_t*)p;const uint8_t faults[]={0x15,0x0A,0x25,0x0B,0x12,0x34};
    (void)h;(void)o;
    if(n!=3||!mems_rc5_validate_frame(b,n)||b[0]!=0x82)return FALSE;
    *done=n;if(g.echo)push(b,n);
    switch(b[1]){
      case 0x00:++g.pings;if(!g.no_ping)reply(0x00,NULL,0);break;
      case 0x33:++g.reads;reply(0x33,faults,g.empty?0:sizeof(faults));break;
      case 0xC3:++g.clears;if(!g.no_clear)reply(0xC3,NULL,0);break;
      default:return FALSE;
    }
    return TRUE;
}
static BOOL WINAPI FakeReadFile(HANDLE h,LPVOID p,DWORD n,LPDWORD got,LPOVERLAPPED o){
    size_t count=g.n-g.pos;(void)h;(void)o;
    if(count>n)count=n;if(count>2)count=2;
    if(count)memcpy(p,g.q+g.pos,count);
    g.pos+=count;*got=(DWORD)count;return TRUE;
}
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
#include "rc5_session.c"
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
static bool cancelled(void *user){(void)user;return g.cancel;}
static void trace(void *user,int dir,uint64_t time,const uint8_t *p,size_t n){
    (void)user;CHECK(time==g.clock&&p&&n);if(dir==1)++g.tx;else if(dir==2)++g.rx;else CHECK(0);
}
int main(void){
    mems_manager_rc5_session *s=NULL;uint8_t p[64];size_t n=0;
    uint16_t code;bool historical,known;int key;unsigned i;
    static const ULONGLONG times[]={0,2000,2200,2600,3000,3400,3800};
    reset();
    CHECK(mems_manager_rc5_open_transport("COM99",&s,cancelled,trace,NULL));
    CHECK(s&&g.opens==1&&g.levels==0);
    CHECK(!mems_manager_rc5_ping(s));
    CHECK(!mems_manager_rc5_read_faults(s,p,sizeof(p),&n));
    CHECK(!mems_manager_rc5_clear_faults(s,true)&&g.clears==0);
    CHECK(mems_manager_rc5_wakeup(s)&&g.levels==7);
    for(i=0;i<7;++i){CHECK(g.level_log[i]==(i%2!=0));CHECK(g.time_log[i]==times[i]);}
    CHECK(!mems_manager_rc5_read_faults(s,p,sizeof(p),&n));
    CHECK(mems_manager_rc5_ping(s)&&g.pings==1&&g.clears==0);
    CHECK(mems_manager_rc5_read_faults(s,p,sizeof(p),&n)&&n==9);
    CHECK(mems_rc5_fault_count(p,n)==3);
    CHECK(mems_manager_rc5_decode_fault_detail(p,n,0,&code,&historical,&known,&key));
    CHECK(code==0x150A&&!historical&&known&&key==16840);
    CHECK(mems_manager_rc5_decode_fault_detail(p,n,1,&code,&historical,&known,&key));
    CHECK(code==0x250B&&historical&&known&&key==16841);
    CHECK(mems_manager_rc5_decode_fault_detail(p,n,2,&code,&historical,&known,&key));
    CHECK(code==0x1234&&!historical&&!known&&key==16880);
    CHECK(!mems_manager_rc5_decode_fault_detail(p,n,3,&code,&historical,&known,&key));
    CHECK(!mems_manager_rc5_clear_faults(s,false)&&g.clears==0);
    CHECK(mems_manager_rc5_clear_faults(s,true)&&g.clears==1);
    g.no_clear=true;CHECK(!mems_manager_rc5_clear_faults(s,true)&&g.clears==2);
    g.empty=true;CHECK(mems_manager_rc5_read_faults(s,p,sizeof(p),&n)&&n==3);
    CHECK(mems_rc5_fault_count(p,n)==0);
    g.bad_checksum=true;CHECK(!mems_manager_rc5_read_faults(s,p,sizeof(p),&n));
    g.bad_checksum=false;g.echo=false;
    CHECK(mems_manager_rc5_read_faults(s,p,sizeof(p),&n));
    CHECK(g.tx>0&&g.rx>0);g.cancel=true;
    CHECK(!mems_manager_rc5_read_faults(s,p,sizeof(p),&n));
    mems_manager_rc5_close_session(s);s=NULL;CHECK(g.closes==1);
    reset();g.no_wake=true;
    CHECK(mems_manager_rc5_open_transport("COM99",&s,NULL,NULL,NULL));
    CHECK(!mems_manager_rc5_wakeup(s)&&!mems_manager_rc5_ping(s)&&g.pings==0);
    mems_manager_rc5_close_session(s);s=NULL;
    reset();g.no_ping=true;
    CHECK(mems_manager_rc5_open_transport("COM99",&s,NULL,NULL,NULL));
    CHECK(mems_manager_rc5_wakeup(s)&&!mems_manager_rc5_ping(s));
    CHECK(!mems_manager_rc5_read_faults(s,p,sizeof(p),&n)&&g.reads==0);
    mems_manager_rc5_close_session(s);s=NULL;
    reset();g.fail_port=true;
    CHECK(!mems_manager_rc5_open_transport("COM99",&s,NULL,NULL,NULL)&&s==NULL);
    mems_manager_rc5_close_session(NULL);
    if(failures)return 1;
    puts("RC5_SESSION_SELFTEST_PASS: 2400 8N1, BREAK wake, ping, fragmented echo, DTC and explicit clear, cancellation");
    return 0;
}
