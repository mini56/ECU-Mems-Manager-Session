cmake_minimum_required(VERSION 3.15)
project(mems_documentary_network_probe LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
find_package(Qt5 REQUIRED COMPONENTS Core Network Sql Widgets)
add_executable(mems_documentary_network_probe probe.cpp)
target_link_libraries(mems_documentary_network_probe PRIVATE Qt5::Core Qt5::Network)

if(WIN32)
    add_executable(mems_documentary_native_https_probe native_probe.cpp)
    target_link_libraries(mems_documentary_native_https_probe PRIVATE winhttp)
    add_executable(mems_documentary_transport_probe transport_probe.cpp
        ../../database/MemsDocumentaryWindowsTransport.cpp
        ../../database/MemsDocumentaryPackCatalog.cpp)
    target_link_libraries(mems_documentary_transport_probe PRIVATE Qt5::Core winhttp)
endif()

add_library(mems_documentary_update_ui_syntax OBJECT
    ../../database/MemsDocumentaryUpdateDialog.cpp)
target_link_libraries(mems_documentary_update_ui_syntax PRIVATE
    Qt5::Core Qt5::Widgets Qt5::Sql Qt5::Network)
