#include "IaMemsService.h"

#include "IaResponseLogic.h"
#include "IaMemsConversationRouting.h"
#include "LocalAiClient.h"
#include "i18n.h"

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

int boundedKnowledgeEditDistance(const QString &left, const QString &right, int maximum)
{
    if (qAbs(left.size() - right.size()) > maximum)
        return maximum + 1;
    QVector<int> previous(right.size() + 1);
    QVector<int> current(right.size() + 1);
    for (int j = 0; j <= right.size(); ++j)
        previous[j] = j;
    for (int i = 1; i <= left.size(); ++i) {
        current[0] = i;
        int rowMinimum = current[0];
        for (int j = 1; j <= right.size(); ++j) {
            const int substitution = previous[j - 1]
                + (left.at(i - 1) == right.at(j - 1) ? 0 : 1);
            current[j] = qMin(qMin(previous[j] + 1, current[j - 1] + 1), substitution);
            rowMinimum = qMin(rowMinimum, current[j]);
        }
        if (rowMinimum > maximum)
            return maximum + 1;
        previous.swap(current);
    }
    return previous[right.size()];
}

bool fuzzyKnowledgeTermMatches(const QString &text, const QString &term)
{
    if (term.size() < 5 || term.contains(QRegularExpression(QStringLiteral("[^a-z]"))))
        return false;
    const int maximum = term.size() >= 9 ? 2 : 1;
    QRegularExpressionMatchIterator it =
        QRegularExpression(QStringLiteral("[a-z]+")) .globalMatch(text);
    while (it.hasNext()) {
        const QString candidate = it.next().captured(0);
        if (qAbs(candidate.size() - term.size()) <= maximum
            && boundedKnowledgeEditDistance(candidate, term, maximum) <= maximum)
            return true;
    }
    return false;
}

bool knowledgeTermMatches(const QString &text, const QString &term)
{
    static const QSet<QString> tokenTerms = {
        QStringLiteral("ect"), QStringLiteral("iat"), QStringLiteral("map"),
        QStringLiteral("tps"), QStringLiteral("ckp"), QStringLiteral("cmp"),
        QStringLiteral("iac"), QStringLiteral("iacv"), QStringLiteral("obd"),
        QStringLiteral("spi"), QStringLiteral("mpi"), QStringLiteral("rpm"),
        QStringLiteral("pin")
    };
    if (tokenTerms.contains(term) || term.size() <= 3)
        return containsWord(text, term);
    if (text.contains(term))
        return true;
    return fuzzyKnowledgeTermMatches(text, term);
}

KnowledgeQueryKind knowledgeQueryKind(const QString &questionText)
{
    if (IaMemsConversationRouting::isWireColourQuestion(questionText))
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
    return IaMemsConversationRouting::hasDirectWireColourEvidence(text);
}

bool hasDirectConnectorEvidence(const QString &text)
{
    static const QRegularExpression connectorRx(
        QStringLiteral("\\bC[0-9]{2,4}-[0-9]{1,3}\\b"),
        QRegularExpression::CaseInsensitiveOption);
    return connectorRx.match(text).hasMatch();
}

bool hasPinoutEvidence(const QString &text)
{
    return hasDirectConnectorEvidence(text)
        || text.contains(QStringLiteral("wiring"))
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

QString firstVisualReferenceLine(const QString &statement)
{
    const QStringList lines = statement.split(
        QRegularExpression(QStringLiteral("[\\r\\n]+")), Qt::SkipEmptyParts);
    for (const QString &raw : lines) {
        QString probe = raw.trimmed();
        probe.replace(QStringLiteral("\\:"), QStringLiteral(":"));
        probe.replace(QStringLiteral("\\_"), QStringLiteral("_"));
        if (probe.contains(QRegularExpression(
                QStringLiteral("(?:rave\\s*:|images[\\\\/])"),
                QRegularExpression::CaseInsensitiveOption)))
            return raw.trimmed();
    }
    return QString();
}

int focusedKnowledgeLineScore(const QString &line,
                              const QStringList &specificTerms,
                              KnowledgeQueryKind queryKind,
                              bool asksTorque)
{
    const QString text = normalized(line);
    int score = 0;
    for (const QString &term : specificTerms) {
        if (knowledgeTermMatches(text, term))
            score += term.size() >= 5 ? 8 : 5;
    }
    if (queryKind == KnowledgeQueryKind::Pinout && hasDirectConnectorEvidence(line))
        score += 30;
    if (asksTorque && IaMemsConversationRouting::hasTorqueEvidence(text))
        score += 24;
    if (line.size() <= 260)
        score += 3;
    return score;
}

QString focusedKnowledgeStatement(const QString &statement,
                                  const QStringList &specificTerms,
                                  KnowledgeQueryKind queryKind,
                                  bool asksTorque,
                                  bool procedureIntent)
{
    const QString trimmed = statement.trimmed();
    if (trimmed.size() <= 420 || procedureIntent)
        return trimmed;

    const QStringList lines = trimmed.split(
        QRegularExpression(QStringLiteral("[\\r\\n]+")), Qt::SkipEmptyParts);
    QString best;
    int bestScore = -1;
    for (int i = 0; i < lines.size(); ++i) {
        const QString line = lines.at(i).trimmed();
        const int score = focusedKnowledgeLineScore(line, specificTerms, queryKind, asksTorque);
        if (score > bestScore) {
            bestScore = score;
            best = line;
        }
        if (i + 1 < lines.size()
            && line.size() + lines.at(i + 1).trimmed().size() <= 420) {
            const QString pair = line + QLatin1Char(' ') + lines.at(i + 1).trimmed();
            const int pairScore = focusedKnowledgeLineScore(pair, specificTerms, queryKind, asksTorque);
            if (pairScore > bestScore) {
                bestScore = pairScore;
                best = pair;
            }
        }
    }
    if (bestScore < 8 || best.isEmpty())
        return trimmed;
    const QString visual = firstVisualReferenceLine(trimmed);
    if (!visual.isEmpty() && !best.contains(visual))
        best += QLatin1Char('\n') + visual;
    return best;
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

struct KnowledgeScopeRequest
{
    QString induction;
    QString market;
    QString transmission;
    bool highCompression = false;
};

KnowledgeScopeRequest knowledgeScopeRequest(const QString &questionText)
{
    KnowledgeScopeRequest request;
    if (containsWord(questionText, QStringLiteral("spi"))
        || questionText.contains(QStringLiteral("monopoint")))
        request.induction = QStringLiteral("spi");
    else if (containsWord(questionText, QStringLiteral("mpi"))
             || questionText.contains(QStringLiteral("multipoint")))
        request.induction = QStringLiteral("mpi");

    if (questionText.contains(QStringLiteral("japon"))
        || questionText.contains(QStringLiteral("japan")))
        request.market = QStringLiteral("japan");
    else if (containsWord(questionText, QStringLiteral("uk"))
             || questionText.contains(QStringLiteral("royaume uni"))
             || questionText.contains(QStringLiteral("britannique")))
        request.market = QStringLiteral("uk");
    else if (questionText.contains(QStringLiteral("europe"))
             || questionText.contains(QStringLiteral("europeen")))
        request.market = QStringLiteral("europe");

    if (questionText.contains(QStringLiteral("automatique"))
        || questionText.contains(QStringLiteral("automatic")))
        request.transmission = QStringLiteral("automatic");
    else if (questionText.contains(QStringLiteral("manuelle"))
             || containsWord(questionText, QStringLiteral("manual")))
        request.transmission = QStringLiteral("manual");

    request.highCompression = questionText.contains(QStringLiteral("haute compression"))
        || questionText.contains(QStringLiteral("high compression"));
    return request;
}

bool factMatchesScopeRequest(const ExpertFact &fact,
                             const KnowledgeScopeRequest &request,
                             const QString &questionText)
{
    const QString structuredScope = normalized(fact.notes);
    const QString primaryScope = normalized(QStringLiteral("%1 %2 %3 %4")
        .arg(fact.factKey, fact.topic, fact.statement, fact.firmware));
    if (!IaMemsConversationRouting::explicitVariantLabelsCompatible(questionText, primaryScope))
        return false;

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

bool scopeQualifierTerm(const QString &term)
{
    return IaMemsConversationRouting::isKnowledgeContextQualifier(term);
}

bool genericIntentTerm(const QString &term, KnowledgeQueryKind kind)
{
    if (kind == KnowledgeQueryKind::Pinout) {
        return term == QStringLiteral("broche") || term == QStringLiteral("broches")
            || term == QStringLiteral("pinout") || term == QStringLiteral("connecteur")
            || term == QStringLiteral("connector") || term == QStringLiteral("cablage")
            || term == QStringLiteral("wiring") || term == QStringLiteral("pin")
            || term == QStringLiteral("prise");
    }
    if (kind == KnowledgeQueryKind::WireColor) {
        return term == QStringLiteral("couleur") || term == QStringLiteral("color")
            || term == QStringLiteral("colour") || term == QStringLiteral("wire")
            || term == QStringLiteral("wiring") || term == QStringLiteral("fil")
            || term == QStringLiteral("fils") || term == QStringLiteral("sonde")
            || term == QStringLiteral("capteur");
    }
    return false;
}

int scopeMatchBonus(const ExpertFact &fact, const KnowledgeScopeRequest &request)
{
    const QString scope = normalized(fact.notes);
    if (!scope.contains(QStringLiteral("portee")))
        return 0;
    int bonus = 0;
    if (!request.induction.isEmpty() && containsWord(scope, request.induction))
        bonus += 4;
    if (request.market == QStringLiteral("japan")
        && (scope.contains(QStringLiteral("japan")) || scope.contains(QStringLiteral("japon"))))
        bonus += 4;
    if (request.transmission == QStringLiteral("automatic")
        && (scope.contains(QStringLiteral("automatic")) || scope.contains(QStringLiteral("automatique"))))
        bonus += 3;
    if (request.transmission == QStringLiteral("manual")
        && (containsWord(scope, QStringLiteral("manual")) || scope.contains(QStringLiteral("manuelle"))))
        bonus += 3;
    if (request.highCompression && scope.contains(QStringLiteral("high compression")))
        bonus += 3;
    return bonus;
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
                QString fallback;
                if (I18n::language().trimmed().toLower() == QStringLiteral("fr"))
                    fallback = m_pendingGrounding.trimmed();
                if (fallback.isEmpty())
                    fallback = I18n::text(99023);
                m_pendingGrounding.clear();
                emit responseReady(fallback);
                emit systemMessage(I18n::text(99024).arg(message));
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
        m_knowledgeError = I18n::text(99033);
        emit systemMessage(I18n::text(99034));
        emit statusChanged();
        return;
    }

    if (!m_reader.openReadOnly(path)) {
        m_knowledgeError = m_reader.lastError();
        emit systemMessage(I18n::text(99035));
        emit statusChanged();
        return;
    }

    m_databasePath = path;
    m_knowledgeReady = true;
    m_engine.setKnowledgeReader(&m_reader);
    emit systemMessage(I18n::text(99021));
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

    QString fallback;
    if (I18n::language().trimmed().toLower() == QStringLiteral("fr"))
        fallback = m_pendingGrounding.trimmed();
    if (fallback.isEmpty())
        fallback = I18n::text(99022);
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
        parts << I18n::text(99009);
    else if (m_knowledgeAttempted)
        parts << I18n::text(99010);
    else
        parts << I18n::text(99011);

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

    if (IaMemsConversationRouting::isDocumentationQuestion(question)) {
        if (IaMemsConversationRouting::isReferenceSheetRequest(question)) {
            const QString generation = IaMemsConversationRouting::requestedGeneration(question);
            return QStringLiteral("La fiche XML MEMS %1 est disponible dans le package local. Utilisez le bouton proposé pour l'ouvrir.")
                .arg(generation);
        }
        const QString documentaryKnowledge = knowledgeAnswer(question);
        if (!documentaryKnowledge.isEmpty())
            return documentaryKnowledge;
        return QStringLiteral("Je n\'ai pas trouvé de donnée documentaire vérifiée correspondant exactement à cette demande. Je peux préciser la recherche si vous me donnez le véhicule, la génération MEMS ou la variante SPi/MPi lorsqu\'elle est pertinente.");
    }

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
    const bool explicitEct = containsWord(questionText, QStringLiteral("ect"));
    const bool explicitIat = containsWord(questionText, QStringLiteral("iat"));
    const bool asksTorque = containsAny(questionText, {
        QStringLiteral("couple"), QStringLiteral("serrage"), QStringLiteral("torque")
    });
    const bool procedureIntent = containsAny(questionText, {
        QStringLiteral("depose"), QStringLiteral("repose"), QStringLiteral("procedure"),
        QStringLiteral("remove"), QStringLiteral("refit"), QStringLiteral("remplacement")
    });
    const QStringList terms = knowledgeTerms(question);
    if (terms.isEmpty())
        return QString();

    QStringList specificTerms;
    for (const QString &term : terms) {
        if (!scopeQualifierTerm(term) && !genericIntentTerm(term, queryKind))
            specificTerms.append(term);
    }
    if (specificTerms.isEmpty())
        specificTerms = terms;

    struct RankedFact {
        ExpertFact fact;
        int score = 0;
        int matches = 0;
    };

    const bool sourceDetails = asksForSourceDetails(question);
    const KnowledgeScopeRequest requestedScope = knowledgeScopeRequest(questionText);
    const QList<ExpertFact> facts = m_reader.facts(m_context);
    QVector<RankedFact> ranked;
    ranked.reserve(facts.size());

    for (const ExpertFact &fact : facts) {
        if (!factMatchesScopeRequest(fact, requestedScope, questionText))
            continue;

        const bool foundationFact = normalized(fact.notes).contains(QStringLiteral("portee"));
        const QString identity = normalized(QStringLiteral("%1 %2 %3")
                                                .arg(fact.factKey, fact.topic, fact.firmware));
        const QString body = foundationFact
            ? normalized(QStringLiteral("%1 %2").arg(fact.statement, fact.family))
            : normalized(QStringLiteral("%1 %2 %3").arg(fact.statement, fact.notes, fact.family));
        const QString searchable = identity + QLatin1Char(' ') + body;
        const QString directEvidence = normalized(QStringLiteral("%1 %2 %3")
                                                   .arg(fact.factKey, fact.topic, fact.statement));

        if (explicitEct && !knowledgeTermMatches(searchable, QStringLiteral("ect")))
            continue;
        if (explicitIat && !knowledgeTermMatches(searchable, QStringLiteral("iat")))
            continue;

        if (queryKind == KnowledgeQueryKind::WireColor && !hasWireColorEvidence(fact.statement))
            continue;
        if (queryKind == KnowledgeQueryKind::Pinout && !hasPinoutEvidence(searchable))
            continue;
        if (asksTorque && !IaMemsConversationRouting::hasTorqueEvidence(directEvidence))
            continue;

        RankedFact candidate;
        candidate.fact = fact;

        // Scope words are deliberately not sufficient to make a result relevant.
        // At least one component/topic term must match first; scope is then used
        // only to reject incompatibilities and to prefer an exact proved scope.
        for (const QString &term : specificTerms) {
            if (knowledgeTermMatches(identity, term)) {
                candidate.score += 5;
                ++candidate.matches;
            } else if (knowledgeTermMatches(body, term)) {
                candidate.score += 2;
                ++candidate.matches;
            }
        }
        if (candidate.matches == 0)
            continue;

        candidate.score += qMin(candidate.matches, 4) * 3;
        candidate.score += verificationScore(fact.verificationLevel);
        candidate.score += scopeMatchBonus(fact, requestedScope);
        if (queryKind == KnowledgeQueryKind::WireColor)
            candidate.score += 24;
        else if (queryKind == KnowledgeQueryKind::Pinout) {
            candidate.score += 16;
            candidate.score += hasDirectConnectorEvidence(fact.statement) ? 30 : -6;
        }
        if (fact.statement.size() > 5000)
            candidate.score -= 16;
        else if (fact.statement.size() > 1800)
            candidate.score -= 8;

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
        if (queryKind == KnowledgeQueryKind::Pinout) {
            if (IaMemsConversationRouting::isMiniSpiMapPinoutQuestion(question))
                return IaMemsConversationRouting::miniSpiMapPinoutAnswer();
            return QStringLiteral(
                "Je n'ai pas de brochage vérifié correspondant exactement à cette demande dans le contexte MEMS sélectionné. Précise le signal, le capteur ou le connecteur recherché ; je ne vais pas inventer une broche.");
        }
        return QString();
    }

    std::sort(ranked.begin(), ranked.end(), [](const RankedFact &left, const RankedFact &right) {
        if (left.score != right.score)
            return left.score > right.score;
        if (left.matches != right.matches)
            return left.matches > right.matches;
        return left.fact.factKey < right.fact.factKey;
    });

    QVector<RankedFact> uniqueRanked;
    uniqueRanked.reserve(ranked.size());
    QSet<QString> seenStatements;
    for (const RankedFact &candidate : ranked) {
        const QString signature =
            IaMemsConversationRouting::knowledgeStatementSignature(candidate.fact.statement);
        if (signature.trimmed().isEmpty() || seenStatements.contains(signature))
            continue;
        seenStatements.insert(signature);
        uniqueRanked.append(candidate);
    }
    ranked = uniqueRanked;

    const bool lambdaWireColour = queryKind == KnowledgeQueryKind::WireColor
        && (questionText.contains(QStringLiteral("lambda"))
            || questionText.contains(QStringLiteral("oxygen"))
            || questionText.contains(QStringLiteral("o2")));
    const int maximum = qMin(asksTorque ? 2
                                 : (queryKind == KnowledgeQueryKind::General ? 2 : 4),
                             ranked.size());
    if (maximum == 1 && !lambdaWireColour) {
        const ExpertFact &fact = ranked.constFirst().fact;
        QString answer = focusedKnowledgeStatement(
            fact.statement, specificTerms, queryKind, asksTorque, procedureIntent);
        if (queryKind == KnowledgeQueryKind::WireColor
            && (questionText.contains(QStringLiteral("lambda")) || questionText.contains(QStringLiteral("oxygen")))
            && normalized(fact.statement).contains(QStringLiteral("relais"))) {
            answer.prepend(QStringLiteral("La base ne donne pas directement les couleurs des fils de la sonde lambda pour ce contexte. Fait de câblage vérifié le plus proche :\n"));
        }
        if (!fact.verificationLevel.trimmed().isEmpty())
            answer += QStringLiteral("\nNiveau de preuve : %1.").arg(verificationLabel(fact.verificationLevel));
        if (sourceDetails && !fact.sourceKey.trimmed().isEmpty())
            answer += QStringLiteral("\nSource : %1.").arg(fact.sourceKey.trimmed());
        if (sourceDetails && !fact.notes.trimmed().isEmpty())
            answer += QStringLiteral("\nNote : %1").arg(fact.notes.trimmed());
        return answer;
    }

    QStringList answers;
    if (lambdaWireColour) {
        bool topFactsAreSpiJapan = true;
        bool hasSignalColour = false;
        bool hasRelayColour = false;
        bool hasShieldColour = false;
        for (int i = 0; i < maximum; ++i) {
            const QString statement = normalized(ranked.at(i).fact.statement);
            const bool spiJapan = containsWord(statement, QStringLiteral("spi"))
                && (statement.contains(QStringLiteral("japan")) || statement.contains(QStringLiteral("japon")));
            if (!spiJapan)
                topFactsAreSpiJapan = false;
            const QString role = IaMemsConversationRouting::wireColourRoleLabel(ranked.at(i).fact.statement);
            if (role == QStringLiteral("Signal sonde"))
                hasSignalColour = true;
            else if (role == QStringLiteral("Commande relais/chauffage"))
                hasRelayColour = true;
            else if (role == QStringLiteral("Blindage / masse écran"))
                hasShieldColour = true;
        }
        answers << QStringLiteral("Couleurs de fil vérifiées disponibles :");
        if (requestedScope.induction.isEmpty() && requestedScope.market.isEmpty() && topFactsAreSpiJapan)
            answers << QStringLiteral("Portée des faits trouvés : Mini SPi Japon 97MY.");
        if (!hasSignalColour && (hasRelayColour || hasShieldColour))
            answers << QStringLiteral("La base ne fournit pas la couleur des deux voies de signal +VE/-VE de la sonde. Les faits ci-dessous concernent uniquement les liaisons directement vérifiées ; je ne complète pas par déduction.");
    } else if (queryKind == KnowledgeQueryKind::WireColor)
        answers << QStringLiteral("Couleurs de fil vérifiées les plus pertinentes :");
    else if (queryKind == KnowledgeQueryKind::Pinout)
        answers << QStringLiteral("Brochage vérifié le plus pertinent :");
    else
        answers << QStringLiteral("Les faits les plus pertinents de la base MEMS sont :");

    for (int i = 0; i < maximum; ++i) {
        const ExpertFact &fact = ranked.at(i).fact;
        const QString focused = focusedKnowledgeStatement(
            fact.statement, specificTerms, queryKind, asksTorque, procedureIntent);
        QString line = QStringLiteral("• %1").arg(focused);
        if (lambdaWireColour) {
            const QString role = IaMemsConversationRouting::wireColourRoleLabel(fact.statement);
            if (!role.isEmpty())
                line = QStringLiteral("• %1 — %2").arg(role, focused);
        }
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
