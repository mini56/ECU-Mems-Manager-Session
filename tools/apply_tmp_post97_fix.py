from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def write(path: str, text: str) -> None:
    (ROOT / path).write_text(text, encoding="utf-8")


def replace_once(path: str, old: str, new: str) -> None:
    text = read(path)
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f"{path}: expected one match, got {count} for {old[:80]!r}")
    write(path, text.replace(old, new, 1))


def replace_between(path: str, start: str, end: str, replacement: str) -> None:
    text = read(path)
    a = text.find(start)
    if a < 0:
        raise RuntimeError(f"{path}: start marker not found: {start!r}")
    b = text.find(end, a)
    if b < 0:
        raise RuntimeError(f"{path}: end marker not found: {end!r}")
    if text.find(start, a + len(start)) >= 0 and text.find(start, a + len(start)) < b:
        raise RuntimeError(f"{path}: ambiguous start marker")
    write(path, text[:a] + replacement + text[b:])


renderer = r'''#ifndef MEMSREFERENCESHEETRENDERER_H
#define MEMSREFERENCESHEETRENDERER_H

#include <QFile>
#include <QFileInfo>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QXmlStreamReader>

namespace MemsReferenceSheetRenderer {

inline QString htmlStyle()
{
    return QStringLiteral(
        "<style>body{background:#0a1015;color:#dce3e8;font-family:'Segoe UI',Arial,sans-serif;font-size:9pt;}"
        "h1{color:#ff9828;font-size:16pt;margin:0 0 5px 0;}"
        "h2{color:#ff9828;font-size:10.5pt;border-bottom:1px solid #34414b;padding-bottom:4px;margin-top:14px;}"
        "p{margin:4px 0 7px 0;line-height:1.35}.muted{color:#94a1ab}.note{background:#15100b;border:1px solid #60401f;color:#ffd0a0;padding:7px;}"
        "table{border-collapse:collapse;width:100%;margin:5px 0 8px 0}th{background:#151e25;color:#ff9828;border-bottom:2px solid #ff7a00;text-align:left;padding:5px}"
        "td{border-bottom:1px solid #26323b;padding:5px;vertical-align:top}.wire{font-size:14pt;line-height:10px;margin-right:2px}</style>");
}

struct ElementContent
{
    QString text;
    QStringList fills;
};

inline bool usefulFill(const QString &fill)
{
    const QString value = fill.trimmed().toLower();
    return !value.isEmpty() && value != QStringLiteral("none") && value != QStringLiteral("transparent");
}

inline ElementContent readElementContent(QXmlStreamReader &xml)
{
    ElementContent result;
    int depth = 1;
    while (!xml.atEnd() && depth > 0) {
        xml.readNext();
        if (xml.isStartElement()) {
            ++depth;
            const QString name = xml.name().toString().toLower();
            if (name == QStringLiteral("br"))
                result.text += QLatin1Char('\n');
            const QString fill = xml.attributes().value(QStringLiteral("fill")).toString();
            if (usefulFill(fill) && !result.fills.contains(fill, Qt::CaseInsensitive) && result.fills.size() < 2)
                result.fills.append(fill);
        } else if (xml.isCharacters() && !xml.isWhitespace()) {
            const QString value = xml.text().toString().trimmed();
            if (!value.isEmpty()) {
                if (!result.text.isEmpty() && !result.text.endsWith(QLatin1Char('\n')))
                    result.text += QLatin1Char(' ');
                result.text += value;
            }
        } else if (xml.isEndElement()) {
            --depth;
        }
    }
    return result;
}

inline QString renderElementContent(const ElementContent &content)
{
    QString html;
    for (const QString &fill : content.fills) {
        html += QStringLiteral("<span class='wire' style='color:%1'>&#9632;</span>")
                    .arg(fill.toHtmlEscaped());
    }
    QString text = content.text.simplified().toHtmlEscaped();
    if (!text.isEmpty()) {
        if (!html.isEmpty())
            html += QStringLiteral("&nbsp;");
        html += text;
    }
    return html.isEmpty() ? QStringLiteral("—") : html;
}

inline QString renderFile(const QString &path, const QString &errorText)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return htmlStyle() + QStringLiteral("<p class='note'>%1</p>").arg(errorText.toHtmlEscaped());

    QXmlStreamReader xml(&file);
    QString html = htmlStyle();
    bool firstRow = true;
    bool inTable = false;

    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isEndElement()) {
            const QString name = xml.name().toString().toLower();
            if (name == QStringLiteral("ligne")) {
                html += QStringLiteral("</tr>");
                firstRow = false;
            } else if (name == QStringLiteral("table")) {
                html += QStringLiteral("</table>");
                inTable = false;
            }
            continue;
        }
        if (!xml.isStartElement())
            continue;

        const QString name = xml.name().toString().toLower();
        if (name == QStringLiteral("titre")) {
            html += QStringLiteral("<h1>%1</h1>")
                        .arg(readElementContent(xml).text.simplified().toHtmlEscaped());
        } else if (name == QStringLiteral("sous-titre")) {
            html += QStringLiteral("<p class='muted'>%1</p>")
                        .arg(readElementContent(xml).text.simplified().toHtmlEscaped());
        } else if (name == QStringLiteral("section")) {
            html += QStringLiteral("<h2>%1</h2>")
                        .arg(xml.attributes().value(QStringLiteral("titre")).toString().toHtmlEscaped());
        } else if (name == QStringLiteral("p")) {
            ElementContent content = readElementContent(xml);
            QString text = content.text.toHtmlEscaped();
            text.replace(QLatin1Char('\n'), QStringLiteral("<br>"));
            html += QStringLiteral("<p>%1</p>").arg(text);
        } else if (name == QStringLiteral("note")) {
            ElementContent content = readElementContent(xml);
            QString text = content.text.toHtmlEscaped();
            text.replace(QLatin1Char('\n'), QStringLiteral("<br>"));
            html += QStringLiteral("<div class='note'>%1</div>").arg(text);
        } else if (name == QStringLiteral("table")) {
            firstRow = true;
            inTable = true;
            html += QStringLiteral("<table>");
        } else if (name == QStringLiteral("ligne")) {
            if (!inTable) {
                firstRow = true;
                inTable = true;
                html += QStringLiteral("<table>");
            }
            html += QStringLiteral("<tr>");
        } else if (name == QStringLiteral("cellule")
                   || name == QStringLiteral("broche")
                   || name == QStringLiteral("fonction")
                   || name == QStringLiteral("couleur")) {
            const ElementContent content = readElementContent(xml);
            const QString tag = firstRow ? QStringLiteral("th") : QStringLiteral("td");
            html += QStringLiteral("<%1>%2</%1>").arg(tag, renderElementContent(content));
        }
    }

    if (inTable)
        html += QStringLiteral("</table>");
    if (xml.hasError())
        html += QStringLiteral("<div class='note'>%1</div>").arg(errorText.toHtmlEscaped());
    return html;
}

} // namespace MemsReferenceSheetRenderer

#endif // MEMSREFERENCESHEETRENDERER_H
'''
write("database/MemsReferenceSheetRenderer.h", renderer)

# IA tab: share the reference-sheet renderer and stop using a second incomplete XML parser.
replace_once(
    "iamemstab.cpp",
    '#include "database/MemsReferenceDatabase.h"\n',
    '#include "database/MemsReferenceDatabase.h"\n#include "database/MemsReferenceSheetRenderer.h"\n')
replace_between(
    "iamemstab.cpp",
    "QString iaReferenceXmlHtml(const QString &path)\n{",
    "QString printableFirmware(QByteArray response)",
    '''QString iaReferenceXmlHtml(const QString &path)\n{\n    return MemsReferenceSheetRenderer::renderFile(\n        path, QStringLiteral("Impossible d'ouvrir ou de lire la fiche XML locale."));\n}\n\n''')

# Strong evidence only for `cherche`: never treat generic "Mini" as proof of SPi/MPi.
replace_once(
    "expert/IaMemsConversationRouting.h",
    "#include <QStringList>\n",
    "#include <QStringList>\n#include <QRegularExpression>\n")
insert_marker = "inline bool isUnknownDirective(const QString &answer)\n{"
text = read("expert/IaMemsConversationRouting.h")
pos = text.find(insert_marker)
if pos < 0:
    raise RuntimeError("routing insertion marker missing")
helper = r'''inline QStringList inductionEvidenceProbes(const QString &question,
                                           bool ecuConnected,
                                           const QString &firmware)
{
    QStringList probes;
    const auto appendUnique = [&probes](const QString &value) {
        const QString trimmed = value.trimmed().toUpper();
        if (!trimmed.isEmpty() && !probes.contains(trimmed, Qt::CaseInsensitive))
            probes.append(trimmed);
    };

    if (ecuConnected)
        appendUnique(firmware);

    static const QRegularExpression ecuReferenceRx(
        QStringLiteral("\\b(?:MNE|MKC|NNN)[A-Z0-9-]{3,}\\b|\\bAANMP[0-9]{3,}\\b"),
        QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatchIterator iterator = ecuReferenceRx.globalMatch(question);
    while (iterator.hasNext())
        appendUnique(iterator.next().captured(0));

    return probes;
}

'''
write("expert/IaMemsConversationRouting.h", text[:pos] + helper + text[pos:])

replace_once(
    "iamemstab.h",
    "    QString resolveInductionFromKnownContext(const QString &question) const;\n",
    "    QString resolveInductionFromKnownContext(const QString &question, QString *evidence = nullptr) const;\n")
replace_between(
    "iamemstab.cpp",
    "QString IaMemsTab::resolveInductionFromKnownContext(const QString &question) const\n{",
    "QString IaMemsTab::clarificationPrompt(const QString &question) const",
    r'''QString IaMemsTab::resolveInductionFromKnownContext(const QString &question, QString *evidence) const
{
    if (evidence)
        evidence->clear();

    MemsReferenceDatabase database;
    if (!database.open())
        return QString();

    const QStringList probes = IaMemsConversationRouting::inductionEvidenceProbes(
        question, m_connected, m_firmwareIdentifier);
    if (probes.isEmpty())
        return QString();

    int requestedYear = 0;
    const QRegularExpression yearRx(QStringLiteral("\\b(19[89][0-9]|20[0-2][0-9])\\b"));
    const QRegularExpressionMatch yearMatch = yearRx.match(question);
    if (yearMatch.hasMatch())
        requestedYear = yearMatch.captured(1).toInt();

    for (const QString &probe : probes) {
        const QVariantList rows = database.searchEcus(probe, m_detectedFamily, 250);
        QSet<QString> inductions;
        for (const QVariant &item : rows) {
            const QVariantMap row = item.toMap();
            if (requestedYear > 0) {
                const int from = row.value(QStringLiteral("year_from")).toInt();
                const int to = row.value(QStringLiteral("year_to")).toInt();
                if ((from > 0 && requestedYear < from) || (to > 0 && requestedYear > to))
                    continue;
            }
            const QString injection = injectionLabel(row.value(QStringLiteral("injection")).toString());
            if (!injection.isEmpty())
                inductions.insert(injection);
        }
        if (inductions.size() == 1) {
            if (evidence)
                *evidence = probe;
            return *inductions.constBegin();
        }
    }
    return QString();
}

''')
replace_once(
    "iamemstab.cpp",
    '''                const QString resolved = resolveInductionFromKnownContext(pending);\n                if (resolved.isEmpty()) {\n                    answerLocally(QStringLiteral("J'ai cherché dans le contexte ECU connu et dans la base, mais je ne peux pas trancher SPi/MPi sans risque. Donne-moi l'année du véhicule ou la référence inscrite sur le calculateur."));\n                    return;\n                }\n                effectiveQuestion = QStringLiteral("%1 %2").arg(pending, resolved);\n                answerLocally(QStringLiteral("J'ai identifié %1 à partir des informations disponibles. Je poursuis la recherche initiale.").arg(resolved));\n''',
    '''                QString evidence;\n                const QString resolved = resolveInductionFromKnownContext(pending, &evidence);\n                if (resolved.isEmpty()) {\n                    answerLocally(QStringLiteral("J'ai cherché dans les références ECU réellement disponibles, mais je ne peux pas trancher SPi/MPi sans preuve suffisante. Donne-moi la référence inscrite sur le calculateur, ou l'année et le marché du véhicule."));\n                    return;\n                }\n                effectiveQuestion = QStringLiteral("%1 %2").arg(pending, resolved);\n                answerLocally(QStringLiteral("J'ai identifié %1 à partir de la référence ECU/firmware %2 retrouvée dans la base. Je poursuis la recherche initiale.").arg(resolved, evidence));\n''')

# Scope filtering must also reject explicit incompatible legacy facts, not only foundation facts carrying "Portee" notes.
replace_between(
    "expert/IaMemsService.cpp",
    "bool factMatchesScopeRequest(const ExpertFact &fact, const KnowledgeScopeRequest &request)\n{",
    "bool scopeQualifierTerm(const QString &term)",
    r'''bool factMatchesScopeRequest(const ExpertFact &fact, const KnowledgeScopeRequest &request)
{
    const QString structuredScope = normalized(fact.notes);
    const QString explicitScope = normalized(QStringLiteral("%1 %2 %3 %4 %5")
        .arg(fact.factKey, fact.topic, fact.statement, fact.notes, fact.family));

    // Legacy facts do not always carry the new structured "Portee" note.  An
    // explicit incompatible label in their own statement/topic is still a
    // proved incompatibility and must be rejected before ranking/Qwen.
    const bool explicitSpi = containsWord(explicitScope, QStringLiteral("spi"));
    const bool explicitMpi = containsWord(explicitScope, QStringLiteral("mpi"));
    if (request.induction == QStringLiteral("spi") && explicitMpi && !explicitSpi)
        return false;
    if (request.induction == QStringLiteral("mpi") && explicitSpi && !explicitMpi)
        return false;

    const bool explicitJapan = explicitScope.contains(QStringLiteral("japan"))
        || explicitScope.contains(QStringLiteral("japon"));
    const bool explicitEurope = explicitScope.contains(QStringLiteral("europe"));
    const bool explicitUk = containsWord(explicitScope, QStringLiteral("uk"))
        || explicitScope.contains(QStringLiteral("royaume uni"));
    if (request.market == QStringLiteral("japan")
        && (explicitEurope || explicitUk) && !explicitJapan)
        return false;
    if ((request.market == QStringLiteral("europe") || request.market == QStringLiteral("uk"))
        && explicitJapan && !explicitEurope && !explicitUk)
        return false;

    const bool explicitAutomatic = explicitScope.contains(QStringLiteral("automatic"))
        || explicitScope.contains(QStringLiteral("automatique"));
    const bool explicitManual = containsWord(explicitScope, QStringLiteral("manual"))
        || explicitScope.contains(QStringLiteral("manuelle"));
    if (request.transmission == QStringLiteral("automatic") && explicitManual && !explicitAutomatic)
        return false;
    if (request.transmission == QStringLiteral("manual") && explicitAutomatic && !explicitManual)
        return false;

    if (!structuredScope.contains(QStringLiteral("portee")))
        return true;

    // Keep the stricter structured-scope checks for foundation facts.
    const bool scopeSpi = containsWord(structuredScope, QStringLiteral("spi"));
    const bool scopeMpi = containsWord(structuredScope, QStringLiteral("mpi"));
    if (request.induction == QStringLiteral("spi") && scopeMpi && !scopeSpi)
        return false;
    if (request.induction == QStringLiteral("mpi") && scopeSpi && !scopeMpi)
        return false;

    const bool scopeJapan = structuredScope.contains(QStringLiteral("japan"))
        || structuredScope.contains(QStringLiteral("japon"));
    const bool scopeEurope = structuredScope.contains(QStringLiteral("europe"));
    const bool scopeUk = containsWord(structuredScope, QStringLiteral("uk"))
        || structuredScope.contains(QStringLiteral("royaume uni"));
    if (request.market == QStringLiteral("japan") && (scopeEurope || scopeUk) && !scopeJapan)
        return false;
    if ((request.market == QStringLiteral("europe") || request.market == QStringLiteral("uk"))
        && scopeJapan && !scopeEurope && !scopeUk)
        return false;

    const bool scopeAutomatic = structuredScope.contains(QStringLiteral("automatic"))
        || structuredScope.contains(QStringLiteral("automatique"));
    const bool scopeManual = containsWord(structuredScope, QStringLiteral("manual"))
        || structuredScope.contains(QStringLiteral("manuelle"));
    if (request.transmission == QStringLiteral("automatic") && scopeManual && !scopeAutomatic)
        return false;
    if (request.transmission == QStringLiteral("manual") && scopeAutomatic && !scopeManual)
        return false;

    return true;
}

''')

# Documentary facts containing "preuve" are factual grounding, not a diagnostic request.
replace_once(
    "expert/LocalAiClient.cpp",
    '''    return ground.contains(QStringLiteral("hypotheses actuelles"))\n        || ground.contains(QStringLiteral("confiance"))\n        || ground.contains(QStringLiteral("preuve"));\n''',
    '''    return ground.contains(QStringLiteral("hypotheses actuelles"))\n        || ground.contains(QStringLiteral("confiance"));\n''')
replace_once(
    "expert/LocalAiClient.cpp",
    '''        QStringLiteral("reponds uniquement en francais sauf si l utilisateur")\n''',
    '''        QStringLiteral("reponds uniquement en francais sauf si l utilisateur"),\n        QStringLiteral("reponse attendue"),\n        QStringLiteral("diagnostic bref hypotheses les plus probables"),\n        QStringLiteral("ne montre aucun raisonnement interne"),\n        QStringLiteral("faits fournis par mems manager"),\n        QStringLiteral("a utiliser seulement s ils repondent a la question")\n''')

# Database tab and IA tab must use the same complete XML renderer.
replace_once(
    "database/MemsDatabaseBrowser.cpp",
    '#include "MemsReferenceDatabase.h"\n',
    '#include "MemsReferenceDatabase.h"\n#include "MemsReferenceSheetRenderer.h"\n')
replace_between(
    "database/MemsDatabaseBrowser.cpp",
    "QString MemsDatabaseBrowser::renderGenerationXml(const QString &path) const\n{",
    "void MemsDatabaseBrowser::showGenerationSheet()",
    '''QString MemsDatabaseBrowser::renderGenerationXml(const QString &path) const\n{\n    return MemsReferenceSheetRenderer::renderFile(path, I18n::text(7232));\n}\n\n''')

# Deterministic self-tests: prove generic Mini is no longer accepted as evidence and prove both XML dialects keep pins/functions/colors.
replace_once(
    "expert/IaResponseLogicTest.cpp",
    '#include "IaResponseLogic.h"\n#include "i18n.h"\n',
    '#include "IaResponseLogic.h"\n#include "../database/MemsReferenceSheetRenderer.h"\n#include "i18n.h"\n')
replace_once(
    "expert/IaResponseLogicTest.cpp",
    "#include <QDebug>\n",
    "#include <QDebug>\n#include <QTemporaryFile>\n")
needle = '''    ok &= require(IaMemsConversationRouting::isSearchDirective(QStringLiteral("cherche")),\n                  "search directive is recognised");\n    ok &= require(IaMemsConversationRouting::requestedGeneration(QStringLiteral("Je cherche la documentation MEMS 1.9")) == QStringLiteral("1.9"),\n                  "documentation generation extraction");\n'''
replacement = needle + r'''
    ok &= require(IaMemsConversationRouting::inductionEvidenceProbes(
                      QStringLiteral("Broche MAP Mini"), false, QString()).isEmpty(),
                  "generic Mini while disconnected is not SPi/MPi evidence");
    ok &= require(IaMemsConversationRouting::inductionEvidenceProbes(
                      QStringLiteral("Broche MAP Mini"), true, QStringLiteral("AANMP002"))
                      == QStringList{QStringLiteral("AANMP002")},
                  "connected firmware is strong SPi/MPi evidence");
    ok &= require(IaMemsConversationRouting::inductionEvidenceProbes(
                      QStringLiteral("Broche MAP Mini MKC104341"), false, QString()).contains(QStringLiteral("MKC104341")),
                  "explicit ECU reference in the question is strong evidence");

    QTemporaryFile customSheet;
    ok &= require(customSheet.open(), "open temporary custom XML sheet");
    if (customSheet.isOpen()) {
        const QByteArray xml = QByteArrayLiteral(
            "<fiche><table><ligne><broche>BROCHE</broche><fonction>FONCTION</fonction><couleur>COULEUR</couleur></ligne>"
            "<ligne><broche>8</broche><fonction>MAP</fonction><couleur><svg><rect fill='#f2a5bc'/><path fill='#050505'/></svg><texte>Rose / Noir</texte></couleur></ligne></table></fiche>");
        customSheet.write(xml);
        customSheet.flush();
        const QString rendered = MemsReferenceSheetRenderer::renderFile(customSheet.fileName(), QStringLiteral("erreur"));
        ok &= require(rendered.contains(QStringLiteral("MAP")) && rendered.contains(QStringLiteral("Rose / Noir"))
                      && rendered.contains(QStringLiteral("#f2a5bc")) && rendered.contains(QStringLiteral("#050505")),
                      "custom broche/fonction/couleur XML keeps data and visual colours");
    }

    QTemporaryFile genericSheet;
    ok &= require(genericSheet.open(), "open temporary generic XML sheet");
    if (genericSheet.isOpen()) {
        const QByteArray xml = QByteArrayLiteral(
            "<fiche><table><ligne><cellule>BROCHE</cellule><cellule>COULEUR</cellule></ligne>"
            "<ligne><cellule>7</cellule><cellule><svg><rect fill='#1769e8'/><path fill='#ed2224'/></svg><texte>Bleu / Rouge</texte></cellule></ligne></table></fiche>");
        genericSheet.write(xml);
        genericSheet.flush();
        const QString rendered = MemsReferenceSheetRenderer::renderFile(genericSheet.fileName(), QStringLiteral("erreur"));
        ok &= require(rendered.contains(QStringLiteral("Bleu / Rouge"))
                      && rendered.contains(QStringLiteral("#1769e8")) && rendered.contains(QStringLiteral("#ed2224")),
                      "generic cellule XML keeps embedded visual colours");
    }
'''
replace_once("expert/IaResponseLogicTest.cpp", needle, replacement)

# Native ONNX self-test: final user-facing documentary grounding must bypass diagnostic prompt even when it says "preuve".
replace_once(
    "expert/LocalAiOnnxSelfTest.cpp",
    '''        QStringLiteral("you are ia mems, the local assistant")\n''',
    '''        QStringLiteral("you are ia mems, the local assistant"),\n        QStringLiteral("réponse attendue"),\n        QStringLiteral("reponse attendue"),\n        QStringLiteral("diagnostic bref"),\n        QStringLiteral("ne montre aucun raisonnement interne"),\n        QStringLiteral("faits fournis par mems manager")\n''')
replace_once(
    "expert/LocalAiOnnxSelfTest.cpp",
    '''            stage = 4;\n            client.ask(QStringLiteral("Réponds uniquement par OK."), QString());\n            return;\n        }\n\n        if (stage == 4) {\n            if (!answer.contains(QStringLiteral("OK"), Qt::CaseInsensitive)) {\n                fail(QStringLiteral("La génération native ne contient pas le marqueur OK attendu."));\n                return;\n            }\n            finished = true;\n            out << "PASS LocalAiClient native ONNX response quality and real targeting cases" << Qt::endl;\n            app.exit(0);\n        }\n''',
    '''            stage = 4;\n            client.ask(QStringLiteral("Couple de serrage sonde température ECT"),\n                       QStringLiteral("Couple ECT vérifié : 15 Nm.\\nNiveau de preuve : constructeur."));\n            return;\n        }\n\n        if (stage == 4) {\n            if (!answer.contains(QStringLiteral("15 Nm"), Qt::CaseInsensitive)) {\n                fail(QStringLiteral("Le fait documentaire ECT a été remplacé au lieu d'être rendu directement."));\n                return;\n            }\n            stage = 5;\n            client.ask(QStringLiteral("Couleur des fils sonde lambda"),\n                       QStringLiteral("Fils sonde lambda : gris et vert clair/gris — preuve : constructeur."));\n            return;\n        }\n\n        if (stage == 5) {\n            if (!answer.contains(QStringLiteral("gris"), Qt::CaseInsensitive)\n                || answer.contains(QStringLiteral("réponse attendue"), Qt::CaseInsensitive)\n                || answer.contains(QStringLiteral("diagnostic bref"), Qt::CaseInsensitive)) {\n                fail(QStringLiteral("La réponse documentaire lambda fuit encore une consigne interne."));\n                return;\n            }\n            stage = 6;\n            client.ask(QStringLiteral("Réponds uniquement par OK."), QString());\n            return;\n        }\n\n        if (stage == 6) {\n            if (!answer.contains(QStringLiteral("OK"), Qt::CaseInsensitive)) {\n                fail(QStringLiteral("La génération native ne contient pas le marqueur OK attendu."));\n                return;\n            }\n            finished = true;\n            out << "PASS LocalAiClient native ONNX response quality, documentary grounding and leak guards" << Qt::endl;\n            app.exit(0);\n        }\n''')

print("POST97_PATCH_APPLIED")
