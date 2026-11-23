#include "lucas5as.h"
#include <string.h>

/* Faithful, Qt-free port of the pure normal-diagnostic protocol in
 * Session 205 lucas5asprotocol.h. These routines send NO ECU commands.
 * This is NOT an EEPROM/PLIP/key-learning implementation.
 */
bool mems_manager_lucas5as_slow_bit_high(int bit_index, bool *high)
{
    if (!high || bit_index < 0 || bit_index > 9)
        return false;
    if (bit_index == 0)
        *high = false; /* Start bit low */
    else if (bit_index == 9)
        *high = true;  /* Stop bit high */
    else
        *high = ((0x1Cu >> (bit_index - 1)) & 1u) != 0u;
    return true;
}

bool mems_manager_lucas5as_has_normal_wake(const uint8_t *bytes,
                                           size_t length)
{
    size_t index;
    if (!bytes || length < 3)
        return false;
    for (index = 0; index + 2 < length; ++index)
        if (bytes[index] == 0x55u && bytes[index + 1] == 0x83u &&
            bytes[index + 2] == 0x76u)
            return true;
    return false;
}

bool mems_manager_lucas5as_decode_status(
    const uint8_t *frame, size_t length,
    mems_manager_lucas5as_status *status)
{
    mems_manager_lucas5as_status decoded;
    size_t i;
    if (!frame || !status ||
        length < 2 || length > MEMS_MANAGER_LUCAS5AS_MAX_PACKET ||
        frame[0] != length)
        return false;
    memset(&decoded, 0, sizeof(decoded));
    decoded.packet_length = (uint8_t)length;
    decoded.available_fields |= MEMS_MANAGER_LUCAS5AS_FIELD_BONNET;
    decoded.switch_bit7[0] = frame[1] & 0x80u;
    if (length > 6) {
        decoded.available_fields |= MEMS_MANAGER_LUCAS5AS_FIELD_IGNITION;
        decoded.switch_bit7[1] = frame[6] & 0x80u;
    }
    if (length > 9) {
        decoded.available_fields |= MEMS_MANAGER_LUCAS5AS_FIELD_DOORS;
        decoded.switch_bit7[2] = frame[9] & 0x80u;
    }
    if (length > 11) {
        decoded.available_fields |= MEMS_MANAGER_LUCAS5AS_FIELD_POSSIBLE_BOOT;
        decoded.switch_bit7[3] = frame[11] & 0x80u;
    }
    if (length > 19) {
        memcpy(decoded.fob_counters_raw, frame + 16, 4);
        decoded.available_fields |= MEMS_MANAGER_LUCAS5AS_FIELD_FOBS;
    }
    if (length > 22) {
        memcpy(decoded.serial_little_endian, frame + 20, 3);
        decoded.available_fields |= MEMS_MANAGER_LUCAS5AS_FIELD_SERIAL;
    }
    if (length > 35) {
        /* Source's documented example contains 14 printable bytes
         * starting at offset 35. Do not interpret arbitrary trailer. */
        const size_t end = length < 49 ? length : 49;
        for (i = 35; i < end; ++i) {
            if (frame[i] < 0x20u || frame[i] > 0x7Eu)
                break;
            decoded.unit_description[decoded.description_length++] =
                (char)frame[i];
        }
        if (decoded.description_length)
            decoded.available_fields |=
                MEMS_MANAGER_LUCAS5AS_FIELD_DESCRIPTION;
    }
    *status = decoded;
    return true;
}

bool mems_manager_lucas5as_extract_status(
    const uint8_t *received, size_t received_length,
    uint8_t *frame, size_t frame_capacity, size_t *frame_length,
    size_t *frame_offset, mems_manager_lucas5as_status *status)
{
    size_t offset = 0;
    size_t n;
    mems_manager_lucas5as_status decoded;
    if (!received || !frame || !frame_length || !frame_offset || !status)
        return false;
    /* The source does not permit scanning arbitrary bytes for an
     * apparent tiny length. Only skip known 0x00/0x83 local echo. */
    while (offset < received_length &&
           (received[offset] == 0x00u || received[offset] == 0x83u))
        ++offset;
    if (offset >= received_length)
        return false;
    n = received[offset];
    if (n < 2 || n > MEMS_MANAGER_LUCAS5AS_MAX_PACKET ||
        received_length - offset < n || frame_capacity < n)
        return false;
    if (!mems_manager_lucas5as_decode_status(received + offset, n, &decoded))
        return false;
    /* On success only, publish output and preserve all raw bytes. */
    memcpy(frame, received + offset, n);
    *frame_length = n;
    *frame_offset = offset;
    *status = decoded;
    return true;
}

bool mems_manager_lucas5as_output_test_commands(
    int output, uint8_t *start, uint8_t *stop)
{
    if (!start || !stop || (output != 8 && output != 9 && output != 13))
        return false;
    *start = (uint8_t)(0x30u | (unsigned)output);
    *stop = (uint8_t)(0x20u | (unsigned)output);
    return true;
}
