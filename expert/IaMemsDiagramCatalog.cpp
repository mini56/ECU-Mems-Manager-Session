#include "IaMemsDiagramCatalog.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSet>
#include <QStringList>

namespace {

QString normalize(QString input)
{
    input = input.normalized(QString::NormalizationForm_D).toCaseFolded();
    QString output;
    bool previousSpace = true;

    for (const QChar ch : input) {
        const QChar::Category category = ch.category();
        if (category == QChar::Mark_NonSpacing
            || category == QChar::Mark_SpacingCombining
            || category == QChar::Mark_Enclosing)
            continue;

        if (ch.isLetterOrNumber() || ch == QLatin1Char('.')) {
            output += ch;
            previousSpace = false;
        } else if (!previousSpace) {
            output += QLatin1Char(' ');
            previousSpace = true;
        }
    }

    return output.simplified();
}

bool containsAny(const QString &text, const QStringList &needles)
{
    for (const QString &needle : needles) {
        if (text.contains(needle))
            return true;
    }
    return false;
}

QString requestedGeneration(const QString &text)
{
    for (const QString &generation : {QStringLiteral("1.2"), QStringLiteral("1.3"),
                                      QStringLiteral("1.6"), QStringLiteral("1.9")}) {
        QString compact = generation;
        compact.remove(QLatin1Char('.'));
        const QString spaced = QString(generation).replace(QLatin1Char('.'), QLatin1Char(' '));
        if (text.contains(generation)
            || text.contains(QStringLiteral("mems %1").arg(spaced))
            || text.contains(QStringLiteral("mems%1").arg(compact)))
            return generation;
    }
    return QString();
}

QStringList significantTerms(const QString &text)
{
    const QSet<QString> ignored = {
        QStringLiteral("schema"), QStringLiteral("diagramme"), QStringLiteral("diagram"),
        QStringLiteral("montre"), QStringLiteral("ouvrir"), QStringLiteral("ouvre"),
        QStringLiteral("voir"), QStringLiteral("veux"), QStringLiteral("quel"),
        QStringLiteral("quelle"), QStringLiteral("brochage"), QStringLiteral("pinout"),
        QStringLiteral("connecteur"), QStringLiteral("connector"), QStringLiteral("prise"),
        QStringLiteral("cablage"), QStringLiteral("wiring"), QStringLiteral("broche"),
        QStringLiteral("broches"), QStringLiteral("pins"), QStringLiteral("mems"),
        QStringLiteral("ecu"), QStringLiteral("de"), QStringLiteral("du"),
        QStringLiteral("la"), QStringLiteral("le"), QStringLiteral("les"),
        QStringLiteral("un"), QStringLiteral("une"), QStringLiteral("des"),
        QStringLiteral("est")
    };

    QStringList result;
    const QStringList words = text.split(QLatin1Char(' '), Qt::SkipEmptyParts);
    for (const QString &word : words) {
        if (word.size() < 2 || ignored.contains(word))
            continue;
        if (!result.contains(word))
            result.append(word);
    }
    return result;
}

QString effectiveReferenceRoot(const QString &referenceRoot)
{
    if (!referenceRoot.trimmed().isEmpty())
        return QDir::cleanPath(referenceRoot);
    return QDir(QCoreApplication::applicationDirPath())
        .filePath(QStringLiteral("database/reference"));
}

bool safeRelativePath(const QString &relativePath)
{
    if (relativePath.isEmpty() || relativePath == QStringLiteral("."))
        return false;
    if (QDir::isAbsolutePath(relativePath)
        || relativePath == QStringLiteral("..")
        || relativePath.startsWith(QStringLiteral("../")))
        return false;
    return true;
}

bool safeDeclaredPath(const QString &relativePath)
{
    return safeRelativePath(relativePath)
        && relativePath.startsWith(QStringLiteral("images/"));
}

IaMemsDiagramSuggestion resolvePath(const QString &root,
                                    const QString &key,
                                    const QString &relativePath,
                                    bool manifestPathOnly)
{
    IaMemsDiagramSuggestion suggestion;
    const bool safe = manifestPathOnly ? safeDeclaredPath(relativePath)
                                       : safeRelativePath(relativePath);
    if (key.trimmed().isEmpty() || !safe)
        return suggestion;

    const QFileInfo rootInfo(root);
    const QFileInfo diagramInfo(QDir(root).filePath(relativePath));
    if (!rootInfo.exists() || !rootInfo.isDir()
        || !diagramInfo.exists() || !diagramInfo.isFile())
        return suggestion;

    const QString canonicalRoot = rootInfo.canonicalFilePath();
    const QString canonicalDiagram = diagramInfo.canonicalFilePath();
    if (canonicalRoot.isEmpty() || canonicalDiagram.isEmpty())
        return suggestion;

    const QString relativeCheck = QDir(canonicalRoot).relativeFilePath(canonicalDiagram);
    if (relativeCheck == QStringLiteral("..")
        || relativeCheck.startsWith(QStringLiteral("../"))
        || QDir::isAbsolutePath(relativeCheck))
        return suggestion;

    suggestion.key = key.trimmed();
    suggestion.relativePath = QDir::cleanPath(relativePath);
    suggestion.absolutePath = canonicalDiagram;
    return suggestion;
}

bool fileMatchesSha256(const QString &absolutePath, const QString &expected)
{
    const QString normalizedExpected = expected.trimmed().toLower();
    if (normalizedExpected.isEmpty())
        return true;
    if (normalizedExpected.size() != 64)
        return false;

    QFile file(absolutePath);
    if (!file.open(QIODevice::ReadOnly))
        return false;
    QCryptographicHash hash(QCryptographicHash::Sha256);
    if (!hash.addData(&file))
        return false;
    return QString::fromLatin1(hash.result().toHex()) == normalizedExpected;
}

bool publicationOnlyTerms(const QStringList &terms)
{
    if (terms.size() != 1)
        return false;
    const QString t = terms.first();
    return (t.startsWith(QStringLiteral("rcl")) || t.startsWith(QStringLiteral("akm")))
        && t.size() >= 7;
}

int runtimeCandidateScore(const QString &question,
                          const QString &generation,
                          const QStringList &terms,
                          const QJsonObject &entry)
{
    const QString sourceType = normalize(entry.value(QStringLiteral("source_type")).toString());
    if (sourceType != QStringLiteral("ravemems") && sourceType != QStringLiteral("legacy"))
        return -1;
    if (!entry.value(QStringLiteral("ui_visible")).toBool(false))
        return -1;
    if (normalize(entry.value(QStringLiteral("ui_label")).toString()) != QStringLiteral("voir le schema"))
        return -1;
    if (sourceType == QStringLiteral("legacy")) {
        const QString treatment = normalize(entry.value(QStringLiteral("treatment")).toString());
        if (!treatment.isEmpty() && treatment != QStringLiteral("conserver migrer legacy"))
            return -1;
    }

    const QString searchable = normalize(
        entry.value(QStringLiteral("runtime_key")).toString() + QLatin1Char(' ')
        + entry.value(QStringLiteral("source_occurrence_key")).toString() + QLatin1Char(' ')
        + entry.value(QStringLiteral("asset_entity_key")).toString() + QLatin1Char(' ')
        + entry.value(QStringLiteral("publication_code")).toString() + QLatin1Char(' ')
        + QString::number(entry.value(QStringLiteral("physical_page")).toInt()) + QLatin1Char(' ')
        + entry.value(QStringLiteral("context_text")).toString() + QLatin1Char(' ')
        + entry.value(QStringLiteral("runtime_path")).toString() + QLatin1Char(' ')
        + entry.value(QStringLiteral("legacy_old_path")).toString());

    const bool asksRosco = question.contains(QStringLiteral("rosco"));
    const bool asksObd = question.contains(QStringLiteral("obd"))
        || question.contains(QStringLiteral("j1962"));
    if (asksRosco && !searchable.contains(QStringLiteral("rosco")))
        return -1;
    if (asksObd && !searchable.contains(QStringLiteral("obd"))
        && !searchable.contains(QStringLiteral("j1962")))
        return -1;

    int score = 0;
    int matched = 0;
    for (const QString &term : terms) {
        if (term == generation)
            continue;
        if (searchable.contains(term)) {
            ++matched;
            score += term.size() >= 4 ? 8 : 4;
        }
    }
    if (matched == 0)
        return -1;

    const QString publication = normalize(entry.value(QStringLiteral("publication_code")).toString());
    if (!publication.isEmpty() && question.contains(publication))
        score += 16;
    if (!generation.isEmpty() && searchable.contains(generation))
        score += 8;
    if (sourceType == QStringLiteral("ravemems"))
        score += 1;
    return score;
}

QString runtimeDisplayKey(const QJsonObject &entry)
{
    const QString publication = entry.value(QStringLiteral("publication_code")).toString().trimmed();
    const int page = entry.value(QStringLiteral("physical_page")).toInt();
    if (!publication.isEmpty() && page > 0)
        return QStringLiteral("%1 p.%2").arg(publication).arg(page);
    const QString runtimeKey = entry.value(QStringLiteral("runtime_key")).toString().trimmed();
    return runtimeKey;
}

IaMemsDiagramSuggestion resolvedRuntimeEntry(const QString &root, const QJsonObject &entry)
{
    const QString relativePath = QDir::cleanPath(
        entry.value(QStringLiteral("runtime_path")).toString().trimmed());
    const QString displayKey = runtimeDisplayKey(entry);
    const IaMemsDiagramSuggestion candidate = resolvePath(root, displayKey, relativePath, false);
    if (!candidate.isValid())
        return IaMemsDiagramSuggestion();
    if (!fileMatchesSha256(candidate.absolutePath,
                           entry.value(QStringLiteral("sha256")).toString()))
        return IaMemsDiagramSuggestion();
    return candidate;
}

IaMemsDiagramSuggestion runtimeSuggestionForResponse(const QString &response,
                                                      const QString &root)
{
    IaMemsDiagramSuggestion none;
    QString referenceText = response;
    referenceText.replace(QStringLiteral("\\:"), QStringLiteral(":"));
    referenceText.replace(QStringLiteral("\\_"), QStringLiteral("_"));
    referenceText.replace(QLatin1Char('\\'), QLatin1Char('/'));

    QFile catalog(QDir(root).filePath(QStringLiteral("runtime_visual_catalog.json")));
    if (!catalog.open(QIODevice::ReadOnly))
        return none;

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(catalog.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject())
        return none;

    const QJsonArray entries = document.object().value(QStringLiteral("entries")).toArray();

    const QRegularExpression referenceRx(
        QStringLiteral("(?:^|[^A-Za-z0-9_])rave\\s*:\\s*([A-Za-z0-9_-]+)\\s*:\\s*pdf\\s*:\\s*([0-9]+)"),
        QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatchIterator matches = referenceRx.globalMatch(referenceText);
    while (matches.hasNext()) {
        const QRegularExpressionMatch match = matches.next();
        const QString publication = match.captured(1);
        const int page = match.captured(2).toInt();
        for (const QJsonValue &value : entries) {
            if (!value.isObject())
                continue;
            const QJsonObject entry = value.toObject();
            if (!entry.value(QStringLiteral("ui_visible")).toBool(false))
                continue;
            if (entry.value(QStringLiteral("publication_code")).toString()
                    .compare(publication, Qt::CaseInsensitive) != 0)
                continue;
            if (entry.value(QStringLiteral("physical_page")).toInt() != page)
                continue;
            const IaMemsDiagramSuggestion candidate = resolvedRuntimeEntry(root, entry);
            if (candidate.isValid())
                return candidate;
        }
    }

    for (const QJsonValue &value : entries) {
        if (!value.isObject())
            continue;
        const QJsonObject entry = value.toObject();
        if (!entry.value(QStringLiteral("ui_visible")).toBool(false))
            continue;

        QString runtimePath = entry.value(QStringLiteral("runtime_path")).toString().trimmed();
        runtimePath.replace(QLatin1Char('\\'), QLatin1Char('/'));
        const QString runtimeKey = entry.value(QStringLiteral("runtime_key")).toString().trimmed();
        const QString occurrenceKey = entry.value(QStringLiteral("source_occurrence_key")).toString().trimmed();
        const QString assetKey = entry.value(QStringLiteral("asset_entity_key")).toString().trimmed();

        const bool referenced = (!runtimePath.isEmpty()
                                 && referenceText.contains(runtimePath, Qt::CaseInsensitive))
            || (!runtimeKey.isEmpty()
                && referenceText.contains(runtimeKey, Qt::CaseInsensitive))
            || (!occurrenceKey.isEmpty()
                && referenceText.contains(occurrenceKey, Qt::CaseInsensitive))
            || (!assetKey.isEmpty()
                && referenceText.contains(assetKey, Qt::CaseInsensitive));
        if (!referenced)
            continue;

        const IaMemsDiagramSuggestion candidate = resolvedRuntimeEntry(root, entry);
        if (candidate.isValid())
            return candidate;
    }

    QFile manifest(QDir(root).filePath(QStringLiteral("manifest.json")));
    if (manifest.open(QIODevice::ReadOnly)) {
        QJsonParseError manifestError;
        const QJsonDocument manifestDocument =
            QJsonDocument::fromJson(manifest.readAll(), &manifestError);
        if (manifestError.error == QJsonParseError::NoError && manifestDocument.isObject()) {
            const QJsonObject diagrams =
                manifestDocument.object().value(QStringLiteral("diagrams")).toObject();

            QStringList requestedPaths;
            const QRegularExpression pathRx(
                QStringLiteral("(images/[A-Za-z0-9_./-]+\\.(?:png|svg|jpe?g|webp))"),
                QRegularExpression::CaseInsensitiveOption);
            QRegularExpressionMatchIterator pathMatches = pathRx.globalMatch(referenceText);
            while (pathMatches.hasNext())
                requestedPaths.append(QDir::cleanPath(pathMatches.next().captured(1)));

            const QRegularExpression structuredRx(
                QStringLiteral("(?:^|[^A-Za-z0-9_])rave\\s*:\\s*([A-Za-z0-9_-]+)\\s*:\\s*pdf\\s*:\\s*([0-9]+)"),
                QRegularExpression::CaseInsensitiveOption);
            QRegularExpressionMatchIterator structuredMatches = structuredRx.globalMatch(referenceText);
            while (structuredMatches.hasNext()) {
                const QRegularExpressionMatch match = structuredMatches.next();
                requestedPaths.append(QStringLiteral("images/rave/%1_PDF_%2.png")
                    .arg(match.captured(1))
                    .arg(match.captured(2).toInt(), 3, 10, QLatin1Char('0')));
            }

            for (const QString &requestedPath : requestedPaths) {
                for (auto it = diagrams.constBegin(); it != diagrams.constEnd(); ++it) {
                    const QString declaredPath = QDir::cleanPath(it.value().toString().trimmed());
                    if (declaredPath.compare(requestedPath, Qt::CaseInsensitive) != 0)
                        continue;
                    const IaMemsDiagramSuggestion candidate =
                        resolvePath(root, it.key().trimmed(), declaredPath, true);
                    if (candidate.isValid())
                        return candidate;
                }
            }
        }
    }

    return none;
}

IaMemsDiagramSuggestion runtimeSuggestion(const QString &question,
                                           const QString &generation,
                                           const QStringList &terms,
                                           const QString &root)
{
    IaMemsDiagramSuggestion best;
    QStringList runtimeTerms = terms;
    if (!generation.isEmpty())
        runtimeTerms.removeAll(generation);
    if (runtimeTerms.isEmpty() || publicationOnlyTerms(runtimeTerms))
        return best;

    QFile catalog(QDir(root).filePath(QStringLiteral("runtime_visual_catalog.json")));
    if (!catalog.open(QIODevice::ReadOnly))
        return best;

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(catalog.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject())
        return best;

    const QJsonArray entries = document.object().value(QStringLiteral("entries")).toArray();
    int bestScore = -1;
    QString bestStableKey;
    for (const QJsonValue &value : entries) {
        if (!value.isObject())
            continue;
        const QJsonObject entry = value.toObject();
        const int score = runtimeCandidateScore(question, generation, runtimeTerms, entry);
        if (score < 8)
            continue;

        const QString stableKey = entry.value(QStringLiteral("runtime_key")).toString();
        if (score < bestScore || (score == bestScore && !bestStableKey.isEmpty()
                                  && stableKey >= bestStableKey))
            continue;

        const IaMemsDiagramSuggestion candidate = resolvedRuntimeEntry(root, entry);
        if (!candidate.isValid())
            continue;

        bestScore = score;
        bestStableKey = stableKey;
        best = candidate;
    }
    return best;
}

int candidateScore(const QString &question,
                   const QString &generation,
                   const QStringList &terms,
                   const QString &key,
                   const QString &relativePath)
{
    const QString searchable = normalize(key + QLatin1Char(' ') + relativePath);
    int score = 0;

    if (!generation.isEmpty()) {
        if (!searchable.contains(generation))
            return -1;
        score += 12;
    }

    const bool asksRosco = question.contains(QStringLiteral("rosco"));
    const bool asksObd = question.contains(QStringLiteral("obd"))
        || question.contains(QStringLiteral("j1962"));

    if (asksRosco) {
        if (!searchable.contains(QStringLiteral("rosco")))
            return -1;
        score += 30;
    } else if (asksObd) {
        if (!searchable.contains(QStringLiteral("obd"))
            && !searchable.contains(QStringLiteral("j1962")))
            return -1;
        score += 30;
    } else {
        if (searchable.contains(QStringLiteral("rosco"))
            || searchable.contains(QStringLiteral("obd"))
            || searchable.contains(QStringLiteral("j1962")))
            score -= 20;
    }

    for (const QString &term : terms) {
        if (searchable.contains(term))
            score += term.size() >= 4 ? 6 : 3;
    }

    return score;
}

} // namespace

IaMemsDiagramSuggestion IaMemsDiagramCatalog::suggestionForQuestion(
    const QString &question,
    const QString &referenceRoot)
{
    const QString root = effectiveReferenceRoot(referenceRoot);
    const IaMemsDiagramSuggestion referenced = runtimeSuggestionForResponse(question, root);
    if (referenced.isValid())
        return referenced;

    const QString text = normalize(question);
    const bool diagramIntent = containsAny(text, {
        QStringLiteral("schema"), QStringLiteral("broch"), QStringLiteral("pinout"),
        QStringLiteral("connecteur"), QStringLiteral("connector"), QStringLiteral("prise"),
        QStringLiteral("cablage"), QStringLiteral("wiring"), QStringLiteral("socket"),
        QStringLiteral("pin "), QStringLiteral(" pins"), QStringLiteral("broche"),
        QStringLiteral("voir")
    });
    if (!diagramIntent)
        return IaMemsDiagramSuggestion();

    const bool asksRosco = text.contains(QStringLiteral("rosco"));
    const bool asksObd = text.contains(QStringLiteral("obd"))
        || text.contains(QStringLiteral("j1962"));
    const bool genericDiagnostic = text.contains(QStringLiteral("diagnostic"))
        && !asksRosco && !asksObd;
    if (genericDiagnostic)
        return IaMemsDiagramSuggestion();

    const QString generation = requestedGeneration(text);
    if (asksObd && generation.isEmpty())
        return IaMemsDiagramSuggestion();

    const QStringList terms = significantTerms(text);

    const IaMemsDiagramSuggestion runtime = runtimeSuggestion(text, generation, terms, root);
    if (runtime.isValid())
        return runtime;

    QFile manifest(QDir(root).filePath(QStringLiteral("manifest.json")));
    if (!manifest.open(QIODevice::ReadOnly))
        return IaMemsDiagramSuggestion();

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(manifest.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject())
        return IaMemsDiagramSuggestion();

    const QJsonObject diagrams = document.object().value(QStringLiteral("diagrams")).toObject();

    IaMemsDiagramSuggestion best;
    int bestScore = -1;
    for (auto it = diagrams.constBegin(); it != diagrams.constEnd(); ++it) {
        const QString key = it.key().trimmed();
        const QString relativePath = QDir::cleanPath(it.value().toString().trimmed());
        const IaMemsDiagramSuggestion candidate = resolvePath(root, key, relativePath, true);
        if (!candidate.isValid())
            continue;

        const int score = candidateScore(text, generation, terms, key, relativePath);
        if (score > bestScore) {
            bestScore = score;
            best = candidate;
        }
    }

    if (bestScore < 8)
        return IaMemsDiagramSuggestion();
    return best;
}
