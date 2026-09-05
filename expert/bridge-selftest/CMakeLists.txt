cmake_minimum_required(VERSION 3.16)
project(ia_memslibrary_bridge_selftest CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC OFF)
set(CMAKE_AUTOUIC OFF)
set(CMAKE_AUTORCC OFF)

find_package(Qt5 REQUIRED COMPONENTS Core)

add_executable(ia_memslibrary_bridge_selftest
    ../IaMemsLibraryBridgeSelfTest.cpp
    ../IaMemsLibraryBridge.cpp
    ../IaMemsLibraryBridge.h
    ../IaMemsDiagramCatalog.cpp
    ../IaMemsDiagramCatalog.h)

target_include_directories(ia_memslibrary_bridge_selftest PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/..
    ${CMAKE_CURRENT_LIST_DIR}/../..)

target_link_libraries(ia_memslibrary_bridge_selftest PRIVATE Qt5::Core)

if(MSVC)
    target_compile_options(ia_memslibrary_bridge_selftest PRIVATE /permissive /utf-8)
endif()
