#include "LocalAiClient.h"
#include "i18n.h"

#include <QCoreApplication>
#include <QDate>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QTimer>
#include <QUrl>

namespace {
const quint16 kAiPort = 18089;
const int kMaximumTurns = 8;

QString firstExisting(const QStringList &paths)
{
    for (const QString &path : paths) {
        if (QFileInfo::exists(path) && QFileInfo(path).isFile())
            return QDir::cleanPath(path);
    }
    return QString();
}

QString activeLanguageName(const QString &code)
{
    if (code == QStringLiteral("en")) return QStringLiteral("English");
    if (code == QStringLiteral("es")) return QStringLiteral("Español");
    if (code == QStringLiteral("it")) return QStringLiteral("Italiano");
    if (code == QStringLiteral("pt")) return QStringLiteral("Português");
    if (code == QStringLiteral("de")) return QStringLiteral("Deutsch");
    return QStringLiteral("Français");
}

QString normalizedPlainText(QString text)
{
    text = text.toLower().normalized(QString::NormalizationForm_D);
    QString result;
    result.reserve(text.size());
    for (const QChar ch : text) {
        const QChar::Category category = ch.category();
        if (category == QChar::Mark_NonSpacing
            || category == QChar::Mark_SpacingCombining
            || category == QChar::Mark_Enclosing)
            continue;
        if (ch.isLetterOrNumber())
            result.append(ch);
        else
            result.append(QLatin1Char(' '));
    }
    return result.simplified();
}

QString normalizedEchoText(QString text)
{
    const QString plain = normalizedPlainText(text);
    QString result;
    result.reserve(plain.size());
    for (const QChar ch : plain) {
        if (ch.isLetterOrNumber())
            result.append(ch);
    }
    return result;
}

bool isQuestionEcho(const QString &question, const QString &answer)
{
    const QString q = normalizedEchoText(question);
    const QString a = normalizedEchoText(answer);
    return !q.isEmpty() && q == a;
}

bool isGenericGrounding(const QString &grounding)
{
    return grounding.startsWith(QStringLiteral(
        "Je n'ai pas assez d'éléments pour relier cette question à une mesure ou à un fait MEMS précis."));
}

bool asksCurrentDate(const QString &question)
{
    const QString plain = normalizedPlainText(question);

    return plain.contains(QStringLiteral("quel jour"))
        || plain.contains(QStringLiteral("quelle date"))
        || plain.contains(QStringLiteral("date aujourd"))
        || plain.contains(QStringLiteral("jour sommes"))
        || plain.contains(QStringLiteral("date sommes"))
        || plain.contains(QStringLiteral("what day"))
        || plain.contains(QStringLiteral("what date"))
        || plain.contains(QStringLiteral("today s date"))
        || plain.contains(QStringLiteral("todays date"))
        || plain.contains(QStringLiteral("que dia"))
        || plain.contains(QStringLiteral("que fecha"))
        || plain.contains(QStringLiteral("che giorno"))
        || plain.contains(QStringLiteral("che data"))
        || plain.contains(QStringLiteral("welcher tag"))
        || plain.contains(QStringLiteral("welches datum"));
}

QString currentDateAnswer()
{
    const QDate today = QDate::currentDate();
    const QString code = I18n::language().trimmed().toLower();

    if (code == QStringLiteral("en")) {
        const QLocale locale(QLocale::English, QLocale::UnitedKingdom);
        return QStringLiteral("Today is %1.").arg(locale.toString(today, QStringLiteral("dddd d MMMM yyyy")));
    }
    if (code == QStringLiteral("es")) {
        const QLocale locale(QLocale::Spanish, QLocale::Spain);
        return QStringLiteral("Hoy es %1.").arg(locale.toString(today, QStringLiteral("dddd d 'de' MMMM 'de' yyyy")));
    }
    if (code == QStringLiteral("it")) {
        const QLocale locale(QLocale::Italian, QLocale::Italy);
        return QStringLiteral("Oggi è %1.").arg(locale.toString(today, QStringLiteral("dddd d MMMM yyyy")));
    }
    if (code == QStringLiteral("pt")) {
        const QLocale locale(QLocale::Portuguese, QLocale::Portugal);
        return QStringLiteral("Hoje é %1.").arg(locale.toString(today, QStringLiteral("dddd d 'de' MMMM 'de' yyyy")));
    }
    if (code == QStringLiteral("de")) {
        const QLocale locale(QLocale::German, QLocale::Germany);
        return QStringLiteral("Heute ist %1.").arg(locale.toString(today, QStringLiteral("dddd, d. MMMM yyyy")));
    }

    const QLocale locale(QLocale::French, QLocale::France);
    return QStringLiteral("Nous sommes le %1.").arg(locale.toString(today, QStringLiteral("dddd d MMMM yyyy")));
}

QString controlledTechnicalAnswer(const QString &question)
{
    const QString plain = normalizedPlainText(question);
    static const QRegularExpression iacRx(QStringLiteral("(^|\\s)iacv?(\\s|$)"));
    if (!iacRx.match(plain).hasMatch())
        return QString();

    const QString code = I18n::language().trimmed().toLower();
    if (code == QStringLiteral("en"))
        return QStringLiteral("IAC means Idle Air Control. It is the idle-air regulation system used by the ECU to adjust the air needed to stabilise idle speed. Depending on the MEMS installation, the physical idle actuator can differ.");
    if (code == QStringLiteral("es"))
        return QStringLiteral("IAC significa Idle Air Control: es el sistema de regulación del aire de ralentí que utiliza la ECU para estabilizar el régimen. El actuador físico puede variar según la instalación MEMS.");
    if (code == QStringLiteral("it"))
        return QStringLiteral("IAC significa Idle Air Control: è il sistema di regolazione dell'aria al minimo usato dalla ECU per stabilizzare il regime. L'attuatore fisico può variare secondo l'installazione MEMS.");
    if (code == QStringLiteral("pt"))
        return QStringLiteral("IAC significa Idle Air Control: é o sistema de controlo do ar de ralenti usado pela ECU para estabilizar a rotação. O atuador físico pode variar consoante a instalação MEMS.");
    if (code == QStringLiteral("de"))
        return QStringLiteral("IAC bedeutet Idle Air Control. Damit regelt das ECU-System die Leerlaufluft, um die Leerlaufdrehzahl zu stabilisieren. Der konkrete Leerlaufsteller kann je nach MEMS-Ausführung unterschiedlich sein.");

    return QStringLiteral("IAC signifie « Idle Air Control » : c'est le système de régulation de l'air de ralenti utilisé par l'ECU pour stabiliser le régime. L'actionneur physique de ralenti peut varier selon le montage MEMS.");
}

bool isFollowUpQuestion(const QString &question)
{
    const QString plain = normalizedPlainText(question);
    return plain.startsWith(QStringLiteral("et "))
        || plain.startsWith(QStringLiteral("et si "))
        || plain.startsWith(QStringLiteral("et pour "))
        || plain.startsWith(QStringLiteral("et pourquoi "))
        || plain.startsWith(QStringLiteral("pourquoi ca"))
        || plain.startsWith(QStringLiteral("pourquoi cela"))
        || plain.startsWith(QStringLiteral("comment ca"))
        || plain.startsWith(QStringLiteral("et ca"))
        || plain.startsWith(QStringLiteral("et cela"))
        || plain.startsWith(QStringLiteral("dans ce cas"))
        || plain.startsWith(QStringLiteral("tu peux preciser"))
        || plain.startsWith(QStringLiteral("peux tu preciser"))
        || plain.startsWith(QStringLiteral("peux tu detailler"))
        || plain.startsWith(QStringLiteral("plus de details"))
        || plain.startsWith(QStringLiteral("cette valeur"))
        || plain.startsWith(QStringLiteral("ce resultat"))
        || plain.startsWith(QStringLiteral("ce moteur"))
        || plain.startsWith(QStringLiteral("cet ecu"));
}

bool requiresReasoning(const QString &question, const QString &grounding)
{
    const QString plain = normalizedPlainText(question);
    const QString ground = normalizedPlainText(grounding);

    if (plain.contains(QStringLiteral("diagnostic"))
        || plain.contains(QStringLiteral("diagnostique"))
        || plain.contains(QStringLiteral("analyse"))
        || plain.contains(QStringLiteral("analyser"))
        || plain.contains(QStringLiteral("anormal"))
        || plain.contains(QStringLiteral("panne"))
        || plain.contains(QStringLiteral("probleme"))
        || plain.contains(QStringLiteral("hypothese"))
        || plain.contains(QStringLiteral("oscill"))
        || plain.contains(QStringLiteral("instable")))
        return true;

    if (plain.contains(QStringLiteral("pourquoi"))) {
        static const QStringList ecuTerms = {
            QStringLiteral("ralenti"), QStringLiteral("ecu"), QStringLiteral("mems"),
            QStringLiteral("lambda"), QStringLiteral("injection"), QStringLiteral("avance"),
            QStringLiteral("bobine"), QStringLiteral("capteur"), QStringLiteral("map"),
            QStringLiteral("temperature"), QStringLiteral("batterie"), QStringLiteral("regime"),
            QStringLiteral("moteur")
        };
        for (const QString &term : ecuTerms) {
            if (plain.contains(term))
                return true;
        }
    }

    return ground.contains(QStringLiteral("hypotheses actuelles"))
        || ground.contains(QStringLiteral("confiance"))
        || ground.contains(QStringLiteral("preuve"));
}

void rememberTurn(QVector<QPair<QString, QString>> &conversation,
                  const QString &question,
                  const QString &answer)
{
    conversation.append(qMakePair(question.trimmed(), answer.trimmed()));
    if (conversation.size() > kMaximumTurns)
        conversation.remove(0, conversation.size() - kMaximumTurns);
}
}

LocalAiClient::LocalAiClient(QObject *parent)
    : QObject(parent),
      m_network(new QNetworkAccessManager(this)),
      m_server(new QProcess(this)),
      m_healthTimer(new QTimer(this))
{
    m_healthTimer->setSingleShot(true);
    connect(m_healthTimer, &QTimer::timeout, this, &LocalAiClient::checkHealth);

    connect(m_server,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this,
            [this](int exitCode, QProcess::ExitStatus exitStatus) {
                if (m_startedServer && m_state != NotStarted && m_state != MissingModel
                    && m_state != MissingRuntime) {
                    const quint32 nativeCode = static_cast<quint32>(exitCode);
                    QString detail = QStringLiteral("llama-server s'est arrêté (code %1 / 0x%2, statut %3).")
                        .arg(exitCode)
                        .arg(QString::number(nativeCode, 16).toUpper())
                        .arg(exitStatus == QProcess::NormalExit ? QStringLiteral("normal")
                                                               : QStringLiteral("crash"));
                    const QString output = QString::fromUtf8(m_server->readAll()).trimmed();
                    if (!output.isEmpty())
                        detail += QStringLiteral(" ") + output.right(1200);
                    setState(Error, detail);
                }
            });

    connect(m_server, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (!m_startedServer || m_state == NotStarted)
            return;
        QString detail = QStringLiteral("Impossible de démarrer llama-server (erreur QProcess %1) : %2")
            .arg(static_cast<int>(error))
            .arg(m_server->errorString());
        const QString output = QString::fromUtf8(m_server->readAll()).trimmed();
        if (!output.isEmpty())
            detail += QStringLiteral(" ") + output.right(1200);
        setState(Error, detail);
    });
}

LocalAiClient::~LocalAiClient()
{
    shutdown();
}

void LocalAiClient::initialize()
{
    if (m_state == Starting || m_state == Ready || m_state == Busy)
        return;

    discoverAssets();

    // Reuse a server that is already listening on the dedicated loopback port.
    m_healthAttempts = 0;
    setState(Starting);
    checkHealth();
}

void LocalAiClient::discoverAssets()
{
    const QString root = QCoreApplication::applicationDirPath();
    const QProcessEnvironment env = QProcessEnvironment::systemEnvironment();

    const QString envRuntime = env.value(QStringLiteral("MEMS_AI_SERVER"));
    const QString envModel = env.value(QStringLiteral("MEMS_AI_MODEL"));

    m_runtimePath = firstExisting({
        envRuntime,
        QDir(root).filePath(QStringLiteral("ai/llama-server.exe")),
        QDir(root).filePath(QStringLiteral("ai/runtime/llama-server.exe")),
        QDir(root).filePath(QStringLiteral("llama-server.exe"))
    });

    m_modelPath = firstExisting({
        envModel,
        QDir(root).filePath(QStringLiteral("ai/models/ia-mems.gguf")),
        QDir(root).filePath(QStringLiteral("ai/ia-mems.gguf")),
        QDir(root).filePath(QStringLiteral("models/ia-mems.gguf"))
    });
}

void LocalAiClient::startServer()
{
    if (m_runtimePath.isEmpty()) {
        setState(MissingRuntime,
                 QStringLiteral("Moteur llama.cpp local absent du dossier IA."));
        return;
    }
    if (m_modelPath.isEmpty()) {
        setState(MissingModel,
                 QStringLiteral("Modèle GGUF IA MEMS absent du dossier IA."));
        return;
    }

    if (m_server->state() != QProcess::NotRunning)
        return;

    QStringList args;
    args << QStringLiteral("-m") << m_modelPath
         << QStringLiteral("--alias") << QStringLiteral("ia-mems")
         << QStringLiteral("--host") << QStringLiteral("127.0.0.1")
         << QStringLiteral("--port") << QString::number(kAiPort)
         << QStringLiteral("-c") << QStringLiteral("4096")
         << QStringLiteral("-np") << QStringLiteral("1")
         << QStringLiteral("--no-webui")
         << QStringLiteral("--offline");

    const QString runtimeDirectory = QFileInfo(m_runtimePath).absolutePath();
    m_server->setProgram(m_runtimePath);
    m_server->setArguments(args);
    m_server->setWorkingDirectory(runtimeDirectory);
    QProcessEnvironment processEnvironment = QProcessEnvironment::systemEnvironment();
    const QString currentPath = processEnvironment.value(QStringLiteral("PATH"));
    processEnvironment.insert(QStringLiteral("PATH"),
                              runtimeDirectory + QDir::listSeparator() + currentPath);
    m_server->setProcessEnvironment(processEnvironment);
    m_server->setProcessChannelMode(QProcess::MergedChannels);
    m_startedServer = true;
    m_server->start();
    m_healthAttempts = 0;
    scheduleHealthCheck();
}

void LocalAiClient::scheduleHealthCheck()
{
    if (m_healthAttempts >= 150) {
        setState(Error,
                 QStringLiteral("Le modèle local n'a pas terminé son démarrage."));
        return;
    }
    m_healthTimer->start(600);
}

void LocalAiClient::checkHealth()
{
    ++m_healthAttempts;
    QNetworkRequest request(QUrl(QStringLiteral("http://127.0.0.1:%1/health").arg(kAiPort)));
    QNetworkReply *reply = m_network->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        handleHealthReply(reply);
        reply->deleteLater();
    });
}

void LocalAiClient::handleHealthReply(QNetworkReply *reply)
{
    const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QByteArray body = reply->readAll();
    const QJsonDocument document = QJsonDocument::fromJson(body);

    if (httpStatus == 200 && document.isObject()
        && document.object().value(QStringLiteral("status")).toString() == QStringLiteral("ok")) {
        setState(Ready);
        return;
    }

    // llama.cpp returns HTTP 503 while the model is loading. If a server already
    // owns the loopback port, keep waiting rather than starting a competing one.
    if (httpStatus == 503) {
        scheduleHealthCheck();
        return;
    }

    if (!m_startedServer) {
        startServer();
        return;
    }

    if (m_server->state() == QProcess::NotRunning) {
        const QString output = QString::fromUtf8(m_server->readAll()).trimmed();
        setState(Error,
                 output.isEmpty()
                     ? QStringLiteral("Impossible de démarrer le moteur d'IA locale.")
                     : output.right(900));
        return;
    }

    scheduleHealthCheck();
}

void LocalAiClient::ask(const QString &question, const QString &groundingContext)
{
    if (m_state != Ready) {
        emit responseError(QStringLiteral("L'IA conversationnelle locale n'est pas prête."));
        return;
    }

    const QString trimmedQuestion = question.trimmed();
    if (trimmedQuestion.isEmpty())
        return;

    // Runtime facts and controlled technical definitions must not pay the cost
    // of an LLM generation and must never be guessed by the model.
    if (asksCurrentDate(trimmedQuestion)) {
        const QString answer = currentDateAnswer();
        rememberTurn(m_conversation, trimmedQuestion, answer);
        emit responseReady(answer);
        return;
    }

    const QString controlled = controlledTechnicalAnswer(trimmedQuestion);
    if (!controlled.isEmpty()) {
        rememberTurn(m_conversation, trimmedQuestion, controlled);
        emit responseReady(controlled);
        return;
    }

    QString grounding = groundingContext.trimmed();
    if (isGenericGrounding(grounding))
        grounding.clear();

    const bool reasoning = requiresReasoning(trimmedQuestion, grounding);

    // The deterministic engine/database already owns many exact answers. For a
    // simple lookup, software question or decoded measurement, return that fact
    // immediately instead of asking a 0.6B model to paraphrase or corrupt it.
    if (!reasoning && !grounding.isEmpty()) {
        rememberTurn(m_conversation, trimmedQuestion, grounding);
        emit responseReady(grounding);
        return;
    }

    setState(Busy);

    QJsonArray messages;
    QJsonObject system;
    system.insert(QStringLiteral("role"), QStringLiteral("system"));
    system.insert(QStringLiteral("content"), systemPrompt());
    messages.append(system);

    // Independent questions start with a clean conversational context. Only a
    // clearly dependent follow-up receives the immediately previous turn.
    if (isFollowUpQuestion(trimmedQuestion) && !m_conversation.isEmpty()) {
        const QPair<QString, QString> &turn = m_conversation.constLast();
        QJsonObject user;
        user.insert(QStringLiteral("role"), QStringLiteral("user"));
        user.insert(QStringLiteral("content"), turn.first);
        messages.append(user);

        QJsonObject assistant;
        assistant.insert(QStringLiteral("role"), QStringLiteral("assistant"));
        assistant.insert(QStringLiteral("content"), turn.second);
        messages.append(assistant);
    }

    QString userContent = trimmedQuestion;
    if (!grounding.isEmpty()) {
        userContent += QStringLiteral(
            "\n\nCONTEXTE CANDIDAT FOURNI PAR MEMS MANAGER :\n%1\n\n"
            "Réponds d'abord à la question exacte de l'utilisateur. "
            "N'utilise que les éléments de ce contexte qui répondent directement à cette question et ignore les éléments hors sujet, même s'ils sont vrais. "
            "Pour les faits techniques MEMS ou les mesures ECU, ce contexte pertinent est prioritaire sur tes connaissances générales. "
            "Ne cite pas un auteur, un site, un dépôt ou une source seulement parce que son nom apparaît dans le contexte : cite-le uniquement si l'utilisateur demande la source, l'historique ou si ce nom est indispensable à la réponse. "
            "N'invente pas de mesure, de panne, de fonction du logiciel ni de niveau de certitude.")
                           .arg(grounding);
    }

    // Qwen3 keeps its reasoning capability available, but only complex ECU
    // analysis pays for it. Simple questions use the documented soft switch.
    userContent += reasoning ? QStringLiteral("\n\n/think")
                             : QStringLiteral("\n\n/no_think");

    QJsonObject currentUser;
    currentUser.insert(QStringLiteral("role"), QStringLiteral("user"));
    currentUser.insert(QStringLiteral("content"), userContent);
    messages.append(currentUser);

    QJsonObject payload;
    payload.insert(QStringLiteral("model"), QStringLiteral("ia-mems"));
    payload.insert(QStringLiteral("messages"), messages);
    payload.insert(QStringLiteral("stream"), false);
    payload.insert(QStringLiteral("top_k"), 20);
    payload.insert(QStringLiteral("min_p"), 0.0);

    if (reasoning) {
        // Qwen3 thinking-mode sampling. The budget is deliberately bounded for
        // an interactive CPU application while preserving real reasoning.
        payload.insert(QStringLiteral("temperature"), 0.6);
        payload.insert(QStringLiteral("top_p"), 0.95);
        payload.insert(QStringLiteral("presence_penalty"), 0.3);
        payload.insert(QStringLiteral("max_tokens"), 768);
    } else {
        // Qwen3 non-thinking settings recommended for direct, fast answers.
        payload.insert(QStringLiteral("temperature"), 0.7);
        payload.insert(QStringLiteral("top_p"), 0.8);
        payload.insert(QStringLiteral("presence_penalty"), 0.2);
        payload.insert(QStringLiteral("max_tokens"), 256);
    }

    QNetworkRequest request(
        QUrl(QStringLiteral("http://127.0.0.1:%1/v1/chat/completions").arg(kAiPort)));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));

    QNetworkReply *reply = m_network->post(request, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply, trimmedQuestion, grounding]() {
        if (reply->error() != QNetworkReply::NoError) {
            const QString error = reply->errorString();
            reply->deleteLater();
            setState(Ready);
            emit responseError(QStringLiteral("Erreur du moteur d'IA locale : %1").arg(error));
            return;
        }

        const QJsonDocument document = QJsonDocument::fromJson(reply->readAll());
        reply->deleteLater();
        QString answer;
        if (document.isObject()) {
            const QJsonArray choices = document.object().value(QStringLiteral("choices")).toArray();
            if (!choices.isEmpty()) {
                answer = choices.first().toObject()
                             .value(QStringLiteral("message")).toObject()
                             .value(QStringLiteral("content")).toString();
            }
        }

        answer = cleanModelReply(answer);
        setState(Ready);

        // If the model only echoes the question or exhausts its reasoning budget,
        // a reliable MEMS Manager fact wins. Otherwise fail explicitly.
        if (answer.isEmpty() || isQuestionEcho(trimmedQuestion, answer)) {
            if (!grounding.isEmpty())
                answer = grounding;
            else
                answer.clear();
        }

        if (answer.isEmpty()) {
            emit responseError(QStringLiteral(
                "Le modèle local n'a pas produit de réponse exploitable."));
            return;
        }

        rememberTurn(m_conversation, trimmedQuestion, answer);
        emit responseReady(answer);
    });
}

void LocalAiClient::clearConversation()
{
    m_conversation.clear();
}

void LocalAiClient::shutdown()
{
    if (m_healthTimer)
        m_healthTimer->stop();

    // Prevent a deliberate shutdown from being reported as an engine failure.
    m_state = NotStarted;
    if (m_startedServer && m_server && m_server->state() != QProcess::NotRunning) {
        m_server->terminate();
        if (!m_server->waitForFinished(1200)) {
            m_server->kill();
            m_server->waitForFinished(500);
        }
    }
    m_startedServer = false;
}

void LocalAiClient::setState(State state, const QString &error)
{
    if (!error.isEmpty())
        m_lastError = error;
    else if (state == Ready || state == Starting)
        m_lastError.clear();

    if (m_state == state && error.isEmpty())
        return;
    m_state = state;
    emit stateChanged();
}

QString LocalAiClient::statusText() const
{
    switch (m_state) {
    case NotStarted: return QStringLiteral("IA locale non démarrée");
    case MissingRuntime: return QStringLiteral("moteur IA local absent");
    case MissingModel: return QStringLiteral("modèle IA local absent");
    case Starting: return QStringLiteral("IA locale en démarrage");
    case Ready: return QStringLiteral("IA locale prête");
    case Busy: return QStringLiteral("IA locale en réponse");
    case Error:
        if (m_lastError.isEmpty())
            return QStringLiteral("erreur IA locale");
        return QStringLiteral("erreur IA locale : %1")
            .arg(m_lastError.simplified().left(140));
    }
    return QStringLiteral("IA locale");
}

QString LocalAiClient::systemPrompt() const
{
    QString languageCode = I18n::language().trimmed().toLower();
    const QStringList supported = {QStringLiteral("fr"), QStringLiteral("en"), QStringLiteral("es"),
                                   QStringLiteral("it"), QStringLiteral("pt"), QStringLiteral("de")};
    if (!supported.contains(languageCode))
        languageCode = QStringLiteral("fr");
    const QString languageName = activeLanguageName(languageCode);
    const QString runtimeDate = QDate::currentDate().toString(Qt::ISODate);

    return QStringLiteral(
        "You are IA MEMS, the conversational assistant integrated into ECU MEMS Manager. "
        "The active MEMS Manager interface language is %1 (%2). Answer in that language by default. "
        "The local runtime date of this computer is %3. If the user asks for today's date or day, use this date and do not guess another one. "
        "If the user explicitly asks for another language, follow that request. "
        "Understand spelling and typing mistakes when the meaning is clear; do not comment on them unnecessarily. "
        "Answer the exact question first and do not drift to a neighbouring topic. "
        "Never answer by merely repeating, correcting, translating or reformulating the user's question. "
        "For ordinary conversation, answer naturally without forcing a link to the ECU. "
        "For software questions, explain the requested function directly before technical detail. "
        "For MEMS technical questions, relevant facts supplied by MEMS Manager take priority over general knowledge. "
        "For automotive or MEMS acronyms, never invent an expansion. If no reliable expansion is supplied and you are uncertain, say so. "
        "The supplied context may contain several facts: strictly ignore facts that do not answer the current question. "
        "Never invent an ECU measurement, fault, protocol address, software function, source or confidence level. "
        "Clearly distinguish observed measurements, hypotheses, external information and uncertain information. "
        "Never condemn a component from one measurement alone. "
        "When technical data is insufficient or contradictory, say that you cannot conclude and briefly state what is missing. "
        "If ambiguity would materially change the answer, ask one useful clarification rather than guessing. "
        "Keep ECU numbers, units and factual values unchanged when changing language. "
        "Be clear, natural and concise unless the user asks for detail.")
        .arg(languageName, languageCode, runtimeDate);
}

QString LocalAiClient::cleanModelReply(QString text) const
{
    text.remove(QRegularExpression(QStringLiteral("<think>.*?</think>"),
                                   QRegularExpression::DotMatchesEverythingOption));
    text.replace(QStringLiteral("/no_think"), QString());
    text.replace(QStringLiteral("/think"), QString());
    return text.trimmed();
}
