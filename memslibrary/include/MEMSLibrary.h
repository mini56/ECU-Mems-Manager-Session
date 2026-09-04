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

#define MEMSLIBRARY_ABI_VERSION 3u
#define MEMSLIBRARY_TEXT_CAPACITY 512u
#define MEMSLIBRARY_ID_CAPACITY 128u
#define MEMSLIBRARY_LANGUAGE_CAPACITY 16u

enum MEMSLibraryStatus : std::int32_t {
    MEMSLIBRARY_OK = 0,
    MEMSLIBRARY_INVALID_ARGUMENT = 1,
    MEMSLIBRARY_PACK_NOT_FOUND = 2,
    MEMSLIBRARY_SQLITE_OPEN_FAILED = 3,
    MEMSLIBRARY_PACK_INVALID = 4,
    MEMSLIBRARY_QUERY_FAILED = 5,
    MEMSLIBRARY_LANGUAGE_REQUIRED = 6,
    MEMSLIBRARY_LANGUAGE_NOT_FOUND = 7
};

struct MEMSLibraryPackInfo {
    std::uint32_t struct_size;
    std::uint32_t schema_version;
    std::uint32_t document_count;
    std::uint32_t source_database_count;
    char pack_id[MEMSLIBRARY_ID_CAPACITY];
};

struct MEMSLibrarySearchResult {
    std::uint32_t struct_size;
    std::int32_t page_number;
    char document_key[MEMSLIBRARY_ID_CAPACITY];
    char entity_kind[64];
    char entity_key[MEMSLIBRARY_ID_CAPACITY];
    char source_language[MEMSLIBRARY_LANGUAGE_CAPACITY];
    char title[MEMSLIBRARY_TEXT_CAPACITY];
    char body[MEMSLIBRARY_TEXT_CAPACITY];
};

extern "C" {

MEMSLIBRARY_API std::uint32_t MEMSLibrary_GetAbiVersion();
MEMSLIBRARY_API const char* MEMSLibrary_GetName();
MEMSLIBRARY_API const char* MEMSLibrary_GetEngineRole();

MEMSLIBRARY_API std::int32_t MEMSLibrary_ValidatePack(
    const wchar_t* pack_directory,
    MEMSLibraryPackInfo* out_info);

// ABI3 rule: an unscoped search is forbidden because it can mix translations.
// The legacy symbol remains exported only to fail safely with LANGUAGE_REQUIRED.
MEMSLIBRARY_API std::int32_t MEMSLibrary_SearchPack(
    const wchar_t* pack_directory,
    const char* query_utf8,
    MEMSLibrarySearchResult* out_results,
    std::uint32_t result_capacity,
    std::uint32_t* out_result_count);

// Search exactly one source-language corpus. Language uses the RAVEMEMS V2
// canonical codes (fr, en, de, it, es, pt, nl, und). Results from another
// language can never enter this result set. A fallback must be a separate call.
MEMSLIBRARY_API std::int32_t MEMSLibrary_SearchPackByLanguage(
    const wchar_t* pack_directory,
    const char* source_language_utf8,
    const char* query_utf8,
    MEMSLibrarySearchResult* out_results,
    std::uint32_t result_capacity,
    std::uint32_t* out_result_count);

}
