#ifndef MEMS_MANAGER_SERIAL_ADAPTERS_H
#define MEMS_MANAGER_SERIAL_ADAPTERS_H
#include "rosco.h"
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
/* ABI v1: fixed-size strings, UTF-8, no Qt types or borrowed pointers.
 * USB chipset identification is NOT proof of physical cable or ECU type.
 * Enumeration is read-only and never opens a COM port. */
typedef enum mems_manager_adapter_family {
    MEMS_MANAGER_ADAPTER_SERIAL=0, MEMS_MANAGER_ADAPTER_FTDI=1,
    MEMS_MANAGER_ADAPTER_PROLIFIC=2, MEMS_MANAGER_ADAPTER_WCH=3,
    MEMS_MANAGER_ADAPTER_CP210X=4, MEMS_MANAGER_ADAPTER_USB_OTHER=5,
    MEMS_MANAGER_ADAPTER_CONFIGURED=6
} mems_manager_adapter_family;
typedef struct mems_manager_serial_adapter {
    char port[16];
    char device_path[24];
    char description[256];
    char manufacturer[128];
    uint16_t vendor_id,product_id;
    uint8_t has_vendor_id,has_product_id,detected_by_system,prefers_kline_first;
    uint32_t family;
} mems_manager_serial_adapter;
/* Pure classification; K-Line preference is only the confirmed CH340
 * VID/PID hint from Session 205. All alternate protocols remain possible. */
ROSCO_API mems_manager_adapter_family mems_manager_detect_adapter_family(
    uint16_t vid,bool has_vid,uint16_t pid,bool has_pid,
    const char *manufacturer,const char *description,bool *prefer_kline);
/* Snapshot ordered preferred port first, then ascending COM number.
 * A syntactically valid saved port absent from Windows is a manual fallback.
 * out=NULL/capacity=0 queries count. On short capacity writes first entries
 * and returns total count, allowing caller to allocate and retry. False means
 * enumeration failed or invalid args, not "no adapter". No writes to system. */
ROSCO_API bool mems_manager_list_serial_adapters(const char *preferred_device,
    mems_manager_serial_adapter *out,size_t capacity,size_t *total);
/* Fresh OS snapshot lookup by COM or Win32 path; absent port returns false. */
ROSCO_API bool mems_manager_get_adapter_details(const char *device,
    mems_manager_serial_adapter *out);
#ifdef __cplusplus
}
#endif
#endif
