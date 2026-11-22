#ifndef MEMS_MANAGER_FAMILY_H
#define MEMS_MANAGER_FAMILY_H

#include "rosco.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Family values are stable ABI integers, not USB identifiers.
 * The caller must supply the ECU family established by a successful session.
 * Cable discovery and the 5-baud MEMS 1.9 wake-up remain in the proven
 * application's transport; this library never guesses the family.
 */
typedef enum mems_manager_family
{
    MEMS_MANAGER_FAMILY_12 = 12,
    MEMS_MANAGER_FAMILY_13 = 13,
    MEMS_MANAGER_FAMILY_16 = 16,
    MEMS_MANAGER_FAMILY_19 = 19
} mems_manager_family;

/* No COM access for these two queries. */
ROSCO_API bool mems_manager_family_supported(mems_manager_family family);
ROSCO_API bool mems_manager_family_uses_kline19(mems_manager_family family);

/* Operates on an already opened, verified ECU connection.
 * For MEMS 1.9, the external ISO9141 5-baud wake-up must ALREADY be complete.
 * K-Line adapters may produce an additional local echo.
 * Legacy 1.2/1.3/1.6 ignore local_echo and keep the original ROSCO path.
 */
ROSCO_API bool mems_manager_family_init_link(
    mems_info* info, mems_manager_family family, uint8_t* four_byte_id,
    bool local_echo);
ROSCO_API bool mems_manager_family_read(
    mems_info* info, mems_manager_family family, mems_data* data,
    bool local_echo);
ROSCO_API bool mems_manager_family_read_iac(
    mems_info* info, mems_manager_family family, uint8_t* position,
    bool local_echo);
ROSCO_API bool mems_manager_family_move_iac(
    mems_info* info, mems_manager_family family, uint8_t desired_position,
    bool local_echo);

/* Explicitly requested commands only, never run during autodetection.
 * Caller owns diagnostic-mode verification and permission to mutate ECU.
 * No automatic write, reprogramming, or reset occurs at connect time.
 */
ROSCO_API bool mems_manager_family_clear_faults(
    mems_info* info, mems_manager_family family, bool local_echo);
ROSCO_API bool mems_manager_family_reset_adjustments(
    mems_info* info, mems_manager_family family, bool local_echo);
ROSCO_API bool mems_manager_family_reset_ecu(
    mems_info* info, mems_manager_family family, bool local_echo);
ROSCO_API bool mems_manager_family_save_adjustments(
    mems_info* info, mems_manager_family family, bool local_echo,
    uint8_t* reply);
ROSCO_API bool mems_manager_family_actuator(
    mems_info* info, mems_manager_family family, actuator_cmd command,
    bool local_echo, uint8_t* reply);

#ifdef __cplusplus
}
#endif

#endif /* MEMS_MANAGER_FAMILY_H */
