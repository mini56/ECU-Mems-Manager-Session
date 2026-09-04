#pragma once

#include <cstdint>

#if defined(_WIN32)
#  if defined(MEMSLIBRARY_BUILD)
#    define MEMSLIBRARY_API __declspec(dllexport)
#  else
#    define MEMSLIBRARY_API __declspec(dllimport)
#  endif
#else
#  define MEMSLIBRARY_API
#endif

#define MEMSLIBRARY_ABI_VERSION 1u

extern "C" {

MEMSLIBRARY_API std::uint32_t MEMSLibrary_GetAbiVersion();
MEMSLIBRARY_API const char* MEMSLibrary_GetName();
MEMSLIBRARY_API const char* MEMSLibrary_GetEngineRole();

}
