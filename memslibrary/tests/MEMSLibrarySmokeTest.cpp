#include <windows.h>

#include "MEMSLibrary.h"

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace {
using GetAbiVersionFn = std::uint32_t (*)();
using GetTextFn = const char* (*)();
using ValidatePackFn = std::int32_t (*)(const wchar_t*, MEMSLibraryPackInfo*);
using SearchPackFn = std::int32_t (*)(const wchar_t*, const char*, MEMSLibrarySearchResult*, std::uint32_t, std::uint32_t*);
using SearchPackByLanguageFn = std::int32_t (*)(const wchar_t*, const char*, const char*, MEMSLibrarySearchResult*, std::uint32_t, std::uint32_t*);

std::wstring executableDirectory()
{
    wchar_t path[MAX_PATH] = {};
    const DWORD length = GetModuleFileNameW(nullptr, path, MAX_PATH);
    if (length == 0 || length >= MAX_PATH) return {};
    std::wstring full(path, length);
    const auto separator = full.find_last_of(L"\\/");
    return separator == std::wstring::npos ? std::wstring{} : full.substr(0, separator);
}

bool contains(const char* text, const char* needle)
{
    return text && needle && std::strstr(text, needle) != nullptr;
}

void reset(std::vector<MEMSLibrarySearchResult>& results)
{
    for (auto& r : results) {
        r = {};
        r.struct_size = sizeof(r);
    }
}

bool allLanguage(const std::vector<MEMSLibrarySearchResult>& results, std::uint32_t count, const char* language)
{
    for (std::uint32_t i = 0; i < count; ++i) {
        if (std::strcmp(results[i].source_language, language) != 0) return false;
    }
    return true;
}
}

int wmain(int argc, wchar_t** argv)
{
    if (argc != 3) {
        std::cerr << "usage: MEMSLibrarySmokeTest <good-pack-dir> <corrupt-pack-dir>\n";
        return 1;
    }
    const std::wstring directory = executableDirectory();
    const std::wstring dllPath = directory + L"\\MEMSLibrary.dll";
    HMODULE module = LoadLibraryW(dllPath.c_str());
    if (!module) {
        std::cerr << "FAIL LoadLibrary MEMSLibrary.dll error=" << GetLastError() << "\n";
        return 2;
    }

    const auto abi = reinterpret_cast<GetAbiVersionFn>(GetProcAddress(module, "MEMSLibrary_GetAbiVersion"));
    const auto name = reinterpret_cast<GetTextFn>(GetProcAddress(module, "MEMSLibrary_GetName"));
    const auto role = reinterpret_cast<GetTextFn>(GetProcAddress(module, "MEMSLibrary_GetEngineRole"));
    const auto validate = reinterpret_cast<ValidatePackFn>(GetProcAddress(module, "MEMSLibrary_ValidatePack"));
    const auto legacySearch = reinterpret_cast<SearchPackFn>(GetProcAddress(module, "MEMSLibrary_SearchPack"));
    const auto search = reinterpret_cast<SearchPackByLanguageFn>(GetProcAddress(module, "MEMSLibrary_SearchPackByLanguage"));
    if (!abi || !name || !role || !validate || !legacySearch || !search) {
        std::cerr << "FAIL required ABI3 export missing\n";
        FreeLibrary(module);
        return 3;
    }
    if (abi() != 3u || std::strcmp(name(), "MEMSLibrary") != 0 ||
        std::strcmp(role(), "generic_knowledge_library_engine") != 0) {
        std::cerr << "FAIL ABI3 identity mismatch\n";
        FreeLibrary(module);
        return 4;
    }

    MEMSLibraryPackInfo info{};
    info.struct_size = sizeof(info);
    const auto validStatus = validate(argv[1], &info);
    if (validStatus != MEMSLIBRARY_OK || info.schema_version < 2u || info.document_count != 47u ||
        info.source_database_count != 47u || std::strcmp(info.pack_id, "MEMSLibrary_Pack_001") != 0) {
        std::cerr << "FAIL Pack001 validation status=" << validStatus << " schema=" << info.schema_version
                  << " docs=" << info.document_count << " sources=" << info.source_database_count
                  << " id=" << info.pack_id << "\n";
        FreeLibrary(module);
        return 5;
    }

    std::vector<MEMSLibrarySearchResult> results(32);
    reset(results);
    std::uint32_t count = 0;

    const auto unsafeStatus = legacySearch(argv[1], "primary gear end float", results.data(),
                                           static_cast<std::uint32_t>(results.size()), &count);
    if (unsafeStatus != MEMSLIBRARY_LANGUAGE_REQUIRED || count != 0) {
        std::cerr << "FAIL unscoped search was not blocked status=" << unsafeStatus << " count=" << count << "\n";
        FreeLibrary(module);
        return 6;
    }

    reset(results);
    count = 0;
    auto searchStatus = search(argv[1], "en", "primary gear end float", results.data(),
                               static_cast<std::uint32_t>(results.size()), &count);
    bool foundPrimaryEnglish = false;
    for (std::uint32_t i = 0; i < count; ++i) {
        if (std::strcmp(results[i].document_key, "DOC_RCL0193ENG") == 0 && results[i].page_number == 53 &&
            (contains(results[i].body, "0.089") || contains(results[i].body, "0.165"))) {
            foundPrimaryEnglish = true;
        }
    }
    if (searchStatus != MEMSLIBRARY_OK || count == 0 || !allLanguage(results, count, "en") || !foundPrimaryEnglish) {
        std::cerr << "FAIL isolated English primary search status=" << searchStatus << " count=" << count << "\n";
        FreeLibrary(module);
        return 7;
    }

    reset(results);
    count = 0;
    searchStatus = search(argv[1], "fr", "jeu axial pignon primaire", results.data(),
                          static_cast<std::uint32_t>(results.size()), &count);
    bool foundPrimaryFrench = false;
    for (std::uint32_t i = 0; i < count; ++i) {
        if (std::strcmp(results[i].document_key, "DOC_RCL0193FRE") == 0 &&
            (contains(results[i].body, "0,089") || contains(results[i].body, "0,165") ||
             contains(results[i].title, "PIGNON PRIMAIRE"))) {
            foundPrimaryFrench = true;
        }
        if (contains(results[i].body, "coaxial") || contains(results[i].title, "coaxial")) {
            std::cerr << "FAIL French primary search contains coaxial false positive\n";
            FreeLibrary(module);
            return 8;
        }
    }
    if (searchStatus != MEMSLIBRARY_OK || count == 0 || !allLanguage(results, count, "fr") || !foundPrimaryFrench) {
        std::cerr << "FAIL isolated French primary search status=" << searchStatus << " count=" << count << "\n";
        FreeLibrary(module);
        return 9;
    }

    reset(results);
    count = 0;
    searchStatus = search(argv[1], "en", "axial", results.data(),
                          static_cast<std::uint32_t>(results.size()), &count);
    for (std::uint32_t i = 0; i < count; ++i) {
        if ((results[i].page_number == 342 || results[i].page_number == 343) &&
            (contains(results[i].body, "coaxial") || contains(results[i].title, "coaxial"))) {
            std::cerr << "FAIL token boundary: axial matched coaxial on p342/p343\n";
            FreeLibrary(module);
            return 10;
        }
    }
    if (searchStatus != MEMSLIBRARY_OK || !allLanguage(results, count, "en")) {
        std::cerr << "FAIL token-boundary English search status=" << searchStatus << " count=" << count << "\n";
        FreeLibrary(module);
        return 11;
    }

    reset(results);
    count = 0;
    const auto missingLanguageStatus = search(argv[1], "zz", "primary gear", results.data(),
                                              static_cast<std::uint32_t>(results.size()), &count);
    if (missingLanguageStatus != MEMSLIBRARY_LANGUAGE_NOT_FOUND || count != 0) {
        std::cerr << "FAIL unknown language was not rejected status=" << missingLanguageStatus << " count=" << count << "\n";
        FreeLibrary(module);
        return 12;
    }

    MEMSLibraryPackInfo badInfo{};
    badInfo.struct_size = sizeof(badInfo);
    const auto corruptStatus = validate(argv[2], &badInfo);
    if (corruptStatus == MEMSLIBRARY_OK) {
        std::cerr << "FAIL corrupt Pack002 unexpectedly accepted\n";
        FreeLibrary(module);
        return 13;
    }

    MEMSLibraryPackInfo recheck{};
    recheck.struct_size = sizeof(recheck);
    if (validate(argv[1], &recheck) != MEMSLIBRARY_OK || recheck.document_count != 47u) {
        std::cerr << "FAIL Pack001 unavailable after corrupt Pack002 test\n";
        FreeLibrary(module);
        return 14;
    }

    std::cout << "MEMSLIBRARY_PACK001_LANGUAGE_ISOLATION_PASS abi=3 schema=" << info.schema_version
              << " pack=" << info.pack_id << " documents=" << info.document_count
              << " en_primary=DOC_RCL0193ENG:p53 fr_primary=DOC_RCL0193FRE"
              << " unscoped_blocked=1 axial_not_coaxial=1 corrupt_pack_isolated=1\n";
    FreeLibrary(module);
    return 0;
}
