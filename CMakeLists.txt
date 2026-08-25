cmake_minimum_required(VERSION 3.16)

set(PROJECTNAME "ECU_Mems_Manager")
string(TOLOWER ${PROJECTNAME} PNAME)
project(${PNAME} CXX)
add_definitions(-DPROJECTNAME="${PROJECTNAME}")
set(VER_MAJOR 1)
set(VER_MINOR 6)
set(VER_PATCH 0)
set(APP_BUILD_NUMBER "local")
if(DEFINED ENV{MEMS_BUILD_NUMBER} AND NOT "$ENV{MEMS_BUILD_NUMBER}" STREQUAL "")
    set(APP_BUILD_NUMBER "$ENV{MEMS_BUILD_NUMBER}")
endif()
if(APP_BUILD_NUMBER MATCHES "^[0-9]+$")
    math(EXPR VER_MINOR "${APP_BUILD_NUMBER} / 100")
    math(EXPR VER_PATCH "${APP_BUILD_NUMBER} % 100")
endif()
set(APP_VERSION "${VER_MAJOR}.${VER_MINOR}.${VER_PATCH}")
set(APP_COMMIT_SHA "local")
if(DEFINED ENV{GITHUB_SHA} AND NOT "$ENV{GITHUB_SHA}" STREQUAL "")
    string(SUBSTRING "$ENV{GITHUB_SHA}" 0 7 APP_COMMIT_SHA)
endif()
add_compile_definitions(APP_NAME="ECU Mems Manager" APP_VERSION="${APP_VERSION}" APP_BUILD_NUMBER="${APP_BUILD_NUMBER}" APP_COMMIT_SHA="${APP_COMMIT_SHA}")
add_definitions("-DVER_MAJOR=${VER_MAJOR}" "-DVER_MINOR=${VER_MINOR}" "-DVER_PATCH=${VER_PATCH}")
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
find_package(Qt5 REQUIRED COMPONENTS Widgets SerialPort Charts Sql Network)
include_directories("${CMAKE_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/qledindicator" "${CMAKE_SOURCE_DIR}/analogwidgets" "${CMAKE_SOURCE_DIR}/ecu" "${CMAKE_CURRENT_BINARY_DIR}")

option(MEMS_USE_BUNDLED_LIBROSCO_X64 "Build and link the verified source-controlled native librosco implementation" OFF)
if(MEMS_USE_BUNDLED_LIBROSCO_X64)
    if(NOT WIN32)
        message(FATAL_ERROR "MEMS_USE_BUNDLED_LIBROSCO_X64 currently targets Windows only")
    endif()
    enable_language(C)
    # Historical source files use WIN32 while MSVC guarantees _WIN32. Define
    # WIN32 for the compatibility build so the unchanged application code takes
    # the same Windows paths as the old MinGW build.
    add_compile_definitions(WIN32)
    add_subdirectory(librosco-x64)
endif()

configure_file("${CMAKE_SOURCE_DIR}/help.html.in" "${CMAKE_SOURCE_DIR}/help.html")
configure_file("${CMAKE_SOURCE_DIR}/help_en.html.in" "${CMAKE_SOURCE_DIR}/help_en.html")
set(SOURCES
    main.cpp singleinstanceforce.cpp i18n.cpp i18n.h memsinterface_dispatch.cpp memsinterface.h overviewramlive.cpp mappedinjection.cpp injectionramtest.cpp injectionramtestlogger.cpp memslogic.cpp database/DatabaseSeed.cpp
    helpviewer.cpp logger.cpp serialdevenumerator.cpp splashprogress.cpp mainwindow.cpp aboutbox.cpp
    optionsdialog.cpp summarytab.cpp diagnosticpanel.cpp analysistab.cpp captureviewer.cpp captureviewer.h
    tooltipbubblemanager.cpp tooltipbubblemanager.h scr/ecu/ecumanager.cpp scr/ecu/ecumanager.h
    qledindicator/qledindicator.cpp analogwidgets/led.cpp analogwidgets/functions.cpp
    analogwidgets/widgetwithbackground.cpp analogwidgets/manometer.cpp analogwidgets/abstractmeter.cpp
    analogwidgets/manometer100.cpp analogwidgets/abstractmeter100.cpp analogwidgets/manometer900.cpp
    analogwidgets/abstractmeter900.cpp mainwindow.ui resources.qrc database/DatabaseManager.cpp
    database/MemsReferenceDatabase.cpp database/MemsReferenceDatabase.h database/MemsReferencePackageRefresh.cpp
    database/MemsGlobalSearchIndex.cpp database/MemsGlobalSearchIndex.h database/MemsDatabaseExplorerV2.cpp
    database/MemsDatabaseUniversalAccess.cpp database/MemsReferenceLanguageSync.cpp
    database/MemsXmlRowIndexer.cpp database/MemsSearchCompletenessPatch.cpp database/MemsDatabaseBrowser.cpp database/MemsDatabaseBrowser.h
    iamemstab_clean.cpp iamemstab.h iamemsqualitypatch.cpp iaresponsecontextpatch.cpp expert/IaResponseLogic.h expert/ExpertEngine.cpp expert/ExpertEngine.h expert/ExpertKnowledgeReader.cpp expert/ExpertKnowledgeReader.h expert/ExpertRuntimeDatabase.cpp expert/ExpertRuntimeDatabase.h expert/ExpertTypes.h expert/LocalAiClient.cpp expert/LocalAiClient.h
    uirebuild.cpp overviewrebuild.cpp majorpagesrebuild.cpp errorsvisualpatch.cpp analysismodernizer.cpp
    strictvisualrebuild.cpp dedicatedpagesrebuild.cpp visualcompletion.cpp navigationorderpatch.cpp darkstyle.cpp darkfixes.cpp
    settingstabdark.cpp sidebarbubblecleaner.cpp splashtextfix.cpp settingsvisualpatch.cpp readabilityfix.cpp mainwindowtitlebarfix.cpp systemstateconnectionfix.cpp)
add_executable(${PNAME} ${SOURCES})
target_sources(${PNAME} PRIVATE ${CMAKE_SOURCE_DIR}/desktopshortcut.cpp ${CMAKE_SOURCE_DIR}/desktopshortcut.h)

# BUILD #27 safety policy self-test. It is intentionally independent of the
# serial transport and proves the family/mode command authorization matrix.
add_executable(protocol_context_selftest protocolcontext_test.cpp protocolcontext.h)
add_dependencies(${PNAME} protocol_context_selftest)
add_custom_command(TARGET ${PNAME} POST_BUILD
    COMMAND "$<TARGET_FILE:protocol_context_selftest>")

# BUILD #29 IA response self-test: reproduce the exact user questions that
# exposed over-broad and non-actionable fallback answers without needing an ECU.
# It is compiled here and executed explicitly by GitHub Actions after the build,
# with the Qt runtime configured by PowerShell rather than from MSBuild POST_BUILD.
add_executable(ia_response_selftest
    expert/IaResponseLogicTest.cpp
    expert/IaResponseLogic.h)
target_link_libraries(ia_response_selftest PRIVATE Qt5::Core)
add_dependencies(${PNAME} ia_response_selftest)

# Functional search self-test: exercises the same reference DB + global index code
# used by the application, without starting the GUI.
add_executable(mems_search_selftest
    database/MemsSearchSelfTest.cpp
    database/MemsReferenceDatabase.cpp database/MemsReferenceDatabase.h
    database/MemsGlobalSearchIndex.cpp database/MemsGlobalSearchIndex.h)
target_link_libraries(mems_search_selftest PRIVATE Qt5::Core Qt5::Sql)
add_dependencies(${PNAME} mems_search_selftest)

if(WIN32)
    target_sources(${PNAME} PRIVATE ${CMAKE_SOURCE_DIR}/appicon.rc)
    target_link_libraries(${PNAME} PRIVATE ole32 shell32 uuid)

    add_executable(ecu_mems_uninstaller WIN32
        uninstaller.cpp i18n.cpp i18n.h appicon.rc)
    target_link_libraries(ecu_mems_uninstaller PRIVATE Qt5::Widgets)
endif()

add_custom_command(TARGET ${PNAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PNAME}>/translations"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/translations/fr" "$<TARGET_FILE_DIR:${PNAME}>/translations/fr"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/translations/en" "$<TARGET_FILE_DIR:${PNAME}>/translations/en"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/translations/es" "$<TARGET_FILE_DIR:${PNAME}>/translations/es"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/translations/it" "$<TARGET_FILE_DIR:${PNAME}>/translations/it"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/translations/pt" "$<TARGET_FILE_DIR:${PNAME}>/translations/pt"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/translations/de" "$<TARGET_FILE_DIR:${PNAME}>/translations/de"
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PNAME}>/database"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/database/reference" "$<TARGET_FILE_DIR:${PNAME}>/database/reference"
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PNAME}>/ai"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/ai" "$<TARGET_FILE_DIR:${PNAME}>/ai")

option(MEMS_RUN_REFERENCE_SELFTEST_POST_BUILD "Run the heavyweight full reference search self-test after the application build" ON)
if(MEMS_RUN_REFERENCE_SELFTEST_POST_BUILD)
    add_custom_command(TARGET ${PNAME} POST_BUILD
        COMMAND "$<TARGET_FILE:mems_search_selftest>")
endif()
if(MSVC)
    add_compile_definitions(_HAS_STD_BYTE=0)
    target_compile_options(${PNAME} PRIVATE /permissive /utf-8)
    target_compile_options(ia_response_selftest PRIVATE /utf-8)
endif()
if(WIN32)
    message(STATUS "Plateforme Windows détectée (MSVC / MinGW).")
    if(MSVC)
        set_target_properties(${PNAME} PROPERTIES WIN32_EXECUTABLE TRUE)
    elseif(MINGW)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mwindows")
    endif()

    if(MEMS_USE_BUNDLED_LIBROSCO_X64)
        message(STATUS "Using source-controlled bundled librosco target")
        target_link_libraries(${PNAME} PRIVATE rosco Qt5::Widgets Qt5::SerialPort Qt5::Charts Qt5::Sql Qt5::Network)
        add_dependencies(${PNAME} rosco)
    else()
        find_library(LIBROSCO_DLL rosco)
        if(LIBROSCO_DLL)
            message(STATUS "Bibliothèque rosco trouvée : ${LIBROSCO_DLL}")
            target_link_libraries(${PNAME} PRIVATE ${LIBROSCO_DLL} Qt5::Widgets Qt5::SerialPort Qt5::Charts Qt5::Sql Qt5::Network)
        else()
            message(STATUS "Bibliothèque rosco non trouvée.")
            target_link_libraries(${PNAME} PRIVATE Qt5::Widgets Qt5::SerialPort Qt5::Charts Qt5::Sql Qt5::Network)
        endif()
    endif()
else()
    message(STATUS "Environnement Linux/Unix détecté.")
    target_link_libraries(${PNAME} PRIVATE rosco Qt5::Widgets Qt5::SerialPort Qt5::Network)
    set(CMAKE_SKIP_RPATH TRUE)
    set(CMAKE_INSTALL_PREFIX "/usr")
    set(EXE_FILE "${CMAKE_CURRENT_BINARY_DIR}/${PNAME}")
    install(FILES ${EXE_FILE} DESTINATION "bin" PERMISSIONS OWNER_READ OWNER_EXECUTE OWNER_WRITE GROUP_READ OWNER_EXECUTE GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
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