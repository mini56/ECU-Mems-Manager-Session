from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def replace_once(path: str, old: str, new: str) -> None:
    file_path = ROOT / path
    text = file_path.read_text(encoding="utf-8")
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"PATCH_FAIL {path}: expected one match, got {count}: {old[:100]!r}")
    file_path.write_text(text.replace(old, new, 1), encoding="utf-8")
    print(f"PATCH_OK {path}")


# 1) Documentary questions must never be intercepted as live MAP/lambda/etc values.
replace_once(
    "expert/IaResponseLogic.h",
    '#include "i18n.h"\n',
    '#include "i18n.h"\n#include "IaMemsConversationRouting.h"\n',
)
replace_once(
    "expert/IaResponseLogic.h",
    '    const QString text = normalize(question);\n\n    if (isMetricDefinitionQuestion(text))\n',
    '    const QString text = normalize(question);\n\n'
    '    // A technical-document request has priority over a live-value intent.\n'
    '    // Example: "broche MAP" is documentation, not a request for the MAP reading.\n'
    '    if (IaMemsConversationRouting::isDocumentationQuestion(question))\n'
    '        return Intent::None;\n\n'
    '    if (isMetricDefinitionQuestion(text))\n',
)

# 2) The service itself also gives documentary lookup priority, then keeps Qwen as writer.
replace_once(
    "expert/IaMemsService.cpp",
    '#include "IaResponseLogic.h"\n#include "LocalAiClient.h"\n',
    '#include "IaResponseLogic.h"\n#include "IaMemsConversationRouting.h"\n#include "LocalAiClient.h"\n',
)
replace_once(
    "expert/IaMemsService.cpp",
    '    const IaResponseLogic::Intent intent = IaResponseLogic::classify(question);\n',
    '    if (IaMemsConversationRouting::isDocumentationQuestion(question)) {\n'
    '        const QString documentaryKnowledge = knowledgeAnswer(question);\n'
    '        if (!documentaryKnowledge.isEmpty())\n'
    '            return documentaryKnowledge;\n'
    '        return QStringLiteral("Je n\\\'ai pas trouvé de donnée documentaire vérifiée correspondant exactement à cette demande. Je peux préciser la recherche si vous me donnez le véhicule, la génération MEMS ou la variante SPi/MPi lorsqu\\\'elle est pertinente.");\n'
    '    }\n\n'
    '    const IaResponseLogic::Intent intent = IaResponseLogic::classify(question);\n',
)

# 3) Exact ECT/IAT requests stay exact; do not bring the neighbouring temperature sensor.
replace_once(
    "expert/IaMemsService.cpp",
    '    const KnowledgeQueryKind queryKind = knowledgeQueryKind(questionText);\n    const QStringList terms = knowledgeTerms(question);\n',
    '    const KnowledgeQueryKind queryKind = knowledgeQueryKind(questionText);\n'
    '    const bool explicitEct = containsWord(questionText, QStringLiteral("ect"));\n'
    '    const bool explicitIat = containsWord(questionText, QStringLiteral("iat"));\n'
    '    const QStringList terms = knowledgeTerms(question);\n',
)
replace_once(
    "expert/IaMemsService.cpp",
    '        const QString searchable = identity + QLatin1Char(\' \') + body;\n\n        if (queryKind == KnowledgeQueryKind::WireColor',
    '        const QString searchable = identity + QLatin1Char(\' \') + body;\n\n'
    '        if (explicitEct && !knowledgeTermMatches(searchable, QStringLiteral("ect")))\n'
    '            continue;\n'
    '        if (explicitIat && !knowledgeTermMatches(searchable, QStringLiteral("iat")))\n'
    '            continue;\n\n'
    '        if (queryKind == KnowledgeQueryKind::WireColor',
)

# 4) Remove duplicate knowledge statements before building the grounding sent to Qwen.
replace_once(
    "expert/IaMemsService.cpp",
    '    const int maximum = qMin(queryKind == KnowledgeQueryKind::General ? 3 : 4, ranked.size());\n',
    '    QVector<RankedFact> uniqueRanked;\n'
    '    uniqueRanked.reserve(ranked.size());\n'
    '    QSet<QString> seenStatements;\n'
    '    for (const RankedFact &candidate : ranked) {\n'
    '        const QString signature = normalized(candidate.fact.statement)\n'
    '            + QLatin1Char(\'|\') + normalized(candidate.fact.family);\n'
    '        if (signature.trimmed().isEmpty() || seenStatements.contains(signature))\n'
    '            continue;\n'
    '        seenStatements.insert(signature);\n'
    '        uniqueRanked.append(candidate);\n'
    '    }\n'
    '    ranked = uniqueRanked;\n\n'
    '    const int maximum = qMin(queryKind == KnowledgeQueryKind::General ? 3 : 4, ranked.size());\n',
)

# 5) IA tab: add an XML sheet button and conversation clarification state.
replace_once(
    "iamemstab.h",
    '    void openSuggestedDiagram();\n',
    '    void openSuggestedDiagram();\n    void openSuggestedDocument();\n',
)
replace_once(
    "iamemstab.h",
    '    void updateDiagramSuggestion(const QString &question);\n',
    '    void updateDiagramSuggestion(const QString &question);\n'
    '    void updateDocumentSuggestion(const QString &question);\n'
    '    QString resolveInductionFromKnownContext(const QString &question) const;\n'
    '    QString clarificationPrompt(const QString &question) const;\n'
    '    void answerLocally(const QString &text);\n',
)
replace_once(
    "iamemstab.h",
    '    QPushButton *m_diagramButton = nullptr;\n',
    '    QPushButton *m_diagramButton = nullptr;\n    QPushButton *m_documentButton = nullptr;\n',
)
replace_once(
    "iamemstab.h",
    '    QString m_diagramQuestion;\n\n    bool m_connected = false;\n',
    '    QString m_diagramQuestion;\n'
    '    QString m_documentGeneration;\n'
    '    QString m_pendingClarificationQuestion;\n'
    '    QString m_detectedFamily;\n'
    '    QString m_firmwareIdentifier;\n'
    '    QString m_ecuIdHex;\n\n'
    '    bool m_connected = false;\n',
)

replace_once(
    "iamemstab.cpp",
    '#include "expert/IaMemsDiagramCatalog.h"\n#include "expert/IaMemsService.h"\n',
    '#include "expert/IaMemsDiagramCatalog.h"\n#include "expert/IaMemsConversationRouting.h"\n#include "expert/IaMemsService.h"\n#include "database/MemsReferenceDatabase.h"\n',
)
replace_once(
    "iamemstab.cpp",
    '#include <QDateTime>\n#include <QDialog>\n',
    '#include <QDateTime>\n#include <QDialog>\n#include <QFile>\n',
)
replace_once(
    "iamemstab.cpp",
    '#include <QScrollBar>\n#include <QShowEvent>\n',
    '#include <QScrollBar>\n#include <QSet>\n#include <QShowEvent>\n',
)
replace_once(
    "iamemstab.cpp",
    '#include <QUrl>\n#include <QVBoxLayout>\n',
    '#include <QUrl>\n#include <QVBoxLayout>\n#include <QVariantMap>\n#include <QXmlStreamReader>\n#include <QRegularExpression>\n',
)

xml_helpers = r'''
namespace {

QString iaReferenceXmlHtml(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return QStringLiteral("<p>Impossible d'ouvrir la fiche XML locale.</p>");

    QXmlStreamReader xml(&file);
    QString html = QStringLiteral(
        "<style>body{background:#0a1015;color:#dce3e8;font-family:'Segoe UI',Arial,sans-serif;font-size:9pt;}"
        "h1{color:#ff9828;font-size:16pt;margin:0 0 5px 0;}"
        "h2{color:#ff9828;font-size:10.5pt;border-bottom:1px solid #34414b;padding-bottom:4px;margin-top:14px;}"
        "p{margin:4px 0 7px 0;line-height:1.35}.muted{color:#94a1ab}.note{background:#15100b;border:1px solid #60401f;color:#ffd0a0;padding:7px;}"
        "table{border-collapse:collapse;width:100%;margin:5px 0 8px 0}th{background:#151e25;color:#ff9828;border-bottom:2px solid #ff7a00;text-align:left;padding:5px}"
        "td{border-bottom:1px solid #26323b;padding:5px;vertical-align:top}</style>");
    bool firstRow = true;
    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isEndElement()) {
            if (xml.name() == QStringLiteral("ligne")) {
                html += QStringLiteral("</tr>");
                firstRow = false;
            } else if (xml.name() == QStringLiteral("table")) {
                html += QStringLiteral("</table>");
            }
            continue;
        }
        if (!xml.isStartElement())
            continue;
        const QStringRef name = xml.name();
        if (name == QStringLiteral("titre"))
            html += QStringLiteral("<h1>%1</h1>").arg(xml.readElementText(QXmlStreamReader::IncludeChildElements).toHtmlEscaped());
        else if (name == QStringLiteral("sous-titre"))
            html += QStringLiteral("<p class='muted'>%1</p>").arg(xml.readElementText(QXmlStreamReader::IncludeChildElements).toHtmlEscaped());
        else if (name == QStringLiteral("section"))
            html += QStringLiteral("<h2>%1</h2>").arg(xml.attributes().value(QStringLiteral("titre")).toString().toHtmlEscaped());
        else if (name == QStringLiteral("p")) {
            QString text = xml.readElementText(QXmlStreamReader::IncludeChildElements).toHtmlEscaped();
            text.replace(QStringLiteral("\n"), QStringLiteral("<br>"));
            html += QStringLiteral("<p>%1</p>").arg(text);
        } else if (name == QStringLiteral("note")) {
            QString text = xml.readElementText(QXmlStreamReader::IncludeChildElements).toHtmlEscaped();
            text.replace(QStringLiteral("\n"), QStringLiteral("<br>"));
            html += QStringLiteral("<div class='note'>%1</div>").arg(text);
        } else if (name == QStringLiteral("table")) {
            firstRow = true;
            html += QStringLiteral("<table>");
        } else if (name == QStringLiteral("ligne")) {
            html += QStringLiteral("<tr>");
        } else if (name == QStringLiteral("cellule")) {
            const QString text = xml.readElementText(QXmlStreamReader::IncludeChildElements).trimmed().toHtmlEscaped();
            const QString tag = firstRow ? QStringLiteral("th") : QStringLiteral("td");
            html += QStringLiteral("<%1>%2</%1>").arg(tag, text);
        }
    }
    if (xml.hasError())
        html += QStringLiteral("<div class='note'>Fiche XML invalide ou incomplète.</div>");
    return html;
}

QString printableFirmware(QByteArray response)
{
    QByteArray printable;
    for (char byte : response) {
        const unsigned char c = static_cast<unsigned char>(byte);
        if (c >= 0x20 && c <= 0x7e)
            printable.append(byte);
    }
    QString text = QString::fromLatin1(printable).trimmed();
    if (text.size() >= 8 && text.size() % 2 == 0) {
        const int half = text.size() / 2;
        if (text.left(half) == text.mid(half))
            text = text.left(half);
    }
    return text;
}

QString injectionLabel(const QString &raw)
{
    const QString text = IaMemsConversationRouting::normalize(raw);
    if (text.contains(QStringLiteral("spi")) || text.contains(QStringLiteral("monopoint"))
        || text.contains(QStringLiteral("single point")))
        return QStringLiteral("SPi");
    if (text.contains(QStringLiteral("mpi")) || text.contains(QStringLiteral("multipoint"))
        || text.contains(QStringLiteral("multi point")))
        return QStringLiteral("MPi");
    return QString();
}

} // namespace

'''
replace_once(
    "iamemstab.cpp",
    'IaMemsTab::IaMemsTab(MainWindow *mainWindow, QWidget *parent)\n',
    xml_helpers + 'IaMemsTab::IaMemsTab(MainWindow *mainWindow, QWidget *parent)\n',
)

replace_once(
    "iamemstab.cpp",
    '    m_diagramButton->setVisible(false);\n    root->addWidget(m_diagramButton, 0, Qt::AlignLeft);\n\n    QHBoxLayout *input = new QHBoxLayout;\n',
    '    m_diagramButton->setVisible(false);\n'
    '    root->addWidget(m_diagramButton, 0, Qt::AlignLeft);\n\n'
    '    m_documentButton = new QPushButton(this);\n'
    '    m_documentButton->setObjectName(QStringLiteral("iaMemsDocumentButton"));\n'
    '    m_documentButton->setVisible(false);\n'
    '    root->addWidget(m_documentButton, 0, Qt::AlignLeft);\n\n'
    '    QHBoxLayout *input = new QHBoxLayout;\n',
)
replace_once(
    "iamemstab.cpp",
    '    connect(m_diagramButton, &QPushButton::clicked,\n            this, &IaMemsTab::openSuggestedDiagram);\n',
    '    connect(m_diagramButton, &QPushButton::clicked,\n'
    '            this, &IaMemsTab::openSuggestedDiagram);\n'
    '    connect(m_documentButton, &QPushButton::clicked,\n'
    '            this, &IaMemsTab::openSuggestedDocument);\n',
)
replace_once(
    "iamemstab.cpp",
    '                    if (!context.family.isEmpty())\n                        m_service->setContext(context);\n',
    '                    if (!context.family.isEmpty()) {\n'
    '                        m_detectedFamily = context.family;\n'
    '                        context.firmware = m_firmwareIdentifier;\n'
    '                        m_service->setContext(context);\n'
    '                    }\n',
)
replace_once(
    "iamemstab.cpp",
    '                Qt::QueuedConnection);\n    }\n\n    appendSystemMessage(QStringLiteral(\n',
    '                Qt::QueuedConnection);\n\n'
    '        connect(m_mems, &MEMSInterface::protocolResponse,\n'
    '                this,\n'
    '                [this](quint8 command, const QByteArray &response) {\n'
    '                    if (command == 0xD0 && !response.isEmpty())\n'
    '                        m_ecuIdHex = QString::fromLatin1(response.toHex(\' \')).toUpper();\n'
    '                    if (command != 0xD1)\n'
    '                        return;\n'
    '                    const QString firmware = printableFirmware(response);\n'
    '                    if (firmware.isEmpty())\n'
    '                        return;\n'
    '                    m_firmwareIdentifier = firmware;\n'
    '                    if (m_service) {\n'
    '                        ExpertContext context;\n'
    '                        context.family = m_detectedFamily;\n'
    '                        context.firmware = m_firmwareIdentifier;\n'
    '                        m_service->setContext(context);\n'
    '                    }\n'
    '                },\n'
    '                Qt::QueuedConnection);\n'
    '    }\n\n'
    '    appendSystemMessage(QStringLiteral(\n',
)

new_methods = r'''
void IaMemsTab::updateDocumentSuggestion(const QString &question)
{
    m_documentGeneration.clear();
    if (!m_documentButton)
        return;

    if (!IaMemsConversationRouting::isDocumentationQuestion(question)) {
        m_documentButton->setVisible(false);
        m_documentButton->setText(QString());
        return;
    }

    QString generation = IaMemsConversationRouting::requestedGeneration(question);
    if (generation.isEmpty())
        generation = m_detectedFamily;
    if (generation.isEmpty()) {
        m_documentButton->setVisible(false);
        m_documentButton->setText(QString());
        return;
    }

    m_documentGeneration = generation;
    m_documentButton->setText(QStringLiteral("Ouvrir la fiche XML MEMS %1").arg(generation));
    m_documentButton->setVisible(true);
}

void IaMemsTab::openSuggestedDocument()
{
    if (m_documentGeneration.isEmpty())
        return;

    MemsReferenceDatabase database;
    if (!database.open()) {
        appendSystemMessage(QStringLiteral("La base documentaire locale n'a pas pu être ouverte."));
        return;
    }
    const QString path = database.generationXmlPath(QStringLiteral("MEMS %1").arg(m_documentGeneration));
    if (path.isEmpty() || !QFileInfo::exists(path)) {
        appendSystemMessage(QStringLiteral("La fiche XML MEMS %1 n'est pas disponible dans le package.").arg(m_documentGeneration));
        return;
    }

    QDialog viewer(this);
    viewer.setObjectName(QStringLiteral("iaMemsDocumentViewer"));
    viewer.setWindowTitle(QStringLiteral("IA MEMS — Fiche XML MEMS %1").arg(m_documentGeneration));
    viewer.resize(qBound(520, width() - 40, 1000), qBound(380, height() - 40, 700));

    QVBoxLayout *layout = new QVBoxLayout(&viewer);
    QTextBrowser *browser = new QTextBrowser(&viewer);
    browser->setObjectName(QStringLiteral("iaMemsDocumentBrowser"));
    browser->setOpenExternalLinks(false);
    browser->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    browser->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    browser->setHtml(iaReferenceXmlHtml(path));
    layout->addWidget(browser, 1);

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Close, &viewer);
    connect(buttons, &QDialogButtonBox::rejected, &viewer, &QDialog::reject);
    layout->addWidget(buttons);
    viewer.exec();
}

QString IaMemsTab::resolveInductionFromKnownContext(const QString &question) const
{
    MemsReferenceDatabase database;
    if (!database.open())
        return QString();

    QStringList probes;
    if (!m_firmwareIdentifier.trimmed().isEmpty())
        probes << m_firmwareIdentifier.trimmed();
    if (IaMemsConversationRouting::mentionsMini(question))
        probes << QStringLiteral("Mini");
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
        if (inductions.size() == 1)
            return *inductions.constBegin();
    }
    return QString();
}

QString IaMemsTab::clarificationPrompt(const QString &question) const
{
    if (IaMemsConversationRouting::needsInductionClarification(question))
        return QStringLiteral("Pour éviter de mélanger les brochages Mini, est-ce une SPi ou une MPi ? Si tu ne sais pas, réponds « cherche » : j'utiliserai l'ECU connecté et la base avant de te redemander une information.");
    if (IaMemsConversationRouting::needsGenerationClarification(question, m_detectedFamily))
        return QStringLiteral("Quelle génération MEMS faut-il utiliser : 1.2, 1.3, 1.6 ou 1.9 ? Si tu ne sais pas, réponds « cherche » et j'utiliserai d'abord le contexte ECU disponible.");
    return QString();
}

void IaMemsTab::answerLocally(const QString &text)
{
    appendMessage(QStringLiteral("IA MEMS"), text);
    if (m_sendButton)
        m_sendButton->setEnabled(true);
    if (m_question) {
        m_question->setEnabled(true);
        m_question->setFocus();
    }
    updateStatus();
}

'''

old_send = r'''void IaMemsTab::sendQuestion()
{
    if (!m_question || !m_service)
        return;

    const QString question = m_question->text().trimmed();
    if (question.isEmpty())
        return;

    m_question->clear();
    appendMessage(QStringLiteral("Vous"), question);
    updateDiagramSuggestion(question);

    if (m_sendButton)
        m_sendButton->setEnabled(false);
    m_question->setEnabled(false);

    m_service->ask(question);
}
'''
new_send = r'''void IaMemsTab::sendQuestion()
{
    if (!m_question || !m_service)
        return;

    const QString question = m_question->text().trimmed();
    if (question.isEmpty())
        return;

    m_question->clear();
    appendMessage(QStringLiteral("Vous"), question);

    QString effectiveQuestion = question;
    if (!m_pendingClarificationQuestion.isEmpty()) {
        const QString pending = m_pendingClarificationQuestion;
        if (IaMemsConversationRouting::isSearchDirective(question)
            || IaMemsConversationRouting::isUnknownDirective(question)) {
            if (IaMemsConversationRouting::needsInductionClarification(pending)) {
                const QString resolved = resolveInductionFromKnownContext(pending);
                if (resolved.isEmpty()) {
                    answerLocally(QStringLiteral("J'ai cherché dans le contexte ECU connu et dans la base, mais je ne peux pas trancher SPi/MPi sans risque. Donne-moi l'année du véhicule ou la référence inscrite sur le calculateur."));
                    return;
                }
                effectiveQuestion = QStringLiteral("%1 %2").arg(pending, resolved);
                answerLocally(QStringLiteral("J'ai identifié %1 à partir des informations disponibles. Je poursuis la recherche initiale.").arg(resolved));
            } else if (IaMemsConversationRouting::needsGenerationClarification(pending, m_detectedFamily)) {
                if (m_detectedFamily.isEmpty()) {
                    answerLocally(QStringLiteral("J'ai cherché dans le contexte disponible, mais la génération MEMS n'est pas déterminée. Donne-moi l'année, la référence ECU ou connecte l'ECU pour que je puisse continuer sans deviner."));
                    return;
                }
                effectiveQuestion = QStringLiteral("%1 MEMS %2").arg(pending, m_detectedFamily);
                answerLocally(QStringLiteral("L'ECU connecté indique MEMS %1. Je poursuis donc avec cette documentation.").arg(m_detectedFamily));
            } else {
                effectiveQuestion = pending;
            }
        } else {
            effectiveQuestion = QStringLiteral("%1 %2").arg(pending, question).simplified();
        }
        m_pendingClarificationQuestion.clear();
    }

    effectiveQuestion = IaMemsConversationRouting::enrichWithKnownGeneration(effectiveQuestion, m_detectedFamily);

    const QString prompt = clarificationPrompt(effectiveQuestion);
    if (!prompt.isEmpty()) {
        m_pendingClarificationQuestion = effectiveQuestion;
        answerLocally(prompt);
        return;
    }

    updateDiagramSuggestion(effectiveQuestion);
    updateDocumentSuggestion(effectiveQuestion);

    if (m_sendButton)
        m_sendButton->setEnabled(false);
    m_question->setEnabled(false);

    m_service->ask(IaMemsConversationRouting::focusedQuestion(effectiveQuestion));
}
'''
replace_once("iamemstab.cpp", old_send, new_methods + new_send)

# 6) Extend the existing deterministic self-test with the real regression cases.
replace_once(
    "expert/IaResponseLogicTest.cpp",
    '    ok &= require(IaResponseLogic::classify(QStringLiteral("MESURE TPS ACTUELLE ?")) == IaResponseLogic::Intent::Throttle,\n                  "TPS current measurement stays a live measurement");\n',
    '    ok &= require(IaResponseLogic::classify(QStringLiteral("MESURE TPS ACTUELLE ?")) == IaResponseLogic::Intent::Throttle,\n'
    '                  "TPS current measurement stays a live measurement");\n\n'
    '    ok &= require(IaResponseLogic::classify(QStringLiteral("Broche MAP Mini MPi 1997")) == IaResponseLogic::Intent::None,\n'
    '                  "MAP pinout is documentation, not a live MAP reading");\n'
    '    ok &= require(IaResponseLogic::classify(QStringLiteral("Couleur des fils sonde lambda")) == IaResponseLogic::Intent::None,\n'
    '                  "lambda wire colours are documentation, not a live lambda reading");\n'
    '    ok &= require(IaResponseLogic::classify(QStringLiteral("Couple de serrage sonde température ECT")) == IaResponseLogic::Intent::None,\n'
    '                  "ECT torque is documentation, not a live coolant reading");\n'
    '    ok &= require(IaResponseLogic::classify(QStringLiteral("Valeur MAP ?")) == IaResponseLogic::Intent::Map,\n'
    '                  "explicit MAP value request stays live");\n'
    '    ok &= require(IaMemsConversationRouting::needsInductionClarification(QStringLiteral("Broche MAP Mini")),\n'
    '                  "ambiguous Mini pinout requests SPi/MPi clarification");\n'
    '    ok &= require(!IaMemsConversationRouting::needsInductionClarification(QStringLiteral("Broche MAP Mini MPi 1997")),\n'
    '                  "explicit MPi does not ask again");\n'
    '    ok &= require(IaMemsConversationRouting::isSearchDirective(QStringLiteral("cherche")),\n'
    '                  "search directive is recognised");\n'
    '    ok &= require(IaMemsConversationRouting::requestedGeneration(QStringLiteral("Je cherche la documentation MEMS 1.9")) == QStringLiteral("1.9"),\n'
    '                  "documentation generation extraction");\n',
)

print("IA_CONVERSATION_PATCH_PASS")
