#include "IaMemsDiagramCatalog.h"
#include "IaMemsLibraryBridge.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QTextStream>
#include <QVector>

namespace {

struct SourceRef
{
    QString document;
    int page = -1;
};

struct TestCase
{
    QString name;
    QString question;
    QStringList keywords;
};

QString normalizedDocument(QString value)
{
    value = value.trimmed();
    if (value.startsWith(QStringLiteral("DOC_")))
        value.remove(0, 4);
    return value.toUpper();
}

QVector<SourceRef> groundingSources(const QString &text)
{
    QVector<SourceRef> sources;
    const QRegularExpression rx(QStringLiteral("Source\\s+([^,\\s]+),\\s+page\\s+(\\d+)"),
                                QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatchIterator it = rx.globalMatch(text);
    while (it.hasNext()) {
        const QRegularExpressionMatch match = it.next();
        SourceRef ref;
        ref.document = normalizedDocument(match.captured(1));
        ref.page = match.captured(2).toInt();
        sources.append(ref);
    }
    return sources;
}

SourceRef visualSource(const IaMemsDiagramSuggestion &suggestion)
{
    SourceRef ref;
    const QRegularExpression rx(QStringLiteral("^(.+?)\\s+p\\.(\\d+)$"),
                                QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch match = rx.match(suggestion.key.trimmed());
    if (match.hasMatch()) {
        ref.document = normalizedDocument(match.captured(1));
        ref.page = match.captured(2).toInt();
    }
    return ref;
}

QString oneLine(QString value, int limit = 360)
{
    value.replace(QLatin1Char('\r'), QLatin1Char(' '));
    value.replace(QLatin1Char('\n'), QLatin1Char(' '));
    value = value.simplified();
    if (value.size() > limit)
        value = value.left(limit) + QStringLiteral("...");
    return value;
}

QString visualContext(const QString &root, const QString &relativePath)
{
    QFile file(QDir(root).filePath(QStringLiteral("runtime_visual_catalog.json")));
    if (!file.open(QIODevice::ReadOnly))
        return QStringLiteral("<catalog unavailable>");
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    if (!document.isObject())
        return QStringLiteral("<catalog invalid>");
    const QString wanted = QDir::cleanPath(relativePath);
    const QJsonArray entries = document.object().value(QStringLiteral("entries")).toArray();
    for (const QJsonValue &value : entries) {
        if (!value.isObject())
            continue;
        const QJsonObject entry = value.toObject();
        if (QDir::cleanPath(entry.value(QStringLiteral("runtime_path")).toString()) != wanted)
            continue;
        return QStringLiteral("publication=%1 page=%2 context=%3")
            .arg(entry.value(QStringLiteral("publication_code")).toString())
            .arg(entry.value(QStringLiteral("physical_page")).toInt())
            .arg(oneLine(entry.value(QStringLiteral("context_text")).toString()));
    }
    return QStringLiteral("<selected visual entry not found>");
}

QString sourceList(const QVector<SourceRef> &sources)
{
    QStringList parts;
    for (const SourceRef &source : sources)
        parts.append(QStringLiteral("%1:p%2").arg(source.document).arg(source.page));
    return parts.join(QStringLiteral(" | "));
}

bool sameSource(const SourceRef &a, const SourceRef &b)
{
    return !a.document.isEmpty() && !b.document.isEmpty()
        && a.document == b.document && a.page == b.page;
}

bool sameDocument(const SourceRef &a, const SourceRef &b)
{
    return !a.document.isEmpty() && !b.document.isEmpty() && a.document == b.document;
}

int runCase(const TestCase &test, const QString &referenceRoot, QTextStream &out)
{
    out << "COHERENCE_CASE_BEGIN name=" << test.name << Qt::endl;
    out << "QUESTION " << test.question << Qt::endl;
    out << "RESPONSE_KEYWORDS " << test.keywords.join(QStringLiteral(" | ")) << Qt::endl;

    const IaMemsLibraryGrounding grounding = IaMemsLibraryBridge::retrieve(test.question, test.keywords);
    const QVector<SourceRef> textSources = groundingSources(grounding.text);
    out << "RESPONSE_READY " << (grounding.libraryReady ? 1 : 0)
        << " count=" << grounding.resultCount << Qt::endl;
    out << "RESPONSE_SOURCES " << sourceList(textSources) << Qt::endl;
    out << "RESPONSE_GROUNDING " << oneLine(grounding.text, 800) << Qt::endl;

    const IaMemsDiagramSuggestion visual =
        IaMemsDiagramCatalog::suggestionForQuestion(test.question, referenceRoot);
    if (!visual.isValid()) {
        out << "VISUAL none" << Qt::endl;
        const QString verdict = textSources.isEmpty()
            ? QStringLiteral("NO_TEXT_NO_VISUAL")
            : QStringLiteral("TEXT_WITHOUT_VISUAL");
        out << "COHERENCE_VERDICT " << verdict << Qt::endl;
        out << "COHERENCE_CASE_END name=" << test.name << Qt::endl;
        return 0;
    }

    const SourceRef imageSource = visualSource(visual);
    out << "VISUAL key=" << visual.key << " path=" << visual.relativePath << Qt::endl;
    out << "VISUAL_CONTEXT " << visualContext(referenceRoot, visual.relativePath) << Qt::endl;

    bool exactAny = false;
    bool documentAny = false;
    for (const SourceRef &source : textSources) {
        exactAny = exactAny || sameSource(source, imageSource);
        documentAny = documentAny || sameDocument(source, imageSource);
    }
    const bool exactFirst = !textSources.isEmpty() && sameSource(textSources.first(), imageSource);

    QString verdict;
    int issue = 0;
    if (textSources.isEmpty()) {
        verdict = QStringLiteral("VISUAL_WITHOUT_TEXT_SOURCE");
        issue = 1;
    } else if (exactFirst) {
        verdict = QStringLiteral("COHERENT_EXACT_FIRST");
    } else if (exactAny) {
        verdict = QStringLiteral("COHERENT_ONLY_WITH_LATER_TEXT_RESULT");
        issue = 1;
    } else if (documentAny) {
        verdict = QStringLiteral("DIVERGENT_PAGE_SAME_DOCUMENT");
        issue = 1;
    } else {
        verdict = QStringLiteral("DIVERGENT_DOCUMENT");
        issue = 1;
    }

    out << "COHERENCE_COMPARE first_match=" << (exactFirst ? 1 : 0)
        << " any_exact_match=" << (exactAny ? 1 : 0)
        << " same_document=" << (documentAny ? 1 : 0) << Qt::endl;
    out << "COHERENCE_VERDICT " << verdict << Qt::endl;
    out << "COHERENCE_CASE_END name=" << test.name << Qt::endl;
    return issue;
}

} // namespace

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QTextStream out(stdout);
    QTextStream err(stderr);
    out.setCodec("UTF-8");
    err.setCodec("UTF-8");

    if (argc != 4) {
        err << "usage: ia_response_visual_coherence_selftest <MEMSLibrary.dll> <Pack001-dir> <database/reference>" << Qt::endl;
        return 2;
    }

    const QString dll = QDir::cleanPath(QString::fromLocal8Bit(argv[1]));
    const QString pack = QDir::cleanPath(QString::fromLocal8Bit(argv[2]));
    const QString referenceRoot = QDir::cleanPath(QString::fromLocal8Bit(argv[3]));
    if (!QFileInfo::exists(dll) || !QFileInfo(pack).isDir() || !QFileInfo(referenceRoot).isDir()) {
        err << "FAIL required runtime input missing" << Qt::endl;
        return 3;
    }

    qputenv("MEMS_LIBRARY_DLL", dll.toLocal8Bit());
    qputenv("MEMS_LIBRARY_PACK", pack.toLocal8Bit());

    const QVector<TestCase> cases = {
        {QStringLiteral("french_axial_primary"),
         QStringLiteral("Quel est le jeu axial du pignon primaire ?"),
         {QStringLiteral("jeu"), QStringLiteral("axial"), QStringLiteral("pignon"), QStringLiteral("primaire"),
          QStringLiteral("primary"), QStringLiteral("end"), QStringLiteral("float")}},
        {QStringLiteral("french_battery_restoration"),
         QStringLiteral("Quelle est la procédure de restauration de la batterie ?"),
         {QStringLiteral("restauration"), QStringLiteral("batterie"),
          QStringLiteral("battery"), QStringLiteral("restoration")}},
        {QStringLiteral("french_primary_removal"),
         QStringLiteral("Comment démonter le pignon primaire sur le moteur ?"),
         {QStringLiteral("comment"), QStringLiteral("demonter"), QStringLiteral("pignon"),
          QStringLiteral("primaire"), QStringLiteral("moteur"), QStringLiteral("primary")}},
        {QStringLiteral("french_crankshaft_check"),
         QStringLiteral("Comment contrôler le vilebrequin ?"),
         {QStringLiteral("comment"), QStringLiteral("controler"), QStringLiteral("vilebrequin")}}
    };

    int issues = 0;
    for (const TestCase &test : cases)
        issues += runCase(test, referenceRoot, out);

    out << "COHERENCE_SUMMARY cases=" << cases.size() << " issues=" << issues << Qt::endl;
    if (issues > 0) {
        out << "IA_RESPONSE_VISUAL_COHERENCE_FAIL issues=" << issues << Qt::endl;
        return 10;
    }

    out << "IA_RESPONSE_VISUAL_COHERENCE_PASS" << Qt::endl;
    return 0;
}
