#include "rosco.h"
#include "legacy_trace.h"

#include <stdio.h>
#include <string.h>

int16_t mems_read_serial(mems_info* info, uint8_t* buffer, uint16_t quantity)
{
    uint16_t total = 0;

    if (!info || !buffer || quantity == 0 || !mems_is_connected(info))
        return 0;

    while (total < quantity)
    {
        DWORD got = 0;
        const DWORD remaining = (DWORD)(quantity - total);
        if (ReadFile(info->sd, buffer + total, remaining, &got, NULL) != TRUE || got == 0)
            break;
        mems_internal_trace_bytes(info->sd,2,(uint64_t)GetTickCount64(),buffer+total,got);
        total = (uint16_t)(total + (uint16_t)got);
    }

    if (total < quantity)
        fprintf(stderr, "mems_read_serial(): expected %u, got %u\n", (unsigned)quantity, (unsigned)total);

    return (int16_t)total;
}

int16_t mems_write_serial(mems_info* info, uint8_t* buffer, uint16_t quantity)
{
    DWORD written = 0;

    if (!info || !buffer || quantity == 0 || !mems_is_connected(info))
        return -1;

    if (WriteFile(info->sd, buffer, (DWORD)quantity, &written, NULL) == TRUE)
    {
        if (written && written <= quantity)
            mems_internal_trace_bytes(info->sd,1,(uint64_t)GetTickCount64(),buffer,written);
        if (written == (DWORD)quantity) return (int16_t)written;
    }

    return -1;
}

bool mems_send_command(mems_info* info, uint8_t cmd)
{
    uint8_t response = 0xFF;

    if (mems_write_serial(info, &cmd, 1) != 1)
    {
        fprintf(stderr, "mems_send_command(): failed to send command %02X\n", cmd);
        return false;
    }

    if (mems_read_serial(info, &response, 1) != 1)
    {
        fprintf(stderr, "mems_send_command(): did not receive echo of command %02X\n", cmd);
        return false;
    }

    if (response != cmd)
    {
        fprintf(stderr,
                "mems_send_command(): received nonmatching byte %02X for command %02X\n",
                response, cmd);
        return false;
    }

    return true;
}

bool mems_init_link(mems_info* info, uint8_t* d0_response_buffer)
{
    const uint8_t command_a = 0xCA;
    const uint8_t command_b = 0x75;
    const uint8_t command_c = MEMS_Heartbeat;
    const uint8_t command_d = 0xD0;
    uint8_t terminator = 0xFF;

    if (!info || !d0_response_buffer)
        return false;

    if (!mems_send_command(info, command_a))
        return false;
    if (!mems_send_command(info, command_b))
        return false;
    if (!mems_send_command(info, command_c))
        return false;

    if (mems_read_serial(info, &terminator, 1) != 1)
        return false;

    if (!mems_send_command(info, command_d))
        return false;

    return mems_read_serial(info, d0_response_buffer, 4) == 4;
}

bool mems_lock(mems_info* info)
{
    if (!info || !info->mutex)
        return false;
    return WaitForSingleObject(info->mutex, INFINITE) == WAIT_OBJECT_0;
}

void mems_unlock(mems_info* info)
{
    if (info && info->mutex)
        ReleaseMutex(info->mutex);
}

bool mems_read_raw(mems_info* info,
                   mems_data_frame_80* frame80,
                   mems_data_frame_7d* frame7d)
{
    bool status = false;

    if (!info || !frame80 || !frame7d)
        return false;

    if (!mems_lock(info))
        return false;

    if (mems_send_command(info, MEMS_ReqData80) &&
        mems_read_serial(info, (uint8_t*)frame80, (uint16_t)sizeof(*frame80)) == (int16_t)sizeof(*frame80))
    {
        status = true;
    }

    if (status)
    {
        /* The 32-bit Haro binary reads exactly 32 bytes here.  Unlike the
         * historical precedence bug, this x64 source verifies the length;
         * successful ECU traffic remains byte-for-byte compatible while a
         * truncated frame is no longer accepted as valid. */
        if (!mems_send_command(info, MEMS_ReqData7D) ||
            mems_read_serial(info, (uint8_t*)frame7d, (uint16_t)sizeof(*frame7d)) != (int16_t)sizeof(*frame7d))
        {
            status = false;
        }
    }

    mems_unlock(info);
    return status;
}

/* Decode both the historical 1.x frames and independently received K-Line
 * frames through the same mapping. Existing mems_read() behaviour is intact. */
static bool mems_decode_frames(const mems_data_frame_80* source80,
                               const mems_data_frame_7d* source7d,
                               mems_data* data)
{
    if (!source80 || !source7d || !data)
        return false;

    const mems_data_frame_80 dframe80 = *source80;
    const mems_data_frame_7d dframe7d = *source7d;

    /* This mapping is reconstructed from the actual 49,672-byte DLL.
     * It deliberately preserves the Haro 60-byte raw-oriented structure:
     * values such as temperatures, battery, ignition advance and coil time
     * are not converted here.  ECU MEMS Manager applies its own decoding. */
    memset(data, 0, sizeof(*data));

    data->engine_rpm = (uint16_t)(((uint16_t)dframe80.engine_rpm_hi << 8) |
                                  (uint16_t)dframe80.engine_rpm_lo);
    data->coolant_temp = dframe80.coolant_temp;
    data->ambient_temp = dframe80.ambient_temp;
    data->intake_air_temp = dframe80.intake_air_temp;
    data->fuel_temp = dframe80.fuel_temp;
    data->map_kpa = dframe80.map_kpa;
    data->battery_voltage = dframe80.battery_voltage;
    data->throttle_pot = dframe80.throttle_pot;
    data->idle_switch = dframe80.idle_switch;
    data->uk1 = dframe80.uk1;
    data->park_neutral_switch = dframe80.park_neutral_switch;
    data->fault_codes = 0;
    data->idle_set_point = dframe80.idle_set_point;
    data->idle_hot = dframe80.idle_hot;
    data->uk2 = dframe80.uk2;
    data->iac_position = dframe80.iac_position;
    data->idle_error = (uint16_t)(((uint16_t)dframe80.idle_error_hi << 8) |
                                  (uint16_t)dframe80.idle_error_lo);
    data->ignition_advance_offset = dframe80.ignition_advance_offset;
    data->ignition_advance = dframe80.ignition_advance;
    data->coil_time = (uint16_t)(((uint16_t)dframe80.coil_time_hi << 8) |
                                 (uint16_t)dframe80.coil_time_lo);
    data->uk3 = dframe80.uk3;
    data->uk4 = dframe80.uk4;
    data->uk5 = dframe80.uk5;

    data->ignition_switch = dframe7d.ignition_switch;
    data->throttle_angle = dframe7d.throttle_angle;
    data->uk6 = dframe7d.uk6;
    data->air_fuel_ratio = dframe7d.air_fuel_ratio;
    data->dtc2 = dframe7d.fault_code0;
    data->lambda_voltage = dframe7d.lambda_voltage;
    data->lambda_sensor_frequency = dframe7d.lambda_sensor_frequency;
    data->lambda_sensor_dutycycle = dframe7d.lambda_sensor_dutycycle;
    data->lambda_sensor_status = dframe7d.lambda_sensor_status;
    data->closed_loop = dframe7d.closed_loop;
    data->long_term_fuel_trim = dframe7d.long_term_fuel_trim;
    data->short_term_fuel_trim = dframe7d.short_term_fuel_trim;
    data->carbon_canister_dutycycle = dframe7d.carbon_canister_dutycycle;
    data->dtc3 = dframe7d.fault_code1;
    data->idle_base_pos = dframe7d.idle_base_pos;
    data->uk7 = dframe7d.uk7;
    data->dtc4 = dframe7d.uk8;
    data->ignition_advance2 = dframe7d.ignition_advance2;
    data->idle_speed_offset = dframe7d.uk9;
    data->idle_error2 = dframe7d.idle_error2;
    data->uk10 = dframe7d.uk10;
    data->dtc5 = dframe7d.fault_code4;
    data->uk11 = dframe7d.uk11;
    data->uk12 = dframe7d.uk12;
    data->uk13 = dframe7d.uk13;
    data->uk14 = dframe7d.uk14;
    data->uk15 = dframe7d.uk15;
    data->uk16 = dframe7d.uk16;
    data->uk1A = dframe7d.uk1A;
    data->uk1B = dframe7d.uk1B;
    data->uk1C = dframe7d.uk1C;

    data->dtc0 = dframe80.dtc0;
    data->dtc1 = dframe80.dtc1;

    if (dframe80.dtc0 & 0x01)
        data->fault_codes |= (uint8_t)(1u << 0);
    if (dframe80.dtc0 & 0x02)
        data->fault_codes |= (uint8_t)(1u << 1);
    if (dframe80.dtc1 & 0x02)
        data->fault_codes |= (uint8_t)(1u << 2);
    if (dframe80.dtc1 & 0x80)
        data->fault_codes |= (uint8_t)(1u << 3);

    return true;
}

bool mems_read(mems_info* info, mems_data* data)
{
    mems_data_frame_80 frame80;
    mems_data_frame_7d frame7d;
    if (!info || !data || !mems_read_raw(info, &frame80, &frame7d))
        return false;
    return mems_decode_frames(&frame80, &frame7d, data);
}

/* MEMS 1.9 runs the same 1.x command set over K-Line. The KKL hardware may
 * echo our transmitted byte before the ECU echoes it again. Keep this path
 * distinct from the historical three-pin ROSCO transport. */
static bool mems19_send(mems_info* info, uint8_t command, bool local_echo)
{
    uint8_t received = 0;
    if (mems_write_serial(info, &command, 1) != 1)
        return false;
    if (local_echo)
    {
        if (mems_read_serial(info, &received, 1) != 1 || received != command)
            return false;
    }
    return mems_read_serial(info, &received, 1) == 1 && received == command;
}

/* Reuse the connected K-Line handle and consume exactly the local echo (when
 * present), the ECU echo and then the command's one-byte reply. The legacy
 * mems_send_command() remains unchanged for real ROSCO 1.x cables. */
bool mems_command_kline19(mems_info* info, uint8_t command,
                          bool local_echo, uint8_t* reply)
{
    uint8_t result = 0;
    bool ok;
    if (!info || !mems_is_connected(info) || !mems_lock(info))
        return false;
    ok = mems19_send(info, command, local_echo) &&
         mems_read_serial(info, &result, 1) == 1;
    if (ok && reply)
        *reply = result;
    mems_unlock(info);
    return ok;
}

bool mems_move_iac_kline19(mems_info* info, uint8_t desired_pos,
                           bool local_echo)
{
    uint8_t current = 0;
    bool ok = false;
    uint16_t attempts = 0;
    actuator_cmd cmd;

    if (!mems_command_kline19(info, MEMS_GetIACPosition, local_echo, &current))
        return false;
    if (desired_pos == current)
        return true;
    if (desired_pos > current && current >= IAC_MAXIMUM)
        return false;

    cmd = desired_pos > current ? MEMS_OpenIAC : MEMS_CloseIAC;
    do
    {
        ok = mems_command_kline19(info, (uint8_t)cmd, local_echo, &current);
        ++attempts;
    }
    while (ok && current != desired_pos && attempts < 300);
    return ok && current == desired_pos;
}

bool mems_init_link_kline19(mems_info* info, uint8_t* ecu_id, bool local_echo)
{
    uint8_t status = 0xFF;
    if (!info || !ecu_id || !mems_is_connected(info))
        return false;
    if (!mems19_send(info, 0xCA, local_echo) ||
        !mems19_send(info, 0x75, local_echo) ||
        !mems19_send(info, MEMS_Heartbeat, local_echo))
        return false;
    if (mems_read_serial(info, &status, 1) != 1 || status != 0x00)
        return false;
    if (!mems19_send(info, 0xD0, local_echo))
        return false;
    return mems_read_serial(info, ecu_id, 4) == 4;
}

/* First payload byte is its announced length, including that byte.
 * Consume the actual announced length, never a hard-coded 28/32-byte read.
 * Keep original frame structures zero-initialized for optional tail fields. */
static bool mems19_packet(mems_info* info, uint8_t command,
                          bool local_echo, void* output, size_t output_size,
                          uint8_t minimum_length)
{
    uint8_t bytes[255];
    uint8_t packet_length = 0;
    if (!output || !mems19_send(info, command, local_echo) ||
        mems_read_serial(info, &packet_length, 1) != 1 ||
        packet_length < minimum_length)
        return false;

    memset(bytes, 0, sizeof(bytes));
    bytes[0] = packet_length;
    if (packet_length > 1 &&
        mems_read_serial(info, bytes + 1, (uint16_t)(packet_length - 1)) !=
            (int16_t)(packet_length - 1))
        return false;

    memset(output, 0, output_size);
    memcpy(output, bytes, packet_length < output_size ?
           packet_length : output_size);
    return true;
}

bool mems_read_kline19(mems_info* info, mems_data* data, bool local_echo)
{
    mems_data_frame_80 frame80;
    mems_data_frame_7d frame7d;
    bool result = false;
    if (!info || !data || !mems_is_connected(info) || !mems_lock(info))
        return false;

    /* 0x80 requires the two-byte RPM and basic fault fields. 0x7D requires
     * at least the core sensor and fault fields. Shorter packets cannot be
     * interpreted as valid measurements. Tail data is ECU-version-dependent. */
    result = mems19_packet(info, MEMS_ReqData80, local_echo,
                           &frame80, sizeof(frame80), 15) &&
             mems19_packet(info, MEMS_ReqData7D, local_echo,
                           &frame7d, sizeof(frame7d), 15);
    mems_unlock(info);
    return result && mems_decode_frames(&frame80, &frame7d, data);
}

bool mems_read_iac_position(mems_info* info, uint8_t* position)
{
    bool status = false;

    if (!info || !position)
        return false;

    if (mems_lock(info))
    {
        status = mems_send_command(info, MEMS_GetIACPosition) &&
                 mems_read_serial(info, position, 1) == 1;
        mems_unlock(info);
    }

    return status;
}

bool mems_move_iac(mems_info* info, uint8_t desired_pos)
{
    bool status = false;
    uint16_t attempts = 0;
    uint8_t current_pos = 0;
    actuator_cmd cmd;

    if (!info)
        return false;

    if (mems_read_iac_position(info, &current_pos))
    {
        if ((desired_pos < current_pos) ||
            ((desired_pos > current_pos) && (current_pos < IAC_MAXIMUM)))
        {
            cmd = (desired_pos > current_pos) ? MEMS_OpenIAC : MEMS_CloseIAC;
            do
            {
                status = mems_test_actuator(info, cmd, &current_pos);
                ++attempts;
            }
            while (status && current_pos != desired_pos && attempts < 300);
        }
    }

    return desired_pos == current_pos;
}

bool mems_test_actuator(mems_info* info, actuator_cmd cmd, uint8_t* data)
{
    bool status = false;
    uint8_t response = 0x00;

    if (!info)
        return false;

    if (mems_lock(info))
    {
        if (mems_send_command(info, (uint8_t)cmd) &&
            mems_read_serial(info, &response, 1) == 1)
        {
            if (data)
                *data = response;
            status = true;
        }
        mems_unlock(info);
    }

    return status;
}

static bool send_command_with_one_byte_reply(mems_info* info, uint8_t cmd)
{
    bool status = false;
    uint8_t response = 0xFF;

    if (!info)
        return false;

    if (mems_lock(info))
    {
        status = mems_send_command(info, cmd) &&
                 mems_read_serial(info, &response, 1) == 1;
        mems_unlock(info);
    }

    return status;
}

/* All four original commands were verified against the 63,161-byte
 * LeopoldG x86 DLL. Returning true means a byte was received, not
 * independently verified nonvolatile storage.
 */
bool mems_Save(mems_info* info)
{
    return send_command_with_one_byte_reply(info, MEMS_Save);
}

bool mems_save_adjustments(mems_info* info)
{
    return mems_Save(info);
}

bool mems_reset_ADJ(mems_info* info)
{
    return send_command_with_one_byte_reply(info, MEMS_ResetAdj);
}

bool mems_reset_EMI(mems_info* info)
{
    return send_command_with_one_byte_reply(info, MEMS_ResetEMI);
}

bool mems_clear_faults(mems_info* info)
{
    return send_command_with_one_byte_reply(info, MEMS_ClearFaults);
}

/* FIXED ECU-MEMS-MANAGER-SESSION FUNCTIONAL CONTRACT (2026-11-22):
 * 0x0F = reset user-adjustable settings (Reset Adjustments).
 * 0xFA = clear computed/learned ECU parameters (Reset ECU in Manager UI).
 * These meanings were selected by the user and match the MemsFCR command
 * mapping. NEVER silently swap them or interpret 0xFA as an ECU reboot.
 * LeopoldG-45 called 0xAF "Reset ECU"; its distinct command is exported
 * separately. Preserve the two existing Manager entrypoints and ABI.
 */
bool mems_reset_ECU(mems_info* info)
{
    return mems_reset_ADJ(info); /* Manager legacy reset ECU: 0xFA */
}

bool mems_reset_adjustments(mems_info* info)
{
    return mems_reset_EMI(info); /* Manager legacy adjustments: 0x0F */
}

/* An explicit, additive entrypoint for the distinct LeopoldG ECU=AF command.
 * It is never substituted silently for Manager's existing reset action.
 */
bool mems_reset_ECU_leopold(mems_info* info)
{
    return send_command_with_one_byte_reply(info, MEMS_ResetECU);
}

/* Exact legacy wrappers: existing actuator transport transmits the
 * one-byte command, validates echo and receives the one-byte result.
 * These exports restore original API names without duplicating serial I/O.
 */
#define ROSCO_LEGACY_WRAPPER(symbol, command) \
    bool symbol(mems_info* info, uint8_t* reply) \
    { \
        return mems_test_actuator(info, command, reply); \
    }

ROSCO_LEGACY_WRAPPER(mems_fuel_trim_plus, MEMS_FuelTrimPlus)
ROSCO_LEGACY_WRAPPER(mems_fuel_trim_minus, MEMS_FuelTrimMinus)
ROSCO_LEGACY_WRAPPER(mems_idle_decay_plus, MEMS_IdleDecayPlus)
ROSCO_LEGACY_WRAPPER(mems_idle_decay_minus, MEMS_IdleDecayMinus)
ROSCO_LEGACY_WRAPPER(mems_idle_speed_plus, MEMS_IdleSpeedPlus)
ROSCO_LEGACY_WRAPPER(mems_idle_speed_minus, MEMS_IdleSpeedMinus)
ROSCO_LEGACY_WRAPPER(mems_ignition_advance_plus, MEMS_IgnitionAdvancePlus)
ROSCO_LEGACY_WRAPPER(mems_ignition_advance_minus, MEMS_IgnitionAdvanceMinus)
ROSCO_LEGACY_WRAPPER(mems_OpenIAC, MEMS_OpenIAC)
ROSCO_LEGACY_WRAPPER(mems_CloseIAC, MEMS_CloseIAC)
ROSCO_LEGACY_WRAPPER(mems_Purge_Valve_On, MEMS_PurgeValveOn)
ROSCO_LEGACY_WRAPPER(mems_Purge_Valve_Off, MEMS_PurgeValveOff)
ROSCO_LEGACY_WRAPPER(mems_O2Heater_On, MEMS_O2HeaterOn)
ROSCO_LEGACY_WRAPPER(mems_O2Heater_Off, MEMS_O2HeaterOff)
ROSCO_LEGACY_WRAPPER(mems_Boost_Valve_On, MEMS_BoostValveOn)
ROSCO_LEGACY_WRAPPER(mems_Boost_Valve_Off, MEMS_BoostValveOff)
ROSCO_LEGACY_WRAPPER(mems_Fan1_On, MEMS_Fan1On)
ROSCO_LEGACY_WRAPPER(mems_Fan1_Off, MEMS_Fan1Off)
ROSCO_LEGACY_WRAPPER(mems_Fan2_On, MEMS_Fan2On)
ROSCO_LEGACY_WRAPPER(mems_Fan2_Off, MEMS_Fan2Off)
#undef ROSCO_LEGACY_WRAPPER

/* Additional fuel adaptation channel: NOT an injector pulse/fire command. */
bool mems_fuel_trim_2_plus(mems_info* info,uint8_t* reply)
{
    return mems_test_actuator(info,MEMS_FuelTrim2Plus,reply);
}
bool mems_fuel_trim_2_minus(mems_info* info,uint8_t* reply)
{
    return mems_test_actuator(info,MEMS_FuelTrim2Minus,reply);
}

/* Reconstructed from the old DLL's x87 instructions / constants:
 * raw is first converted by unsigned 8-bit subtraction of 55,
 * then multiplied by 1.8, added to 32 and truncated toward zero.
 * The x86 declaration returned one byte: retain that result exactly.
 */
uint8_t temperature_value_to_degrees_f(uint8_t value)
{
    const uint8_t shifted=(uint8_t)(value-55u);
    const unsigned int rounded=(unsigned int)((double)shifted*1.8+32.0);
    return (uint8_t)(rounded & 255u);
}

bool mems_heartbeat(mems_info* info)
{
    return send_command_with_one_byte_reply(info, MEMS_Heartbeat);
}
