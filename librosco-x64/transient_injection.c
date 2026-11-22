#include "transient_injection.h"

#include <string.h>

bool mems_manager_transient_parse_mode_reply(
    const uint8_t* reply, uint16_t count, uint8_t* mode)
{
    uint8_t candidate;
    if (!reply || !mode)
        return false;
    if (count == 2 && reply[0] == 0xF0)
        candidate = reply[1];
    else if (count == 3 && reply[0] == 0xF0 && reply[1] == 0xF0)
        candidate = reply[2];
    else
        return false;
    if (candidate != 0x14 && candidate != 0x1E && candidate != 0x50)
        return false;
    *mode = candidate;
    return true;
}

bool mems_manager_transient_is_aanmp002(
    const uint8_t* reply, uint16_t count)
{
    static const uint8_t expected[8] =
        {'A', 'A', 'N', 'M', 'P', '0', '0', '2'};
    uint16_t i;
    if (!reply || count < 9)
        return false;
    for (i = 1; (uint32_t)i + 8u <= count; ++i)
    {
        if (memcmp(reply + i, expected, 8) != 0)
            continue;
        if (reply[i - 1] == 0xD1 ||
            (i >= 2 && reply[i - 2] == 0xD1))
            return true;
    }
    return false;
}

bool mems_manager_transient_parse_data80(
    const uint8_t* reply, uint16_t count,
    uint16_t* rpm, uint8_t* battery_decivolts)
{
    uint8_t start = 0;
    if (!reply || !rpm)
        return false;
    if (count == 29 && reply[0] == 0x80 && reply[1] == 0x1C)
        start = 0;
    else if (count == 30 && reply[0] == 0x80 &&
             reply[1] == 0x80 && reply[2] == 0x1C)
        start = 1;
    else
        return false;

    *rpm = (uint16_t)(((uint16_t)reply[start + 2] << 8) |
                      (uint16_t)reply[start + 3]);
    if (battery_decivolts)
        *battery_decivolts = reply[start + 9];
    return true;
}

bool mems_manager_transient_parse_word_reply(
    const uint8_t* reply, uint16_t count,
    uint8_t command, uint16_t* word)
{
    uint8_t payload;
    if (!reply || !word)
        return false;
    if (count == 3 && reply[0] == command)
        payload = 1;
    else if (count == 4 && reply[0] == command && reply[1] == command)
        payload = 2;
    else
        return false;
    *word = (uint16_t)((uint16_t)reply[payload] |
                       ((uint16_t)reply[payload + 1] << 8));
    return true;
}

bool mems_manager_transient_decode_pair(
    uint16_t word, mems_manager_transient_sample* sample)
{
    if (!sample)
        return false;
    sample->counter_0280 = (uint8_t)(word & 0xFFu);
    sample->previous_x_0281 = (uint8_t)(word >> 8);
    sample->transient_seen = sample->counter_0280 != 0;
    return true;
}

bool mems_manager_transient_compare_ticks(
    uint16_t word_026e, uint16_t word_03c8,
    mems_manager_transient_comparison* out)
{
    uint32_t product, candidate;
    if (!out)
        return false;
    product = (uint32_t)word_026e * (uint32_t)0xC001u;
    out->conditional_ticks_026e = (uint16_t)(product >> 16);
    candidate = (uint32_t)word_03c8 +
                (uint32_t)out->conditional_ticks_026e;
    out->ticks_03c8 = word_03c8;
    out->candidate_ticks = (uint16_t)(candidate > 0xFFFFu ?
                                      0xFFFFu : candidate);
    out->milliseconds_03c8 = (double)word_03c8 * 0.002;
    out->candidate_milliseconds = (double)out->candidate_ticks * 0.002;
    return true;
}
