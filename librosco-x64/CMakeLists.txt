cmake_minimum_required(VERSION 3.20)
project(mems_manager_x64_compat LANGUAGES C CXX)

if(NOT WIN32)
    message(FATAL_ERROR "This compatibility target currently validates the Windows MEMS Manager protocol DLL ABI only.")
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Keep the internal CMake target name 'rosco' for source/API compatibility with
# the existing application while giving the new native x64 binary its own clear
# product identity. The historical 32-bit binary remains librosco.dll.
add_library(rosco SHARED
    setup.c
    protocol.c
    family.c
    transient_injection.c
    protocol_catalogue.c
    lucas5as_codec.c
    lucas5as_session.c
    mems19_wakeup_codec.c
    mems19_wakeup.c
    mems2j_session.c
    mems3_session.c
    rc5_session.c
    serial_adapters.c
    auto_connect.c
    mems_manager_x64.def
)

target_link_libraries(rosco PRIVATE setupapi cfgmgr32 advapi32)

target_include_directories(rosco PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_definitions(rosco PRIVATE
    ROSCO_BUILD_DLL
    LIBROSCO_VER_MAJOR=3
    LIBROSCO_VER_MINOR=0
    LIBROSCO_VER_PATCH=0
)

if(MSVC)
    target_compile_options(rosco PRIVATE /W4)
else()
    target_compile_options(rosco PRIVATE -Wall -Wextra)
endif()

set_target_properties(rosco PROPERTIES
    OUTPUT_NAME "mems_manager_x64"
    PREFIX ""
)

add_executable(rosco_abi_selftest abi_selftest.c)
target_include_directories(rosco_abi_selftest PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(rosco_abi_selftest PRIVATE rosco)

if(MSVC)
    target_compile_options(rosco_abi_selftest PRIVATE /W4)
endif()

enable_testing()
add_test(NAME rosco_abi_selftest COMMAND rosco_abi_selftest)

# Compile and link from the exact header consumed by ECU MEMS Manager,
# not only from the internal DLL header. This catches missing declarations
# and mismatched prototype/structure changes before any Setup is built.
add_executable(manager_dll_contract_selftest manager_dll_contract_selftest.cpp)
target_compile_definitions(manager_dll_contract_selftest PRIVATE MEMS_NATIVE_KLINE_19=1)
target_link_libraries(manager_dll_contract_selftest PRIVATE rosco)
add_test(NAME manager_dll_contract_selftest COMMAND manager_dll_contract_selftest)

# Simulated transport regression uses protocol.c unchanged with fake Win32
# I/O solely inside this offline test executable. No real COM access possible.
add_executable(serial_protocol_selftest serial_protocol_selftest.c)
add_test(NAME serial_protocol_selftest COMMAND serial_protocol_selftest)

# Protocol-only C tests never open COM ports or talk to real ECUs.
add_executable(protocol_catalogue_selftest protocol_catalogue_selftest.c protocol_catalogue.c)
add_test(NAME protocol_catalogue_selftest COMMAND protocol_catalogue_selftest)

# Additional independent Lucas 5AS C ABI. The protocol vector test links
# the real DLL; the transport selftest compiles the real source against
# fake Win32 serial/clock calls, so it CANNOT open any physical ECU/COM.
add_executable(dll_lucas5as_protocol_selftest lucas5as_protocol_selftest.c)
target_link_libraries(dll_lucas5as_protocol_selftest PRIVATE rosco)
add_test(NAME dll_lucas5as_protocol_selftest COMMAND dll_lucas5as_protocol_selftest)

add_executable(dll_lucas5as_session_selftest lucas5as_session_selftest.c lucas5as_codec.c)
add_test(NAME dll_lucas5as_session_selftest COMMAND dll_lucas5as_session_selftest)

# New independent MEMS 1.9 physical ISO9141 wake and same-handle D0 ID.
# The C codec test loads the DLL. The Win32 mock test compiles the same
# wake source with intercepted COM and clock functions; NO real ECU/COM.
add_executable(dll_mems19_wakeup_codec_selftest mems19_wakeup_codec_selftest.c)
target_link_libraries(dll_mems19_wakeup_codec_selftest PRIVATE rosco)
add_test(NAME dll_mems19_wakeup_codec_selftest COMMAND dll_mems19_wakeup_codec_selftest)

add_executable(dll_mems19_wakeup_selftest mems19_wakeup_selftest.c mems19_wakeup_codec.c)
add_test(NAME dll_mems19_wakeup_selftest COMMAND dll_mems19_wakeup_selftest)

# Native MEMS 2J K-Line authenticated diagnostic session. The transport
# selftest includes the real source with Win32 COM and clock intercepted;
# the compiled executable cannot access any physical serial port or ECU.
add_executable(dll_mems2j_session_selftest mems2j_session_selftest.c protocol_catalogue.c)
add_test(NAME dll_mems2j_session_selftest COMMAND dll_mems2j_session_selftest)

# Independent MEMS 3 native transport, fake serial I/O regression.
add_executable(dll_mems3_session_selftest mems3_session_selftest.c protocol_catalogue.c)
add_test(NAME dll_mems3_session_selftest COMMAND dll_mems3_session_selftest)

# Independent RC5 SRS diagnostic transport (offline simulated serial regression).
add_executable(dll_rc5_session_selftest rc5_session_selftest.c protocol_catalogue.c)
add_test(NAME dll_rc5_session_selftest COMMAND dll_rc5_session_selftest)

# Read-only adapter discovery tests; no physical serial communication.
add_executable(dll_serial_adapters_selftest serial_adapters_selftest.c)
target_link_libraries(dll_serial_adapters_selftest PRIVATE setupapi cfgmgr32 advapi32)
add_test(NAME dll_serial_adapters_selftest COMMAND dll_serial_adapters_selftest)

# Engine auto-dispatch regression; all protocol and enumeration calls mocked.
add_executable(dll_auto_connect_selftest auto_connect_selftest.c)
add_test(NAME dll_auto_connect_selftest COMMAND dll_auto_connect_selftest)
