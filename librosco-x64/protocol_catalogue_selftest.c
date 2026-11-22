/*
 * Offline regression tests for the standalone Qt-free protocol codecs.
 * Zero serial I/O: no ECU or physical device is opened or modified.
 */
#include "protocol_catalogue.h"
#include <stdio.h>
#include <string.h>

static int failures;
#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "FAILED line %d: %s\n", __LINE__, #condition); \
    ++failures; \
} } while (0)
int main(void)
{
    uint8_t packet[128] = {0}, payload[128] = {0};
    size_t n = 0, decoded = 0;
    mems_codec_measurements values;
    bool active = false, historical = false;
    uint16_t code = 0;
    uint8_t state = 0;
    {
        const uint8_t read_rpm[] = {0x02, 0x21, 0x09, 0x2C};
        const uint8_t wake2j[] = {0x03, 0xC1, 0xD5, 0x8F, 0x28};
        const uint8_t positive[] = {0x61,0x09,0x0B,0xB8};
        const uint8_t pending[] = {0x7F,0x21,0x78};
        const uint8_t negative[] = {0x7F,0x21,0x31};
        const uint8_t wrong_id[] = {0x61,0x08,0x0B,0xB8};
        CHECK(mems2j_make_read_request(0x09, packet, sizeof(packet), &n));
        CHECK(n == sizeof(read_rpm) && memcmp(packet,read_rpm,n)==0);
        CHECK(mems2j_frame_decode(packet,n,payload,sizeof(payload),&decoded));
        CHECK(decoded==2 && payload[0]==0x21 && payload[1]==0x09);
        packet[3]^=0x01;
        CHECK(!mems2j_frame_decode(packet,n,payload,sizeof(payload),&decoded));
        packet[3]^=0x01;
        CHECK(!mems2j_frame_decode(packet,n-1,payload,sizeof(payload),&decoded));
        CHECK(mems2j_frame_decode(wake2j,sizeof(wake2j),
                                  payload,sizeof(payload),&decoded));
        CHECK(decoded==3 && payload[0]==0xC1 && payload[2]==0x8F);
        CHECK(!mems2j_make_read_request(0x09,packet,3,&n));
        CHECK(mems2j_make_clear_fault_request(packet,sizeof(packet),&n));
        CHECK(n==19 && packet[0]==17 && packet[1]==0x31 &&
              packet[2]==0xCB && packet[18]==(uint8_t)(17+0x31+0xCB));
        CHECK(mems2j_make_key_request(0x8247,packet,sizeof(packet),&n));
        CHECK(n==6 && packet[0]==4 && packet[1]==0x27 &&
              packet[2]==0x02 && packet[3]==0x82 && packet[4]==0x47);
        CHECK(mems2j_seed_key(0x1234)==0x8247);
        CHECK(mems2j_seed_key(0xABCD)==0x85AF);
        CHECK(mems2j_seed_key(0xFFFF)==0x8081);
        CHECK(mems2j_response_status(positive,sizeof(positive),
              0x21,0x61,0x09)==MEMS_RESPONSE_POSITIVE);
        CHECK(mems2j_response_status(pending,sizeof(pending),
              0x21,0x61,0x09)==MEMS_RESPONSE_PENDING);
        CHECK(mems2j_response_status(negative,sizeof(negative),
              0x21,0x61,0x09)==MEMS_RESPONSE_NEGATIVE);
        CHECK(mems2j_response_status(wrong_id,sizeof(wrong_id),
              0x21,0x61,0x09)==MEMS_RESPONSE_INVALID);
    }
    {
        const uint8_t rpm[] = {0x61,0x09,0x0B,0xB8};
        const uint8_t coolant[] = {0x61,0x01,0x0B,0x74};
        const uint8_t error[] = {0x61,0x21,0xFF,0xFF};
        const uint8_t injector[] = {0x61,0x0C,2,3,4,5};
        const uint8_t bits[] = {0x61,0x19,0x10,0x40};
        CHECK(mems2j_decode_measurement(0x09,rpm,sizeof(rpm),&values));
        CHECK(values.count==1 && values.metrics[0].field==MEMS_FIELD_RPM &&
              values.metrics[0].value==3000);
        CHECK(mems2j_decode_measurement(0x01,coolant,sizeof(coolant),&values));
        CHECK(values.count==1 && values.metrics[0].value==20);
        CHECK(mems2j_decode_measurement(0x21,error,sizeof(error),&values));
        CHECK(values.count==1 && values.metrics[0].value==-1);
        CHECK(mems2j_decode_measurement(0x0C,injector,sizeof(injector),&values));
        CHECK(values.count==4 && values.metrics[3].value==5);
        CHECK(!mems2j_decode_measurement(0x09,rpm,3,&values));
        CHECK(!mems2j_decode_measurement(0x09,coolant,4,&values));
        CHECK(!mems2j_decode_measurement(0x00,rpm,4,&values));
        CHECK(mems2j_fault_bit(bits,sizeof(bits),3,0x40,&active) && active);
        CHECK(mems2j_fault_bit(bits,sizeof(bits),2,0x80,&active) && !active);
        CHECK(!mems2j_fault_bit(bits,sizeof(bits),7,0x40,&active));
    }
    {
        const uint8_t wake_payload[] = {0x1A,0x9A};
        const uint8_t wake_frame[] = {0xB8,0x13,0xF7,0x02,0x1A,0x9A,0xDE};
        const uint8_t measures[] = {0x61,0x06,0x27,0x10,0,0,0,0,4,0xD2,0x0D,0xAC};
        const uint8_t faults[] = {0x58,0x00,0x12,0x32,0x61,0x06,0x50,0x20};
        const uint8_t pending3[] = {0x7F,0x21,0x78};
        CHECK(mems3_frame_encode(wake_payload,sizeof(wake_payload),
                                packet,sizeof(packet),&n));
        CHECK(n==sizeof(wake_frame) && memcmp(packet,wake_frame,n)==0);
        CHECK(mems3_frame_decode(packet,n,payload,sizeof(payload),&decoded));
        CHECK(decoded==2 && payload[0]==0x1A && payload[1]==0x9A);
        packet[n-1]^=0x80;
        CHECK(!mems3_frame_decode(packet,n,payload,sizeof(payload),&decoded));
        CHECK(mems3_make_read_request(0x06,packet,sizeof(packet),&n));
        CHECK(n==7 && packet[4]==0x21 && packet[5]==0x06);
        CHECK(mems3_decode_measurement(0x06,measures,sizeof(measures),&values));
        CHECK(values.count==3 && values.metrics[0].value==100 &&
              values.metrics[1].value==1234 &&
              values.metrics[2].value==3500);
        CHECK(!mems3_decode_measurement(0x06,measures,9,&values));
        CHECK(mems3_response_status(pending3,sizeof(pending3),
              0x21,0x61,0x06)==MEMS_RESPONSE_PENDING);
        CHECK(mems3_fault_count(faults,sizeof(faults))==2);
        CHECK(mems3_fault_at(faults,sizeof(faults),1,&code,&state));
        CHECK(code==0x0650 && state==0x20);
        CHECK(!mems3_fault_at(faults,sizeof(faults),2,&code,&state));
        CHECK(mems3_fault_count(faults,sizeof(faults)-1)==0);
    }
    {
        const uint8_t fault_present[] = {0xC4,0x33,0x15,0x0A,0xE9};
        const uint8_t fault_history[] = {0xC4,0x33,0x25,0x0A,0xD9};
        CHECK(mems_rc5_make_request(0x33,packet,sizeof(packet)));
        CHECK(packet[0]==0x82 && packet[1]==0x33 && packet[2]==0x4A);
        CHECK(mems_rc5_validate_frame(fault_present,sizeof(fault_present)));
        CHECK(mems_rc5_fault_count(fault_present,sizeof(fault_present))==1);
        CHECK(mems_rc5_fault_at(fault_present,sizeof(fault_present),0,&code,
                                &historical));
        CHECK(code==0x150A && !historical);
        CHECK(mems_rc5_fault_at(fault_history,sizeof(fault_history),0,&code,
                                &historical));
        CHECK(code==0x250A && historical);
        CHECK(!mems_rc5_fault_at(fault_present,sizeof(fault_present),1,&code,
                                 &historical));
        CHECK(!mems_rc5_validate_frame(fault_present,sizeof(fault_present)-1));
        CHECK(!mems_rc5_make_request(0x33,packet,2));
    }
    if (failures)
    {
        fprintf(stderr,"PROTOCOL_CATALOGUE_FAIL: %d failed checks\n",failures);
        return 1;
    }
    printf("PROTOCOL_CATALOGUE_PASS: 2J/3/RC5 frame, key, measurements, faults\n");
    return 0;
}
