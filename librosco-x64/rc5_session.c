/* Native RC5 diagnostics, separate from all MEMS engine sessions.
 * Behavioral source: immutable Session 205 rc5protocol.h. Reuses RC5 codecs.
 * No Qt, deployment tests, coding or crash-data reset.
 */
#include "rc5_session.h"
#include <stdlib.h>
#include <string.h>
struct mems_manager_rc5_session {
    HANDLE port;
    bool awake,ready;
    mems_manager_rc5_cancel_fn cancelled;
    mems_manager_rc5_trace_fn trace;
    void *user;
    uint8_t received[256];
    size_t received_count;
};
static bool port_ok(HANDLE port)
{
    return port != NULL && port != INVALID_HANDLE_VALUE;
}
static bool is_cancelled(const mems_manager_rc5_session *s)
{
    return s && s->cancelled && s->cancelled(s->user);
}
static uint32_t elapsed_ms(ULONGLONG started)
{
    ULONGLONG elapsed = GetTickCount64() - started;
    return elapsed > UINT32_MAX ? UINT32_MAX : (uint32_t)elapsed;
}
static bool delay_cancelable(mems_manager_rc5_session *s, DWORD ms)
{
    const ULONGLONG begin = GetTickCount64();
    while (GetTickCount64() - begin < ms) {
        ULONGLONG left = ms - (GetTickCount64() - begin);
        if (is_cancelled(s)) return false;
        Sleep((DWORD)(left < 20u ? left : 20u));
    }
    return !is_cancelled(s);
}
static void report_io(mems_manager_rc5_session *s, int direction,
                      const uint8_t *bytes, size_t size)
{
    if (s && s->trace && bytes && size)
        s->trace(s->user, direction, (uint64_t)GetTickCount64(), bytes, size);
}
static void consume(mems_manager_rc5_session *s, size_t n)
{
    if (n >= s->received_count) {
        s->received_count = 0;
        return;
    }
    memmove(s->received, s->received + n, s->received_count - n);
    s->received_count -= n;
}

static bool configure_port(HANDLE port)
{
    DCB dcb;
    COMMTIMEOUTS timeout;
    if (!port_ok(port)) return false;
    memset(&dcb, 0, sizeof(dcb));
    dcb.DCBlength = sizeof(dcb);
    if (!GetCommState(port, &dcb) || !GetCommTimeouts(port, &timeout))
        return false;
    dcb.BaudRate = 2400;
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    dcb.fBinary = TRUE;
    dcb.fParity = FALSE;
    dcb.fOutxCtsFlow = FALSE;
    dcb.fOutxDsrFlow = FALSE;
    dcb.fDtrControl = DTR_CONTROL_DISABLE;
    dcb.fRtsControl = RTS_CONTROL_DISABLE;
    dcb.fOutX = FALSE;
    dcb.fInX = FALSE;
    timeout.ReadIntervalTimeout = 20;
    timeout.ReadTotalTimeoutMultiplier = 0;
    timeout.ReadTotalTimeoutConstant = 40;
    timeout.WriteTotalTimeoutMultiplier = 0;
    timeout.WriteTotalTimeoutConstant = 400;
    return SetCommState(port, &dcb) && SetCommTimeouts(port, &timeout);
}

static bool read_more(mems_manager_rc5_session *s)
{
    DWORD got=0;
    if (s->received_count>=sizeof(s->received) || is_cancelled(s))return false;
    if (!ReadFile(s->port,s->received+s->received_count,
                  (DWORD)(sizeof(s->received)-s->received_count),&got,NULL) ||
        got>sizeof(s->received)-s->received_count) {
        s->awake=s->ready=false;
        return false;
    }
    if (got) {
        report_io(s,2,s->received+s->received_count,got);
        s->received_count+=got;
    } else if (!delay_cancelable(s,10))return false;
    return true;
}
static bool valid_fault_packet(const uint8_t *p,size_t n)
{
    return mems_rc5_validate_frame(p,n) && (p[0]&0xC0u)==0xC0u &&
        p[1]==0x33 && (n-3)%2==0;
}
static bool exchange(mems_manager_rc5_session *s,uint8_t command,
    const uint8_t *expected,bool faults,DWORD timeout,
    uint8_t *packet,size_t capacity,size_t *length)
{
    uint8_t sent[3]; DWORD written=0; ULONGLONG start;
    if (!s || !port_ok(s->port) || is_cancelled(s) ||
        !mems_rc5_make_request(command,sent,sizeof(sent)))return false;
    /* Discard stale input before each explicit request. */
    s->received_count=0;
    if (!PurgeComm(s->port,PURGE_RXCLEAR))return false;
    if (!WriteFile(s->port,sent,sizeof(sent),&written,NULL) || written!=sizeof(sent) ||
        !FlushFileBuffers(s->port)) {s->awake=s->ready=false;return false;}
    report_io(s,1,sent,sizeof(sent));
    start=GetTickCount64();
    while (elapsed_ms(start)<timeout && !is_cancelled(s)) {
        while(s->received_count) {
            size_t n=s->received_count<sizeof(sent)?s->received_count:sizeof(sent);
            if (memcmp(s->received,sent,n)==0) {
                if(s->received_count<sizeof(sent))break;
                consume(s,sizeof(sent));continue;
            }
            n=(size_t)(s->received[0]&0x3Fu)+1;
            if (((s->received[0]&0xC0u)!=0x80u && (s->received[0]&0xC0u)!=0xC0u)||n<3||n>64) {
                consume(s,1);continue;
            }
            if(s->received_count<n)break;
            if(!mems_rc5_validate_frame(s->received,n)){consume(s,1);continue;}
            if ((faults && valid_fault_packet(s->received,n)) ||
                (!faults && expected && n==3 && memcmp(s->received,expected,3)==0)) {
                if(packet && length && capacity>=n) {
                    memcpy(packet,s->received,n);*length=n;
                } else if(faults) {consume(s,n);return false;}
                consume(s,n);return true;
            }
            consume(s,n);
        }
        if(!read_more(s))return false;
    }
    return false;
}
bool mems_manager_rc5_open_transport(const char *device_path,
    mems_manager_rc5_session **out_session,mems_manager_rc5_cancel_fn cancel,
    mems_manager_rc5_trace_fn trace,void *user)
{
    mems_manager_rc5_session *s;
    if(!out_session)return false;
    *out_session=NULL;
    if(!device_path || !device_path[0])return false;
    s=(mems_manager_rc5_session*)calloc(1,sizeof(*s));
    if(!s)return false;
    s->port=INVALID_HANDLE_VALUE;s->cancelled=cancel;s->trace=trace;s->user=user;
    if(is_cancelled(s))goto fail;
    s->port=CreateFileA(device_path,GENERIC_READ|GENERIC_WRITE,0,NULL,
                        OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if(!port_ok(s->port)||!configure_port(s->port)||
       !PurgeComm(s->port,PURGE_RXCLEAR|PURGE_TXCLEAR))goto fail;
    *out_session=s;return true;
fail:
    mems_manager_rc5_close_session(s);return false;
}
bool mems_manager_rc5_wakeup(mems_manager_rc5_session *s)
{
    static const bool levels[]={false,true,false,true,false,true,false};
    static const DWORD times[]={2000,200,400,400,400,400,200};
    static const uint8_t wake[]={0x55,0x06,0x3B};
    size_t i; ULONGLONG start;
    if(!s||!port_ok(s->port))return false;
    s->awake=s->ready=false;s->received_count=0;
    if(is_cancelled(s)||!PurgeComm(s->port,PURGE_RXCLEAR|PURGE_TXCLEAR))goto fail;
    for(i=0;i<sizeof(levels)/sizeof(levels[0]);++i) {
        if(is_cancelled(s)||!(levels[i]?SetCommBreak(s->port):ClearCommBreak(s->port))||
           !delay_cancelable(s,times[i]))goto fail;
    }
    start=GetTickCount64();
    while(elapsed_ms(start)<1100 && !is_cancelled(s)) {
        for(i=0;i+sizeof(wake)<=s->received_count;++i) {
            if(memcmp(s->received+i,wake,sizeof(wake))==0) {
                consume(s,i+sizeof(wake));
                if(!delay_cancelable(s,700))goto fail;
                s->awake=true;return true;
            }
        }
        if(s->received_count>96)consume(s,s->received_count-96);
        if(!read_more(s))goto fail;
    }
fail:
    ClearCommBreak(s->port);return false;
}
bool mems_manager_rc5_ping(mems_manager_rc5_session *s)
{
    const uint8_t expected[]={0xC2,0x00,0x3D};
    if(!s||!s->awake)return false;
    s->ready=exchange(s,0x00,expected,false,1100,NULL,0,NULL);
    return s->ready;
}
bool mems_manager_rc5_read_faults(mems_manager_rc5_session *s,
    uint8_t *packet,size_t capacity,size_t *length)
{
    if(!s||!s->ready||!packet||!length||capacity<3)return false;
    return exchange(s,0x33,NULL,true,1300,packet,capacity,length);
}
bool mems_manager_rc5_decode_fault_detail(const uint8_t *packet,
    size_t length,size_t index,uint16_t *code,bool *historical,bool *known,int *key)
{
    static const uint16_t ids[]={0x150A,0x150B,0x150C,0x150D,0x150E,
        0x1512,0x1513,0x1514,0x1515,0x1516,0x151A,0x151B,0x151C,0x151D,0x151E,
        0x1524,0x1525,0x1526,0x152C,0x152D,0x152E,0x160C,0x160D,0x160E};
    uint16_t id,normal;bool hist;size_t i;
    if(!code||!historical||!known||!key||
       !mems_rc5_fault_at(packet,length,index,&id,&hist))return false;
    normal=hist?(uint16_t)(id-0x1000u):id;
    *code=id;*historical=hist;*known=false;*key=16880;
    for(i=0;i<sizeof(ids)/sizeof(ids[0]);++i)
        if(normal==ids[i]){*known=true;*key=16840+(int)i;break;}
    return true;
}
bool mems_manager_rc5_clear_faults(mems_manager_rc5_session *s,bool explicitly_confirmed)
{
    const uint8_t expected[]={0xC2,0xC3,0x7A};
    if(!s||!s->ready||!explicitly_confirmed)return false;
    /* Only standard diagnostic DTC clear, never crash-data programming. */
    return exchange(s,0xC3,expected,false,1300,NULL,0,NULL);
}
void mems_manager_rc5_close_session(mems_manager_rc5_session *s)
{
    if(!s)return;
    s->awake=s->ready=false;
    if(port_ok(s->port)){ClearCommBreak(s->port);CloseHandle(s->port);}
    free(s);
}
