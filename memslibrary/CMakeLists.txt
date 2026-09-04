cmake_minimum_required(VERSION 3.16)

project(MEMSLibrary LANGUAGES CXX)

if(NOT WIN32)
    message(FATAL_ERROR "MEMSLibrary currently targets Windows x64 only")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(MEMSLibrary SHARED
    src/MEMSLibrary.cpp
    include/MEMSLibrary.h)

target_include_directories(MEMSLibrary PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_compile_definitions(MEMSLibrary PRIVATE MEMSLIBRARY_BUILD=1)
target_link_libraries(MEMSLibrary PRIVATE winsqlite3)
set_target_properties(MEMSLibrary PROPERTIES OUTPUT_NAME "MEMSLibrary")

add_executable(MEMSLibrarySmokeTest tests/MEMSLibrarySmokeTest.cpp)
target_include_directories(MEMSLibrarySmokeTest PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
add_dependencies(MEMSLibrarySmokeTest MEMSLibrary)

if(MSVC)
    target_compile_options(MEMSLibrary PRIVATE /W4 /permissive- /utf-8)
    target_compile_options(MEMSLibrarySmokeTest PRIVATE /W4 /permissive- /utf-8)
endif()
