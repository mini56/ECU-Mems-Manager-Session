#ifndef MEMS_MANAGER_MEMS19_WAKEUP_H
#define MEMS_MANAGER_MEMS19_WAKEUP_H

/* Additive Windows native MEMS 1.9 ISO 9141 slow wake-up.
 * Pure helpers and the physical 5-baud wake are based on exactly
 * Session 205's mems19protocol.h and memsinterface_dispatch.cpp:180-300.
 * Historical mems_info layout and exports 1..100 remain UNCHANGED.
 * No changes to the Manager program; no implicit ECU mutations.
 */
#include "rosco.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum mems_manager_mems19_wakeup_result {
    MEMS_MANAGER_MEMS19_WAKE_OK = 0,
    MEMS_MANAGER_MEMS19_WAKE_INVALID_ARGUMENT = 1,
    MEMS_MANAGER_MEMS19_WAKE_NO_OPEN_PORT = 2,
    MEMS_MANAGER_MEMS19_WAKE_BREAK_FAILED = 3,
    MEMS_MANAGER_MEMS19_WAKE_NO_KEY_BYTES = 4,
    MEMS_MANAGER_MEMS19_WAKE_ACK_FAILED = 5,
    MEMS_MANAGER_MEMS19_WAKE_NO_ADDRESS_CONFIRMATION = 6,
    MEMS_MANAGER_MEMS19_WAKE_CANCELLED = 7,
    MEMS_MANAGER_MEMS19_WAKE_OPEN_FAILED = 8,
    MEMS_MANAGER_MEMS19_WAKE_LINK_INIT_FAILED = 9
} mems_manager_mems19_wakeup_result;

/* Optional thread-safe cancellation predicate, called synchronously on
 * the same worker thread as wake-up. Return true to abort. */
typedef bool (*mems_manager_mems19_cancel_fn)(void* user);

/* 0x16 slow-init address with 10 ISO 9141 bits at 200 ms/bit.
 * This is a PURE function, it never talks to an ECU. */
ROSCO_API bool mems_manager_mems19_slow_bit_high(
    int bit_index, bool* high);

/* Accept arbitrary ISO9141 key bytes after 0x55; invert the SECOND key.
 * 0x55 0x76 0x83 => ACK 0x7C. No hardcoded key pair. PURE. */
ROSCO_API bool mems_manager_mems19_key_complement(
    const uint8_t* stream, size_t length, uint8_t* complement);

/* Address complement of 0x16 is 0xE9. PURE. */
ROSCO_API bool mems_manager_mems19_has_address_confirmation(
    const uint8_t* stream, size_t length);

/* Wake an ALREADY OPEN mems_info serial HANDLE, preserving that exact
 * HANDLE for the subsequent mems_init_link_kline19 call. Never closes/
 * reopens on successful wake, and never calls any ECU tuning command.
 * Requires 9600 8N1, as in current mems_connect.
 * On failure BREAK is cleared; owning caller manages disconnect.
 */
ROSCO_API bool mems_manager_mems19_wakeup(
    mems_info* info, bool* local_echo,
    mems_manager_mems19_wakeup_result* result,
    mems_manager_mems19_cancel_fn cancelled, void* context);

/* OPEN + wake + CA/75/F4/D0 on SAME handle, returning real D0 ECU ID.
 * This is an independent future API, never called by Manager 205 yet.
 * Requires info initialized by mems_init() on its owner thread and
 * disconnected on entry. Fails closed, never pretends a port opened
 * or ACK were enough to identify an ECU.
 */
ROSCO_API bool mems_manager_mems19_connect_identify(
    mems_info* info, const char* device_path, uint8_t* four_byte_ecu_id,
    bool* local_echo, mems_manager_mems19_wakeup_result* result,
    mems_manager_mems19_cancel_fn cancelled, void* context);

#ifdef __cplusplus
}
#endif
#endif
