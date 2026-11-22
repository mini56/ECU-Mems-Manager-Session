#ifndef MEMS_MANAGER_TRANSIENT_INJECTION_H
#define MEMS_MANAGER_TRANSIENT_INJECTION_H

#include "rosco.h"

#ifdef __cplusplus
extern "C" {
#endif

/* AANMP002 transient injection read-only data, copied from the proven
 * Manager injectionramtest.cpp interpretations. Reading RAM in Mode 4 is
 * a distinct transaction owned by Manager; these functions validate and
 * decode received data, without sending any ECU command or switching mode.
 */
typedef struct mems_manager_transient_sample
{
    uint8_t counter_0280;    /* Low byte of the Mode-4 0x40 word. */
    uint8_t previous_x_0281; /* High byte of the Mode-4 0x40 word. */
    bool transient_seen;     /* Only counter_0280 != 0 is observed, no inference. */
} mems_manager_transient_sample;

typedef struct mems_manager_transient_comparison
{
    uint16_t ticks_03c8;
    uint16_t conditional_ticks_026e;
    uint16_t candidate_ticks;
    double milliseconds_03c8;
    double candidate_milliseconds;
    /* Candidate is mathematical only: not proof of an ECU correction. */
} mems_manager_transient_comparison;

/* Exactly F0 14 (mode 3), F0 1E (mode 4), or F0 50 (normal);
 * one optional duplicated 0xF0 from a local-echo adapter is accepted. */
ROSCO_API bool mems_manager_transient_parse_mode_reply(
    const uint8_t* reply, uint16_t count, uint8_t* mode);

/* Strict identity proof before a Mode-4 session: a D1 frame containing
 * AANMP002 with an optional duplicated leading echo. */
ROSCO_API bool mems_manager_transient_is_aanmp002(
    const uint8_t* reply, uint16_t count);

/* Normal 0x80 reply: exactly 29 bytes or 30 with duplicated echo.
 * RPM and battery in decivolts are extracted without unit rounding. */
ROSCO_API bool mems_manager_transient_parse_data80(
    const uint8_t* reply, uint16_t count,
    uint16_t* rpm, uint8_t* battery_decivolts);

/* Strict Mode-4 word reply: [command, low, high] or
 * [command, command, low, high] for a local echo. */
ROSCO_API bool mems_manager_transient_parse_word_reply(
    const uint8_t* reply, uint16_t count,
    uint8_t command, uint16_t* word);

/* Mode-4 block 02, command 0x40: word packs [0281:0280]. */
ROSCO_API bool mems_manager_transient_decode_pair(
    uint16_t word, mems_manager_transient_sample* sample);

/* Preserve Manager's experimental comparison logic:
 * S(026E) = high16(026E * C001); candidate = clamp16(03C8 + S).
 * Tick = 0.002 ms. This is explicitly not a validated correction value. */
ROSCO_API bool mems_manager_transient_compare_ticks(
    uint16_t word_026e, uint16_t word_03c8,
    mems_manager_transient_comparison* out);

#ifdef __cplusplus
}
#endif
#endif
