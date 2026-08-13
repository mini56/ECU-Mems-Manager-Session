#include "i18n.h"

#include <QAbstractButton>
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QCoreApplication>
#include <QEvent>
#include <QFile>
#include <QGroupBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPlainTextEdit>
#include <QTabWidget>
#include <QTableWidget>
#include <QTextEdit>
#include <QWidget>
#include <QXmlStreamReader>

I18n::I18n(QObject *parent) : QObject(parent) {}

I18n *I18n::instance()
{
    static I18n s_instance;
    return &s_instance;
}

bool I18n::load(const QString &languageCode)
{
    I18n *self = instance();
    self->m_language = languageCode.toLower();
    self->m_dictionary.clear();
    self->m_keyDictionary.clear();

    const QString base = QCoreApplication::applicationDirPath() + "/translations/";

    // Nouveau système : dictionnaire numérique stable (ex. 0001, 1001, 2001...).
    QFile jsonFile(base + self->m_language + ".json");
    if (jsonFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        const QJsonDocument doc = QJsonDocument::fromJson(jsonFile.readAll());
        if (doc.isObject()) {
            const QJsonObject obj = doc.object();
            for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) {
                bool ok = false;
                const int key = it.key().toInt(&ok);
                if (ok && it.value().isString())
                    self->m_keyDictionary.insert(key, it.value().toString());
            }
        }
    }

    // Compatibilité temporaire pendant la migration complète des chaînes.
    const QString tsFile = base + "ECUMemsManager_" + self->m_language + ".ts";
    const bool legacyLoaded = self->loadDictionary(tsFile);

    return !self->m_keyDictionary.isEmpty() || legacyLoaded;
}

bool I18n::loadDictionary(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    QXmlStreamReader xml(&file);
    QString source;
    QString translation;
    bool inMessage = false;

    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement()) {
            if (xml.name() == QStringLiteral("message")) {
                inMessage = true;
                source.clear();
                translation.clear();
            } else if (inMessage && xml.name() == QStringLiteral("source")) {
                source = xml.readElementText(QXmlStreamReader::IncludeChildElements);
            } else if (inMessage && xml.name() == QStringLiteral("translation")) {
                translation = xml.readElementText(QXmlStreamReader::IncludeChildElements);
            }
        } else if (xml.isEndElement() && xml.name() == QStringLiteral("message")) {
            if (!source.isEmpty())
                m_dictionary.insert(source, translation.isEmpty() ? source : translation);
            inMessage = false;
        }
    }

    return !xml.hasError() && !m_dictionary.isEmpty();
}

QString I18n::lookup(const QString &source) const
{
    if (source.isEmpty())
        return source;
    const auto it = m_dictionary.constFind(source);
    return it == m_dictionary.constEnd() || it.value().isEmpty() ? source : it.value();
}

QString I18n::lookupKey(int key) const
{
    const auto it = m_keyDictionary.constFind(key);
    if (it != m_keyDictionary.constEnd() && !it.value().isEmpty())
        return it.value();
    return QString("[%1]").arg(key, 4, 10, QLatin1Char('0'));
}

QString I18n::text(const char *source)
{
    return instance()->lookup(QString::fromUtf8(source));
}

QString I18n::text(const QString &source)
{
    return instance()->lookup(source);
}

QString I18n::text(int key)
{
    return instance()->lookupKey(key);
}

QString I18n::language()
{
    return instance()->m_language;
}

void I18n::install(QApplication *app)
{
    if (!app)
        return;
    app->installEventFilter(instance());
}

void I18n::apply(QWidget *root)
{
    if (!root)
        return;
    I18n *self = instance();
    if (self->m_applying)
        return;
    self->m_applying = true;
    self->applyObject(root);
    const QList<QObject*> objects = root->findChildren<QObject*>();
    for (QObject *object : objects)
        self->applyObject(object);
    self->m_applying = false;
}

void I18n::applyObject(QObject *object)
{
    if (!object)
        return;

    if (QWidget *w = qobject_cast<QWidget*>(object)) {
        if (!w->windowTitle().isEmpty()) w->setWindowTitle(lookup(w->windowTitle()));
        if (!w->toolTip().isEmpty()) w->setToolTip(lookup(w->toolTip()));
        if (!w->statusTip().isEmpty()) w->setStatusTip(lookup(w->statusTip()));
        if (!w->whatsThis().isEmpty()) w->setWhatsThis(lookup(w->whatsThis()));
    }
    if (QLabel *w = qobject_cast<QLabel*>(object)) w->setText(lookup(w->text()));
    if (QAbstractButton *w = qobject_cast<QAbstractButton*>(object)) w->setText(lookup(w->text()));
    if (QGroupBox *w = qobject_cast<QGroupBox*>(object)) w->setTitle(lookup(w->title()));
    if (QLineEdit *w = qobject_cast<QLineEdit*>(object)) w->setPlaceholderText(lookup(w->placeholderText()));
    if (QTextEdit *w = qobject_cast<QTextEdit*>(object)) w->setPlaceholderText(lookup(w->placeholderText()));
    if (QPlainTextEdit *w = qobject_cast<QPlainTextEdit*>(object)) w->setPlaceholderText(lookup(w->placeholderText()));
    if (QAction *a = qobject_cast<QAction*>(object)) {
        a->setText(lookup(a->text()));
        a->setToolTip(lookup(a->toolTip()));
        a->setStatusTip(lookup(a->statusTip()));
    }
    if (QComboBox *box = qobject_cast<QComboBox*>(object)) {
        for (int i = 0; i < box->count(); ++i)
            box->setItemText(i, lookup(box->itemText(i)));
    }
    if (QTabWidget *tabs = qobject_cast<QTabWidget*>(object)) {
        for (int i = 0; i < tabs->count(); ++i) {
            tabs->setTabText(i, lookup(tabs->tabText(i)));
            tabs->setTabToolTip(i, lookup(tabs->tabToolTip(i)));
        }
    }
    if (QTableWidget *table = qobject_cast<QTableWidget*>(object)) {
        for (int c = 0; c < table->columnCount(); ++c) {
            if (QTableWidgetItem *item = table->horizontalHeaderItem(c))
                item->setText(lookup(item->text()));
        }
        for (int r = 0; r < table->rowCount(); ++r) {
            if (QTableWidgetItem *item = table->verticalHeaderItem(r))
                item->setText(lookup(item->text()));
        }
    }
}

bool I18n::eventFilter(QObject *watched, QEvent *event)
{
    if (!m_applying && watched && (event->type() == QEvent::Show || event->type() == QEvent::Polish)) {
        if (QWidget *w = qobject_cast<QWidget*>(watched))
            apply(w);
    }
    return QObject::eventFilter(watched, event);
}
