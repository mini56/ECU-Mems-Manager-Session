cmake_minimum_required(VERSION 3.16)
project(mems_expert_engine_selftest CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt5 REQUIRED COMPONENTS Core Sql)

add_library(mems_expert_engine STATIC
    ExpertEngine.cpp
    ExpertEngine.h
    ExpertKnowledgeReader.cpp
    ExpertKnowledgeReader.h
    ExpertRuntimeDatabase.cpp
    ExpertRuntimeDatabase.h
    ExpertTypes.h)

target_link_libraries(mems_expert_engine PUBLIC Qt5::Core Qt5::Sql)
target_include_directories(mems_expert_engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(mems_expert_selftest ExpertEngineSelfTest.cpp)
target_link_libraries(mems_expert_selftest PRIVATE mems_expert_engine Qt5::Core Qt5::Sql)

add_executable(mems_expert_reference_selftest
    ExpertReferenceIntegrationSelfTest.cpp)
target_include_directories(mems_expert_reference_selftest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(mems_expert_reference_selftest PRIVATE mems_expert_engine Qt5::Core Qt5::Sql)

add_custom_command(TARGET mems_expert_reference_selftest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:mems_expert_reference_selftest>/database/reference"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/../database/reference"
            "$<TARGET_FILE_DIR:mems_expert_reference_selftest>/database/reference")

enable_testing()
add_test(NAME mems_expert_selftest COMMAND mems_expert_selftest)
add_test(NAME mems_expert_reference_selftest COMMAND mems_expert_reference_selftest)
