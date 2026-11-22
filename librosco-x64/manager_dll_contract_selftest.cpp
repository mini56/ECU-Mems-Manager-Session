/* ABI contract compiled against the UNMODIFIED header used by Session 196.
 * New DLL-only exports are tested separately by abi_selftest.c.
 * These assertions intentionally use Manager's 0xFA/0x0F constants,
 * never the different names/semantics from the LeopoldG DLL.
 */
#include "../rosco.h"
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <cstdio>

static_assert(sizeof(mems_data_frame_80)==28, "Manager frame80 ABI mismatch");
static_assert(sizeof(mems_data_frame_7d)==32, "Manager frame7d ABI mismatch");
static_assert(sizeof(mems_data)==60, "Manager data ABI mismatch");
static_assert(sizeof(mems_info)==16, "Manager 64-bit handles ABI mismatch");
static_assert(offsetof(mems_data, engine_rpm)==0, "RPM offset mismatch");
static_assert(offsetof(mems_data, fault_codes)==12, "DTC bits offset mismatch");
static_assert(offsetof(mems_data, iac_position)==16, "IAC offset mismatch");
static_assert(offsetof(mems_data, dtc0)==58, "DTC0 offset mismatch");
static_assert(offsetof(mems_data, dtc1)==59, "DTC1 offset mismatch");
static_assert(MEMS_ResetECU==0xFA, "Manager ECU action expects FA");
static_assert(MEMS_ResetAdj==0x0F, "Manager adjustments action expects 0F");
static_assert(MEMS_ClearFaults==0xCC, "Manager clear DTC expects CC");

#define VERIFY_CPP_ABI(name,sig) \
    static_assert(std::is_same<decltype(&name),sig>::value, \
                  #name " Manager-declared function signature changed")
using InfoVoid=void (*)(mems_info*);
using InfoBool=bool (*)(mems_info*);
using Connect=bool (*)(mems_info*,const char*);
using IdOut=bool (*)(mems_info*,uint8_t*);
using Read=bool (*)(mems_info*,mems_data*);
using ReadRaw=bool (*)(mems_info*,mems_data_frame_80*,mems_data_frame_7d*);
using Iac=bool (*)(mems_info*,uint8_t);
using Actuator=bool (*)(mems_info*,actuator_cmd,uint8_t*);
using NameReply=bool (*)(mems_info*,uint8_t*);
using KlineCommand=bool (*)(mems_info*,uint8_t,bool,uint8_t*);
using KlineMove=bool (*)(mems_info*,uint8_t,bool);
using KlineId=bool (*)(mems_info*,uint8_t*,bool);
using KlineRead=bool (*)(mems_info*,mems_data*,bool);
using Version=librosco_version (*)(void);

VERIFY_CPP_ABI(mems_init,InfoVoid);
VERIFY_CPP_ABI(mems_init_link,IdOut);
VERIFY_CPP_ABI(mems_cleanup,InfoVoid);
VERIFY_CPP_ABI(mems_connect,Connect);
VERIFY_CPP_ABI(mems_disconnect,InfoVoid);
VERIFY_CPP_ABI(mems_is_connected,InfoBool);
VERIFY_CPP_ABI(mems_read_raw,ReadRaw);
VERIFY_CPP_ABI(mems_read,Read);
VERIFY_CPP_ABI(mems_command_kline19,KlineCommand);
VERIFY_CPP_ABI(mems_move_iac_kline19,KlineMove);
VERIFY_CPP_ABI(mems_init_link_kline19,KlineId);
VERIFY_CPP_ABI(mems_read_kline19,KlineRead);
VERIFY_CPP_ABI(mems_read_iac_position,IdOut);
VERIFY_CPP_ABI(mems_move_iac,Iac);
VERIFY_CPP_ABI(mems_test_actuator,Actuator);
VERIFY_CPP_ABI(mems_clear_faults,InfoBool);
VERIFY_CPP_ABI(mems_reset_ECU,InfoBool);
VERIFY_CPP_ABI(mems_reset_adjustments,InfoBool);
VERIFY_CPP_ABI(mems_heartbeat,InfoBool);
VERIFY_CPP_ABI(mems_fuel_trim_plus,NameReply);
VERIFY_CPP_ABI(mems_fuel_trim_minus,NameReply);
VERIFY_CPP_ABI(mems_idle_decay_plus,NameReply);
VERIFY_CPP_ABI(mems_idle_decay_minus,NameReply);
VERIFY_CPP_ABI(mems_idle_speed_plus,NameReply);
VERIFY_CPP_ABI(mems_idle_speed_minus,NameReply);
VERIFY_CPP_ABI(mems_ignition_advance_plus,NameReply);
VERIFY_CPP_ABI(mems_ignition_advance_minus,NameReply);
VERIFY_CPP_ABI(mems_Purge_Valve_On,NameReply);
VERIFY_CPP_ABI(mems_Purge_Valve_Off,NameReply);
VERIFY_CPP_ABI(mems_O2Heater_On,NameReply);
VERIFY_CPP_ABI(mems_O2Heater_Off,NameReply);
VERIFY_CPP_ABI(mems_Fan1_On,NameReply);
VERIFY_CPP_ABI(mems_Fan1_Off,NameReply);
VERIFY_CPP_ABI(mems_Fan2_On,NameReply);
VERIFY_CPP_ABI(mems_Fan2_Off,NameReply);
VERIFY_CPP_ABI(mems_Boost_Valve_On,NameReply);
VERIFY_CPP_ABI(mems_Boost_Valve_Off,NameReply);
VERIFY_CPP_ABI(mems_get_lib_version,Version);

/* 'mems_interactive_mode' is declared in the historic Manager header,
 * but the Session 196 interactive button does not invoke it, and it was not
 * among the candidate DLL's first 91 exports. Keep it tracked separately;
 * never claim it is already exported or tested.
 */
VERIFY_CPP_ABI(mems_interactive_mode,NameReply);

int main()
{
    const librosco_version ver=mems_get_lib_version();
    if(ver.major!=3 || ver.minor!=0 || ver.patch!=0) {
        std::fprintf(stderr,"Manager DLL version mismatch\n");
        return 1;
    }
    std::puts("MANAGER_196_DLL_HEADER_PASS: structs, reset byte mapping, declared signatures");
    return 0;
}
