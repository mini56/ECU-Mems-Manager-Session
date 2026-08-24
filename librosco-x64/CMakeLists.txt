cmake_minimum_required(VERSION 3.20)
project(librosco_compat_x64 LANGUAGES C)

if(NOT WIN32)
    message(FATAL_ERROR "This compatibility target currently validates the Windows librosco ABI only.")
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_library(rosco SHARED
    setup.c
    protocol.c
    librosco.def
)

target_include_directories(rosco PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_definitions(rosco PRIVATE
    ROSCO_BUILD_DLL
    LIBROSCO_VER_MAJOR=0
    LIBROSCO_VER_MINOR=1
    LIBROSCO_VER_PATCH=12
)

if(MSVC)
    target_compile_options(rosco PRIVATE /W4)
else()
    target_compile_options(rosco PRIVATE -Wall -Wextra)
endif()

set_target_properties(rosco PROPERTIES
    OUTPUT_NAME "librosco"
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
