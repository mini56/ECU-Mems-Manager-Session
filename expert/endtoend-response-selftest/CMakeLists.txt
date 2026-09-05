cmake_minimum_required(VERSION 3.16)
project(ia_mems_e2e_response_selftest CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt5 REQUIRED COMPONENTS Core Widgets)

if(NOT DEFINED ORT_GENAI_INCLUDE_DIR OR NOT EXISTS "${ORT_GENAI_INCLUDE_DIR}/ort_genai_c.h")
    message(FATAL_ERROR "ORT_GENAI_INCLUDE_DIR invalid")
endif()
if(NOT DEFINED ORT_GENAI_LIBRARY OR NOT EXISTS "${ORT_GENAI_LIBRARY}")
    message(FATAL_ERROR "ORT_GENAI_LIBRARY invalid")
endif()

add_executable(ia_mems_e2e_response_selftest
    ../IaMemsEndToEndResponseSelfTest.cpp
    ../IaMemsLibraryBridge.cpp
    ../IaMemsLibraryBridge.h
    ../LocalAiClient.cpp
    ../LocalAiClient.h
    ../../i18n.cpp
    ../../i18n.h)

target_compile_definitions(ia_mems_e2e_response_selftest PRIVATE MEMS_USE_ONNX_GENAI=1)
target_include_directories(ia_mems_e2e_response_selftest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${ORT_GENAI_INCLUDE_DIR})
target_link_libraries(ia_mems_e2e_response_selftest PRIVATE Qt5::Core Qt5::Widgets "${ORT_GENAI_LIBRARY}")

if(MSVC)
    target_compile_options(ia_mems_e2e_response_selftest PRIVATE /permissive /utf-8)
endif()
