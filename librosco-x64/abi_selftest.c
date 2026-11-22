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

    CHECK_EQ("librosco major", version.major, 3);
    CHECK_EQ("librosco minor", version.minor, 0);
    CHECK_EQ("librosco patch", version.patch, 0);

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

    /* Runtime proof: the built DLL must physically export all 45 legacy
     * names and all eight distinct additive exports (not just .def text).
     */
    {
        static const char* exports[]={
        "mems_Boost_Valve_Off",
        "mems_Boost_Valve_On",
        "mems_CloseIAC",
        "mems_Fan1_Off",
        "mems_Fan1_On",
        "mems_Fan2_Off",
        "mems_Fan2_On",
        "mems_O2Heater_Off",
        "mems_O2Heater_On",
        "mems_OpenIAC",
        "mems_Purge_Valve_Off",
        "mems_Purge_Valve_On",
        "mems_Save",
        "mems_cleanup",
        "mems_clear_faults",
        "mems_connect",
        "mems_disconnect",
        "mems_fuel_trim_minus",
        "mems_fuel_trim_plus",
        "mems_get_lib_version",
        "mems_heartbeat",
        "mems_idle_decay_minus",
        "mems_idle_decay_plus",
        "mems_idle_speed_minus",
        "mems_idle_speed_plus",
        "mems_ignition_advance_minus",
        "mems_ignition_advance_plus",
        "mems_init",
        "mems_init_link",
        "mems_is_connected",
        "mems_lock",
        "mems_move_iac",
        "mems_openserial",
        "mems_read",
        "mems_read_iac_position",
        "mems_read_raw",
        "mems_read_serial",
        "mems_reset_ADJ",
        "mems_reset_ECU",
        "mems_reset_EMI",
        "mems_send_command",
        "mems_test_actuator",
        "mems_unlock",
        "mems_write_serial",
        "temperature_value_to_degrees_f",
        "mems_reset_adjustments",
        "mems_save_adjustments",
        "mems_init_link_kline19",
        "mems_read_kline19",
        "mems_command_kline19",
        "mems_move_iac_kline19",
        "mems_fuel_trim_2_plus",
        "mems_fuel_trim_2_minus",
        "mems_reset_ECU_leopold"
        };
        HMODULE module=GetModuleHandleA("mems_manager_x64.dll");
        size_t i=0;
        if(!module){
            fprintf(stderr,"FAIL: native mems_manager_x64.dll not loaded\n");
            ++failures;
        }else{
            CHECK_EQ("historical and additive export total",
                     sizeof(exports)/sizeof(exports[0]),54);
            for(i=0;i<sizeof(exports)/sizeof(exports[0]);++i){
                if(!GetProcAddress(module,exports[i])){
                    fprintf(stderr,"FAIL: missing original/additive export %s\n",exports[i]);
                    ++failures;
                }
            }
        }
    }
    /* New export: must link and reject a missing transport. */
    if (mems_Save(NULL) || mems_reset_ADJ(NULL) ||
        mems_reset_EMI(NULL) || mems_fuel_trim_2_plus(NULL,NULL) ||
        mems_fuel_trim_2_minus(NULL,NULL) || mems_save_adjustments(NULL) ||
        mems_reset_ECU_leopold(NULL)) {
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
