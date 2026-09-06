#include "MemsGlobalSearchIndex.h"
#include "../expert/IaMemsService.h"

#include <QApplication>
#include <QCoreApplication>
#include <QEvent>
#include <QLineEdit>
#include <QMetaObject>
#include <QPointer>
#include <QPushButton>
#include <QRegularExpression>
#include <QSet>
#include <QTextBrowser>
#include <QTimer>
#include <QVariantMap>

namespace {

QString normalizedLock(QString input)
{
    input = input.normalized(QString::NormalizationForm_D).toCaseFolded();
    QString out;
    bool space = true;
    for (const QChar ch : input) {
        const QChar::Category category = ch.category();
        if (category == QChar::Mark_NonSpacing || category == QChar::Mark_SpacingCombining || category == QChar::Mark_Enclosing)
            continue;
        if (ch.isLetterOrNumber()) {
            out += ch;
            space = false;
        } else if (!space) {
            out += QLatin1Char(' ');
            space = true;
        }
    }
    return out.simplified();
}

QString contentFieldLock(const QString &content, const QString &field)
{
    const QString prefix = field + QStringLiteral(":");
    for (const QString &rawLine : content.split(QLatin1Char('\n'))) {
        const QString line = rawLine.trimmed();
        if (line.startsWith(prefix, Qt::CaseInsensitive))
            return line.mid(prefix.size()).trimmed();
    }
    return QString();
}

QString cleanLabelLock(QString value)
{
    value = value.simplified();
    value.replace(QLatin1Char('_'), QLatin1Char(' '));
    if (value.size() > 88)
        value = value.left(85) + QStringLiteral("...");
    return value.trimmed();
}

bool internalOrPageLabelLock(const QString &label)
{
    const QString n = normalizedLock(label);
    if (n.isEmpty())
        return true;
    if (n.startsWith(QStringLiteral("know rcl")) || n.startsWith(QStringLiteral("doc rcl"))
        || n.startsWith(QStringLiteral("rev rcl")) || n.startsWith(QStringLiteral("src rcl")))
        return true;
    if (QRegularExpression(QStringLiteral("^rcl[0-9]{4}[a-z]{3}$"), QRegularExpression::CaseInsensitiveOption).match(n).hasMatch())
        return true;
    return QRegularExpression(QStringLiteral("(^| )pdf p? [0-9]+($| )")).match(n).hasMatch()
        || QRegularExpression(QStringLiteral("(^| )page [0-9]+($| )")).match(n).hasMatch();
}

QString polishLabelLock(QString label)
{
    label = cleanLabelLock(label);
    const QString n = normalizedLock(label);
    if (n == QStringLiteral("rear brake drum")) return QStringLiteral("Tambour de frein arrière");
    if (n == QStringLiteral("front brake caliper")) return QStringLiteral("Étrier de frein avant");
    if (n == QStringLiteral("front brake pads")) return QStringLiteral("Plaquettes de frein avant");
    if (n == QStringLiteral("rear brake shoes")) return QStringLiteral("Segments de frein arrière");
    if (n == QStringLiteral("master cylinder")) return QStringLiteral("Maître-cylindre");
    if (n == QStringLiteral("brake servo")) return QStringLiteral("Servocommande de frein");
    if (n == QStringLiteral("handbrake") || n == QStringLiteral("parking brake")) return QStringLiteral("Frein à main");
    if (n == QStringLiteral("purge du circuit des freins")) return QStringLiteral("Purge du circuit des freins");
    if (n == QStringLiteral("contacteur des feux stop")) return QStringLiteral("Contacteur des feux stop");
    if (n == QStringLiteral("tambour de frein arriere")) return QStringLiteral("Tambour de frein arrière");
    if (n == QStringLiteral("contacteur de temoin de defaillance de frein")) return QStringLiteral("Contacteur de témoin de défaillance de frein");
    if (n == QStringLiteral("soupape de tarage")) return QStringLiteral("Soupape de tarage");
    if (n == QStringLiteral("maitre cylindre")) return QStringLiteral("Maître-cylindre");
    if (n == QStringLiteral("pedale de frein")) return QStringLiteral("Pédale de frein");
    if (n == QStringLiteral("plaquettes de frein avant")) return QStringLiteral("Plaquettes de frein avant");
    if (n == QStringLiteral("segments de frein arriere")) return QStringLiteral("Segments de frein arrière");
    if (n == QStringLiteral("ensemble de servocommande")) return QStringLiteral("Servocommande");
    if (n == QStringLiteral("etrier de frein avant")) return QStringLiteral("Étrier de frein avant");
    if (n == QStringLiteral("cylindre de frein arriere")) return QStringLiteral("Cylindre de frein arrière");
    if (n.contains(QStringLiteral("cable de frein a main"))) return QStringLiteral("Frein à main / câble de frein à main");
    if (!label.isEmpty())
        label[0] = label.at(0).toUpper();
    return label;
}

QStringList meaningfulTermsLock(const QString &question)
{
    static const QSet<QString> stop = {
        QStringLiteral("que"), QStringLiteral("quoi"), QStringLiteral("quel"), QStringLiteral("quelle"),
        QStringLiteral("le"), QStringLiteral("la"), QStringLiteral("les"), QStringLiteral("un"), QStringLiteral("une"),
        QStringLiteral("de"), QStringLiteral("du"), QStringLiteral("des"), QStringLiteral("sur"), QStringLiteral("dans"),
        QStringLiteral("pour"), QStringLiteral("avec"), QStringLiteral("est"), QStringLiteral("sais"), QStringLiteral("dire")
    };
    QStringList out;
    for (const QString &word : normalizedLock(question).split(QLatin1Char(' '), Qt::SkipEmptyParts)) {
        if (word.size() < 3 || stop.contains(word))
            continue;
        if (!out.contains(word))
            out.append(word);
    }
    return out;
}

bool broadBrakeLabelLock(const QString &label)
{
    const QString n = normalizedLock(label);
    if (!n.contains(QStringLiteral("frein")))
        return false;
    int broadTerms = 0;
    for (const QString &term : {QStringLiteral("purge"), QStringLiteral("tambour"), QStringLiteral("hydraul"),
                                QStringLiteral("servocommande"), QStringLiteral("frein a main"), QStringLiteral("table des matieres")}) {
        if (n.contains(term))
            ++broadTerms;
    }
    return broadTerms >= 2 || n.contains(QStringLiteral("documentation generale"));
}

struct LockedChoice
{
    QString label;
    QString query;
    bool parent = false;
    bool locked = false;
};

QList<LockedChoice> brakeChapterChoicesLock()
{
    const QStringList headings = {
        QStringLiteral("PURGE DU CIRCUIT DES FREINS"),
        QStringLiteral("CONTACTEUR DES FEUX STOP"),
        QStringLiteral("TAMBOUR DE FREIN ARRIERE"),
        QStringLiteral("CONTACTEUR DE TEMOIN DE DEFAILLANCE DE FREIN"),
        QStringLiteral("SOUPAPE DE TARAGE"),
        QStringLiteral("MAITRE-CYLINDRE"),
        QStringLiteral("PEDALE DE FREIN"),
        QStringLiteral("PLAQUETTES DE FREIN AVANT"),
        QStringLiteral("SEGMENTS DE FREIN ARRIERE"),
        QStringLiteral("ENSEMBLE DE SERVOCOMMANDE"),
        QStringLiteral("ETRIER DE FREIN AVANT"),
        QStringLiteral("CYLINDRE DE FREIN ARRIERE"),
        QStringLiteral("CABLE DE FREIN A MAIN - AVANT")
    };
    QList<LockedChoice> out;
    for (const QString &heading : headings) {
        LockedChoice choice;
        choice.label = polishLabelLock(heading);
        choice.query = heading;
        choice.locked = true;
        out.append(choice);
    }
    return out;
}

QString candidateLabelLock(const QVariantMap &row)
{
    const QString content = row.value(QStringLiteral("content")).toString();
    const QStringList fields = {
        QStringLiteral("procedure_title"), QStringLiteral("operation_title"), QStringLiteral("subject"),
        QStringLiteral("component_name"), QStringLiteral("source_section"), QStringLiteral("topic"),
        QStringLiteral("title_source"), QStringLiteral("operation"), QStringLiteral("name_fr")
    };
    for (const QString &field : fields) {
        const QString value = polishLabelLock(contentFieldLock(content, field));
        if (!value.isEmpty() && !internalOrPageLabelLock(value))
            return value;
    }
    const QString title = polishLabelLock(row.value(QStringLiteral("title")).toString());
    return internalOrPageLabelLock(title) ? QString() : title;
}

QList<LockedChoice> initialChoicesLock(const QString &question)
{
    const QStringList terms = meaningfulTermsLock(question);
    if (terms.size() != 1)
        return QList<LockedChoice>();
    const QString term = terms.first();
    static const QSet<QString> precise = {
        QStringLiteral("ckp"), QStringLiteral("ect"), QStringLiteral("iat"), QStringLiteral("map"),
        QStringLiteral("tps"), QStringLiteral("iac"), QStringLiteral("iacv"), QStringLiteral("ecu"),
        QStringLiteral("spi"), QStringLiteral("mpi"), QStringLiteral("obd"), QStringLiteral("rosco")
    };
    if (precise.contains(term))
        return QList<LockedChoice>();

    const QVariantList rows = MemsGlobalSearchIndex::search(question, QString(), 40);
    if (rows.size() < 10)
        return QList<LockedChoice>();

    QList<LockedChoice> out;
    QSet<QString> seen;
    if (term == QStringLiteral("frein") || term == QStringLiteral("freins")) {
        LockedChoice parent;
        parent.label = QStringLiteral("Freins — documentation générale");
        parent.query = QStringLiteral("FREINS");
        parent.parent = true;
        out.append(parent);
        seen.insert(normalizedLock(parent.label));
    }

    for (const QVariant &item : rows) {
        const QVariantMap row = item.toMap();
        QString label = candidateLabelLock(row);
        if (label.isEmpty())
            continue;
        const QString n = normalizedLock(label);
        if (n == term || n == QStringLiteral("freins") || n == QStringLiteral("brakes")
            || n == QStringLiteral("embrayage") || n == QStringLiteral("clutch")
            || n.contains(QStringLiteral("table des matieres")) || broadBrakeLabelLock(label))
            continue;
        if (seen.contains(n))
            continue;
        seen.insert(n);
        LockedChoice choice;
        choice.label = label;
        choice.query = QStringLiteral("%1 %2").arg(question, label).simplified();
        choice.locked = (term == QStringLiteral("frein") || term == QStringLiteral("freins"));
        out.append(choice);
        if (out.size() >= 5)
            break;
    }
    return out.size() >= 3 ? out : QList<LockedChoice>();
}

QList<LockedChoice> refinementChoicesLock(const QString &query)
{
    const QVariantList rows = MemsGlobalSearchIndex::search(query, QString(), 80);
    if (rows.size() < 2)
        return QList<LockedChoice>();

    QList<LockedChoice> out;
    QSet<QString> seen;
    for (const QVariant &item : rows) {
        const QVariantMap row = item.toMap();
        const QString label = candidateLabelLock(row);
        if (label.isEmpty())
            continue;
        const QString n = normalizedLock(label);
        if (n.isEmpty() || n == normalizedLock(query) || seen.contains(n)
            || n.contains(QStringLiteral("table des matieres")) || broadBrakeLabelLock(label))
            continue;
        seen.insert(n);
        LockedChoice choice;
        choice.label = label;
        choice.query = QStringLiteral("%1 %2").arg(query, label).simplified();
        choice.locked = true;
        out.append(choice);
        if (out.size() >= 12)
            break;
    }
    return out.size() >= 2 ? out : QList<LockedChoice>();
}

void appendTranscriptLock(QTextBrowser *browser, const QString &speaker, const QString &text)
{
    if (!browser)
        return;
    QString safe = text.toHtmlEscaped();
    safe.replace(QLatin1Char('\n'), QStringLiteral("<br>"));
    browser->append(QStringLiteral("<p><b>%1</b><br>%2</p>").arg(speaker.toHtmlEscaped(), safe));
}

class LockedDocumentaryController : public QObject
{
public:
    explicit LockedDocumentaryController(QWidget *tab) : QObject(tab), m_tab(tab)
    {
        if (!m_tab)
            return;
        m_question = m_tab->findChild<QLineEdit*>(QStringLiteral("iaMemsQuestion"));
        m_send = m_tab->findChild<QPushButton*>(QStringLiteral("iaMemsSend"));
        m_transcript = m_tab->findChild<QTextBrowser*>(QStringLiteral("iaMemsTranscript"));
        if (!m_question || !m_send)
            return;

        QObject::disconnect(m_send, nullptr, nullptr, nullptr);
        QObject::disconnect(m_question, nullptr, nullptr, nullptr);
        QObject::connect(m_send, &QPushButton::clicked, this, [this]() { handleSend(); });
        QObject::connect(m_question, &QLineEdit::returnPressed, this, [this]() { handleSend(); });
        m_tab->setProperty("iaDocumentaryClarificationTest", true);
        m_tab->setProperty("iaDocumentaryLockedControllerV1", true);
    }

private:
    void invokeOriginalSend(const QString &question)
    {
        if (!m_tab || !m_question)
            return;
        m_question->setText(question);
        QMetaObject::invokeMethod(m_tab.data(), "sendQuestion", Qt::DirectConnection);
    }

    void sendLocked(const QString &query)
    {
        IaMemsService *service = IaMemsService::instance();
        if (!service || !m_question || !m_send)
            return;
        m_question->clear();
        m_send->setEnabled(false);
        m_question->setEnabled(false);
        service->askWithLibrary(QStringLiteral("[[MEMS_LOCKED]]%1").arg(query.trimmed()));
    }

    void showChoices(const QString &subject, const QList<LockedChoice> &choices)
    {
        m_pendingSubject = subject;
        m_choices = choices;
        m_question->clear();
        QString prompt = QStringLiteral("J'ai trouvé plusieurs sujets correspondant à « %1 ». Lequel recherchez-vous ?").arg(subject);
        for (int i = 0; i < m_choices.size(); ++i)
            prompt += QStringLiteral("\n%1. %2").arg(i + 1).arg(m_choices.at(i).label);
        prompt += QStringLiteral("\nRépondez par le numéro ou précisez directement le sujet.");
        appendTranscriptLock(m_transcript, QStringLiteral("IA MEMS"), prompt);
        m_question->setFocus();
    }

    void handleSend()
    {
        if (!m_question || !m_tab)
            return;
        const QString raw = m_question->text().trimmed();
        if (raw.isEmpty())
            return;

        if (!m_pendingSubject.isEmpty()) {
            appendTranscriptLock(m_transcript, QStringLiteral("Vous"), raw);
            bool ok = false;
            const int selected = raw.toInt(&ok);
            LockedChoice choice;
            bool haveChoice = ok && selected >= 1 && selected <= m_choices.size();
            QString refined;
            if (haveChoice) {
                choice = m_choices.at(selected - 1);
                refined = choice.query;
            } else {
                refined = QStringLiteral("%1 %2").arg(m_pendingSubject, raw).simplified();
            }

            m_pendingSubject.clear();
            m_choices.clear();

            if (haveChoice && (choice.parent || broadBrakeLabelLock(choice.label))) {
                showChoices(QStringLiteral("Freins — documentation générale"), brakeChapterChoicesLock());
                return;
            }

            if (haveChoice && choice.locked) {
                sendLocked(refined);
                return;
            }

            const QList<LockedChoice> next = refinementChoicesLock(refined);
            if (next.size() >= 2) {
                showChoices(haveChoice ? choice.label : refined, next);
                return;
            }
            sendLocked(refined);
            return;
        }

        const QList<LockedChoice> choices = initialChoicesLock(raw);
        if (choices.isEmpty()) {
            invokeOriginalSend(raw);
            return;
        }

        appendTranscriptLock(m_transcript, QStringLiteral("Vous"), raw);
        showChoices(raw, choices);
    }

    QPointer<QWidget> m_tab;
    QPointer<QLineEdit> m_question;
    QPointer<QPushButton> m_send;
    QPointer<QTextBrowser> m_transcript;
    QString m_pendingSubject;
    QList<LockedChoice> m_choices;
};

void patchLockedController(QWidget *tab)
{
    if (!tab || tab->property("iaDocumentaryLockedControllerV1").toBool())
        return;
    new LockedDocumentaryController(tab);
}

class LockedControllerInstaller : public QObject
{
public:
    explicit LockedControllerInstaller(QObject *parent = nullptr) : QObject(parent) {}

    bool eventFilter(QObject *watched, QEvent *event) override
    {
        QWidget *widget = qobject_cast<QWidget*>(watched);
        if (widget && event && (event->type() == QEvent::Show || event->type() == QEvent::Polish)
            && widget->objectName() == QStringLiteral("ia_mems_tab")) {
            QPointer<QWidget> guarded(widget);
            QTimer::singleShot(250, this, [guarded]() { if (guarded) patchLockedController(guarded); });
        }
        return QObject::eventFilter(watched, event);
    }
};

void installLockedDocumentaryController()
{
    QCoreApplication *core = QCoreApplication::instance();
    if (!core)
        return;
    LockedControllerInstaller *installer = new LockedControllerInstaller(core);
    core->installEventFilter(installer);
    QTimer::singleShot(800, installer, [installer]() {
        Q_UNUSED(installer)
        for (QWidget *widget : QApplication::allWidgets()) {
            if (widget && widget->objectName() == QStringLiteral("ia_mems_tab"))
                patchLockedController(widget);
        }
    });
}

} // namespace

Q_COREAPP_STARTUP_FUNCTION(installLockedDocumentaryController)
