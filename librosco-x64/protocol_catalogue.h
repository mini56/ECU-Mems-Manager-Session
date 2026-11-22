#ifndef MEMS_MANAGER_PROTOCOL_CATALOGUE_H
#define MEMS_MANAGER_PROTOCOL_CATALOGUE_H
/* Standalone, Qt-free protocol codecs. No COM access or ECU commands.
 * Wire contracts cross-checked against Manager SESSION196 and RoverMEMS
 * documentation; no RoverMEMS Go source has been copied here.
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#if defined(_WIN32) && defined(ROSCO_BUILD_DLL)
# define MEMS_CODEC_API __declspec(dllexport)
#elif defined(_WIN32) && defined(ROSCO_USE_DLL)
# define MEMS_CODEC_API __declspec(dllimport)
#else
# define MEMS_CODEC_API
#endif
#ifdef __cplusplus
extern "C" {
#endif

#define MEMS_CODEC_MAX_PAYLOAD 96u
#define MEMS_CODEC_MAX_METRICS 6u
typedef enum mems_codec_response {
    MEMS_RESPONSE_INVALID=0,
    MEMS_RESPONSE_POSITIVE=1,
    MEMS_RESPONSE_NEGATIVE=2,
    MEMS_RESPONSE_PENDING=3
} mems_codec_response;
typedef enum mems_codec_field {
    MEMS_FIELD_COOLANT_C=1, MEMS_FIELD_OIL_C=2, MEMS_FIELD_INTAKE_C=3,
    MEMS_FIELD_FUEL_RAW=4, MEMS_FIELD_MAP_KPA=5, MEMS_FIELD_THROTTLE_DEG=6,
    MEMS_FIELD_RPM=7, MEMS_FIELD_LAMBDA_FEEDBACK_PERCENT=8,
    MEMS_FIELD_LAMBDA_RAW=9, MEMS_FIELD_COIL_1_RAW=10, MEMS_FIELD_COIL_2_RAW=11,
    MEMS_FIELD_INJECTOR_1_RAW=12, MEMS_FIELD_INJECTOR_2_RAW=13,
    MEMS_FIELD_INJECTOR_3_RAW=14, MEMS_FIELD_INJECTOR_4_RAW=15,
    MEMS_FIELD_SPEED_RAW=16, MEMS_FIELD_THROTTLE_CLOSED=17,
    MEMS_FIELD_IGNITION_ON=18, MEMS_FIELD_AC_REQUESTED=19,
    MEMS_FIELD_BATTERY_V=20, MEMS_FIELD_PRIMARY_SYNC=21,
    MEMS_FIELD_SECONDARY_SYNC=22, MEMS_FIELD_IDLE_STEPS=23,
    MEMS_FIELD_CLOSED_LOOP=24, MEMS_FIELD_RPM_ERROR_SIGNED=25,
    MEMS_FIELD_CAM_RAW=26, MEMS_FIELD_IDLE_OFFSET_RAW10=27,
    MEMS_FIELD_IDLE_TARGET_RPM=28, MEMS_FIELD_THROTTLE_MV=29,
    MEMS_FIELD_COIL_1_US=30, MEMS_FIELD_COIL_2_US=31,
    MEMS_FIELD_RPM_DEVIATION_RAW=32
} mems_codec_field;
typedef struct mems_codec_metric {
    uint16_t field;
    double value;
} mems_codec_metric;
typedef struct mems_codec_measurements {
    uint8_t id;
    uint8_t count;
    mems_codec_metric metrics[MEMS_CODEC_MAX_METRICS];
} mems_codec_measurements;

/* MEMS 2J: [LEN][PAYLOAD][additive checksum]. */
MEMS_CODEC_API bool mems2j_frame_encode(const uint8_t* payload,size_t length,
                                       uint8_t* output,size_t capacity,size_t* written);
MEMS_CODEC_API bool mems2j_frame_decode(const uint8_t* frame,size_t length,
                                       uint8_t* output,size_t capacity,size_t* written);
MEMS_CODEC_API bool mems2j_make_read_request(uint8_t id,uint8_t* output,
                                           size_t capacity,size_t* written);
MEMS_CODEC_API bool mems2j_make_clear_fault_request(uint8_t* output,
                                                   size_t capacity,size_t* written);
MEMS_CODEC_API bool mems2j_make_key_request(uint16_t key,uint8_t* output,
                                          size_t capacity,size_t* written);
MEMS_CODEC_API uint16_t mems2j_seed_key(uint16_t seed);
MEMS_CODEC_API mems_codec_response mems2j_response_status(
    const uint8_t* payload,size_t length,uint8_t request_service,
    uint8_t positive_service,uint8_t expected_id);
MEMS_CODEC_API bool mems2j_decode_measurement(uint8_t id,const uint8_t* payload,
                                             size_t length,mems_codec_measurements* output);
MEMS_CODEC_API bool mems2j_fault_bit(const uint8_t* payload,size_t length,
                                   size_t index,uint8_t mask,bool* active);

/* MEMS3: 9600 8E1, B8 13 F7 LEN PAYLOAD XOR.
 * Reply header may differ; framing validates length and XOR.
 */
MEMS_CODEC_API bool mems3_frame_encode(const uint8_t* payload,size_t length,
                                      uint8_t* output,size_t capacity,size_t* written);
MEMS_CODEC_API bool mems3_frame_decode(const uint8_t* frame,size_t length,
                                      uint8_t* output,size_t capacity,size_t* written);
MEMS_CODEC_API bool mems3_make_read_request(uint8_t id,uint8_t* output,
                                           size_t capacity,size_t* written);
MEMS_CODEC_API mems_codec_response mems3_response_status(
    const uint8_t* payload,size_t length,uint8_t request_service,
    uint8_t positive_service,uint8_t expected_id);
MEMS_CODEC_API bool mems3_decode_measurement(uint8_t id,const uint8_t* payload,
                                            size_t length,mems_codec_measurements* output);
MEMS_CODEC_API size_t mems3_fault_count(const uint8_t* payload,size_t length);
MEMS_CODEC_API bool mems3_fault_at(const uint8_t* payload,size_t length,
                                 size_t index,uint16_t* code,uint8_t* state);

/* RC5 is an SRS/airbag module, never an engine ECU. */
MEMS_CODEC_API bool mems_rc5_make_request(uint8_t command,
                                        uint8_t* output,size_t capacity);
MEMS_CODEC_API bool mems_rc5_validate_frame(const uint8_t* packet,size_t length);
MEMS_CODEC_API size_t mems_rc5_fault_count(const uint8_t* packet,size_t length);
MEMS_CODEC_API bool mems_rc5_fault_at(const uint8_t* packet,size_t length,
                                    size_t index,uint16_t* code,bool* historical);
#ifdef __cplusplus
}
#endif
#endif
