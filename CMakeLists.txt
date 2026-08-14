cmake_minimum_required(VERSION 3.16)

set(PROJECTNAME "ECU_Mems_Manager")
string(TOLOWER ${PROJECTNAME} PNAME)
project(${PNAME} CXX)
add_definitions(-DPROJECTNAME="${PROJECTNAME}")
set(VER_MAJOR 1)
set(VER_MINOR 1)
set(VER_PATCH 0)
set(APP_VERSION "${VER_MAJOR}.${VER_MINOR}.${VER_PATCH}")
add_compile_definitions(APP_NAME="ECU Mems Manager" APP_VERSION="${APP_VERSION}")
add_definitions("-DVER_MAJOR=${VER_MAJOR}" "-DVER_MINOR=${VER_MINOR}" "-DVER_PATCH=${VER_PATCH}")
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
find_package(Qt5 REQUIRED COMPONENTS Widgets SerialPort Charts Sql)
include_directories("${CMAKE_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/qledindicator" "${CMAKE_SOURCE_DIR}/analogwidgets" "${CMAKE_SOURCE_DIR}/ecu" "${CMAKE_CURRENT_BINARY_DIR}")
configure_file("${CMAKE_SOURCE_DIR}/help.html.in" "${CMAKE_SOURCE_DIR}/help.html")
configure_file("${CMAKE_SOURCE_DIR}/help_en.html.in" "${CMAKE_SOURCE_DIR}/help_en.html")
set(SOURCES
    main.cpp i18n.cpp i18n.h memsinterface.cpp memslogic.cpp database/DatabaseSeed.cpp
    helpviewer.cpp logger.cpp serialdevenumerator.cpp splashprogress.cpp mainwindow.cpp aboutbox.cpp
    optionsdialog.cpp summarytab.cpp diagnosticpanel.cpp analysistab.cpp captureviewer.cpp captureviewer.h
    tooltipbubblemanager.cpp tooltipbubblemanager.h scr/ecu/ecumanager.cpp scr/ecu/ecumanager.h
    qledindicator/qledindicator.cpp analogwidgets/led.cpp analogwidgets/functions.cpp
    analogwidgets/widgetwithbackground.cpp analogwidgets/manometer.cpp analogwidgets/abstractmeter.cpp
    analogwidgets/manometer100.cpp analogwidgets/abstractmeter100.cpp analogwidgets/manometer900.cpp
    analogwidgets/abstractmeter900.cpp mainwindow.ui resources.qrc database/DatabaseManager.cpp
    uirebuild.cpp overviewrebuild.cpp majorpagesrebuild.cpp analysismodernizer.cpp
    strictvisualrebuild.cpp dedicatedpagesrebuild.cpp uipreflight.cpp visualcompletion.cpp)
add_executable(${PNAME} ${SOURCES})
target_sources(${PNAME} PRIVATE ${CMAKE_SOURCE_DIR}/desktopshortcut.cpp ${CMAKE_SOURCE_DIR}/desktopshortcut.h)
if(WIN32)
    # Embed the real multi-resolution key.ico directly in the Windows EXE.
    target_sources(${PNAME} PRIVATE ${CMAKE_SOURCE_DIR}/appicon.rc)
    target_link_libraries(${PNAME} PRIVATE ole32 shell32 uuid)
endif()

# Runtime translations: v1.1.0 uses only the six per-language JSON folders.
# Legacy flat JSON catalogs, Qt .ts catalogs and migration scripts are no
# longer deployed with the application.
add_custom_command(TARGET ${PNAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PNAME}>/translations"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/translations/fr" "$<TARGET_FILE_DIR:${PNAME}>/translations/fr"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/translations/en" "$<TARGET_FILE_DIR:${PNAME}>/translations/en"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/translations/es" "$<TARGET_FILE_DIR:${PNAME}>/translations/es"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/translations/it" "$<TARGET_FILE_DIR:${PNAME}>/translations/it"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/translations/pt" "$<TARGET_FILE_DIR:${PNAME}>/translations/pt"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/translations/de" "$<TARGET_FILE_DIR:${PNAME}>/translations/de")

if(MSVC)
    add_compile_definitions(_HAS_STD_BYTE=0)
    target_compile_options(${PNAME} PRIVATE /permissive)
endif()
if(WIN32)
    message(STATUS "Plateforme Windows détectée (MSVC / MinGW).")
    if(MSVC)
        set_target_properties(${PNAME} PROPERTIES WIN32_EXECUTABLE TRUE)
    elseif(MINGW)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mwindows")
    endif()
    find_library(LIBROSCO_DLL rosco)
    if(LIBROSCO_DLL)
        message(STATUS "Bibliothèque rosco trouvée : ${LIBROSCO_DLL}")
        target_link_libraries(${PNAME} PRIVATE ${LIBROSCO_DLL} Qt5::Widgets Qt5::SerialPort Qt5::Charts Qt5::Sql)
    else()
        message(STATUS "Bibliothèque rosco non trouvée.")
        target_link_libraries(${PNAME} PRIVATE Qt5::Widgets Qt5::SerialPort Qt5::Charts Qt5::Sql)
    endif()
else()
    message(STATUS "Environnement Linux/Unix détecté.")
    target_link_libraries(${PNAME} PRIVATE rosco Qt5::Widgets Qt5::SerialPort)
    set(CMAKE_SKIP_RPATH TRUE)
    set(CMAKE_INSTALL_PREFIX "/usr")
    set(EXE_FILE "${CMAKE_CURRENT_BINARY_DIR}/${PNAME}")
    install(FILES ${EXE_FILE} DESTINATION "bin" PERMISSIONS OWNER_READ OWNER_EXECUTE OWNER_WRITE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
    install(DIRECTORY DESTINATION "share/doc/${PNAME}" DIRECTORY_PERMISSIONS OWNER_READ OWNER_EXECUTE OWNER_WRITE GROUP_READ OWNER_EXECUTE GROUP_EXECUTE WORLD_READ GROUP_EXECUTE)
    install(FILES "${CMAKE_SOURCE_DIR}/README" "${CMAKE_SOURCE_DIR}/LICENSE" DESTINATION "share/doc/${PNAME}" PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)
    if("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86_64")
        set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE amd64)
    elseif("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "i686")
        set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE i386)
    endif()
    set(CPACK_GENERATOR "DEB")
    set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
    set(CPACK_PACKAGE_VERSION_MAJOR ${VER_MAJOR})
    set(CPACK_PACKAGE_VERSION_MINOR ${VER_MINOR})
    set(CPACK_PACKAGE_VERSION_PATCH ${VER_PATCH})
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Colin Bourassa [colin.bourassa@gmail.com]")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "ECU diagnostic manager for Rover MEMS")
    set(CPACK_DEBIAN_PACKAGE_SECTION "Science")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6 (>= 2.13), libstdc++6 (>= 4.6.3), libqt5core5a (>= 5.0.2), libqt5gui5 (>= 5.0.2), libqt5widgets5 (>= 5.0.2)")
    set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${VER_MAJOR}.${VER_MINOR}.${VER_PATCH}-${CMAKE_SYSTEM_NAME}-${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}")
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
    set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README")
    include(CPack)
endif()