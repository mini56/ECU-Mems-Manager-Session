#include <QApplication>
#include <QDate>
#include <QEvent>
#include <QKeyEvent>
#include <QLineEdit>
#include <QLocale>
#include <QMouseEvent>
#include <QPushButton>
#include <QScrollBar>
#include <QSet>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTextBrowser>
#include <QTimer>

namespace {

QString normalizeIaText(QString text)
{
    text = text.toLower().simplified();
    const QPair<QChar, QChar> replacements[] = {
        {QChar(0x00E0), QLatin1Char('a')}, {QChar(0x00E2), QLatin1Char('a')},
        {QChar(0x00E4), QLatin1Char('a')}, {QChar(0x00E7), QLatin1Char('c')},
        {QChar(0x00E8), QLatin1Char('e')}, {QChar(0x00E9), QLatin1Char('e')},
        {QChar(0x00EA), QLatin1Char('e')}, {QChar(0x00EB), QLatin1Char('e')},
        {QChar(0x00EE), QLatin1Char('i')}, {QChar(0x00EF), QLatin1Char('i')},
        {QChar(0x00F4), QLatin1Char('o')}, {QChar(0x00F6), QLatin1Char('o')},
        {QChar(0x00F9), QLatin1Char('u')}, {QChar(0x00FB), QLatin1Char('u')},
        {QChar(0x00FC), QLatin1Char('u')}
    };
    for (const auto &replacement : replacements)
        text.replace(replacement.first, replacement.second);
    return text;
}

bool hasAny(const QString &text, std::initializer_list<const char*> words)
{
    for (const char *word : words) {
        if (text.contains(QString::fromLatin1(word)))
            return true;
    }
    return false;
}

void ensureTranscriptScrollBar(QTextBrowser *browser)
{
    if (!browser)
        return;
    browser->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    browser->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    if (QScrollBar *bar = browser->verticalScrollBar()) {
        bar->setMinimumWidth(14);
        bar->show();
    }
}

QString databaseCountAnswer()
{
    const QStringList connections = QSqlDatabase::connectionNames();
    for (const QString &connection : connections) {
        if (!connection.startsWith(QStringLiteral("MEMS_EXPERT_")))
            continue;

        QSqlDatabase db = QSqlDatabase::database(connection, false);
        if (!db.isValid() || !db.isOpen() || db.driverName().compare(QStringLiteral("QSQLITE"), Qt::CaseInsensitive) != 0)
            continue;

        QSqlQuery tables(db);
        if (!tables.exec(QStringLiteral(
                "SELECT name FROM sqlite_master "
                "WHERE type='table' AND name NOT LIKE 'sqlite_%' ORDER BY name")))
            continue;

        QStringList names;
        while (tables.next()) {
            const QString name = tables.value(0).toString();
            if (!name.isEmpty())
                names.append(name);
        }

        qint64 rows = 0;
        int countedTables = 0;
        for (QString name : names) {
            name.replace(QLatin1Char('"'), QStringLiteral("\"\""));
            QSqlQuery count(db);
            if (count.exec(QStringLiteral("SELECT COUNT(*) FROM \"%1\"").arg(name)) && count.next()) {
                rows += count.value(0).toLongLong();
                ++countedTables;
            }
        }

        if (countedTables > 0) {
            return QStringLiteral(
                "La base de connaissances MEMS actuellement ouverte contient %1 tables et %2 lignes enregistrées au total. "
                "Ce total est lu directement dans la base SQLite ; il ne s'agit pas d'une estimation du modèle IA.")
                .arg(countedTables)
                .arg(rows);
        }
    }

    return QStringLiteral(
        "La base MEMS n'est pas encore ouverte dans cette session, donc je ne vais pas inventer un nombre. "
        "Ouvrez IA MEMS jusqu'à voir « base prête », puis reposez la question.");
}

class IaMemsQualityPatch final : public QObject
{
public:
    explicit IaMemsQualityPatch(QObject *parent = nullptr) : QObject(parent) {}

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if (QTextBrowser *browser = qobject_cast<QTextBrowser*>(watched)) {
            if (browser->objectName() == QStringLiteral("iaMemsTranscript")
                && (event->type() == QEvent::Show
                    || event->type() == QEvent::Polish
                    || event->type() == QEvent::Resize)) {
                ensureTranscriptScrollBar(browser);
            }
        }

        if (QLineEdit *line = qobject_cast<QLineEdit*>(watched)) {
            if (line->objectName() == QStringLiteral("iaMemsQuestion") && event->type() == QEvent::KeyPress) {
                QKeyEvent *key = static_cast<QKeyEvent*>(event);
                if ((key->key() == Qt::Key_Return || key->key() == Qt::Key_Enter)
                    && handleKnownQuestion(line)) {
                    return true;
                }
            }
        }

        if (QPushButton *button = qobject_cast<QPushButton*>(watched)) {
            if (button->objectName() == QStringLiteral("iaMemsSend")) {
                if (event->type() == QEvent::MouseButtonPress) {
                    QLineEdit *line = button->window()->findChild<QLineEdit*>(QStringLiteral("iaMemsQuestion"));
                    if (line && handleKnownQuestion(line)) {
                        button->setDown(false);
                        m_suppressButtonRelease = true;
                        return true;
                    }
                } else if (event->type() == QEvent::MouseButtonRelease && m_suppressButtonRelease) {
                    button->setDown(false);
                    m_suppressButtonRelease = false;
                    return true;
                } else if (event->type() == QEvent::KeyPress) {
                    QKeyEvent *key = static_cast<QKeyEvent*>(event);
                    if (key->key() == Qt::Key_Return || key->key() == Qt::Key_Enter || key->key() == Qt::Key_Space) {
                        QLineEdit *line = button->window()->findChild<QLineEdit*>(QStringLiteral("iaMemsQuestion"));
                        if (line && handleKnownQuestion(line))
                            return true;
                    }
                }
            }
        }

        return QObject::eventFilter(watched, event);
    }

private:
    bool handleKnownQuestion(QLineEdit *line)
    {
        if (!line)
            return false;
        const QString question = line->text().trimmed();
        if (question.isEmpty())
            return false;

        const QString text = normalizeIaText(question);
        QString answer;
        QString topic;

        const bool creatorQuestion =
            (hasAny(text, {"qui a", "auteur"})
             && hasAny(text, {"cree", "creer", "concu", "developpe", "auteur"})
             && hasAny(text, {"mems manager", "programme", "logiciel"}))
            || (text.contains(QStringLiteral("claude lespagnol")) && m_lastTopic == QStringLiteral("creator"));
        if (creatorQuestion) {
            topic = QStringLiteral("creator");
            answer = QStringLiteral(
                "ECU MEMS Manager a été conçu et développé par Claude Lespagnol. "
                "Andrew Revill, RoverMEMS et les autres documents intégrés sont des références techniques utilisées par le projet ; "
                "ils ne sont pas les concepteurs de MEMS Manager.");
        } else if (hasAny(text, {"quel jour", "quelle date", "date aujourd", "jour sommes", "jour on est"})) {
            topic = QStringLiteral("date");
            const QLocale french(QLocale::French, QLocale::France);
            answer = QStringLiteral("Nous sommes %1.")
                         .arg(french.toString(QDate::currentDate(), QStringLiteral("dddd d MMMM yyyy")));
        } else if ((text.contains(QStringLiteral("combien")) && text.contains(QStringLiteral("base")))
                   || (text == QStringLiteral("combien") && m_lastTopic == QStringLiteral("database"))) {
            topic = QStringLiteral("database");
            answer = databaseCountAnswer();
        } else if (text.contains(QStringLiteral("cable"))
                   && hasAny(text, {"mems manager", "programme", "logiciel", "utiliser"})) {
            topic = QStringLiteral("cable");
            answer = QStringLiteral(
                "MEMS Manager communique par une interface de diagnostic série/USB qui apparaît comme un port COM. "
                "Le programme sait détecter notamment les adaptateurs FTDI FT232, Prolific PL2303, WCH CH340/CH341 et Silicon Labs CP210x. "
                "Le câblage côté ECU dépend de la famille MEMS : il ne faut pas considérer n'importe quel câble OBD/KKL comme automatiquement compatible.");
        } else if (hasAny(text, {"bobine", "dwell"})
                   && hasAny(text, {"bonne valeur", "valeur correcte", "combien", "mini spi", "temps"})) {
            topic = QStringLiteral("coil");
            if (hasAny(text, {"resistance", "capacite", "ohm", "farad"})) {
                answer = QStringLiteral(
                    "Je n'ai pas dans les données validées de MEMS Manager une valeur universelle de résistance ou de capacité pour la bobine de votre Mini SPi. "
                    "Donnez la référence exacte de la bobine si c'est cette caractéristique que vous cherchez ; je ne vais pas inventer une valeur.");
            } else {
                answer = QStringLiteral(
                    "Si vous parlez du temps de charge de bobine (dwell) surveillé par MEMS Manager, la plage de référence utilisée par l'application est d'environ 1,9 à 3,1 ms vers 14 V. "
                    "Cette plage concerne le dwell mesuré par l'ECU, pas la résistance électrique de la bobine.");
            }
        } else if ((text.contains(QStringLiteral("barre")) && hasAny(text, {"scroll", "defile"}))) {
            topic = QStringLiteral("scroll");
            answer = QStringLiteral(
                "Vous avez raison : l'historique IA doit rester consultable. La barre de défilement verticale de cette zone est maintenant forcée visible afin de pouvoir remonter aux anciennes questions et réponses.");
        } else {
            return false;
        }

        QTextBrowser *browser = line->window()->findChild<QTextBrowser*>(QStringLiteral("iaMemsTranscript"));
        if (!browser)
            return false;

        ensureTranscriptScrollBar(browser);
        append(browser, QStringLiteral("Vous"), question);
        append(browser, QStringLiteral("IA MEMS"), answer);
        m_lastTopic = topic;
        line->clear();
        line->setFocus();

        QTimer::singleShot(0, browser, [browser]() {
            ensureTranscriptScrollBar(browser);
            if (QScrollBar *bar = browser->verticalScrollBar())
                bar->setValue(bar->maximum());
        });
        return true;
    }

    static void append(QTextBrowser *browser, const QString &speaker, const QString &text)
    {
        QString safeText = text.toHtmlEscaped();
        safeText.replace(QLatin1Char('\n'), QStringLiteral("<br>"));
        browser->append(QStringLiteral("<p><b>%1</b><br>%2</p>")
                            .arg(speaker.toHtmlEscaped(), safeText));
    }

private:
    QString m_lastTopic;
    bool m_suppressButtonRelease = false;
};

void installIaMemsQualityPatch()
{
    if (!qApp)
        return;
    IaMemsQualityPatch *patch = new IaMemsQualityPatch(qApp);
    qApp->installEventFilter(patch);
}

Q_COREAPP_STARTUP_FUNCTION(installIaMemsQualityPatch)

} // namespace
