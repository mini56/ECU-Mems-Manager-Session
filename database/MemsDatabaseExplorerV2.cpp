#include "MemsGlobalSearchIndex.h"
#include "MemsReferenceDatabase.h"
#include "../i18n.h"

#include <QAbstractScrollArea>
#include <QAbstractItemView>
#include <QApplication>
#include <QColor>
#include <QComboBox>
#include <QCoreApplication>
#include <QDialog>
#include <QDir>
#include <QEvent>
#include <QFile>
#include <QFileInfo>
#include <QFrame>
#include <QGridLayout>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMap>
#include <QPointer>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollBar>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTextBrowser>
#include <QTimer>
#include <QUrl>
#include <QVariantMap>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <QXmlStreamReader>

namespace {

const int RoleSearchRow = Qt::UserRole + 201;

QString normalize(QString text)
{
    text = text.normalized(QString::NormalizationForm_D).toCaseFolded();
    QString out;
    bool spaced = true;
    for (const QChar ch : text) {
        const QChar::Category cat = ch.category();
        if (cat == QChar::Mark_NonSpacing || cat == QChar::Mark_SpacingCombining || cat == QChar::Mark_Enclosing)
            continue;
        if (ch.isLetterOrNumber() || ch == QLatin1Char('.') || ch == QLatin1Char('_')) {
            out += ch;
            spaced = false;
        } else if (!spaced) {
            out += QLatin1Char(' ');
            spaced = true;
        }
    }
    return out.simplified();
}

bool forbidden(const QString &text)
{
    QString token = text.toUpper();
    token.remove(QRegularExpression(QStringLiteral("[^A-Z0-9]")));
    return token.contains(QStringLiteral("MEMSFCR"));
}

QString fieldValue(const QString &content, const QString &field)
{
    const QString prefix = field + QStringLiteral(": ");
    for (const QString &line : content.split(QLatin1Char('\n'), Qt::SkipEmptyParts))
        if (line.startsWith(prefix, Qt::CaseInsensitive)) return line.mid(prefix.size()).trimmed();
    return QString();
}

QMap<QString, QMap<QString, QString>> parsedLocalizedFields(const QString &content)
{
    QMap<QString, QMap<QString, QString>> fields;
    static const QRegularExpression suffix(QStringLiteral("^(.*)_(fr|en|es|it|pt|de)$"), QRegularExpression::CaseInsensitiveOption);
    for (const QString &line : content.split(QLatin1Char('\n'), Qt::SkipEmptyParts)) {
        const int sep = line.indexOf(QStringLiteral(": "));
        if (sep <= 0) continue;
        const QString name = line.left(sep).trimmed();
        const QString value = line.mid(sep + 2).trimmed();
        if (value.isEmpty() || forbidden(value)) continue;
        const QRegularExpressionMatch match = suffix.match(name);
        if (match.hasMatch()) fields[match.captured(1).toLower()][match.captured(2).toLower()] = value;
        else fields[name.toLower()][QStringLiteral("raw")] = value;
    }
    return fields;
}

QString localizedValue(const QMap<QString, QString> &variants)
{
    const QString lang = I18n::language().left(2).toLower();
    if (variants.contains(lang)) return variants.value(lang);
    if (variants.contains(QStringLiteral("en"))) return variants.value(QStringLiteral("en"));
    if (variants.contains(QStringLiteral("fr"))) return variants.value(QStringLiteral("fr"));
    return variants.value(QStringLiteral("raw"));
}

int connectorTitleKey(const QString &content)
{
    const QString token = (fieldValue(content, QStringLiteral("relative_path")) + QLatin1Char(' ') + fieldValue(content, QStringLiteral("filename"))).toLower();
    if (token.contains(QStringLiteral("mems_1_2_ecu_connector.svg"))) return 7480;
    if (token.contains(QStringLiteral("mems_1_3_ecu_connectors.svg"))) return 7481;
    if (token.contains(QStringLiteral("mems_1_6_ecu_connector.svg"))) return 7482;
    if (token.contains(QStringLiteral("mems_1_9_ecu_connector.svg"))) return 7483;
    if (token.contains(QStringLiteral("rover_rosco_3pin_black.svg"))) return 7484;
    if (token.contains(QStringLiteral("mems_1_9_obd_16pin.svg"))) return 7485;
    return 0;
}

QString assetPath(const QString &content)
{
    QString relative = fieldValue(content, QStringLiteral("relative_path"));
    if (relative.isEmpty()) relative = fieldValue(content, QStringLiteral("filename"));
    relative = QDir::cleanPath(relative).replace(QLatin1Char('\\'), QLatin1Char('/'));
    if (relative.isEmpty() || relative == QStringLiteral("..") || relative.startsWith(QStringLiteral("../")) || relative.startsWith(QLatin1Char('/')))
        return QString();
    if (!relative.startsWith(QStringLiteral("images/"))) relative = QStringLiteral("images/") + QFileInfo(relative).fileName();
    return QCoreApplication::applicationDirPath() + QStringLiteral("/database/reference/") + relative;
}

int categoryKey(const QString &category)
{
    if (category == QStringLiteral("wiring")) return 7498;
    if (category == QStringLiteral("dtc")) return 7499;
    if (category == QStringLiteral("command")) return 7500;
    if (category == QStringLiteral("data")) return 7501;
    if (category == QStringLiteral("fitment")) return 7502;
    if (category == QStringLiteral("capability")) return 7503;
    if (category == QStringLiteral("ecu")) return 7505;
    if (category == QStringLiteral("actuator")) return 7509;
    if (category == QStringLiteral("protocol")) return 7510;
    return 7504;
}

QString htmlStyle()
{
    return QStringLiteral(
        "<style>body{background:#0a1015;color:#dce5ea;font-family:'Segoe UI',Arial,sans-serif;font-size:9.5pt;margin:9px;}"
        "h1{color:#ff8a00;font-size:16pt;margin:0 0 7px;}h2{color:#ff9b31;font-size:11pt;border-bottom:1px solid #34414b;padding-bottom:4px;margin-top:13px;}"
        "p{line-height:1.4;color:#c8d3d9}.muted{color:#91a1aa}.box{background:#0d151b;border:1px solid #34414b;padding:8px;}"
        "table{border-collapse:collapse;width:100%;}td,th{border-bottom:1px solid #27333c;padding:5px;text-align:left;vertical-align:top;}th{color:#ff9b31;width:30%;}"
        "img{background:#070b0e;}</style>");
}

QString displayTitle(const QVariantMap &row)
{
    const QString content = row.value(QStringLiteral("content")).toString();
    const int connectorKey = connectorTitleKey(content);
    if (connectorKey > 0) return I18n::text(connectorKey);
    const auto fields = parsedLocalizedFields(content);
    for (const QString &candidate : {QStringLiteral("title"), QStringLiteral("name"), QStringLiteral("label"), QStringLiteral("designation"),
                                    QStringLiteral("function"), QStringLiteral("fonction"), QStringLiteral("description_short"), QStringLiteral("description")}) {
        const QString value = localizedValue(fields.value(candidate));
        if (!value.isEmpty() && !forbidden(value)) return value;
    }
    const QString title = row.value(QStringLiteral("title")).toString().trimmed();
    if (!title.isEmpty() && !forbidden(title)) return title;
    return I18n::text(categoryKey(row.value(QStringLiteral("category")).toString()));
}

const QStringList &wiringWords()
{
    static const QStringList words = {
        QStringLiteral("broche"), QStringLiteral("brochage"), QStringLiteral("cablage"), QStringLiteral("connecteur"), QStringLiteral("prise"),
        QStringLiteral("schema"), QStringLiteral("pin"), QStringLiteral("pins"), QStringLiteral("pinout"), QStringLiteral("connector"), QStringLiteral("wiring"),
        QStringLiteral("diagram"), QStringLiteral("conector"), QStringLiteral("cableado"), QStringLiteral("esquema"), QStringLiteral("connettore"),
        QStringLiteral("cablaggio"), QStringLiteral("pino"), QStringLiteral("cablagem"), QStringLiteral("stecker"), QStringLiteral("pinbelegung"),
        QStringLiteral("verkabelung"), QStringLiteral("schaltplan"), QStringLiteral("diagnostic"), QStringLiteral("diagnostique"), QStringLiteral("diagnostico")
    };
    return words;
}

bool wiringIntent(const QString &query)
{
    for (const QString &token : normalize(query).split(QLatin1Char(' '), Qt::SkipEmptyParts))
        if (wiringWords().contains(token)) return true;
    return false;
}

QString strippedWiringQuery(const QString &query)
{
    QStringList kept;
    for (const QString &token : normalize(query).split(QLatin1Char(' '), Qt::SkipEmptyParts)) {
        if (wiringWords().contains(token)) continue;
        if (QRegularExpression(QStringLiteral("^1\\.[2369]$")).match(token).hasMatch()) continue;
        kept << token;
    }
    return kept.join(QLatin1Char(' '));
}

QString generationInQuery(const QString &query)
{
    const QRegularExpressionMatch match = QRegularExpression(QStringLiteral("(?:^|\\s)(1\\.[2369])(?:\\s|$)")).match(normalize(query));
    return match.hasMatch() ? match.captured(1) : QString();
}

class DirectWheelTextBrowser : public QTextBrowser
{
public:
    explicit DirectWheelTextBrowser(QWidget *parent = nullptr) : QTextBrowser(parent) {}
protected:
    void wheelEvent(QWheelEvent *event) override
    {
        QScrollBar *bar = verticalScrollBar();
        if (bar && bar->maximum() > bar->minimum() && event && !event->angleDelta().isNull()) {
            const int steps = event->angleDelta().y() / 120;
            bar->setValue(bar->value() - steps * qMax(48, bar->singleStep() * 3));
            event->accept();
            return;
        }
        QTextBrowser::wheelEvent(event);
    }
};

class DatabaseExplorerController : public QObject
{
public:
    explicit DatabaseExplorerController(QWidget *browser)
        : QObject(browser), m_browser(browser), m_search(nullptr), m_generation(nullptr), m_results(nullptr), m_detail(nullptr), m_resultCount(nullptr),
          m_sheetButton(nullptr), m_enlargeButton(nullptr), m_statsButton(nullptr), m_debounce(new QTimer(this)), m_currentImage()
    {
        if (!m_browser || !m_reference.open()) return;
        m_search = m_browser->findChild<QLineEdit*>();
        m_generation = m_browser->findChild<QComboBox*>();
        m_results = m_browser->findChild<QTableWidget*>();
        m_detail = m_browser->findChild<QTextBrowser*>();
        if (!m_search || !m_generation || !m_results || !m_detail) return;

        QWidget *searchBox = m_search->parentWidget();
        if (searchBox) {
            for (QLabel *label : searchBox->findChildren<QLabel*>(QString(), Qt::FindDirectChildrenOnly))
                if (!label->property("i18nKey").isValid()) m_resultCount = label;
        }
        for (QPushButton *button : m_browser->findChildren<QPushButton*>())
            if (button->text() == I18n::text(7253)) m_sheetButton = button;

        QObject::disconnect(m_search, nullptr, m_browser, nullptr);
        QObject::disconnect(m_generation, nullptr, m_browser, nullptr);
        QObject::disconnect(m_results, nullptr, m_browser, nullptr);
        if (m_sheetButton) QObject::disconnect(m_sheetButton, nullptr, m_browser, nullptr);
        m_search->setCompleter(nullptr);

        configureResults();
        configureScrolling();
        compactMetrics();
        addButtons();

        m_debounce->setSingleShot(true);
        m_debounce->setInterval(170);
        connect(m_debounce, &QTimer::timeout, this, [this](){ runSearch(); });
        connect(m_search, &QLineEdit::textChanged, this, [this](const QString &){ m_debounce->start(); });
        connect(m_generation, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int){ runSearch(); });
        connect(m_results, &QTableWidget::itemSelectionChanged, this, [this](){ renderSelected(); });
        if (m_sheetButton) connect(m_sheetButton, &QPushButton::clicked, this, [this](){ openFullXml(); });

        m_browser->installEventFilter(this);
        m_results->viewport()->installEventFilter(this);
        m_detail->viewport()->installEventFilter(this);
        retranslate();
        runSearch();
    }

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if (!event) return QObject::eventFilter(watched, event);
        if (event->type() == QEvent::Wheel) {
            if (m_results && watched == m_results->viewport()) return driveWheel(m_results, static_cast<QWheelEvent*>(event));
            if (m_detail && watched == m_detail->viewport()) return driveWheel(m_detail, static_cast<QWheelEvent*>(event));
        }
        if (watched == m_browser && event->type() == QEvent::LanguageChange)
            QTimer::singleShot(0, this, [this](){ retranslate(); runSearch(); });
        if (watched == m_browser && event->type() == QEvent::Resize && !m_currentImage.isEmpty())
            QTimer::singleShot(0, this, [this](){ renderSelected(); });
        return QObject::eventFilter(watched, event);
    }

private:
    bool driveWheel(QAbstractScrollArea *area, QWheelEvent *event)
    {
        if (!area || !event) return false;
        QScrollBar *bar = area->verticalScrollBar();
        if (!bar || bar->maximum() <= bar->minimum()) return false;
        int amount = 0;
        if (!event->pixelDelta().isNull()) amount = event->pixelDelta().y();
        else if (!event->angleDelta().isNull()) amount = (event->angleDelta().y() / 120) * qMax(54, bar->singleStep() * 3);
        if (amount == 0) return false;
        bar->setValue(bar->value() - amount);
        event->accept();
        return true;
    }

    void configureResults()
    {
        m_results->setColumnCount(3);
        m_results->verticalHeader()->setVisible(false);
        m_results->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
        m_results->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
        m_results->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
        m_results->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_results->setSelectionMode(QAbstractItemView::SingleSelection);
        m_results->setEditTriggers(QAbstractItemView::NoEditTriggers);
        m_results->setAlternatingRowColors(true);
        m_results->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        m_results->setStyleSheet(QStringLiteral(
            "QTableWidget{background:#0a1117;color:#e7edf1;alternate-background-color:#101820;border:1px solid #27323b;outline:0;}"
            "QTableWidget::item{color:#e7edf1;background:transparent;padding:5px;border:0;border-bottom:1px solid #172129;}"
            "QTableWidget::item:selected{background:#3a2614;color:#ffffff;}"
            "QHeaderView::section{background:#141c23;color:#f3f6f8;border:0;border-right:1px solid #29343e;border-bottom:2px solid #ff7a00;padding:5px;font-weight:700;}"));
    }

    void configureScrolling()
    {
        m_results->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        m_detail->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        m_detail->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        if (m_results->verticalScrollBar()) m_results->verticalScrollBar()->setSingleStep(28);
        if (m_detail->verticalScrollBar()) m_detail->verticalScrollBar()->setSingleStep(42);
    }

    void compactMetrics()
    {
        for (QLabel *label : m_browser->findChildren<QLabel*>()) {
            const QVariant key = label->property("i18nKey");
            if (!key.isValid()) continue;
            const int k = key.toInt();
            if (k >= 7168 && k <= 7173 && label->parentWidget()) label->parentWidget()->hide();
        }
    }

    void addButtons()
    {
        QWidget *searchBox = m_search->parentWidget();
        QHBoxLayout *searchLayout = searchBox ? qobject_cast<QHBoxLayout*>(searchBox->layout()) : nullptr;
        if (searchLayout) {
            m_statsButton = new QPushButton(searchBox);
            m_statsButton->setMinimumHeight(30);
            m_statsButton->setStyleSheet(QStringLiteral("QPushButton{background:#182129;color:#e9eef1;border:1px solid #3b4852;border-radius:4px;padding:4px 9px;}QPushButton:hover{border-color:#ff7a00;color:#ffad5c;}"));
            const int insertAt = m_resultCount ? qMax(0, searchLayout->indexOf(m_resultCount)) : searchLayout->count();
            searchLayout->insertWidget(insertAt, m_statsButton);
            connect(m_statsButton, &QPushButton::clicked, this, [this](){ showStats(); });
        }
        if (!m_sheetButton) return;
        QWidget *detailCard = m_sheetButton->parentWidget();
        QVBoxLayout *outer = detailCard ? qobject_cast<QVBoxLayout*>(detailCard->layout()) : nullptr;
        QHBoxLayout *tools = nullptr;
        if (outer) {
            for (int i = 0; i < outer->count(); ++i) {
                QLayoutItem *item = outer->itemAt(i);
                QHBoxLayout *candidate = item ? qobject_cast<QHBoxLayout*>(item->layout()) : nullptr;
                if (candidate && candidate->indexOf(m_sheetButton) >= 0) { tools = candidate; break; }
            }
        }
        if (tools) {
            m_enlargeButton = new QPushButton(detailCard);
            m_enlargeButton->setMinimumHeight(28);
            m_enlargeButton->setStyleSheet(m_sheetButton->styleSheet());
            tools->insertWidget(tools->indexOf(m_sheetButton) + 1, m_enlargeButton);
            connect(m_enlargeButton, &QPushButton::clicked, this, [this](){ openLargeImage(); });
        }
    }

    void retranslate()
    {
        if (m_statsButton) m_statsButton->setText(I18n::text(7490));
        if (m_enlargeButton) m_enlargeButton->setText(I18n::text(7492));
        if (m_sheetButton) m_sheetButton->setText(I18n::text(7253));
        const int keys[] = {7495, 7496, 7497};
        for (int i = 0; i < 3; ++i) {
            QTableWidgetItem *header = m_results->horizontalHeaderItem(i);
            if (!header) { header = new QTableWidgetItem; m_results->setHorizontalHeaderItem(i, header); }
            header->setText(I18n::text(keys[i]));
        }
    }

    QVariantList safeSearch() const
    {
        const QString typed = m_search->text().trimmed();
        const bool wiring = wiringIntent(typed);
        const QString query = wiring ? strippedWiringQuery(typed) : typed;
        const QString category = wiring ? QStringLiteral("wiring") : QString();
        const QVariantList rows = MemsGlobalSearchIndex::search(query, category, 400);
        QString generation = m_generation->currentData().toString();
        const QString inlineGeneration = generationInQuery(typed);
        if (!inlineGeneration.isEmpty()) generation = inlineGeneration;

        QVariantList filtered;
        for (const QVariant &value : rows) {
            const QVariantMap row = value.toMap();
            if (forbidden(row.value(QStringLiteral("title")).toString()) || forbidden(row.value(QStringLiteral("content")).toString())) continue;
            if (!generation.isEmpty() && row.value(QStringLiteral("generation")).toString() != generation) continue;
            filtered << row;
            if (filtered.size() >= 250) break;
        }
        return filtered;
    }

    void runSearch()
    {
        const QVariantList rows = safeSearch();
        m_results->setUpdatesEnabled(false);
        m_results->clearContents();
        m_results->setRowCount(rows.size());
        for (int r = 0; r < rows.size(); ++r) {
            const QVariantMap row = rows.at(r).toMap();
            QTableWidgetItem *title = new QTableWidgetItem(displayTitle(row));
            QTableWidgetItem *generation = new QTableWidgetItem(row.value(QStringLiteral("generation")).toString());
            QTableWidgetItem *type = new QTableWidgetItem(I18n::text(categoryKey(row.value(QStringLiteral("category")).toString())));
            title->setData(RoleSearchRow, row);
            for (QTableWidgetItem *item : {title, generation, type}) {
                item->setForeground(QColor(QStringLiteral("#e7edf1")));
                item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
            }
            m_results->setItem(r, 0, title);
            m_results->setItem(r, 1, generation);
            m_results->setItem(r, 2, type);
        }
        m_results->setUpdatesEnabled(true);
        if (m_resultCount) m_resultCount->setText(I18n::text(7506).arg(rows.size()));
        if (!rows.isEmpty()) {
            m_results->selectRow(0);
            renderSelected();
        } else {
            m_currentImage.clear();
            if (m_enlargeButton) m_enlargeButton->setEnabled(false);
            m_detail->setHtml(htmlStyle() + QStringLiteral("<h1>%1</h1>").arg(I18n::text(7507).toHtmlEscaped()));
        }
    }

    QVariantMap currentRow() const
    {
        if (m_results->currentRow() < 0) return QVariantMap();
        QTableWidgetItem *item = m_results->item(m_results->currentRow(), 0);
        return item ? item->data(RoleSearchRow).toMap() : QVariantMap();
    }

    void renderSelected()
    {
        const QVariantMap row = currentRow();
        if (row.isEmpty()) return;
        const QString content = row.value(QStringLiteral("content")).toString();
        const int connectorKey = connectorTitleKey(content);
        const QString path = connectorKey > 0 ? assetPath(content) : QString();
        if (connectorKey > 0 && QFileInfo::exists(path)) {
            m_currentImage = path;
            if (m_enlargeButton) m_enlargeButton->setEnabled(true);
            const int width = qMax(360, m_detail->viewport()->width() - 50);
            const QString url = QUrl::fromLocalFile(path).toString().toHtmlEscaped();
            const QString generation = row.value(QStringLiteral("generation")).toString().toHtmlEscaped();
            m_detail->setHtml(htmlStyle() + QStringLiteral("<h1>%1</h1><p class='muted'>%2%3</p><div class='box' style='text-align:center'><img src='%4' width='%5'></div>")
                .arg(I18n::text(connectorKey).toHtmlEscaped(), I18n::text(7486).toHtmlEscaped(),
                     generation.isEmpty() ? QString() : QStringLiteral(" — MEMS ") + generation, url).arg(width));
            m_detail->verticalScrollBar()->setValue(0);
            return;
        }

        m_currentImage.clear();
        if (m_enlargeButton) m_enlargeButton->setEnabled(false);
        const auto fields = parsedLocalizedFields(content);
        QString table;
        for (auto it = fields.constBegin(); it != fields.constEnd(); ++it) {
            const QString value = localizedValue(it.value());
            if (value.isEmpty() || forbidden(value)) continue;
            table += QStringLiteral("<tr><th>%1</th><td>%2</td></tr>").arg(it.key().toHtmlEscaped(), value.toHtmlEscaped());
        }
        if (table.isEmpty()) {
            QStringList lines;
            for (const QString &line : content.split(QLatin1Char('\n'), Qt::SkipEmptyParts)) if (!forbidden(line)) lines << line.toHtmlEscaped();
            table = QStringLiteral("<tr><td>%1</td></tr>").arg(lines.join(QStringLiteral("<br>")));
        }
        m_detail->setHtml(htmlStyle() + QStringLiteral("<h1>%1</h1><p class='muted'>%2</p><table>%3</table>")
            .arg(displayTitle(row).toHtmlEscaped(), I18n::text(categoryKey(row.value(QStringLiteral("category")).toString())).toHtmlEscaped(), table));
        m_detail->verticalScrollBar()->setValue(0);
    }

    void showStats()
    {
        QDialog dialog(m_browser);
        dialog.setWindowTitle(I18n::text(7491));
        dialog.setStyleSheet(QStringLiteral("QDialog{background:#090e13;color:#edf3f6;}QLabel{color:#edf3f6;}QFrame{background:#10171d;border:1px solid #34414b;border-radius:5px;}"));
        QGridLayout *grid = new QGridLayout(&dialog);
        grid->setContentsMargins(12, 12, 12, 12);
        const int keys[] = {7168, 7169, 7170, 7171, 7172, 7173};
        for (int i = 0; i < 6; ++i) {
            QString count = QStringLiteral("—");
            for (QLabel *label : m_browser->findChildren<QLabel*>()) {
                if (label->property("i18nKey").toInt() != keys[i] || !label->parentWidget()) continue;
                for (QLabel *sibling : label->parentWidget()->findChildren<QLabel*>(QString(), Qt::FindDirectChildrenOnly))
                    if (sibling != label && !sibling->property("i18nKey").isValid()) count = sibling->text();
            }
            QFrame *box = new QFrame(&dialog);
            QVBoxLayout *layout = new QVBoxLayout(box);
            QLabel *value = new QLabel(count, box);
            value->setAlignment(Qt::AlignCenter);
            value->setStyleSheet(QStringLiteral("color:#ff8a00;font-size:17pt;font-weight:800;"));
            QLabel *name = new QLabel(I18n::text(keys[i]), box);
            name->setAlignment(Qt::AlignCenter);
            name->setWordWrap(true);
            layout->addWidget(value);
            layout->addWidget(name);
            grid->addWidget(box, i / 3, i % 3);
        }
        dialog.resize(680, 250);
        dialog.exec();
    }

    QString xmlHtml(const QString &path, const QString &generation) const
    {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            return htmlStyle() + QStringLiteral("<h1>%1</h1>").arg(I18n::text(7508).toHtmlEscaped());
        QXmlStreamReader xml(&file);
        QString html = htmlStyle() + QStringLiteral("<h1>MEMS %1 — %2</h1>").arg(generation.toHtmlEscaped(), I18n::text(7493).toHtmlEscaped());
        while (!xml.atEnd()) {
            xml.readNext();
            if (!xml.isStartElement()) continue;
            const QString tag = xml.name().toString().toCaseFolded();
            if (tag == QStringLiteral("section")) {
                const QString title = xml.attributes().value(QStringLiteral("titre")).toString().simplified();
                if (forbidden(title)) { xml.skipCurrentElement(); continue; }
                if (!title.isEmpty()) html += QStringLiteral("<h2>%1</h2>").arg(title.toHtmlEscaped());
            } else if (tag == QStringLiteral("ligne")) {
                QStringList cells;
                bool bad = false;
                while (!xml.atEnd()) {
                    xml.readNext();
                    if (xml.isEndElement() && xml.name().toString().compare(QStringLiteral("ligne"), Qt::CaseInsensitive) == 0) break;
                    if (!xml.isStartElement()) continue;
                    const QString child = xml.name().toString().toCaseFolded();
                    const QString text = xml.readElementText(QXmlStreamReader::IncludeChildElements).simplified();
                    if (forbidden(text)) { bad = true; continue; }
                    if (!text.isEmpty() && (child == QStringLiteral("cellule") || child == QStringLiteral("broche") || child == QStringLiteral("fonction") || child == QStringLiteral("couleur"))) cells << text;
                }
                if (!bad && !cells.isEmpty()) {
                    html += QStringLiteral("<table><tr>");
                    for (const QString &cell : cells) html += QStringLiteral("<td>%1</td>").arg(cell.toHtmlEscaped());
                    html += QStringLiteral("</tr></table>");
                }
            } else if (tag == QStringLiteral("p") || tag == QStringLiteral("note") || tag == QStringLiteral("sous-titre") || tag == QStringLiteral("titre")) {
                const QString text = xml.readElementText(QXmlStreamReader::IncludeChildElements).simplified();
                if (text.isEmpty() || forbidden(text)) continue;
                if (tag == QStringLiteral("sous-titre") || tag == QStringLiteral("titre")) html += QStringLiteral("<h2>%1</h2>").arg(text.toHtmlEscaped());
                else html += QStringLiteral("<p>%1</p>").arg(text.toHtmlEscaped());
            }
        }
        return html;
    }

    void openFullXml()
    {
        QString generation = currentRow().value(QStringLiteral("generation")).toString();
        if (generation.isEmpty()) generation = m_generation->currentData().toString();
        if (generation.isEmpty()) return;
        const QString path = m_reference.generationXmlPath(QStringLiteral("MEMS %1").arg(generation));
        if (!QFileInfo::exists(path)) return;
        QDialog dialog(m_browser);
        dialog.setWindowTitle(QStringLiteral("MEMS %1 — %2").arg(generation, I18n::text(7493)));
        dialog.resize(1040, 760);
        dialog.setStyleSheet(QStringLiteral("QDialog{background:#090e13;}QTextBrowser{background:#0a1015;color:#e7edf1;border:1px solid #34414b;}"));
        QVBoxLayout *layout = new QVBoxLayout(&dialog);
        DirectWheelTextBrowser *viewer = new DirectWheelTextBrowser(&dialog);
        viewer->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        viewer->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        viewer->setHtml(xmlHtml(path, generation));
        layout->addWidget(viewer);
        dialog.exec();
    }

    void openLargeImage()
    {
        if (m_currentImage.isEmpty() || !QFileInfo::exists(m_currentImage)) return;
        QDialog dialog(m_browser);
        dialog.setWindowTitle(I18n::text(7492));
        dialog.resize(1180, 760);
        dialog.setStyleSheet(QStringLiteral("QDialog{background:#090e13;}QTextBrowser{background:#070b0e;color:#edf3f6;border:1px solid #34414b;}"));
        QVBoxLayout *layout = new QVBoxLayout(&dialog);
        DirectWheelTextBrowser *viewer = new DirectWheelTextBrowser(&dialog);
        viewer->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        viewer->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        viewer->setHtml(QStringLiteral("<html><body style='background:#070b0e;margin:8px'><img src='%1' width='1500'></body></html>")
                        .arg(QUrl::fromLocalFile(m_currentImage).toString().toHtmlEscaped()));
        layout->addWidget(viewer);
        dialog.exec();
    }

    QPointer<QWidget> m_browser;
    QPointer<QLineEdit> m_search;
    QPointer<QComboBox> m_generation;
    QPointer<QTableWidget> m_results;
    QPointer<QTextBrowser> m_detail;
    QPointer<QLabel> m_resultCount;
    QPointer<QPushButton> m_sheetButton;
    QPointer<QPushButton> m_enlargeButton;
    QPointer<QPushButton> m_statsButton;
    QTimer *m_debounce;
    QString m_currentImage;
    MemsReferenceDatabase m_reference;
};

class DatabaseExplorerInstaller : public QObject
{
public:
    explicit DatabaseExplorerInstaller(QObject *parent = nullptr) : QObject(parent) {}
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        QWidget *widget = qobject_cast<QWidget*>(watched);
        if (widget && event && (event->type() == QEvent::Show || event->type() == QEvent::Polish) &&
            widget->objectName() == QStringLiteral("memsDatabaseBrowser") && !widget->property("cleanDatabaseExplorerV1").toBool()) {
            widget->setProperty("cleanDatabaseExplorerV1", true);
            QPointer<QWidget> guarded(widget);
            QTimer::singleShot(80, this, [guarded](){ if (guarded) new DatabaseExplorerController(guarded); });
        }
        return QObject::eventFilter(watched, event);
    }
};

void installCleanDatabaseExplorer()
{
    QCoreApplication *core = QCoreApplication::instance();
    if (!core) return;
    DatabaseExplorerInstaller *installer = new DatabaseExplorerInstaller(core);
    core->installEventFilter(installer);
}

}

Q_COREAPP_STARTUP_FUNCTION(installCleanDatabaseExplorer)
