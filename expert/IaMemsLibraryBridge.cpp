#include "IaMemsLibraryBridge.h"

#include "memslibrary/include/MEMSLibrary.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QHash>
#include <QLibrary>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QSet>
#include <QVector>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <limits>
#include <string>

namespace {

using GetAbiVersionFn = std::uint32_t (*)();
using GetTextFn = const char* (*)();
using ValidatePackFn = std::int32_t (*)(const wchar_t*, MEMSLibraryPackInfo*);
using SearchPackFn = std::int32_t (*)(const wchar_t*, const char*, MEMSLibrarySearchResult*, std::uint32_t, std::uint32_t*);
using SearchPackFilteredFn = std::int32_t (*)(const wchar_t*, const char*, const MEMSLibrarySearchFilters*, MEMSLibrarySearchResultWithProvenance*, std::uint32_t, std::uint32_t*);

struct BridgeRuntime
{
    QLibrary library;
    QString packDirectory;
    QString error;
    GetAbiVersionFn abi = nullptr;
    GetTextFn name = nullptr;
    GetTextFn role = nullptr;
    ValidatePackFn validate = nullptr;
    SearchPackFn search = nullptr;
    SearchPackFilteredFn searchFiltered = nullptr;
    bool attempted = false;
    bool ready = false;
};

struct EvidenceCandidate
{
    QString document;
    QString revision;
    QString sourceLanguage;
    QString entityKind;
    QString entityKey;
    QString title;
    QString body;
    QString query;
    int page = -1;
    int score = -1;
};

struct EvidenceGroup
{
    QString document;
    QString revision;
    QString sourceLanguage;
    int page = -1;
    int bestScore = -1;
    int supportScore = 0;
    QVector<EvidenceCandidate> rows;
};

QString firstConfiguredPath(const QString &environmentName, const QString &fallback)
{
    const QString configured = QProcessEnvironment::systemEnvironment().value(environmentName).trimmed();
    return configured.isEmpty() ? fallback : configured;
}

void appendUnique(QStringList &items, const QString &value)
{
    const QString simplified = value.simplified();
    if (!simplified.isEmpty() && !items.contains(simplified, Qt::CaseInsensitive))
        items.append(simplified);
}

QString normalizeForMatching(const QString &input)
{
    const QString decomposed = input.normalized(QString::NormalizationForm_D).toCaseFolded();
    QString output;
    output.reserve(decomposed.size());
    bool previousSpace = true;

    for (const QChar ch : decomposed) {
        const QChar::Category category = ch.category();
        if (category == QChar::Mark_NonSpacing
            || category == QChar::Mark_SpacingCombining
            || category == QChar::Mark_Enclosing) {
            continue;
        }

        if (ch.isLetterOrNumber() || ch == QLatin1Char('.') || ch == QLatin1Char('_')) {
            output += ch;
            previousSpace = false;
        } else if (!previousSpace) {
            output += QLatin1Char(' ');
            previousSpace = true;
        }
    }
    return output.simplified();
}

QStringList normalizedTerms(const QString &text)
{
    return normalizeForMatching(text).split(QLatin1Char(' '), Qt::SkipEmptyParts);
}

bool containsExactToken(const QString &normalizedText, const QString &normalizedToken)
{
    if (normalizedText.isEmpty() || normalizedToken.isEmpty())
        return false;
    const QString padded = QStringLiteral(" ") + normalizedText + QStringLiteral(" ");
    return padded.contains(QStringLiteral(" ") + normalizedToken + QStringLiteral(" "));
}

bool isManufacturerReferenceAlias(const QString &value)
{
    static const QRegularExpression aliasRx(
        QStringLiteral("^\\d{1,3}(?:_\\d{1,3}){2,3}$"));
    return aliasRx.match(value).hasMatch();
}

QString dottedReference(const QString &alias)
{
    QString result = alias;
    result.replace(QLatin1Char('_'), QLatin1Char('.'));
    return result;
}

bool containsManufacturerReference(const EvidenceCandidate &result, const QString &alias)
{
    if (!isManufacturerReferenceAlias(alias))
        return false;

    const QString normalizedEntityKey = normalizeForMatching(result.entityKey);
    const QRegularExpression entityRx(
        QStringLiteral("(?:^|_)%1(?:_|$)").arg(QRegularExpression::escape(alias)));
    if (entityRx.match(normalizedEntityKey).hasMatch())
        return true;

    const QString normalizedContent = normalizeForMatching(
        result.title + QLatin1Char(' ') + result.body);
    return containsExactToken(normalizedContent, dottedReference(alias));
}

QStringList candidateQueries(const QString &question, const QStringList &keywords)
{
    QStringList candidates;
    appendUnique(candidates, question);

    QStringList clean;
    for (const QString &keyword : keywords) {
        const QString value = keyword.trimmed();
        if (value.size() >= 2 && !clean.contains(value, Qt::CaseInsensitive) && clean.size() < 10)
            clean.append(value);
    }

    if (clean.isEmpty())
        return candidates;

    // Semantic aliases and normalized manufacturer references are appended by
    // the service. Try their combined forms first, before broad single terms.
    if (clean.size() >= 3)
        appendUnique(candidates, clean.mid(clean.size() - 3, 3).join(QLatin1Char(' ')));
    if (clean.size() >= 2)
        appendUnique(candidates, clean.mid(clean.size() - 2, 2).join(QLatin1Char(' ')));

    appendUnique(candidates, clean.mid(0, qMin(4, clean.size())).join(QLatin1Char(' ')));
    if (clean.size() >= 3)
        appendUnique(candidates, clean.mid(0, 3).join(QLatin1Char(' ')));

    for (int i = clean.size() - 1; i >= 1; --i) {
        for (int j = i - 1; j >= 0; --j)
            appendUnique(candidates, clean.at(j) + QLatin1Char(' ') + clean.at(i));
    }

    for (int i = clean.size() - 1; i >= 0; --i)
        appendUnique(candidates, clean.at(i));

    return candidates;
}

BridgeRuntime &runtime()
{
    static BridgeRuntime state;
    if (state.attempted)
        return state;

    state.attempted = true;
    const QString root = QCoreApplication::applicationDirPath();
    const QString dllPath = firstConfiguredPath(
        QStringLiteral("MEMS_LIBRARY_DLL"),
        QDir(root).filePath(QStringLiteral("MEMSLibrary.dll")));
    state.packDirectory = firstConfiguredPath(
        QStringLiteral("MEMS_LIBRARY_PACK"),
        QDir(root).filePath(QStringLiteral("MEMSLibrary_Pack_001")));

    if (!QFileInfo::exists(dllPath) || !QFileInfo(dllPath).isFile()) {
        state.error = QStringLiteral("MEMSLibrary.dll absente.");
        return state;
    }
    if (!QFileInfo(state.packDirectory).isDir()) {
        state.error = QStringLiteral("MEMSLibrary_Pack_001 absent.");
        return state;
    }

    state.library.setFileName(dllPath);
    if (!state.library.load()) {
        state.error = QStringLiteral("Chargement MEMSLibrary.dll impossible : %1").arg(state.library.errorString());
        return state;
    }

    state.abi = reinterpret_cast<GetAbiVersionFn>(state.library.resolve("MEMSLibrary_GetAbiVersion"));
    state.name = reinterpret_cast<GetTextFn>(state.library.resolve("MEMSLibrary_GetName"));
    state.role = reinterpret_cast<GetTextFn>(state.library.resolve("MEMSLibrary_GetEngineRole"));
    state.validate = reinterpret_cast<ValidatePackFn>(state.library.resolve("MEMSLibrary_ValidatePack"));
    state.search = reinterpret_cast<SearchPackFn>(state.library.resolve("MEMSLibrary_SearchPack"));
    state.searchFiltered = reinterpret_cast<SearchPackFilteredFn>(
        state.library.resolve("MEMSLibrary_SearchPackFiltered"));

    // The additive export remains optional for backward compatibility. The
    // corrected runtime uses it; an older ABI2 DLL keeps the historical path.
    if (!state.abi || !state.name || !state.role || !state.validate || !state.search) {
        state.error = QStringLiteral("ABI2 MEMSLibrary incomplète.");
        return state;
    }
    if (state.abi() != MEMSLIBRARY_ABI_VERSION
        || std::strcmp(state.name(), "MEMSLibrary") != 0
        || std::strcmp(state.role(), "generic_knowledge_library_engine") != 0) {
        state.error = QStringLiteral("Identité ABI2 MEMSLibrary invalide.");
        return state;
    }

    MEMSLibraryPackInfo info{};
    info.struct_size = sizeof(info);
    const std::wstring packWide = state.packDirectory.toStdWString();
    const std::int32_t status = state.validate(packWide.c_str(), &info);
    if (status != MEMSLIBRARY_OK) {
        state.error = QStringLiteral("Validation du pack MEMSLibrary impossible (code %1).").arg(status);
        return state;
    }
    if (std::strcmp(info.pack_id, "MEMSLibrary_Pack_001") != 0 || info.document_count != 47u) {
        state.error = QStringLiteral("Le pack MEMSLibrary chargé n'est pas le Pack001 validé.");
        return state;
    }

    state.ready = true;
    return state;
}

QString resultKey(const EvidenceCandidate &result)
{
    return QStringLiteral("%1|%2|%3|%4|%5|%6")
        .arg(result.document, result.revision, result.sourceLanguage)
        .arg(result.page)
        .arg(result.entityKind, result.entityKey);
}

QString groupKey(const EvidenceCandidate &result)
{
    return QStringLiteral("%1|%2|%3|%4")
        .arg(result.document, result.revision, result.sourceLanguage)
        .arg(result.page);
}

QString searchableText(const EvidenceCandidate &result)
{
    return normalizeForMatching(QStringLiteral("%1 %2 %3 %4 %5 %6 %7")
        .arg(result.document, result.revision, result.entityKind, result.entityKey,
             result.title, result.body, result.sourceLanguage));
}

int evidenceScore(const EvidenceCandidate &result,
                  const QString &query,
                  const QStringList &keywords)
{
    const QString searchable = searchableText(result);
    const QStringList queryTerms = normalizedTerms(query);
    if (queryTerms.isEmpty())
        return -1;

    int referenceBoost = 0;
    // MEMSLibrary uses substring LIKE matching. Accept ordinary terms only as
    // exact tokens (rejecting axial/coaxial). Manufacturer references use the
    // generic underscore form present in RAVEMEMS entity keys.
    for (const QString &term : queryTerms) {
        if (isManufacturerReferenceAlias(term)) {
            if (!containsManufacturerReference(result, term))
                return -1;
            referenceBoost += 500;
            continue;
        }
        if (!containsExactToken(searchable, term))
            return -1;
    }

    int score = queryTerms.size() * 100 + referenceBoost;
    QSet<QString> scoredKeywords;
    for (const QString &keyword : keywords) {
        const QString normalized = normalizeForMatching(keyword);
        if (normalized.isEmpty() || scoredKeywords.contains(normalized))
            continue;
        scoredKeywords.insert(normalized);
        if (isManufacturerReferenceAlias(normalized)) {
            if (containsManufacturerReference(result, normalized))
                score += 24;
        } else if (containsExactToken(searchable, normalized)) {
            score += 12;
        }
    }

    if (result.entityKind == QStringLiteral("step"))
        score += 8;
    else if (result.entityKind == QStringLiteral("requirement"))
        score += 7;
    else if (result.entityKind == QStringLiteral("notice"))
        score += 6;
    else if (result.entityKind == QStringLiteral("operation"))
        score += 5;
    else if (result.entityKind == QStringLiteral("section"))
        score += 4;

    // For an explicit operation-reference lookup, keep the operation heading
    // ahead of its steps so Qwen receives the procedure identity as context.
    bool referenceQuery = false;
    for (const QString &term : queryTerms) {
        if (isManufacturerReferenceAlias(term)) {
            referenceQuery = true;
            break;
        }
    }
    if (referenceQuery && result.entityKind == QStringLiteral("operation"))
        score += 30;

    if (!result.title.trimmed().isEmpty())
        score += 2;
    return score;
}

void upsertCandidate(QVector<EvidenceCandidate> &candidates,
                     QHash<QString, int> &indices,
                     EvidenceCandidate candidate)
{
    const QString key = resultKey(candidate);
    const auto it = indices.constFind(key);
    if (it == indices.constEnd()) {
        indices.insert(key, candidates.size());
        candidates.append(candidate);
        return;
    }

    EvidenceCandidate &existing = candidates[*it];
    if (candidate.score > existing.score)
        existing = candidate;
}

EvidenceCandidate fromFilteredResult(const MEMSLibrarySearchResultWithProvenance &result)
{
    EvidenceCandidate candidate;
    candidate.document = QString::fromUtf8(result.document_key).trimmed();
    candidate.revision = QString::fromUtf8(result.revision_key).trimmed();
    candidate.sourceLanguage = QString::fromUtf8(result.source_language).trimmed();
    candidate.entityKind = QString::fromUtf8(result.entity_kind).trimmed();
    candidate.entityKey = QString::fromUtf8(result.entity_key).trimmed();
    candidate.title = QString::fromUtf8(result.title).trimmed();
    candidate.body = QString::fromUtf8(result.body).trimmed();
    candidate.page = result.page_number;
    return candidate;
}

EvidenceCandidate fromHistoricalResult(const MEMSLibrarySearchResult &result)
{
    EvidenceCandidate candidate;
    candidate.document = QString::fromUtf8(result.document_key).trimmed();
    candidate.entityKind = QString::fromUtf8(result.entity_kind).trimmed();
    candidate.entityKey = QString::fromUtf8(result.entity_key).trimmed();
    candidate.title = QString::fromUtf8(result.title).trimmed();
    candidate.body = QString::fromUtf8(result.body).trimmed();
    candidate.page = result.page_number;
    return candidate;
}

bool collectFilteredCandidates(BridgeRuntime &state,
                               const std::wstring &packWide,
                               const QStringList &queries,
                               const QStringList &keywords,
                               const MEMSLibrarySearchFilters *filters,
                               QVector<EvidenceCandidate> &candidates,
                               QHash<QString, int> &indices)
{
    if (!state.searchFiltered)
        return false;

    bool anySuccessfulQuery = false;
    for (const QString &query : queries) {
        QVector<MEMSLibrarySearchResultWithProvenance> results(24);
        for (MEMSLibrarySearchResultWithProvenance &result : results) {
            result = {};
            result.struct_size = sizeof(result);
        }

        std::uint32_t count = 0;
        const QByteArray utf8 = query.toUtf8();
        const std::int32_t status = state.searchFiltered(
            packWide.c_str(), utf8.constData(), filters, results.data(),
            static_cast<std::uint32_t>(results.size()), &count);
        if (status != MEMSLIBRARY_OK)
            continue;
        anySuccessfulQuery = true;

        for (std::uint32_t i = 0; i < count; ++i) {
            EvidenceCandidate candidate = fromFilteredResult(results.at(static_cast<int>(i)));
            candidate.query = query;
            candidate.score = evidenceScore(candidate, query, keywords);
            if (candidate.score < 0)
                continue;
            upsertCandidate(candidates, indices, candidate);
        }
    }
    return anySuccessfulQuery;
}

void collectHistoricalCandidates(BridgeRuntime &state,
                                 const std::wstring &packWide,
                                 const QStringList &queries,
                                 const QStringList &keywords,
                                 QVector<EvidenceCandidate> &candidates,
                                 QHash<QString, int> &indices)
{
    for (const QString &query : queries) {
        QVector<MEMSLibrarySearchResult> results(24);
        for (MEMSLibrarySearchResult &result : results) {
            result = {};
            result.struct_size = sizeof(result);
        }

        std::uint32_t count = 0;
        const QByteArray utf8 = query.toUtf8();
        const std::int32_t status = state.search(
            packWide.c_str(), utf8.constData(), results.data(),
            static_cast<std::uint32_t>(results.size()), &count);
        if (status != MEMSLIBRARY_OK)
            continue;

        for (std::uint32_t i = 0; i < count; ++i) {
            EvidenceCandidate candidate = fromHistoricalResult(results.at(static_cast<int>(i)));
            candidate.query = query;
            candidate.score = evidenceScore(candidate, query, keywords);
            if (candidate.score < 0)
                continue;
            upsertCandidate(candidates, indices, candidate);
        }
    }
}

bool betterCandidate(const EvidenceCandidate &left, const EvidenceCandidate &right)
{
    if (left.score != right.score)
        return left.score > right.score;
    if (left.entityKind != right.entityKind)
        return left.entityKind < right.entityKind;
    return left.entityKey < right.entityKey;
}

QVector<EvidenceGroup> buildGroups(const QVector<EvidenceCandidate> &candidates)
{
    QVector<EvidenceGroup> groups;
    QHash<QString, int> indices;

    for (const EvidenceCandidate &candidate : candidates) {
        const QString key = groupKey(candidate);
        auto it = indices.constFind(key);
        if (it == indices.constEnd()) {
            EvidenceGroup group;
            group.document = candidate.document;
            group.revision = candidate.revision;
            group.sourceLanguage = candidate.sourceLanguage;
            group.page = candidate.page;
            indices.insert(key, groups.size());
            groups.append(group);
            it = indices.constFind(key);
        }
        groups[*it].rows.append(candidate);
    }

    for (EvidenceGroup &group : groups) {
        std::sort(group.rows.begin(), group.rows.end(), betterCandidate);
        if (group.rows.isEmpty())
            continue;

        // A precise multi-term hit must outrank any volume of generic
        // single-term hits. Corroborating rows are only a tie-breaker after
        // the best individual evidence strength has been compared.
        group.bestScore = group.rows.first().score;
        const int limit = qMin(4, group.rows.size());
        for (int i = 1; i < limit; ++i)
            group.supportScore += group.rows.at(i).score;
        group.supportScore += limit * 5;
    }
    return groups;
}

bool betterGroup(const EvidenceGroup &left, const EvidenceGroup &right)
{
    if (left.bestScore != right.bestScore)
        return left.bestScore > right.bestScore;
    if (left.supportScore != right.supportScore)
        return left.supportScore > right.supportScore;
    if (left.rows.size() != right.rows.size())
        return left.rows.size() > right.rows.size();
    if (left.document != right.document)
        return left.document < right.document;
    if (left.revision != right.revision)
        return left.revision < right.revision;
    if (left.sourceLanguage != right.sourceLanguage)
        return left.sourceLanguage < right.sourceLanguage;

    const int leftPage = left.page >= 0 ? left.page : std::numeric_limits<int>::max();
    const int rightPage = right.page >= 0 ? right.page : std::numeric_limits<int>::max();
    return leftPage < rightPage;
}

bool sameProvenanceAndPage(const EvidenceCandidate &candidate, const EvidenceGroup &selected)
{
    return candidate.document == selected.document
        && candidate.revision == selected.revision
        && candidate.sourceLanguage == selected.sourceLanguage
        && candidate.page == selected.page;
}

template <std::size_t N>
void copyFilterText(char (&destination)[N], const QString &value)
{
    const QByteArray utf8 = value.toUtf8();
    const std::size_t sourceLength = static_cast<std::size_t>(utf8.size());
    const std::size_t copyLength = sourceLength < (N - 1) ? sourceLength : (N - 1);
    std::memcpy(destination, utf8.constData(), copyLength);
    destination[copyLength] = '\0';
}

QString evidenceText(const EvidenceCandidate &result)
{
    const QString page = result.page >= 0
        ? QStringLiteral("page %1").arg(result.page)
        : QStringLiteral("page non indiquée");

    QString line = QStringLiteral("Source %1, %2").arg(result.document, page);
    QStringList provenance;
    if (!result.revision.isEmpty())
        provenance.append(QStringLiteral("révision %1").arg(result.revision));
    if (!result.sourceLanguage.isEmpty())
        provenance.append(QStringLiteral("langue %1").arg(result.sourceLanguage));
    if (!result.entityKind.isEmpty())
        provenance.append(QStringLiteral("type %1").arg(result.entityKind));
    if (!provenance.isEmpty())
        line += QStringLiteral(" [%1]").arg(provenance.join(QStringLiteral(", ")));
    if (!result.title.isEmpty())
        line += QStringLiteral(" — %1").arg(result.title);
    if (!result.body.isEmpty())
        line += QStringLiteral("\n%1").arg(result.body);
    return line;
}

} // namespace

IaMemsLibraryGrounding IaMemsLibraryBridge::retrieve(const QString &question,
                                                      const QStringList &keywords)
{
    IaMemsLibraryGrounding output;
    BridgeRuntime &state = runtime();
    output.libraryReady = state.ready;
    output.error = state.error;
    if (!state.ready || question.trimmed().isEmpty())
        return output;

    const QStringList queries = candidateQueries(question, keywords);
    const std::wstring packWide = state.packDirectory.toStdWString();
    QVector<EvidenceCandidate> candidates;
    QHash<QString, int> indices;

    const bool filteredApiOperational = collectFilteredCandidates(
        state, packWide, queries, keywords, nullptr, candidates, indices);

    if (candidates.isEmpty()) {
        indices.clear();
        collectHistoricalCandidates(state, packWide, queries, keywords, candidates, indices);
    }
    if (candidates.isEmpty())
        return output;

    QVector<EvidenceGroup> groups = buildGroups(candidates);
    if (groups.isEmpty())
        return output;
    std::sort(groups.begin(), groups.end(), betterGroup);
    const EvidenceGroup selectedGroup = groups.first();

    output.selectedDocument = selectedGroup.document;
    output.selectedRevision = selectedGroup.revision;
    output.selectedSourceLanguage = selectedGroup.sourceLanguage;
    output.selectedPage = selectedGroup.page;

    QVector<EvidenceCandidate> finalRows = selectedGroup.rows;
    QHash<QString, int> finalIndices;
    for (int i = 0; i < finalRows.size(); ++i)
        finalIndices.insert(resultKey(finalRows.at(i)), i);

    if (filteredApiOperational && state.searchFiltered
        && !selectedGroup.document.isEmpty() && !selectedGroup.revision.isEmpty()
        && !selectedGroup.rows.isEmpty()) {
        MEMSLibrarySearchFilters filters{};
        filters.struct_size = sizeof(filters);
        copyFilterText(filters.document_key, selectedGroup.document);
        copyFilterText(filters.revision_key, selectedGroup.revision);
        if (!selectedGroup.sourceLanguage.isEmpty())
            copyFilterText(filters.source_language, selectedGroup.sourceLanguage);

        QVector<EvidenceCandidate> verification;
        QHash<QString, int> verificationIndices;
        const QString verificationQuery = selectedGroup.rows.first().query;
        const bool verificationQueryOk = collectFilteredCandidates(
            state, packWide, QStringList{verificationQuery}, keywords, &filters,
            verification, verificationIndices);

        bool selectedPageVerified = false;
        if (verificationQueryOk) {
            for (EvidenceCandidate candidate : verification) {
                if (!sameProvenanceAndPage(candidate, selectedGroup))
                    continue;
                selectedPageVerified = true;
                candidate.score += 40;
                upsertCandidate(finalRows, finalIndices, candidate);
            }
        }
        output.provenanceFiltered = selectedPageVerified;
    }

    // Critical isolation rule: once a physical page has won, every excerpt
    // handed to Qwen must come from that exact page. A same-document page is
    // not an acceptable substitute and cannot re-enter through broad queries.
    QVector<EvidenceCandidate> isolatedRows;
    for (const EvidenceCandidate &candidate : finalRows) {
        if (sameProvenanceAndPage(candidate, selectedGroup))
            isolatedRows.append(candidate);
    }
    if (isolatedRows.isEmpty())
        return output;

    std::sort(isolatedRows.begin(), isolatedRows.end(), betterCandidate);

    bool explicitReferenceLookup = false;
    if (!selectedGroup.rows.isEmpty()) {
        const QStringList selectedTerms = normalizedTerms(selectedGroup.rows.first().query);
        for (const QString &term : selectedTerms) {
            if (isManufacturerReferenceAlias(term)) {
                explicitReferenceLookup = true;
                break;
            }
        }
    }

    QStringList evidence;
    QSet<QString> seen;
    const int evidenceLimit = explicitReferenceLookup ? 12 : 6;
    for (const EvidenceCandidate &candidate : isolatedRows) {
        if (evidence.size() >= evidenceLimit)
            break;
        const QString key = resultKey(candidate);
        if (seen.contains(key))
            continue;
        seen.insert(key);
        evidence.append(evidenceText(candidate));
    }

    output.resultCount = evidence.size();
    if (evidence.isEmpty())
        return output;

    output.text = QStringLiteral(
        "Documentation RAVEMEMS retrouvée dans MEMSLibrary_Pack_001. "
        "Utiliser uniquement les extraits pertinents ci-dessous et conserver leur provenance ; "
        "si aucun extrait ne répond exactement à la question, ne pas extrapoler.\n%1")
        .arg(evidence.join(QStringLiteral("\n\n")));
    return output;
}
