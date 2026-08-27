#include "IaMemsService.h"

#include "IaResponseLogic.h"
#include "LocalAiClient.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QPointer>
#include <QRegularExpression>
#include <QSet>
#include <QVector>

#include <algorithm>
#include <cmath>

namespace {

enum class KnowledgeQueryKind {
    General,
    WireColor,
    Pinout
};

QString normalized(const QString &text)
{
    QString value = IaResponseLogic::normalize(text);
    // Narrow corrections for real AZERTY typing mistakes observed in IA MEMS.
    // Do not globally translate digit 4: protocol commands such as D4 must stay intact.
    value.replace(QRegularExpression(QStringLiteral("\\bc4est\\b")), QStringLiteral("c est"));
    value.replace(QRegularExpression(QStringLiteral("\\bl4onglet\\b")), QStringLiteral("l onglet"));
    value.replace(QRegularExpression(QStringLiteral("\\bl4apercu\\b")), QStringLiteral("l apercu"));
    if (value.contains(QStringLiteral("cadran"))
        || value.contains(QStringLiteral("regime"))
        || value.contains(QStringLiteral("tr/min")))
        value.replace(QRegularExpression(QStringLiteral("\\btpm\\b")), QStringLiteral("rpm"));
    return value.simplified();
}

bool containsAny(const QString &text, const QStringList &needles)
{
    for (const QString &needle : needles) {
        if (text.contains(needle))
            return true;
    }
    return false;
}

bool containsWord(const QString &text, const QString &word)
{
    const QRegularExpression rx(QStringLiteral("(^|[^a-z0-9_])%1([^a-z0-9_]|$)")
                                    .arg(QRegularExpression::escape(word)));
    return rx.match(text).hasMatch();
}

KnowledgeQueryKind knowledgeQueryKind(const QString &questionText)
{
    const bool asksColor = questionText.contains(QStringLiteral("couleur"))
        && (containsWord(questionText, QStringLiteral("fil"))
            || questionText.contains(QStringLiteral("cable"))
            || questionText.contains(QStringLiteral("cablage")));
    if (asksColor
        || questionText.contains(QStringLiteral("wire color"))
        || questionText.contains(QStringLiteral("wire colour")))
        return KnowledgeQueryKind::WireColor;

    if (containsWord(questionText, QStringLiteral("broche"))
        || questionText.contains(QStringLiteral("pinout"))
        || questionText.contains(QStringLiteral("connecteur"))
        || containsWord(questionText, QStringLiteral("prise"))
        || containsWord(questionText, QStringLiteral("obd"))
        || questionText.contains(QStringLiteral("cablage"))
        || questionText.contains(QStringLiteral("wiring")))
        return KnowledgeQueryKind::Pinout;

    return KnowledgeQueryKind::General;
}

bool hasWireColorEvidence(const QString &text)
{
    return text.contains(QStringLiteral("wire_color"))
        || text.contains(QStringLiteral("wire color"))
        || text.contains(QStringLiteral("wire colour"))
        || text.contains(QStringLiteral("couleur de fil"))
        || text.contains(QStringLiteral("couleur du fil"))
        || text.contains(QStringLiteral("couleur fil"))
        || text.contains(QStringLiteral("code couleur"))
        || containsWord(text, QStringLiteral("fil"));
}

bool hasPinoutEvidence(const QString &text)
{
    return text.contains(QStringLiteral("wiring"))
        || containsWord(text, QStringLiteral("broche"))
        || text.contains(QStringLiteral("pinout"))
        || text.contains(QStringLiteral("connector"))
        || text.contains(QStringLiteral("connecteur"))
        || text.contains(QStringLiteral("socket"))
        || text.contains(QStringLiteral("diagnostic_socket"))
        || containsWord(text, QStringLiteral("obd"))
        || text.contains(QStringLiteral("c159"))
        || text.contains(QStringLiteral("c549"))
        || containsWord(text, QStringLiteral("prise"));
}

QString number(double value, int decimals = 1)
{
    if (!std::isfinite(value))
        return QStringLiteral("—");
    return QString::number(value, 'f', decimals);
}

void appendUniqueTerm(QStringList &terms, const QString &term)
{
    if (!term.isEmpty() && !terms.contains(term) && terms.size() < 12)
        terms.append(term);
}

QStringList knowledgeTerms(const QString &question)
{
    QString text = normalized(question);
    text.replace(QRegularExpression(QStringLiteral("[^a-z0-9_.-]+")), QStringLiteral(" "));
    const QSet<QString> stopWords = {
        QStringLiteral("que"), QStringLiteral("quoi"), QStringLiteral("sur"), QStringLiteral("les"),
        QStringLiteral("des"), QStringLiteral("une"), QStringLiteral("dans"), QStringLiteral("pour"),
        QStringLiteral("avec"), QStringLiteral("est"), QStringLiteral("sais"), QStringLiteral("sait"),
        QStringLiteral("peux"), QStringLiteral("dire"), QStringLiteral("cette"), QStringLiteral("ce"),
        QStringLiteral("cet"), QStringLiteral("mon"), QStringLiteral("ma"), QStringLiteral("mes"),
        QStringLiteral("mems"), QStringLiteral("source"), QStringLiteral("sources"),
        QStringLiteral("preuve"), QStringLiteral("preuves"), QStringLiteral("document"),
        QStringLiteral("documentation")
    };

    QStringList result;
    const QStringList words = text.split(QLatin1Char(' '), Qt::SkipEmptyParts);
    for (const QString &word : words) {
        if (word.size() < 3 || stopWords.contains(word))
            continue;
        appendUniqueTerm(result, word);
        if (result.size() >= 7)
            break;
    }

    const QStringList original = result;
    for (const QString &term : original) {
        if (term == QStringLiteral("bobine")) {
            appendUniqueTerm(result, QStringLiteral("dwell"));
            appendUniqueTerm(result, QStringLiteral("coil"));
            appendUniqueTerm(result, QStringLiteral("allumage"));
        } else if (term == QStringLiteral("papillon")) {
            appendUniqueTerm(result, QStringLiteral("tps"));
            appendUniqueTerm(result, QStringLiteral("throttle"));
        } else if (term == QStringLiteral("ralenti")) {
            appendUniqueTerm(result, QStringLiteral("iac"));
            appendUniqueTerm(result, QStringLiteral("iacv"));
            appendUniqueTerm(result, QStringLiteral("idle"));
        } else if (term == QStringLiteral("liquide") || term == QStringLiteral("refroidissement")) {
            appendUniqueTerm(result, QStringLiteral("ect"));
            appendUniqueTerm(result, QStringLiteral("coolant"));
        } else if (term == QStringLiteral("temperature")) {
            appendUniqueTerm(result, QStringLiteral("ect"));
            appendUniqueTerm(result, QStringLiteral("iat"));
        } else if (term == QStringLiteral("admission")) {
            appendUniqueTerm(result, QStringLiteral("iat"));
        } else if (term == QStringLiteral("vilebrequin")) {
            appendUniqueTerm(result, QStringLiteral("ckp"));
        } else if (term == QStringLiteral("cames")) {
            appendUniqueTerm(result, QStringLiteral("cmp"));
        } else if (term == QStringLiteral("lambda")) {
            appendUniqueTerm(result, QStringLiteral("oxygen"));
        } else if (term == QStringLiteral("broche") || term == QStringLiteral("connecteur")
                   || term == QStringLiteral("obd")) {
            appendUniqueTerm(result, QStringLiteral("wiring"));
            appendUniqueTerm(result, QStringLiteral("pin"));
        }
    }
    return result;
}

int verificationScore(const QString &level)
{
    if (level == QStringLiteral("verifie_constructeur")) return 8;
    if (level == QStringLiteral("decoded_by_project")) return 7;
    if (level == QStringLiteral("recoupee")) return 6;
    if (level == QStringLiteral("source_externe")) return 4;
    if (level == QStringLiteral("plausible")) return 2;
    if (level == QStringLiteral("non_verifie")) return 0;
    if (level == QStringLiteral("conflit_a_verifier")) return -2;
    return 0;
}

bool asksForSourceDetails(const QString &question)
{
    const QString text = normalized(question);
    return containsAny(text, {
        QStringLiteral("source"), QStringLiteral("preuve"), QStringLiteral("document"),
        QStringLiteral("documentation"), QStringLiteral("d ou vient"),
        QStringLiteral("origine de l information"), QStringLiteral("reference constructeur")
    });
}

} // namespace

IaMemsService *IaMemsService::instance()
{
    static QPointer<IaMemsService> service;
    if (!service) {
        QObject *application = QCoreApplication::instance();
        service = new IaMemsService(application);
        service->setObjectName(QStringLiteral("iaMemsApplicationService"));
    }
    return service.data();
}

IaMemsService::IaMemsService(QObject *parent)
    : QObject(parent),
      m_localAi(new LocalAiClient(this))
{
    connect(m_localAi, &LocalAiClient::stateChanged,
            this, &IaMemsService::statusChanged);

    connect(m_localAi, &LocalAiClient::responseReady,
            this, [this](const QString &text) {
                m_pendingGrounding.clear();
                emit responseReady(text);
                emit statusChanged();
            });

    connect(m_localAi, &LocalAiClient::responseError,
            this, [this](const QString &message) {
                QString fallback = m_pendingGrounding.trimmed();
                if (fallback.isEmpty())
                    fallback = QStringLiteral("Je ne peux pas répondre avec le moteur conversationnel local pour le moment.");
                m_pendingGrounding.clear();
                emit responseReady(fallback);
                emit systemMessage(QStringLiteral("Moteur conversationnel local indisponible : %1").arg(message));
                emit statusChanged();
            });

    m_engine.setContext(m_context);
}

void IaMemsService::activate()
{
    if (!m_knowledgeAttempted)
        openPackagedKnowledge();

    m_activated = true;

    if (m_localAi
        && m_localAi->state() != LocalAiClient::Starting
        && m_localAi->state() != LocalAiClient::Ready
        && m_localAi->state() != LocalAiClient::Busy) {
        m_localAi->initialize();
    }

    emit statusChanged();
}

void IaMemsService::openPackagedKnowledge()
{
    m_knowledgeAttempted = true;
    m_knowledgeReady = false;
    m_databasePath.clear();
    m_knowledgeError.clear();

    const QString path = QDir(QCoreApplication::applicationDirPath())
                             .filePath(QStringLiteral("database/expert/ia_mems_reference_r20.sqlite"));

    if (!QFileInfo::exists(path) || !QFileInfo(path).isFile()) {
        m_knowledgeError = QStringLiteral("Base experte r20 absente du package.");
        emit systemMessage(QStringLiteral(
            "La base experte MEMS préconstruite n'est pas disponible. Le dialogue local reste utilisable sans cette base."));
        emit statusChanged();
        return;
    }

    if (!m_reader.openReadOnly(path)) {
        m_knowledgeError = m_reader.lastError();
        emit systemMessage(QStringLiteral(
            "La base experte MEMS n'a pas pu être ouverte en lecture seule. Le dialogue local reste utilisable."));
        emit statusChanged();
        return;
    }

    m_databasePath = path;
    m_knowledgeReady = true;
    m_engine.setKnowledgeReader(&m_reader);
    emit systemMessage(QStringLiteral("Base de connaissances MEMS prête en lecture seule."));
    emit statusChanged();
}

void IaMemsService::setConnected(bool connected)
{
    if (m_connected == connected)
        return;
    m_connected = connected;
    emit statusChanged();
}

void IaMemsService::setContext(const ExpertContext &context)
{
    m_context = context;
    m_engine.setContext(m_context);
    emit statusChanged();
}

void IaMemsService::addObservation(const ExpertObservation &observation)
{
    if (observation.timestampMs > 0)
        IaResponseLogic::noteMeasurementTimestamp(observation.timestampMs);

    m_history.append(observation);
    if (m_history.size() > kMaximumSamples)
        m_history.removeFirst();

    if (m_engine.sampleCount() >= kMaximumSamples) {
        m_engine.clearSamples();
        for (const ExpertObservation &sample : m_history)
            m_engine.addSample(sample);
    } else {
        m_engine.addSample(observation);
    }
}

void IaMemsService::ask(const QString &question)
{
    const QString trimmed = question.trimmed();
    if (trimmed.isEmpty())
        return;

    updateContextFromQuestion(trimmed);
    m_pendingGrounding = groundingFor(trimmed);

    if (m_localAi && m_localAi->isReady()) {
        m_localAi->ask(trimmed, m_pendingGrounding);
        emit statusChanged();
        return;
    }

    const QString fallback = m_pendingGrounding.trimmed().isEmpty()
        ? QStringLiteral("L'IA locale n'est pas encore prête.")
        : m_pendingGrounding;
    m_pendingGrounding.clear();
    emit responseReady(fallback);
    emit statusChanged();
}

void IaMemsService::clearConversation()
{
    if (m_localAi)
        m_localAi->clearConversation();
}

bool IaMemsService::localAiReady() const
{
    return m_localAi && m_localAi->isReady();
}

QString IaMemsService::statusText() const
{
    QStringList parts;
    if (m_knowledgeReady)
        parts << QStringLiteral("base prête");
    else if (m_knowledgeAttempted)
        parts << QStringLiteral("base indisponible");
    else
        parts << QStringLiteral("base non chargée");

    if (!m_context.family.isEmpty())
        parts << QStringLiteral("MEMS %1").arg(m_context.family);
    if (!m_context.firmware.isEmpty())
        parts << m_context.firmware;
    if (m_localAi)
        parts << m_localAi->statusText();

    return parts.join(QStringLiteral("  •  "));
}

QString IaMemsService::lastError() const
{
    QStringList errors;
    if (!m_knowledgeError.trimmed().isEmpty())
        errors << m_knowledgeError.trimmed();
    if (m_localAi && !m_localAi->lastError().trimmed().isEmpty())
        errors << m_localAi->lastError().trimmed();
    return errors.join(QStringLiteral(" | "));
}

void IaMemsService::updateContextFromQuestion(const QString &question)
{
    const QString text = normalized(question);
    const bool versionContext = containsAny(text, {
        QStringLiteral("mems"), QStringLiteral("ecu"), QStringLiteral("obd"), QStringLiteral("rosco")
    });
    if (versionContext && text.contains(QStringLiteral("1.2")))
        m_context.family = QStringLiteral("1.2");
    else if (versionContext && text.contains(QStringLiteral("1.3")))
        m_context.family = QStringLiteral("1.3");
    else if (versionContext && text.contains(QStringLiteral("1.6")))
        m_context.family = QStringLiteral("1.6");
    else if (versionContext && text.contains(QStringLiteral("1.9")))
        m_context.family = QStringLiteral("1.9");

    static const QRegularExpression firmwareRx(QStringLiteral("\\b([A-Z]{4,}[A-Z0-9]*[0-9]{3,})\\b"));
    QRegularExpressionMatchIterator iterator = firmwareRx.globalMatch(question.toUpper());
    while (iterator.hasNext()) {
        const QString candidate = iterator.next().captured(1);
        if (!candidate.startsWith(QStringLiteral("MNE")) && !candidate.startsWith(QStringLiteral("MKC"))) {
            m_context.firmware = candidate;
            break;
        }
    }

    m_engine.setContext(m_context);
    emit statusChanged();
}

QString IaMemsService::groundingFor(const QString &question)
{
    const QString text = normalized(question);

    if (containsAny(text, {QStringLiteral("bonjour"), QStringLiteral("bonsoir"),
                           QStringLiteral("salut"), QStringLiteral("hello")}))
        return QStringLiteral(
            "Bonjour. Je suis IA MEMS. Je peux répondre à vos questions sur ECU MEMS Manager, les systèmes MEMS et les mesures de l'ECU connecté.");

    if (containsAny(text, {QStringLiteral("faute"), QStringLiteral("fautes"),
                           QStringLiteral("orthographe"), QStringLiteral("mal ecrit"),
                           QStringLiteral("erreur de frappe")}))
        return QStringLiteral(
            "Les fautes d'orthographe ou de frappe ne sont pas un problème si le sens reste compréhensible. Une précision ne doit être demandée que si l'ambiguïté change réellement la réponse.");

    const QString software = softwareAnswer(question);
    if (!software.isEmpty())
        return software;

    if (containsAny(text, {QStringLiteral("aide"), QStringLiteral("que peux tu"),
                           QStringLiteral("que peux-tu"), QStringLiteral("comment te parler")}))
        return helpAnswer();

    const IaResponseLogic::Intent intent = IaResponseLogic::classify(question);
    if (intent == IaResponseLogic::Intent::Captures)
        return IaResponseLogic::capturesAnswer();
    if (intent == IaResponseLogic::Intent::EngineState)
        return IaResponseLogic::engineStateAnswer(m_history.isEmpty() ? QHash<QString, double>() : m_history.constLast().values,
                                                  m_connected);
    if (intent == IaResponseLogic::Intent::Diagnostic)
        return IaResponseLogic::diagnosticAnswer(m_history.isEmpty() ? QHash<QString, double>() : m_history.constLast().values,
                                                 m_connected);
    if (intent != IaResponseLogic::Intent::None)
        return IaResponseLogic::metricAnswer(intent,
                                             m_history.isEmpty() ? QHash<QString, double>() : m_history.constLast().values,
                                             m_connected);

    if (containsAny(text, {QStringLiteral("historique"), QStringLiteral("evolution"),
                           QStringLiteral("tendance"), QStringLiteral("oscill"),
                           QStringLiteral("depuis")}))
        return historyAnswer();

    if (containsAny(text, {QStringLiteral("anormal"), QStringLiteral("normal"),
                           QStringLiteral("normale"), QStringLiteral("diagnostic"),
                           QStringLiteral("panne"), QStringLiteral("probleme"),
                           QStringLiteral("cause"), QStringLiteral("hypothese")}))
        return analysisAnswer();

    if (containsAny(text, {QStringLiteral("valeur"), QStringLiteral("mesure"),
                           QStringLiteral("actuel"), QStringLiteral("actuelle")}))
        return currentValuesAnswer();

    const QString knowledge = knowledgeAnswer(question);
    if (!knowledge.isEmpty())
        return knowledge;

    return QStringLiteral(
        "Je n'ai pas assez d'éléments pour relier cette question à une mesure ou à un fait MEMS précis.");
}

QString IaMemsService::softwareAnswer(const QString &question) const
{
    const QString text = normalized(question);
    const bool softwareIntent = containsAny(text, {
        QStringLiteral("onglet"), QStringLiteral("cadran"), QStringLiteral("mems manager"),
        QStringLiteral("logiciel"), QStringLiteral("programme"),
        QStringLiteral("a quoi sert"), QStringLiteral("comment fonctionne"),
        QStringLiteral("c'est quoi"), QStringLiteral("c est quoi")
    });
    if (!softwareIntent)
        return QString();

    if (containsAny(text, {QStringLiteral("que peut faire"), QStringLiteral("que peux faire"),
                           QStringLiteral("fonctionnalite"), QStringLiteral("possibilites")}))
        return QStringLiteral(
            "ECU MEMS Manager dialogue avec les ECU Rover/Mini MEMS pris en charge, affiche les mesures en direct, lit les défauts, exécute les tests et réglages supportés, analyse les journaux CSV/TXT, consulte la base technique, expose les fonctions ROSCO, suit l'injection lorsque ce mode est disponible et fournit IA MEMS pour expliquer les mesures, l'historique et les hypothèses diagnostiques.");

    if (containsAny(text, {QStringLiteral("qui a concu"), QStringLiteral("qui a cree"),
                           QStringLiteral("auteur"), QStringLiteral("developpe par")}))
        return QStringLiteral(
            "ECU MEMS Manager a été conçu et développé par Claude Lespagnol. Les sources comme Andrew Revill ou RoverMEMS sont des références techniques intégrées au projet, pas les concepteurs de MEMS Manager.");

    if (text.contains(QStringLiteral("apercu")) && text.contains(QStringLiteral("cadran")))
        return QStringLiteral(
            "L'onglet Aperçu contient 11 cadrans : régime moteur, température du liquide, MAP, position papillon, tension batterie, correction court terme, tension lambda, temps d'injection, température d'air, position de ralenti/IAC et avance à l'allumage. Un indicateur d'état système complète ces cadrans.");

    if (text.contains(QStringLiteral("cadran"))
        && !containsAny(text, {QStringLiteral("valeur"), QStringLiteral("mesure"),
                               QStringLiteral("actuel"), QStringLiteral("actuelle")})) {
        if (containsAny(text, {QStringLiteral("rpm"), QStringLiteral("regime"), QStringLiteral("tr/min")}))
            return QStringLiteral("Le cadran Régime affiche la vitesse de rotation du moteur en tr/min (RPM).");
        if (containsAny(text, {QStringLiteral("map"), QStringLiteral("pression collecteur")}))
            return QStringLiteral("Le cadran MAP affiche la pression absolue du collecteur d'admission en kPa.");
        if (containsAny(text, {QStringLiteral("temperature liquide"), QStringLiteral("liquide refroidissement")}))
            return QStringLiteral("Le cadran Température liquide affiche la température du liquide de refroidissement en °C.");
        if (containsAny(text, {QStringLiteral("papillon"), QStringLiteral("tps")}))
            return QStringLiteral("Le cadran Papillon affiche la position du papillon en pourcentage.");
        if (text.contains(QStringLiteral("batterie")))
            return QStringLiteral("Le cadran Batterie affiche la tension d'alimentation en volts.");
        if (text.contains(QStringLiteral("lambda")))
            return QStringLiteral("Le cadran Lambda affiche la tension de la sonde lambda en mV.");
        if (text.contains(QStringLiteral("injection")))
            return QStringLiteral("Le cadran Temps d'injection affiche la durée d'injection disponible dans le mode d'acquisition prévu.");
        if (containsAny(text, {QStringLiteral("temperature air"), QStringLiteral("iat")}))
            return QStringLiteral("Le cadran Température d'air affiche la température d'air d'admission en °C.");
        if (containsAny(text, {QStringLiteral("ralenti"), QStringLiteral("iac")}))
            return QStringLiteral("Le cadran Position de ralenti affiche la position de commande IAC disponible.");
        if (containsAny(text, {QStringLiteral("avance"), QStringLiteral("allumage")}))
            return QStringLiteral("Le cadran Avance affiche l'avance à l'allumage en degrés.");
    }

    if (text.contains(QStringLiteral("analyse")))
        return QStringLiteral("L'onglet Analyse étudie les journaux de diagnostic CSV/TXT et permet de visualiser l'évolution des canaux de mesure.");
    if (text.contains(QStringLiteral("apercu")))
        return QStringLiteral("L'onglet Aperçu présente les principales mesures ECU en direct sous forme de cadrans et d'indicateurs.");
    if (text.contains(QStringLiteral("injection")))
        return QStringLiteral("L'onglet Injection affiche les mesures d'injection acquises par le mode prévu pour cela. IA MEMS n'active aucun mode de polling de son propre chef.");
    if (text.contains(QStringLiteral("reglage")))
        return QStringLiteral("L'onglet Réglages contient les ajustements de service supportés par l'ECU. Ces fonctions peuvent modifier réellement le comportement de l'ECU.");
    if (text.contains(QStringLiteral("actionneur")))
        return QStringLiteral("L'onglet Actionneurs exécute les tests d'actionneurs supportés. Ces commandes peuvent actionner réellement des organes du véhicule.");
    if (text.contains(QStringLiteral("erreur")) || text.contains(QStringLiteral("defaut")))
        return QStringLiteral("L'onglet Erreurs affiche les défauts et états associés renvoyés par l'ECU.");
    if (text.contains(QStringLiteral("diagnostic automatique")) || text.contains(QStringLiteral("diagnostique automatique")))
        return QStringLiteral("L'onglet Diagnostic automatique vérifie les mesures ECU courantes et signale les anomalies ou points à surveiller.");
    if (text.contains(QStringLiteral("toutes les mesures")))
        return QStringLiteral("L'onglet Toutes les mesures rassemble les paramètres décodés dans une vue détaillée.");
    if (text.contains(QStringLiteral("rosco")))
        return QStringLiteral("L'onglet ECU/ROSCO expose les fonctions de session et de diagnostic ROSCO prises en charge. Les fonctions dangereuses ou non validées restent bloquées.");
    if (text.contains(QStringLiteral("toutes les donnees")))
        return QStringLiteral("L'onglet Toutes les données présente les champs MEMS bruts et décodés.");
    if (text.contains(QStringLiteral("base")) && text.contains(QStringLiteral("donnee")))
        return QStringLiteral("L'onglet Base de données donne accès à la documentation et aux données techniques intégrées à MEMS Manager. IA MEMS utilise la base experte préconstruite en lecture seule.");
    if (text.contains(QStringLiteral("ia mems")) || text == QStringLiteral("ia"))
        return QStringLiteral("IA MEMS combine les mesures ECU déjà acquises, leur historique, le moteur expert, la base de connaissances et Qwen local. Le modèle n'a aucun accès aux commandes d'écriture ECU.");

    return QString();
}

QString IaMemsService::currentValuesAnswer() const
{
    if (m_history.isEmpty())
        return QStringLiteral("Je n'ai encore reçu aucune mesure ECU.");

    const QHash<QString, double> &v = m_history.constLast().values;
    QStringList lines;
    lines << QStringLiteral("Dernière mesure ECU disponible :")
          << QStringLiteral("• Régime : %1 tr/min").arg(number(v.value(QStringLiteral("rpm")), 0))
          << QStringLiteral("• Température liquide : %1 °C").arg(number(v.value(QStringLiteral("coolant_c")), 0))
          << QStringLiteral("• MAP : %1 kPa").arg(number(v.value(QStringLiteral("map_kpa")), 0))
          << QStringLiteral("• Batterie : %1 V").arg(number(v.value(QStringLiteral("battery_v")), 1))
          << QStringLiteral("• Lambda : %1 mV").arg(number(v.value(QStringLiteral("lambda_mv")), 0))
          << QStringLiteral("• Avance : %1°").arg(number(v.value(QStringLiteral("ignition_advance_deg")), 1))
          << QStringLiteral("• Dwell bobine : %1 ms").arg(number(v.value(QStringLiteral("coil_time_ms")), 3));
    if (v.contains(QStringLiteral("injection_final_ms")))
        lines << QStringLiteral("• Injection finale : %1 ms").arg(number(v.value(QStringLiteral("injection_final_ms")), 2));
    return lines.join(QLatin1Char('\n'));
}

QString IaMemsService::historyAnswer() const
{
    if (m_history.size() < 2)
        return QStringLiteral("Je n'ai pas encore assez d'historique pour décrire une évolution.");

    const ExpertAnalysisResult analysis = m_engine.analyze();
    const qint64 elapsedMs = m_history.constLast().timestampMs - m_history.constFirst().timestampMs;
    QStringList lines;
    lines << QStringLiteral("Historique analysé : %1 échantillons sur environ %2 s.")
             .arg(m_history.size())
             .arg(number(elapsedMs / 1000.0, 0));

    const struct Metric { const char *key; const char *label; const char *unit; int decimals; } metrics[] = {
        {"rpm", "Régime", "tr/min", 0},
        {"map_kpa", "MAP", "kPa", 1},
        {"coolant_c", "Température liquide", "°C", 1},
        {"battery_v", "Batterie", "V", 2},
        {"lambda_mv", "Lambda", "mV", 0},
        {"ignition_advance_deg", "Avance", "°", 1},
        {"coil_time_ms", "Dwell bobine", "ms", 3}
    };

    for (const Metric &metric : metrics) {
        const QString key = QString::fromLatin1(metric.key);
        if (!analysis.statistics.contains(key))
            continue;
        const ExpertSeriesStats stats = analysis.statistics.value(key);
        lines << QStringLiteral("• %1 : moyenne %2 %3, min %4, max %5, tendance %6 %3/s")
                 .arg(QString::fromUtf8(metric.label),
                      number(stats.mean, metric.decimals),
                      QString::fromUtf8(metric.unit),
                      number(stats.minimum, metric.decimals),
                      number(stats.maximum, metric.decimals),
                      number(stats.slopePerSecond, metric.decimals));
    }
    return lines.join(QLatin1Char('\n'));
}

QString IaMemsService::analysisAnswer()
{
    if (m_history.isEmpty())
        return QStringLiteral("Je ne peux pas faire d'analyse sans mesure ECU.");

    m_lastAnalysis = m_engine.analyze();
    if (m_lastAnalysis.hypotheses.isEmpty()) {
        QString text = QStringLiteral(
            "Je n'ai actuellement aucune hypothèse diagnostique suffisamment étayée par les règles chargées. Je ne vais pas inventer une cause à partir d'une seule valeur.");
        if (!m_knowledgeReady)
            text += QStringLiteral(" La base experte n'est pas disponible.");
        return text;
    }

    QStringList lines;
    lines << QStringLiteral("Hypothèses actuelles du moteur expert :");
    const int maximum = qMin(5, m_lastAnalysis.hypotheses.size());
    for (int i = 0; i < maximum; ++i) {
        const ExpertHypothesis &hypothesis = m_lastAnalysis.hypotheses.at(i);
        lines << QStringLiteral("%1. %2 — confiance %3 % — preuve %4")
                 .arg(i + 1)
                 .arg(hypothesis.title)
                 .arg(number(hypothesis.confidence * 100.0, 0))
                 .arg(verificationLabel(hypothesis.verificationLevel));
        for (const QString &evidence : hypothesis.evidence)
            lines << QStringLiteral("   • %1").arg(evidence);
        if (!hypothesis.strongConclusionAllowed)
            lines << QStringLiteral("   • Conclusion forte interdite avec ce niveau de preuve.");
        for (const QString &recommendation : hypothesis.recommendations)
            lines << QStringLiteral("   → %1").arg(recommendation);
    }
    return lines.join(QLatin1Char('\n'));
}

QString IaMemsService::knowledgeAnswer(const QString &question) const
{
    if (!m_knowledgeReady || !m_reader.isOpen())
        return QString();

    const QString questionText = normalized(question);
    const KnowledgeQueryKind queryKind = knowledgeQueryKind(questionText);
    const QStringList terms = knowledgeTerms(question);
    if (terms.isEmpty())
        return QString();

    struct RankedFact {
        ExpertFact fact;
        int score = 0;
        int matches = 0;
    };

    const bool sourceDetails = asksForSourceDetails(question);
    const QList<ExpertFact> facts = m_reader.facts(m_context);
    QVector<RankedFact> ranked;
    ranked.reserve(facts.size());

    for (const ExpertFact &fact : facts) {
        const QString identity = normalized(QStringLiteral("%1 %2 %3")
                                                .arg(fact.factKey, fact.topic, fact.firmware));
        const QString body = normalized(QStringLiteral("%1 %2 %3")
                                            .arg(fact.statement, fact.notes, fact.family));
        const QString searchable = identity + QLatin1Char(' ') + body;

        if (queryKind == KnowledgeQueryKind::WireColor && !hasWireColorEvidence(searchable))
            continue;
        if (queryKind == KnowledgeQueryKind::Pinout && !hasPinoutEvidence(searchable))
            continue;

        RankedFact candidate;
        candidate.fact = fact;

        for (const QString &term : terms) {
            if (identity.contains(term)) {
                candidate.score += 5;
                ++candidate.matches;
            } else if (body.contains(term)) {
                candidate.score += 2;
                ++candidate.matches;
            }
        }
        if (candidate.matches == 0)
            continue;

        candidate.score += qMin(candidate.matches, 4) * 3;
        candidate.score += verificationScore(fact.verificationLevel);
        if (queryKind == KnowledgeQueryKind::WireColor)
            candidate.score += 24;
        else if (queryKind == KnowledgeQueryKind::Pinout)
            candidate.score += 16;

        if (!m_context.family.trimmed().isEmpty()
            && fact.family.compare(m_context.family, Qt::CaseInsensitive) == 0)
            candidate.score += 4;
        if (!m_context.firmware.trimmed().isEmpty()
            && fact.firmware.compare(m_context.firmware, Qt::CaseInsensitive) == 0)
            candidate.score += 6;

        const QString topic = normalized(fact.topic);
        if (topic.size() >= 3 && questionText.contains(topic))
            candidate.score += 4;

        ranked.append(candidate);
    }

    if (ranked.isEmpty()) {
        if (queryKind == KnowledgeQueryKind::WireColor) {
            if (questionText.contains(QStringLiteral("temperature")))
                return QStringLiteral(
                    "Je n'ai pas de couleur de fil vérifiée correspondant exactement à cette demande dans la base. Je ne vais pas en inventer une. Sur les montages MEMS, « capteur de température » peut désigner l'ECT (liquide de refroidissement) ou l'IAT (air d'admission) : précise lequel et, si possible, le véhicule ou la variante ECU.");
            return QStringLiteral(
                "Je n'ai pas de couleur de fil vérifiée correspondant exactement à cette demande dans la base. Je ne vais pas en inventer une ; précise le capteur, le véhicule ou la variante ECU recherchée.");
        }
        if (queryKind == KnowledgeQueryKind::Pinout)
            return QStringLiteral(
                "Je n'ai pas de brochage vérifié correspondant exactement à cette demande dans le contexte MEMS sélectionné. Précise le signal, le capteur ou le connecteur recherché ; je ne vais pas inventer une broche.");
        return QString();
    }

    std::sort(ranked.begin(), ranked.end(), [](const RankedFact &left, const RankedFact &right) {
        if (left.score != right.score)
            return left.score > right.score;
        if (left.matches != right.matches)
            return left.matches > right.matches;
        return left.fact.factKey < right.fact.factKey;
    });

    const int maximum = qMin(queryKind == KnowledgeQueryKind::General ? 3 : 4, ranked.size());
    if (maximum == 1) {
        const ExpertFact &fact = ranked.constFirst().fact;
        QString answer = fact.statement.trimmed();
        if (!fact.verificationLevel.trimmed().isEmpty())
            answer += QStringLiteral("\nNiveau de preuve : %1.").arg(verificationLabel(fact.verificationLevel));
        if (sourceDetails && !fact.sourceKey.trimmed().isEmpty())
            answer += QStringLiteral("\nSource : %1.").arg(fact.sourceKey.trimmed());
        if (sourceDetails && !fact.notes.trimmed().isEmpty())
            answer += QStringLiteral("\nNote : %1").arg(fact.notes.trimmed());
        return answer;
    }

    QStringList answers;
    if (queryKind == KnowledgeQueryKind::WireColor)
        answers << QStringLiteral("Couleurs de fil vérifiées les plus pertinentes :");
    else if (queryKind == KnowledgeQueryKind::Pinout)
        answers << QStringLiteral("Brochage vérifié le plus pertinent :");
    else
        answers << QStringLiteral("Les faits les plus pertinents de la base MEMS sont :");

    for (int i = 0; i < maximum; ++i) {
        const ExpertFact &fact = ranked.at(i).fact;
        QString line = QStringLiteral("• %1").arg(fact.statement.trimmed());
        if (!fact.verificationLevel.trimmed().isEmpty())
            line += QStringLiteral(" — preuve : %1").arg(verificationLabel(fact.verificationLevel));
        if (sourceDetails && !fact.sourceKey.trimmed().isEmpty())
            line += QStringLiteral(" ; source : %1").arg(fact.sourceKey.trimmed());
        answers << line;
        if (sourceDetails && !fact.notes.trimmed().isEmpty())
            answers << QStringLiteral("  Note : %1").arg(fact.notes.trimmed());
    }
    return answers.join(QLatin1Char('\n'));
}

QString IaMemsService::verificationLabel(const QString &level) const
{
    if (level == QStringLiteral("verifie_constructeur")) return QStringLiteral("constructeur");
    if (level == QStringLiteral("decoded_by_project")) return QStringLiteral("décodé par le projet");
    if (level == QStringLiteral("recoupee")) return QStringLiteral("recoupée");
    if (level == QStringLiteral("source_externe")) return QStringLiteral("source externe");
    if (level == QStringLiteral("plausible")) return QStringLiteral("plausible");
    if (level == QStringLiteral("non_verifie")) return QStringLiteral("non vérifiée");
    if (level == QStringLiteral("conflit_a_verifier")) return QStringLiteral("conflit à vérifier");
    return level.isEmpty() ? QStringLiteral("inconnu") : level;
}

QString IaMemsService::helpAnswer() const
{
    return QStringLiteral(
        "Vous pouvez me demander ce que je vois d'anormal, les valeurs actuelles, l'évolution des mesures, une information sur un firmware ou une famille MEMS, ou le fonctionnement d'un onglet de MEMS Manager. Les mesures réelles, les hypothèses et les connaissances externes restent distinguées.");
}
