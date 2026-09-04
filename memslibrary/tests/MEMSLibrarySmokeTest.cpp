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
    const auto search = reinterpret_cast<SearchPackFn>(GetProcAddress(module, "MEMSLibrary_SearchPack"));
    if (!abi || !name || !role || !validate || !search) {
        std::cerr << "FAIL required ABI2 export missing\n";
        FreeLibrary(module);
        return 3;
    }
    if (abi() != 2u || std::strcmp(name(), "MEMSLibrary") != 0 ||
        std::strcmp(role(), "generic_knowledge_library_engine") != 0) {
        std::cerr << "FAIL ABI identity mismatch\n";
        FreeLibrary(module);
        return 4;
    }

    MEMSLibraryPackInfo info{};
    info.struct_size = sizeof(info);
    const auto validStatus = validate(argv[1], &info);
    if (validStatus != MEMSLIBRARY_OK || info.document_count != 47u || info.source_database_count != 47u ||
        std::strcmp(info.pack_id, "MEMSLibrary_Pack_001") != 0) {
        std::cerr << "FAIL Pack001 validation status=" << validStatus << " docs=" << info.document_count << " sources=" << info.source_database_count << " id=" << info.pack_id << "\n";
        FreeLibrary(module);
        return 5;
    }

    std::vector<MEMSLibrarySearchResult> results(16);
    for (auto& r : results) r.struct_size = sizeof(r);
    std::uint32_t count = 0;
    auto searchStatus = search(argv[1], "primary gear end float", results.data(), static_cast<std::uint32_t>(results.size()), &count);
    bool foundPrimary = false;
    for (std::uint32_t i = 0; i < count; ++i) {
        if (std::strcmp(results[i].document_key, "RCL0193ENG") == 0 && results[i].page_number == 53 &&
            (contains(results[i].body, "0.089") || contains(results[i].body, "0.165"))) {
            foundPrimary = true;
        }
    }
    if (searchStatus != MEMSLIBRARY_OK || !foundPrimary) {
        std::cerr << "FAIL primary gear search status=" << searchStatus << " count=" << count << "\n";
        FreeLibrary(module);
        return 6;
    }

    for (auto& r : results) { r = {}; r.struct_size = sizeof(r); }
    count = 0;
    searchStatus = search(argv[1], "battery restoration procedure", results.data(), static_cast<std::uint32_t>(results.size()), &count);
    bool foundBattery = false;
    for (std::uint32_t i = 0; i < count; ++i) {
        if (std::strcmp(results[i].document_key, "RCL0221ENG") == 0 && results[i].page_number == 20) foundBattery = true;
    }
    if (searchStatus != MEMSLIBRARY_OK || !foundBattery) {
        std::cerr << "FAIL battery search status=" << searchStatus << " count=" << count << "\n";
        FreeLibrary(module);
        return 7;
    }

    MEMSLibraryPackInfo badInfo{};
    badInfo.struct_size = sizeof(badInfo);
    const auto corruptStatus = validate(argv[2], &badInfo);
    if (corruptStatus == MEMSLIBRARY_OK) {
        std::cerr << "FAIL corrupt Pack002 unexpectedly accepted\n";
        FreeLibrary(module);
        return 8;
    }

    MEMSLibraryPackInfo recheck{};
    recheck.struct_size = sizeof(recheck);
    if (validate(argv[1], &recheck) != MEMSLIBRARY_OK || recheck.document_count != 47u) {
        std::cerr << "FAIL Pack001 unavailable after corrupt Pack002 test\n";
        FreeLibrary(module);
        return 9;
    }

    std::cout << "MEMSLIBRARY_PACK001_PASS abi=2 pack=" << info.pack_id
              << " documents=" << info.document_count
              << " primary=RCL0193ENG:p53 battery=RCL0221ENG:p20 corrupt_pack_isolated=1\n";
    FreeLibrary(module);
    return 0;
}
