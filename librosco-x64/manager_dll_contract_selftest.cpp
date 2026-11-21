#include "../rosco.h"
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <cstdio>

// This test uses the REAL header included by Manager, and the REAL x64 DLL.
// A green DLL-only C test is not enough: test all 53 C++ symbol contracts.
static_assert(sizeof(mems_data_frame_80) == 28, "Manager frame 0x80 ABI mismatch");
static_assert(sizeof(mems_data_frame_7d) == 32, "Manager frame 0x7D ABI mismatch");
static_assert(sizeof(mems_data) == 60, "Manager decoded data ABI mismatch");
static_assert(sizeof(mems_info) == 16, "Manager connection handle ABI mismatch");
static_assert(offsetof(mems_data, engine_rpm) == 0, "Manager engine RPM offset mismatch");
static_assert(offsetof(mems_data, fault_codes) == 12, "Manager fault code offset mismatch");
static_assert(offsetof(mems_data, iac_position) == 16, "Manager IAC offset mismatch");
static_assert(offsetof(mems_data, dtc0) == 58, "Manager dtc0 offset mismatch");
static_assert(offsetof(mems_data, dtc1) == 59, "Manager dtc1 offset mismatch");
static_assert(MEMS_ResetECU == 0xAF, "Manager ECU reset must match original 45-export DLL");
static_assert(MEMS_ResetAdj == 0xFA, "Manager adjustment reset must match original 45-export DLL");
static_assert(MEMS_ResetEMI == 0x0F, "Manager emissions reset must match original 45-export DLL");
static_assert(MEMS_Save == 0x81, "Manager save command must match original 45-export DLL");
static_assert(MEMS_FuelTrimPlus == 0x79 && MEMS_FuelTrimMinus == 0x7A, "Original richness commands mismatch");
static_assert(MEMS_FuelTrim2Plus == 0x7B && MEMS_FuelTrim2Minus == 0x7C, "Additional richness commands mismatch");

#define VERIFY_CPP_ABI(symbol, type) static_assert(std::is_same<decltype(&symbol), type>::value, #symbol " signature mismatch");
using Signature0 = bool (*)(mems_info*, uint8_t*);
using Signature1 = bool (*)(mems_info*);
using Signature2 = void (*)(mems_info*);
using Signature3 = bool (*)(mems_info*, const char*);
using Signature4 = librosco_version (*)(void);
using Signature5 = bool (*)(mems_info*, uint8_t);
using Signature6 = bool (*)(mems_info*, mems_data*);
using Signature7 = bool (*)(mems_info*, mems_data_frame_80*, mems_data_frame_7d*);
using Signature8 = int16_t (*)(mems_info*, uint8_t*, uint16_t);
using Signature9 = bool (*)(mems_info*, actuator_cmd, uint8_t*);
using Signature10 = uint8_t (*)(uint8_t);
using Signature11 = bool (*)(mems_info*, uint8_t*, bool);
using Signature12 = bool (*)(mems_info*, mems_data*, bool);
using Signature13 = bool (*)(mems_info*, uint8_t, bool, uint8_t*);
using Signature14 = bool (*)(mems_info*, uint8_t, bool);

VERIFY_CPP_ABI(mems_Boost_Valve_Off, Signature0);
VERIFY_CPP_ABI(mems_Boost_Valve_On, Signature0);
VERIFY_CPP_ABI(mems_CloseIAC, Signature0);
VERIFY_CPP_ABI(mems_Fan1_Off, Signature0);
VERIFY_CPP_ABI(mems_Fan1_On, Signature0);
VERIFY_CPP_ABI(mems_Fan2_Off, Signature0);
VERIFY_CPP_ABI(mems_Fan2_On, Signature0);
VERIFY_CPP_ABI(mems_O2Heater_Off, Signature0);
VERIFY_CPP_ABI(mems_O2Heater_On, Signature0);
VERIFY_CPP_ABI(mems_OpenIAC, Signature0);
VERIFY_CPP_ABI(mems_Purge_Valve_Off, Signature0);
VERIFY_CPP_ABI(mems_Purge_Valve_On, Signature0);
VERIFY_CPP_ABI(mems_Save, Signature1);
VERIFY_CPP_ABI(mems_cleanup, Signature2);
VERIFY_CPP_ABI(mems_clear_faults, Signature1);
VERIFY_CPP_ABI(mems_connect, Signature3);
VERIFY_CPP_ABI(mems_disconnect, Signature2);
VERIFY_CPP_ABI(mems_fuel_trim_minus, Signature0);
VERIFY_CPP_ABI(mems_fuel_trim_plus, Signature0);
VERIFY_CPP_ABI(mems_get_lib_version, Signature4);
VERIFY_CPP_ABI(mems_heartbeat, Signature1);
VERIFY_CPP_ABI(mems_idle_decay_minus, Signature0);
VERIFY_CPP_ABI(mems_idle_decay_plus, Signature0);
VERIFY_CPP_ABI(mems_idle_speed_minus, Signature0);
VERIFY_CPP_ABI(mems_idle_speed_plus, Signature0);
VERIFY_CPP_ABI(mems_ignition_advance_minus, Signature0);
VERIFY_CPP_ABI(mems_ignition_advance_plus, Signature0);
VERIFY_CPP_ABI(mems_init, Signature2);
VERIFY_CPP_ABI(mems_init_link, Signature0);
VERIFY_CPP_ABI(mems_is_connected, Signature1);
VERIFY_CPP_ABI(mems_lock, Signature1);
VERIFY_CPP_ABI(mems_move_iac, Signature5);
VERIFY_CPP_ABI(mems_openserial, Signature3);
VERIFY_CPP_ABI(mems_read, Signature6);
VERIFY_CPP_ABI(mems_read_iac_position, Signature0);
VERIFY_CPP_ABI(mems_read_raw, Signature7);
VERIFY_CPP_ABI(mems_read_serial, Signature8);
VERIFY_CPP_ABI(mems_reset_ADJ, Signature1);
VERIFY_CPP_ABI(mems_reset_ECU, Signature1);
VERIFY_CPP_ABI(mems_reset_EMI, Signature1);
VERIFY_CPP_ABI(mems_send_command, Signature5);
VERIFY_CPP_ABI(mems_test_actuator, Signature9);
VERIFY_CPP_ABI(mems_unlock, Signature2);
VERIFY_CPP_ABI(mems_write_serial, Signature8);
VERIFY_CPP_ABI(temperature_value_to_degrees_f, Signature10);
VERIFY_CPP_ABI(mems_reset_adjustments, Signature1);
VERIFY_CPP_ABI(mems_save_adjustments, Signature1);
VERIFY_CPP_ABI(mems_init_link_kline19, Signature11);
VERIFY_CPP_ABI(mems_read_kline19, Signature12);
VERIFY_CPP_ABI(mems_command_kline19, Signature13);
VERIFY_CPP_ABI(mems_move_iac_kline19, Signature14);
VERIFY_CPP_ABI(mems_fuel_trim_2_plus, Signature0);
VERIFY_CPP_ABI(mems_fuel_trim_2_minus, Signature0);

int main()
{
    const librosco_version v = mems_get_lib_version();
    if (v.major != 3 || v.minor != 0 || v.patch != 0) {
        std::fprintf(stderr, "Manager x64 DLL version differs from original 3.0.0\\n");
        return 1;
    }
    std::printf("Manager x64 header, 53 prototypes, ABI and original version: PASS\\n");
    return 0;
}
