# ECU MEMS Manager — absolute build/version mapping.
# The GitHub Actions Windows build number is the source of truth.
# #610 -> 1.6.10, #611 -> 1.6.11, #699 -> 1.6.99, #700 -> 1.7.0.

if(DEFINED ENV{GITHUB_RUN_NUMBER} AND NOT "$ENV{GITHUB_RUN_NUMBER}" STREQUAL "")
    set(APP_BUILD_NUMBER "$ENV{GITHUB_RUN_NUMBER}")
    if(APP_BUILD_NUMBER MATCHES "^[0-9]+$")
        math(EXPR VER_MINOR "${APP_BUILD_NUMBER} / 100")
        math(EXPR VER_PATCH "${APP_BUILD_NUMBER} % 100")
        set(APP_VERSION "${VER_MAJOR}.${VER_MINOR}.${VER_PATCH}")
    endif()
endif()
