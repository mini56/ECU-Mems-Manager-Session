#include "mems19_wakeup.h"

/* Exact semantics from Session 205 Mems19Protocol pure helpers.
 * Keep the protocol family-specific; do not reuse Lucas 5AS 0x1C.
 */
bool mems_manager_mems19_slow_bit_high(int index, bool* high)
{
    if (!high || index < 0 || index >= 10)
        return false;
    if (index == 0)
        *high = false;
    else if (index == 9)
        *high = true;
    else
        *high = ((0x16u >> (index - 1)) & 1u) != 0u;
    return true;
}

bool mems_manager_mems19_key_complement(
    const uint8_t* stream, size_t length, uint8_t* complement)
{
    size_t i;
    if (!stream || !complement || length < 3)
        return false;
    for (i = 0; i + 2 < length; ++i) {
        if (stream[i] == 0x55u) {
            *complement = (uint8_t)(~stream[i + 2]);
            return true;
        }
    }
    return false;
}

bool mems_manager_mems19_has_address_confirmation(
    const uint8_t* stream, size_t length)
{
    size_t i;
    if (!stream)
        return false;
    for (i = 0; i < length; ++i)
        if (stream[i] == 0xE9u)
            return true;
    return false;
}
