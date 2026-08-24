#ifndef LIBROSCO_X64_ROSCO_H
#define LIBROSCO_X64_ROSCO_H

#include <stdint.h>
#include <stdbool.h>

#if defined(_WIN32) || defined(WIN32)
  #include <windows.h>
#else
  #error "The x64 compatibility baseline currently targets Windows only."
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32) || defined(WIN32)
  #if defined(ROSCO_BUILD_DLL)
    #define ROSCO_API __declspec(dllexport)
  #elif defined(ROSCO_USE_DLL)
    #define ROSCO_API __declspec(dllimport)
  #else
    #define ROSCO_API
  #endif
#else
  #define ROSCO_API
#endif

#define IAC_MAXIMUM 0xB4

/* Commands verified against the Haro-compatible header and/or the actual
 * 49,672-byte librosco.dll used by ECU MEMS Manager. */
enum mems_data_command
{
    MEMS_ReqData7D      = 0x7D,
    MEMS_ReqData80      = 0x80,
    MEMS_ClearFaults    = 0xCC,
    MEMS_Heartbeat      = 0xF4,
    MEMS_GetIACPosition = 0xFB,
    MEMS_ResetAdj       = 0x0F,
    MEMS_ResetECU       = 0xFA
};

/* Command catalogue carried by the Haro/MEMS-Scan header.  The generic
 * mems_test_actuator() export is the verified transport primitive for these
 * one-byte commands.  A command being listed here does not imply that every
 * MEMS family supports it. */
enum mems_actuator_command
{
    MEMS_FuelPumpOn             = 0x11,
    MEMS_FuelPumpOff            = 0x01,
    MEMS_PTCRelayOn             = 0x12,
    MEMS_PTCRelayOff            = 0x02,
    MEMS_ACRelayOn              = 0x13,
    MEMS_ACRelayOff             = 0x03,
    MEMS_PurgeValveOn           = 0x18,
    MEMS_PurgeValveOff          = 0x08,
    MEMS_O2HeaterOn             = 0x19,
    MEMS_O2HeaterOff            = 0x09,
    MEMS_BoostValveOn           = 0x1B,
    MEMS_BoostValveOff          = 0x0B,
    MEMS_Fan1On                 = 0x1D,
    MEMS_Fan1Off                = 0x0D,
    MEMS_Fan2On                 = 0x1E,
    MEMS_Fan2Off                = 0x0E,
    MEMS_Fan3On                 = 0x6F,
    MEMS_Fan3Off                = 0x67,
    MEMS_WasteGateOn            = 0x1B,
    MEMS_WasteGateOff           = 0x0B,
    MEMS_TestInjectors          = 0xF7,
    MEMS_TestInjectorsMPi       = 0xEF,
    MEMS_FireCoil               = 0xF8,
    MEMS_OpenIAC                = 0xFD,
    MEMS_CloseIAC               = 0xFE,
    MEMS_AllActuatorsOff        = 0xF4,
    MEMS_FuelTrimPlus           = 0x79,
    MEMS_FuelTrimMinus          = 0x7A,
    MEMS_IdleDecayPlus          = 0x89,
    MEMS_IdleDecayMinus         = 0x8A,
    MEMS_IdleSpeedPlus          = 0x91,
    MEMS_IdleSpeedMinus         = 0x92,
    MEMS_IgnitionAdvancePlus    = 0x93,
    MEMS_IgnitionAdvanceMinus   = 0x94
};

typedef enum mems_actuator_command actuator_cmd;

typedef struct
{
    uint8_t bytes_in_frame;
    uint8_t ignition_switch;
    uint8_t throttle_angle;
    uint8_t uk6;
    uint8_t air_fuel_ratio;
    uint8_t fault_code0;
    uint8_t lambda_voltage;
    uint8_t lambda_sensor_frequency;
    uint8_t lambda_sensor_dutycycle;
    uint8_t lambda_sensor_status;
    uint8_t closed_loop;
    uint8_t long_term_fuel_trim;
    uint8_t short_term_fuel_trim;
    uint8_t carbon_canister_dutycycle;
    uint8_t fault_code1;
    uint8_t idle_base_pos;
    uint8_t uk7;
    uint8_t uk8;
    uint8_t ignition_advance2;
    uint8_t uk9;
    uint8_t idle_error2;
    uint8_t uk10;
    uint8_t fault_code4;
    uint8_t uk11;
    uint8_t uk12;
    uint8_t uk13;
    uint8_t uk14;
    uint8_t uk15;
    uint8_t uk16;
    uint8_t uk1A;
    uint8_t uk1B;
    uint8_t uk1C;
} mems_data_frame_7d;

typedef struct
{
    uint8_t bytes_in_frame;
    uint8_t engine_rpm_hi;
    uint8_t engine_rpm_lo;
    uint8_t coolant_temp;
    uint8_t ambient_temp;
    uint8_t intake_air_temp;
    uint8_t fuel_temp;
    uint8_t map_kpa;
    uint8_t battery_voltage;
    uint8_t throttle_pot;
    uint8_t idle_switch;
    uint8_t uk1;
    uint8_t park_neutral_switch;
    uint8_t dtc0;
    uint8_t dtc1;
    uint8_t idle_set_point;
    uint8_t idle_hot;
    uint8_t uk2;
    uint8_t iac_position;
    uint8_t idle_error_hi;
    uint8_t idle_error_lo;
    uint8_t ignition_advance_offset;
    uint8_t ignition_advance;
    uint8_t coil_time_hi;
    uint8_t coil_time_lo;
    uint8_t uk3;
    uint8_t uk4;
    uint8_t uk5;
} mems_data_frame_80;

typedef struct
{
    uint16_t engine_rpm;
    uint8_t coolant_temp;
    uint8_t ambient_temp;
    uint8_t intake_air_temp;
    uint8_t fuel_temp;
    uint8_t map_kpa;
    uint8_t battery_voltage;
    uint8_t throttle_pot;
    uint8_t idle_switch;
    uint8_t uk1;
    uint8_t park_neutral_switch;
    uint8_t fault_codes;
    uint8_t idle_set_point;
    uint8_t idle_hot;
    uint8_t uk2;
    uint8_t iac_position;
    uint16_t idle_error;
    uint8_t ignition_advance_offset;
    uint8_t ignition_advance;
    uint16_t coil_time;
    uint8_t uk3;
    uint8_t uk4;
    uint8_t uk5;
    uint8_t ignition_switch;
    uint8_t throttle_angle;
    uint8_t uk6;
    uint8_t air_fuel_ratio;
    uint8_t dtc2;
    uint8_t lambda_voltage;
    uint8_t lambda_sensor_frequency;
    uint8_t lambda_sensor_dutycycle;
    uint8_t lambda_sensor_status;
    uint8_t closed_loop;
    uint8_t long_term_fuel_trim;
    uint8_t short_term_fuel_trim;
    uint8_t carbon_canister_dutycycle;
    uint8_t dtc3;
    uint8_t idle_base_pos;
    uint8_t uk7;
    uint8_t dtc4;
    uint8_t ignition_advance2;
    uint8_t idle_speed_offset;
    uint8_t idle_error2;
    uint8_t uk10;
    uint8_t dtc5;
    uint8_t uk11;
    uint8_t uk12;
    uint8_t uk13;
    uint8_t uk14;
    uint8_t uk15;
    uint8_t uk16;
    uint8_t uk1A;
    uint8_t uk1B;
    uint8_t uk1C;
    uint8_t dtc0;
    uint8_t dtc1;
} mems_data;

typedef struct
{
    uint8_t major;
    uint8_t minor;
    uint8_t patch;
} librosco_version;

typedef struct
{
    HANDLE sd;
    HANDLE mutex;
} mems_info;

/* Exact 22-symbol contract exported by the historical DLL. */
ROSCO_API void mems_init(mems_info* info);
ROSCO_API bool mems_init_link(mems_info* info, uint8_t* d0_response_buffer);
ROSCO_API void mems_cleanup(mems_info* info);
ROSCO_API bool mems_connect(mems_info* info, const char* devPath);
ROSCO_API void mems_disconnect(mems_info* info);
ROSCO_API bool mems_is_connected(mems_info* info);
ROSCO_API bool mems_read_raw(mems_info* info, mems_data_frame_80* frame80, mems_data_frame_7d* frame7d);
ROSCO_API bool mems_read(mems_info* info, mems_data* data);
ROSCO_API bool mems_read_iac_position(mems_info* info, uint8_t* position);
ROSCO_API bool mems_move_iac(mems_info* info, uint8_t desired_pos);
ROSCO_API bool mems_test_actuator(mems_info* info, actuator_cmd cmd, uint8_t* data);
ROSCO_API bool mems_clear_faults(mems_info* info);
ROSCO_API bool mems_reset_ECU(mems_info* info);
ROSCO_API bool mems_reset_adjustments(mems_info* info);
ROSCO_API bool mems_heartbeat(mems_info* info);
ROSCO_API librosco_version mems_get_lib_version(void);

ROSCO_API bool mems_lock(mems_info* info);
ROSCO_API void mems_unlock(mems_info* info);
ROSCO_API bool mems_openserial(mems_info* info, const char* devPath);
ROSCO_API int16_t mems_read_serial(mems_info* info, uint8_t* buffer, uint16_t quantity);
ROSCO_API int16_t mems_write_serial(mems_info* info, uint8_t* buffer, uint16_t quantity);
ROSCO_API bool mems_send_command(mems_info* info, uint8_t cmd);

#ifdef __cplusplus
}
#endif

#endif
