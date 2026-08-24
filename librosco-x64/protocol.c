#include "rosco.h"

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

    if (WriteFile(info->sd, buffer, (DWORD)quantity, &written, NULL) == TRUE &&
        written == (DWORD)quantity)
        return (int16_t)written;

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

bool mems_read(mems_info* info, mems_data* data)
{
    mems_data_frame_80 dframe80;
    mems_data_frame_7d dframe7d;

    if (!info || !data)
        return false;

    if (!mems_read_raw(info, &dframe80, &dframe7d))
        return false;

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

bool mems_clear_faults(mems_info* info)
{
    return send_command_with_one_byte_reply(info, MEMS_ClearFaults);
}

bool mems_reset_ECU(mems_info* info)
{
    /* Confirmed by direct disassembly of the current DLL. */
    return send_command_with_one_byte_reply(info, MEMS_ResetECU);
}

bool mems_reset_adjustments(mems_info* info)
{
    /* Confirmed by direct disassembly of the current DLL. */
    return send_command_with_one_byte_reply(info, MEMS_ResetAdj);
}

bool mems_heartbeat(mems_info* info)
{
    return send_command_with_one_byte_reply(info, MEMS_Heartbeat);
}
