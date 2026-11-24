/* Native MEMS 3 session, ported from immutable Session 205 mems3protocol.h.
 * 9600 8E1, framed wake, XOR checksum. No Qt or Manager changes.
 * Existing MEMS 2J transport is not used; only the proven shared seed/key
 * algorithm and MEMS 3-specific codecs in protocol_catalogue are reused.
 */
#include "mems3_session.h"
#include <stdlib.h>
#include <string.h>

#define M3_RX_MAX 512u
#define M3_WIRE_MAX (MEMS_CODEC_MAX_PAYLOAD + 5u)

struct mems_manager_mems3_session {
    HANDLE port;
    bool ready;
    mems_manager_mems3_cancel_fn cancelled;
    mems_manager_mems3_trace_fn trace;
    void *user;
    uint8_t received[M3_RX_MAX];
    size_t received_count;
    uint8_t sent[M3_WIRE_MAX];
    size_t sent_count;
    mems_manager_mems3_result last_result;
    uint8_t last_negative;
    uint32_t last_duration;
};

static const uint8_t g_ids[]={0xff,0x00,0x06,0x0a,0x0b,0x21};

static bool port_ok(HANDLE port)
{
    return port != NULL && port != INVALID_HANDLE_VALUE;
}
static bool is_cancelled(const mems_manager_mems3_session *s)
{
    return s && s->cancelled && s->cancelled(s->user);
}
static void set_result(mems_manager_mems3_session *s,
                       mems_manager_mems3_result status,
                       uint8_t negative, uint32_t duration)
{
    if (s) {
        s->last_result = status;
        s->last_negative = negative;
        s->last_duration = duration;
    }
}
static uint32_t elapsed_ms(ULONGLONG started)
{
    ULONGLONG elapsed = GetTickCount64() - started;
    return elapsed > UINT32_MAX ? UINT32_MAX : (uint32_t)elapsed;
}
static bool delay_cancelable(mems_manager_mems3_session *s, DWORD ms)
{
    const ULONGLONG begin = GetTickCount64();
    while (GetTickCount64() - begin < ms) {
        ULONGLONG left = ms - (GetTickCount64() - begin);
        if (is_cancelled(s)) return false;
        Sleep((DWORD)(left < 20u ? left : 20u));
    }
    return !is_cancelled(s);
}
static void report_io(mems_manager_mems3_session *s, int direction,
                      const uint8_t *bytes, size_t size)
{
    if (s && s->trace && bytes && size)
        s->trace(s->user, direction, (uint64_t)GetTickCount64(), bytes, size);
}
static void consume(mems_manager_mems3_session *s, size_t n)
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
    dcb.BaudRate = 9600;
    dcb.ByteSize = 8;
    dcb.Parity = EVENPARITY;
    dcb.StopBits = ONESTOPBIT;
    dcb.fBinary = TRUE;
    dcb.fParity = TRUE;
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
static bool transmit(mems_manager_mems3_session *s,
                     const uint8_t *bytes, size_t size)
{
    DWORD written = 0;
    if (!s || !port_ok(s->port) || !bytes ||
        size == 0 || size > sizeof(s->sent) || is_cancelled(s)) {
        set_result(s, is_cancelled(s) ? MEMS_MANAGER_3_CANCELLED :
                   MEMS_MANAGER_3_INVALID_ARGUMENT,0,0);
        return false;
    }
    /* Record first: the response may include this command as USB echo. */
    memcpy(s->sent, bytes, size);
    s->sent_count = size;
    if (!WriteFile(s->port, bytes, (DWORD)size, &written, NULL) ||
        written != size || !FlushFileBuffers(s->port)) {
        set_result(s, MEMS_MANAGER_3_WRITE_FAILURE,0,0);
        return false;
    }
    report_io(s,1,bytes,size);
    return true;
}

/* Handle fragmented local echo, MEMS 3 XOR frames and pending replies. */
static bool receive_expected(mems_manager_mems3_session *s,
                             const uint8_t *positive_prefix,
                             size_t prefix_count, uint8_t requested_service,
                             DWORD timeout_ms, uint8_t *payload,
                             size_t payload_capacity, size_t *payload_size)
{
    ULONGLONG started;
    uint8_t decoded[MEMS_CODEC_MAX_PAYLOAD];
    size_t decoded_n = 0;
    bool received_negative = false;
    uint8_t negative_code = 0;
    if (!s || !positive_prefix || prefix_count == 0 ||
        !payload || !payload_size)
        return false;
    started = GetTickCount64();
    set_result(s,MEMS_MANAGER_3_TIMEOUT,0,0);
    while (elapsed_ms(started) < timeout_ms) {
        DWORD got = 0;
        if (is_cancelled(s)) {
            set_result(s,MEMS_MANAGER_3_CANCELLED,0,elapsed_ms(started));
            return false;
        }
        while (s->received_count) {
            /* Incomplete echo prefix: do not consume as a fake frame. */
            size_t n = s->received_count < s->sent_count
                ? s->received_count : s->sent_count;
            if (n && memcmp(s->received,s->sent,n) == 0) {
                if (s->received_count < s->sent_count)
                    break;
                consume(s,s->sent_count);
                continue;
            }
            /* Some adapters echo only the raw wake payload 1A 9A. */
            if (requested_service==0x1A && s->received[0]==0x1A) {
                if (s->received_count<2) break;
                if (s->received[1]==0x9A) { consume(s,2); continue; }
            }
            if (s->received_count<5) break;
            if (s->received[3] < 1 ||
                s->received[3] > MEMS_CODEC_MAX_PAYLOAD) {
                consume(s,1);
                continue;
            }
            {
                const size_t wire_len=(size_t)s->received[3]+5u;
                if (s->received_count < wire_len)
                    break; /* A genuine length-framed reply may be split. */
                if (!mems3_frame_decode(s->received,wire_len,decoded,
                                         sizeof(decoded),&decoded_n)) {
                    consume(s,1); /* checksum failure: resynchronise */
                    continue;
                }
                consume(s,wire_len);
                if (decoded_n >= prefix_count &&
                    memcmp(decoded,positive_prefix,prefix_count)==0) {
                    if (decoded_n > payload_capacity) {
                        set_result(s,MEMS_MANAGER_3_PROTOCOL_FAILURE,0,
                                   elapsed_ms(started));
                        return false;
                    }
                    memcpy(payload,decoded,decoded_n);
                    *payload_size=decoded_n;
                    set_result(s,MEMS_MANAGER_3_OK,0,elapsed_ms(started));
                    return true;
                }
                if (decoded_n >= 3 && decoded[0] == 0x7F &&
                    decoded[1] == requested_service) {
                    if (decoded[2] == 0x78)
                        continue;
                    negative_code=decoded[2];
                    received_negative=true;
                    break;
                }
                /* Ignore stale reply from a previous request, do not
                 * mislabel another service's reply as this one's. */
            }
        }
        if (received_negative) {
            set_result(s,MEMS_MANAGER_3_NEGATIVE_RESPONSE,negative_code,
                       elapsed_ms(started));
            return false;
        }
        if (s->received_count >= sizeof(s->received)) {
            set_result(s,MEMS_MANAGER_3_PROTOCOL_FAILURE,0,
                       elapsed_ms(started));
            return false;
        }
        /* Read only a limited chunk; with COMMTIMEOUTS it returns within
         * 40 ms when no incoming bytes arrive. Do not spin at 100% CPU. */
        if (!ReadFile(s->port,s->received+s->received_count,
                      (DWORD)(sizeof(s->received)-s->received_count),
                      &got,NULL)) {
            set_result(s,MEMS_MANAGER_3_PORT_FAILURE,0,elapsed_ms(started));
            return false;
        }
        if (got > sizeof(s->received)-s->received_count) {
            set_result(s,MEMS_MANAGER_3_PROTOCOL_FAILURE,0,elapsed_ms(started));
            return false;
        }
        if (got) {
            report_io(s,2,s->received+s->received_count,got);
            s->received_count+=(size_t)got;
        } else if (!delay_cancelable(s,10u)) {
            set_result(s,MEMS_MANAGER_3_CANCELLED,0,elapsed_ms(started));
            return false;
        }
    }
    set_result(s,MEMS_MANAGER_3_TIMEOUT,0,elapsed_ms(started));
    return false;
}
static bool exchange_frame(mems_manager_mems3_session *s,
                           const uint8_t *frame, size_t frame_n,
                           const uint8_t *positive, size_t positive_n,
                           uint8_t service, DWORD timeout_ms,
                           uint8_t *payload, size_t capacity,size_t *got)
{
    if (!transmit(s,frame,frame_n))
        return false;
    return receive_expected(s,positive,positive_n,service,
                            timeout_ms,payload,capacity,got);
}
static bool exchange(mems_manager_mems3_session *s,
                     const uint8_t *service_payload,size_t payload_n,
                     const uint8_t *positive,size_t positive_n,
                     DWORD timeout_ms,uint8_t *response,
                     size_t capacity,size_t *got)
{
    uint8_t wire[M3_WIRE_MAX];
    size_t n=0;
    if (!mems3_frame_encode(service_payload,payload_n,wire,sizeof(wire),&n)) {
        set_result(s,MEMS_MANAGER_3_INVALID_ARGUMENT,0,0);
        return false;
    }
    return exchange_frame(s,wire,n,positive,positive_n,service_payload[0],
                          timeout_ms,response,capacity,got);
}
bool mems_manager_mems3_connect(
    const char *device_path, mems_manager_mems3_session **out_session,
    mems_manager_mems3_cancel_fn cancel,
    mems_manager_mems3_trace_fn trace, void *user,
    mems_manager_mems3_result *failure_reason)
{
    mems_manager_mems3_session *s;
    uint8_t response[MEMS_CODEC_MAX_PAYLOAD];
    uint8_t key_request[4] = {0x27,0x02,0,0};
    size_t n=0;
    const uint8_t start_service[]={0x10,0xA0};
    const uint8_t seed_service[]={0x27,0x01};
    const uint8_t live_service[]={0x3E};
    const uint8_t wake[]={0x1A,0x9A};
    const uint8_t positive_init[]={0x5A,0x9A};
    const uint8_t positive_start[]={0x50};
    const uint8_t positive_seed[]={0x67,0x01};
    const uint8_t positive_key[]={0x67,0x02};
    const uint8_t positive_live[]={0x7E};
    uint16_t key;
    if (failure_reason) *failure_reason = MEMS_MANAGER_3_INVALID_ARGUMENT;
    if (!out_session || !device_path || !device_path[0])
        return false;
    *out_session = NULL;
    s=(mems_manager_mems3_session*)calloc(1,sizeof(*s));
    if (!s) {
        if (failure_reason) *failure_reason = MEMS_MANAGER_3_PORT_FAILURE;
        return false;
    }
    s->port=INVALID_HANDLE_VALUE;
    s->cancelled=cancel;
    s->trace=trace;
    s->user=user;
    set_result(s,MEMS_MANAGER_3_NOT_CONNECTED,0,0);
    if (is_cancelled(s)) {
        set_result(s,MEMS_MANAGER_3_CANCELLED,0,0);
        goto fail;
    }
    s->port=CreateFileA(device_path,GENERIC_READ|GENERIC_WRITE,0,NULL,
                        OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if (!port_ok(s->port) || !configure_port(s->port) ||
        !PurgeComm(s->port,PURGE_RXCLEAR|PURGE_TXCLEAR) ||
        !ClearCommBreak(s->port)) {
        set_result(s,MEMS_MANAGER_3_PORT_FAILURE,0,0);
        goto fail;
    }
    if (!exchange(s,wake,sizeof(wake),positive_init,sizeof(positive_init),
                  1100,response,sizeof(response),&n))
        goto fail;
    if (!exchange(s,start_service,sizeof(start_service),positive_start,
                  sizeof(positive_start),900,response,sizeof(response),&n))
        goto fail;
    if (!exchange(s,seed_service,sizeof(seed_service),positive_seed,
                  sizeof(positive_seed),900,response,sizeof(response),&n) ||
        n < 4u) {
        if (s->last_result == MEMS_MANAGER_3_OK)
            set_result(s,MEMS_MANAGER_3_PROTOCOL_FAILURE,0,s->last_duration);
        goto fail;
    }
    key=(uint16_t)(((uint16_t)response[2]<<8)|response[3]);
    if (key != 0) {
        key=mems2j_seed_key(key);
        key_request[2]=(uint8_t)(key>>8);
        key_request[3]=(uint8_t)key;
        if (!exchange(s,key_request,sizeof(key_request),positive_key,
                      sizeof(positive_key),900,response,sizeof(response),&n))
            goto fail;
    }
    (void)exchange(s,live_service,sizeof(live_service),positive_live,
                   sizeof(positive_live),350,response,sizeof(response),&n);
    if (is_cancelled(s) || s->last_result==MEMS_MANAGER_3_PORT_FAILURE ||
        s->last_result==MEMS_MANAGER_3_WRITE_FAILURE) {
        if (is_cancelled(s)) set_result(s,MEMS_MANAGER_3_CANCELLED,0,s->last_duration);
        goto fail;
    }
    s->ready=true;
    *out_session=s;
    if (failure_reason) *failure_reason=MEMS_MANAGER_3_OK;
    return true;
fail:
    if (failure_reason) *failure_reason=s->last_result;
    mems_manager_mems3_close(s);
    return false;
}

bool mems_manager_mems3_is_open(const mems_manager_mems3_session *s)
{
    return s && s->ready && port_ok(s->port);
}
bool mems_manager_mems3_last_transaction(
    const mems_manager_mems3_session *s,
    mems_manager_mems3_result *result,
    uint8_t *negative_response_code, uint32_t *duration_ms)
{
    if (!s)
        return false;
    if (result) *result=s->last_result;
    if (negative_response_code) *negative_response_code=s->last_negative;
    if (duration_ms) *duration_ms=s->last_duration;
    return true;
}
static bool read_id(mems_manager_mems3_session *s, uint8_t id,
                    uint8_t *raw,size_t capacity,size_t *written,
                    DWORD timeout)
{
    uint8_t request[2]={0x21,id};
    uint8_t positive[2]={0x61,id};
    size_t n=0;
    uint8_t response[MEMS_CODEC_MAX_PAYLOAD];
    if (!s || !mems_manager_mems3_is_open(s) ||
        !raw || !written || capacity==0) {
        set_result(s,MEMS_MANAGER_3_NOT_CONNECTED,0,0);
        return false;
    }
    if (!exchange(s,request,sizeof(request),positive,sizeof(positive),
                  timeout,response,sizeof(response),&n))
        return false;
    if (capacity<n) {
        set_result(s,MEMS_MANAGER_3_INVALID_ARGUMENT,0,s->last_duration);
        return false;
    }
    memcpy(raw,response,n);
    *written=n;
    return true;
}
bool mems_manager_mems3_read_measurement(
    mems_manager_mems3_session *s,uint8_t id,
    uint8_t *raw,size_t capacity,size_t *written,
    mems_codec_measurements *measurements)
{
    mems_codec_measurements decoded;
    size_t n=0;
    uint8_t response[MEMS_CODEC_MAX_PAYLOAD];
    if (!raw || !written || capacity<2u) {
        set_result(s,MEMS_MANAGER_3_INVALID_ARGUMENT,0,0);
        return false; /* Invalid output cannot trigger a wire transaction. */
    }
    if (!read_id(s,id,response,sizeof(response),&n,450u))
        return false;
    if (capacity<n) {
        set_result(s,MEMS_MANAGER_3_INVALID_ARGUMENT,0,s->last_duration);
        return false;
    }
    /* Preserve raw replies even if a conversion is not established. */
    memset(&decoded,0,sizeof(decoded));
    decoded.id=id;
    if (measurements &&
        !mems3_decode_measurement(id,response,n,&decoded)) {
        memset(&decoded,0,sizeof(decoded));
        decoded.id=id;
    }
    memcpy(raw,response,n);
    *written=n;
    if (measurements) *measurements=decoded;
    return true;
}
bool mems_manager_mems3_read_faults(
    mems_manager_mems3_session *s,uint8_t *raw,
    size_t capacity,size_t *written)
{
    const uint8_t cmd[]={0x18,0,0,0}, positive[]={0x58};
    if (!mems_manager_mems3_is_open(s)) {
        set_result(s,MEMS_MANAGER_3_NOT_CONNECTED,0,0);
        return false;
    }
    if (!raw || !written || capacity<2) {
        set_result(s,MEMS_MANAGER_3_INVALID_ARGUMENT,0,0);
        return false;
    }
    return exchange(s,cmd,sizeof(cmd),positive,sizeof(positive),450u,raw,capacity,written);
}
bool mems_manager_mems3_keepalive(mems_manager_mems3_session *s)
{
    const uint8_t cmd[]={0x3E};
    const uint8_t positive[]={0x7E};
    uint8_t received[MEMS_CODEC_MAX_PAYLOAD];
    size_t n=0;
    if (!mems_manager_mems3_is_open(s)) {
        set_result(s,MEMS_MANAGER_3_NOT_CONNECTED,0,0);
        return false;
    }
    return exchange(s,cmd,sizeof(cmd),positive,sizeof(positive),350u,
                    received,sizeof(received),&n);
}
bool mems_manager_mems3_clear_faults(mems_manager_mems3_session *s)
{
    const uint8_t cmd[]={0x14,0,0}, positive[]={0x54,0,0};
    uint8_t response[MEMS_CODEC_MAX_PAYLOAD];
    size_t n=0;
    if (!mems_manager_mems3_is_open(s)) {
        set_result(s,MEMS_MANAGER_3_NOT_CONNECTED,0,0);
        return false;
    }
    return exchange(s,cmd,sizeof(cmd),positive,sizeof(positive),1000u,response,sizeof(response),&n);
}
bool mems_manager_mems3_read_cycle(
    mems_manager_mems3_session *s,
    mems_manager_mems3_cycle_fn on_result,void *user,
    unsigned int *success_count)
{
    unsigned int good=0;
    size_t i;
    if (success_count) *success_count=0;
    if (!mems_manager_mems3_is_open(s)) {
        set_result(s,MEMS_MANAGER_3_NOT_CONNECTED,0,0);
        return false;
    }
    for (i=0;i<sizeof(g_ids);++i) {
        uint8_t raw[MEMS_CODEC_MAX_PAYLOAD];
        size_t n=0;
        bool ok;
        if (is_cancelled(s)) {
            set_result(s,MEMS_MANAGER_3_CANCELLED,0,0);
            break;
        }
        ok=g_ids[i]==0xff ? mems_manager_mems3_read_faults(s,raw,sizeof(raw),&n)
                          : read_id(s,g_ids[i],raw,sizeof(raw),&n,450u);
        if (ok) ++good;
        if (on_result)
            on_result(user,g_ids[i],ok,ok?raw:NULL,ok?n:0);
        if (is_cancelled(s)) {
            set_result(s,MEMS_MANAGER_3_CANCELLED,0,0);
            break;
        }
    }
    if (success_count) *success_count=good;
    return good>0 && !is_cancelled(s);
}
void mems_manager_mems3_close(mems_manager_mems3_session *s)
{
    if (!s) return;
    s->ready=false;
    if (port_ok(s->port)) {
        ClearCommBreak(s->port);
        CloseHandle(s->port);
        s->port=INVALID_HANDLE_VALUE;
    }
    free(s);
}
