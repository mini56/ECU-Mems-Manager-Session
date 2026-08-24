#include "LocalAiClient.h"
#include "diagnosticlogger.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
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

QString stateName(LocalAiClient::State state)
{
    switch (state) {
    case LocalAiClient::NotStarted: return QStringLiteral("NotStarted");
    case LocalAiClient::MissingRuntime: return QStringLiteral("MissingRuntime");
    case LocalAiClient::MissingModel: return QStringLiteral("MissingModel");
    case LocalAiClient::Starting: return QStringLiteral("Starting");
    case LocalAiClient::Ready: return QStringLiteral("Ready");
    case LocalAiClient::Busy: return QStringLiteral("Busy");
    case LocalAiClient::Error: return QStringLiteral("Error");
    }
    return QStringLiteral("Unknown");
}

QString candidateInfo(const QString &label, const QString &path)
{
    const QFileInfo info(path);
    return QStringLiteral("AI_CANDIDATE: %1=%2 | exists=%3 | file=%4 | size=%5")
        .arg(label,
             path.isEmpty() ? QStringLiteral("<empty>") : QDir::cleanPath(path),
             info.exists() ? QStringLiteral("yes") : QStringLiteral("no"),
             info.isFile() ? QStringLiteral("yes") : QStringLiteral("no"))
        .arg(info.exists() ? info.size() : 0);
}
}

LocalAiClient::LocalAiClient(QObject *parent)
    : QObject(parent),
      m_network(new QNetworkAccessManager(this)),
      m_server(new QProcess(this)),
      m_healthTimer(new QTimer(this))
{
    DiagnosticLogger::checkpoint(QStringLiteral("LocalAiClient constructed"));
    m_healthTimer->setSingleShot(true);
    connect(m_healthTimer, &QTimer::timeout, this, &LocalAiClient::checkHealth);

    connect(m_server, &QProcess::started, this, [this]() {
        DiagnosticLogger::log(QStringLiteral("AI_PROCESS_STARTED: pid=%1 program=%2 workingDirectory=%3")
                                  .arg(m_server->processId())
                                  .arg(m_server->program(), m_server->workingDirectory()));
    });

    connect(m_server,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this,
            [this](int exitCode, QProcess::ExitStatus exitStatus) {
                const QString output = QString::fromUtf8(m_server->readAll()).trimmed();
                DiagnosticLogger::log(QStringLiteral("AI_PROCESS_FINISHED: pid=%1 exitCode=%2 hex=0x%3 exitStatus=%4 state=%5 output=%6")
                                          .arg(m_server->processId())
                                          .arg(exitCode)
                                          .arg(QString::number(static_cast<quint32>(exitCode), 16).toUpper())
                                          .arg(exitStatus == QProcess::NormalExit ? QStringLiteral("normal") : QStringLiteral("crash"),
                                               stateName(m_state),
                                               output.isEmpty() ? QStringLiteral("<empty>") : output.right(1600)));
                if (m_startedServer && m_state != NotStarted && m_state != MissingModel
                    && m_state != MissingRuntime) {
                    const quint32 nativeCode = static_cast<quint32>(exitCode);
                    QString detail = QStringLiteral("llama-server s'est arrêté (code %1 / 0x%2, statut %3).")
                        .arg(exitCode)
                        .arg(QString::number(nativeCode, 16).toUpper())
                        .arg(exitStatus == QProcess::NormalExit ? QStringLiteral("normal")
                                                               : QStringLiteral("crash"));
                    if (!output.isEmpty())
                        detail += QStringLiteral(" ") + output.right(1200);
                    setState(Error, detail);
                }
            });

    connect(m_server, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        const QString output = QString::fromUtf8(m_server->readAll()).trimmed();
        DiagnosticLogger::log(QStringLiteral("AI_PROCESS_ERROR: error=%1 errorString=%2 state=%3 output=%4")
                                  .arg(static_cast<int>(error))
                                  .arg(m_server->errorString(), stateName(m_state),
                                       output.isEmpty() ? QStringLiteral("<empty>") : output.right(1600)));
        if (!m_startedServer || m_state == NotStarted)
            return;
        QString detail = QStringLiteral("Impossible de démarrer llama-server (erreur QProcess %1) : %2")
            .arg(static_cast<int>(error))
            .arg(m_server->errorString());
        if (!output.isEmpty())
            detail += QStringLiteral(" ") + output.right(1200);
        setState(Error, detail);
    });
}

LocalAiClient::~LocalAiClient()
{
    DiagnosticLogger::checkpoint(QStringLiteral("LocalAiClient destructor entered"));
    shutdown();
    DiagnosticLogger::checkpoint(QStringLiteral("LocalAiClient destructor completed"));
}

void LocalAiClient::initialize()
{
    DiagnosticLogger::log(QStringLiteral("AI_INITIALIZE: currentState=%1 startedServer=%2")
                              .arg(stateName(m_state))
                              .arg(m_startedServer ? QStringLiteral("yes") : QStringLiteral("no")));
    if (m_state == Starting || m_state == Ready || m_state == Busy) {
        DiagnosticLogger::log(QStringLiteral("AI_INITIALIZE: ignored because state=%1").arg(stateName(m_state)));
        return;
    }

    discoverAssets();

    // Reuse a server that is already listening on the dedicated loopback port.
    m_healthAttempts = 0;
    setState(Starting);
    DiagnosticLogger::checkpoint(QStringLiteral("AI initial health probe about to start on 127.0.0.1:18089"));
    checkHealth();
}

void LocalAiClient::discoverAssets()
{
    const QString root = QCoreApplication::applicationDirPath();
    const QProcessEnvironment env = QProcessEnvironment::systemEnvironment();

    const QString envRuntime = env.value(QStringLiteral("MEMS_AI_SERVER"));
    const QString envModel = env.value(QStringLiteral("MEMS_AI_MODEL"));
    const QString appRuntime = QDir(root).filePath(QStringLiteral("ai/llama-server.exe"));
    const QString nestedRuntime = QDir(root).filePath(QStringLiteral("ai/runtime/llama-server.exe"));
    const QString rootRuntime = QDir(root).filePath(QStringLiteral("llama-server.exe"));
    const QString appModel = QDir(root).filePath(QStringLiteral("ai/models/ia-mems.gguf"));
    const QString flatModel = QDir(root).filePath(QStringLiteral("ai/ia-mems.gguf"));
    const QString rootModel = QDir(root).filePath(QStringLiteral("models/ia-mems.gguf"));

    DiagnosticLogger::log(QStringLiteral("AI_DISCOVERY_ROOT: %1").arg(root));
    DiagnosticLogger::log(candidateInfo(QStringLiteral("ENV_MEMS_AI_SERVER"), envRuntime));
    DiagnosticLogger::log(candidateInfo(QStringLiteral("APP_ai_llama-server"), appRuntime));
    DiagnosticLogger::log(candidateInfo(QStringLiteral("APP_ai_runtime_llama-server"), nestedRuntime));
    DiagnosticLogger::log(candidateInfo(QStringLiteral("APP_root_llama-server"), rootRuntime));
    DiagnosticLogger::log(candidateInfo(QStringLiteral("ENV_MEMS_AI_MODEL"), envModel));
    DiagnosticLogger::log(candidateInfo(QStringLiteral("APP_ai_models_ia-mems"), appModel));
    DiagnosticLogger::log(candidateInfo(QStringLiteral("APP_ai_ia-mems"), flatModel));
    DiagnosticLogger::log(candidateInfo(QStringLiteral("APP_models_ia-mems"), rootModel));

    m_runtimePath = firstExisting({
        envRuntime,
        appRuntime,
        nestedRuntime,
        rootRuntime
    });

    m_modelPath = firstExisting({
        envModel,
        appModel,
        flatModel,
        rootModel
    });

    DiagnosticLogger::log(QStringLiteral("AI_DISCOVERY_SELECTED: runtime=%1 model=%2")
                              .arg(m_runtimePath.isEmpty() ? QStringLiteral("<none>") : m_runtimePath,
                                   m_modelPath.isEmpty() ? QStringLiteral("<none>") : m_modelPath));
}

void LocalAiClient::startServer()
{
    DiagnosticLogger::log(QStringLiteral("AI_START_SERVER: runtime=%1 model=%2 processState=%3")
                              .arg(m_runtimePath.isEmpty() ? QStringLiteral("<none>") : m_runtimePath,
                                   m_modelPath.isEmpty() ? QStringLiteral("<none>") : m_modelPath)
                              .arg(static_cast<int>(m_server->state())));
    if (m_runtimePath.isEmpty()) {
        DiagnosticLogger::checkpoint(QStringLiteral("AI start aborted: runtime missing"));
        setState(MissingRuntime,
                 QStringLiteral("Moteur llama.cpp local absent du dossier IA."));
        return;
    }
    if (m_modelPath.isEmpty()) {
        DiagnosticLogger::checkpoint(QStringLiteral("AI start aborted: model missing"));
        setState(MissingModel,
                 QStringLiteral("Modèle GGUF IA MEMS absent du dossier IA."));
        return;
    }

    if (m_server->state() != QProcess::NotRunning) {
        DiagnosticLogger::checkpoint(QStringLiteral("AI start ignored: QProcess already running"));
        return;
    }

    QStringList args;
    args << QStringLiteral("-m") << m_modelPath
         << QStringLiteral("--alias") << QStringLiteral("ia-mems")
         << QStringLiteral("--host") << QStringLiteral("127.0.0.1")
         << QStringLiteral("--port") << QString::number(kAiPort)
         << QStringLiteral("-c") << QStringLiteral("4096")
         << QStringLiteral("-np") << QStringLiteral("1");

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
    DiagnosticLogger::log(QStringLiteral("AI_PROCESS_START_REQUEST: program=%1 workingDirectory=%2 args=%3")
                              .arg(m_runtimePath, runtimeDirectory, args.join(QLatin1Char(' '))));
    m_server->start();
    m_healthAttempts = 0;
    scheduleHealthCheck();
}

void LocalAiClient::scheduleHealthCheck()
{
    if (m_healthAttempts >= 150) {
        DiagnosticLogger::checkpoint(QStringLiteral("AI health timeout reached after 150 attempts"));
        setState(Error,
                 QStringLiteral("Le modèle local n'a pas terminé son démarrage."));
        return;
    }
    m_healthTimer->start(600);
}

void LocalAiClient::checkHealth()
{
    ++m_healthAttempts;
    if (m_healthAttempts == 1 || (m_healthAttempts % 10) == 0)
        DiagnosticLogger::log(QStringLiteral("AI_HEALTH_REQUEST: attempt=%1 startedServer=%2 processState=%3")
                                  .arg(m_healthAttempts)
                                  .arg(m_startedServer ? QStringLiteral("yes") : QStringLiteral("no"))
                                  .arg(static_cast<int>(m_server->state())));
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
    const QNetworkReply::NetworkError networkError = reply->error();
    const QString networkErrorText = reply->errorString();
    const QByteArray body = reply->readAll();
    const QJsonDocument document = QJsonDocument::fromJson(body);

    if (m_healthAttempts == 1 || httpStatus != 0 || networkError != QNetworkReply::ConnectionRefusedError
        || (m_healthAttempts % 10) == 0) {
        DiagnosticLogger::log(QStringLiteral("AI_HEALTH_REPLY: attempt=%1 http=%2 networkError=%3 errorString=%4 startedServer=%5 processState=%6 body=%7")
                                  .arg(m_healthAttempts)
                                  .arg(httpStatus)
                                  .arg(static_cast<int>(networkError))
                                  .arg(networkErrorText)
                                  .arg(m_startedServer ? QStringLiteral("yes") : QStringLiteral("no"))
                                  .arg(static_cast<int>(m_server->state()))
                                  .arg(body.isEmpty() ? QStringLiteral("<empty>") : QString::fromUtf8(body).simplified().left(600)));
    }

    if (httpStatus == 200 && document.isObject()
        && document.object().value(QStringLiteral("status")).toString() == QStringLiteral("ok")) {
        DiagnosticLogger::checkpoint(m_startedServer
            ? QStringLiteral("AI health OK from server started by MEMS Manager")
            : QStringLiteral("AI health OK from pre-existing server on port 18089"));
        setState(Ready);
        return;
    }

    // llama.cpp deliberately returns HTTP 503 while a model is loading. If a
    // server already owns our loopback port, simply keep waiting instead of
    // trying to spawn a competing process.
    if (httpStatus == 503) {
        if (!m_startedServer && m_healthAttempts == 1)
            DiagnosticLogger::checkpoint(QStringLiteral("Pre-existing process on port 18089 returned HTTP 503; MEMS Manager will wait and will not spawn a second server"));
        scheduleHealthCheck();
        return;
    }

    if (!m_startedServer) {
        DiagnosticLogger::checkpoint(QStringLiteral("No healthy pre-existing AI server detected; invoking startServer"));
        startServer();
        return;
    }

    if (m_server->state() == QProcess::NotRunning) {
        const QString output = QString::fromUtf8(m_server->readAll()).trimmed();
        DiagnosticLogger::log(QStringLiteral("AI server is NotRunning during health probe; output=%1")
                                  .arg(output.isEmpty() ? QStringLiteral("<empty>") : output.right(1600)));
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
    DiagnosticLogger::log(QStringLiteral("AI_ASK: state=%1 questionLength=%2 groundingLength=%3")
                              .arg(stateName(m_state))
                              .arg(question.size())
                              .arg(groundingContext.size()));
    if (m_state != Ready) {
        DiagnosticLogger::checkpoint(QStringLiteral("AI_ASK rejected because local model is not Ready"));
        emit responseError(QStringLiteral("L'IA conversationnelle locale n'est pas prête."));
        return;
    }

    setState(Busy);

    QJsonArray messages;
    QJsonObject system;
    system.insert(QStringLiteral("role"), QStringLiteral("system"));
    system.insert(QStringLiteral("content"), systemPrompt());
    messages.append(system);

    const int first = qMax(0, m_conversation.size() - kMaximumTurns);
    for (int i = first; i < m_conversation.size(); ++i) {
        QJsonObject user;
        user.insert(QStringLiteral("role"), QStringLiteral("user"));
        user.insert(QStringLiteral("content"), m_conversation.at(i).first);
        messages.append(user);

        QJsonObject assistant;
        assistant.insert(QStringLiteral("role"), QStringLiteral("assistant"));
        assistant.insert(QStringLiteral("content"), m_conversation.at(i).second);
        messages.append(assistant);
    }

    QString userContent = question.trimmed();
    userContent += QStringLiteral("\n\n/no_think\n\n");

    QString grounding = groundingContext.trimmed();
    // The deterministic router has a deliberately conservative generic fallback.
    // It is useful when the local model is unavailable, but it must not force a
    // working conversational model to reject ordinary dialogue or a question it
    // can answer naturally.
    if (grounding.startsWith(QStringLiteral(
            "Je n'ai pas assez d'éléments pour relier cette question à une mesure ou à un fait MEMS précis.")))
        grounding.clear();

    if (!grounding.isEmpty()) {
        userContent += QStringLiteral(
            "CONTEXTE CANDIDAT FOURNI PAR MEMS MANAGER :\n%1\n\n"
            "Réponds d'abord à la question exacte de l'utilisateur. "
            "N'utilise que les éléments de ce contexte qui répondent directement à cette question et ignore les éléments hors sujet, même s'ils sont vrais. "
            "Pour les faits techniques MEMS ou les mesures ECU, ce contexte pertinent est prioritaire sur tes connaissances générales. "
            "Ne cite pas un auteur, un site, un dépôt ou une source seulement parce que son nom apparaît dans le contexte : cite-le uniquement si l'utilisateur demande la source, l'historique ou si ce nom est indispensable à la réponse. "
            "N'invente pas de mesure, de panne, de fonction du logiciel ni de niveau de certitude.")
                           .arg(grounding);
    }

    QJsonObject currentUser;
    currentUser.insert(QStringLiteral("role"), QStringLiteral("user"));
    currentUser.insert(QStringLiteral("content"), userContent);
    messages.append(currentUser);

    QJsonObject payload;
    payload.insert(QStringLiteral("model"), QStringLiteral("ia-mems"));
    payload.insert(QStringLiteral("messages"), messages);
    payload.insert(QStringLiteral("stream"), false);
    payload.insert(QStringLiteral("temperature"), 0.25);
    payload.insert(QStringLiteral("top_p"), 0.9);
    payload.insert(QStringLiteral("max_tokens"), 460);

    QNetworkRequest request(
        QUrl(QStringLiteral("http://127.0.0.1:%1/v1/chat/completions").arg(kAiPort)));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));

    DiagnosticLogger::checkpoint(QStringLiteral("AI chat completion HTTP request submitted"));
    QNetworkReply *reply = m_network->post(request, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply, question]() {
        if (reply->error() != QNetworkReply::NoError) {
            const QString error = reply->errorString();
            DiagnosticLogger::log(QStringLiteral("AI_CHAT_ERROR: networkError=%1 errorString=%2")
                                      .arg(static_cast<int>(reply->error()))
                                      .arg(error));
            reply->deleteLater();
            setState(Ready);
            emit responseError(QStringLiteral("Erreur du moteur d'IA locale : %1").arg(error));
            return;
        }

        const QByteArray responseBytes = reply->readAll();
        DiagnosticLogger::log(QStringLiteral("AI_CHAT_REPLY: bytes=%1").arg(responseBytes.size()));
        const QJsonDocument document = QJsonDocument::fromJson(responseBytes);
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
        if (answer.isEmpty()) {
            DiagnosticLogger::checkpoint(QStringLiteral("AI chat reply contained no usable answer"));
            emit responseError(QStringLiteral("Le modèle local n'a produit aucune réponse exploitable."));
            return;
        }

        m_conversation.append(qMakePair(question.trimmed(), answer));
        if (m_conversation.size() > kMaximumTurns)
            m_conversation.remove(0, m_conversation.size() - kMaximumTurns);
        DiagnosticLogger::log(QStringLiteral("AI_CHAT_SUCCESS: answerLength=%1 conversationTurns=%2")
                                  .arg(answer.size()).arg(m_conversation.size()));
        emit responseReady(answer);
    });
}

void LocalAiClient::clearConversation()
{
    m_conversation.clear();
    DiagnosticLogger::checkpoint(QStringLiteral("AI conversation history cleared"));
}

void LocalAiClient::shutdown()
{
    DiagnosticLogger::log(QStringLiteral("AI_SHUTDOWN: state=%1 startedServer=%2 processState=%3 pid=%4")
                              .arg(stateName(m_state))
                              .arg(m_startedServer ? QStringLiteral("yes") : QStringLiteral("no"))
                              .arg(static_cast<int>(m_server ? m_server->state() : QProcess::NotRunning))
                              .arg(m_server ? m_server->processId() : 0));
    if (m_healthTimer)
        m_healthTimer->stop();

    // Prevent a deliberate shutdown from being reported as an engine failure.
    m_state = NotStarted;
    if (m_startedServer && m_server && m_server->state() != QProcess::NotRunning) {
        DiagnosticLogger::checkpoint(QStringLiteral("Terminating llama-server started by MEMS Manager"));
        m_server->terminate();
        if (!m_server->waitForFinished(1200)) {
            DiagnosticLogger::checkpoint(QStringLiteral("llama-server did not terminate in 1200 ms; killing process"));
            m_server->kill();
            m_server->waitForFinished(500);
        }
    }
    m_startedServer = false;
    DiagnosticLogger::checkpoint(QStringLiteral("AI shutdown completed"));
}

void LocalAiClient::setState(State state, const QString &error)
{
    const State previous = m_state;
    if (!error.isEmpty())
        m_lastError = error;
    else if (state == Ready || state == Starting)
        m_lastError.clear();

    if (m_state == state && error.isEmpty())
        return;
    m_state = state;
    DiagnosticLogger::log(QStringLiteral("AI_STATE: %1 -> %2 error=%3")
                              .arg(stateName(previous), stateName(state),
                                   error.isEmpty() ? QStringLiteral("<none>") : error.simplified().left(1000)));
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
    return QStringLiteral(
        "Tu es IA MEMS, l'assistant conversationnel intégré à ECU MEMS Manager. "
        "Tu parles naturellement en français, avec courtoisie et sans familiarité excessive. "
        "Réponds toujours d'abord à la question réellement posée, sans dériver vers un sujet voisin. "
        "Tolère les fautes d'orthographe et de frappe ; si le sens reste clair, ne les commente pas. "
        "Pour une conversation courante sans enjeu technique MEMS, réponds naturellement sans forcer un lien avec l'ECU. "
        "Pour une question sur le logiciel, explique directement la fonction demandée avant tout détail technique. "
        "Pour une question technique MEMS, les faits pertinents fournis par MEMS Manager sont prioritaires sur tes connaissances générales. "
        "Le contexte fourni peut contenir plusieurs éléments : ignore strictement tout élément qui ne répond pas directement à la question. "
        "Ne cite pas de nom de personne, de site, de dépôt ou de source sauf si l'utilisateur le demande ou si cette provenance est nécessaire pour qualifier la fiabilité d'un fait. "
        "Tu ne dois jamais inventer une mesure ECU, un défaut, une adresse, une fonction du logiciel ou une source. "
        "Tu distingues clairement une mesure observée, une hypothèse, une information externe et une information incertaine. "
        "Tu ne condamnes jamais une pièce sur une seule mesure. "
        "Quand les données techniques sont insuffisantes ou contradictoires, dis que tu ne peux pas conclure et précise brièvement ce qui manque. "
        "Si la question est ambiguë au point de changer la réponse, demande une seule précision utile au lieu de deviner. "
        "Réponds de façon claire, naturelle et concise, sauf si l'utilisateur demande des détails. /no_think");
}

QString LocalAiClient::cleanModelReply(QString text) const
{
    text.remove(QRegularExpression(QStringLiteral("<think>.*?</think>"),
                                   QRegularExpression::DotMatchesEverythingOption));
    text.replace(QStringLiteral("/no_think"), QString());
    return text.trimmed();
}
