#include "LocalAiClient.h"
#include "i18n.h"

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

QString activeLanguageName(const QString &code)
{
    if (code == QStringLiteral("en")) return QStringLiteral("English");
    if (code == QStringLiteral("es")) return QStringLiteral("Español");
    if (code == QStringLiteral("it")) return QStringLiteral("Italiano");
    if (code == QStringLiteral("pt")) return QStringLiteral("Português");
    if (code == QStringLiteral("de")) return QStringLiteral("Deutsch");
    return QStringLiteral("Français");
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

    // llama.cpp deliberately returns HTTP 503 while a model is loading. If a
    // server already owns our loopback port, simply keep waiting instead of
    // trying to spawn a competing process.
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

    QNetworkReply *reply = m_network->post(request, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply, question]() {
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
        if (answer.isEmpty()) {
            emit responseError(QStringLiteral("Le modèle local n'a produit aucune réponse exploitable."));
            return;
        }

        m_conversation.append(qMakePair(question.trimmed(), answer));
        if (m_conversation.size() > kMaximumTurns)
            m_conversation.remove(0, m_conversation.size() - kMaximumTurns);
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

    return QStringLiteral(
        "You are IA MEMS, the conversational assistant integrated into ECU MEMS Manager. "
        "The active MEMS Manager interface language is %1 (%2). Answer in that language by default. "
        "If the user explicitly asks for another language, follow that request. "
        "Understand spelling and typing mistakes when the meaning is clear; do not comment on them unnecessarily. "
        "Answer the exact question first and do not drift to a neighbouring topic. "
        "For ordinary conversation, answer naturally without forcing a link to the ECU. "
        "For software questions, explain the requested function directly before technical detail. "
        "For MEMS technical questions, relevant facts supplied by MEMS Manager take priority over general knowledge. "
        "The supplied context may contain several facts: strictly ignore facts that do not answer the current question. "
        "Never invent an ECU measurement, fault, protocol address, software function, source or confidence level. "
        "Clearly distinguish observed measurements, hypotheses, external information and uncertain information. "
        "Never condemn a component from one measurement alone. "
        "When technical data is insufficient or contradictory, say that you cannot conclude and briefly state what is missing. "
        "If ambiguity would materially change the answer, ask one useful clarification rather than guessing. "
        "Keep ECU numbers, units and factual values unchanged when changing language. "
        "Be clear, natural and concise unless the user asks for detail. /no_think")
        .arg(languageName, languageCode);
}

QString LocalAiClient::cleanModelReply(QString text) const
{
    text.remove(QRegularExpression(QStringLiteral("<think>.*?</think>"),
                                   QRegularExpression::DotMatchesEverythingOption));
    text.replace(QStringLiteral("/no_think"), QString());
    return text.trimmed();
}