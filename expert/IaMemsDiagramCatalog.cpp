#include "IaMemsDiagramCatalog.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
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

bool hasGeneration(const QString &text, const QString &generation)
{
    const QString spaced = QString(generation).replace(QLatin1Char('.'), QLatin1Char(' '));
    QString compact = generation;
    compact.remove(QLatin1Char('.'));
    return text.contains(generation)
        || text.contains(QStringLiteral("mems %1").arg(spaced))
        || text.contains(QStringLiteral("mems%1").arg(compact));
}

QString requestedDiagramKey(const QString &question)
{
    const QString text = normalize(question);
    const bool diagramIntent = containsAny(text, {
        QStringLiteral("schema"), QStringLiteral("broch"), QStringLiteral("pinout"),
        QStringLiteral("connecteur"), QStringLiteral("connector"), QStringLiteral("prise"),
        QStringLiteral("cablage"), QStringLiteral("wiring"), QStringLiteral("socket"),
        QStringLiteral("pin "), QStringLiteral(" pins"), QStringLiteral("broche")
    });

    if (!diagramIntent)
        return QString();

    const bool rosco = text.contains(QStringLiteral("rosco"));
    const bool obd = text.contains(QStringLiteral("obd"))
        || text.contains(QStringLiteral("j1962"))
        || text.contains(QStringLiteral("16 broche"))
        || text.contains(QStringLiteral("16 pin"));
    const bool diagnostic = text.contains(QStringLiteral("diagnostic"));

    if (rosco)
        return QStringLiteral("ROSCO 3 broches");

    if (obd) {
        if (hasGeneration(text, QStringLiteral("1.9")))
            return QStringLiteral("MEMS 1.9 OBD 16 broches");
        return QString();
    }

    // A generic diagnostic socket request is intentionally not mapped to an
    // ECU connector. Without an explicit ROSCO/OBD indication it is ambiguous.
    if (diagnostic)
        return QString();

    for (const QString &generation : {QStringLiteral("1.2"), QStringLiteral("1.3"),
                                      QStringLiteral("1.6"), QStringLiteral("1.9")}) {
        if (hasGeneration(text, generation))
            return QStringLiteral("MEMS %1 ECU").arg(generation);
    }

    return QString();
}

QString effectiveReferenceRoot(const QString &referenceRoot)
{
    if (!referenceRoot.trimmed().isEmpty())
        return QDir::cleanPath(referenceRoot);
    return QDir(QCoreApplication::applicationDirPath())
        .filePath(QStringLiteral("database/reference"));
}

IaMemsDiagramSuggestion declaredDiagram(const QString &key, const QString &referenceRoot)
{
    IaMemsDiagramSuggestion suggestion;
    if (key.isEmpty())
        return suggestion;

    const QString root = effectiveReferenceRoot(referenceRoot);
    const QFileInfo rootInfo(root);
    if (!rootInfo.exists() || !rootInfo.isDir())
        return suggestion;

    QFile manifest(QDir(root).filePath(QStringLiteral("manifest.json")));
    if (!manifest.open(QIODevice::ReadOnly))
        return suggestion;

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(manifest.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject())
        return suggestion;

    const QJsonObject diagrams = document.object().value(QStringLiteral("diagrams")).toObject();
    const QString relativePath = QDir::cleanPath(diagrams.value(key).toString().trimmed());
    if (relativePath.isEmpty() || relativePath == QStringLiteral(".")
        || QDir::isAbsolutePath(relativePath)
        || relativePath == QStringLiteral("..")
        || relativePath.startsWith(QStringLiteral("../"))
        || !relativePath.startsWith(QStringLiteral("images/")))
        return suggestion;

    const QFileInfo diagramInfo(QDir(root).filePath(relativePath));
    if (!diagramInfo.exists() || !diagramInfo.isFile())
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

    suggestion.key = key;
    suggestion.relativePath = relativePath;
    suggestion.absolutePath = canonicalDiagram;
    return suggestion;
}

} // namespace

IaMemsDiagramSuggestion IaMemsDiagramCatalog::suggestionForQuestion(
    const QString &question,
    const QString &referenceRoot)
{
    return declaredDiagram(requestedDiagramKey(question), referenceRoot);
}
