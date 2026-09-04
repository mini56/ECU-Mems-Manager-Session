#include "iamemstab.h"

#include "mainwindow.h"
#include "memsinterface.h"
#include "expert/IaMemsDiagramCatalog.h"
#include "expert/IaMemsConversationRouting.h"
#include "expert/IaMemsService.h"
#include "database/MemsReferenceDatabase.h"
#include "database/MemsReferenceSheetRenderer.h"

#include <QDateTime>
#include <QDialog>
#include <QFile>
#include <QDialogButtonBox>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollBar>
#include <QSet>
#include <QShowEvent>
#include <QTextBrowser>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>
#include <QVariantMap>
#include <QXmlStreamReader>
#include <QRegularExpression>


namespace {

QString iaReferenceXmlHtml(const QString &path)
{
    return MemsReferenceSheetRenderer::renderFile(
        path, QStringLiteral("Impossible d'ouvrir ou de lire la fiche XML locale."));
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

bool isBareInductionSelection(const QString &raw)
{
    const QString text = IaMemsConversationRouting::normalize(raw);
    return text == QStringLiteral("spi") || text == QStringLiteral("mpi");
}

QString shortMpiAnswer(const QString &raw)
{
    const QString text = IaMemsConversationRouting::normalize(raw);
    if (IaMemsConversationRouting::explicitInduction(raw) != QStringLiteral("MPi"))
        return QString();

    if (text == QStringLiteral("mpi"))
        return QStringLiteral(
            "MPi signifie « Multi Point Injection », c'est-à-dire injection multipoint. "
            "Sur les montages Rover/Mini concernés, plusieurs injecteurs distribuent le carburant, typiquement un injecteur par cylindre.");

    if (text == QStringLiteral("injecteur mpi")
        || text == QStringLiteral("injecteurs mpi")
        || text == QStringLiteral("mpi injecteur")
        || text == QStringLiteral("mpi injecteurs")) {
        return QStringLiteral(
            "Sur un système MPi Rover/Mini MEMS, l'injection est multipoint : plusieurs injecteurs distribuent le carburant, typiquement un par cylindre. "
            "L'ECU commande leur ouverture pour doser le carburant dans l'admission. Si tu cherches le brochage, le câblage, un contrôle ou une procédure précise, indique simplement ce point et je chercherai la donnée correspondante.");
    }
    return QString();
}

} // namespace

IaMemsTab::IaMemsTab(MainWindow *mainWindow, QWidget *parent)
    : QWidget(parent),
      m_mainWindow(mainWindow),
      m_mems(mainWindow ? mainWindow->memsInterface() : nullptr),
      m_service(IaMemsService::instance())
{
    setObjectName(QStringLiteral("ia_mems_tab"));
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QVBoxLayout *root = new QVBoxLayout(this);
    root->setContentsMargins(18, 14, 18, 14);
    root->setSpacing(10);

    QHBoxLayout *header = new QHBoxLayout;
    QLabel *title = new QLabel(QStringLiteral("IA MEMS"), this);
    QFont titleFont = title->font();
    titleFont.setPointSize(qMax(titleFont.pointSize() + 4, 14));
    titleFont.setBold(true);
    title->setFont(titleFont);
    header->addWidget(title);
    header->addStretch(1);

    m_status = new QLabel(this);
    m_status->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    header->addWidget(m_status);
    root->addLayout(header);

    QLabel *subtitle = new QLabel(
        QStringLiteral("Dialogue local avec le moteur expert et la base de connaissances MEMS. "
                       "Les réponses distinguent mesures, hypothèses et niveau de preuve."),
        this);
    subtitle->setWordWrap(true);
    root->addWidget(subtitle);

    QWidget *transcriptPane = new QWidget(this);
    QHBoxLayout *transcriptLayout = new QHBoxLayout(transcriptPane);
    transcriptLayout->setContentsMargins(0, 0, 0, 0);
    transcriptLayout->setSpacing(6);

    m_transcript = new QTextBrowser(transcriptPane);
    m_transcript->setObjectName(QStringLiteral("iaMemsTranscript"));
    m_transcript->setOpenExternalLinks(false);
    m_transcript->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_transcript->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_transcript->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    transcriptLayout->addWidget(m_transcript, 1);

    QScrollBar *transcriptScroll = new QScrollBar(Qt::Vertical, transcriptPane);
    transcriptScroll->setObjectName(QStringLiteral("iaMemsTranscriptScroll"));
    transcriptScroll->setMinimumWidth(18);
    transcriptScroll->setMaximumWidth(18);
    transcriptLayout->addWidget(transcriptScroll);

    QScrollBar *internalScroll = m_transcript->verticalScrollBar();
    QObject::connect(internalScroll, &QScrollBar::rangeChanged,
                     transcriptPane,
                     [internalScroll, transcriptScroll](int minimum, int maximum) {
                         transcriptScroll->setRange(minimum, maximum);
                         transcriptScroll->setPageStep(internalScroll->pageStep());
                         transcriptScroll->setSingleStep(internalScroll->singleStep());
                     });
    QObject::connect(internalScroll, &QScrollBar::valueChanged,
                     transcriptScroll, &QScrollBar::setValue);
    QObject::connect(transcriptScroll, &QScrollBar::valueChanged,
                     internalScroll, &QScrollBar::setValue);
    transcriptScroll->setRange(internalScroll->minimum(), internalScroll->maximum());
    transcriptScroll->setPageStep(internalScroll->pageStep());
    transcriptScroll->setSingleStep(internalScroll->singleStep());

    root->addWidget(transcriptPane, 1);

    m_diagramButton = new QPushButton(this);
    m_diagramButton->setObjectName(QStringLiteral("iaMemsDiagramButton"));
    m_diagramButton->setVisible(false);
    root->addWidget(m_diagramButton, 0, Qt::AlignLeft);

    m_documentButton = new QPushButton(this);
    m_documentButton->setObjectName(QStringLiteral("iaMemsDocumentButton"));
    m_documentButton->setVisible(false);
    root->addWidget(m_documentButton, 0, Qt::AlignLeft);

    QHBoxLayout *input = new QHBoxLayout;
    m_question = new QLineEdit(this);
    m_question->setObjectName(QStringLiteral("iaMemsQuestion"));
    m_question->setPlaceholderText(QStringLiteral(
        "Posez une question : Qu'est-ce que tu vois d'anormal ? Mon avance est-elle normale ? Que sait-on sur AANMP002 ?"));
    m_question->setClearButtonEnabled(true);
    input->addWidget(m_question, 1);

    m_sendButton = new QPushButton(QStringLiteral("Envoyer"), this);
    m_sendButton->setObjectName(QStringLiteral("iaMemsSend"));
    input->addWidget(m_sendButton);
    root->addLayout(input);

    connect(m_sendButton, &QPushButton::clicked,
            this, &IaMemsTab::sendQuestion);
    connect(m_question, &QLineEdit::returnPressed,
            this, &IaMemsTab::sendQuestion);
    connect(m_diagramButton, &QPushButton::clicked,
            this, &IaMemsTab::openSuggestedDiagram);
    connect(m_documentButton, &QPushButton::clicked,
            this, &IaMemsTab::openSuggestedDocument);

    if (m_service) {
        connect(m_service, &IaMemsService::responseReady,
                this, &IaMemsTab::onServiceResponse);
        connect(m_service, &IaMemsService::systemMessage,
                this, &IaMemsTab::onServiceSystemMessage);
        connect(m_service, &IaMemsService::statusChanged,
                this, &IaMemsTab::updateStatus);
    }

    if (m_mems) {
        m_connected = m_mems->isConnected();
        if (m_service)
            m_service->setConnected(m_connected);

        connect(m_mems, &MEMSInterface::dataReady,
                this, &IaMemsTab::captureEcuSample, Qt::QueuedConnection);
        connect(m_mems, &MEMSInterface::injectionLiveSample,
                this, &IaMemsTab::onInjectionSample, Qt::QueuedConnection);
        connect(m_mems, &MEMSInterface::connected,
                this, &IaMemsTab::onEcuConnected, Qt::QueuedConnection);
        connect(m_mems, &MEMSInterface::disconnected,
                this, &IaMemsTab::onEcuDisconnected, Qt::QueuedConnection);
        connect(m_mems, &MEMSInterface::serialInterfaceDetected,
                this,
                [this](const QString &, const QString &, const QString &protocol) {
                    if (!m_service)
                        return;
                    ExpertContext context;
                    if (protocol.contains(QStringLiteral("1.9")))
                        context.family = QStringLiteral("1.9");
                    else if (protocol.contains(QStringLiteral("1.6")))
                        context.family = QStringLiteral("1.6");
                    else if (protocol.contains(QStringLiteral("1.3")))
                        context.family = QStringLiteral("1.3");
                    else if (protocol.contains(QStringLiteral("1.2")))
                        context.family = QStringLiteral("1.2");
                    if (!context.family.isEmpty()) {
                        m_detectedFamily = context.family;
                        context.firmware = m_firmwareIdentifier;
                        m_service->setContext(context);
                    }
                },
                Qt::QueuedConnection);

        connect(m_mems, &MEMSInterface::protocolResponse,
                this,
                [this](quint8 command, const QByteArray &response) {
                    if (command == 0xD0 && !response.isEmpty())
                        m_ecuIdHex = QString::fromLatin1(response.toHex(' ')).toUpper();
                    if (command != 0xD1)
                        return;
                    const QString firmware = printableFirmware(response);
                    if (firmware.isEmpty())
                        return;
                    m_firmwareIdentifier = firmware;
                    if (m_service) {
                        ExpertContext context;
                        context.family = m_detectedFamily;
                        context.firmware = m_firmwareIdentifier;
                        m_service->setContext(context);
                    }
                },
                Qt::QueuedConnection);
    }

    appendSystemMessage(QStringLiteral(
        "Bonjour, je suis IA MEMS, l'assistant intégré à ECU MEMS Manager. "
        "Je peux expliquer le logiciel, les systèmes MEMS et les mesures déjà acquises par le programme. "
        "Je n'envoie aucune commande ECU et je n'invente pas une mesure absente."));

    updateStatus();
}

void IaMemsTab::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);

    // L'onglet est uniquement une vue. Le service IA est unique et appartient
    // à l'application ; il est activé après l'affichage, sans reparentage et
    // sans filtre global sur les événements QWidget.
    if (m_service) {
        IaMemsService *service = m_service;
        QTimer::singleShot(0, service, [service]() {
            service->activate();
        });
    }
}

void IaMemsTab::appendMessage(const QString &speaker, const QString &text)
{
    if (!m_transcript)
        return;

    const QString safeSpeaker = speaker.toHtmlEscaped();
    QString safeText = text.toHtmlEscaped();
    safeText.replace(QLatin1Char('\n'), QStringLiteral("<br>"));

    const QString messageAnchor = QStringLiteral("ia-message-%1")
                  .arg(m_transcript->document()->characterCount());
    m_transcript->append(QStringLiteral("<a name='%1'></a><p><b>%2</b><br>%3</p>")
         .arg(messageAnchor, safeSpeaker, safeText));

    if (QScrollBar *bar = m_transcript->verticalScrollBar()) {
        if (speaker == QStringLiteral("IA MEMS"))
            m_transcript->scrollToAnchor(messageAnchor);
        else
            bar->setValue(bar->maximum());
    }
}

void IaMemsTab::appendSystemMessage(const QString &text)
{
    appendMessage(QStringLiteral("IA MEMS"), text);
}

void IaMemsTab::updateDiagramSuggestion(const QString &question)
{
    m_diagramTitle.clear();
    m_diagramQuestion.clear();

    if (!m_diagramButton)
        return;

    m_diagramButton->setProperty("iaMemsDiagramEvidence", QString());

    const IaMemsDiagramSuggestion suggestion =
        IaMemsDiagramCatalog::suggestionForQuestion(question);
    if (!suggestion.isValid()) {
        m_diagramButton->setVisible(false);
        m_diagramButton->setText(QString());
        return;
    }

    m_diagramTitle = suggestion.key;
    m_diagramQuestion = question;
    m_diagramButton->setText(QStringLiteral("Voir le schéma"));
    m_diagramButton->setVisible(true);
}

void IaMemsTab::openSuggestedDiagram()
{
    if (m_diagramTitle.isEmpty() || m_diagramQuestion.isEmpty())
        return;

    const QString evidence = m_diagramButton
        ? m_diagramButton->property("iaMemsDiagramEvidence").toString()
        : QString();
    const IaMemsDiagramSuggestion suggestion = evidence.trimmed().isEmpty()
        ? IaMemsDiagramCatalog::suggestionForQuestion(m_diagramQuestion)
        : IaMemsDiagramCatalog::suggestionForEvidence(m_diagramQuestion, evidence);
    if (!suggestion.isValid() || suggestion.key != m_diagramTitle) {
        m_diagramTitle.clear();
        m_diagramQuestion.clear();
        if (m_diagramButton) {
            m_diagramButton->setText(QString());
            m_diagramButton->setVisible(false);
        }
        appendSystemMessage(QStringLiteral(
            "Le schéma local proposé n'est plus disponible ou déclaré dans le package."));
        return;
    }

    const QFileInfo fileInfo(suggestion.absolutePath);
    QDialog viewer(this);
    viewer.setObjectName(QStringLiteral("iaMemsDiagramViewer"));
    viewer.setWindowTitle(QStringLiteral("IA MEMS — %1").arg(suggestion.key));
    viewer.resize(qBound(480, width() - 40, 900),
                  qBound(360, height() - 40, 650));

    QVBoxLayout *layout = new QVBoxLayout(&viewer);
    QTextBrowser *browser = new QTextBrowser(&viewer);
    browser->setObjectName(QStringLiteral("iaMemsDiagramBrowser"));
    browser->setOpenExternalLinks(false);
    browser->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    browser->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    const QString localUrl = QUrl::fromLocalFile(fileInfo.canonicalFilePath())
                                 .toString()
                                 .toHtmlEscaped();
    const QString title = suggestion.key.toHtmlEscaped();
    browser->setHtml(QStringLiteral(
        "<style>body{background:#0a1015;color:#dce3e8;font-family:'Segoe UI',Arial,sans-serif;}"
        "h1{color:#ff9828;font-size:16pt;margin:0 0 10px 0;}"
        ".diagram{background:#0d151b;border:1px solid #34414b;padding:10px;text-align:center;}"
        ".diagram img{max-width:100%;height:auto;}</style>"
        "<h1>%1</h1><div class='diagram'><img src='%2'></div>")
        .arg(title, localUrl));
    layout->addWidget(browser, 1);

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Close, &viewer);
    connect(buttons, &QDialogButtonBox::rejected, &viewer, &QDialog::reject);
    layout->addWidget(buttons);
    viewer.exec();
}


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

QString IaMemsTab::resolveInductionFromKnownContext(const QString &question, QString *evidence) const
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

void IaMemsTab::sendQuestion()
{
    if (!m_question || !m_service)
        return;

    const QString question = m_question->text().trimmed();
    if (question.isEmpty())
        return;

    m_question->clear();
    appendMessage(QStringLiteral("Vous"), question);

    QString effectiveQuestion = question;
    const QString explicitSelection = IaMemsConversationRouting::explicitInduction(question);
    const bool bareSelection = isBareInductionSelection(question);

    if (m_pendingClarificationQuestion.isEmpty() && bareSelection && !explicitSelection.isEmpty()) {
        const QString previousQuestion = property("iaLastVariantClarificationQuestion").toString();
        const QString previousSelection = property("iaLastVariantClarificationAnswer").toString();
        if (!previousQuestion.isEmpty()
            && !previousSelection.isEmpty()
            && explicitSelection.compare(previousSelection, Qt::CaseInsensitive) != 0) {
            effectiveQuestion = QStringLiteral("%1 %2")
                                    .arg(previousQuestion, explicitSelection)
                                    .simplified();
            setProperty("iaLastVariantClarificationAnswer", explicitSelection);
            appendMessage(QStringLiteral("IA MEMS"),
                          QStringLiteral("Correction prise en compte : %1. Je reprends la demande précédente avec cette variante.")
                              .arg(explicitSelection));
        }
    } else if (m_pendingClarificationQuestion.isEmpty() && !bareSelection) {
        setProperty("iaLastVariantClarificationQuestion", QVariant());
        setProperty("iaLastVariantClarificationAnswer", QVariant());
    }

    if (!m_pendingClarificationQuestion.isEmpty()) {
        const QString pending = m_pendingClarificationQuestion;
        m_pendingClarificationQuestion.clear();
        if (IaMemsConversationRouting::isSearchDirective(question)
            || IaMemsConversationRouting::isUnknownDirective(question)) {
            if (IaMemsConversationRouting::needsInductionClarification(pending)) {
                QString evidence;
                const QString resolved = resolveInductionFromKnownContext(pending, &evidence);
                if (resolved.isEmpty()) {
                    answerLocally(QStringLiteral("J'ai cherché dans les références ECU réellement disponibles, mais je ne peux pas trancher SPi/MPi sans preuve suffisante. Donne-moi la référence inscrite sur le calculateur, ou l'année et le marché du véhicule."));
                    return;
                }
                effectiveQuestion = QStringLiteral("%1 %2").arg(pending, resolved);
                answerLocally(QStringLiteral("J'ai identifié %1 à partir de la référence ECU/firmware %2 retrouvée dans la base. Je poursuis la recherche initiale.").arg(resolved, evidence));
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
            if (IaMemsConversationRouting::needsInductionClarification(pending)) {
                const QString selected = IaMemsConversationRouting::explicitInduction(question);
                if (!selected.isEmpty()) {
                    setProperty("iaLastVariantClarificationQuestion", pending);
                    setProperty("iaLastVariantClarificationAnswer", selected);
                }
            }
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

    const QString localMpiAnswer = shortMpiAnswer(effectiveQuestion);
    if (!localMpiAnswer.isEmpty()) {
        answerLocally(localMpiAnswer);
        return;
    }

    updateDiagramSuggestion(effectiveQuestion);
    updateDocumentSuggestion(effectiveQuestion);

    if (m_sendButton)
        m_sendButton->setEnabled(false);
    m_question->setEnabled(false);

    m_service->ask(IaMemsConversationRouting::focusedQuestion(effectiveQuestion));
}

void IaMemsTab::onServiceResponse(const QString &text)
{
    appendMessage(QStringLiteral("IA MEMS"), text);

    if (m_service && m_diagramButton) {
        const QString evidenceQuestion =
            m_service->property("iaMemsLastLibraryQuestion").toString().trimmed();
        const QString evidence =
            m_service->property("iaMemsLastLibraryEvidence").toString().trimmed();
        if (!evidenceQuestion.isEmpty() && !evidence.isEmpty()) {
            const IaMemsDiagramSuggestion suggestion =
                IaMemsDiagramCatalog::suggestionForEvidence(evidenceQuestion, evidence);
            if (suggestion.isValid()) {
                m_diagramTitle = suggestion.key;
                m_diagramQuestion = evidenceQuestion;
                m_diagramButton->setProperty("iaMemsDiagramEvidence", evidence);
                m_diagramButton->setText(QStringLiteral("Voir le schéma"));
                m_diagramButton->setVisible(true);
            }
        }
    }

    if (m_sendButton)
        m_sendButton->setEnabled(true);
    if (m_question) {
        m_question->setEnabled(true);
        m_question->setFocus();
    }
    updateStatus();
}

void IaMemsTab::onServiceSystemMessage(const QString &text)
{
    if (!text.trimmed().isEmpty())
        appendSystemMessage(text);
    updateStatus();
}

void IaMemsTab::updateStatus()
{
    if (!m_status)
        return;

    QStringList parts;
    parts << (m_connected ? QStringLiteral("ECU connecté") : QStringLiteral("ECU non connecté"));
    if (m_service)
        parts << m_service->statusText();

    m_status->setText(parts.join(QStringLiteral("  •  ")));
    m_status->setToolTip(m_service ? m_service->lastError() : QString());
}

void IaMemsTab::onEcuConnected()
{
    m_connected = true;
    if (m_service)
        m_service->setConnected(true);
    updateStatus();
}

void IaMemsTab::onEcuDisconnected()
{
    m_connected = false;
    if (m_service)
        m_service->setConnected(false);
    updateStatus();
}

void IaMemsTab::onInjectionSample(double finalMs,
                                  double baseMs,
                                  double transientMs,
                                  quint16,
                                  quint16,
                                  quint8 transientCounter)
{
    m_haveInjection = true;
    m_injectionFinalMs = finalMs;
    m_injectionBaseMs = baseMs;
    m_injectionTransientMs = transientMs;
    m_injectionTransientCounter = transientCounter;
}

void IaMemsTab::captureEcuSample()
{
    if (!m_mems || !m_service)
        return;

    mems_data *data = m_mems->getData();
    if (!data)
        return;

    ExpertObservation observation;
    observation.timestampMs = QDateTime::currentMSecsSinceEpoch();

    observation.values.insert(QStringLiteral("rpm"), data->engine_rpm);
    observation.values.insert(QStringLiteral("coolant_c"), static_cast<int>(data->coolant_temp) - 55.0);
    observation.values.insert(QStringLiteral("intake_air_c"), static_cast<int>(data->intake_air_temp) - 55.0);
    observation.values.insert(QStringLiteral("map_kpa"), data->map_kpa);
    observation.values.insert(QStringLiteral("battery_v"), data->battery_voltage / 10.0);
    observation.values.insert(QStringLiteral("lambda_mv"), data->lambda_voltage * 5.0);
    observation.values.insert(QStringLiteral("short_term_trim_pct"), static_cast<int>(data->short_term_fuel_trim) - 100.0);
    observation.values.insert(QStringLiteral("long_term_trim_raw"), data->long_term_fuel_trim);
    observation.values.insert(QStringLiteral("long_term_trim_pct"), static_cast<int>(data->long_term_fuel_trim) - 128.0);
    observation.values.insert(QStringLiteral("ignition_advance_deg"), data->ignition_advance * 0.5 - 24.0);
    observation.values.insert(QStringLiteral("coil_time_ms"), data->coil_time * 0.002);
    observation.values.insert(QStringLiteral("iac_position"), data->iac_position);
    observation.values.insert(QStringLiteral("idle_error_raw"), data->idle_error);

    const int rawHotIdleError = (static_cast<int>(data->idle_error2) << 8)
                              | static_cast<int>(data->uk10);
    const int hotIdleCorrection = static_cast<int>(data->idle_hot) - 35;
    observation.values.insert(QStringLiteral("idle_error_hot_corrected"),
                              rawHotIdleError - 32768 - hotIdleCorrection);

    observation.values.insert(QStringLiteral("uk3_raw"), data->uk3);
    observation.values.insert(QStringLiteral("closed_loop"), data->closed_loop != 0 ? 1.0 : 0.0);
    observation.values.insert(QStringLiteral("idle_switch_closed"), data->idle_switch == 0 ? 1.0 : 0.0);
    observation.values.insert(QStringLiteral("throttle_pot_raw"), data->throttle_pot);
    observation.values.insert(QStringLiteral("lambda_fault_active"),
                              ((data->dtc2 & 0x04) || (data->dtc2 & 0x08)) ? 1.0 : 0.0);
    observation.values.insert(QStringLiteral("tps_fault_active"),
                              ((data->dtc1 & 0x80) || (data->dtc2 & 0x01)) ? 1.0 : 0.0);
    observation.values.insert(QStringLiteral("fault_mask"),
                              static_cast<double>(static_cast<quint32>(data->dtc0)
                              | (static_cast<quint32>(data->dtc1) << 8)
                              | (static_cast<quint32>(data->dtc2) << 16)));

    if (m_haveInjection) {
        observation.values.insert(QStringLiteral("injection_final_ms"), m_injectionFinalMs);
        observation.values.insert(QStringLiteral("injection_base_ms"), m_injectionBaseMs);
        observation.values.insert(QStringLiteral("injection_transient_ms"), m_injectionTransientMs);
        observation.values.insert(QStringLiteral("injection_transient_state"), m_injectionTransientCounter);
    }

    m_service->addObservation(observation);
}
