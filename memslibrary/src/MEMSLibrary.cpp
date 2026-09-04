#include "MEMSLibrary.h"

#include <windows.h>
#include <winsqlite/winsqlite3.h>

#include <algorithm>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

namespace {
constexpr char kLibraryName[] = "MEMSLibrary";
constexpr char kEngineRole[] = "generic_knowledge_library_engine";
constexpr char kPackFormat[] = "MEMSLibraryKnowledgePack";

void copyText(char* dest, std::size_t capacity, const unsigned char* src)
{
    if (!dest || capacity == 0) return;
    dest[0] = '\0';
    if (!src) return;
    const char* text = reinterpret_cast<const char*>(src);
    std::strncpy(dest, text, capacity - 1);
    dest[capacity - 1] = '\0';
}

std::filesystem::path knowledgePath(const wchar_t* packDirectory)
{
    return std::filesystem::path(packDirectory) / L"knowledge.sqlite";
}

bool manifestExists(const wchar_t* packDirectory)
{
    std::error_code ec;
    return std::filesystem::is_regular_file(
        std::filesystem::path(packDirectory) / L"manifest.json", ec);
}

std::string utf8FromWide(const std::wstring& wide)
{
    if (wide.empty()) return {};
    const int needed = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (needed <= 1) return {};
    std::string result(static_cast<std::size_t>(needed), '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, result.data(), needed, nullptr, nullptr);
    result.resize(static_cast<std::size_t>(needed - 1));
    return result;
}

int openReadonly(const wchar_t* packDirectory, sqlite3** db)
{
    if (!packDirectory || !db) return MEMSLIBRARY_INVALID_ARGUMENT;
    *db = nullptr;
    if (!manifestExists(packDirectory)) return MEMSLIBRARY_PACK_NOT_FOUND;
    const auto path = knowledgePath(packDirectory);
    std::error_code ec;
    if (!std::filesystem::is_regular_file(path, ec)) return MEMSLIBRARY_PACK_NOT_FOUND;
    const std::string utf8 = utf8FromWide(path.wstring());
    if (utf8.empty()) return MEMSLIBRARY_SQLITE_OPEN_FAILED;
    const int rc = sqlite3_open_v2(utf8.c_str(), db, SQLITE_OPEN_READONLY | SQLITE_OPEN_NOMUTEX, nullptr);
    if (rc != SQLITE_OK) {
        if (*db) sqlite3_close(*db);
        *db = nullptr;
        return MEMSLIBRARY_SQLITE_OPEN_FAILED;
    }
    return MEMSLIBRARY_OK;
}

bool singleText(sqlite3* db, const char* sql, std::string* value)
{
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    const bool ok = sqlite3_step(stmt) == SQLITE_ROW;
    if (ok && value) {
        const auto* txt = sqlite3_column_text(stmt, 0);
        *value = txt ? reinterpret_cast<const char*>(txt) : "";
    }
    sqlite3_finalize(stmt);
    return ok;
}

bool hasForeignKeyFailures(sqlite3* db)
{
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, "PRAGMA foreign_key_check", -1, &stmt, nullptr) != SQLITE_OK) return true;
    const bool hasRow = sqlite3_step(stmt) == SQLITE_ROW;
    sqlite3_finalize(stmt);
    return hasRow;
}

std::vector<std::string> queryTerms(const char* query)
{
    std::vector<std::string> terms;
    std::string current;
    for (const unsigned char c : std::string(query ? query : "")) {
        if (std::isalnum(c) || c >= 0x80 || c == '-' || c == '_') {
            current.push_back(static_cast<char>(std::tolower(c)));
        } else if (!current.empty()) {
            terms.push_back(current);
            current.clear();
        }
    }
    if (!current.empty()) terms.push_back(current);
    terms.erase(std::remove_if(terms.begin(), terms.end(), [](const std::string& s){ return s.size() < 2; }), terms.end());
    if (terms.size() > 8) terms.resize(8);
    return terms;
}
}

std::uint32_t MEMSLibrary_GetAbiVersion()
{
    return MEMSLIBRARY_ABI_VERSION;
}

const char* MEMSLibrary_GetName()
{
    return kLibraryName;
}

const char* MEMSLibrary_GetEngineRole()
{
    return kEngineRole;
}

std::int32_t MEMSLibrary_ValidatePack(const wchar_t* packDirectory, MEMSLibraryPackInfo* outInfo)
{
    if (!outInfo || outInfo->struct_size != sizeof(MEMSLibraryPackInfo)) return MEMSLIBRARY_INVALID_ARGUMENT;
    sqlite3* db = nullptr;
    const int openStatus = openReadonly(packDirectory, &db);
    if (openStatus != MEMSLIBRARY_OK) return openStatus;

    std::string integrity;
    if (!singleText(db, "PRAGMA integrity_check", &integrity) || integrity != "ok" || hasForeignKeyFailures(db)) {
        sqlite3_close(db);
        return MEMSLIBRARY_PACK_INVALID;
    }

    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT pack_id,pack_format,pack_schema_version,document_count,source_database_count FROM memslibrary_pack LIMIT 1";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK || sqlite3_step(stmt) != SQLITE_ROW) {
        if (stmt) sqlite3_finalize(stmt);
        sqlite3_close(db);
        return MEMSLIBRARY_PACK_INVALID;
    }

    const auto* packId = sqlite3_column_text(stmt, 0);
    const auto* packFormat = sqlite3_column_text(stmt, 1);
    const int schema = sqlite3_column_int(stmt, 2);
    const int docs = sqlite3_column_int(stmt, 3);
    const int sources = sqlite3_column_int(stmt, 4);
    const bool valid = packFormat && std::strcmp(reinterpret_cast<const char*>(packFormat), kPackFormat) == 0 &&
        schema > 0 && docs > 0 && sources > 0;
    if (valid) {
        outInfo->schema_version = static_cast<std::uint32_t>(schema);
        outInfo->document_count = static_cast<std::uint32_t>(docs);
        outInfo->source_database_count = static_cast<std::uint32_t>(sources);
        copyText(outInfo->pack_id, sizeof(outInfo->pack_id), packId);
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return valid ? MEMSLIBRARY_OK : MEMSLIBRARY_PACK_INVALID;
}

std::int32_t MEMSLibrary_SearchPack(
    const wchar_t* packDirectory,
    const char* queryUtf8,
    MEMSLibrarySearchResult* outResults,
    std::uint32_t resultCapacity,
    std::uint32_t* outResultCount)
{
    if (!queryUtf8 || !outResults || resultCapacity == 0 || !outResultCount) return MEMSLIBRARY_INVALID_ARGUMENT;
    *outResultCount = 0;
    const auto terms = queryTerms(queryUtf8);
    if (terms.empty()) return MEMSLIBRARY_INVALID_ARGUMENT;

    sqlite3* db = nullptr;
    const int openStatus = openReadonly(packDirectory, &db);
    if (openStatus != MEMSLIBRARY_OK) return openStatus;

    std::string sql = "SELECT document_key,page_number,entity_kind,entity_key,title,body FROM memslibrary_search WHERE 1=1";
    for (std::size_t i = 0; i < terms.size(); ++i) sql += " AND search_text LIKE ?";
    sql += " ORDER BY CASE entity_kind WHEN 'step' THEN 0 WHEN 'requirement' THEN 1 WHEN 'notice' THEN 2 WHEN 'operation' THEN 3 WHEN 'section' THEN 4 ELSE 5 END, document_key, COALESCE(page_number,2147483647), entity_key LIMIT ?";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_close(db);
        return MEMSLIBRARY_QUERY_FAILED;
    }
    int bindIndex = 1;
    for (const auto& term : terms) {
        const std::string pattern = "%" + term + "%";
        sqlite3_bind_text(stmt, bindIndex++, pattern.c_str(), -1, SQLITE_TRANSIENT);
    }
    sqlite3_bind_int(stmt, bindIndex, static_cast<int>(resultCapacity));

    std::uint32_t count = 0;
    while (count < resultCapacity && sqlite3_step(stmt) == SQLITE_ROW) {
        auto& r = outResults[count];
        if (r.struct_size != sizeof(MEMSLibrarySearchResult)) {
            sqlite3_finalize(stmt);
            sqlite3_close(db);
            return MEMSLIBRARY_INVALID_ARGUMENT;
        }
        r.page_number = sqlite3_column_type(stmt, 1) == SQLITE_NULL ? -1 : sqlite3_column_int(stmt, 1);
        copyText(r.document_key, sizeof(r.document_key), sqlite3_column_text(stmt, 0));
        copyText(r.entity_kind, sizeof(r.entity_kind), sqlite3_column_text(stmt, 2));
        copyText(r.entity_key, sizeof(r.entity_key), sqlite3_column_text(stmt, 3));
        copyText(r.title, sizeof(r.title), sqlite3_column_text(stmt, 4));
        copyText(r.body, sizeof(r.body), sqlite3_column_text(stmt, 5));
        ++count;
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    *outResultCount = count;
    return MEMSLIBRARY_OK;
}
