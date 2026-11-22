#include "family.h"

/* The existing ROSCO/K-Line primitives stay the unique implementation of
 * serial I/O, frames, IAC, faults and reset bytes. This file only provides
 * an explicit, additive per-family dispatch API without modifying Manager.
 */
bool mems_manager_family_supported(mems_manager_family family)
{
    return family == MEMS_MANAGER_FAMILY_12 ||
           family == MEMS_MANAGER_FAMILY_13 ||
           family == MEMS_MANAGER_FAMILY_16 ||
           family == MEMS_MANAGER_FAMILY_19;
}

bool mems_manager_family_uses_kline19(mems_manager_family family)
{
    return family == MEMS_MANAGER_FAMILY_19;
}

static bool family_session(mems_info* info, mems_manager_family family)
{
    return mems_manager_family_supported(family) &&
           info != NULL && mems_is_connected(info);
}

bool mems_manager_family_init_link(mems_info* info,
                                   mems_manager_family family,
                                   uint8_t* four_byte_id, bool local_echo)
{
    if (!family_session(info, family) || four_byte_id == NULL)
        return false;
    if (mems_manager_family_uses_kline19(family))
        return mems_init_link_kline19(info, four_byte_id, local_echo);
    return mems_init_link(info, four_byte_id);
}

bool mems_manager_family_read(mems_info* info, mems_manager_family family,
                              mems_data* data, bool local_echo)
{
    if (!family_session(info, family) || data == NULL)
        return false;
    if (mems_manager_family_uses_kline19(family))
        return mems_read_kline19(info, data, local_echo);
    return mems_read(info, data);
}

bool mems_manager_family_read_iac(mems_info* info,
                                  mems_manager_family family,
                                  uint8_t* position, bool local_echo)
{
    if (!family_session(info, family) || position == NULL)
        return false;
    if (mems_manager_family_uses_kline19(family))
        return mems_command_kline19(info, MEMS_GetIACPosition, local_echo,
                                    position);
    return mems_read_iac_position(info, position);
}

bool mems_manager_family_move_iac(mems_info* info,
                                  mems_manager_family family,
                                  uint8_t desired_position, bool local_echo)
{
    if (!family_session(info, family))
        return false;
    if (mems_manager_family_uses_kline19(family))
        return mems_move_iac_kline19(info, desired_position, local_echo);
    return mems_move_iac(info, desired_position);
}

bool mems_manager_family_clear_faults(mems_info* info,
                                      mems_manager_family family,
                                      bool local_echo)
{
    if (!family_session(info, family))
        return false;
    if (mems_manager_family_uses_kline19(family))
        return mems_command_kline19(info, MEMS_ClearFaults, local_echo, NULL);
    return mems_clear_faults(info);
}

bool mems_manager_family_reset_adjustments(mems_info* info,
                                           mems_manager_family family,
                                           bool local_echo)
{
    if (!family_session(info, family))
        return false;
    if (mems_manager_family_uses_kline19(family))
        return mems_command_kline19(info, 0x0Fu, local_echo, NULL);
    return mems_reset_adjustments(info); /* Manager's original 0x0F */
}

bool mems_manager_family_reset_ecu(mems_info* info,
                                   mems_manager_family family,
                                   bool local_echo)
{
    if (!family_session(info, family))
        return false;
    if (mems_manager_family_uses_kline19(family))
        return mems_command_kline19(info, 0xFAu, local_echo, NULL);
    return mems_reset_ECU(info); /* Manager's original 0xFA */
}

bool mems_manager_family_save_adjustments(mems_info* info,
                                          mems_manager_family family,
                                          bool local_echo, uint8_t* reply)
{
    if (!family_session(info, family))
        return false;
    if (mems_manager_family_uses_kline19(family))
        return mems_command_kline19(info, MEMS_Save, local_echo, reply);
    return mems_test_actuator(info, (actuator_cmd)MEMS_Save, reply);
}

bool mems_manager_family_actuator(mems_info* info,
                                  mems_manager_family family,
                                  actuator_cmd command, bool local_echo,
                                  uint8_t* reply)
{
    if (!family_session(info, family))
        return false;
    if (mems_manager_family_uses_kline19(family))
        return mems_command_kline19(info, (uint8_t)command, local_echo, reply);
    return mems_test_actuator(info, command, reply);
}
