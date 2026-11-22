#include "protocol_catalogue.h"
#include <string.h>

static uint16_t read_be16(const uint8_t* p)
{
    return (uint16_t)(((uint16_t)p[0] << 8) | p[1]);
}
static bool add_metric(mems_codec_measurements* output, uint16_t field, double value)
{
    if (output->count >= MEMS_CODEC_MAX_METRICS)
        return false;
    output->metrics[output->count].field = field;
    output->metrics[output->count].value = value;
    ++output->count;
    return true;
}
static mems_codec_response response_status(const uint8_t* p, size_t length,
    uint8_t requested, uint8_t positive, uint8_t id)
{
    if (!p || length == 0)
        return MEMS_RESPONSE_INVALID;
    if (length >= 2 && p[0] == 0x7F && p[1] == requested)
        return length == 3 ?
            (p[2] == 0x78 ? MEMS_RESPONSE_PENDING : MEMS_RESPONSE_NEGATIVE) :
            MEMS_RESPONSE_INVALID;
    if (p[0] != positive)
        return MEMS_RESPONSE_INVALID;
    if (positive == 0x61 && (length < 2 || p[1] != id))
        return MEMS_RESPONSE_INVALID;
    return MEMS_RESPONSE_POSITIVE;
}

/* MEMS 2J -- framed requests and measured replies */
bool mems2j_frame_encode(const uint8_t* payload, size_t length,
    uint8_t* output, size_t capacity, size_t* written)
{
    size_t i;
    uint32_t sum;
    if (written) *written = 0;
    if (!payload || !output || !written || length < 1 ||
        length > MEMS_CODEC_MAX_PAYLOAD || capacity < length + 2)
        return false;
    output[0] = (uint8_t)length;
    sum = (uint32_t)length;
    for (i = 0; i < length; ++i)
    {
        output[1+i] = payload[i];
        sum += payload[i];
    }
    output[length+1] = (uint8_t)sum;
    *written = length + 2;
    return true;
}
bool mems2j_frame_decode(const uint8_t* frame, size_t length,
    uint8_t* output, size_t capacity, size_t* written)
{
    size_t i, count;
    uint32_t sum = 0;
    if (written) *written = 0;
    if (!frame || !output || !written || length < 3)
        return false;
    count = frame[0];
    if (count < 1 || count > MEMS_CODEC_MAX_PAYLOAD ||
        length != count + 2 || capacity < count)
        return false;
    for (i = 0; i <= count; ++i)
        sum += frame[i];
    if ((uint8_t)sum != frame[count+1])
        return false;
    memcpy(output, frame + 1, count);
    *written = count;
    return true;
}
bool mems2j_make_read_request(uint8_t id,
    uint8_t* output, size_t capacity, size_t* written)
{
    uint8_t bytes[2] = {0x21, id};
    return mems2j_frame_encode(bytes, 2, output, capacity, written);
}
bool mems2j_make_clear_fault_request(
    uint8_t* output, size_t capacity, size_t* written)
{
    uint8_t bytes[17] = {0x31, 0xCB};
    /* The 15 trailing zero bytes are confirmed by the archived 2J code. */
    return mems2j_frame_encode(bytes, sizeof(bytes), output, capacity, written);
}
bool mems2j_make_key_request(uint16_t key,
    uint8_t* output, size_t capacity, size_t* written)
{
    uint8_t bytes[4] = {0x27, 0x02, (uint8_t)(key >> 8), (uint8_t)key};
    return mems2j_frame_encode(bytes, sizeof(bytes), output, capacity, written);
}
uint16_t mems2j_seed_key(uint16_t seed)
{
    unsigned int loops = 1 + ((seed & 0x8000u) ? 8 : 0) +
        ((seed & 0x0080u) ? 4 : 0) + ((seed & 0x0010u) ? 2 : 0) +
        ((seed & 0x0001u) ? 1 : 0);
    uint16_t key = 0;
    while (loops-- > 0)
    {
        key = (uint16_t)(seed >> 1);
        if ((seed & 0x2000u) && (seed & 0x0008u))
            key = (uint16_t)(key & 0xFFFEu);
        else
            key = (uint16_t)(key | 0x0001u);
        if (!!(seed & 0x0200u) ^ !!(seed & 0x0100u) ^
            !!(seed & 0x0004u) ^ !!(seed & 0x0002u))
            key = (uint16_t)(key | 0x8000u);
        seed = key;
    }
    return key;
}
mems_codec_response mems2j_response_status(
    const uint8_t* p, size_t length,
    uint8_t requested, uint8_t positive, uint8_t id)
{
    return response_status(p, length, requested, positive, id);
}
bool mems2j_decode_measurement(uint8_t id, const uint8_t* p, size_t n,
    mems_codec_measurements* output)
{
    if (!p || !output || n < 2 || p[0] != 0x61 || p[1] != id)
        return false;
    memset(output, 0, sizeof(*output));
    output->id = id;
    switch (id)
    {
    case 0x01: case 0x02: case 0x03:
        if (n < 4) return false;
        return add_metric(output, id == 1 ? MEMS_FIELD_COOLANT_C :
            id == 2 ? MEMS_FIELD_OIL_C : MEMS_FIELD_INTAKE_C,
            ((double)read_be16(p+2)-2732.0)/10.0);
    case 0x05:
        return n >= 4 && add_metric(output, MEMS_FIELD_FUEL_RAW, read_be16(p+2));
    case 0x07:
        return n >= 4 && add_metric(output, MEMS_FIELD_MAP_KPA, read_be16(p+2)/100.0);
    case 0x08:
        return n >= 4 && add_metric(output, MEMS_FIELD_THROTTLE_DEG, read_be16(p+2)/100.0);
    case 0x09:
        return n >= 4 && add_metric(output, MEMS_FIELD_RPM, read_be16(p+2));
    case 0x0A:
        return n >= 6 &&
            add_metric(output, MEMS_FIELD_LAMBDA_FEEDBACK_PERCENT, read_be16(p+2)/100.0) &&
            add_metric(output, MEMS_FIELD_LAMBDA_RAW, read_be16(p+4));
    case 0x0B:
        return n >= 4 &&
            add_metric(output, MEMS_FIELD_COIL_1_RAW, p[2]) &&
            add_metric(output, MEMS_FIELD_COIL_2_RAW, p[3]);
    case 0x0C:
        return n >= 6 &&
            add_metric(output, MEMS_FIELD_INJECTOR_1_RAW, p[2]) &&
            add_metric(output, MEMS_FIELD_INJECTOR_2_RAW, p[3]) &&
            add_metric(output, MEMS_FIELD_INJECTOR_3_RAW, p[4]) &&
            add_metric(output, MEMS_FIELD_INJECTOR_4_RAW, p[5]);
    case 0x0D:
        return n >= 3 && add_metric(output, MEMS_FIELD_SPEED_RAW, p[2]);
    case 0x0F:
        return n >= 3 &&
            add_metric(output, MEMS_FIELD_THROTTLE_CLOSED, (p[2]&1u) ? 1:0) &&
            add_metric(output, MEMS_FIELD_IGNITION_ON, (p[2]&2u) ? 1:0) &&
            add_metric(output, MEMS_FIELD_AC_REQUESTED, (p[2]&8u) ? 1:0);
    case 0x10:
        return n >= 6 && add_metric(output, MEMS_FIELD_BATTERY_V, read_be16(p+4)/1000.0);
    case 0x11:
        return n >= 3 &&
            add_metric(output, MEMS_FIELD_PRIMARY_SYNC, (p[2]&1u) ? 0:1) &&
            add_metric(output, MEMS_FIELD_SECONDARY_SYNC, (p[2]&2u) ? 0:1);
    case 0x12:
        return n >= 4 && add_metric(output, MEMS_FIELD_IDLE_STEPS, read_be16(p+2)/2.0);
    case 0x13:
        return n >= 3 && add_metric(output, MEMS_FIELD_CLOSED_LOOP, (p[2]&1u) ? 1:0);
    case 0x21:
        return n >= 4 && add_metric(output, MEMS_FIELD_RPM_ERROR_SIGNED,
            (int16_t)read_be16(p+2));
    case 0x25:
        return n >= 4 && add_metric(output, MEMS_FIELD_CAM_RAW, read_be16(p+2));
    case 0x3A:
        return n >= 6 &&
            add_metric(output, MEMS_FIELD_IDLE_OFFSET_RAW10, read_be16(p+2)/10.0) &&
            add_metric(output, MEMS_FIELD_IDLE_TARGET_RPM, read_be16(p+4));
    /* 0x00 and 0x06 have no established units; preserve their raw payload. */
    default:
        return false;
    }
}
bool mems2j_fault_bit(const uint8_t* payload,size_t length,
    size_t index,uint8_t mask,bool* active)
{
    if (!payload || !active || length < 2 ||
        payload[0] != 0x61 || payload[1] != 0x19 ||
        index < 2 || index >= length || mask == 0)
        return false;
    *active = (payload[index] & mask) != 0;
    return true;
}

/* MEMS3 -- separate 8E1 XOR protocol, never ROSCO / 2J. */
bool mems3_frame_encode(const uint8_t* payload,size_t length,
    uint8_t* output,size_t capacity,size_t* written)
{
    size_t i;
    uint8_t checksum = 0;
    if (written) *written = 0;
    if (!payload || !output || !written || length < 1 ||
        length > MEMS_CODEC_MAX_PAYLOAD || capacity < length + 5)
        return false;
    output[0] = 0xB8; output[1] = 0x13; output[2] = 0xF7;
    output[3] = (uint8_t)length;
    memcpy(output + 4, payload, length);
    for (i = 0; i < length + 4; ++i)
        checksum ^= output[i];
    output[4+length] = checksum;
    *written = length + 5;
    return true;
}
bool mems3_frame_decode(const uint8_t* frame,size_t length,
    uint8_t* output,size_t capacity,size_t* written)
{
    size_t i,count;
    uint8_t checksum = 0;
    if (written) *written = 0;
    if (!frame || !output || !written || length < 6)
        return false;
    count = frame[3];
    if (count < 1 || count > MEMS_CODEC_MAX_PAYLOAD ||
        count + 5 != length || capacity < count)
        return false;
    for (i = 0; i < length; ++i)
        checksum ^= frame[i];
    if (checksum != 0)
        return false;
    memcpy(output, frame + 4, count);
    *written = count;
    return true;
}
bool mems3_make_read_request(uint8_t id,uint8_t* output,size_t capacity,size_t* written)
{
    uint8_t bytes[2] = {0x21, id};
    return mems3_frame_encode(bytes, 2, output, capacity, written);
}
mems_codec_response mems3_response_status(const uint8_t* p,size_t length,
    uint8_t requested,uint8_t positive,uint8_t id)
{
    return response_status(p, length, requested, positive, id);
}
bool mems3_decode_measurement(uint8_t id,const uint8_t* p,size_t n,
    mems_codec_measurements* output)
{
    if (!p || !output || n < 2 || p[0] != 0x61 || p[1] != id)
        return false;
    memset(output,0,sizeof(*output));
    output->id=id;
    switch (id)
    {
    case 0x00:
        return n >= 12 &&
            add_metric(output, MEMS_FIELD_COOLANT_C, ((double)read_be16(p+2)-2730.0)/10.0) &&
            add_metric(output, MEMS_FIELD_OIL_C, ((double)read_be16(p+6)-2730.0)/10.0) &&
            add_metric(output, MEMS_FIELD_INTAKE_C, ((double)read_be16(p+10)-2730.0)/10.0);
    case 0x06:
        return n >= 12 &&
            add_metric(output, MEMS_FIELD_MAP_KPA, read_be16(p+2)/100.0) &&
            add_metric(output, MEMS_FIELD_THROTTLE_MV, read_be16(p+8)) &&
            add_metric(output, MEMS_FIELD_RPM, read_be16(p+10));
    case 0x0A:
        return n >= 6 &&
            add_metric(output, MEMS_FIELD_LAMBDA_FEEDBACK_PERCENT, read_be16(p+2)/100.0) &&
            add_metric(output, MEMS_FIELD_LAMBDA_RAW, read_be16(p+4));
    case 0x0B:
        return n >= 6 &&
            add_metric(output, MEMS_FIELD_COIL_1_US, read_be16(p+2)) &&
            add_metric(output, MEMS_FIELD_COIL_2_US, read_be16(p+4));
    case 0x21:
        return n >= 4 && add_metric(output, MEMS_FIELD_RPM_DEVIATION_RAW, read_be16(p+2));
    default:
        return false;
    }
}
size_t mems3_fault_count(const uint8_t* payload,size_t length)
{
    if (!payload || length < 2 || payload[0] != 0x58 ||
        (length-2)%3 != 0)
        return 0;
    return (length-2)/3;
}
bool mems3_fault_at(const uint8_t* payload,size_t length,
    size_t index,uint16_t* code,uint8_t* state)
{
    size_t count=mems3_fault_count(payload,length);
    if (!code || !state || index >= count)
        return false;
    *code=read_be16(payload+2+index*3);
    *state=payload[4+index*3];
    return true;
}

/* RC5 -- airbag SRS 2400 8N1, not an engine MEMS protocol */
bool mems_rc5_make_request(uint8_t command,uint8_t* output,size_t capacity)
{
    if (!output || capacity < 3)
        return false;
    output[0]=0x82;
    output[1]=command;
    output[2]=(uint8_t)(0xFFu-0x82u-command);
    return true;
}
bool mems_rc5_validate_frame(const uint8_t* packet,size_t length)
{
    size_t i;
    uint32_t sum=0;
    size_t expected;
    if(!packet || length<3 || length>64 ||
        ((packet[0]&0xC0u)!=0x80u && (packet[0]&0xC0u)!=0xC0u))
        return false;
    expected=(size_t)(packet[0]&0x3Fu)+1;
    if (expected!=length)
        return false;
    for(i=0;i<length;++i)
        sum+=packet[i];
    return ((uint8_t)sum)==0xFFu;
}
size_t mems_rc5_fault_count(const uint8_t* packet,size_t length)
{
    if (!mems_rc5_validate_frame(packet,length) ||
        (packet[0]&0xC0u)!=0xC0u || packet[1]!=0x33 ||
        (length-3)%2!=0)
        return 0;
    return (length-3)/2;
}
bool mems_rc5_fault_at(const uint8_t* packet,size_t length,
    size_t index,uint16_t* code,bool* historical)
{
    size_t count=mems_rc5_fault_count(packet,length);
    if (!code || !historical || index>=count)
        return false;
    *code=read_be16(packet+2+2*index);
    *historical=(*code>=0x2500u && *code<=0x26FFu);
    return true;
}
