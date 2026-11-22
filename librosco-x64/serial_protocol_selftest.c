/*
 * Offline transport regression test. Compiles the actual protocol.c unchanged,
 * intercepting only Win32 serial I/O inside this standalone TEST executable.
 * This test cannot communicate with a real ECU or a COM device.
 */
#include "rosco.h"
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint8_t receive[2048];
    size_t receive_count;
    size_t receive_index;
    uint8_t sent[256];
    size_t sent_count;
    uint8_t iac_position;
    bool connected;
    bool local_echo;
    bool bad_echo;
    bool no_reply;
    bool short_frame_7d;
} mock_session;

static mock_session g;
static int failures = 0;

static void mock_reset(void)
{
    memset(&g, 0, sizeof(g));
    g.connected = true;
    g.iac_position = 10;
}
static void append_receive(const void* ptr, size_t length)
{
    if (g.receive_count + length > sizeof(g.receive)) {
        fprintf(stderr, "Mock receive queue overflow\n");
        ++failures;
        return;
    }
    memcpy(g.receive + g.receive_count, ptr, length);
    g.receive_count += length;
}
static void append_byte(uint8_t v) { append_receive(&v, 1); }

static BOOL WINAPI FakeReadFile(HANDLE h, LPVOID dest, DWORD requested,
                                LPDWORD actual, LPOVERLAPPED asynchronous)
{
    size_t available;
    size_t n;
    (void)h;
    (void)asynchronous;
    if (!actual || !dest)
        return FALSE;
    available = g.receive_count - g.receive_index;
    n = requested < available ? requested : available;
    /* Exercise partial ReadFile() success, not only ideal full reads. */
    if (n > 4)
        n = 4;
    if (n)
        memcpy(dest, g.receive + g.receive_index, n);
    g.receive_index += n;
    *actual = (DWORD)n;
    return TRUE;
}
static BOOL WINAPI FakeWriteFile(HANDLE h, LPCVOID data, DWORD length,
                                 LPDWORD actual, LPOVERLAPPED asynchronous)
{
    const uint8_t* bytes = (const uint8_t*)data;
    DWORD i;
    (void)h;
    (void)asynchronous;
    if (!actual || !bytes || g.sent_count + length > sizeof(g.sent))
        return FALSE;
    for (i = 0; i < length; ++i) {
        const uint8_t cmd = bytes[i];
        uint8_t d0[4] = {0x11, 0x22, 0x33, 0x44};
        mems_data_frame_80 frame80;
        mems_data_frame_7d frame7d;

        g.sent[g.sent_count++] = cmd;
        if (g.local_echo)
            append_byte(cmd);
        append_byte(g.bad_echo ? (uint8_t)(cmd ^ 0xFFu) : cmd);

        if (g.no_reply)
            continue;
        switch (cmd) {
        case 0xCA: case 0x75: break;
        case 0xF4: append_byte(0); break;
        case 0xD0: append_receive(d0, sizeof(d0)); break;
        case 0x80:
            memset(&frame80, 0, sizeof(frame80));
            frame80.bytes_in_frame = (uint8_t)sizeof(frame80);
            frame80.engine_rpm_hi = 0x12;
            frame80.engine_rpm_lo = 0x34;
            frame80.coolant_temp = 79;
            frame80.dtc0 = 0x03;
            frame80.dtc1 = 0x82;
            frame80.iac_position = g.iac_position;
            append_receive(&frame80, sizeof(frame80));
            break;
        case 0x7D:
            memset(&frame7d, 0, sizeof(frame7d));
            frame7d.bytes_in_frame = (uint8_t)sizeof(frame7d);
            frame7d.lambda_voltage = 47;
            frame7d.long_term_fuel_trim = 135;
            append_receive(&frame7d, g.short_frame_7d ? 6 : sizeof(frame7d));
            break;
        case 0xFB: append_byte(g.iac_position); break;
        case 0xFD: if (g.iac_position < IAC_MAXIMUM) ++g.iac_position;
                   append_byte(g.iac_position); break;
        case 0xFE: if (g.iac_position) --g.iac_position;
                   append_byte(g.iac_position); break;
        default: append_byte(0x55); break;
        }
    }
    *actual = length;
    return TRUE;
}
static DWORD WINAPI FakeWaitForSingleObject(HANDLE object, DWORD milliseconds)
{
    (void)object;
    (void)milliseconds;
    return WAIT_OBJECT_0;
}
static BOOL WINAPI FakeReleaseMutex(HANDLE object)
{
    (void)object;
    return TRUE;
}

bool mems_is_connected(mems_info* info)
{
    return info != NULL && g.connected;
}

/* No modification to the production protocol file or its public functions. */
#define ReadFile FakeReadFile
#define WriteFile FakeWriteFile
#define WaitForSingleObject FakeWaitForSingleObject
#define ReleaseMutex FakeReleaseMutex
#include "protocol.c"
#undef ReadFile
#undef WriteFile
#undef WaitForSingleObject
#undef ReleaseMutex

/* Additive library modules are tested against the SAME synthetic
 * session/serial transport used by the historical regression tests. */
#include "family.c"
#include "transient_injection.c"

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "FAILED line %d: %s\n", __LINE__, #expr); ++failures; \
} } while (0)

typedef bool (*reply_operation)(mems_info*, uint8_t*);
typedef bool (*plain_operation)(mems_info*);
typedef struct {
    const char* name;
    reply_operation operation;
    uint8_t command;
} operation_with_reply;
typedef struct {
    const char* name;
    plain_operation operation;
    uint8_t command;
} operation_plain;

int main(void)
{
    mems_info info;
    mems_data_frame_80 frame80;
    mems_data_frame_7d frame7d;
    mems_data data;
    uint8_t reply = 0;
    uint8_t id[4] = {0};
    uint8_t pos = 0;
    size_t i;
    const operation_with_reply commands[] = {
        {"fuel_trim_plus", mems_fuel_trim_plus, 0x79},
        {"fuel_trim_minus", mems_fuel_trim_minus, 0x7A},
        {"idle_decay_plus", mems_idle_decay_plus, 0x89},
        {"idle_decay_minus", mems_idle_decay_minus, 0x8A},
        {"idle_speed_plus", mems_idle_speed_plus, 0x91},
        {"idle_speed_minus", mems_idle_speed_minus, 0x92},
        {"ignition_advance_plus", mems_ignition_advance_plus, 0x93},
        {"ignition_advance_minus", mems_ignition_advance_minus, 0x94},
        {"purge_on", mems_Purge_Valve_On, 0x18},
        {"purge_off", mems_Purge_Valve_Off, 0x08},
        {"oxygen_heater_on", mems_O2Heater_On, 0x19},
        {"oxygen_heater_off", mems_O2Heater_Off, 0x09},
        {"boost_valve_on", mems_Boost_Valve_On, 0x1B},
        {"boost_valve_off", mems_Boost_Valve_Off, 0x0B},
        {"fan1_on", mems_Fan1_On, 0x1D},
        {"fan1_off", mems_Fan1_Off, 0x0D},
        {"fan2_on", mems_Fan2_On, 0x1E},
        {"fan2_off", mems_Fan2_Off, 0x0E},
        {"iac_open", mems_OpenIAC, 0xFD},
        {"iac_close", mems_CloseIAC, 0xFE},
        {"additional_fuel_trim_plus", mems_fuel_trim_2_plus, 0x7B},
        {"additional_fuel_trim_minus", mems_fuel_trim_2_minus, 0x7C}
    };
    const operation_plain requests[] = {
        {"clear_faults", mems_clear_faults, 0xCC},
        {"reset_ADJ", mems_reset_ADJ, 0xFA},
        {"clear_computed_and_learned_params_Manager_reset_ECU", mems_reset_ECU, 0xFA},
        {"reset_EMI", mems_reset_EMI, 0x0F},
        {"save", mems_Save, 0x81},
        {"reset_user_adjustable_settings_Manager", mems_reset_adjustments, 0x0F},
        {"reset_ECU_leopold_explicit", mems_reset_ECU_leopold, 0xAF},
        {"save_adjustments_alias", mems_save_adjustments, 0x81},
        {"heartbeat", mems_heartbeat, 0xF4}
    };

    memset(&info, 0, sizeof(info));
    info.sd = (HANDLE)(uintptr_t)1;
    info.mutex = (HANDLE)(uintptr_t)2;

    for (i = 0; i < sizeof(commands) / sizeof(commands[0]); ++i) {
        mock_reset();
        reply = 0;
        CHECK(commands[i].operation(&info, &reply));
        CHECK(g.sent_count == 1 && g.sent[0] == commands[i].command);
        CHECK(reply == 0x55 || commands[i].command == 0xFD || commands[i].command == 0xFE);
    }
    for (i = 0; i < sizeof(requests) / sizeof(requests[0]); ++i) {
        mock_reset();
        CHECK(requests[i].operation(&info));
        CHECK(g.sent_count == 1 && g.sent[0] == requests[i].command);
        CHECK(g.receive_index == g.receive_count);
    }

    mock_reset();
    CHECK(mems_init_link(&info, id));
    CHECK(g.sent_count == 4 && g.sent[0] == 0xCA && g.sent[1] == 0x75
          && g.sent[2] == 0xF4 && g.sent[3] == 0xD0);
    CHECK(id[0] == 0x11 && id[3] == 0x44);

    mock_reset();
    memset(&frame80, 0, sizeof(frame80));
    memset(&frame7d, 0, sizeof(frame7d));
    CHECK(mems_read_raw(&info, &frame80, &frame7d));
    CHECK(g.sent_count == 2 && g.sent[0] == 0x80 && g.sent[1] == 0x7D);
    CHECK(frame80.bytes_in_frame == 28 && frame7d.bytes_in_frame == 32);
    CHECK(frame80.engine_rpm_hi == 0x12 && frame7d.lambda_voltage == 47);

    mock_reset();
    memset(&data, 0, sizeof(data));
    CHECK(mems_read(&info, &data));
    CHECK(data.engine_rpm == 0x1234);
    CHECK(data.fault_codes == 0x0F);
    CHECK(data.lambda_voltage == 47);

    mock_reset();
    g.short_frame_7d = true;
    CHECK(!mems_read_raw(&info, &frame80, &frame7d));

    mock_reset();
    CHECK(mems_read_iac_position(&info, &pos));
    CHECK(pos == 10);
    mock_reset();
    CHECK(mems_move_iac(&info, 11));
    CHECK(g.sent_count == 2 && g.sent[0] == 0xFB && g.sent[1] == 0xFD);
    CHECK(g.iac_position == 11);

    mock_reset();
    CHECK(mems_command_kline19(&info, 0xCC, false, &reply));
    CHECK(reply == 0x55 && g.sent_count == 1 && g.sent[0] == 0xCC);
    mock_reset();
    g.local_echo = true;
    CHECK(mems_command_kline19(&info, 0xCC, true, &reply));
    CHECK(reply == 0x55);

    mock_reset();
    g.local_echo = true;
    CHECK(mems_init_link_kline19(&info, id, true));
    CHECK(g.sent_count == 4 && g.sent[0] == 0xCA && g.sent[3] == 0xD0);
    CHECK(id[0] == 0x11 && id[3] == 0x44);
    mock_reset();
    g.local_echo = true;
    CHECK(mems_read_kline19(&info, &data, true));
    CHECK(data.engine_rpm == 0x1234 && data.lambda_voltage == 47);

    mock_reset();
    g.local_echo = true;
    CHECK(mems_move_iac_kline19(&info, 11, true));
    CHECK(g.sent_count == 2 && g.sent[0] == 0xFB && g.sent[1] == 0xFD);

    mock_reset();
    g.no_reply = true;
    CHECK(!mems_Save(&info));
    CHECK(!mems_reset_ADJ(&info));
    mock_reset();
    g.bad_echo = true;
    CHECK(!mems_Save(&info));
    mock_reset();
    g.connected = false;
    CHECK(!mems_clear_faults(&info));
    CHECK(!mems_test_actuator(&info, MEMS_Fan1On, &reply));

    CHECK(temperature_value_to_degrees_f(55) == 32);
    CHECK(temperature_value_to_degrees_f(65) == 50);
    CHECK(temperature_value_to_degrees_f(0) == 137);


    /* Exhaust the historical command wrappers on negative responses,
     * not just the success path. No ECU or COM port is ever opened. */
    for (i = 0; i < sizeof(commands) / sizeof(commands[0]); ++i) {
        mock_reset();
        g.bad_echo = true;
        reply = 0xAA;
        CHECK(!commands[i].operation(&info, &reply));
        CHECK(g.sent_count == 1 && g.sent[0] == commands[i].command);
        CHECK(reply == 0xAA);
        mock_reset();
        g.no_reply = true;
        CHECK(!commands[i].operation(&info, &reply));
        CHECK(g.sent_count == 1 && g.sent[0] == commands[i].command);
    }
    for (i = 0; i < sizeof(requests) / sizeof(requests[0]); ++i) {
        mock_reset();
        g.bad_echo = true;
        CHECK(!requests[i].operation(&info));
        CHECK(g.sent_count == 1 && g.sent[0] == requests[i].command);
        mock_reset();
        g.no_reply = true;
        CHECK(!requests[i].operation(&info));
        CHECK(g.sent_count == 1 && g.sent[0] == requests[i].command);
    }
    mock_reset();
    g.bad_echo = true;
    CHECK(!mems_init_link(&info, id));
    mock_reset();
    g.no_reply = true;
    CHECK(!mems_init_link(&info, id));
    mock_reset();
    g.bad_echo = true;
    CHECK(!mems_read_raw(&info, &frame80, &frame7d));
    mock_reset();
    g.local_echo = true;
    g.bad_echo = true;
    CHECK(!mems_command_kline19(&info, 0xCC, true, &reply));
    mock_reset();
    g.local_echo = true;
    g.no_reply = true;
    CHECK(!mems_read_kline19(&info, &data, true));
    mock_reset();
    g.local_echo = true;
    g.short_frame_7d = true;
    CHECK(!mems_read_kline19(&info, &data, true));
    mock_reset();
    CHECK(mems_move_iac(&info, 10));
    CHECK(g.sent_count == 1 && g.sent[0] == 0xFB);
    mock_reset();
    CHECK(mems_move_iac_kline19(&info, 10, false));
    CHECK(g.sent_count == 1 && g.sent[0] == 0xFB);
    mock_reset();
    g.connected = false;
    CHECK(!mems_send_command(&info, 0xCC));
    CHECK(!mems_init_link_kline19(&info, id, false));
    CHECK(!mems_command_kline19(&info, 0xCC, false, &reply));
    CHECK(!mems_read_kline19(&info, &data, false));
    mock_reset();
    CHECK(!mems_clear_faults(NULL));
    CHECK(!mems_reset_ADJ(NULL));
    CHECK(!mems_reset_EMI(NULL));
    CHECK(!mems_reset_ECU(NULL));
    CHECK(!mems_reset_ECU_leopold(NULL));
    CHECK(!mems_Save(NULL));
    CHECK(!mems_read(NULL, &data));
    CHECK(!mems_read(&info, NULL));


    /* Family routes preserve the working three-pin ROSCO 1.x byte sequences;
     * MEMS 1.9 has its already-open K-Line connection and optional cable echo.
     */
    {
        const mems_manager_family families[] = {
            MEMS_MANAGER_FAMILY_12, MEMS_MANAGER_FAMILY_13,
            MEMS_MANAGER_FAMILY_16, MEMS_MANAGER_FAMILY_19
        };
        size_t n;
        CHECK(!mems_manager_family_supported((mems_manager_family)0));
        CHECK(mems_manager_family_uses_kline19(MEMS_MANAGER_FAMILY_19));
        CHECK(!mems_manager_family_uses_kline19(MEMS_MANAGER_FAMILY_16));
        for (n = 0; n < sizeof(families)/sizeof(families[0]); ++n)
        {
            const mems_manager_family family = families[n];
            const bool echo = family == MEMS_MANAGER_FAMILY_19;
            uint8_t family_id[4] = {0};
            uint8_t family_reply = 0;
            mems_data family_data;

            CHECK(mems_manager_family_supported(family));

            mock_reset();
            g.local_echo = echo;
            CHECK(mems_manager_family_init_link(&info, family, family_id, echo));
            CHECK(g.sent_count == 4 && g.sent[0] == 0xCA &&
                  g.sent[1] == 0x75 && g.sent[2] == 0xF4 &&
                  g.sent[3] == 0xD0);
            CHECK(family_id[0] == 0x11 && family_id[3] == 0x44);

            mock_reset();
            g.local_echo = echo;
            memset(&family_data, 0, sizeof(family_data));
            CHECK(mems_manager_family_read(&info, family, &family_data, echo));
            CHECK(g.sent_count == 2 && g.sent[0] == 0x80 && g.sent[1] == 0x7D);
            CHECK(family_data.engine_rpm == 0x1234 &&
                  family_data.lambda_voltage == 47);

            mock_reset();
            g.local_echo = echo;
            CHECK(mems_manager_family_read_iac(&info, family, &family_reply, echo));
            CHECK(g.sent_count == 1 && g.sent[0] == 0xFB &&
                  family_reply == 10);

            mock_reset();
            g.local_echo = echo;
            CHECK(mems_manager_family_move_iac(&info, family, 11, echo));
            CHECK(g.sent_count == 2 && g.sent[0] == 0xFB &&
                  g.sent[1] == 0xFD && g.iac_position == 11);

            mock_reset();
            g.local_echo = echo;
            CHECK(mems_manager_family_clear_faults(&info, family, echo));
            CHECK(g.sent_count == 1 && g.sent[0] == 0xCC);

            mock_reset();
            g.local_echo = echo;
            CHECK(mems_manager_family_reset_adjustments(&info, family, echo));
            CHECK(g.sent_count == 1 && g.sent[0] == 0x0F);

            mock_reset();
            g.local_echo = echo;
            CHECK(mems_manager_family_reset_ecu(&info, family, echo));
            CHECK(g.sent_count == 1 && g.sent[0] == 0xFA);

            mock_reset();
            g.local_echo = echo;
            family_reply = 0xFF;
            CHECK(mems_manager_family_save_adjustments(
                      &info, family, echo, &family_reply));
            CHECK(g.sent_count == 1 && g.sent[0] == 0x81 &&
                  family_reply == 0x55);

            mock_reset();
            g.local_echo = echo;
            family_reply = 0;
            CHECK(mems_manager_family_actuator(
                      &info, family, MEMS_Fan1On, echo, &family_reply));
            CHECK(g.sent_count == 1 && g.sent[0] == 0x1D &&
                  family_reply == 0x55);

            mock_reset();
            CHECK(!mems_manager_family_read(NULL, family, &family_data, echo));
            CHECK(!mems_manager_family_read(&info, family, NULL, echo));
            CHECK(!mems_manager_family_clear_faults(
                      &info, (mems_manager_family)0, echo));
            CHECK(g.sent_count == 0);

            mock_reset();
            g.connected = false;
            CHECK(!mems_manager_family_init_link(&info, family, family_id, echo));
            CHECK(!mems_manager_family_reset_ecu(&info, family, echo));
            CHECK(g.sent_count == 0);

            mock_reset();
            g.local_echo = echo;
            g.bad_echo = true;
            CHECK(!mems_manager_family_clear_faults(&info, family, echo));
        }
    }
    /* AANMP002 transient injection: exact parsing only, NO Mode-4 command,
     * serial write, ECU mutation or unverifiable correction claim. */
    {
        const uint8_t validId[] = {0xD1,'A','A','N','M','P','0','0','2'};
        const uint8_t doubledId[] = {0xD1,0xD1,'A','A','N','M','P','0','0','2'};
        const uint8_t goodMode[] = {0xF0,0xF0,0x1E};
        const uint8_t badMode[] = {0xF0,0xF0,0xFE};
        const uint8_t goodWord[] = {0x40,0x40,0x07,0x52};
        const uint8_t badWord[] = {0x40,0x41,0x07,0x52};
        uint8_t frame[29] = {0};
        uint8_t mode = 0;
        uint8_t battery = 0;
        uint16_t rpm = 0;
        uint16_t word = 0;
        mems_manager_transient_sample sample;
        mems_manager_transient_comparison comparison;

        CHECK(mems_manager_transient_is_aanmp002(validId, sizeof(validId)));
        CHECK(mems_manager_transient_is_aanmp002(doubledId, sizeof(doubledId)));
        CHECK(!mems_manager_transient_is_aanmp002(validId, 4));
        CHECK(mems_manager_transient_parse_mode_reply(
                 goodMode, sizeof(goodMode), &mode) && mode == 0x1E);
        CHECK(!mems_manager_transient_parse_mode_reply(
                 badMode, sizeof(badMode), &mode));

        frame[0] = 0x80;
        frame[1] = 0x1C;
        frame[2] = 0x12;
        frame[3] = 0x34;
        frame[9] = 119;
        CHECK(mems_manager_transient_parse_data80(
                  frame, sizeof(frame), &rpm, &battery) &&
              rpm == 0x1234 && battery == 119);
        CHECK(!mems_manager_transient_parse_data80(
                   frame, 28, &rpm, &battery));

        CHECK(mems_manager_transient_parse_word_reply(
                  goodWord, sizeof(goodWord), 0x40, &word) &&
              word == 0x5207);
        CHECK(!mems_manager_transient_parse_word_reply(
                   badWord, sizeof(badWord), 0x40, &word));
        CHECK(mems_manager_transient_decode_pair(word, &sample));
        CHECK(sample.counter_0280 == 7 && sample.previous_x_0281 == 82 &&
              sample.transient_seen);
        CHECK(mems_manager_transient_decode_pair(0x1200, &sample));
        CHECK(!sample.transient_seen && sample.previous_x_0281 == 18);
        CHECK(mems_manager_transient_compare_ticks(0, 1000, &comparison));
        CHECK(comparison.conditional_ticks_026e == 0 &&
              comparison.ticks_03c8 == 1000 &&
              comparison.candidate_ticks == 1000 &&
              comparison.milliseconds_03c8 > 1.999 &&
              comparison.milliseconds_03c8 < 2.001);
        CHECK(mems_manager_transient_compare_ticks(65535, 65535, &comparison));
        CHECK(comparison.candidate_ticks == 65535);
        CHECK(!mems_manager_transient_compare_ticks(0, 10, NULL));
        /* Pure parsing cannot send any commands. */
    }

    if (failures) {
        fprintf(stderr, "SIMULATED_SERIAL_CONTRACT_FAIL: %d failed checks\n", failures);
        return 1;
    }
    printf("SIMULATED_SERIAL_CONTRACT_PASS: 20 historical setting/actuator commands + 2 additive; "
           "9 clear/reset/save/heartbeat commands; normal & K-Line handshake, frame lengths, "
           "partial reads, IAC, family routing, AANMP002 read-only decoding and negative responses\n");
    return 0;
}
