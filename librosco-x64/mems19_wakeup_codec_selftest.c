/* Pure offline Session 205 1.9 ISO9141 helpers, linked to actual DLL. */
#include "mems19_wakeup.h"
#include <stdio.h>
#include <string.h>

static int failures;
#define CHECK(expr) do { if (!(expr)) { \
  fprintf(stderr,"MEMS19_CODEC_SELFTEST_FAIL line %d: %s\n",__LINE__,#expr); \
  failures++; \
} } while (0)

int main(void)
{
    /* 0x16: start 0, data LSB 0 1 1 0 1 0 0 0, stop 1. */
    const bool expected[10] = { false,false,true,true,false,true,
                                false,false,false,true };
    const uint8_t typical[] = {0x55,0x76,0x83};
    const uint8_t prefixed[] = {0,0x16,0x55,0x12,0x34};
    const uint8_t incomplete[] = {0x55,0x76};
    const uint8_t confirm[] = {0x7C,0xE9};
    const uint8_t bad_confirm[] = {0x7C,0xE8};
    uint8_t inverted = 0xCA;
    bool high = true;
    size_t i;
    for (i=0;i<10;++i) {
        CHECK(mems_manager_mems19_slow_bit_high((int)i,&high));
        CHECK(high==expected[i]);
    }
    CHECK(!mems_manager_mems19_slow_bit_high(-1,&high));
    CHECK(!mems_manager_mems19_slow_bit_high(10,&high));
    CHECK(!mems_manager_mems19_slow_bit_high(0,NULL));

    CHECK(mems_manager_mems19_key_complement(typical,sizeof(typical),&inverted));
    CHECK(inverted==0x7Cu);
    CHECK(mems_manager_mems19_key_complement(prefixed,sizeof(prefixed),&inverted));
    CHECK(inverted==(uint8_t)~0x34u);
    inverted=0xAB;
    CHECK(!mems_manager_mems19_key_complement(incomplete,sizeof(incomplete),&inverted));
    CHECK(inverted==0xAB); /* rejected input must not poison caller's output */
    CHECK(!mems_manager_mems19_key_complement(NULL,3,&inverted));
    CHECK(!mems_manager_mems19_key_complement(typical,3,NULL));

    CHECK(mems_manager_mems19_has_address_confirmation(confirm,sizeof(confirm)));
    CHECK(!mems_manager_mems19_has_address_confirmation(bad_confirm,sizeof(bad_confirm)));
    CHECK(!mems_manager_mems19_has_address_confirmation(NULL,3));
    CHECK(!mems_manager_mems19_has_address_confirmation(confirm,1));
    if (failures) return 1;
    puts("MEMS19_CODEC_SELFTEST_PASS: Session205 address 16 ISO9141 and flexible key bytes");
    return 0;
}
