cmake_minimum_required(VERSION 3.16)
project(build105_regression_selftest LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt5 REQUIRED COMPONENTS Widgets)

set(ORT_GENAI_INCLUDE_DIR "" CACHE PATH "Directory containing ort_genai_c.h")
set(ORT_GENAI_LIBRARY "" CACHE FILEPATH "ONNX Runtime GenAI import library")
if(NOT EXISTS "${ORT_GENAI_INCLUDE_DIR}/ort_genai_c.h")
    message(FATAL_ERROR "ORT_GENAI_INCLUDE_DIR does not contain ort_genai_c.h")
endif()
if(NOT EXISTS "${ORT_GENAI_LIBRARY}")
    message(FATAL_ERROR "ORT_GENAI_LIBRARY not found")
endif()

add_executable(build105_regression_selftest
    ../Build105RegressionSelfTest.cpp
    ../LocalAiClient.cpp ../LocalAiClient.h
    ../IaMemsDiagramCatalog.cpp ../IaMemsDiagramCatalog.h
    ../../i18n.cpp ../../i18n.h)

target_compile_definitions(build105_regression_selftest PRIVATE MEMS_USE_ONNX_GENAI=1)
target_include_directories(build105_regression_selftest PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/../.."
    "${CMAKE_CURRENT_LIST_DIR}/.."
    "${ORT_GENAI_INCLUDE_DIR}")
target_link_libraries(build105_regression_selftest PRIVATE Qt5::Widgets "${ORT_GENAI_LIBRARY}")

if(MSVC)
    target_compile_definitions(build105_regression_selftest PRIVATE _HAS_STD_BYTE=0)
    target_compile_options(build105_regression_selftest PRIVATE /permissive /utf-8)
endif()
