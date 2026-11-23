#ifndef MEMS_MANAGER_LUCAS5AS_H
#define MEMS_MANAGER_LUCAS5AS_H

/* Independent Lucas 5AS NORMAL-mode ECU API. No Qt, no UI and no
 * EEPROM, pairing or key-secret access. C ABI is additive to the 91
 * existing Session 205-compatible exports.
 * Source parity: Session 205 lucas5asprotocol.h / lucas5asreader.h.
 */
#include "rosco.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    MEMS_MANAGER_LUCAS5AS_MAX_PACKET = 127,
    MEMS_MANAGER_LUCAS5AS_FIELD_BONNET = 1u << 0,
    MEMS_MANAGER_LUCAS5AS_FIELD_IGNITION = 1u << 1,
    MEMS_MANAGER_LUCAS5AS_FIELD_DOORS = 1u << 2,
    MEMS_MANAGER_LUCAS5AS_FIELD_POSSIBLE_BOOT = 1u << 3,
    MEMS_MANAGER_LUCAS5AS_FIELD_FOBS = 1u << 4,
    MEMS_MANAGER_LUCAS5AS_FIELD_SERIAL = 1u << 5,
    MEMS_MANAGER_LUCAS5AS_FIELD_DESCRIPTION = 1u << 6
};

/* All binary fields are raw, not guessed human-readable states:
 * "bit7 high" is not necessarily "door open" for every firmware.
 * No raw packet bytes are discarded by caller: decode takes an input
 * buffer and transport read functions return the exact received bytes.
 * Layout has fixed-width members for a stable x64 C ABI.
 */
typedef struct mems_manager_lucas5as_status {
    uint32_t available_fields;
    uint8_t packet_length;
    uint8_t switch_bit7[4];  /* bonnet, ignition, doors, possible boot */
    uint8_t fob_counters_raw[4];
    uint8_t serial_little_endian[3];
    char unit_description[15];  /* at most 14 printable ASCII + NUL */
    uint8_t description_length;
} mems_manager_lucas5as_status;

typedef struct mems_manager_lucas5as_session mems_manager_lucas5as_session;

/* 0x1C address, ten async bits, 200ms per bit; no serial I/O. */
ROSCO_API bool mems_manager_lucas5as_slow_bit_high(int bit_index,
                                                   bool *high);
ROSCO_API bool mems_manager_lucas5as_has_normal_wake(
    const uint8_t *bytes, size_t length);

/* Normal 0x83 status with first byte = full packet length, 2..127.
 * A complete return frame is required; checksum is not invented.
 */
ROSCO_API bool mems_manager_lucas5as_decode_status(
    const uint8_t *frame, size_t length,
    mems_manager_lucas5as_status *status);
/* Only 0x00 or local echo 0x83 are legitimate prefix bytes. */
ROSCO_API bool mems_manager_lucas5as_extract_status(
    const uint8_t *received, size_t received_length,
    uint8_t *frame, size_t frame_capacity, size_t *frame_length,
    size_t *frame_offset, mems_manager_lucas5as_status *status);
/* Known NORMAL diagnostics: horn 8, LED 9, hazards 13. No transmission. */
ROSCO_API bool mems_manager_lucas5as_output_test_commands(
    int output, uint8_t *start, uint8_t *stop);

/* Real independent Win32 read-only session, not connected to legacy
 * mems_info. It opens a serial port, does the 0x1C five-baud wake,
 * verifies 55 83 76, sends 0x83, and requires a complete status frame
 * before returning success. No EEPROM, PLIP or actuator command sent.
 * Not yet verified with physical 5AS; no extra steps on Manager Connect.
 */
ROSCO_API bool mems_manager_lucas5as_open_normal(
    const char *device_path, mems_manager_lucas5as_session **session);
ROSCO_API bool mems_manager_lucas5as_initial_status(
    const mems_manager_lucas5as_session *session,
    uint8_t *frame, size_t capacity, size_t *written);
ROSCO_API bool mems_manager_lucas5as_read_status(
    mems_manager_lucas5as_session *session,
    uint8_t *frame, size_t capacity, size_t *written);
ROSCO_API void mems_manager_lucas5as_close(
    mems_manager_lucas5as_session *session);

#ifdef __cplusplus
}
#endif
#endif
