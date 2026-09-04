if(CMAKE_VERSION VERSION_LESS 3.19)
    message(FATAL_ERROR "IA MEMSLibrary temporary integration requires CMake 3.19 or newer")
endif()

# This file is injected only by the temporary IA/MEMSLibrary validation workflow.
# Production MEMSX64 CMake behaviour stays unchanged.
cmake_language(DEFER CALL target_sources ${PNAME} PRIVATE
    "${CMAKE_SOURCE_DIR}/expert/IaMemsLibraryBridge.cpp"
    "${CMAKE_SOURCE_DIR}/expert/IaMemsLibraryBridge.h"
    "${CMAKE_SOURCE_DIR}/expert/IaMemsLibraryIntegration.cpp")

# Redirect only the IA tab call site to the isolated library-aware request path.
# IaMemsService::ask remains compiled and available as the untouched fallback path.
cmake_language(DEFER CALL set_source_files_properties iamemstab.cpp PROPERTIES
    COMPILE_DEFINITIONS "ask=askWithLibrary")

message(STATUS "Temporary IA MEMSLibrary bridge injection scheduled")
