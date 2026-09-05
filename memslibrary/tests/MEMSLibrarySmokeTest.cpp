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
using SearchPackFilteredFn = std::int32_t (*)(const wchar_t*, const char*, const MEMSLibrarySearchFilters*, MEMSLibrarySearchResultWithProvenance*, std::uint32_t, std::uint32_t*);

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

template <std::size_t N>
void setText(char (&dest)[N], const char* source)
{
    dest[0] = '\0';
    if (!source) return;
    std::strncpy(dest, source, N - 1);
    dest[N - 1] = '\0';
}

void resetResults(std::vector<MEMSLibrarySearchResult>& results)
{
    for (auto& r : results) {
        r = {};
        r.struct_size = sizeof(r);
    }
}

void resetProvenanceResults(std::vector<MEMSLibrarySearchResultWithProvenance>& results)
{
    for (auto& r : results) {
        r = {};
        r.struct_size = sizeof(r);
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
    const auto searchFiltered = reinterpret_cast<SearchPackFilteredFn>(GetProcAddress(module, "MEMSLibrary_SearchPackFiltered"));
    if (!abi || !name || !role || !validate || !search || !searchFiltered) {
        std::cerr << "FAIL required ABI2/provenance export missing\n";
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

    // Historical ABI2 search must stay byte-contract compatible and operational.
    std::vector<MEMSLibrarySearchResult> results(16);
    resetResults(results);
    std::uint32_t count = 0;
    auto searchStatus = search(argv[1], "primary gear end float", results.data(), static_cast<std::uint32_t>(results.size()), &count);
    bool foundPrimary = false;
    for (std::uint32_t i = 0; i < count; ++i) {
        if (std::strcmp(results[i].document_key, "DOC_RCL0193ENG") == 0 && results[i].page_number == 53 &&
            (contains(results[i].body, "0.089") || contains(results[i].body, "0.165"))) {
            foundPrimary = true;
        }
    }
    if (searchStatus != MEMSLIBRARY_OK || !foundPrimary) {
        std::cerr << "FAIL historical primary gear search status=" << searchStatus << " count=" << count << "\n";
        FreeLibrary(module);
        return 6;
    }

    resetResults(results);
    count = 0;
    searchStatus = search(argv[1], "battery restoration procedure", results.data(), static_cast<std::uint32_t>(results.size()), &count);
    bool foundBattery = false;
    for (std::uint32_t i = 0; i < count; ++i) {
        if (std::strcmp(results[i].document_key, "DOC_RCL0221ENG") == 0 && results[i].page_number == 20) foundBattery = true;
    }
    if (searchStatus != MEMSLIBRARY_OK || !foundBattery) {
        std::cerr << "FAIL historical battery search status=" << searchStatus << " count=" << count << "\n";
        FreeLibrary(module);
        return 7;
    }

    // New additive search: exact document + language constraints and returned provenance.
    std::vector<MEMSLibrarySearchResultWithProvenance> provenanceResults(16);
    resetProvenanceResults(provenanceResults);
    MEMSLibrarySearchFilters filters{};
    filters.struct_size = sizeof(filters);
    setText(filters.document_key, "DOC_RCL0193ENG");
    setText(filters.source_language, "en");

    count = 0;
    auto filteredStatus = searchFiltered(argv[1], "primary gear end float", &filters,
        provenanceResults.data(), static_cast<std::uint32_t>(provenanceResults.size()), &count);
    bool foundFilteredPrimary = false;
    bool foundWrongPage342 = false;
    std::string correctRevision;
    std::string correctKind;
    for (std::uint32_t i = 0; i < count; ++i) {
        const auto& r = provenanceResults[i];
        if (std::strcmp(r.document_key, "DOC_RCL0193ENG") != 0 || std::strcmp(r.source_language, "en") != 0) {
            std::cerr << "FAIL filtered result escaped document/language constraint\n";
            FreeLibrary(module);
            return 8;
        }
        if (r.page_number == 342) foundWrongPage342 = true;
        if (r.page_number == 53 && (contains(r.body, "0.089") || contains(r.body, "0.165"))) {
            foundFilteredPrimary = true;
            correctRevision = r.revision_key;
            correctKind = r.entity_kind;
        }
    }
    if (filteredStatus != MEMSLIBRARY_OK || !foundFilteredPrimary || foundWrongPage342 || correctRevision.empty() || correctKind.empty()) {
        std::cerr << "FAIL filtered primary provenance status=" << filteredStatus << " count=" << count
                  << " found=" << foundFilteredPrimary << " p342=" << foundWrongPage342
                  << " revision=" << correctRevision << " kind=" << correctKind << "\n";
        FreeLibrary(module);
        return 9;
    }

    // A wrong document must not leak a result from RCL0193ENG.
    resetProvenanceResults(provenanceResults);
    MEMSLibrarySearchFilters wrongDocument{};
    wrongDocument.struct_size = sizeof(wrongDocument);
    setText(wrongDocument.document_key, "DOC_RCL0221ENG");
    setText(wrongDocument.source_language, "en");
    count = 0;
    filteredStatus = searchFiltered(argv[1], "primary gear end float", &wrongDocument,
        provenanceResults.data(), static_cast<std::uint32_t>(provenanceResults.size()), &count);
    if (filteredStatus != MEMSLIBRARY_OK || count != 0) {
        std::cerr << "FAIL wrong-document isolation status=" << filteredStatus << " count=" << count << "\n";
        FreeLibrary(module);
        return 10;
    }

    // A wrong language must not silently fall back to another language.
    resetProvenanceResults(provenanceResults);
    MEMSLibrarySearchFilters wrongLanguage{};
    wrongLanguage.struct_size = sizeof(wrongLanguage);
    setText(wrongLanguage.document_key, "DOC_RCL0193ENG");
    setText(wrongLanguage.source_language, "fr");
    count = 0;
    filteredStatus = searchFiltered(argv[1], "primary gear end float", &wrongLanguage,
        provenanceResults.data(), static_cast<std::uint32_t>(provenanceResults.size()), &count);
    if (filteredStatus != MEMSLIBRARY_OK || count != 0) {
        std::cerr << "FAIL wrong-language isolation status=" << filteredStatus << " count=" << count << "\n";
        FreeLibrary(module);
        return 11;
    }

    // A wrong revision must not silently fall back to another revision.
    resetProvenanceResults(provenanceResults);
    MEMSLibrarySearchFilters wrongRevision{};
    wrongRevision.struct_size = sizeof(wrongRevision);
    setText(wrongRevision.document_key, "DOC_RCL0193ENG");
    setText(wrongRevision.source_language, "en");
    setText(wrongRevision.revision_key, "REVISION_DOES_NOT_EXIST");
    count = 0;
    filteredStatus = searchFiltered(argv[1], "primary gear end float", &wrongRevision,
        provenanceResults.data(), static_cast<std::uint32_t>(provenanceResults.size()), &count);
    if (filteredStatus != MEMSLIBRARY_OK || count != 0) {
        std::cerr << "FAIL wrong-revision isolation status=" << filteredStatus << " count=" << count << "\n";
        FreeLibrary(module);
        return 12;
    }

    // Reapply the exact returned revision and entity kind: provenance must remain stable.
    resetProvenanceResults(provenanceResults);
    MEMSLibrarySearchFilters exactContext{};
    exactContext.struct_size = sizeof(exactContext);
    setText(exactContext.document_key, "DOC_RCL0193ENG");
    setText(exactContext.source_language, "en");
    setText(exactContext.revision_key, correctRevision.c_str());
    setText(exactContext.entity_kind, correctKind.c_str());
    count = 0;
    filteredStatus = searchFiltered(argv[1], "primary gear end float", &exactContext,
        provenanceResults.data(), static_cast<std::uint32_t>(provenanceResults.size()), &count);
    bool exactPrimary = false;
    for (std::uint32_t i = 0; i < count; ++i) {
        const auto& r = provenanceResults[i];
        if (std::strcmp(r.document_key, exactContext.document_key) != 0 ||
            std::strcmp(r.revision_key, exactContext.revision_key) != 0 ||
            std::strcmp(r.source_language, exactContext.source_language) != 0 ||
            std::strcmp(r.entity_kind, exactContext.entity_kind) != 0) {
            std::cerr << "FAIL exact-context provenance escaped constraint\n";
            FreeLibrary(module);
            return 13;
        }
        if (r.page_number == 53 && (contains(r.body, "0.089") || contains(r.body, "0.165"))) exactPrimary = true;
        if (r.page_number == 342) foundWrongPage342 = true;
    }
    if (filteredStatus != MEMSLIBRARY_OK || !exactPrimary || foundWrongPage342) {
        std::cerr << "FAIL exact-context primary status=" << filteredStatus << " count=" << count
                  << " primary=" << exactPrimary << " p342=" << foundWrongPage342 << "\n";
        FreeLibrary(module);
        return 14;
    }

    MEMSLibraryPackInfo badInfo{};
    badInfo.struct_size = sizeof(badInfo);
    const auto corruptStatus = validate(argv[2], &badInfo);
    if (corruptStatus == MEMSLIBRARY_OK) {
        std::cerr << "FAIL corrupt Pack002 unexpectedly accepted\n";
        FreeLibrary(module);
        return 15;
    }

    MEMSLibraryPackInfo recheck{};
    recheck.struct_size = sizeof(recheck);
    if (validate(argv[1], &recheck) != MEMSLIBRARY_OK || recheck.document_count != 47u) {
        std::cerr << "FAIL Pack001 unavailable after corrupt Pack002 test\n";
        FreeLibrary(module);
        return 16;
    }

    std::cout << "MEMSLIBRARY_PROVENANCE_PASS abi=2 pack=" << info.pack_id
              << " documents=" << info.document_count
              << " historical_primary=DOC_RCL0193ENG:p53 historical_battery=DOC_RCL0221ENG:p20"
              << " filtered_primary=DOC_RCL0193ENG:p53 language=en revision=" << correctRevision
              << " wrong_document=0 wrong_language=0 wrong_revision=0 p342=0 corrupt_pack_isolated=1\n";
    FreeLibrary(module);
    return 0;
}
