#include "rosco.h"

#include <stddef.h>
#include <stdio.h>

#define CHECK_EQ(label, got, expected) \
    do { \
        size_t _g = (size_t)(got); \
        size_t _e = (size_t)(expected); \
        if (_g != _e) { \
            fprintf(stderr, "FAIL: %s = %zu, expected %zu\n", (label), _g, _e); \
            ++failures; \
        } else { \
            printf("OK: %s = %zu\n", (label), _g); \
        } \
    } while (0)

int main(void)
{
    int failures = 0;
    librosco_version version = mems_get_lib_version();

    CHECK_EQ("sizeof(mems_data_frame_80)", sizeof(mems_data_frame_80), 28);
    CHECK_EQ("sizeof(mems_data_frame_7d)", sizeof(mems_data_frame_7d), 32);
    CHECK_EQ("sizeof(mems_data)", sizeof(mems_data), 60);
    CHECK_EQ("offsetof(mems_data, engine_rpm)", offsetof(mems_data, engine_rpm), 0);
    CHECK_EQ("offsetof(mems_data, fault_codes)", offsetof(mems_data, fault_codes), 12);
    CHECK_EQ("offsetof(mems_data, iac_position)", offsetof(mems_data, iac_position), 16);
    CHECK_EQ("offsetof(mems_data, idle_error)", offsetof(mems_data, idle_error), 18);
    CHECK_EQ("offsetof(mems_data, coil_time)", offsetof(mems_data, coil_time), 22);
    CHECK_EQ("offsetof(mems_data, ignition_switch)", offsetof(mems_data, ignition_switch), 27);
    CHECK_EQ("offsetof(mems_data, dtc0)", offsetof(mems_data, dtc0), 58);
    CHECK_EQ("offsetof(mems_data, dtc1)", offsetof(mems_data, dtc1), 59);

#if defined(_WIN64)
    CHECK_EQ("sizeof(mems_info) on Win64", sizeof(mems_info), 16);
#else
    fprintf(stderr, "FAIL: self-test was not compiled for _WIN64\n");
    ++failures;
#endif

    CHECK_EQ("librosco major", version.major, 0);
    CHECK_EQ("librosco minor", version.minor, 1);
    CHECK_EQ("librosco patch", version.patch, 12);

    CHECK_EQ("MEMS_ResetECU", MEMS_ResetECU, 0xAF);
    CHECK_EQ("MEMS_ResetAdj", MEMS_ResetAdj, 0xFA);
    CHECK_EQ("MEMS_ResetEMI", MEMS_ResetEMI, 0x0F);
    CHECK_EQ("MEMS_Save", MEMS_Save, 0x81);
    CHECK_EQ("MEMS_FuelTrim2Plus", MEMS_FuelTrim2Plus, 0x7B);
    CHECK_EQ("MEMS_FuelTrim2Minus", MEMS_FuelTrim2Minus, 0x7C);
    CHECK_EQ("temperature value=55", temperature_value_to_degrees_f(55), 32);
    CHECK_EQ("temperature value=65", temperature_value_to_degrees_f(65), 50);
    CHECK_EQ("temperature wraps uint8",temperature_value_to_degrees_f(0), 137);
    CHECK_EQ("MEMS_ReqData80", MEMS_ReqData80, 0x80);
    CHECK_EQ("MEMS_ReqData7D", MEMS_ReqData7D, 0x7D);
    /* New export: must link and reject a missing transport. */
    if (mems_Save(NULL) || mems_reset_ADJ(NULL) ||
        mems_reset_EMI(NULL) || mems_fuel_trim_2_plus(NULL,NULL) ||
        mems_fuel_trim_2_minus(NULL,NULL) || mems_save_adjustments(NULL)) {
        fprintf(stderr,"FAIL: save accepted invalid session\n");
        ++failures;
    }
    /* Additive MEMS 1.9 K-Line exports: no hardware needed, and the two
     * original init/read exports remain independently linked unchanged. */
    if (mems_init_link_kline19(NULL, NULL, false) ||
        mems_read_kline19(NULL, NULL, false) ||
        mems_command_kline19(NULL, 0xCC, false, NULL) ||
        mems_move_iac_kline19(NULL, 0, false))
    {
        fprintf(stderr, "FAIL: K-Line functions accepted invalid session\n");
        ++failures;
    }


    if (failures)
    {
        fprintf(stderr, "ABI SELFTEST FAILED: %d check(s) failed\n", failures);
        return 1;
    }

    printf("ABI SELFTEST OK\n");
    return 0;
}
