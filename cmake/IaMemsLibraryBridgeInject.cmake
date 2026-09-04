if(NOT CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    return()
endif()

if(CMAKE_VERSION VERSION_LESS 3.19)
    message(FATAL_ERROR "IA MEMSLibrary temporary integration requires CMake 3.19 or newer")
endif()

# This file is injected only by the temporary IA/MEMSLibrary validation workflow.
# Production MEMSX64 CMake behaviour stays unchanged.
#
# Redirect the single IA tab request explicitly in the temporary CI workspace.
# Do not use a global/source compile definition named "ask": that macro also
# rewrites IaMemsService.h, where ask() and askWithLibrary() are both declared.
set(_ia_tab_path "${CMAKE_SOURCE_DIR}/iamemstab.cpp")
file(READ "${_ia_tab_path}" _ia_tab_source)
set(_ia_legacy_call "m_service->ask(IaMemsConversationRouting::focusedQuestion(effectiveQuestion));")
set(_ia_library_call "m_service->askWithLibrary(IaMemsConversationRouting::focusedQuestion(effectiveQuestion));")
string(FIND "${_ia_tab_source}" "${_ia_legacy_call}" _ia_legacy_call_pos)
if(_ia_legacy_call_pos EQUAL -1)
    string(FIND "${_ia_tab_source}" "${_ia_library_call}" _ia_library_call_pos)
    if(_ia_library_call_pos EQUAL -1)
        message(FATAL_ERROR "Temporary IA bridge could not locate the IA tab request call site")
    endif()
else()
    string(REPLACE "${_ia_legacy_call}" "${_ia_library_call}" _ia_tab_source "${_ia_tab_source}")
    file(WRITE "${_ia_tab_path}" "${_ia_tab_source}")
    message(STATUS "Temporary IA tab request redirected to askWithLibrary")
endif()

cmake_language(DEFER CALL target_sources ${PNAME} PRIVATE
    "${CMAKE_SOURCE_DIR}/expert/IaMemsLibraryBridge.cpp"
    "${CMAKE_SOURCE_DIR}/expert/IaMemsLibraryBridge.h"
    "${CMAKE_SOURCE_DIR}/expert/IaMemsLibraryIntegration.cpp")

message(STATUS "Temporary IA MEMSLibrary bridge injection scheduled")
