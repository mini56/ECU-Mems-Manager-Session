#include "LocalAiClient.h"
#include "i18n.h"

#include <QCoreApplication>
#include <QDate>
#include <QDir>
#include <QFileInfo>
#include <QLocale>
#include <QMetaObject>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QThread>

#include <atomic>
#include <string>

#ifdef MEMS_USE_ONNX_GENAI
#include <ort_genai_c.h>
#endif

namespace {
const int kMaximumTurns = 8;

QString firstExistingFile(const QStringList &paths)
{
    for (const QString &path : paths) {
        if (!path.trimmed().isEmpty() && QFileInfo::exists(path) && QFileInfo(path).isFile())
            return QDir::cleanPath(path);
    }
    return QString();
}

QString firstExistingModelDirectory(const QStringList &paths)
{
    for (const QString &path : paths) {
        if (path.trimmed().isEmpty())
            continue;
        const QFileInfo info(path);
        const QString directory = info.isFile() ? info.absolutePath() : info.absoluteFilePath();
        if (QFileInfo(QDir(directory).filePath(QStringLiteral("genai_config.json"))).isFile())
            return QDir::cleanPath(directory);
    }
    return QString();
}

QString activeLanguageCode()
{
    QString code = I18n::language().trimmed().toLower();
    const QStringList supported = {QStringLiteral("fr"), QStringLiteral("en"), QStringLiteral("es"),
                                   QStringLiteral("it"), QStringLiteral("pt"), QStringLiteral("de")};
    if (!supported.contains(code))
        code = QStringLiteral("fr");
    return code;
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
        if (ch.isLetterOrNumber() || ch == QLatin1Char('.'))
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
    const QString code = activeLanguageCode();

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

bool asksLocation(const QString &question)
{
    const QString plain = normalizedPlainText(question);
    return plain.startsWith(QStringLiteral("ou il est"))
        || plain.startsWith(QStringLiteral("ou est"))
        || plain.startsWith(QStringLiteral("ou se trouve"))
        || plain.contains(QStringLiteral("ou est place"))
        || plain.contains(QStringLiteral("ou est situe"))
        || plain.contains(QStringLiteral("quel emplacement"));
}

bool isMemsDomainQuestion(const QString &question, const QString &grounding)
{
    const QString plain = normalizedPlainText(question);
    return plain.contains(QStringLiteral("mems"))
        || plain.contains(QStringLiteral("ecu"))
        || plain.contains(QStringLiteral("rosco"))
        || plain.contains(QStringLiteral("firmware"))
        || plain.contains(QStringLiteral("iac"))
        || plain.contains(QStringLiteral("lambda"))
        || plain.contains(QStringLiteral("injection"))
        || plain.contains(QStringLiteral("injecteur"))
        || plain.contains(QStringLiteral("spi"))
        || plain.contains(QStringLiteral("map"))
        || plain.contains(QStringLiteral("bobine"))
        || plain.contains(QStringLiteral("dwell"))
        || !grounding.trimmed().isEmpty();
}

QString controlledTechnicalAnswer(const QString &question)
{
    const QString plain = normalizedPlainText(question);
    const QString code = activeLanguageCode();

    static const QRegularExpression iacRx(QStringLiteral("(^|\\s)iacv?(\\s|$)"));
    if (iacRx.match(plain).hasMatch()) {
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

    if (code != QStringLiteral("fr"))
        return QString();

    static const QRegularExpression mapRx(QStringLiteral("(^|\\s)map(\\s|$)"));
    if (mapRx.match(plain).hasMatch()
        && (plain == QStringLiteral("map")
            || plain.contains(QStringLiteral("c est quoi"))
            || plain.contains(QStringLiteral("signifie"))
            || plain.contains(QStringLiteral("capteur"))
            || plain.contains(QStringLiteral("sert"))))
        return QStringLiteral("MAP signifie « Manifold Absolute Pressure ». Le capteur MAP mesure la pression absolue dans le collecteur d'admission. L'ECU utilise notamment cette information pour estimer la charge moteur et adapter ses calculs de gestion moteur. Une valeur MAP réelle n'est disponible ici que lorsque l'ECU la fournit.");

    if (plain.contains(QStringLiteral("injecteur"))
        && (plain == QStringLiteral("injecteur")
            || plain.contains(QStringLiteral("c est quoi"))
            || plain.contains(QStringLiteral("role"))
            || plain.contains(QStringLiteral("sert"))))
        return QStringLiteral("Un injecteur d'essence est une électrovanne commandée par l'ECU qui dose et pulvérise le carburant dans l'admission. Sur un système SPI il y a une injection monopoint centralisée ; sur un système MPI, plusieurs injecteurs alimentent les cylindres. Ce n'est pas un système d'injection d'huile.");

    static const QRegularExpression spiRx(QStringLiteral("(^|\\s)spi(\\s|$)"));
    if (spiRx.match(plain).hasMatch()
        && (plain == QStringLiteral("spi")
            || plain.contains(QStringLiteral("rover"))
            || plain.contains(QStringLiteral("mems"))
            || plain.contains(QStringLiteral("c est quoi"))
            || plain.contains(QStringLiteral("signifie"))))
        return QStringLiteral("Dans le contexte Rover/Mini MEMS, SPI signifie « Single Point Injection », c'est-à-dire injection monopoint. Le dosage de carburant est centralisé au niveau du boîtier papillon, contrairement au MPI où plusieurs injecteurs sont utilisés. SPI ne signifie pas « Signal Pulse Intensité ».");

    if (plain.contains(QStringLiteral("repond en francais"))
        || plain.contains(QStringLiteral("reponds en francais")))
        return QStringLiteral("D'accord. Je répondrai en français.");

    if (plain.contains(QStringLiteral("moteur 4 temps"))
        || plain.contains(QStringLiteral("moteur quatre temps")))
        return QStringLiteral("Un moteur 4 temps réalise successivement l'admission, la compression, la combustion/détente puis l'échappement. Un cycle complet se fait sur deux tours de vilebrequin.");

    if (plain.contains(QStringLiteral("ecu"))
        && (plain.contains(QStringLiteral("role"))
            || plain.contains(QStringLiteral("sert"))
            || plain.contains(QStringLiteral("c est quoi"))))
        return QStringLiteral("L'ECU est le calculateur moteur. Il lit les capteurs, calcule notamment l'injection et l'avance à l'allumage, pilote les actionneurs prévus par la version MEMS et gère les fonctions de diagnostic disponibles. Il ne faut pas lui attribuer de fonctions non documentées pour l'ECU concerné.");

    if ((plain.contains(QStringLiteral("bobine")) || plain.contains(QStringLiteral("dwell")))
        && (plain.contains(QStringLiteral("valeur"))
            || plain.contains(QStringLiteral("reference"))
            || plain.contains(QStringLiteral("normal"))))
        return QStringLiteral("Pour le dwell primaire de bobine, la plage de contrôle retenue dans MEMS Manager est d'environ 1,9 à 3,1 ms vers 14 V. C'est une valeur de référence, pas une mesure du véhicule tant que l'ECU n'est pas connecté.");

    const bool asksMemsTypes = plain.contains(QStringLiteral("type"))
        && plain.contains(QStringLiteral("ecu"))
        && plain.contains(QStringLiteral("mems"));
    if (asksMemsTypes)
        return QStringLiteral("Dans le périmètre d'ECU MEMS Manager, les familles de travail sont MEMS 1.2, 1.3, 1.6 et 1.9. Elles ne partagent pas toutes le même protocole ni la même interface ; les fonctions disponibles dépendent donc de la famille, du calculateur et du firmware réellement identifiés.");

    static const QRegularExpression versionRx(QStringLiteral("(^|\\s)(?:mems\\s*)?(1\\.[2369])($|\\s)"));
    const QRegularExpressionMatch versionMatch = versionRx.match(plain);
    if (versionMatch.hasMatch()) {
        const QString version = versionMatch.captured(2);
        return QStringLiteral("Dans IA MEMS, MEMS %1 désigne une génération du système de gestion moteur Rover/Lucas utilisé sur les véhicules concernés par le projet. Ici, MEMS ne signifie pas « Micro-Electro-Mechanical Systems ». Les capacités exactes dépendent de l'ECU, du firmware et du protocole réellement identifiés.").arg(version);
    }

    return QString();
}

QString controlledFollowUpAnswer(const QString &question,
                                 const QVector<QPair<QString, QString>> &conversation)
{
    if (activeLanguageCode() != QStringLiteral("fr") || conversation.isEmpty() || !asksLocation(question))
        return QString();

    const QPair<QString, QString> &turn = conversation.constLast();
    const QString previous = normalizedPlainText(turn.first + QLatin1Char(' ') + turn.second);
    if (previous.contains(QStringLiteral("iac")) || previous.contains(QStringLiteral("idle air control")))
        return QStringLiteral("Sur un système MEMS, l'organe de régulation de ralenti est monté sur le boîtier papillon ou relié à son circuit de dérivation d'air selon le montage. L'emplacement exact varie avec le moteur et la version MEMS ; donne-moi le véhicule, le moteur ou la référence ECU si tu veux l'emplacement précis.");

    return QString();
}

bool isFollowUpQuestion(const QString &question)
{
    const QString plain = normalizedPlainText(question);
    return asksLocation(question)
        || plain.startsWith(QStringLiteral("et "))
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

QString languageDirective()
{
    const QString code = activeLanguageCode();
    if (code == QStringLiteral("fr"))
        return QStringLiteral("LANGUE OBLIGATOIRE : réponds uniquement en français, sauf si l'utilisateur demande explicitement une autre langue.");
    if (code == QStringLiteral("en"))
        return QStringLiteral("MANDATORY LANGUAGE: answer only in English unless the user explicitly requests another language.");
    if (code == QStringLiteral("es"))
        return QStringLiteral("IDIOMA OBLIGATORIO: responde únicamente en español salvo que el usuario pida explícitamente otro idioma.");
    if (code == QStringLiteral("it"))
        return QStringLiteral("LINGUA OBBLIGATORIA: rispondi solo in italiano salvo richiesta esplicita di un'altra lingua.");
    if (code == QStringLiteral("pt"))
        return QStringLiteral("IDIOMA OBRIGATÓRIO: responde apenas em português, salvo pedido explícito de outro idioma.");
    return QStringLiteral("VERBINDLICHE SPRACHE: antworte nur auf Deutsch, außer der Benutzer verlangt ausdrücklich eine andere Sprache.");
}

bool likelyWrongLanguage(const QString &answer)
{
    if (activeLanguageCode() != QStringLiteral("fr"))
        return false;

    const QString plain = QStringLiteral(" ") + normalizedPlainText(answer) + QStringLiteral(" ");
    const QStringList englishMarkers = {
        QStringLiteral(" the "), QStringLiteral(" this "), QStringLiteral(" that "),
        QStringLiteral(" is "), QStringLiteral(" are "), QStringLiteral(" used "),
        QStringLiteral(" with "), QStringLiteral(" for "), QStringLiteral(" please "),
        QStringLiteral(" refer "), QStringLiteral(" version "), QStringLiteral(" commonly ")
    };
    const QStringList frenchMarkers = {
        QStringLiteral(" le "), QStringLiteral(" la "), QStringLiteral(" les "),
        QStringLiteral(" est "), QStringLiteral(" sont "), QStringLiteral(" une "),
        QStringLiteral(" un "), QStringLiteral(" des "), QStringLiteral(" pour "),
        QStringLiteral(" avec "), QStringLiteral(" dans "), QStringLiteral(" cette ")
    };

    int english = 0;
    int french = 0;
    for (const QString &marker : englishMarkers)
        if (plain.contains(marker)) ++english;
    for (const QString &marker : frenchMarkers)
        if (plain.contains(marker)) ++french;
    return english >= 3 && french <= 1;
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

class LocalAiWorker final : public QObject
{
public:
    explicit LocalAiWorker(QObject *parent = nullptr) : QObject(parent) {}
    ~LocalAiWorker() override
    {
        reset();
#ifdef MEMS_USE_ONNX_GENAI
        OgaShutdown();
#endif
    }

    bool load(const QString &modelPath, QString *error)
    {
#ifdef MEMS_USE_ONNX_GENAI
        reset();
        const QByteArray pathUtf8 = QDir::cleanPath(modelPath).toUtf8();
        if (!check(OgaCreateModel(pathUtf8.constData(), &m_model), error)) {
            reset();
            return false;
        }
        if (!check(OgaCreateTokenizer(m_model, &m_tokenizer), error)) {
            reset();
            return false;
        }
        m_loadedPath = modelPath;
        return true;
#else
        Q_UNUSED(modelPath)
        if (error)
            *error = QStringLiteral("ONNX Runtime GenAI n'est pas activé dans cette compilation.");
        return false;
#endif
    }

    QString generate(const QString &prompt, bool reasoning, QString *error)
    {
#ifdef MEMS_USE_ONNX_GENAI
        if (!m_model || !m_tokenizer) {
            if (error)
                *error = QStringLiteral("Le modèle ONNX n'est pas chargé.");
            return QString();
        }

        m_cancelRequested.store(false, std::memory_order_release);
        OgaSequences *sequences = nullptr;
        OgaGeneratorParams *params = nullptr;
        OgaGenerator *generator = nullptr;
        OgaTokenizerStream *stream = nullptr;

        const auto cleanup = [&]() {
            if (stream) OgaDestroyTokenizerStream(stream);
            if (generator) OgaDestroyGenerator(generator);
            if (params) OgaDestroyGeneratorParams(params);
            if (sequences) OgaDestroySequences(sequences);
        };

        const QByteArray promptUtf8 = prompt.toUtf8();
        if (!check(OgaCreateSequences(&sequences), error)
            || !check(OgaTokenizerEncode(m_tokenizer, promptUtf8.constData(), sequences), error)) {
            cleanup();
            return QString();
        }

        if (OgaSequencesCount(sequences) == 0 || OgaSequencesGetSequenceCount(sequences, 0) == 0) {
            if (error)
                *error = QStringLiteral("Le tokenizer ONNX n'a produit aucun token d'entrée.");
            cleanup();
            return QString();
        }

        const size_t promptTokens = OgaSequencesGetSequenceCount(sequences, 0);
        const int maxNewTokens = reasoning ? 768 : 256;
        if (!check(OgaCreateGeneratorParams(m_model, &params), error)
            || !check(OgaGeneratorParamsSetSearchNumber(params, "max_length", static_cast<double>(promptTokens + maxNewTokens)), error)
            || !check(OgaGeneratorParamsSetSearchNumber(params, "batch_size", 1.0), error)
            || !check(OgaGeneratorParamsSetSearchBool(params, "do_sample", true), error)
            || !check(OgaGeneratorParamsSetSearchNumber(params, "temperature", reasoning ? 0.6 : 0.7), error)
            || !check(OgaGeneratorParamsSetSearchNumber(params, "top_p", reasoning ? 0.95 : 0.8), error)
            || !check(OgaGeneratorParamsSetSearchNumber(params, "top_k", 20.0), error)
            || !check(OgaCreateGenerator(m_model, params, &generator), error)
            || !check(OgaGenerator_AppendTokenSequences(generator, sequences), error)
            || !check(OgaCreateTokenizerStream(m_tokenizer, &stream), error)) {
            cleanup();
            return QString();
        }

        std::string output;
        int generated = 0;
        while (!OgaGenerator_IsDone(generator) && generated < maxNewTokens
               && !m_cancelRequested.load(std::memory_order_acquire)) {
            if (!check(OgaGenerator_GenerateNextToken(generator), error)) {
                cleanup();
                return QString();
            }
            const int32_t *tokens = nullptr;
            size_t tokenCount = 0;
            if (!check(OgaGenerator_GetNextTokens(generator, &tokens, &tokenCount), error)) {
                cleanup();
                return QString();
            }
            if (!tokens || tokenCount == 0)
                break;
            const char *chunk = nullptr;
            if (!check(OgaTokenizerStreamDecode(stream, tokens[0], &chunk), error)) {
                cleanup();
                return QString();
            }
            if (chunk)
                output += chunk;
            ++generated;
        }

        if (m_cancelRequested.load(std::memory_order_acquire)) {
            if (error)
                *error = QStringLiteral("Génération annulée.");
            cleanup();
            return QString();
        }

        const QString answer = QString::fromUtf8(output.data(), static_cast<int>(output.size()));
        cleanup();
        return answer;
#else
        Q_UNUSED(prompt)
        Q_UNUSED(reasoning)
        if (error)
            *error = QStringLiteral("ONNX Runtime GenAI n'est pas activé dans cette compilation.");
        return QString();
#endif
    }

    void requestCancel() { m_cancelRequested.store(true, std::memory_order_release); }

    void reset()
    {
        m_cancelRequested.store(true, std::memory_order_release);
#ifdef MEMS_USE_ONNX_GENAI
        if (m_tokenizer) {
            OgaDestroyTokenizer(m_tokenizer);
            m_tokenizer = nullptr;
        }
        if (m_model) {
            OgaDestroyModel(m_model);
            m_model = nullptr;
        }
#endif
        m_loadedPath.clear();
    }

private:
#ifdef MEMS_USE_ONNX_GENAI
    static bool check(OgaResult *result, QString *error)
    {
        if (!result)
            return true;
        if (error) {
            const char *message = OgaResultGetError(result);
            *error = message ? QString::fromUtf8(message) : QStringLiteral("Erreur ONNX Runtime GenAI sans message.");
        }
        OgaDestroyResult(result);
        return false;
    }
#endif

    std::atomic_bool m_cancelRequested{false};
    QString m_loadedPath;
#ifdef MEMS_USE_ONNX_GENAI
    OgaModel *m_model = nullptr;
    OgaTokenizer *m_tokenizer = nullptr;
#endif
};

LocalAiClient::LocalAiClient(QObject *parent)
    : QObject(parent), m_workerThread(new QThread(this)), m_worker(new LocalAiWorker)
{
    m_worker->moveToThread(m_workerThread);
    connect(m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);
    m_workerThread->start();
}

LocalAiClient::~LocalAiClient()
{
    shutdown();
    if (m_workerThread && m_workerThread->isRunning()) {
        m_workerThread->quit();
        m_workerThread->wait();
    }
    m_worker = nullptr;
}

void LocalAiClient::initialize()
{
    if (m_state == Starting || m_state == Ready || m_state == Busy)
        return;

    discoverAssets();

#ifndef MEMS_USE_ONNX_GENAI
    setState(MissingRuntime, QStringLiteral("Cette compilation ne contient pas ONNX Runtime GenAI."));
    return;
#endif

    if (m_runtimePath.isEmpty()) {
        setState(MissingRuntime, QStringLiteral("Runtime ONNX Runtime GenAI absent du package."));
        return;
    }
    if (m_modelPath.isEmpty()) {
        setState(MissingModel, QStringLiteral("Modèle Qwen ONNX IA MEMS absent du dossier IA."));
        return;
    }
    if (!m_workerThread || !m_workerThread->isRunning() || !m_worker) {
        setState(Error, QStringLiteral("Thread du moteur IA local indisponible."));
        return;
    }

    const quint64 epoch = ++m_epoch;
    const QString modelPath = m_modelPath;
    setState(Starting);

    QMetaObject::invokeMethod(m_worker, [this, epoch, modelPath]() {
        QString error;
        const bool ok = m_worker->load(modelPath, &error);
        QMetaObject::invokeMethod(this, [this, epoch, ok, error]() {
            if (epoch != m_epoch)
                return;
            if (ok)
                setState(Ready);
            else
                setState(Error, QStringLiteral("Chargement ONNX impossible : %1").arg(error));
        }, Qt::QueuedConnection);
    }, Qt::QueuedConnection);
}

void LocalAiClient::discoverAssets()
{
    const QString root = QCoreApplication::applicationDirPath();
    const QProcessEnvironment env = QProcessEnvironment::systemEnvironment();

    QString envRuntime = env.value(QStringLiteral("MEMS_AI_RUNTIME"));
    if (QFileInfo(envRuntime).isDir())
        envRuntime = QDir(envRuntime).filePath(QStringLiteral("onnxruntime-genai.dll"));
    const QString envModel = env.value(QStringLiteral("MEMS_AI_MODEL"));

    m_runtimePath = firstExistingFile({
        envRuntime,
        QDir(root).filePath(QStringLiteral("onnxruntime-genai.dll")),
        QDir(root).filePath(QStringLiteral("ai/onnxruntime-genai.dll"))
    });
    m_modelPath = firstExistingModelDirectory({
        envModel,
        QDir(root).filePath(QStringLiteral("ai/models/qwen3-0.6b-int4")),
        QDir(root).filePath(QStringLiteral("ai/models/ia-mems-onnx")),
        QDir(root).filePath(QStringLiteral("models/qwen3-0.6b-int4"))
    });
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

    const QString controlledFollowUp = controlledFollowUpAnswer(trimmedQuestion, m_conversation);
    if (!controlledFollowUp.isEmpty()) {
        rememberTurn(m_conversation, trimmedQuestion, controlledFollowUp);
        emit responseReady(controlledFollowUp);
        return;
    }

    QString grounding = groundingContext.trimmed();
    if (isGenericGrounding(grounding))
        grounding.clear();

    const bool reasoning = requiresReasoning(trimmedQuestion, grounding);
    if (!reasoning && !grounding.isEmpty()) {
        rememberTurn(m_conversation, trimmedQuestion, grounding);
        emit responseReady(grounding);
        return;
    }

    QString userContent = languageDirective() + QStringLiteral("\n\n") + trimmedQuestion;
    if (isMemsDomainQuestion(trimmedQuestion, grounding)) {
        userContent += QStringLiteral(
            "\n\nDOMAINE OBLIGATOIRE : dans IA MEMS, le terme MEMS désigne par défaut le système de gestion moteur Rover/Lucas/Mini-Rover concerné par ECU MEMS Manager. "
            "Ne l'interprète jamais comme Micro-Electro-Mechanical Systems sauf si l'utilisateur demande explicitement ce domaine. "
            "Pour une information technique Rover/Lucas MEMS, préfère le contexte fourni et reconnais ton incertitude plutôt que d'inventer.");
    }

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

    userContent += reasoning ? QStringLiteral("\n\n/think") : QStringLiteral("\n\n/no_think");

    QString prompt = QStringLiteral("<|im_start|>system\n%1<|im_end|>\n").arg(systemPrompt());
    if (isFollowUpQuestion(trimmedQuestion) && !m_conversation.isEmpty()) {
        const QPair<QString, QString> &turn = m_conversation.constLast();
        prompt += QStringLiteral("<|im_start|>user\n%1<|im_end|>\n").arg(turn.first);
        prompt += QStringLiteral("<|im_start|>assistant\n%1<|im_end|>\n").arg(turn.second);
    }
    prompt += QStringLiteral("<|im_start|>user\n%1<|im_end|>\n<|im_start|>assistant\n").arg(userContent);

    const quint64 epoch = m_epoch;
    setState(Busy);

    QMetaObject::invokeMethod(m_worker, [this, epoch, prompt, reasoning, trimmedQuestion, grounding]() {
        QString generationError;
        const QString rawAnswer = m_worker->generate(prompt, reasoning, &generationError);
        QMetaObject::invokeMethod(this, [this, epoch, rawAnswer, generationError, trimmedQuestion, grounding]() {
            if (epoch != m_epoch)
                return;

            QString answer = cleanModelReply(rawAnswer);
            setState(Ready);
            if (!generationError.isEmpty() && answer.isEmpty()) {
                emit responseError(QStringLiteral("Erreur du moteur d'IA locale ONNX : %1").arg(generationError));
                return;
            }
            if (likelyWrongLanguage(answer))
                answer.clear();
            if (answer.isEmpty() || isQuestionEcho(trimmedQuestion, answer))
                answer = grounding;
            if (answer.isEmpty()) {
                emit responseError(QStringLiteral("Le modèle local n'a pas produit de réponse exploitable dans la langue active."));
                return;
            }
            rememberTurn(m_conversation, trimmedQuestion, answer);
            emit responseReady(answer);
        }, Qt::QueuedConnection);
    }, Qt::QueuedConnection);
}

void LocalAiClient::clearConversation() { m_conversation.clear(); }

void LocalAiClient::shutdown()
{
    ++m_epoch;
    if (m_worker)
        m_worker->requestCancel();
    if (m_worker && m_workerThread && m_workerThread->isRunning()
        && QThread::currentThread() != m_workerThread) {
        QMetaObject::invokeMethod(m_worker, [this]() { m_worker->reset(); }, Qt::BlockingQueuedConnection);
    }
    m_state = NotStarted;
    m_lastError.clear();
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
        if (m_lastError.isEmpty()) return QStringLiteral("erreur IA locale");
        return QStringLiteral("erreur IA locale : %1").arg(m_lastError.simplified().left(140));
    }
    return QStringLiteral("IA locale");
}

QString LocalAiClient::systemPrompt() const
{
    const QString languageCode = activeLanguageCode();
    const QString languageName = activeLanguageName(languageCode);
    const QString runtimeDate = QDate::currentDate().toString(Qt::ISODate);
    return QStringLiteral(
        "You are IA MEMS, the conversational assistant integrated into ECU MEMS Manager. "
        "The active MEMS Manager interface language is %1 (%2). Answer strictly in that language unless the user explicitly asks for another language. "
        "In this application, MEMS means the Rover/Lucas engine-management family by default, never Micro-Electro-Mechanical Systems unless the user explicitly asks about that unrelated field. "
        "The local runtime date of this computer is %3. If the user asks for today's date or day, use this date and do not guess another one. "
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
    text.remove(QRegularExpression(QStringLiteral("<think>.*?</think>"), QRegularExpression::DotMatchesEverythingOption));
    text.replace(QStringLiteral("/no_think"), QString());
    text.replace(QStringLiteral("/think"), QString());
    return text.trimmed();
}
