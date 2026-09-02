from pathlib import Path
import json

root = Path(__file__).resolve().parents[1]


def replace_once(path, old, new):
    p = root / path
    text = p.read_text(encoding="utf-8")
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{path}: expected exactly one occurrence, got {count}: {old[:100]!r}")
    p.write_text(text.replace(old, new, 1), encoding="utf-8", newline="\n")


def replace_all_checked(path, old, new, expected):
    p = root / path
    text = p.read_text(encoding="utf-8")
    count = text.count(old)
    if count != expected:
        raise SystemExit(f"{path}: expected {expected} occurrences, got {count}: {old[:100]!r}")
    p.write_text(text.replace(old, new), encoding="utf-8", newline="\n")


# LocalAiClient: active MEMS Manager language governs response rendering.
replace_once(
    "expert/LocalAiClient.cpp",
    """    if (m_state != Ready) {
        emit responseError(QStringLiteral("L'IA conversationnelle locale n'est pas prête."));
        return;
    }""",
    """    if (m_state != Ready) {
        emit responseError(I18n::text(99022));
        return;
    }""")

replace_once(
    "expert/LocalAiClient.cpp",
    """    const bool reasoning = requiresReasoning(trimmedQuestion, grounding);
    if (!reasoning && !grounding.isEmpty()) {
        rememberTurn(m_conversation, trimmedQuestion, grounding);
        emit responseReady(grounding);
        return;
    }

    QString userContent = trimmedQuestion;
    if (!grounding.isEmpty()) {
        userContent += QStringLiteral(
            "\\n\\nFaits fournis par MEMS Manager, à utiliser seulement s'ils répondent à la question :\\n%1")
                           .arg(grounding);
    }
    if (reasoning) {
        userContent += QStringLiteral(
            "\\n\\nRéponse attendue : diagnostic bref, hypothèses les plus probables dans l'ordre, puis contrôles prioritaires. Ne montre aucun raisonnement interne.");
    }""",
    """    const bool reasoning = requiresReasoning(trimmedQuestion, grounding);
    const QString responseLanguageCode = activeLanguageCode();
    const QString responseLanguageName = activeLanguageName(responseLanguageCode);

    // The expert service stores neutral evidence in its source language. Only
    // French can currently be returned verbatim; every other active UI language
    // must go through Qwen so the facts are rendered in the selected language.
    if (responseLanguageCode == QStringLiteral("fr") && !reasoning && !grounding.isEmpty()) {
        rememberTurn(m_conversation, trimmedQuestion, grounding);
        emit responseReady(grounding);
        return;
    }

    QString userContent = trimmedQuestion;
    if (!grounding.isEmpty()) {
        userContent += QStringLiteral(
            "\\n\\nVerified facts supplied by MEMS Manager. Use only facts relevant to the question. "
            "These facts can be written in a different language. Answer in %1 (%2), translating and synthesizing "
            "the facts without changing technical values, units, identifiers, references or evidence:\\n%3")
                           .arg(responseLanguageName, responseLanguageCode, grounding);
    }
    if (reasoning) {
        userContent += QStringLiteral(
            "\\n\\nExpected answer: concise diagnosis, most likely hypotheses in order, then priority checks. "
            "Do not reveal internal reasoning.");
    }""")

replace_once(
    "expert/LocalAiClient.cpp",
    """                emit responseError(QStringLiteral("Erreur du moteur d'IA locale ONNX : %1").arg(generationError));""",
    """                emit responseError(I18n::text(99026).arg(generationError));""")

replace_once(
    "expert/LocalAiClient.cpp",
    """            if (answer.isEmpty() || isQuestionEcho(trimmedQuestion, answer))
                answer = grounding;
            if (answer.isEmpty()) {
                emit responseError(QStringLiteral("Le modèle local n'a pas produit de réponse exploitable dans la langue active."));
                return;
            }""",
    """            if (answer.isEmpty() || isQuestionEcho(trimmedQuestion, answer)) {
                // Never leak source-language grounding into another UI language.
                answer = activeLanguageCode() == QStringLiteral("fr") ? grounding : QString();
            }
            if (answer.isEmpty()) {
                emit responseError(I18n::text(99025));
                return;
            }""")

for old, new in [
    ('    setState(MissingRuntime, QStringLiteral("Cette compilation ne contient pas ONNX Runtime GenAI."));',
     '    setState(MissingRuntime, I18n::text(99027));'),
    ('        setState(MissingRuntime, QStringLiteral("Runtime ONNX Runtime GenAI absent du package."));',
     '        setState(MissingRuntime, I18n::text(99028));'),
    ('        setState(MissingModel, QStringLiteral("Modèle Qwen ONNX IA MEMS absent du dossier IA."));',
     '        setState(MissingModel, I18n::text(99029));'),
    ('        setState(Error, QStringLiteral("Thread du moteur IA local indisponible."));',
     '        setState(Error, I18n::text(99030));'),
    ('                setState(Error, QStringLiteral("Chargement ONNX impossible : %1").arg(error));',
     '                setState(Error, I18n::text(99031).arg(error));'),
]:
    replace_once("expert/LocalAiClient.cpp", old, new)

replace_once(
    "expert/LocalAiClient.cpp",
    """QString LocalAiClient::statusText() const
{
    switch (m_state) {
    case NotStarted: return QStringLiteral("IA locale non démarrée");
    case MissingRuntime: return QStringLiteral("moteur IA local absent");
    case MissingModel: return QStringLiteral("modèle IA local absent");
    case Starting: return QStringLiteral("IA locale en démarrage");
    case Ready: return QStringLiteral("IA locale prête");
    case Busy: return QStringLiteral("IA locale en réponse");
    case Error:
        if (m_lastError.isEmpty()) return QStringLiteral("erreur IA locale");
        return QStringLiteral("erreur IA locale : %1").arg(m_lastError.simplified().left(140));
    }
    return QStringLiteral("IA locale");
}""",
    """QString LocalAiClient::statusText() const
{
    switch (m_state) {
    case NotStarted: return I18n::text(99012);
    case MissingRuntime: return I18n::text(99013);
    case MissingModel: return I18n::text(99014);
    case Starting: return I18n::text(99015);
    case Ready: return I18n::text(99016);
    case Busy: return I18n::text(99017);
    case Error:
        if (m_lastError.isEmpty()) return I18n::text(99018);
        return I18n::text(99019).arg(m_lastError.simplified().left(140));
    }
    return I18n::text(99020);
}""")

# IaMemsService: local status/fallback strings use the existing I18n dictionaries.
replace_once("expert/IaMemsService.cpp",
             '#include "LocalAiClient.h"\n',
             '#include "LocalAiClient.h"\n#include "i18n.h"\n')

replace_once(
    "expert/IaMemsService.cpp",
    """            this, [this](const QString &message) {
                QString fallback = m_pendingGrounding.trimmed();
                if (fallback.isEmpty())
                    fallback = QStringLiteral("Je ne peux pas répondre avec le moteur conversationnel local pour le moment.");
                m_pendingGrounding.clear();
                emit responseReady(fallback);
                emit systemMessage(QStringLiteral("Moteur conversationnel local indisponible : %1").arg(message));
                emit statusChanged();
            });""",
    """            this, [this](const QString &message) {
                QString fallback;
                if (I18n::language().trimmed().toLower() == QStringLiteral("fr"))
                    fallback = m_pendingGrounding.trimmed();
                if (fallback.isEmpty())
                    fallback = I18n::text(99023);
                m_pendingGrounding.clear();
                emit responseReady(fallback);
                emit systemMessage(I18n::text(99024).arg(message));
                emit statusChanged();
            });""")

for old, new in [
    ('        m_knowledgeError = QStringLiteral("Base experte r20 absente du package.");',
     '        m_knowledgeError = I18n::text(99033);'),
    ('''        emit systemMessage(QStringLiteral(
            "La base experte MEMS préconstruite n'est pas disponible. Le dialogue local reste utilisable sans cette base."));''',
     '        emit systemMessage(I18n::text(99034));'),
    ('''        emit systemMessage(QStringLiteral(
            "La base experte MEMS n'a pas pu être ouverte en lecture seule. Le dialogue local reste utilisable."));''',
     '        emit systemMessage(I18n::text(99035));'),
    ('    emit systemMessage(QStringLiteral("Base de connaissances MEMS prête en lecture seule."));',
     '    emit systemMessage(I18n::text(99021));'),
]:
    replace_once("expert/IaMemsService.cpp", old, new)

replace_once(
    "expert/IaMemsService.cpp",
    """    const QString fallback = m_pendingGrounding.trimmed().isEmpty()
        ? QStringLiteral("L'IA locale n'est pas encore prête.")
        : m_pendingGrounding;
    m_pendingGrounding.clear();""",
    """    QString fallback;
    if (I18n::language().trimmed().toLower() == QStringLiteral("fr"))
        fallback = m_pendingGrounding.trimmed();
    if (fallback.isEmpty())
        fallback = I18n::text(99022);
    m_pendingGrounding.clear();""")

replace_once(
    "expert/IaMemsService.cpp",
    """    if (m_knowledgeReady)
        parts << QStringLiteral("base prête");
    else if (m_knowledgeAttempted)
        parts << QStringLiteral("base indisponible");
    else
        parts << QStringLiteral("base non chargée");""",
    """    if (m_knowledgeReady)
        parts << I18n::text(99009);
    else if (m_knowledgeAttempted)
        parts << I18n::text(99010);
    else
        parts << I18n::text(99011);""")

# IA tab user-visible strings.
replace_once("iamemstab.cpp",
             '#include "database/MemsReferenceSheetRenderer.h"\n',
             '#include "database/MemsReferenceSheetRenderer.h"\n#include "i18n.h"\n')
replace_once("iamemstab.cpp",
             '        path, QStringLiteral("Impossible d\'ouvrir ou de lire la fiche XML locale."));',
             '        path, I18n::text(99041));')
replace_once("iamemstab.cpp",
             '''        return QStringLiteral(
            "MPi signifie « Multi Point Injection », c'est-à-dire injection multipoint. "
            "Sur les montages Rover/Mini concernés, plusieurs injecteurs distribuent le carburant, typiquement un injecteur par cylindre.");''',
             '        return I18n::text(99049);')
replace_once("iamemstab.cpp",
             '''        return QStringLiteral(
            "Sur un système MPi Rover/Mini MEMS, l'injection est multipoint : plusieurs injecteurs distribuent le carburant, typiquement un par cylindre. "
            "L'ECU commande leur ouverture pour doser le carburant dans l'admission. Si tu cherches le brochage, le câblage, un contrôle ou une procédure précise, indique simplement ce point et je chercherai la donnée correspondante.");''',
             '        return I18n::text(99050);')
replace_once("iamemstab.cpp",
             '''    QLabel *subtitle = new QLabel(
        QStringLiteral("Dialogue local avec le moteur expert et la base de connaissances MEMS. "
                       "Les réponses distinguent mesures, hypothèses et niveau de preuve."),
        this);''',
             '    QLabel *subtitle = new QLabel(I18n::text(99001), this);')
replace_once("iamemstab.cpp",
             '''    m_question->setPlaceholderText(QStringLiteral(
        "Posez une question : Qu'est-ce que tu vois d'anormal ? Mon avance est-elle normale ? Que sait-on sur AANMP002 ?"));''',
             '    m_question->setPlaceholderText(I18n::text(99002));')
replace_once("iamemstab.cpp", '    m_sendButton = new QPushButton(QStringLiteral("Envoyer"), this);',
             '    m_sendButton = new QPushButton(I18n::text(99003), this);')
replace_once("iamemstab.cpp",
             '''    appendSystemMessage(QStringLiteral(
        "Bonjour, je suis IA MEMS, l'assistant intégré à ECU MEMS Manager. "
        "Je peux expliquer le logiciel, les systèmes MEMS et les mesures déjà acquises par le programme. "
        "Je n'envoie aucune commande ECU et je n'invente pas une mesure absente."));''',
             '    appendSystemMessage(I18n::text(99004));')
replace_all_checked("iamemstab.cpp", 'm_diagramButton->setText(QStringLiteral("Voir le schéma"));',
                    'm_diagramButton->setText(I18n::text(99006));', 2)
replace_once("iamemstab.cpp",
             '''        appendSystemMessage(QStringLiteral(
            "Le schéma local proposé n'est plus disponible ou déclaré dans le package."));''',
             '        appendSystemMessage(I18n::text(99036));')
replace_once("iamemstab.cpp", '    m_documentButton->setText(QStringLiteral("Ouvrir la fiche XML MEMS %1").arg(generation));',
             '    m_documentButton->setText(I18n::text(99037).arg(generation));')
replace_once("iamemstab.cpp", '        appendSystemMessage(QStringLiteral("La base documentaire locale n\'a pas pu être ouverte."));',
             '        appendSystemMessage(I18n::text(99038));')
replace_once("iamemstab.cpp", '        appendSystemMessage(QStringLiteral("La fiche XML MEMS %1 n\'est pas disponible dans le package.").arg(m_documentGeneration));',
             '        appendSystemMessage(I18n::text(99039).arg(m_documentGeneration));')
replace_once("iamemstab.cpp", '    viewer.setWindowTitle(QStringLiteral("IA MEMS — Fiche XML MEMS %1").arg(m_documentGeneration));',
             '    viewer.setWindowTitle(I18n::text(99040).arg(m_documentGeneration));')
replace_once("iamemstab.cpp", '        return QStringLiteral("Pour éviter de mélanger les brochages Mini, est-ce une SPi ou une MPi ? Si tu ne sais pas, réponds « cherche » : j\'utiliserai l\'ECU connecté et la base avant de te redemander une information.");',
             '        return I18n::text(99042);')
replace_once("iamemstab.cpp", '        return QStringLiteral("Quelle génération MEMS faut-il utiliser : 1.2, 1.3, 1.6 ou 1.9 ? Si tu ne sais pas, réponds « cherche » et j\'utiliserai d\'abord le contexte ECU disponible.");',
             '        return I18n::text(99043);')
replace_once("iamemstab.cpp", '    appendMessage(QStringLiteral("Vous"), question);',
             '    appendMessage(I18n::text(99005), question);')
replace_once("iamemstab.cpp",
             '''                          QStringLiteral("Correction prise en compte : %1. Je reprends la demande précédente avec cette variante.")
                              .arg(explicitSelection));''',
             '                          I18n::text(99044).arg(explicitSelection));')
replace_once("iamemstab.cpp", '                    answerLocally(QStringLiteral("J\'ai cherché dans les références ECU réellement disponibles, mais je ne peux pas trancher SPi/MPi sans preuve suffisante. Donne-moi la référence inscrite sur le calculateur, ou l\'année et le marché du véhicule."));',
             '                    answerLocally(I18n::text(99045));')
replace_once("iamemstab.cpp", '                answerLocally(QStringLiteral("J\'ai identifié %1 à partir de la référence ECU/firmware %2 retrouvée dans la base. Je poursuis la recherche initiale.").arg(resolved, evidence));',
             '                answerLocally(I18n::text(99046).arg(resolved, evidence));')
replace_once("iamemstab.cpp", '                    answerLocally(QStringLiteral("J\'ai cherché dans le contexte disponible, mais la génération MEMS n\'est pas déterminée. Donne-moi l\'année, la référence ECU ou connecte l\'ECU pour que je puisse continuer sans deviner."));',
             '                    answerLocally(I18n::text(99047));')
replace_once("iamemstab.cpp", '                answerLocally(QStringLiteral("L\'ECU connecté indique MEMS %1. Je poursuis donc avec cette documentation.").arg(m_detectedFamily));',
             '                answerLocally(I18n::text(99048).arg(m_detectedFamily));')
replace_once("iamemstab.cpp", '    parts << (m_connected ? QStringLiteral("ECU connecté") : QStringLiteral("ECU non connecté"));',
             '    parts << (m_connected ? I18n::text(99007) : I18n::text(99008));')

# Native self-test: verify all six resource dictionaries and cross-language grounding.
selftest = r'''#include "LocalAiClient.h"
#include "i18n.h"

#include <QCoreApplication>
#include <QDate>
#include <QTextStream>
#include <QTimer>

namespace {
bool containsInternalLeak(const QString &text)
{
    const QString lower = text.toLower();
    const QStringList markers = {
        QStringLiteral("<think>"), QStringLiteral("</think>"), QStringLiteral("<|im_"),
        QStringLiteral("langage obligatoire"), QStringLiteral("mandatory language"),
        QStringLiteral("domaine obligatoire"), QStringLiteral("contexte candidat fourni par mems manager"),
        QStringLiteral("mode diagnostic rapide"), QStringLiteral("you are ia mems, the local assistant"),
        QStringLiteral("réponse attendue"), QStringLiteral("reponse attendue"),
        QStringLiteral("diagnostic bref"), QStringLiteral("ne montre aucun raisonnement interne"),
        QStringLiteral("faits fournis par mems manager")
    };
    for (const QString &marker : markers)
        if (lower.contains(marker)) return true;
    return false;
}
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    I18n::load(QStringLiteral("fr"));
    QTextStream out(stdout), err(stderr);
    out.setCodec("UTF-8"); err.setCodec("UTF-8");

    LocalAiClient client;
    bool started = false, finished = false;
    int stage = 0, languageIndex = -1;
    const QStringList languageCodes = {"fr","en","es","it","pt","de"};
    const QStringList generatedCodes = {"it","en","es","pt","de"};
    const QStringList generatedMarkers = {"IT_OK","EN_OK","ES_OK","PT_OK","DE_OK"};
    const QStringList generatedQuestions = {
        QStringLiteral("Rispondi iniziando esattamente con IT_OK. Qual è la coppia di serraggio?"),
        QStringLiteral("Answer starting exactly with EN_OK. What is the tightening torque?"),
        QStringLiteral("Responde empezando exactamente con ES_OK. ¿Cuál es el par de apriete?"),
        QStringLiteral("Responde começando exatamente com PT_OK. Qual é o binário de aperto?"),
        QStringLiteral("Antworte exakt beginnend mit DE_OK. Wie hoch ist das Anzugsdrehmoment?")
    };
    const QString foreignGrounding = QStringLiteral("Couple de serrage vérifié : 68 Nm. Niveau de preuve : constructeur.");

    const auto fail = [&](const QString &message) {
        if (finished) return;
        finished = true;
        err << "FAIL LocalAiClient ONNX: " << message << Qt::endl;
        app.exit(1);
    };

    for (const QString &code : languageCodes) {
        if (!I18n::load(code)) {
            fail(QStringLiteral("Dictionnaire IA MEMS non chargé pour %1").arg(code));
            return 1;
        }
        const QString ready = I18n::text(99016);
        if (ready.startsWith(QLatin1Char('[')) || ready.trimmed().isEmpty()) {
            fail(QStringLiteral("Clé IA MEMS 99016 absente pour %1").arg(code));
            return 1;
        }
    }
    I18n::load(QStringLiteral("fr"));

    const auto rejectLeak = [&](const QString &answer) -> bool {
        if (!containsInternalLeak(answer)) return false;
        fail(QStringLiteral("Directive interne ou balise Qwen dans la réponse : %1").arg(answer.left(180)));
        return true;
    };

    const auto askLanguageCase = [&]() {
        I18n::load(generatedCodes.at(languageIndex));
        client.clearConversation();
        client.ask(generatedQuestions.at(languageIndex), foreignGrounding);
    };

    QObject::connect(&client, &LocalAiClient::stateChanged, &app, [&]() {
        out << "STATE=" << client.statusText() << Qt::endl;
        switch (client.state()) {
        case LocalAiClient::Ready:
            if (!started) {
                started = true; stage = 0; I18n::load(QStringLiteral("fr"));
                client.ask(QStringLiteral("C4EST QUOI LA BOBINE ?"), QString());
            }
            break;
        case LocalAiClient::MissingRuntime:
        case LocalAiClient::MissingModel:
        case LocalAiClient::Error:
            fail(client.statusText()); break;
        default: break;
        }
    });

    QObject::connect(&client, &LocalAiClient::responseError, &app,
                     [&](const QString &message) { fail(message); });

    QObject::connect(&client, &LocalAiClient::responseReady, &app, [&](const QString &text) {
        if (finished) return;
        const QString answer = text.trimmed();
        out << "ANSWER_STAGE_" << stage << "=" << answer << Qt::endl;
        if (answer.isEmpty()) { fail(QStringLiteral("Réponse vide étape %1").arg(stage)); return; }
        if (rejectLeak(answer)) return;

        if (stage == 0) {
            if (!answer.contains("bobine", Qt::CaseInsensitive) || !answer.contains("haute tension", Qt::CaseInsensitive) || !answer.contains("bougie", Qt::CaseInsensitive)) { fail("Définition bobine invalide"); return; }
            stage=1; client.ask(QStringLiteral("QUELLE JOURS SOMME NOUS ?"), QString()); return;
        }
        if (stage == 1) {
            if (!answer.contains(QString::number(QDate::currentDate().year())) || !answer.contains("Nous sommes", Qt::CaseInsensitive)) { fail("Date non reconnue"); return; }
            stage=2; client.ask(QStringLiteral("BROCHE ECU 1.3"), QStringLiteral("BROCHAGE_TEST_ECU_13 : information de brochage vérifiée fournie par la base.")); return;
        }
        if (stage == 2) {
            if (!answer.contains("BROCHAGE_TEST_ECU_13") || answer.contains("Micro-Electro-Mechanical", Qt::CaseInsensitive)) { fail("BROCHE ECU 1.3 détourné"); return; }
            stage=3; client.ask(QStringLiteral("BROCHE OBD 1.9 ?"), QStringLiteral("BROCHAGE_TEST_OBD_19 : information OBD vérifiée fournie par la base.")); return;
        }
        if (stage == 3) {
            if (!answer.contains("BROCHAGE_TEST_OBD_19") || answer.contains("Micro-Electro-Mechanical", Qt::CaseInsensitive)) { fail("BROCHE OBD 1.9 détourné"); return; }
            stage=4; client.ask(QStringLiteral("Couple de serrage sonde température ECT"), QStringLiteral("Couple ECT vérifié : 15 Nm.\nNiveau de preuve : constructeur.")); return;
        }
        if (stage == 4) {
            if (!answer.contains("15 Nm", Qt::CaseInsensitive)) { fail("Fait ECT remplacé"); return; }
            stage=5; client.ask(QStringLiteral("Couleur des fils sonde lambda"), QStringLiteral("Fils sonde lambda : gris et vert clair/gris — preuve : constructeur.")); return;
        }
        if (stage == 5) {
            if (!answer.contains("gris", Qt::CaseInsensitive) || answer.contains("réponse attendue", Qt::CaseInsensitive) || answer.contains("diagnostic bref", Qt::CaseInsensitive)) { fail("Réponse lambda invalide"); return; }
            stage=6; client.ask(QStringLiteral("Réponds uniquement par OK."), QString()); return;
        }
        if (stage == 6) {
            if (!answer.contains("OK", Qt::CaseInsensitive)) { fail("Marqueur OK absent"); return; }
            stage=7; languageIndex=0; askLanguageCase(); return;
        }
        if (stage == 7) {
            const QString marker=generatedMarkers.at(languageIndex), code=generatedCodes.at(languageIndex);
            if (!answer.contains(marker, Qt::CaseInsensitive) || !answer.contains("68 Nm", Qt::CaseInsensitive)) { fail(QStringLiteral("Restitution %1 invalide: %2").arg(code, answer.left(180))); return; }
            if (answer == foreignGrounding || answer.contains("Couple de serrage vérifié", Qt::CaseInsensitive)) { fail(QStringLiteral("Grounding français brut en %1").arg(code)); return; }
            out << "MULTILINGUAL_PASS=" << code << Qt::endl;
            ++languageIndex;
            if (languageIndex < generatedCodes.size()) { askLanguageCase(); return; }
            I18n::load(QStringLiteral("fr"));
            finished=true;
            out << "PASS LocalAiClient native ONNX response quality, documentary grounding, multilingual rendering and leak guards" << Qt::endl;
            app.exit(0);
        }
    });

    QTimer::singleShot(300000, &app, [&]() { fail(QStringLiteral("Délai dépassé pendant le test ONNX.")); });
    client.initialize();
    const int result=app.exec();
    client.shutdown();
    return result;
}
'''
(root / "expert/LocalAiOnnxSelfTest.cpp").write_text(selftest, encoding="utf-8", newline="\n")

keys = list(range(99001, 99032)) + list(range(99033, 99051))
translations = {
"fr": [
"Dialogue local avec le moteur expert et la base de connaissances MEMS. Les réponses distinguent mesures, hypothèses et niveau de preuve.",
"Posez une question : Qu'est-ce que tu vois d'anormal ? Mon avance est-elle normale ? Que sait-on sur AANMP002 ?",
"Envoyer","Bonjour, je suis IA MEMS, l'assistant intégré à ECU MEMS Manager. Je peux expliquer le logiciel, les systèmes MEMS et les mesures déjà acquises par le programme. Je n'envoie aucune commande ECU et je n'invente pas une mesure absente.","Vous","Voir le schéma","ECU connecté","ECU non connecté","base prête","base indisponible","base non chargée","IA locale non démarrée","moteur IA local absent","modèle IA local absent","IA locale en démarrage","IA locale prête","IA locale en réponse","erreur IA locale","erreur IA locale : %1","IA locale","Base de connaissances MEMS prête en lecture seule.","L'IA locale n'est pas encore prête.","Je ne peux pas répondre avec le moteur conversationnel local pour le moment.","Moteur conversationnel local indisponible : %1","Le modèle local n'a pas produit de réponse exploitable dans la langue active.","Erreur du moteur d'IA locale ONNX : %1","Cette compilation ne contient pas ONNX Runtime GenAI.","Runtime ONNX Runtime GenAI absent du package.","Modèle Qwen ONNX IA MEMS absent du dossier IA.","Thread du moteur IA local indisponible.","Chargement ONNX impossible : %1",
"Base experte r20 absente du package.","La base experte MEMS préconstruite n'est pas disponible. Le dialogue local reste utilisable sans cette base.","La base experte MEMS n'a pas pu être ouverte en lecture seule. Le dialogue local reste utilisable.","Le schéma local proposé n'est plus disponible ou déclaré dans le package.","Ouvrir la fiche XML MEMS %1","La base documentaire locale n'a pas pu être ouverte.","La fiche XML MEMS %1 n'est pas disponible dans le package.","IA MEMS — Fiche XML MEMS %1","Impossible d'ouvrir ou de lire la fiche XML locale.","Pour éviter de mélanger les brochages Mini, est-ce une SPi ou une MPi ? Si tu ne sais pas, réponds « cherche » : j'utiliserai l'ECU connecté et la base avant de te redemander une information.","Quelle génération MEMS faut-il utiliser : 1.2, 1.3, 1.6 ou 1.9 ? Si tu ne sais pas, réponds « cherche » et j'utiliserai d'abord le contexte ECU disponible.","Correction prise en compte : %1. Je reprends la demande précédente avec cette variante.","J'ai cherché dans les références ECU réellement disponibles, mais je ne peux pas trancher SPi/MPi sans preuve suffisante. Donne-moi la référence inscrite sur le calculateur, ou l'année et le marché du véhicule.","J'ai identifié %1 à partir de la référence ECU/firmware %2 retrouvée dans la base. Je poursuis la recherche initiale.","J'ai cherché dans le contexte disponible, mais la génération MEMS n'est pas déterminée. Donne-moi l'année, la référence ECU ou connecte l'ECU pour que je puisse continuer sans deviner.","L'ECU connecté indique MEMS %1. Je poursuis donc avec cette documentation.","MPi signifie « Multi Point Injection », c'est-à-dire injection multipoint. Sur les montages Rover/Mini concernés, plusieurs injecteurs distribuent le carburant, typiquement un injecteur par cylindre.","Sur un système MPi Rover/Mini MEMS, l'injection est multipoint : plusieurs injecteurs distribuent le carburant, typiquement un par cylindre. L'ECU commande leur ouverture pour doser le carburant dans l'admission. Si tu cherches le brochage, le câblage, un contrôle ou une procédure précise, indique simplement ce point et je chercherai la donnée correspondante."
],
"en": [
"Local dialogue with the expert engine and the MEMS knowledge base. Answers distinguish measurements, hypotheses and evidence level.","Ask a question: What do you see that is abnormal? Is my ignition advance normal? What do we know about AANMP002?","Send","Hello, I am IA MEMS, the assistant integrated into ECU MEMS Manager. I can explain the software, MEMS systems and measurements already acquired by the program. I do not send ECU commands and I do not invent missing measurements.","You","View diagram","ECU connected","ECU not connected","database ready","database unavailable","database not loaded","local AI not started","local AI engine missing","local AI model missing","local AI starting","local AI ready","local AI responding","local AI error","local AI error: %1","local AI","MEMS knowledge base ready in read-only mode.","The local AI is not ready yet.","I cannot answer with the local conversational engine at the moment.","Local conversational engine unavailable: %1","The local model did not produce a usable answer in the active language.","Local ONNX AI engine error: %1","This build does not include ONNX Runtime GenAI.","ONNX Runtime GenAI runtime is missing from the package.","Qwen ONNX IA MEMS model is missing from the AI folder.","Local AI engine thread unavailable.","Unable to load ONNX: %1","Expert r20 database is missing from the package.","The prebuilt MEMS expert database is unavailable. Local dialogue remains usable without this database.","The MEMS expert database could not be opened read-only. Local dialogue remains usable.","The proposed local diagram is no longer available or declared in the package.","Open MEMS %1 XML sheet","The local documentation database could not be opened.","The MEMS %1 XML sheet is not available in the package.","IA MEMS — MEMS %1 XML sheet","Unable to open or read the local XML sheet.","To avoid mixing Mini pinouts, is this an SPi or an MPi? If you do not know, answer “search”: I will use the connected ECU and the database before asking you again.","Which MEMS generation should be used: 1.2, 1.3, 1.6 or 1.9? If you do not know, answer “search” and I will use the available ECU context first.","Correction accepted: %1. I am resuming the previous request with this variant.","I searched the ECU references actually available, but I cannot distinguish SPi/MPi with sufficient evidence. Give me the reference printed on the ECU, or the vehicle year and market.","I identified %1 from ECU/firmware reference %2 found in the database. I am continuing the original search.","I searched the available context, but the MEMS generation is not determined. Give me the year, ECU reference or connect the ECU so I can continue without guessing.","The connected ECU indicates MEMS %1. I will therefore continue with this documentation.","MPi means Multi Point Injection. On the relevant Rover/Mini systems, several injectors distribute fuel, typically one injector per cylinder.","On a Rover/Mini MEMS MPi system, injection is multipoint: several injectors distribute fuel, typically one per cylinder. The ECU controls their opening to meter fuel into the intake. If you need a pinout, wiring, a check or a specific procedure, state that point and I will look for the corresponding data."
],
"es": [
"Diálogo local con el motor experto y la base de conocimientos MEMS. Las respuestas distinguen medidas, hipótesis y nivel de evidencia.","Haz una pregunta: ¿Qué ves de anormal? ¿Mi avance de encendido es normal? ¿Qué sabemos sobre AANMP002?","Enviar","Hola, soy IA MEMS, el asistente integrado en ECU MEMS Manager. Puedo explicar el programa, los sistemas MEMS y las medidas ya adquiridas por la aplicación. No envío comandos a la ECU ni invento medidas ausentes.","Tú","Ver esquema","ECU conectada","ECU no conectada","base lista","base no disponible","base no cargada","IA local no iniciada","motor de IA local ausente","modelo de IA local ausente","IA local iniciándose","IA local lista","IA local respondiendo","error de IA local","error de IA local: %1","IA local","Base de conocimientos MEMS lista en modo de solo lectura.","La IA local todavía no está lista.","No puedo responder con el motor conversacional local en este momento.","Motor conversacional local no disponible: %1","El modelo local no ha producido una respuesta utilizable en el idioma activo.","Error del motor de IA local ONNX: %1","Esta compilación no incluye ONNX Runtime GenAI.","Falta el runtime ONNX Runtime GenAI en el paquete.","Falta el modelo Qwen ONNX IA MEMS en la carpeta de IA.","El hilo del motor de IA local no está disponible.","No se puede cargar ONNX: %1","Falta la base experta r20 en el paquete.","La base experta MEMS precompilada no está disponible. El diálogo local sigue siendo utilizable sin esta base.","No se ha podido abrir la base experta MEMS en modo de solo lectura. El diálogo local sigue siendo utilizable.","El esquema local propuesto ya no está disponible o declarado en el paquete.","Abrir ficha XML MEMS %1","No se ha podido abrir la base documental local.","La ficha XML MEMS %1 no está disponible en el paquete.","IA MEMS — Ficha XML MEMS %1","No se puede abrir o leer la ficha XML local.","Para evitar mezclar los pinouts de Mini, ¿es una SPi o una MPi? Si no lo sabes, responde «buscar»: usaré primero la ECU conectada y la base.","¿Qué generación MEMS debe utilizarse: 1.2, 1.3, 1.6 o 1.9? Si no lo sabes, responde «buscar» y usaré primero el contexto ECU disponible.","Corrección aplicada: %1. Retomo la solicitud anterior con esta variante.","He buscado en las referencias ECU realmente disponibles, pero no puedo distinguir SPi/MPi con evidencia suficiente. Indícame la referencia de la centralita, o el año y el mercado del vehículo.","He identificado %1 a partir de la referencia ECU/firmware %2 encontrada en la base. Continúo la búsqueda inicial.","He buscado en el contexto disponible, pero la generación MEMS no está determinada. Indícame el año, la referencia ECU o conecta la ECU para continuar sin adivinar.","La ECU conectada indica MEMS %1. Continuaré con esta documentación.","MPi significa «Multi Point Injection», es decir, inyección multipunto. En los sistemas Rover/Mini correspondientes, varios inyectores distribuyen el combustible, normalmente uno por cilindro.","En un sistema MPi Rover/Mini MEMS, la inyección es multipunto: varios inyectores distribuyen el combustible, normalmente uno por cilindro. La ECU controla su apertura para dosificar el combustible en la admisión. Si buscas pinout, cableado, una comprobación o un procedimiento concreto, indícalo y buscaré el dato correspondiente."
],
"it": [
"Dialogo locale con il motore esperto e la base di conoscenze MEMS. Le risposte distinguono misure, ipotesi e livello di prova.","Fai una domanda: Cosa vedi di anomalo? Il mio anticipo è normale? Cosa sappiamo di AANMP002?","Invia","Buongiorno, sono IA MEMS, l'assistente integrato in ECU MEMS Manager. Posso spiegare il software, i sistemi MEMS e le misure già acquisite dal programma. Non invio alcun comando alla ECU e non invento misure assenti.","Tu","Vedi lo schema","ECU connessa","ECU non connessa","base pronta","base non disponibile","base non caricata","IA locale non avviata","motore IA locale assente","modello IA locale assente","IA locale in avvio","IA locale pronta","IA locale in risposta","errore IA locale","errore IA locale: %1","IA locale","Base di conoscenze MEMS pronta in sola lettura.","L'IA locale non è ancora pronta.","Al momento non posso rispondere con il motore conversazionale locale.","Motore conversazionale locale non disponibile: %1","Il modello locale non ha prodotto una risposta utilizzabile nella lingua attiva.","Errore del motore IA locale ONNX: %1","Questa compilazione non include ONNX Runtime GenAI.","Runtime ONNX Runtime GenAI assente dal pacchetto.","Modello Qwen ONNX IA MEMS assente dalla cartella IA.","Thread del motore IA locale non disponibile.","Impossibile caricare ONNX: %1","Base esperta r20 assente dal pacchetto.","La base esperta MEMS precompilata non è disponibile. Il dialogo locale resta utilizzabile senza questa base.","La base esperta MEMS non può essere aperta in sola lettura. Il dialogo locale resta utilizzabile.","Lo schema locale proposto non è più disponibile o dichiarato nel pacchetto.","Apri la scheda XML MEMS %1","Impossibile aprire la base documentale locale.","La scheda XML MEMS %1 non è disponibile nel pacchetto.","IA MEMS — Scheda XML MEMS %1","Impossibile aprire o leggere la scheda XML locale.","Per evitare di mescolare i pinout Mini, si tratta di una SPi o di una MPi? Se non lo sai, rispondi «cerca»: userò prima la ECU connessa e la base.","Quale generazione MEMS devo usare: 1.2, 1.3, 1.6 o 1.9? Se non lo sai, rispondi «cerca» e userò prima il contesto ECU disponibile.","Correzione acquisita: %1. Riprendo la richiesta precedente con questa variante.","Ho cercato nei riferimenti ECU realmente disponibili, ma non posso distinguere SPi/MPi con prove sufficienti. Indicami il riferimento sulla centralina oppure l'anno e il mercato del veicolo.","Ho identificato %1 dal riferimento ECU/firmware %2 trovato nella base. Proseguo con la ricerca iniziale.","Ho cercato nel contesto disponibile, ma la generazione MEMS non è determinata. Indicami l'anno, il riferimento ECU oppure collega la ECU per continuare senza supposizioni.","La ECU connessa indica MEMS %1. Proseguo quindi con questa documentazione.","MPi significa «Multi Point Injection», cioè iniezione multipoint. Sui sistemi Rover/Mini interessati, più iniettori distribuiscono il carburante, tipicamente uno per cilindro.","Su un sistema MPi Rover/Mini MEMS, l'iniezione è multipoint: più iniettori distribuiscono il carburante, tipicamente uno per cilindro. La ECU ne comanda l'apertura per dosare il carburante nell'aspirazione. Se cerchi pinout, cablaggio, controllo o procedura specifica, indica semplicemente quel punto e cercherò il dato corrispondente."
],
"pt": [
"Diálogo local com o motor especialista e a base de conhecimentos MEMS. As respostas distinguem medições, hipóteses e nível de evidência.","Faça uma pergunta: O que vê de anormal? O meu avanço de ignição é normal? O que sabemos sobre AANMP002?","Enviar","Olá, sou a IA MEMS, o assistente integrado no ECU MEMS Manager. Posso explicar o software, os sistemas MEMS e as medições já adquiridas pelo programa. Não envio comandos à ECU nem invento medições ausentes.","Você","Ver esquema","ECU ligada","ECU não ligada","base pronta","base indisponível","base não carregada","IA local não iniciada","motor de IA local ausente","modelo de IA local ausente","IA local a iniciar","IA local pronta","IA local a responder","erro da IA local","erro da IA local: %1","IA local","Base de conhecimentos MEMS pronta em modo só de leitura.","A IA local ainda não está pronta.","Neste momento não consigo responder com o motor de conversação local.","Motor de conversação local indisponível: %1","O modelo local não produziu uma resposta utilizável no idioma ativo.","Erro do motor de IA local ONNX: %1","Esta compilação não inclui o ONNX Runtime GenAI.","O runtime ONNX Runtime GenAI não está presente no pacote.","O modelo Qwen ONNX IA MEMS não está presente na pasta de IA.","Thread do motor de IA local indisponível.","Não foi possível carregar o ONNX: %1","A base especialista r20 não está presente no pacote.","A base especialista MEMS pré-compilada não está disponível. O diálogo local continua utilizável sem esta base.","Não foi possível abrir a base especialista MEMS em modo só de leitura. O diálogo local continua utilizável.","O esquema local proposto já não está disponível ou declarado no pacote.","Abrir ficha XML MEMS %1","Não foi possível abrir a base documental local.","A ficha XML MEMS %1 não está disponível no pacote.","IA MEMS — Ficha XML MEMS %1","Não foi possível abrir ou ler a ficha XML local.","Para evitar misturar os pinouts Mini, é uma SPi ou uma MPi? Se não souber, responda «procurar»: usarei primeiro a ECU ligada e a base.","Que geração MEMS deve ser usada: 1.2, 1.3, 1.6 ou 1.9? Se não souber, responda «procurar» e usarei primeiro o contexto ECU disponível.","Correção aplicada: %1. Retomo o pedido anterior com esta variante.","Procurei nas referências ECU realmente disponíveis, mas não consigo distinguir SPi/MPi com evidência suficiente. Indique a referência inscrita na ECU, ou o ano e o mercado do veículo.","Identifiquei %1 a partir da referência ECU/firmware %2 encontrada na base. Vou continuar a pesquisa inicial.","Procurei no contexto disponível, mas a geração MEMS não está determinada. Indique o ano, a referência ECU ou ligue a ECU para continuar sem adivinhar.","A ECU ligada indica MEMS %1. Vou continuar com esta documentação.","MPi significa «Multi Point Injection», ou seja, injeção multiponto. Nos sistemas Rover/Mini em causa, vários injetores distribuem o combustível, normalmente um por cilindro.","Num sistema MPi Rover/Mini MEMS, a injeção é multiponto: vários injetores distribuem o combustível, normalmente um por cilindro. A ECU controla a abertura para dosear o combustível na admissão. Se procura pinout, cablagem, uma verificação ou um procedimento específico, indique esse ponto e procurarei o dado correspondente."
],
"de": [
"Lokaler Dialog mit der Expertenlogik und der MEMS-Wissensdatenbank. Die Antworten unterscheiden Messwerte, Hypothesen und Belegniveau.","Stellen Sie eine Frage: Was ist auffällig? Ist meine Zündverstellung normal? Was wissen wir über AANMP002?","Senden","Hallo, ich bin IA MEMS, der in ECU MEMS Manager integrierte Assistent. Ich kann die Software, MEMS-Systeme und bereits vom Programm erfasste Messwerte erklären. Ich sende keine ECU-Befehle und erfinde keine fehlenden Messwerte.","Sie","Schaltbild anzeigen","ECU verbunden","ECU nicht verbunden","Datenbank bereit","Datenbank nicht verfügbar","Datenbank nicht geladen","lokale KI nicht gestartet","lokale KI-Engine fehlt","lokales KI-Modell fehlt","lokale KI startet","lokale KI bereit","lokale KI antwortet","Fehler der lokalen KI","Fehler der lokalen KI: %1","lokale KI","MEMS-Wissensdatenbank ist schreibgeschützt bereit.","Die lokale KI ist noch nicht bereit.","Der lokale Dialogmotor kann derzeit keine Antwort liefern.","Lokaler Dialogmotor nicht verfügbar: %1","Das lokale Modell hat keine verwertbare Antwort in der aktiven Sprache erzeugt.","Fehler der lokalen ONNX-KI-Engine: %1","Dieser Build enthält ONNX Runtime GenAI nicht.","ONNX Runtime GenAI fehlt im Paket.","Das Qwen-ONNX-Modell für IA MEMS fehlt im KI-Ordner.","Thread der lokalen KI-Engine nicht verfügbar.","ONNX konnte nicht geladen werden: %1","Die Experten-Datenbank r20 fehlt im Paket.","Die vorkompilierte MEMS-Expertendatenbank ist nicht verfügbar. Der lokale Dialog bleibt ohne diese Datenbank nutzbar.","Die MEMS-Expertendatenbank konnte nicht schreibgeschützt geöffnet werden. Der lokale Dialog bleibt nutzbar.","Das vorgeschlagene lokale Schaltbild ist im Paket nicht mehr verfügbar oder deklariert.","MEMS-%1-XML-Datenblatt öffnen","Die lokale Dokumentationsdatenbank konnte nicht geöffnet werden.","Das MEMS-%1-XML-Datenblatt ist im Paket nicht verfügbar.","IA MEMS — MEMS-%1-XML-Datenblatt","Das lokale XML-Datenblatt konnte nicht geöffnet oder gelesen werden.","Um Mini-Pinbelegungen nicht zu vermischen: Handelt es sich um SPi oder MPi? Falls unbekannt, antworten Sie „suchen“; ich nutze zuerst die verbundene ECU und die Datenbank.","Welche MEMS-Generation soll verwendet werden: 1.2, 1.3, 1.6 oder 1.9? Falls unbekannt, antworten Sie „suchen“; ich nutze zuerst den verfügbaren ECU-Kontext.","Korrektur übernommen: %1. Ich setze die vorherige Anfrage mit dieser Variante fort.","Ich habe die tatsächlich verfügbaren ECU-Referenzen geprüft, kann SPi/MPi aber nicht mit ausreichender Beweislage unterscheiden. Nennen Sie die ECU-Referenz oder Baujahr und Markt des Fahrzeugs.","Ich habe %1 anhand der ECU-/Firmware-Referenz %2 in der Datenbank identifiziert und setze die ursprüngliche Suche fort.","Ich habe den verfügbaren Kontext geprüft, aber die MEMS-Generation ist nicht bestimmt. Nennen Sie Baujahr oder ECU-Referenz oder verbinden Sie die ECU, damit ich ohne Raten fortfahren kann.","Die verbundene ECU meldet MEMS %1. Ich verwende daher diese Dokumentation.","MPi bedeutet „Multi Point Injection“, also Mehrpunkteinspritzung. Bei den betreffenden Rover/Mini-Systemen verteilen mehrere Einspritzventile den Kraftstoff, typischerweise eines pro Zylinder.","Bei einem Rover/Mini-MEMS-MPi-System erfolgt die Einspritzung mehrpunktig: mehrere Einspritzventile verteilen den Kraftstoff, typischerweise eines pro Zylinder. Die ECU steuert ihre Öffnung zur Kraftstoffdosierung im Ansaugtrakt. Wenn Sie Pinbelegung, Verkabelung, eine Prüfung oder ein bestimmtes Verfahren benötigen, nennen Sie diesen Punkt und ich suche die zugehörigen Daten."
]
}

for code, values in translations.items():
    if len(values) != len(keys):
        raise SystemExit(f"{code}: translation count {len(values)} != {len(keys)}")
    data = {str(key): value for key, value in zip(keys, values)}
    path = root / "translations" / code / f"{code}_ia_mems.json"
    path.write_text(json.dumps(data, ensure_ascii=False, indent=2) + "\n", encoding="utf-8", newline="\n")

# Existing x64 test workflow: exact allow-list + multilingual resource checks.
wf = ".github/workflows/tmp-ravemems-visual-test-package-x64.yml"
replace_once(
    wf,
    "          $allowed = @('expert/IaMemsDiagramCatalog.cpp','expert/IaMemsDiagramSelfTest.cpp','iamemstab.cpp')",
    "          $allowed = @('expert/IaMemsDiagramCatalog.cpp','expert/IaMemsDiagramSelfTest.cpp','expert/IaMemsService.cpp','expert/LocalAiClient.cpp','expert/LocalAiOnnxSelfTest.cpp','iamemstab.cpp','translations/fr/fr_ia_mems.json','translations/en/en_ia_mems.json','translations/es/es_ia_mems.json','translations/it/it_ia_mems.json','translations/pt/pt_ia_mems.json','translations/de/de_ia_mems.json')")
replace_once(
    wf,
    """          if ($source -notmatch 'm_diagramButton->setText\\(QStringLiteral\\(\"Voir le schéma\"\\)\\);') { throw 'Voir le schéma button label missing' }
          if ($source -match 'Ouvrir le schéma %1') { throw 'Old dynamic diagram button label still present' }
          Write-Host 'VIEW_BUTTON_LABEL_PASS Voir le schéma'""",
    """          if ($source -notmatch 'm_diagramButton->setText\\(I18n::text\\(99006\\)\\);') { throw 'Localized diagram button label missing' }
          if ($source -match 'm_diagramButton->setText\\(QStringLiteral\\(\"Voir le schéma\"\\)\\);') { throw 'Hard-coded French diagram button label still present' }
          Write-Host 'VIEW_BUTTON_LABEL_I18N_PASS key=99006'
          $env:TRANSLATION_ROOT='${{ github.workspace }}\\translations'
          @'
          import json,os
          from pathlib import Path
          root=Path(os.environ['TRANSLATION_ROOT'])
          required={str(k) for k in range(99001,99032)} | {str(k) for k in range(99033,99051)}
          for code in ('fr','en','es','it','pt','de'):
              p=root/code/f'{code}_ia_mems.json'
              data=json.load(open(p,encoding='utf-8'))
              missing=sorted(required-set(data))
              if missing: raise SystemExit(f'{code} IA translation keys missing: {missing}')
              if any(not str(data[k]).strip() for k in required): raise SystemExit(f'{code} IA translation contains empty value')
          print('IA_MULTILINGUAL_RESOURCES_PASS languages=fr,en,es,it,pt,de')
          '@ | python -""")

p = root / wf
p.write_text(p.read_text(encoding="utf-8").rstrip() + "\n\n# trigger IA multilingual direct source correction 2026-09-02\n", encoding="utf-8", newline="\n")

print("DIRECT_IA_MULTILINGUAL_SOURCE_EDIT_READY")
