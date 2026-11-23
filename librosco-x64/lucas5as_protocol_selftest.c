/* Qt-free, serial-free validation of the independent 5AS C ABI.
 * Mirrors the source's Session 205 lucas5asprotocol_test.cpp vectors.
 * No ECU or port is accessed.
 */
#include "lucas5as.h"
#include <stdio.h>
#include <string.h>

static int failures;
#define CHECK(x) do { if (!(x)) { \
    fprintf(stderr, "5AS CODEC FAIL line %d: %s\n", __LINE__, #x); \
    ++failures; \
} } while(0)

int main(void)
{
    const bool slow[10] = {
        false, false, false, true, true, true, false, false, false, true
    };
    const uint8_t wake[] = { 0x00,0x1C,0x55,0x83,0x76 };
    const uint8_t wrong_wake[] = { 0x55,0x83,0xF7 };
    uint8_t original[49] = {0};
    uint8_t shortened[19] = {0};
    uint8_t output[128] = {0};
    uint8_t start = 0, stop = 0;
    size_t produced = 999, offset = 999;
    mems_manager_lucas5as_status status;
    mems_manager_lucas5as_status prev;
    int i;
    bool unused = false;

    for (i=0;i<10;++i) {
        bool level = !slow[i];
        CHECK(mems_manager_lucas5as_slow_bit_high(i,&level));
        CHECK(level==slow[i]);
    }
    CHECK(!mems_manager_lucas5as_slow_bit_high(-1,&unused));
    CHECK(!mems_manager_lucas5as_slow_bit_high(10,(bool*)&slow[0]));
    CHECK(!mems_manager_lucas5as_slow_bit_high(0,NULL));

    CHECK(mems_manager_lucas5as_has_normal_wake(wake,sizeof(wake)));
    CHECK(!mems_manager_lucas5as_has_normal_wake(wake,4));
    CHECK(!mems_manager_lucas5as_has_normal_wake(wrong_wake,sizeof(wrong_wake)));
    CHECK(!mems_manager_lucas5as_has_normal_wake(NULL,3));

    for (i=0;i<=15;++i) {
        const bool known = (i==8 || i==9 || i==13);
        if (known) {
            CHECK(mems_manager_lucas5as_output_test_commands(i,&start,&stop));
            CHECK(start==(uint8_t)(0x30|i));
            CHECK(stop==(uint8_t)(0x20|i));
        } else CHECK(!mems_manager_lucas5as_output_test_commands(i,&start,&stop));
    }
    CHECK(!mems_manager_lucas5as_output_test_commands(8,NULL,&stop));
    CHECK(!mems_manager_lucas5as_output_test_commands(8,&start,NULL));

    /* Synthetic only, NOT captured from a real 5AS. */
    original[0]=49;
    original[1]=0x80;
    original[6]=0x80;
    original[9]=0x00;
    original[11]=0x80;
    original[16]=1;original[17]=3;original[18]=5;original[19]=7;
    original[20]=0x12;original[21]=0x34;original[22]=0x56;
    memcpy(original+35,"AP3.005AS-98MY",14);
    memset(&status,0x5A,sizeof(status));
    CHECK(mems_manager_lucas5as_decode_status(original,sizeof(original),&status));
    CHECK(status.packet_length==49);
    CHECK(status.available_fields ==
        (MEMS_MANAGER_LUCAS5AS_FIELD_BONNET |
         MEMS_MANAGER_LUCAS5AS_FIELD_IGNITION |
         MEMS_MANAGER_LUCAS5AS_FIELD_DOORS |
         MEMS_MANAGER_LUCAS5AS_FIELD_POSSIBLE_BOOT |
         MEMS_MANAGER_LUCAS5AS_FIELD_FOBS |
         MEMS_MANAGER_LUCAS5AS_FIELD_SERIAL |
         MEMS_MANAGER_LUCAS5AS_FIELD_DESCRIPTION));
    CHECK(status.switch_bit7[0]==0x80 && status.switch_bit7[1]==0x80);
    CHECK(status.switch_bit7[2]==0x00 && status.switch_bit7[3]==0x80);
    CHECK(status.fob_counters_raw[0]==1 && status.fob_counters_raw[3]==7);
    CHECK(status.serial_little_endian[0]==0x12 &&
          status.serial_little_endian[2]==0x56);
    CHECK(status.description_length==14);
    CHECK(strcmp(status.unit_description,"AP3.005AS-98MY")==0);

    memcpy(shortened,original,sizeof(shortened));
    shortened[0]=(uint8_t)sizeof(shortened);
    CHECK(mems_manager_lucas5as_decode_status(shortened,sizeof(shortened),&status));
    CHECK((status.available_fields & MEMS_MANAGER_LUCAS5AS_FIELD_FOBS)==0);
    CHECK((status.available_fields & MEMS_MANAGER_LUCAS5AS_FIELD_SERIAL)==0);
    CHECK((status.available_fields & MEMS_MANAGER_LUCAS5AS_FIELD_DESCRIPTION)==0);
    CHECK(status.unit_description[0]=='\0');
    prev=status;
    CHECK(!mems_manager_lucas5as_decode_status(original,18,&status));
    CHECK(!mems_manager_lucas5as_decode_status(NULL,49,&status));
    CHECK(!mems_manager_lucas5as_decode_status(original,49,NULL));
    CHECK(memcmp(&status,&prev,sizeof(status))==0); /* no partial publish */
    original[0]=48;
    CHECK(!mems_manager_lucas5as_decode_status(original,49,&status));
    original[0]=49;

    /* Valid 00 / 83 echo prefix and additional trailing command. */
    {
        uint8_t received[52] = {0,0x83};
        received[51]=0xF6;
        memcpy(received+2,original,49);
        CHECK(mems_manager_lucas5as_extract_status(
            received,sizeof(received),output,sizeof(output),
            &produced,&offset,&status));
        CHECK(produced==49 && offset==2);
        CHECK(memcmp(output,original,49)==0);
        CHECK(!mems_manager_lucas5as_extract_status(
            received,22,output,sizeof(output),
            &produced,&offset,&status));
        CHECK(produced==49 && offset==2);
        CHECK(!mems_manager_lucas5as_extract_status(
            received,sizeof(received),output,48,
            &produced,&offset,&status));
        CHECK(!mems_manager_lucas5as_extract_status(
            received,sizeof(received),output,sizeof(output),
            NULL,&offset,&status));
        /* Arbitrary 0x55 is not a legal prefix to be scanned through. */
        received[0]=0x55;
        CHECK(!mems_manager_lucas5as_extract_status(
            received,sizeof(received),output,sizeof(output),
            &produced,&offset,&status));
    }
    if (failures) {
        fprintf(stderr,"LUCAS5AS_PROTOCOL_SELFTEST_FAIL (%d)\n",failures);
        return 1;
    }
    puts("LUCAS5AS_PROTOCOL_SELFTEST_PASS: Session205 synthetic codec and boundaries");
    return 0;
}
