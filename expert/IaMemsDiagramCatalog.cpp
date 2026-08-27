#include "IaMemsDiagramCatalog.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
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
        QStringLiteral("veux"), QStringLiteral("quel"), QStringLiteral("quelle"),
        QStringLiteral("brochage"), QStringLiteral("pinout"), QStringLiteral("connecteur"),
        QStringLiteral("connector"), QStringLiteral("prise"), QStringLiteral("cablage"),
        QStringLiteral("wiring"), QStringLiteral("broche"), QStringLiteral("broches"),
        QStringLiteral("pins"), QStringLiteral("mems"), QStringLiteral("ecu"),
        QStringLiteral("de"), QStringLiteral("du"), QStringLiteral("la"), QStringLiteral("le"),
        QStringLiteral("les"), QStringLiteral("un"), QStringLiteral("une"), QStringLiteral("des")
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

bool safeDeclaredPath(const QString &relativePath)
{
    if (relativePath.isEmpty() || relativePath == QStringLiteral("."))
        return false;
    if (QDir::isAbsolutePath(relativePath)
        || relativePath == QStringLiteral("..")
        || relativePath.startsWith(QStringLiteral("../")))
        return false;
    return relativePath.startsWith(QStringLiteral("images/"));
}

IaMemsDiagramSuggestion resolveDeclared(const QString &root,
                                        const QString &key,
                                        const QString &relativePath)
{
    IaMemsDiagramSuggestion suggestion;
    if (key.trimmed().isEmpty() || !safeDeclaredPath(relativePath))
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
    suggestion.relativePath = relativePath;
    suggestion.absolutePath = canonicalDiagram;
    return suggestion;
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
        // Do not offer a diagnostic socket diagram for an ECU connector request.
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
    const QString text = normalize(question);
    const bool diagramIntent = containsAny(text, {
        QStringLiteral("schema"), QStringLiteral("broch"), QStringLiteral("pinout"),
        QStringLiteral("connecteur"), QStringLiteral("connector"), QStringLiteral("prise"),
        QStringLiteral("cablage"), QStringLiteral("wiring"), QStringLiteral("socket"),
        QStringLiteral("pin "), QStringLiteral(" pins"), QStringLiteral("broche")
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
    // Current OBD/J1962 documentation is MEMS 1.9-specific. Keep an ambiguous
    // generation-free OBD request rejected rather than guessing a connector.
    if (asksObd && generation.isEmpty())
        return IaMemsDiagramSuggestion();

    const QString root = effectiveReferenceRoot(referenceRoot);
    QFile manifest(QDir(root).filePath(QStringLiteral("manifest.json")));
    if (!manifest.open(QIODevice::ReadOnly))
        return IaMemsDiagramSuggestion();

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(manifest.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject())
        return IaMemsDiagramSuggestion();

    const QJsonObject diagrams = document.object().value(QStringLiteral("diagrams")).toObject();
    const QStringList terms = significantTerms(text);

    IaMemsDiagramSuggestion best;
    int bestScore = -1;
    for (auto it = diagrams.constBegin(); it != diagrams.constEnd(); ++it) {
        const QString key = it.key().trimmed();
        const QString relativePath = QDir::cleanPath(it.value().toString().trimmed());
        const IaMemsDiagramSuggestion candidate = resolveDeclared(root, key, relativePath);
        if (!candidate.isValid())
            continue;

        const int score = candidateScore(text, generation, terms, key, relativePath);
        if (score > bestScore) {
            bestScore = score;
            best = candidate;
        }
    }

    // A future image only needs a manifest entry with a meaningful key/path;
    // no C++ case needs to be added. Require positive evidence to avoid opening
    // an unrelated picture for a vague question.
    if (bestScore < 8)
        return IaMemsDiagramSuggestion();
    return best;
}
