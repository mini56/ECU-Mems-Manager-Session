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
    auto appendTerm = [&terms](std::string term) {
        if (term.size() < 2) return;
        if (std::find(terms.begin(), terms.end(), term) == terms.end())
            terms.push_back(std::move(term));
    };

    for (const unsigned char c : std::string(query ? query : "")) {
        if (std::isalnum(c) || c >= 0x80 || c == '-' || c == '_') {
            current.push_back(static_cast<char>(c < 0x80 ? std::tolower(c) : c));
        } else if (!current.empty()) {
            appendTerm(current);
            current.clear();
        }
    }
    if (!current.empty()) appendTerm(current);
    if (terms.size() > 16) terms.resize(16);
    return terms;
}

int termWeight(const std::string& term)
{
    const bool technical = std::any_of(term.begin(), term.end(), [](unsigned char c) {
        return std::isdigit(c) || c == '-' || c == '_';
    });
    if (technical || term.size() >= 7) return 4;
    if (term.size() >= 5) return 3;
    if (term.size() >= 4) return 2;
    return 1;
}

std::string relevanceExpression(const std::vector<std::string>& terms)
{
    std::string expression = "(";
    for (std::size_t i = 0; i < terms.size(); ++i) {
        if (i != 0) expression += " + ";
        expression += "CASE WHEN search_text LIKE ?" + std::to_string(i + 1) + " THEN "
            + std::to_string(termWeight(terms[i])) + " ELSE 0 END";
    }
    expression += ")";
    return expression;
}

std::string anyTermPredicate(const std::vector<std::string>& terms)
{
    std::string predicate = "(";
    for (std::size_t i = 0; i < terms.size(); ++i) {
        if (i != 0) predicate += " OR ";
        predicate += "search_text LIKE ?" + std::to_string(i + 1);
    }
    predicate += ")";
    return predicate;
}

std::string allTermPredicate(const std::vector<std::string>& terms)
{
    std::string predicate = "(";
    for (std::size_t i = 0; i < terms.size(); ++i) {
        if (i != 0) predicate += " AND ";
        predicate += "search_text LIKE ?" + std::to_string(i + 1);
    }
    predicate += ")";
    return predicate;
}

bool terminatedField(const char* value, std::size_t capacity)
{
    return value && std::memchr(value, '\0', capacity) != nullptr;
}

bool bindText(sqlite3_stmt* stmt, int index, const std::string& value)
{
    return sqlite3_bind_text(stmt, index, value.c_str(), -1, SQLITE_TRANSIENT) == SQLITE_OK;
}

bool bindText(sqlite3_stmt* stmt, int index, const char* value)
{
    return sqlite3_bind_text(stmt, index, value, -1, SQLITE_TRANSIENT) == SQLITE_OK;
}

bool bindTermPatterns(sqlite3_stmt* stmt, const std::vector<std::string>& terms)
{
    for (std::size_t i = 0; i < terms.size(); ++i) {
        const std::string pattern = "%" + terms[i] + "%";
        if (!bindText(stmt, static_cast<int>(i + 1), pattern))
            return false;
    }
    return true;
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

    const std::string relevance = relevanceExpression(terms);
    std::string sql = "SELECT document_key,page_number,entity_kind,entity_key,title,body," + relevance
        + " AS relevance FROM memslibrary_search WHERE " + anyTermPredicate(terms)
        + " ORDER BY relevance DESC, "
          "CASE entity_kind WHEN 'step' THEN 0 WHEN 'requirement' THEN 1 WHEN 'notice' THEN 2 "
          "WHEN 'operation' THEN 3 WHEN 'section' THEN 4 ELSE 5 END, "
          "document_key, COALESCE(page_number,2147483647), entity_key LIMIT ?"
        + std::to_string(terms.size() + 1);

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_close(db);
        return MEMSLIBRARY_QUERY_FAILED;
    }
    if (!bindTermPatterns(stmt, terms)
        || sqlite3_bind_int(stmt, static_cast<int>(terms.size() + 1), static_cast<int>(resultCapacity)) != SQLITE_OK) {
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return MEMSLIBRARY_QUERY_FAILED;
    }

    std::uint32_t count = 0;
    int stepStatus = SQLITE_DONE;
    while (count < resultCapacity && (stepStatus = sqlite3_step(stmt)) == SQLITE_ROW) {
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

    const bool queryOk = stepStatus == SQLITE_DONE || count == resultCapacity;
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    if (!queryOk) return MEMSLIBRARY_QUERY_FAILED;

    *outResultCount = count;
    return MEMSLIBRARY_OK;
}

std::int32_t MEMSLibrary_SearchPackFiltered(
    const wchar_t* packDirectory,
    const char* queryUtf8,
    const MEMSLibrarySearchFilters* filters,
    MEMSLibrarySearchResultWithProvenance* outResults,
    std::uint32_t resultCapacity,
    std::uint32_t* outResultCount)
{
    if (!queryUtf8 || !outResults || resultCapacity == 0 || !outResultCount) return MEMSLIBRARY_INVALID_ARGUMENT;
    *outResultCount = 0;

    if (filters) {
        if (filters->struct_size != sizeof(MEMSLibrarySearchFilters) ||
            !terminatedField(filters->document_key, sizeof(filters->document_key)) ||
            !terminatedField(filters->revision_key, sizeof(filters->revision_key)) ||
            !terminatedField(filters->source_language, sizeof(filters->source_language)) ||
            !terminatedField(filters->entity_kind, sizeof(filters->entity_kind))) {
            return MEMSLIBRARY_INVALID_ARGUMENT;
        }
    }

    for (std::uint32_t i = 0; i < resultCapacity; ++i) {
        if (outResults[i].struct_size != sizeof(MEMSLibrarySearchResultWithProvenance))
            return MEMSLIBRARY_INVALID_ARGUMENT;
    }

    const auto terms = queryTerms(queryUtf8);
    if (terms.empty()) return MEMSLIBRARY_INVALID_ARGUMENT;

    sqlite3* db = nullptr;
    const int openStatus = openReadonly(packDirectory, &db);
    if (openStatus != MEMSLIBRARY_OK) return openStatus;

    const bool filterDocument = filters && filters->document_key[0] != '\0';
    const bool filterRevision = filters && filters->revision_key[0] != '\0';
    const bool filterLanguage = filters && filters->source_language[0] != '\0';
    const bool filterKind = filters && filters->entity_kind[0] != '\0';
    const bool exactVerification = filterDocument || filterRevision || filterLanguage || filterKind;

    const std::string relevance = relevanceExpression(terms);
    const std::string termPredicate = exactVerification ? allTermPredicate(terms) : anyTermPredicate(terms);
    std::string sql = "SELECT document_key,revision_key,source_language,page_number,entity_kind,entity_key,title,body,"
        + relevance + " AS relevance FROM memslibrary_search WHERE " + termPredicate;

    int nextParameter = static_cast<int>(terms.size() + 1);
    int documentParameter = 0;
    int revisionParameter = 0;
    int languageParameter = 0;
    int kindParameter = 0;

    if (filterDocument) {
        documentParameter = nextParameter++;
        sql += " AND document_key = ?" + std::to_string(documentParameter) + " COLLATE BINARY";
    }
    if (filterRevision) {
        revisionParameter = nextParameter++;
        sql += " AND revision_key = ?" + std::to_string(revisionParameter) + " COLLATE BINARY";
    }
    if (filterLanguage) {
        languageParameter = nextParameter++;
        sql += " AND source_language = ?" + std::to_string(languageParameter) + " COLLATE BINARY";
    }
    if (filterKind) {
        kindParameter = nextParameter++;
        sql += " AND entity_kind = ?" + std::to_string(kindParameter) + " COLLATE BINARY";
    }
    const int limitParameter = nextParameter;
    sql += " ORDER BY relevance DESC, "
           "CASE entity_kind WHEN 'step' THEN 0 WHEN 'requirement' THEN 1 WHEN 'notice' THEN 2 "
           "WHEN 'operation' THEN 3 WHEN 'section' THEN 4 ELSE 5 END, "
           "document_key, revision_key, COALESCE(page_number,2147483647), entity_key LIMIT ?"
        + std::to_string(limitParameter);

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_close(db);
        return MEMSLIBRARY_QUERY_FAILED;
    }

    bool bound = bindTermPatterns(stmt, terms);
    if (filterDocument) bound = bound && bindText(stmt, documentParameter, filters->document_key);
    if (filterRevision) bound = bound && bindText(stmt, revisionParameter, filters->revision_key);
    if (filterLanguage) bound = bound && bindText(stmt, languageParameter, filters->source_language);
    if (filterKind) bound = bound && bindText(stmt, kindParameter, filters->entity_kind);
    bound = bound && sqlite3_bind_int(stmt, limitParameter, static_cast<int>(resultCapacity)) == SQLITE_OK;
    if (!bound) {
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return MEMSLIBRARY_QUERY_FAILED;
    }

    std::uint32_t count = 0;
    int stepStatus = SQLITE_DONE;
    while (count < resultCapacity && (stepStatus = sqlite3_step(stmt)) == SQLITE_ROW) {
        auto& r = outResults[count];
        r.page_number = sqlite3_column_type(stmt, 3) == SQLITE_NULL ? -1 : sqlite3_column_int(stmt, 3);
        copyText(r.document_key, sizeof(r.document_key), sqlite3_column_text(stmt, 0));
        copyText(r.revision_key, sizeof(r.revision_key), sqlite3_column_text(stmt, 1));
        copyText(r.source_language, sizeof(r.source_language), sqlite3_column_text(stmt, 2));
        copyText(r.entity_kind, sizeof(r.entity_kind), sqlite3_column_text(stmt, 4));
        copyText(r.entity_key, sizeof(r.entity_key), sqlite3_column_text(stmt, 5));
        copyText(r.title, sizeof(r.title), sqlite3_column_text(stmt, 6));
        copyText(r.body, sizeof(r.body), sqlite3_column_text(stmt, 7));
        ++count;
    }

    const bool queryOk = stepStatus == SQLITE_DONE || count == resultCapacity;
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    if (!queryOk) return MEMSLIBRARY_QUERY_FAILED;

    *outResultCount = count;
    return MEMSLIBRARY_OK;
}
