/*
 * Qt-free native 2J diagnostic Session. Behavioral source:
 * Session 205 mems2jprotocol.h lines 206-405.
 *
 * This file is NOT called by the existing Manager 205. No need to modify
 * Manager, its Connect button, or its source to build/test this new DLL.
 * Existing ROSCO and legacy 2J codec implementations are left unchanged.
 *
 * Protocol:
 * 10400 8N1 K-Line, 200ms idle, 25ms BREAK, 75ms settle,
 * 81 13 F7 81 0C unframed fast wake; expected C1 D5 8F;
 * 10 A0 / 50; 27 01 / 67 01 seed; 27 02 key / 67 02;
 * optional 3E / 7E, 21 09 / 61 09.
 * Read 21 XX / 61 XX. Explicit 31 CB + 15x00 / 71 CB clears faults.
 *
 * All writes and reads are synchronous, on the calling thread.
 * No persistent file I/O, Qt, COM auto-discovery, programming or UI.
 */
#include "mems2j_session.h"
#include <stdlib.h>
#include <string.h>

#define M2J_RX_MAX 512u
#define M2J_WIRE_MAX (MEMS_CODEC_MAX_PAYLOAD + 2u)

struct mems_manager_mems2j_session {
    HANDLE port;
    bool ready;
    mems_manager_mems2j_cancel_fn cancelled;
    mems_manager_mems2j_trace_fn trace;
    void *user;
    uint8_t received[M2J_RX_MAX];
    size_t received_count;
    uint8_t sent[M2J_WIRE_MAX];
    size_t sent_count;
    unsigned int primary_position;
    unsigned int extra_position;
    mems_manager_mems2j_result last_result;
    uint8_t last_negative;
    uint32_t last_duration;
};

static const uint8_t g_initial[] = {0x81,0x13,0xF7,0x81,0x0C};
static const uint8_t g_primary[] =
    {0x01,0x03,0x07,0x08,0x0A,0x0F,0x10,0x12,0x13};
static const uint8_t g_extra[] =
    {0x00,0x02,0x05,0x06,0x0B,0x0C,0x0D,0x11,0x21,0x25,0x3A};

static bool port_ok(HANDLE port)
{
    return port != NULL && port != INVALID_HANDLE_VALUE;
}
static bool is_cancelled(const mems_manager_mems2j_session *s)
{
    return s && s->cancelled && s->cancelled(s->user);
}
static void set_result(mems_manager_mems2j_session *s,
                       mems_manager_mems2j_result status,
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
static bool delay_cancelable(mems_manager_mems2j_session *s, DWORD ms)
{
    const ULONGLONG begin = GetTickCount64();
    while (GetTickCount64() - begin < ms) {
        ULONGLONG left = ms - (GetTickCount64() - begin);
        if (is_cancelled(s)) return false;
        Sleep((DWORD)(left < 20u ? left : 20u));
    }
    return !is_cancelled(s);
}
static void report_io(mems_manager_mems2j_session *s, int direction,
                      const uint8_t *bytes, size_t size)
{
    if (s && s->trace && bytes && size)
        s->trace(s->user, direction, (uint64_t)GetTickCount64(), bytes, size);
}
static void consume(mems_manager_mems2j_session *s, size_t n)
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
    dcb.BaudRate = 10400;
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
static bool transmit(mems_manager_mems2j_session *s,
                     const uint8_t *bytes, size_t size)
{
    DWORD written = 0;
    if (!s || !port_ok(s->port) || !bytes ||
        size == 0 || size > sizeof(s->sent) || is_cancelled(s)) {
        set_result(s, is_cancelled(s) ? MEMS_MANAGER_2J_CANCELLED :
                   MEMS_MANAGER_2J_INVALID_ARGUMENT,0,0);
        return false;
    }
    /* Record first: the response may include this command as USB echo. */
    memcpy(s->sent, bytes, size);
    s->sent_count = size;
    if (!WriteFile(s->port, bytes, (DWORD)size, &written, NULL) ||
        written != size || !FlushFileBuffers(s->port)) {
        set_result(s, MEMS_MANAGER_2J_WRITE_FAILURE,0,0);
        return false;
    }
    report_io(s,1,bytes,size);
    return true;
}

/* The 2J wire may echo an unframed five-byte wake or a KWP packet,
 * possibly fragmented across several USB reads. Preserve partial echo
 * until enough bytes arrive to decide. Decode only length-checked frames.
 * Negative 7F SERVICE 78 (response pending) is not a failure.
 */
static bool receive_expected(mems_manager_mems2j_session *s,
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
    set_result(s,MEMS_MANAGER_2J_TIMEOUT,0,0);
    while (elapsed_ms(started) < timeout_ms) {
        DWORD got = 0;
        if (is_cancelled(s)) {
            set_result(s,MEMS_MANAGER_2J_CANCELLED,0,elapsed_ms(started));
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
            if (s->received[0] < 1 ||
                s->received[0] > MEMS_CODEC_MAX_PAYLOAD) {
                consume(s,1);
                continue;
            }
            {
                const size_t wire_len=(size_t)s->received[0]+2u;
                if (s->received_count < wire_len)
                    break; /* A genuine length-framed reply may be split. */
                if (!mems2j_frame_decode(s->received,wire_len,decoded,
                                         sizeof(decoded),&decoded_n)) {
                    consume(s,1); /* checksum failure: resynchronise */
                    continue;
                }
                consume(s,wire_len);
                if (decoded_n >= prefix_count &&
                    memcmp(decoded,positive_prefix,prefix_count)==0) {
                    if (decoded_n > payload_capacity) {
                        set_result(s,MEMS_MANAGER_2J_PROTOCOL_FAILURE,0,
                                   elapsed_ms(started));
                        return false;
                    }
                    memcpy(payload,decoded,decoded_n);
                    *payload_size=decoded_n;
                    set_result(s,MEMS_MANAGER_2J_OK,0,elapsed_ms(started));
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
            set_result(s,MEMS_MANAGER_2J_NEGATIVE_RESPONSE,negative_code,
                       elapsed_ms(started));
            return false;
        }
        if (s->received_count >= sizeof(s->received)) {
            set_result(s,MEMS_MANAGER_2J_PROTOCOL_FAILURE,0,
                       elapsed_ms(started));
            return false;
        }
        /* Read only a limited chunk; with COMMTIMEOUTS it returns within
         * 40 ms when no incoming bytes arrive. Do not spin at 100% CPU. */
        if (!ReadFile(s->port,s->received+s->received_count,
                      (DWORD)(sizeof(s->received)-s->received_count),
                      &got,NULL)) {
            set_result(s,MEMS_MANAGER_2J_PORT_FAILURE,0,elapsed_ms(started));
            return false;
        }
        if (got > sizeof(s->received)-s->received_count) {
            set_result(s,MEMS_MANAGER_2J_PROTOCOL_FAILURE,0,elapsed_ms(started));
            return false;
        }
        if (got) {
            report_io(s,2,s->received+s->received_count,got);
            s->received_count+=(size_t)got;
        } else if (!delay_cancelable(s,10u)) {
            set_result(s,MEMS_MANAGER_2J_CANCELLED,0,elapsed_ms(started));
            return false;
        }
    }
    set_result(s,MEMS_MANAGER_2J_TIMEOUT,0,elapsed_ms(started));
    return false;
}
static bool exchange_frame(mems_manager_mems2j_session *s,
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
static bool exchange(mems_manager_mems2j_session *s,
                     const uint8_t *service_payload,size_t payload_n,
                     const uint8_t *positive,size_t positive_n,
                     DWORD timeout_ms,uint8_t *response,
                     size_t capacity,size_t *got)
{
    uint8_t wire[M2J_WIRE_MAX];
    size_t n=0;
    if (!mems2j_frame_encode(service_payload,payload_n,wire,sizeof(wire),&n)) {
        set_result(s,MEMS_MANAGER_2J_INVALID_ARGUMENT,0,0);
        return false;
    }
    return exchange_frame(s,wire,n,positive,positive_n,service_payload[0],
                          timeout_ms,response,capacity,got);
}
bool mems_manager_mems2j_connect(
    const char *device_path, mems_manager_mems2j_session **out_session,
    mems_manager_mems2j_cancel_fn cancel,
    mems_manager_mems2j_trace_fn trace, void *user,
    mems_manager_mems2j_result *failure_reason)
{
    mems_manager_mems2j_session *s;
    uint8_t response[MEMS_CODEC_MAX_PAYLOAD];
    uint8_t key_request[4] = {0x27,0x02,0,0};
    size_t n=0;
    const uint8_t start_service[]={0x10,0xA0};
    const uint8_t seed_service[]={0x27,0x01};
    const uint8_t live_service[]={0x3E};
    const uint8_t rpm_service[]={0x21,0x09};
    const uint8_t positive_init[]={0xC1,0xD5,0x8F};
    const uint8_t positive_start[]={0x50};
    const uint8_t positive_seed[]={0x67,0x01};
    const uint8_t positive_key[]={0x67,0x02};
    const uint8_t positive_live[]={0x7E};
    const uint8_t positive_rpm[]={0x61,0x09};
    uint16_t key;
    if (failure_reason) *failure_reason = MEMS_MANAGER_2J_INVALID_ARGUMENT;
    if (!out_session || !device_path || !device_path[0])
        return false;
    *out_session = NULL;
    s=(mems_manager_mems2j_session*)calloc(1,sizeof(*s));
    if (!s) {
        if (failure_reason) *failure_reason = MEMS_MANAGER_2J_PORT_FAILURE;
        return false;
    }
    s->port=INVALID_HANDLE_VALUE;
    s->cancelled=cancel;
    s->trace=trace;
    s->user=user;
    set_result(s,MEMS_MANAGER_2J_NOT_CONNECTED,0,0);
    if (is_cancelled(s)) {
        set_result(s,MEMS_MANAGER_2J_CANCELLED,0,0);
        goto fail;
    }
    s->port=CreateFileA(device_path,GENERIC_READ|GENERIC_WRITE,0,NULL,
                        OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if (!port_ok(s->port) || !configure_port(s->port) ||
        !PurgeComm(s->port,PURGE_RXCLEAR|PURGE_TXCLEAR) ||
        !ClearCommBreak(s->port)) {
        set_result(s,MEMS_MANAGER_2J_PORT_FAILURE,0,0);
        goto fail;
    }
    if (!delay_cancelable(s,200u)) {
        set_result(s,MEMS_MANAGER_2J_CANCELLED,0,0);
        goto fail;
    }
    if (!SetCommBreak(s->port)) {
        set_result(s,MEMS_MANAGER_2J_WAKE_FAILURE,0,0);
        goto fail;
    }
    if (!delay_cancelable(s,25u)) {
        set_result(s,MEMS_MANAGER_2J_CANCELLED,0,0);
        goto fail;
    }
    if (!ClearCommBreak(s->port)) {
        set_result(s,MEMS_MANAGER_2J_WAKE_FAILURE,0,0);
        goto fail;
    }
    if (!delay_cancelable(s,75u)) {
        set_result(s,MEMS_MANAGER_2J_CANCELLED,0,0);
        goto fail;
    }
    /* Init is intentionally UNFRAMED, unlike every subsequent command. */
    if (!exchange_frame(s,g_initial,sizeof(g_initial),positive_init,
                        sizeof(positive_init),0x81u,1200,
                        response,sizeof(response),&n))
        goto fail;
    if (!exchange(s,start_service,sizeof(start_service),positive_start,
                  sizeof(positive_start),900,response,sizeof(response),&n))
        goto fail;
    if (!exchange(s,seed_service,sizeof(seed_service),positive_seed,
                  sizeof(positive_seed),900,response,sizeof(response),&n) ||
        n < 4u) {
        if (s->last_result == MEMS_MANAGER_2J_OK)
            set_result(s,MEMS_MANAGER_2J_PROTOCOL_FAILURE,0,s->last_duration);
        goto fail;
    }
    key=mems2j_seed_key((uint16_t)(((uint16_t)response[2]<<8)|response[3]));
    key_request[2]=(uint8_t)(key>>8);
    key_request[3]=(uint8_t)key;
    if (!exchange(s,key_request,sizeof(key_request),positive_key,
                  sizeof(positive_key),900,response,sizeof(response),&n))
        goto fail;

    /* The ECU is authenticated. Keepalive and one RPM sensor are only
     * probes; a 2J variant may reject an individual measurement.
     * Such refusal MUST NOT invalidate a real established session. */
    (void)exchange(s,live_service,sizeof(live_service),positive_live,
                   sizeof(positive_live),350,response,sizeof(response),&n);
    if (is_cancelled(s)) {
        set_result(s,MEMS_MANAGER_2J_CANCELLED,0,s->last_duration);
        goto fail;
    }
    (void)exchange(s,rpm_service,sizeof(rpm_service),positive_rpm,
                   sizeof(positive_rpm),900,response,sizeof(response),&n);
    if (is_cancelled(s)) {
        set_result(s,MEMS_MANAGER_2J_CANCELLED,0,s->last_duration);
        goto fail;
    }
    s->ready=true;
    *out_session=s;
    if (failure_reason) *failure_reason=MEMS_MANAGER_2J_OK;
    return true;
fail:
    if (failure_reason) *failure_reason=s->last_result;
    mems_manager_mems2j_close(s);
    return false;
}

bool mems_manager_mems2j_is_open(const mems_manager_mems2j_session *s)
{
    return s && s->ready && port_ok(s->port);
}
bool mems_manager_mems2j_last_transaction(
    const mems_manager_mems2j_session *s,
    mems_manager_mems2j_result *result,
    uint8_t *negative_response_code, uint32_t *duration_ms)
{
    if (!s)
        return false;
    if (result) *result=s->last_result;
    if (negative_response_code) *negative_response_code=s->last_negative;
    if (duration_ms) *duration_ms=s->last_duration;
    return true;
}
static bool read_id(mems_manager_mems2j_session *s, uint8_t id,
                    uint8_t *raw,size_t capacity,size_t *written,
                    DWORD timeout)
{
    uint8_t request[2]={0x21,id};
    uint8_t positive[2]={0x61,id};
    size_t n=0;
    uint8_t response[MEMS_CODEC_MAX_PAYLOAD];
    if (!s || !mems_manager_mems2j_is_open(s) ||
        !raw || !written || capacity==0) {
        set_result(s,MEMS_MANAGER_2J_NOT_CONNECTED,0,0);
        return false;
    }
    if (!exchange(s,request,sizeof(request),positive,sizeof(positive),
                  timeout,response,sizeof(response),&n))
        return false;
    if (capacity<n) {
        set_result(s,MEMS_MANAGER_2J_INVALID_ARGUMENT,0,s->last_duration);
        return false;
    }
    memcpy(raw,response,n);
    *written=n;
    return true;
}
bool mems_manager_mems2j_read_measurement(
    mems_manager_mems2j_session *s,uint8_t id,
    uint8_t *raw,size_t capacity,size_t *written,
    mems_codec_measurements *measurements)
{
    mems_codec_measurements decoded;
    size_t n=0;
    uint8_t response[MEMS_CODEC_MAX_PAYLOAD];
    if (!raw || !written || capacity<2u) {
        set_result(s,MEMS_MANAGER_2J_INVALID_ARGUMENT,0,0);
        return false; /* Invalid output cannot trigger a wire transaction. */
    }
    if (!read_id(s,id,response,sizeof(response),&n,400u))
        return false;
    if (capacity<n) {
        set_result(s,MEMS_MANAGER_2J_INVALID_ARGUMENT,0,s->last_duration);
        return false;
    }
    /* An 0x00/0x06/unknown reply is still valid *raw* ECU data. Never
     * invent a conversion just to return a value. */
    memset(&decoded,0,sizeof(decoded));
    decoded.id=id;
    if (measurements &&
        !mems2j_decode_measurement(id,response,n,&decoded)) {
        memset(&decoded,0,sizeof(decoded));
        decoded.id=id;
    }
    memcpy(raw,response,n);
    *written=n;
    if (measurements) *measurements=decoded;
    return true;
}
bool mems_manager_mems2j_read_faults(
    mems_manager_mems2j_session *s,uint8_t *raw,
    size_t capacity,size_t *written)
{
    return read_id(s,0x19u,raw,capacity,written,400u);
}
bool mems_manager_mems2j_keepalive(mems_manager_mems2j_session *s)
{
    const uint8_t cmd[]={0x3E};
    const uint8_t positive[]={0x7E};
    uint8_t received[MEMS_CODEC_MAX_PAYLOAD];
    size_t n=0;
    if (!mems_manager_mems2j_is_open(s)) {
        set_result(s,MEMS_MANAGER_2J_NOT_CONNECTED,0,0);
        return false;
    }
    return exchange(s,cmd,sizeof(cmd),positive,sizeof(positive),350u,
                    received,sizeof(received),&n);
}
bool mems_manager_mems2j_clear_faults(mems_manager_mems2j_session *s)
{
    uint8_t frame[M2J_WIRE_MAX];
    uint8_t response[MEMS_CODEC_MAX_PAYLOAD];
    size_t frame_n=0;
    size_t received_n=0;
    const uint8_t positive[]={0x71,0xCB};
    if (!mems_manager_mems2j_is_open(s)) {
        set_result(s,MEMS_MANAGER_2J_NOT_CONNECTED,0,0);
        return false;
    }
    if (!mems2j_make_clear_fault_request(frame,sizeof(frame),&frame_n)) {
        set_result(s,MEMS_MANAGER_2J_PROTOCOL_FAILURE,0,0);
        return false;
    }
    return exchange_frame(s,frame,frame_n,positive,sizeof(positive),
                          0x31u,1000u,response,sizeof(response),&received_n);
}
bool mems_manager_mems2j_read_cycle(
    mems_manager_mems2j_session *s,
    mems_manager_mems2j_cycle_fn on_result,void *user,
    unsigned int *success_count)
{
    uint8_t ids[7];
    unsigned int good=0;
    size_t i;
    if (success_count) *success_count=0;
    if (!mems_manager_mems2j_is_open(s)) {
        set_result(s,MEMS_MANAGER_2J_NOT_CONNECTED,0,0);
        return false;
    }
    ids[0]=0x09u;
    ids[1]=0x19u;
    for (i=0;i<4;++i)
        ids[i+2]=g_primary[(s->primary_position++) %
                           (sizeof(g_primary)/sizeof(g_primary[0]))];
    ids[6]=g_extra[(s->extra_position++) %
                    (sizeof(g_extra)/sizeof(g_extra[0]))];
    for (i=0;i<7;++i) {
        uint8_t raw[MEMS_CODEC_MAX_PAYLOAD];
        size_t n=0;
        bool ok=read_id(s,ids[i],raw,sizeof(raw),&n,400u);
        if (ok) ++good;
        if (on_result)
            on_result(user,ids[i],ok,ok?raw:NULL,ok?n:0);
        if (is_cancelled(s)) {
            set_result(s,MEMS_MANAGER_2J_CANCELLED,0,0);
            break;
        }
    }
    if (success_count) *success_count=good;
    return good>0 && !is_cancelled(s);
}
void mems_manager_mems2j_close(mems_manager_mems2j_session *s)
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
