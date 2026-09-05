cmake_minimum_required(VERSION 3.16)
project(ia_response_visual_coherence_selftest CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt5 REQUIRED COMPONENTS Core)

add_executable(ia_response_visual_coherence_selftest
    "${CMAKE_CURRENT_LIST_DIR}/../IaMemsResponseVisualCoherenceSelfTest.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../IaMemsLibraryBridge.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../IaMemsLibraryBridge.h"
    "${CMAKE_CURRENT_LIST_DIR}/../IaMemsDiagramCatalog.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../IaMemsDiagramCatalog.h")

target_include_directories(ia_response_visual_coherence_selftest PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/../.."
    "${CMAKE_CURRENT_LIST_DIR}/..")
target_link_libraries(ia_response_visual_coherence_selftest PRIVATE Qt5::Core)

if(MSVC)
    target_compile_options(ia_response_visual_coherence_selftest PRIVATE /W4 /permissive- /utf-8)
endif()
