cmake_minimum_required(VERSION 3.16)
project(ia_memslibrary_qwen_selftest CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt5 REQUIRED COMPONENTS Core Widgets)

set(ORT_GENAI_INCLUDE_DIR "" CACHE PATH "Directory containing ort_genai_c.h")
set(ORT_GENAI_LIBRARY "" CACHE FILEPATH "ONNX Runtime GenAI import library")
if(NOT WIN32)
    message(FATAL_ERROR "IA MEMSLibrary Qwen self-test targets Windows x64 only")
endif()
if(NOT EXISTS "${ORT_GENAI_INCLUDE_DIR}/ort_genai_c.h")
    message(FATAL_ERROR "ORT_GENAI_INCLUDE_DIR does not contain ort_genai_c.h: ${ORT_GENAI_INCLUDE_DIR}")
endif()
if(NOT EXISTS "${ORT_GENAI_LIBRARY}")
    message(FATAL_ERROR "ORT_GENAI_LIBRARY not found: ${ORT_GENAI_LIBRARY}")
endif()

add_executable(ia_memslibrary_qwen_selftest
    ../expert/IaMemsLibraryQwenSelfTest.cpp
    ../expert/IaMemsLibraryBridge.cpp
    ../expert/IaMemsLibraryBridge.h
    ../expert/IaMemsConversationRouting.cpp
    ../expert/IaMemsConversationRouting.h
    ../expert/LocalAiClient.cpp
    ../expert/LocalAiClient.h
    ../i18n.cpp
    ../i18n.h)

target_compile_definitions(ia_memslibrary_qwen_selftest PRIVATE MEMS_USE_ONNX_GENAI=1)
target_include_directories(ia_memslibrary_qwen_selftest PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/.."
    "${ORT_GENAI_INCLUDE_DIR}")
target_link_libraries(ia_memslibrary_qwen_selftest PRIVATE
    Qt5::Core Qt5::Widgets "${ORT_GENAI_LIBRARY}")

if(MSVC)
    target_compile_options(ia_memslibrary_qwen_selftest PRIVATE /permissive /utf-8)
endif()
