#include "IaMemsLibraryBridge.h"

#include "memslibrary/include/MEMSLibrary.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QLibrary>
#include <QProcessEnvironment>
#include <QSet>
#include <QVector>

#include <cstdint>
#include <cstring>
#include <string>

namespace {

using GetAbiVersionFn = std::uint32_t (*)();
using GetTextFn = const char* (*)();
using ValidatePackFn = std::int32_t (*)(const wchar_t*, MEMSLibraryPackInfo*);
using SearchPackFn = std::int32_t (*)(const wchar_t*, const char*, MEMSLibrarySearchResult*, std::uint32_t, std::uint32_t*);

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
    bool attempted = false;
    bool ready = false;
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

QStringList candidateQueries(const QString &question, const QStringList &keywords)
{
    QStringList candidates;
    appendUnique(candidates, question);

    QStringList clean;
    for (const QString &keyword : keywords) {
        const QString value = keyword.trimmed();
        if (value.size() >= 2 && !clean.contains(value, Qt::CaseInsensitive) && clean.size() < 8)
            clean.append(value);
    }

    if (!clean.isEmpty()) {
        appendUnique(candidates, clean.mid(0, qMin(4, clean.size())).join(QLatin1Char(' ')));
        if (clean.size() >= 3)
            appendUnique(candidates, clean.mid(0, 3).join(QLatin1Char(' ')));

        const int pairLimit = qMin(6, clean.size());
        for (int i = 0; i < pairLimit; ++i) {
            for (int j = i + 1; j < pairLimit; ++j)
                appendUnique(candidates, clean.at(i) + QLatin1Char(' ') + clean.at(j));
        }
        for (const QString &keyword : clean)
            appendUnique(candidates, keyword);
    }

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

QString resultKey(const MEMSLibrarySearchResult &result)
{
    return QStringLiteral("%1|%2|%3")
        .arg(QString::fromUtf8(result.document_key))
        .arg(result.page_number)
        .arg(QString::fromUtf8(result.entity_key));
}

QString evidenceText(const MEMSLibrarySearchResult &result)
{
    const QString document = QString::fromUtf8(result.document_key).trimmed();
    const QString title = QString::fromUtf8(result.title).trimmed();
    const QString body = QString::fromUtf8(result.body).trimmed();
    const QString page = result.page_number >= 0
        ? QStringLiteral("page %1").arg(result.page_number)
        : QStringLiteral("page non indiquée");

    QString line = QStringLiteral("Source %1, %2").arg(document, page);
    if (!title.isEmpty())
        line += QStringLiteral(" — %1").arg(title);
    if (!body.isEmpty())
        line += QStringLiteral("\n%1").arg(body);
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
    QSet<QString> seen;
    QStringList evidence;
    const std::wstring packWide = state.packDirectory.toStdWString();

    for (const QString &query : queries) {
        QVector<MEMSLibrarySearchResult> results(16);
        for (MEMSLibrarySearchResult &result : results)
            result.struct_size = sizeof(result);

        std::uint32_t count = 0;
        const QByteArray utf8 = query.toUtf8();
        const std::int32_t status = state.search(
            packWide.c_str(), utf8.constData(), results.data(),
            static_cast<std::uint32_t>(results.size()), &count);
        if (status != MEMSLIBRARY_OK)
            continue;

        for (std::uint32_t i = 0; i < count && evidence.size() < 6; ++i) {
            const QString key = resultKey(results.at(static_cast<int>(i)));
            if (seen.contains(key))
                continue;
            seen.insert(key);
            evidence.append(evidenceText(results.at(static_cast<int>(i))));
        }
        if (evidence.size() >= 3)
            break;
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
