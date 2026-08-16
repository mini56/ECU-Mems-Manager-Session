#include "MemsGlobalSearchIndex.h"
#include "../i18n.h"

#include <QApplication>
#include <QComboBox>
#include <QCompleter>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QDialog>
#include <QEvent>
#include <QFile>
#include <QFileInfo>
#include <QFont>
#include <QFrame>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMetaObject>
#include <QPointer>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollBar>
#include <QSet>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QStandardPaths>
#include <QStringListModel>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTextBrowser>
#include <QTimer>
#include <QUrl>
#include <QUuid>
#include <QVBoxLayout>
#include <QVariantMap>
#include <QVector>
#include <QXmlStreamReader>

namespace {

QString normalized(QString input)
{
    input=input.normalized(QString::NormalizationForm_D).toCaseFolded();
    QString out;
    bool space=true;
    for(const QChar ch:input){
        const QChar::Category category=ch.category();
        if(category==QChar::Mark_NonSpacing || category==QChar::Mark_SpacingCombining || category==QChar::Mark_Enclosing)
            continue;
        if(ch.isLetterOrNumber()){
            out+=ch;
            space=false;
        }else if(!space){
            out+=QLatin1Char(' ');
            space=true;
        }
    }
    QStringList words=out.simplified().split(QLatin1Char(' '),Qt::SkipEmptyParts);
    for(QString &word:words){
        bool hasDigit=false;
        for(const QChar c:word){if(c.isDigit()){hasDigit=true;break;}}
        if(hasDigit) word.replace(QLatin1Char('o'),QLatin1Char('0'));
    }
    return words.join(QLatin1Char(' '));
}

QString generationFromText(const QString &raw)
{
    const QString n=normalized(raw);
    if(n.contains(QStringLiteral("1 9"))) return QStringLiteral("1.9");
    if(n.contains(QStringLiteral("1 6"))) return QStringLiteral("1.6");
    if(n.contains(QStringLiteral("1 3"))) return QStringLiteral("1.3");
    if(n.contains(QStringLiteral("1 2"))) return QStringLiteral("1.2");
    return QString();
}

QString removeGeneration(QString raw,const QString &generation)
{
    if(generation.isEmpty()) return raw.trimmed();
    const QString digit=generation.right(1);
    raw.replace(QRegularExpression(QStringLiteral("(?:MEMS\\s*)?1[\\s\\.]%1").arg(digit),QRegularExpression::CaseInsensitiveOption),QStringLiteral(" "));
    return raw.simplified();
}

QString categoryLabel(const QString &category)
{
    if(category==QStringLiteral("ecu")) return I18n::text(7312);
    if(category==QStringLiteral("vehicle")) return I18n::text(7313);
    if(category==QStringLiteral("dtc")) return I18n::text(7314);
    if(category==QStringLiteral("command")) return I18n::text(7315);
    if(category==QStringLiteral("wiring")) return I18n::text(7316);
    if(category==QStringLiteral("protocol")) return I18n::text(7317);
    if(category==QStringLiteral("documentation")) return I18n::text(7318);
    if(category==QStringLiteral("actuator")) return I18n::text(7319);
    if(category==QStringLiteral("data")) return I18n::text(7320);
    if(category==QStringLiteral("setting")) return I18n::text(7272);
    if(category==QStringLiteral("capability")) return I18n::text(7173);
    if(category==QStringLiteral("file")) return I18n::text(7169);
    return I18n::text(7326);
}

QString exactCategoryIntent(const QString &raw)
{
    const QString n=normalized(raw);
    static const QSet<QString> ecu={QStringLiteral("ecu"),QStringLiteral("ecus"),QStringLiteral("calculateur"),QStringLiteral("calculateurs")};
    static const QSet<QString> dtc={QStringLiteral("code"),QStringLiteral("codes"),QStringLiteral("dtc"),QStringLiteral("defaut"),QStringLiteral("defauts"),QStringLiteral("fault"),QStringLiteral("faults")};
    static const QSet<QString> vehicle={QStringLiteral("vehicule"),QStringLiteral("vehicules"),QStringLiteral("vehicle"),QStringLiteral("vehicles")};
    static const QSet<QString> command={QStringLiteral("commande"),QStringLiteral("commandes"),QStringLiteral("command"),QStringLiteral("commands")};
    static const QSet<QString> wiring={QStringLiteral("cablage"),QStringLiteral("brochage"),QStringLiteral("pin"),QStringLiteral("pins"),QStringLiteral("broche"),QStringLiteral("broches"),QStringLiteral("wiring")};
    static const QSet<QString> protocol={QStringLiteral("protocole"),QStringLiteral("protocol")};
    static const QSet<QString> documentation={QStringLiteral("documentation"),QStringLiteral("document"),QStringLiteral("documents"),QStringLiteral("xml"),QStringLiteral("fiche"),QStringLiteral("fiches")};
    static const QSet<QString> actuator={QStringLiteral("actionneur"),QStringLiteral("actionneurs"),QStringLiteral("actuator"),QStringLiteral("actuators")};
    static const QSet<QString> data={QStringLiteral("trame"),QStringLiteral("trames"),QStringLiteral("mesure"),QStringLiteral("mesures"),QStringLiteral("pid"),QStringLiteral("capteur"),QStringLiteral("capteurs")};
    if(ecu.contains(n)) return QStringLiteral("ecu");
    if(dtc.contains(n)) return QStringLiteral("dtc");
    if(vehicle.contains(n)) return QStringLiteral("vehicle");
    if(command.contains(n)) return QStringLiteral("command");
    if(wiring.contains(n)) return QStringLiteral("wiring");
    if(protocol.contains(n)) return QStringLiteral("protocol");
    if(documentation.contains(n)) return QStringLiteral("documentation");
    if(actuator.contains(n)) return QStringLiteral("actuator");
    if(data.contains(n)) return QStringLiteral("data");
    return QString();
}

QString htmlStyle()
{
    return QStringLiteral(
        "<style>body{background:#0a1015;color:#dce3e8;font-family:'Segoe UI',Arial,sans-serif;font-size:9pt;}"
        "h1{color:#ff9828;font-size:16pt;margin:0 0 6px 0;}"
        "h2{color:#ff9828;font-size:10.5pt;border-bottom:1px solid #34414b;padding-bottom:4px;margin-top:14px;}"
        "p{line-height:1.4;margin:5px 0}.muted{color:#98a5af}.note{background:#15100b;border:1px solid #60401f;color:#ffd0a0;padding:7px;}"
        "table{border-collapse:collapse;width:100%;}th{color:#ff9828;text-align:left;border-bottom:1px solid #394650;padding:5px;}"
        "td{border-bottom:1px solid #26323b;padding:5px;vertical-align:top}.content{white-space:pre-wrap;line-height:1.45}</style>");
}

QString swatchHtml(const QStringList &fills,const QString &text)
{
    QString html;
    int used=0;
    QSet<QString> seen;
    for(QString fill:fills){
        fill=fill.trimmed();
        if(fill.isEmpty() || fill.compare(QStringLiteral("none"),Qt::CaseInsensitive)==0 || seen.contains(fill)) continue;
        seen.insert(fill);
        html+=QStringLiteral("<span style='background-color:%1;color:%1;border:1px solid #8a949c;'>██</span>").arg(fill.toHtmlEscaped());
        if(++used>=2) break;
    }
    if(!html.isEmpty()) html+=QStringLiteral("&nbsp;&nbsp;");
    html+=text.trimmed().toHtmlEscaped();
    return html;
}

QString readXmlCell(QXmlStreamReader &xml,const QString &endTag)
{
    QString text;
    QStringList fills;
    while(!xml.atEnd()){
        xml.readNext();
        if(xml.isStartElement()){
            const QStringRef name=xml.name();
            if(name==QStringLiteral("rect") || name==QStringLiteral("path")){
                const QString fill=xml.attributes().value(QStringLiteral("fill")).toString();
                if(!fill.isEmpty()) fills.append(fill);
            }
        }else if(xml.isCharacters() && !xml.isWhitespace()){
            text+=xml.text().toString();
        }else if(xml.isEndElement() && xml.name()==endTag){
            break;
        }
    }
    return swatchHtml(fills,text);
}

QString renderXml(const QString &path)
{
    QFile file(path);
    if(!file.open(QIODevice::ReadOnly|QIODevice::Text))
        return htmlStyle()+QStringLiteral("<p class='note'>%1</p>").arg(I18n::text(7232).toHtmlEscaped());
    QXmlStreamReader xml(&file);
    QString html=htmlStyle();
    bool firstRow=true;
    while(!xml.atEnd()){
        xml.readNext();
        if(xml.isEndElement()){
            if(xml.name()==QStringLiteral("ligne")){html+=QStringLiteral("</tr>");firstRow=false;}
            else if(xml.name()==QStringLiteral("table")) html+=QStringLiteral("</table>");
            continue;
        }
        if(!xml.isStartElement()) continue;
        const QStringRef name=xml.name();
        if(name==QStringLiteral("titre")) html+=QStringLiteral("<h1>%1</h1>").arg(xml.readElementText(QXmlStreamReader::IncludeChildElements).toHtmlEscaped());
        else if(name==QStringLiteral("sous-titre")) html+=QStringLiteral("<p class='muted'>%1</p>").arg(xml.readElementText(QXmlStreamReader::IncludeChildElements).toHtmlEscaped());
        else if(name==QStringLiteral("section")) html+=QStringLiteral("<h2>%1</h2>").arg(xml.attributes().value(QStringLiteral("titre")).toString().toHtmlEscaped());
        else if(name==QStringLiteral("p")){
            QString text=xml.readElementText(QXmlStreamReader::IncludeChildElements).toHtmlEscaped();
            text.replace(QStringLiteral("\n"),QStringLiteral("<br>"));
            html+=QStringLiteral("<p>%1</p>").arg(text);
        }else if(name==QStringLiteral("note")){
            QString text=xml.readElementText(QXmlStreamReader::IncludeChildElements).toHtmlEscaped();
            text.replace(QStringLiteral("\n"),QStringLiteral("<br>"));
            html+=QStringLiteral("<div class='note'>%1</div>").arg(text);
        }else if(name==QStringLiteral("table")){
            firstRow=true;
            html+=QStringLiteral("<table>");
        }else if(name==QStringLiteral("ligne")){
            html+=QStringLiteral("<tr>");
        }else if(name==QStringLiteral("cellule") || name==QStringLiteral("broche") || name==QStringLiteral("fonction") || name==QStringLiteral("couleur")){
            const QString tag=firstRow?QStringLiteral("th"):QStringLiteral("td");
            html+=QStringLiteral("<%1>%2</%1>").arg(tag,readXmlCell(xml,name.toString()));
        }
    }
    return html;
}

QString xmlPath(const QString &generation)
{
    const QString root=QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)+QStringLiteral("/reference/fiches/");
    if(generation==QStringLiteral("1.3")) return root+QStringLiteral("mems_1_3.xml");
    if(generation==QStringLiteral("1.6")) return root+QStringLiteral("mems_1_6.xml");
    if(generation==QStringLiteral("1.9")) return root+QStringLiteral("mems_1_9.xml");
    return QString();
}

QString pdfPath(const QString &generation)
{
    const QString root=QCoreApplication::applicationDirPath()+QStringLiteral("/database/reference/pdf/");
    if(generation==QStringLiteral("1.3")) return root+QStringLiteral("mems1.3_source.pdf");
    if(generation==QStringLiteral("1.6")) return root+QStringLiteral("mems1.6_source.pdf");
    if(generation==QStringLiteral("1.9")) return root+QStringLiteral("mems1.9_source.pdf");
    return QString();
}

QLabel *labelByKey(QWidget *root,int key)
{
    if(!root) return nullptr;
    const QList<QLabel*> labels=root->findChildren<QLabel*>();
    for(QLabel *label:labels){
        if(label && label->property("i18nKey").isValid() && label->property("i18nKey").toInt()==key)
            return label;
    }
    return nullptr;
}

class DatabaseExplorerV2 : public QObject
{
public:
    explicit DatabaseExplorerV2(QWidget *browser)
        : QObject(browser),m_browser(browser),m_search(nullptr),m_filter(nullptr),m_results(nullptr),m_detail(nullptr),
          m_count(nullptr),m_scopeCompact(nullptr),m_sheetButton(nullptr),m_pdfButton(nullptr),m_currentGenerationLabel(nullptr),
          m_debounce(new QTimer(this)),m_completionModel(new QStringListModel(this)),m_completer(nullptr),m_ftsEnabled(false)
    {
        if(!m_browser) return;
        m_search=m_browser->findChild<QLineEdit*>();
        m_filter=m_browser->findChild<QComboBox*>();
        m_results=m_browser->findChild<QTableWidget*>();
        m_detail=m_browser->findChild<QTextBrowser*>();
        locateResultCount();
        if(!m_search||!m_filter||!m_results||!m_detail) return;

        if(!MemsGlobalSearchIndex::ensureBuilt()) return;
        if(!openIndex()) return;

        disconnectLegacyUi();
        compactHeader();
        configureFilter();
        configureResults();
        configureDetail();
        configureDocumentation();
        configureSearch();
        retranslate();
        m_browser->installEventFilter(this);
        QTimer::singleShot(0,this,[this](){runSearch();});
    }

    ~DatabaseExplorerV2() override
    {
        if(m_index.isOpen()) m_index.close();
        const QString connection=m_indexConnection;
        m_index=QSqlDatabase();
        if(!connection.isEmpty()) QSqlDatabase::removeDatabase(connection);
    }

protected:
    bool eventFilter(QObject *watched,QEvent *event) override
    {
        if(watched==m_browser && event){
            if(event->type()==QEvent::LanguageChange)
                QTimer::singleShot(30,this,[this](){retranslate();runSearch();});
            else if(event->type()==QEvent::Resize || event->type()==QEvent::Show)
                QTimer::singleShot(0,this,[this](){forceDetailScroll();});
        }
        return QObject::eventFilter(watched,event);
    }

private:
    bool openIndex()
    {
        m_indexConnection=QStringLiteral("MEMS_DB_UI_V2_%1").arg(QUuid::createUuid().toString());
        m_index=QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"),m_indexConnection);
        m_index.setConnectOptions(QStringLiteral("QSQLITE_OPEN_READONLY"));
        m_index.setDatabaseName(MemsGlobalSearchIndex::indexPath());
        if(!m_index.open()) return false;
        QSqlQuery query(m_index);
        if(query.exec(QStringLiteral("SELECT value FROM search_meta WHERE key='fts5_enabled'")) && query.next())
            m_ftsEnabled=query.value(0).toString()==QStringLiteral("1");
        return true;
    }

    void locateResultCount()
    {
        if(!m_search||!m_search->parentWidget()) return;
        const QList<QLabel*> labels=m_search->parentWidget()->findChildren<QLabel*>(QString(),Qt::FindDirectChildrenOnly);
        if(!labels.isEmpty()) m_count=labels.last();
    }

    void disconnectLegacyUi()
    {
        QObject::disconnect(m_search,nullptr,nullptr,nullptr);
        QObject::disconnect(m_filter,nullptr,nullptr,nullptr);
        QObject::disconnect(m_results,nullptr,nullptr,nullptr);
    }

    void compactHeader()
    {
        QLabel *scopeTitle=labelByKey(m_browser,7162);
        if(scopeTitle && scopeTitle->parentWidget()) scopeTitle->parentWidget()->hide();

        QLabel *title=labelByKey(m_browser,7160);
        if(!title || !title->parentWidget()) return;
        QWidget *hero=title->parentWidget();
        QVBoxLayout *layout=qobject_cast<QVBoxLayout*>(hero->layout());
        if(!layout) return;
        const int index=layout->indexOf(title);
        if(index<0) return;
        layout->removeWidget(title);
        QWidget *row=new QWidget(hero);
        row->setStyleSheet(QStringLiteral("background:transparent;"));
        QHBoxLayout *horizontal=new QHBoxLayout(row);
        horizontal->setContentsMargins(0,0,0,0);
        horizontal->setSpacing(8);
        horizontal->addWidget(title);
        horizontal->addStretch(1);
        m_scopeCompact=new QLabel(row);
        m_scopeCompact->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
        m_scopeCompact->setStyleSheet(QStringLiteral("color:#aeb9c2;background:transparent;border:0;font-weight:600;"));
        horizontal->addWidget(m_scopeCompact);
        layout->insertWidget(index,row);
    }

    void configureFilter()
    {
        m_filter->setMinimumWidth(205);
        m_filter->setMaximumWidth(250);
        rebuildFilterItems(QStringLiteral("all"));
    }

    void rebuildFilterItems(const QString &wanted)
    {
        if(!m_filter) return;
        m_filter->blockSignals(true);
        m_filter->clear();
        m_filter->addItem(I18n::text(7330),QStringLiteral("all"));
        m_filter->addItem(I18n::text(7312),QStringLiteral("cat:ecu"));
        m_filter->addItem(I18n::text(7313),QStringLiteral("cat:vehicle"));
        m_filter->addItem(I18n::text(7314),QStringLiteral("cat:dtc"));
        m_filter->addItem(I18n::text(7315),QStringLiteral("cat:command"));
        m_filter->addItem(I18n::text(7316),QStringLiteral("cat:wiring"));
        m_filter->addItem(I18n::text(7317),QStringLiteral("cat:protocol"));
        m_filter->addItem(I18n::text(7318),QStringLiteral("cat:documentation"));
        m_filter->addItem(I18n::text(7319),QStringLiteral("cat:actuator"));
        m_filter->addItem(I18n::text(7320),QStringLiteral("cat:data"));
        m_filter->insertSeparator(m_filter->count());
        m_filter->addItem(I18n::text(7291),QStringLiteral("gen:1.2"));
        m_filter->addItem(I18n::text(7292),QStringLiteral("gen:1.3"));
        m_filter->addItem(I18n::text(7293),QStringLiteral("gen:1.6"));
        m_filter->addItem(I18n::text(7294),QStringLiteral("gen:1.9"));
        const int index=m_filter->findData(wanted);
        m_filter->setCurrentIndex(index>=0?index:0);
        m_filter->blockSignals(false);
    }

    QString filterData() const
    {
        return m_filter?m_filter->currentData().toString():QStringLiteral("all");
    }

    QString filterCategory() const
    {
        const QString data=filterData();
        return data.startsWith(QStringLiteral("cat:"))?data.mid(4):QString();
    }

    QString filterGeneration() const
    {
        const QString data=filterData();
        return data.startsWith(QStringLiteral("gen:"))?data.mid(4):QString();
    }

    void configureResults()
    {
        m_results->setColumnCount(5);
        m_results->setColumnHidden(3,true);
        m_results->setColumnHidden(4,true);
        m_results->horizontalHeader()->setSectionResizeMode(0,QHeaderView::Stretch);
        m_results->horizontalHeader()->setSectionResizeMode(1,QHeaderView::ResizeToContents);
        m_results->horizontalHeader()->setSectionResizeMode(2,QHeaderView::ResizeToContents);
        connect(m_results,&QTableWidget::itemSelectionChanged,this,[this](){showSelectedResult();});
    }

    void forceDetailScroll()
    {
        if(!m_detail) return;
        m_detail->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        m_detail->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        m_detail->setFocusPolicy(Qt::StrongFocus);
        QScrollBar *bar=m_detail->verticalScrollBar();
        if(!bar) return;
        bar->setFixedWidth(16);
        bar->setStyleSheet(QStringLiteral(
            "QScrollBar:vertical{background:#111a21;width:16px;margin:0;border-left:1px solid #53616c;}"
            "QScrollBar::handle:vertical{background:#7c8b96;min-height:38px;border:1px solid #a0abb3;border-radius:5px;margin:2px;}"
            "QScrollBar::handle:vertical:hover{background:#ff8a22;border-color:#ffad5c;}"
            "QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical{height:0;}"
            "QScrollBar::add-page:vertical,QScrollBar::sub-page:vertical{background:#111a21;}"));
        bar->show();
    }

    void configureDetail()
    {
        forceDetailScroll();
        QTimer::singleShot(100,this,[this](){forceDetailScroll();});
    }

    QString generationLabel(const QString &generation) const
    {
        if(generation==QStringLiteral("1.2")) return I18n::text(7291);
        if(generation==QStringLiteral("1.3")) return I18n::text(7292);
        if(generation==QStringLiteral("1.6")) return I18n::text(7293);
        if(generation==QStringLiteral("1.9")) return I18n::text(7294);
        return QString();
    }

    void configureDocumentation()
    {
        const QList<QPushButton*> buttons=m_browser->findChildren<QPushButton*>();
        for(QPushButton *button:buttons){
            if(!button) continue;
            if(button->text()==I18n::text(7253) || button->text()==I18n::text(7287) || button->text()==I18n::text(7325)) m_sheetButton=button;
            else if(button->text()==I18n::text(7261)) m_pdfButton=button;
        }
        if(m_sheetButton){
            QObject::disconnect(m_sheetButton,nullptr,nullptr,nullptr);
            connect(m_sheetButton,&QPushButton::clicked,this,[this](){openXmlDialog();});
        }
        if(m_pdfButton){
            QObject::disconnect(m_pdfButton,nullptr,nullptr,nullptr);
            connect(m_pdfButton,&QPushButton::clicked,this,[this](){
                const QString path=pdfPath(currentGeneration());
                if(!path.isEmpty()&&QFileInfo::exists(path)) QDesktopServices::openUrl(QUrl::fromLocalFile(path));
            });
        }
        const QList<QLabel*> labels=m_detail->parentWidget()->findChildren<QLabel*>(QString(),Qt::FindDirectChildrenOnly);
        for(QLabel *label:labels){
            if(label && label->text().startsWith(QStringLiteral("MEMS "),Qt::CaseInsensitive)){
                m_currentGenerationLabel=label;
                break;
            }
        }
        updateDocumentControls();
    }

    void configureSearch()
    {
        m_debounce->setSingleShot(true);
        m_debounce->setInterval(170);
        connect(m_debounce,&QTimer::timeout,this,[this](){runSearch();});
        connect(m_search,&QLineEdit::textEdited,this,[this](const QString &){m_debounce->start();});
        connect(m_filter,QOverload<int>::of(&QComboBox::currentIndexChanged),this,[this](int){m_debounce->start();});

        m_completer=new QCompleter(m_completionModel,this);
        m_completer->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
        m_completer->setCaseSensitivity(Qt::CaseInsensitive);
        m_completer->setMaxVisibleItems(8);
        m_search->setCompleter(m_completer);
        connect(m_completer,QOverload<const QString&>::of(&QCompleter::activated),this,[this](const QString &text){
            m_search->setText(text);
            m_search->setCursorPosition(text.size());
            runSearch();
        });
    }

    void retranslate()
    {
        if(m_scopeCompact){
            m_scopeCompact->setText(I18n::text(7162)+QStringLiteral("  ")+I18n::text(7163)+QStringLiteral("  ")+I18n::text(7164)+QStringLiteral("  ")+I18n::text(7165)+QStringLiteral("  ")+I18n::text(7166)+QStringLiteral("     ")+I18n::text(7260));
        }
        if(m_search) m_search->setPlaceholderText(I18n::text(7217));
        const QString current=filterData();
        rebuildFilterItems(current);
        if(m_results){
            const int keys[]={7322,7323,7321};
            for(int i=0;i<3;++i){
                QTableWidgetItem *item=m_results->horizontalHeaderItem(i);
                if(!item){item=new QTableWidgetItem;m_results->setHorizontalHeaderItem(i,item);}
                item->setText(I18n::text(keys[i]));
            }
        }
        if(m_sheetButton) m_sheetButton->setText(I18n::text(7325));
        updateDocumentControls();
    }

    QString ftsExpression(const QString &text) const
    {
        const QStringList terms=normalized(text).split(QLatin1Char(' '),Qt::SkipEmptyParts);
        QStringList query;
        for(QString term:terms){
            term.replace(QLatin1Char('"'),QStringLiteral("\"\""));
            if(!term.isEmpty()) query.append(QStringLiteral("\"%1\"*").arg(term));
        }
        return query.join(QStringLiteral(" AND "));
    }

    QVariantList executeQuery(const QString &text,const QString &category,const QString &generation,int limit) const
    {
        QVariantList rows;
        if(!m_index.isOpen()) return rows;
        const QString cleaned=removeGeneration(text,generation);
        const QString fts=ftsExpression(cleaned);
        QString sql;
        const bool useFts=m_ftsEnabled&&!fts.isEmpty();
        if(useFts){
            sql=QStringLiteral(
                "SELECT d.id,d.category,d.source_table,d.source_key,d.generation,d.title,d.content "
                "FROM search_fts f JOIN search_documents d ON d.id=f.rowid WHERE search_fts MATCH :match");
        }else{
            sql=QStringLiteral("SELECT id,category,source_table,source_key,generation,title,content FROM search_documents WHERE 1=1");
        }
        if(!category.isEmpty()) sql+=QStringLiteral(" AND d.category=:category").replace(QStringLiteral("d."),useFts?QStringLiteral("d."):QString());
        if(!generation.isEmpty()) sql+=QStringLiteral(" AND d.generation LIKE :generation").replace(QStringLiteral("d."),useFts?QStringLiteral("d."):QString());
        if(!useFts && !cleaned.trimmed().isEmpty()){
            const QStringList terms=normalized(cleaned).split(QLatin1Char(' '),Qt::SkipEmptyParts);
            for(int i=0;i<terms.size();++i) sql+=QStringLiteral(" AND normalized LIKE :q%1").arg(i);
        }
        if(useFts) sql+=QStringLiteral(" ORDER BY bm25(search_fts),d.category,d.title");
        else sql+=QStringLiteral(" ORDER BY category,title");
        sql+=QStringLiteral(" LIMIT %1").arg(qBound(1,limit,300));

        QSqlQuery query(m_index);
        if(!query.prepare(sql)) return rows;
        if(useFts) query.bindValue(QStringLiteral(":match"),fts);
        if(!category.isEmpty()) query.bindValue(QStringLiteral(":category"),category);
        if(!generation.isEmpty()) query.bindValue(QStringLiteral(":generation"),QStringLiteral("%%1%").arg(generation));
        if(!useFts && !cleaned.trimmed().isEmpty()){
            const QStringList terms=normalized(cleaned).split(QLatin1Char(' '),Qt::SkipEmptyParts);
            for(int i=0;i<terms.size();++i) query.bindValue(QStringLiteral(":q%1").arg(i),QStringLiteral("%%1%").arg(terms.at(i)));
        }
        if(!query.exec()) return rows;
        while(query.next()){
            QVariantMap row;
            const QSqlRecord record=query.record();
            for(int i=0;i<record.count();++i) row.insert(record.fieldName(i),query.value(i));
            rows.append(row);
        }
        return rows;
    }

    QVariantList combinedUnique(const QVariantList &first,const QVariantList &second) const
    {
        QVariantList result;
        QSet<QString> seen;
        const auto append=[&result,&seen](const QVariantList &source){
            for(const QVariant &v:source){
                const QVariantMap row=v.toMap();
                const QString key=row.value(QStringLiteral("category")).toString()+QLatin1Char('|')+
                                  row.value(QStringLiteral("source_table")).toString()+QLatin1Char('|')+
                                  row.value(QStringLiteral("source_key")).toString()+QLatin1Char('|')+
                                  row.value(QStringLiteral("title")).toString();
                if(!seen.contains(key)){seen.insert(key);result.append(v);}
            }
        };
        append(first);append(second);
        return result;
    }

    QVariantList fetchRows(const QString &raw,QString category,const QString &generation) const
    {
        if(category==QStringLiteral("wiring")){
            QVariantList rows=executeQuery(raw,QStringLiteral("wiring"),generation,180);
            rows=combinedUnique(rows,executeQuery(raw.isEmpty()?QStringLiteral("broche"):raw,QStringLiteral("documentation"),generation,80));
            return rows;
        }
        return executeQuery(raw,category,generation,category.isEmpty()?140:220);
    }

    QVector<QVariantMap> cleanAndSort(const QVariantList &fetched,const QString &raw,const QString &requestedCategory) const
    {
        Q_UNUSED(raw);
        QVector<QVariantMap> rows;
        QSet<QString> seen;
        QSet<QString> ecuTargetTitles;
        for(const QVariant &v:fetched){
            const QVariantMap row=v.toMap();
            if(row.value(QStringLiteral("category")).toString()==QStringLiteral("ecu") && row.value(QStringLiteral("source_table")).toString()==QStringLiteral("ecu_target"))
                ecuTargetTitles.insert(row.value(QStringLiteral("title")).toString().trimmed().toCaseFolded());
        }
        for(const QVariant &v:fetched){
            QVariantMap row=v.toMap();
            const QString category=row.value(QStringLiteral("category")).toString();
            const QString source=row.value(QStringLiteral("source_table")).toString();
            const QString title=row.value(QStringLiteral("title")).toString().trimmed();
            if(title.isEmpty()) continue;
            if(category==QStringLiteral("ecu")){
                if(requestedCategory==QStringLiteral("ecu") && source!=QStringLiteral("ecu_target")) continue;
                if(source!=QStringLiteral("ecu_target") && ecuTargetTitles.contains(title.toCaseFolded())) continue;
                bool numeric=true;for(const QChar ch:title){if(!ch.isDigit()){numeric=false;break;}}
                if(numeric) continue;
            }
            const QString key=category+QLatin1Char('|')+title.toCaseFolded()+QLatin1Char('|')+row.value(QStringLiteral("generation")).toString();
            if(seen.contains(key)) continue;
            seen.insert(key);
            rows.append(row);
        }
        return rows;
    }

    void updateSuggestions(const QString &raw,const QVector<QVariantMap> &rows)
    {
        QStringList suggestions;
        QSet<QString> seen;
        const QString needle=normalized(raw);
        if(needle.size()>=2){
            for(const QVariantMap &row:rows){
                const QString title=row.value(QStringLiteral("title")).toString().trimmed();
                const QString key=normalized(title);
                if(!title.isEmpty() && !seen.contains(key) && (key.startsWith(needle)||key.contains(needle))){
                    seen.insert(key);suggestions.append(title);
                    if(suggestions.size()>=8) break;
                }
            }
        }
        m_completionModel->setStringList(suggestions);
        if(suggestions.isEmpty()){
            if(m_completer&&m_completer->popup()) m_completer->popup()->hide();
        }else if(m_search->hasFocus()) m_completer->complete();
    }

    void runSearch()
    {
        if(!m_results) return;
        if(m_debounce) m_debounce->stop();
        const QString raw=m_search?m_search->text().trimmed():QString();
        QString category=filterCategory();
        QString generation=filterGeneration();
        if(category.isEmpty() && generation.isEmpty()) category=exactCategoryIntent(raw);
        if(generation.isEmpty()) generation=generationFromText(raw);

        if(raw.isEmpty() && category.isEmpty() && generation.isEmpty()){
            m_results->setRowCount(0);
            if(m_count) m_count->setText(I18n::text(7324).arg(QStringLiteral("0")));
            m_detail->setHtml(htmlStyle()+QStringLiteral("<h1>%1</h1><p class='muted'>%2</p>").arg(I18n::text(7216).toHtmlEscaped(),I18n::text(7295).toHtmlEscaped()));
            updateDocumentControls();forceDetailScroll();
            return;
        }

        QString effectiveRaw=raw;
        if(!category.isEmpty() && exactCategoryIntent(raw)==category) effectiveRaw.clear();
        const QVariantList fetched=fetchRows(effectiveRaw,category,generation);
        QVector<QVariantMap> rows=cleanAndSort(fetched,raw,category);
        populate(rows);
        if(m_count) m_count->setText(I18n::text(7324).arg(QString::number(rows.size())));
        updateSuggestions(raw,rows);

        if(rows.isEmpty()){
            m_detail->setHtml(htmlStyle()+QStringLiteral("<h1>%1</h1><p class='muted'>%2</p>").arg(I18n::text(7216).toHtmlEscaped(),I18n::text(7328).toHtmlEscaped()));
            updateDocumentControls();forceDetailScroll();
        }else{
            m_results->selectRow(0);
            showSelectedResult();
        }
    }

    void populate(const QVector<QVariantMap> &rows)
    {
        m_results->blockSignals(true);
        m_results->clearSelection();
        m_results->setRowCount(rows.size());
        for(int r=0;r<rows.size();++r){
            const QVariantMap &row=rows.at(r);
            const QString title=row.value(QStringLiteral("title")).toString();
            const QString generation=row.value(QStringLiteral("generation")).toString();
            const QString category=row.value(QStringLiteral("category")).toString();
            QTableWidgetItem *titleItem=new QTableWidgetItem(title);
            titleItem->setData(Qt::UserRole+10,category);
            titleItem->setData(Qt::UserRole+11,row.value(QStringLiteral("source_table")));
            titleItem->setData(Qt::UserRole+12,row.value(QStringLiteral("source_key")));
            titleItem->setData(Qt::UserRole+13,row.value(QStringLiteral("content")));
            titleItem->setData(Qt::UserRole+14,generation);
            if(category==QStringLiteral("ecu") && row.value(QStringLiteral("source_table")).toString()==QStringLiteral("ecu_target"))
                titleItem->setData(Qt::UserRole,title);
            QFont font=titleItem->font();font.setBold(category==QStringLiteral("ecu")||category==QStringLiteral("dtc"));titleItem->setFont(font);
            m_results->setItem(r,0,titleItem);
            m_results->setItem(r,1,new QTableWidgetItem(generation.isEmpty()?QStringLiteral("—"):generation));
            m_results->setItem(r,2,new QTableWidgetItem(categoryLabel(category)));
            m_results->setRowHeight(r,27);
        }
        m_results->blockSignals(false);
    }

    QVariantMap selectedResult() const
    {
        QVariantMap row;
        if(!m_results||m_results->currentRow()<0) return row;
        QTableWidgetItem *item=m_results->item(m_results->currentRow(),0);
        if(!item) return row;
        row.insert(QStringLiteral("title"),item->text());
        row.insert(QStringLiteral("category"),item->data(Qt::UserRole+10));
        row.insert(QStringLiteral("source_table"),item->data(Qt::UserRole+11));
        row.insert(QStringLiteral("source_key"),item->data(Qt::UserRole+12));
        row.insert(QStringLiteral("content"),item->data(Qt::UserRole+13));
        row.insert(QStringLiteral("generation"),item->data(Qt::UserRole+14));
        return row;
    }

    QString currentGeneration() const
    {
        QString generation=filterGeneration();
        if(!generation.isEmpty()) return generation;
        const QVariantMap row=selectedResult();
        const QString rowGeneration=row.value(QStringLiteral("generation")).toString();
        QStringList found;
        for(const QString &token:{QStringLiteral("1.2"),QStringLiteral("1.3"),QStringLiteral("1.6"),QStringLiteral("1.9")})
            if(rowGeneration.contains(token)) found.append(token);
        if(found.size()==1) return found.first();
        generation=generationFromText(m_search?m_search->text():QString());
        return generation;
    }

    void updateDocumentControls()
    {
        const QString generation=currentGeneration();
        if(m_currentGenerationLabel){
            const QString label=generationLabel(generation);
            m_currentGenerationLabel->setText(label.isEmpty()?QStringLiteral("—"):label);
        }
        const QString xml=xmlPath(generation);
        const QString pdf=pdfPath(generation);
        if(m_sheetButton) m_sheetButton->setEnabled(!xml.isEmpty()&&QFileInfo::exists(xml));
        if(m_pdfButton){
            const bool available=!pdf.isEmpty()&&QFileInfo::exists(pdf);
            m_pdfButton->setEnabled(available);
            m_pdfButton->setVisible(true);
        }
    }

    void showSelectedResult()
    {
        const QVariantMap row=selectedResult();
        if(row.isEmpty()) return;
        const QString category=row.value(QStringLiteral("category")).toString();
        const QString source=row.value(QStringLiteral("source_table")).toString();
        const QString generation=row.value(QStringLiteral("generation")).toString();

        if(category==QStringLiteral("ecu") && source==QStringLiteral("ecu_target")){
            QMetaObject::invokeMethod(m_browser.data(),"showSelectedEcu",Qt::DirectConnection);
            updateDocumentControls();forceDetailScroll();
            return;
        }
        if(category==QStringLiteral("documentation") && source==QStringLiteral("xml_documentation")){
            QString token;
            for(const QString &candidate:{QStringLiteral("1.2"),QStringLiteral("1.3"),QStringLiteral("1.6"),QStringLiteral("1.9")})
                if(generation.contains(candidate)){token=candidate;break;}
            const QString path=xmlPath(token);
            if(!path.isEmpty()&&QFileInfo::exists(path)){
                m_detail->setHtml(renderXml(path));
                if(m_detail->verticalScrollBar()) m_detail->verticalScrollBar()->setValue(0);
                updateDocumentControls();forceDetailScroll();
                return;
            }
        }

        const QString title=row.value(QStringLiteral("title")).toString();
        QString content=row.value(QStringLiteral("content")).toString().toHtmlEscaped();
        content.replace(QStringLiteral("\n"),QStringLiteral("<br>"));
        QString subtitle=categoryLabel(category).toHtmlEscaped();
        if(!generation.isEmpty()) subtitle+=QStringLiteral(" — ")+generation.toHtmlEscaped();
        m_detail->setHtml(htmlStyle()+QStringLiteral("<h1>%1</h1><p class='muted'>%2</p><div class='content'>%3</div>").arg(title.toHtmlEscaped(),subtitle,content));
        if(m_detail->verticalScrollBar()) m_detail->verticalScrollBar()->setValue(0);
        updateDocumentControls();forceDetailScroll();
    }

    void openXmlDialog()
    {
        const QString generation=currentGeneration();
        const QString path=xmlPath(generation);
        if(path.isEmpty()||!QFileInfo::exists(path)) return;
        QDialog dialog(m_browser);
        dialog.setWindowTitle(I18n::text(7325)+QStringLiteral(" — ")+generationLabel(generation));
        dialog.resize(940,680);
        QVBoxLayout *layout=new QVBoxLayout(&dialog);
        layout->setContentsMargins(8,8,8,8);
        QTextBrowser *view=new QTextBrowser(&dialog);
        view->setHtml(renderXml(path));
        view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        view->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        view->setFocusPolicy(Qt::StrongFocus);
        view->setStyleSheet(QStringLiteral(
            "QTextBrowser{background:#0a1015;color:#dce3e8;border:1px solid #34414b;}"
            "QScrollBar:vertical{background:#111a21;width:16px;border-left:1px solid #53616c;}"
            "QScrollBar::handle:vertical{background:#7c8b96;min-height:38px;border:1px solid #a0abb3;border-radius:5px;margin:2px;}"
            "QScrollBar::handle:vertical:hover{background:#ff8a22;}QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical{height:0;}"));
        layout->addWidget(view);
        dialog.exec();
    }

private:
    QPointer<QWidget> m_browser;
    QLineEdit *m_search;
    QComboBox *m_filter;
    QTableWidget *m_results;
    QTextBrowser *m_detail;
    QLabel *m_count;
    QLabel *m_scopeCompact;
    QPushButton *m_sheetButton;
    QPushButton *m_pdfButton;
    QLabel *m_currentGenerationLabel;
    QTimer *m_debounce;
    QStringListModel *m_completionModel;
    QCompleter *m_completer;
    QSqlDatabase m_index;
    QString m_indexConnection;
    bool m_ftsEnabled;
};

class DatabaseExplorerV2Installer : public QObject
{
public:
    explicit DatabaseExplorerV2Installer(QObject *parent=nullptr):QObject(parent){}

    bool eventFilter(QObject *watched,QEvent *event) override
    {
        QWidget *widget=qobject_cast<QWidget*>(watched);
        if(widget&&event&&(event->type()==QEvent::Show||event->type()==QEvent::Polish)&&widget->objectName()==QStringLiteral("memsDatabaseBrowser")) schedule(widget);
        return QObject::eventFilter(watched,event);
    }

    void scan()
    {
        for(QWidget *widget:QApplication::allWidgets())
            if(widget&&widget->objectName()==QStringLiteral("memsDatabaseBrowser")) schedule(widget);
    }

private:
    void schedule(QWidget *widget)
    {
        if(!widget||widget->property("databaseExplorerV2Patched").toBool()) return;
        widget->setProperty("databaseExplorerV2Patched",true);
        QPointer<QWidget> guarded(widget);
        QTimer::singleShot(80,this,[guarded](){if(guarded) new DatabaseExplorerV2(guarded);});
    }
};

void installDatabaseExplorerV2()
{
    QCoreApplication *core=QCoreApplication::instance();
    if(!core) return;
    DatabaseExplorerV2Installer *installer=new DatabaseExplorerV2Installer(core);
    core->installEventFilter(installer);
    QTimer::singleShot(250,installer,[installer](){installer->scan();});
}

}

Q_COREAPP_STARTUP_FUNCTION(installDatabaseExplorerV2)
