cmake_minimum_required(VERSION 3.16)
project(ia_visual_question_selftest CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt5 REQUIRED COMPONENTS Core)

add_executable(ia_visual_question_selftest
    ../expert/IaMemsVisualQuestionSelfTest.cpp
    ../expert/IaMemsDiagramCatalog.cpp
    ../expert/IaMemsDiagramCatalog.h)

target_include_directories(ia_visual_question_selftest PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../expert")
target_link_libraries(ia_visual_question_selftest PRIVATE Qt5::Core)

if(MSVC)
    target_compile_options(ia_visual_question_selftest PRIVATE /permissive /utf-8)
endif()
