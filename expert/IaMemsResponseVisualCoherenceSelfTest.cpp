#include "IaMemsConversationRouting.h"
#include "IaMemsDiagramCatalog.h"
#include "IaMemsLibraryBridge.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSet>
#include <QTextStream>

namespace {

void appendUnique(QStringList &terms, const QString &term)
{
    const QString clean = term.trimmed().toLower();
    if (!clean.isEmpty() && !terms.contains(clean) && terms.size() < 12)
        terms.append(clean);
}

QStringList libraryKeywords(const QString &question)
{
    QString text = IaMemsConversationRouting::normalize(question);
    text.replace(QRegularExpression(QStringLiteral("[^a-z0-9_.-]+")), QStringLiteral(" "));
    const QSet<QString> stop = {
        QStringLiteral("que"), QStringLiteral("quoi"), QStringLiteral("quel"), QStringLiteral("quelle"),
        QStringLiteral("les"), QStringLiteral("des"), QStringLiteral("une"), QStringLiteral("dans"),
        QStringLiteral("pour"), QStringLiteral("avec"), QStringLiteral("est"), QStringLiteral("sais"),
        QStringLiteral("sait"), QStringLiteral("peux"), QStringLiteral("dire"), QStringLiteral("cette"),
        QStringLiteral("sur"), QStringLiteral("mems"), QStringLiteral("source"), QStringLiteral("sources"),
        QStringLiteral("document"), QStringLiteral("documentation"), QStringLiteral("procedure")
    };

    QStringList terms;
    for (const QString &word : text.split(QLatin1Char(' '), Qt::SkipEmptyParts)) {
        if (word.size() >= 3 && !stop.contains(word))
            appendUnique(terms, word);
        if (terms.size() >= 7)
            break;
    }

    const QString normalized = text.simplified();
    if (normalized.contains(QStringLiteral("batterie"))) appendUnique(terms, QStringLiteral("battery"));
    if (normalized.contains(QStringLiteral("restauration"))) appendUnique(terms, QStringLiteral("restoration"));
    if (normalized.contains(QStringLiteral("engrenage"))) appendUnique(terms, QStringLiteral("gear"));
    if (normalized.contains(QStringLiteral("primaire"))) appendUnique(terms, QStringLiteral("primary"));
    if (normalized.contains(QStringLiteral("jeu axial"))) {
        appendUnique(terms, QStringLiteral("end"));
        appendUnique(terms, QStringLiteral("float"));
    }
    if (normalized.contains(QStringLiteral("bobine"))) {
        appendUnique(terms, QStringLiteral("coil"));
        appendUnique(terms, QStringLiteral("dwell"));
    }
    if (normalized.contains(QStringLiteral("papillon"))) appendUnique(terms, QStringLiteral("throttle"));
    if (normalized.contains(QStringLiteral("ralenti"))) appendUnique(terms, QStringLiteral("idle"));
    if (normalized.contains(QStringLiteral("refroidissement"))) appendUnique(terms, QStringLiteral("coolant"));
    if (normalized.contains(QStringLiteral("admission"))) appendUnique(terms, QStringLiteral("intake"));
    if (normalized.contains(QStringLiteral("broche")) || normalized.contains(QStringLiteral("brochage"))) {
        appendUnique(terms, QStringLiteral("pin"));
        appendUnique(terms, QStringLiteral("wiring"));
    }
    if (normalized.contains(QStringLiteral("couleur")) && normalized.contains(QStringLiteral("fil"))) {
        appendUnique(terms, QStringLiteral("wire"));
        appendUnique(terms, QStringLiteral("colour"));
    }
    return terms;
}

QString normalizedDoc(QString value)
{
    value = value.trimmed().toUpper();
    if (value.startsWith(QStringLiteral("DOC_")))
        value.remove(0, 4);
    return value;
}

bool parseVisualKey(const QString &key, QString *document, int *page)
{
    const QRegularExpression rx(QStringLiteral("^(.+?)\\s+p\\.(\\d+)$"), QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch match = rx.match(key.trimmed());
    if (!match.hasMatch())
        return false;
    if (document) *document = normalizedDoc(match.captured(1));
    if (page) *page = match.captured(2).toInt();
    return true;
}

QString suggestionLabel(const IaMemsDiagramSuggestion &suggestion)
{
    if (!suggestion.isValid())
        return QStringLiteral("none");
    return QStringLiteral("%1 | %2").arg(suggestion.key, suggestion.relativePath);
}

int runCase(const QString &name, const QString &fullQuestion, const QString &referenceRoot, QTextStream &out)
{
    const QString focused = IaMemsConversationRouting::focusedQuestion(fullQuestion);
    const QStringList keywords = libraryKeywords(focused);

    const IaMemsDiagramSuggestion initial =
        IaMemsDiagramCatalog::suggestionForQuestion(fullQuestion, referenceRoot);
    const IaMemsLibraryGrounding grounding =
        IaMemsLibraryBridge::retrieve(focused, keywords);
    const IaMemsDiagramSuggestion evidence =
        IaMemsDiagramCatalog::suggestionForEvidence(focused, grounding.text, referenceRoot);
    const IaMemsDiagramSuggestion finalSuggestion = evidence.isValid() ? evidence : initial;

    out << "CASE_BEGIN " << name << Qt::endl;
    out << "QUESTION " << fullQuestion << Qt::endl;
    out << "FOCUSED " << focused << Qt::endl;
    out << "KEYWORDS " << keywords.join(QStringLiteral(" | ")) << Qt::endl;
    out << "TEXT_SOURCE doc=" << grounding.selectedDocument
        << " page=" << grounding.selectedPage
        << " filtered=" << (grounding.provenanceFiltered ? 1 : 0)
        << " count=" << grounding.resultCount << Qt::endl;
    out << "INITIAL_IMAGE " << suggestionLabel(initial) << Qt::endl;
    out << "EVIDENCE_IMAGE " << suggestionLabel(evidence) << Qt::endl;
    out << "FINAL_IMAGE " << suggestionLabel(finalSuggestion) << Qt::endl;

    int issue = 0;
    QString verdict;
    if (!grounding.provenanceFiltered || grounding.selectedDocument.isEmpty() || grounding.selectedPage <= 0) {
        verdict = QStringLiteral("NO_VERIFIED_TEXT_SOURCE");
        issue = 1;
    } else if (!finalSuggestion.isValid()) {
        verdict = QStringLiteral("TEXT_OK_NO_IMAGE");
    } else {
        QString imageDoc;
        int imagePage = -1;
        if (!parseVisualKey(finalSuggestion.key, &imageDoc, &imagePage)) {
            verdict = QStringLiteral("IMAGE_SOURCE_UNPARSEABLE");
            issue = 1;
        } else {
            const QString textDoc = normalizedDoc(grounding.selectedDocument);
            if (textDoc == imageDoc && grounding.selectedPage == imagePage) {
                verdict = evidence.isValid()
                    ? QStringLiteral("COHERENT_EVIDENCE_IMAGE")
                    : QStringLiteral("COHERENT_INITIAL_IMAGE");
            } else {
                verdict = evidence.isValid()
                    ? QStringLiteral("DIVERGENT_EVIDENCE_IMAGE")
                    : QStringLiteral("DIVERGENT_STALE_INITIAL_IMAGE");
                issue = 1;
            }
            out << "COMPARE text=" << textDoc << ":p" << grounding.selectedPage
                << " image=" << imageDoc << ":p" << imagePage << Qt::endl;
        }
    }

    out << "VERDICT " << verdict << Qt::endl;
    out << "CASE_END " << name << Qt::endl;
    return issue;
}

} // namespace

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QTextStream out(stdout);
    QTextStream err(stderr);
    out.setCodec("UTF-8");
    err.setCodec("UTF-8");

    if (argc != 4) {
        err << "usage: ia_response_visual_coherence_selftest <MEMSLibrary.dll> <Pack001-dir> <reference-root>" << Qt::endl;
        return 2;
    }

    const QString dllPath = QFileInfo(QString::fromLocal8Bit(argv[1])).absoluteFilePath();
    const QString packDirectory = QFileInfo(QString::fromLocal8Bit(argv[2])).absoluteFilePath();
    const QString referenceRoot = QFileInfo(QString::fromLocal8Bit(argv[3])).absoluteFilePath();
    if (!QFileInfo::exists(dllPath) || !QFileInfo(packDirectory).isDir()
        || !QFileInfo(referenceRoot + QStringLiteral("/runtime_visual_catalog.json")).isFile()) {
        err << "FAIL runtime inputs missing" << Qt::endl;
        return 3;
    }

    qputenv("MEMS_LIBRARY_DLL", dllPath.toLocal8Bit());
    qputenv("MEMS_LIBRARY_PACK", packDirectory.toLocal8Bit());

    int issues = 0;
    issues += runCase(QStringLiteral("plain_primary"),
                      QStringLiteral("Quel est le jeu axial du pignon primaire et comment le contrôler ?"),
                      referenceRoot, out);
    issues += runCase(QStringLiteral("plain_battery"),
                      QStringLiteral("Quelle est la procédure de restauration de la batterie ?"),
                      referenceRoot, out);
    issues += runCase(QStringLiteral("visual_primary"),
                      QStringLiteral("Je veux voir le jeu axial du pignon primaire et comment le contrôler."),
                      referenceRoot, out);
    issues += runCase(QStringLiteral("visual_battery"),
                      QStringLiteral("Je veux voir la procédure de restauration de la batterie."),
                      referenceRoot, out);
    issues += runCase(QStringLiteral("visual_throttle"),
                      QStringLiteral("Je veux voir le potentiomètre de papillon."),
                      referenceRoot, out);

    out << "COHERENCE_SUMMARY cases=5 issues=" << issues << Qt::endl;
    if (issues > 0) {
        out << "IA_RESPONSE_VISUAL_COHERENCE_FAIL issues=" << issues << Qt::endl;
        return 10;
    }
    out << "IA_RESPONSE_VISUAL_COHERENCE_PASS" << Qt::endl;
    return 0;
}
