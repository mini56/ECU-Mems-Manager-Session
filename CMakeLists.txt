
cmake_minimum_required(VERSION 3.16)

set(PROJECTNAME "ECU_Mems_Manager")

string(TOLOWER ${PROJECTNAME} PNAME)
project(${PNAME} CXX)

add_definitions(-DPROJECTNAME="${PROJECTNAME}")

set(VER_MAJOR 1)
set(VER_MINOR 0)
set(VER_PATCH 8)
set(APP_VERSION "${VER_MAJOR}.${VER_MINOR}.${VER_PATCH}")

add_compile_definitions(
    APP_NAME="ECU Mems Manager"
    APP_VERSION="${APP_VERSION}"
)

add_definitions(
    "-DVER_MAJOR=${VER_MAJOR}"
    "-DVER_MINOR=${VER_MINOR}"
    "-DVER_PATCH=${VER_PATCH}"
)

# Configuration Qt
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5 REQUIRED COMPONENTS Widgets SerialPort Charts Sql LinguistTools)

# Dossiers du projet
include_directories(
    "${CMAKE_SOURCE_DIR}"
    "${CMAKE_SOURCE_DIR}/qledindicator"
    "${CMAKE_SOURCE_DIR}/analogwidgets"
    "${CMAKE_SOURCE_DIR}/ecu"
    "${CMAKE_CURRENT_BINARY_DIR}"
)

# Génération du fichier help.html
configure_file(
    "${CMAKE_SOURCE_DIR}/help.html.in"
    "${CMAKE_SOURCE_DIR}/help.html"
)
configure_file(
    "${CMAKE_SOURCE_DIR}/help_en.html.in"
    "${CMAKE_SOURCE_DIR}/help_en.html"
)

# Sources
set(SOURCES
    main.cpp
    memsinterface.cpp
    memslogic.cpp
    database/DatabaseSeed.cpp
    helpviewer.cpp
    logger.cpp
    serialdevenumerator.cpp
    splashprogress.cpp
    mainwindow.cpp
    aboutbox.cpp
    optionsdialog.cpp
    summarytab.cpp
    diagnosticpanel.cpp  
    analysistab.cpp
    captureviewer.cpp
    captureviewer.h
    tooltipbubblemanager.cpp
    tooltipbubblemanager.h
    
    # ECU Manager
    scr/ecu/ecumanager.cpp
    scr/ecu/ecumanager.h

    # QLedIndicator
    qledindicator/qledindicator.cpp

    # Analog widgets
    analogwidgets/led.cpp
    analogwidgets/functions.cpp
    analogwidgets/widgetwithbackground.cpp
    analogwidgets/manometer.cpp
    analogwidgets/abstractmeter.cpp
    analogwidgets/manometer100.cpp
    analogwidgets/abstractmeter100.cpp
    analogwidgets/manometer900.cpp
    analogwidgets/abstractmeter900.cpp

    # Interface Qt
    mainwindow.ui

    # Ressources
    resources.qrc
    
    # Sql
    database/DatabaseManager.cpp
)

# Exécutable
add_executable(${PNAME} ${SOURCES})



# Gestion du raccourci Windows sur le Bureau
target_sources(${PNAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/desktopshortcut.cpp
    ${CMAKE_SOURCE_DIR}/desktopshortcut.h
)
if(WIN32)
    target_link_libraries(${PNAME} PRIVATE ole32 shell32 uuid)
    add_custom_command(TARGET ${PNAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_SOURCE_DIR}/key.ico"
                "$<TARGET_FILE_DIR:${PNAME}>/key.ico"
    )
endif()

# Traductions multilingues
# Les 4 nouvelles langues sont stockées sous forme compacte dans le dépôt puis
# régénérées ici avant lrelease. Cela garantit que les 623 entrées complètes
# (interface, messages dynamiques et bulles d'aide) sont utilisées au build.
find_package(Python3 REQUIRED COMPONENTS Interpreter)
execute_process(
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/decode_translations.py
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    RESULT_VARIABLE TRANSLATION_DECODE_RESULT
)
if(NOT TRANSLATION_DECODE_RESULT EQUAL 0)
    message(FATAL_ERROR "Impossible de régénérer les traductions ES/IT/PT/DE")
endif()

set(TS_FILES
    ${CMAKE_SOURCE_DIR}/translations/ECUMemsManager_fr.ts
    ${CMAKE_SOURCE_DIR}/translations/ECUMemsManager_en.ts
    ${CMAKE_SOURCE_DIR}/translations/ECUMemsManager_es.ts
    ${CMAKE_SOURCE_DIR}/translations/ECUMemsManager_it.ts
    ${CMAKE_SOURCE_DIR}/translations/ECUMemsManager_pt.ts
    ${CMAKE_SOURCE_DIR}/translations/ECUMemsManager_de.ts
)
qt5_add_translation(QM_FILES ${TS_FILES})
add_custom_target(translations ALL DEPENDS ${QM_FILES})
add_dependencies(${PNAME} translations)
add_custom_command(TARGET ${PNAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PNAME}>/translations"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${QM_FILES} "$<TARGET_FILE_DIR:${PNAME}>/translations"
)

# Compatibilité MSVC / Qt 5.15
if(MSVC)
    add_compile_definitions(_HAS_STD_BYTE=0)
    target_compile_options(${PNAME} PRIVATE /permissive)
endif()

# ============================================================
# WINDOWS
# ============================================================

if(WIN32)

    message(STATUS "Plateforme Windows détectée (MSVC / MinGW).")

    if(MSVC)
        set_target_properties(
            ${PNAME}
            PROPERTIES
            WIN32_EXECUTABLE TRUE
        )

    elseif(MINGW)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mwindows")
    endif()

    # Recherche de rosco
    find_library(
        LIBROSCO_DLL
        rosco
    )

    if(LIBROSCO_DLL)

        message(
            STATUS
            "Bibliothèque rosco trouvée : ${LIBROSCO_DLL}"
        )

        target_link_libraries(
            ${PNAME}
            PRIVATE
            ${LIBROSCO_DLL}
            Qt5::Widgets
            Qt5::SerialPort
            Qt5::Charts
            Qt5::Sql
        )

    else()

        message(
            STATUS
            "Bibliothèque rosco non trouvée."
        )

        target_link_libraries(
            ${PNAME}
            PRIVATE
            Qt5::Widgets
            Qt5::SerialPort
            Qt5::Charts
            Qt5::Sql
        )

    endif()

# ============================================================
# LINUX / UNIX
# ============================================================

else()

    message(STATUS "Environnement Linux/Unix détecté.")

    target_link_libraries(
        ${PNAME}
        PRIVATE
        rosco
        Qt5::Widgets
        Qt5::SerialPort
    )

    set(CMAKE_SKIP_RPATH TRUE)
    set(CMAKE_INSTALL_PREFIX "/usr")

    set(
        EXE_FILE
        "${CMAKE_CURRENT_BINARY_DIR}/${PNAME}"
    )

    install(
        FILES ${EXE_FILE}
        DESTINATION "bin"
        PERMISSIONS
            OWNER_READ
            OWNER_EXECUTE
            OWNER_WRITE
            GROUP_READ
            GROUP_EXECUTE
            WORLD_READ
            WORLD_EXECUTE
    )

    install(
        DIRECTORY
        DESTINATION "share/doc/${PNAME}"
        DIRECTORY_PERMISSIONS
            OWNER_READ
            OWNER_EXECUTE
            OWNER_WRITE
            GROUP_READ
            GROUP_EXECUTE
            WORLD_READ
            WORLD_EXECUTE
    )

    install(
        FILES
            "${CMAKE_SOURCE_DIR}/README"
            "${CMAKE_SOURCE_DIR}/LICENSE"
        DESTINATION "share/doc/${PNAME}"
        PERMISSIONS
            OWNER_READ
            OWNER_WRITE
            GROUP_READ
            WORLD_READ
    )

    # CPack Debian
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

    set(
        CPACK_DEBIAN_PACKAGE_MAINTAINER
        "Colin Bourassa [colin.bourassa@gmail.com]"
    )

    set(
        CPACK_PACKAGE_DESCRIPTION_SUMMARY
        "ECU diagnostic manager for Rover MEMS"
    )

    set(CPACK_DEBIAN_PACKAGE_SECTION "Science")

    set(
        CPACK_DEBIAN_PACKAGE_DEPENDS
        "libc6 (>= 2.13), libstdc++6 (>= 4.6.3), libqt5core5a (>= 5.0.2), libqt5gui5 (>= 5.0.2), libqt5widgets5 (>= 5.0.2)"
    )

    set(
        CPACK_PACKAGE_FILE_NAME
        "${PROJECT_NAME}-${VER_MAJOR}.${VER_MINOR}.${VER_PATCH}-${CMAKE_SYSTEM_NAME}-${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}"
    )

    set(
        CPACK_RESOURCE_FILE_LICENSE
        "${CMAKE_SOURCE_DIR}/LICENSE"
    )

    set(
        CPACK_RESOURCE_FILE_README
        "${CMAKE_SOURCE_DIR}/README"
    )

    include(CPack)

endif()

