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

void resetResults(std::vector<MEMSLibrarySearchResult>& results)
{
    for (auto& r : results) {
        r = {};
        r.struct_size = sizeof(r);
    }
}

bool searchFinds(
    SearchPackFn search,
    const wchar_t* pack,
    const char* query,
    const char* expectedDocument,
    std::int32_t expectedPage,
    std::vector<MEMSLibrarySearchResult>& results,
    std::uint32_t* outCount)
{
    resetResults(results);
    std::uint32_t count = 0;
    const auto status = search(pack, query, results.data(), static_cast<std::uint32_t>(results.size()), &count);
    bool found = false;
    for (std::uint32_t i = 0; i < count; ++i) {
        if (std::strcmp(results[i].document_key, expectedDocument) == 0 && results[i].page_number == expectedPage) {
            found = true;
            break;
        }
    }
    std::cout << "RETRIEVAL query=\"" << query << "\" status=" << status << " count=" << count
              << " target=" << expectedDocument << ":p" << expectedPage << " found=" << (found ? 1 : 0) << "\n";
    const std::uint32_t preview = count < 5 ? count : 5;
    for (std::uint32_t i = 0; i < preview; ++i) {
        std::cout << "TOP" << (i + 1) << " doc=" << results[i].document_key
                  << " page=" << results[i].page_number
                  << " kind=" << results[i].entity_kind
                  << " title=\"" << results[i].title << "\"\n";
    }
    if (outCount) *outCount = count;
    return status == MEMSLIBRARY_OK && found;
}

void probeFrench(
    SearchPackFn search,
    const wchar_t* pack,
    const char* query,
    std::vector<MEMSLibrarySearchResult>& results)
{
    resetResults(results);
    std::uint32_t count = 0;
    const auto status = search(pack, query, results.data(), static_cast<std::uint32_t>(results.size()), &count);
    std::cout << "FRENCH_PROBE query=\"" << query << "\" status=" << status << " count=" << count << "\n";
    const std::uint32_t preview = count < 5 ? count : 5;
    for (std::uint32_t i = 0; i < preview; ++i) {
        std::cout << "FR_TOP" << (i + 1) << " doc=" << results[i].document_key
                  << " page=" << results[i].page_number
                  << " kind=" << results[i].entity_kind
                  << " title=\"" << results[i].title << "\"\n";
    }
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

    std::vector<MEMSLibrarySearchResult> results(32);
    std::uint32_t count = 0;

    if (!searchFinds(search, argv[1], "primary gear end float", "DOC_RCL0193ENG", 53, results, &count)) {
        std::cerr << "FAIL baseline primary gear retrieval\n";
        FreeLibrary(module);
        return 6;
    }
    bool primaryValueFound = false;
    for (std::uint32_t i = 0; i < count; ++i) {
        if (std::strcmp(results[i].document_key, "DOC_RCL0193ENG") == 0 && results[i].page_number == 53 &&
            (contains(results[i].body, "0.089") || contains(results[i].body, "0.165"))) {
            primaryValueFound = true;
            break;
        }
    }
    if (!primaryValueFound) {
        std::cerr << "FAIL baseline primary gear values missing\n";
        FreeLibrary(module);
        return 7;
    }

    if (!searchFinds(search, argv[1], "How do I check the primary gear end float?", "DOC_RCL0193ENG", 53, results, nullptr)) {
        std::cerr << "FAIL natural primary gear retrieval\n";
        FreeLibrary(module);
        return 8;
    }

    if (!searchFinds(search, argv[1], "battery restoration procedure", "DOC_RCL0221ENG", 20, results, nullptr)) {
        std::cerr << "FAIL baseline battery retrieval\n";
        FreeLibrary(module);
        return 9;
    }

    if (!searchFinds(search, argv[1], "How do I carry out the battery restoration procedure?", "DOC_RCL0221ENG", 20, results, nullptr)) {
        std::cerr << "FAIL natural battery retrieval\n";
        FreeLibrary(module);
        return 10;
    }

    probeFrench(search, argv[1], u8"Comment contrôler le jeu axial du pignon primaire ?", results);
    probeFrench(search, argv[1], u8"Comment effectuer la procédure de remise en état de la batterie ?", results);

    MEMSLibraryPackInfo badInfo{};
    badInfo.struct_size = sizeof(badInfo);
    const auto corruptStatus = validate(argv[2], &badInfo);
    if (corruptStatus == MEMSLIBRARY_OK) {
        std::cerr << "FAIL corrupt Pack002 unexpectedly accepted\n";
        FreeLibrary(module);
        return 11;
    }

    MEMSLibraryPackInfo recheck{};
    recheck.struct_size = sizeof(recheck);
    if (validate(argv[1], &recheck) != MEMSLIBRARY_OK || recheck.document_count != 47u) {
        std::cerr << "FAIL Pack001 unavailable after corrupt Pack002 test\n";
        FreeLibrary(module);
        return 12;
    }

    std::cout << "MEMSLIBRARY_RELEVANCE_FIX_PASS abi=2 pack=" << info.pack_id
              << " documents=" << info.document_count
              << " natural_primary=DOC_RCL0193ENG:p53 natural_battery=DOC_RCL0221ENG:p20 corrupt_pack_isolated=1\n";
    FreeLibrary(module);
    return 0;
}
