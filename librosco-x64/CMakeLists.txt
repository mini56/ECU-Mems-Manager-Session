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
    mems_manager_x64.def
)

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
