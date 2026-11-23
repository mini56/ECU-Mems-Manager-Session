/* Full physical Session205 5AS algorithm exercised with completely FAKE
 * Win32 serial and FAKE clock. No COM ports or real ECU ever opened.
 * Exactly the production lucas5as_session.c is compiled via inclusion.
 */
#include "lucas5as.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

typedef struct {
    ULONGLONG tick;
    uint8_t in[512];
    size_t in_size;
    size_t in_cursor;
    uint8_t output[64];
    size_t output_size;
    bool wake_delivered;
    bool no_wake;
    bool bad_wake;
    bool no_status;
    bool truncated_status;
    bool fail_write;
    int opens;
    int closes;
    int breaks;
    int clear_breaks;
    int status_reads;
} fake_5as;
static fake_5as g;
static int failures;
#define CHECK(x) do { if (!(x)) { \
    fprintf(stderr,"LUCAS5AS_SERIAL_SIM_FAIL line %d: %s\n", __LINE__,#x); \
    failures++; \
} } while(0)

static void reset_fake(void)
{
    memset(&g,0,sizeof(g));
}
static bool queue_in(const uint8_t *bytes, size_t n)
{
    if (g.in_size + n > sizeof(g.in))return false;
    memcpy(g.in + g.in_size,bytes,n);
    g.in_size += n;
    return true;
}
static void status_reply(void)
{
    uint8_t frame[49]={0};
    uint8_t prefix[]={0x00,0x83};
    frame[0]=(uint8_t)sizeof(frame);
    frame[1]=0x80;
    frame[6]=0x80;
    frame[9]=0;
    frame[11]=0x80;
    frame[16]=1;frame[17]=3;frame[18]=5;frame[19]=7;
    frame[20]=0x12;frame[21]=0x34;frame[22]=0x56;
    memcpy(frame+35,"AP3.005AS-98MY",14);
    if (!queue_in(prefix,sizeof(prefix))) failures++;
    if (!queue_in(frame,g.truncated_status ? 20 : sizeof(frame))) failures++;
}
static HANDLE WINAPI MockCreateFileA(LPCSTR name,DWORD access,DWORD share,
                                      LPSECURITY_ATTRIBUTES sec,DWORD creation,
                                      DWORD attributes,HANDLE other)
{
    (void)access;(void)share;(void)sec;(void)creation;(void)attributes;(void)other;
    g.opens++;
    if (!name || strcmp(name,"INVALID")==0)return INVALID_HANDLE_VALUE;
    return (HANDLE)(uintptr_t)0x5A50;
}
static BOOL WINAPI MockCloseHandle(HANDLE h)
{
    (void)h;
    g.closes++;
    return TRUE;
}
static BOOL WINAPI MockGetCommState(HANDLE h,LPDCB dcb)
{
    (void)h;
    return dcb && dcb->DCBlength==sizeof(DCB);
}
static BOOL WINAPI MockSetCommState(HANDLE h,LPDCB dcb)
{
    (void)h;
    return dcb && dcb->BaudRate==9600 && dcb->ByteSize==8 &&
           dcb->Parity==NOPARITY && dcb->StopBits==ONESTOPBIT;
}
static BOOL WINAPI MockGetCommTimeouts(HANDLE h,LPCOMMTIMEOUTS t)
{
    (void)h;
    if (!t)return FALSE;
    memset(t,0,sizeof(*t));
    return TRUE;
}
static BOOL WINAPI MockSetCommTimeouts(HANDLE h,LPCOMMTIMEOUTS t)
{
    (void)h;
    return t && t->ReadTotalTimeoutConstant==40 &&
           t->WriteTotalTimeoutConstant==500;
}
static BOOL WINAPI MockPurgeComm(HANDLE h,DWORD flags)
{
    (void)h;
    if(flags & PURGE_RXCLEAR) g.in_size=g.in_cursor=0;
    return TRUE;
}
static BOOL WINAPI MockSetCommBreak(HANDLE h)
{
    (void)h;
    g.breaks++;
    return TRUE;
}
static BOOL WINAPI MockClearCommBreak(HANDLE h)
{
    (void)h;
    g.clear_breaks++;
    return TRUE;
}
static ULONGLONG WINAPI MockGetTickCount64(void) {return g.tick;}
static VOID WINAPI MockSleep(DWORD millis) {g.tick+=(millis?millis:1);}
static BOOL WINAPI MockReadFile(HANDLE h,LPVOID dest,DWORD count,LPDWORD got,
                                LPOVERLAPPED overlap)
{
    size_t avail,n;
    (void)h;(void)overlap;
    if(!dest||!got)return FALSE;
    /* Wake arrives only after complete 2-second slow init + 300ms
     * initial delay. Bad wake and silence are tested separately. */
    if (!g.wake_delivered && !g.no_wake && g.tick>=2300 &&
        g.in_cursor==g.in_size) {
        const uint8_t wake_good[] = {0x00,0x1C,0x55,0x83,0x76};
        const uint8_t wake_bad[] = {0x00,0x1C,0x55,0x83,0xF7};
        const uint8_t *w=g.bad_wake?wake_bad:wake_good;
        g.wake_delivered=true;
        if(!queue_in(w,5))return FALSE;
    }
    avail=g.in_size-g.in_cursor;
    n=(size_t)count<avail?(size_t)count:avail;
    if(n>5)n=5; /* Fragmentation is required for this test. */
    if(n)memcpy(dest,g.in+g.in_cursor,n);
    g.in_cursor+=n;
    *got=(DWORD)n;
    return TRUE;
}
static BOOL WINAPI MockWriteFile(HANDLE h,LPCVOID src,DWORD count,LPDWORD sent,
                                 LPOVERLAPPED overlap)
{
    const uint8_t* bytes=(const uint8_t*)src;
    (void)h;(void)overlap;
    if(!src||!sent||g.fail_write||g.output_size+count>sizeof(g.output))return FALSE;
    memcpy(g.output+g.output_size,bytes,count);
    g.output_size+=count;
    *sent=count;
    if(count==1 && bytes[0]==0x83) {
        g.status_reads++;
        if(!g.no_status)status_reply();
    }
    return TRUE;
}

/* Keep Win32 call interception completely inside this fake test executable. */
#define CreateFileA MockCreateFileA
#define CloseHandle MockCloseHandle
#define GetCommState MockGetCommState
#define SetCommState MockSetCommState
#define GetCommTimeouts MockGetCommTimeouts
#define SetCommTimeouts MockSetCommTimeouts
#define PurgeComm MockPurgeComm
#define SetCommBreak MockSetCommBreak
#define ClearCommBreak MockClearCommBreak
#define GetTickCount64 MockGetTickCount64
#define Sleep MockSleep
#define ReadFile MockReadFile
#define WriteFile MockWriteFile
#include "lucas5as_session.c"
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
#undef ReadFile
#undef WriteFile

int main(void)
{
    mems_manager_lucas5as_session *session=NULL;
    uint8_t packet[128]={0};
    size_t got=0;
    mems_manager_lucas5as_status status;

    reset_fake();
    CHECK(mems_manager_lucas5as_open_normal("COM99",&session));
    CHECK(session!=NULL);
    CHECK(g.opens==1 && g.closes==0);
    CHECK(g.tick>=2300 && g.breaks>0 && g.clear_breaks>0);
    CHECK(g.status_reads==1 && g.output[0]==0x83);
    CHECK(mems_manager_lucas5as_initial_status(session,packet,sizeof(packet),&got));
    CHECK(got==49 && packet[0]==49 && packet[1]==0x80);
    CHECK(mems_manager_lucas5as_decode_status(packet,got,&status));
    CHECK(status.description_length==14);
    CHECK(!mems_manager_lucas5as_initial_status(session,packet,30,&got));

    /* Request a second live read through the SAME fake COM session. */
    g.in_size=g.in_cursor=0;
    CHECK(mems_manager_lucas5as_read_status(session,packet,sizeof(packet),&got));
    CHECK(got==49 && g.status_reads==2);
    CHECK(g.output_size==2 && g.output[1]==0x83);

    g.no_status=true;
    got=867;
    CHECK(!mems_manager_lucas5as_read_status(session,packet,sizeof(packet),&got));
    CHECK(got==867); /* no false success on timeout */
    g.no_status=false;
    g.truncated_status=true;
    CHECK(!mems_manager_lucas5as_read_status(session,packet,sizeof(packet),&got));
    CHECK(got==867);
    mems_manager_lucas5as_close(session);
    session=NULL;
    CHECK(g.closes==1);

    reset_fake();
    g.bad_wake=true;
    CHECK(!mems_manager_lucas5as_open_normal("COM99",&session));
    CHECK(session==NULL && g.opens==1 && g.closes==1);
    CHECK(g.status_reads==0); /* no 83 to wrong 55 83 F7 */

    reset_fake();
    g.no_wake=true;
    CHECK(!mems_manager_lucas5as_open_normal("COM99",&session));
    CHECK(session==NULL && g.status_reads==0 && g.closes==1);

    reset_fake();
    g.no_status=true;
    CHECK(!mems_manager_lucas5as_open_normal("COM99",&session));
    CHECK(session==NULL && g.status_reads==1 && g.closes==1);

    reset_fake();
    g.fail_write=true;
    CHECK(!mems_manager_lucas5as_open_normal("COM99",&session));
    CHECK(session==NULL && g.closes==1);

    reset_fake();
    CHECK(!mems_manager_lucas5as_open_normal("INVALID",&session));
    CHECK(session==NULL && g.opens==1);
    CHECK(!mems_manager_lucas5as_open_normal(NULL,&session));
    CHECK(!mems_manager_lucas5as_open_normal("COM99",NULL));

    if(failures){
        fprintf(stderr,"LUCAS5AS_SESSION_SELFTEST_FAIL %d\n",failures);
        return 1;
    }
    puts("LUCAS5AS_SESSION_SELFTEST_PASS: fake Win32 5baud/9600 handshake, real status reader, timeout/invalids");
    return 0;
}
