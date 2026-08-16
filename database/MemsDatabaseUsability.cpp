#include "../i18n.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QComboBox>
#include <QCompleter>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QEvent>
#include <QFile>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
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
#include <QVariant>
#include <QVBoxLayout>
#include <QVector>
#include <QXmlStreamReader>

#include <algorithm>

namespace {

QString normalizedSearchText(const QString &input)
{
    const QString decomposed=input.normalized(QString::NormalizationForm_D).toCaseFolded();
    QString output;
    output.reserve(decomposed.size());
    bool previousSpace=false;
    for(const QChar ch:decomposed){
        const QChar::Category category=ch.category();
        if(category==QChar::Mark_NonSpacing || category==QChar::Mark_SpacingCombining || category==QChar::Mark_Enclosing)
            continue;
        if(ch.isLetterOrNumber()){
            if(ch==QLatin1Char('o') || ch==QLatin1Char('0')) output+=QLatin1Char('0');
            else output+=ch;
            previousSpace=false;
        }else if(!previousSpace){
            output+=QLatin1Char(' ');
            previousSpace=true;
        }
    }
    return output.simplified();
}

int levenshteinDistance(const QString &left,const QString &right)
{
    if(left==right) return 0;
    if(left.isEmpty()) return right.size();
    if(right.isEmpty()) return left.size();
    QVector<int> previous(right.size()+1),current(right.size()+1);
    for(int j=0;j<=right.size();++j) previous[j]=j;
    for(int i=1;i<=left.size();++i){
        current[0]=i;
        for(int j=1;j<=right.size();++j){
            const int substitution=previous[j-1]+(left.at(i-1)==right.at(j-1)?0:1);
            current[j]=qMin(qMin(previous[j]+1,current[j-1]+1),substitution);
        }
        previous.swap(current);
    }
    return previous[right.size()];
}

QString yearsText(const QString &from,const QString &to)
{
    if(from.isEmpty()&&to.isEmpty()) return QStringLiteral("—");
    if(from.isEmpty()) return QStringLiteral("≤ %1").arg(to);
    if(to.isEmpty()) return QStringLiteral("%1 →").arg(from);
    return from==to?from:QStringLiteral("%1–%2").arg(from,to);
}

QString xmlHtmlStyle()
{
    return QStringLiteral(
        "<style>body{background:#0a1015;color:#dce3e8;font-family:'Segoe UI',Arial,sans-serif;font-size:9pt;}"
        "h1{color:#ff9828;font-size:16pt;margin:0 0 5px 0;}"
        "h2{color:#ff9828;font-size:10.5pt;border-bottom:1px solid #34414b;padding-bottom:4px;margin-top:14px;}"
        "p{margin:4px 0 7px 0;line-height:1.35}.muted{color:#94a1ab}.note{background:#15100b;border:1px solid #60401f;color:#ffd0a0;padding:7px;}"
        "table{border-collapse:collapse;width:100%;margin:5px 0 8px 0}th{background:#151e25;color:#ff9828;border-bottom:2px solid #ff7a00;text-align:left;padding:5px}"
        "td{border-bottom:1px solid #26323b;padding:5px;vertical-align:top}</style>");
}

QString generationToken(const QString &system)
{
    if(system.contains(QStringLiteral("1.9"))) return QStringLiteral("1.9");
    if(system.contains(QStringLiteral("1.6"))) return QStringLiteral("1.6");
    if(system.contains(QStringLiteral("1.3"))) return QStringLiteral("1.3");
    if(system.contains(QStringLiteral("1.2"))) return QStringLiteral("1.2");
    return QString();
}

struct SearchRow
{
    QString part;
    QString brand;
    QString system;
    QString injection;
    QString makes;
    QString models;
    QString yearFrom;
    QString yearTo;
    QString notes;
    QString fitments;
};

class DatabaseBrowserController : public QObject
{
public:
    explicit DatabaseBrowserController(QWidget *browser)
        : QObject(browser),m_browser(browser),m_search(nullptr),m_generation(nullptr),m_results(nullptr),m_detail(nullptr),
          m_resultCount(nullptr),m_completionModel(nullptr),m_completer(nullptr),m_documentGeneration(nullptr),
          m_documentLabel(nullptr),m_sheetButton(nullptr),m_pdfButton(nullptr)
    {
        if(!m_browser) return;
        m_search=m_browser->findChild<QLineEdit*>();
        const QList<QComboBox*> combos=m_browser->findChildren<QComboBox*>();
        if(!combos.isEmpty()) m_generation=combos.first();
        m_results=m_browser->findChild<QTableWidget*>();
        m_detail=m_browser->findChild<QTextBrowser*>();
        locateResultCount();
        openDatabase();
        installScrollBar();
        installCompleter();
        installDirectDocumentation();
        connectSignals();
        m_browser->installEventFilter(this);
        QTimer::singleShot(0,this,[this](){updateTranslations();smartRefresh();});
    }

    ~DatabaseBrowserController() override
    {
        if(m_database.isOpen()) m_database.close();
        const QString connection=m_connectionName;
        m_database=QSqlDatabase();
        if(!connection.isEmpty()) QSqlDatabase::removeDatabase(connection);
    }

protected:
    bool eventFilter(QObject *watched,QEvent *event) override
    {
        if(watched==m_browser && event && event->type()==QEvent::LanguageChange)
            QTimer::singleShot(0,this,[this](){updateTranslations();smartRefresh();});
        return QObject::eventFilter(watched,event);
    }

private:
    void locateResultCount()
    {
        if(!m_search||!m_search->parentWidget()) return;
        QHBoxLayout *layout=qobject_cast<QHBoxLayout*>(m_search->parentWidget()->layout());
        if(!layout) return;
        for(int i=layout->count()-1;i>=0;--i){
            QLabel *label=qobject_cast<QLabel*>(layout->itemAt(i)->widget());
            if(label){m_resultCount=label;break;}
        }
    }

    void openDatabase()
    {
        const QString root=QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)+QStringLiteral("/reference");
        QDir dir(root);
        const QStringList databases=dir.entryList(QStringList()<<QStringLiteral("ecu_mems_reference_*.sqlite"),QDir::Files,QDir::Time);
        if(databases.isEmpty()) return;
        m_connectionName=QStringLiteral("MEMS_SMART_SEARCH_%1").arg(reinterpret_cast<quintptr>(this));
        m_database=QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"),m_connectionName);
        m_database.setConnectOptions(QStringLiteral("QSQLITE_OPEN_READONLY"));
        m_database.setDatabaseName(dir.filePath(databases.first()));
        if(m_database.open()){
            QSqlQuery query(m_database);
            query.exec(QStringLiteral("PRAGMA query_only = ON"));
        }
    }

    void installScrollBar()
    {
        if(!m_detail) return;
        m_detail->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        m_detail->verticalScrollBar()->setStyleSheet(QStringLiteral(
            "QScrollBar:vertical{background:#0c1217;width:12px;margin:0;border-left:1px solid #27323b;}"
            "QScrollBar::handle:vertical{background:#53616c;min-height:32px;border-radius:5px;margin:2px;}"
            "QScrollBar::handle:vertical:hover{background:#ff8a22;}"
            "QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical{height:0;}"
            "QScrollBar::add-page:vertical,QScrollBar::sub-page:vertical{background:transparent;}"));
    }

    void installCompleter()
    {
        if(!m_search) return;
        m_completionModel=new QStringListModel(this);
        m_completer=new QCompleter(m_completionModel,this);
        m_completer->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
        m_completer->setCaseSensitivity(Qt::CaseInsensitive);
        m_completer->setMaxVisibleItems(8);
        m_search->setCompleter(m_completer);
        connect(m_completer,QOverload<const QString &>::of(&QCompleter::activated),this,[this](const QString &text){
            if(!m_search) return;
            m_search->setText(text);
            m_search->setCursorPosition(text.size());
            if(m_completer->popup()) m_completer->popup()->hide();
        });
    }

    void installDirectDocumentation()
    {
        if(!m_detail||!m_detail->parentWidget()) return;
        QWidget *detailCard=m_detail->parentWidget();
        QVBoxLayout *vertical=qobject_cast<QVBoxLayout*>(detailCard->layout());
        if(!vertical||vertical->count()<1) return;
        QHBoxLayout *tools=qobject_cast<QHBoxLayout*>(vertical->itemAt(0)->layout());
        if(!tools) return;

        QList<QPushButton*> buttons;
        for(int i=0;i<tools->count();++i){
            if(QPushButton *button=qobject_cast<QPushButton*>(tools->itemAt(i)->widget())) buttons.append(button);
        }
        if(buttons.size()<2) return;
        m_sheetButton=buttons.at(0);
        m_pdfButton=buttons.at(1);

        m_documentLabel=new QLabel(detailCard);
        m_documentLabel->setStyleSheet(QStringLiteral("color:#ff9828;background:transparent;border:0;font-weight:700;"));
        m_documentGeneration=new QComboBox(detailCard);
        m_documentGeneration->setMinimumHeight(28);
        m_documentGeneration->setMinimumWidth(105);
        m_documentGeneration->setStyleSheet(QStringLiteral("QComboBox{background:#0a1117;color:#e7ecef;border:1px solid #34414b;border-radius:4px;padding:3px 7px;}"));

        const int sheetIndex=tools->indexOf(m_sheetButton);
        const int insertAt=sheetIndex>=0?sheetIndex:tools->count();
        tools->insertWidget(insertAt,m_documentLabel);
        tools->insertWidget(insertAt+1,m_documentGeneration);

        QObject::disconnect(m_sheetButton,static_cast<const char *>(nullptr),m_browser.data(),static_cast<const char *>(nullptr));
        QObject::disconnect(m_pdfButton,static_cast<const char *>(nullptr),m_browser.data(),static_cast<const char *>(nullptr));
        connect(m_sheetButton,&QPushButton::clicked,this,[this](){showXml();});
        connect(m_pdfButton,&QPushButton::clicked,this,[this](){openPdf();});
        connect(m_documentGeneration,QOverload<int>::of(&QComboBox::currentIndexChanged),this,[this](int){updateDocumentButtons();});
    }

    void connectSignals()
    {
        if(m_search) connect(m_search,&QLineEdit::textChanged,this,[this](const QString &){smartRefresh();});
        if(m_generation) connect(m_generation,QOverload<int>::of(&QComboBox::currentIndexChanged),this,[this](int){smartRefresh();});
        if(m_results) connect(m_results,&QTableWidget::itemSelectionChanged,this,[this](){syncDocumentGeneration();});
    }

    void updateTranslations()
    {
        if(m_search){
            m_search->setPlaceholderText(I18n::text(7217));
            m_search->setToolTip(I18n::text(7286));
        }
        if(m_documentLabel) m_documentLabel->setText(I18n::text(7288));
        if(m_sheetButton) m_sheetButton->setText(I18n::text(7287));
        if(m_pdfButton) m_pdfButton->setText(I18n::text(7261));
        if(m_documentGeneration){
            const QString current=m_documentGeneration->currentData().toString();
            m_documentGeneration->blockSignals(true);
            m_documentGeneration->clear();
            m_documentGeneration->addItem(I18n::text(7164),QStringLiteral("1.3"));
            m_documentGeneration->addItem(I18n::text(7165),QStringLiteral("1.6"));
            m_documentGeneration->addItem(I18n::text(7166),QStringLiteral("1.9"));
            const int index=m_documentGeneration->findData(current);
            m_documentGeneration->setCurrentIndex(index>=0?index:0);
            m_documentGeneration->blockSignals(false);
            updateDocumentButtons();
        }
    }

    QString currentGenerationFilter() const
    {
        return m_generation?m_generation->currentData().toString().trimmed():QString();
    }

    QVector<SearchRow> loadRows(const QString &generation) const
    {
        QVector<SearchRow> rows;
        if(!m_database.isOpen()) return rows;
        QString sql=QStringLiteral(
            "SELECT e.part_number,e.brand,e.system_family,e.injection,e.makes,e.models,e.year_from,e.year_to,e.notes,"
            "COALESCE((SELECT GROUP_CONCAT(COALESCE(f.make,'') || ' ' || COALESCE(f.model,'') || ' ' || COALESCE(f.variant,'') || ' ' || COALESCE(f.engine,'') || ' ' || COALESCE(f.market,'') || ' ' || COALESCE(f.alt_refs,''),' | ') FROM ecu_fitment_target f WHERE f.part_number=e.part_number),'') AS fit_search "
            "FROM ecu_target e");
        if(!generation.isEmpty()) sql+=QStringLiteral(" WHERE e.system_family LIKE :generation");
        sql+=QStringLiteral(" ORDER BY e.system_family,e.part_number");
        QSqlQuery query(m_database);
        if(!query.prepare(sql)) return rows;
        if(!generation.isEmpty()) query.bindValue(QStringLiteral(":generation"),QStringLiteral("%%1%").arg(generation));
        if(!query.exec()) return rows;
        while(query.next()){
            SearchRow row;
            row.part=query.value(0).toString().trimmed();
            row.brand=query.value(1).toString().trimmed();
            row.system=query.value(2).toString().trimmed();
            row.injection=query.value(3).toString().trimmed();
            row.makes=query.value(4).toString().trimmed();
            row.models=query.value(5).toString().trimmed();
            row.yearFrom=query.value(6).toString().trimmed();
            row.yearTo=query.value(7).toString().trimmed();
            row.notes=query.value(8).toString().trimmed();
            row.fitments=query.value(9).toString().trimmed();
            rows.append(row);
        }
        return rows;
    }

    bool rowMatches(const SearchRow &row,const QString &queryText) const
    {
        const QString normalizedQuery=normalizedSearchText(queryText);
        if(normalizedQuery.isEmpty()) return true;
        const QString haystack=normalizedSearchText(row.part+QLatin1Char(' ')+row.brand+QLatin1Char(' ')+row.system+QLatin1Char(' ')+row.injection+QLatin1Char(' ')+row.makes+QLatin1Char(' ')+row.models+QLatin1Char(' ')+row.notes+QLatin1Char(' ')+row.fitments);
        const QStringList terms=normalizedQuery.split(QLatin1Char(' '),Qt::SkipEmptyParts);
        for(const QString &term:terms) if(!haystack.contains(term)) return false;
        return true;
    }

    void addCandidate(QSet<QString> &set,const QString &raw) const
    {
        const QString cleaned=raw.trimmed();
        if(cleaned.isEmpty()) return;
        const QStringList parts=cleaned.split(QRegularExpression(QStringLiteral("[|;,\\n\\r]+")),Qt::SkipEmptyParts);
        if(parts.isEmpty()) set.insert(cleaned);
        else for(const QString &part:parts){const QString candidate=part.trimmed();if(!candidate.isEmpty()&&candidate.size()<=80)set.insert(candidate);}
    }

    QStringList allCandidates(const QString &generation) const
    {
        QSet<QString> candidates;
        if(!m_database.isOpen()) return QStringList();
        QString ecuSql=QStringLiteral("SELECT part_number,brand,system_family,injection,makes,models FROM ecu_target");
        if(!generation.isEmpty()) ecuSql+=QStringLiteral(" WHERE system_family LIKE :generation");
        QSqlQuery ecuQuery(m_database);
        if(ecuQuery.prepare(ecuSql)){
            if(!generation.isEmpty()) ecuQuery.bindValue(QStringLiteral(":generation"),QStringLiteral("%%1%").arg(generation));
            if(ecuQuery.exec()) while(ecuQuery.next()) for(int i=0;i<6;++i) addCandidate(candidates,ecuQuery.value(i).toString());
        }

        QString fitSql=QStringLiteral("SELECT f.make,f.model,f.variant,f.engine,f.market,f.alt_refs FROM ecu_fitment_target f");
        if(!generation.isEmpty()) fitSql+=QStringLiteral(" JOIN ecu_target e ON e.part_number=f.part_number WHERE e.system_family LIKE :generation");
        QSqlQuery fitQuery(m_database);
        if(fitQuery.prepare(fitSql)){
            if(!generation.isEmpty()) fitQuery.bindValue(QStringLiteral(":generation"),QStringLiteral("%%1%").arg(generation));
            if(fitQuery.exec()) while(fitQuery.next()) for(int i=0;i<6;++i) addCandidate(candidates,fitQuery.value(i).toString());
        }
        QStringList list=candidates.values();
        std::sort(list.begin(),list.end(),[](const QString &a,const QString &b){return QString::localeAwareCompare(a,b)<0;});
        return list;
    }

    QStringList suggestionsFor(const QString &typed,const QString &generation,int limit=8) const
    {
        const QString needle=normalizedSearchText(typed);
        if(needle.size()<2) return QStringList();
        const QStringList candidates=allCandidates(generation);
        QStringList prefix;
        for(const QString &candidate:candidates){
            const QString normalized=normalizedSearchText(candidate);
            bool starts=normalized.startsWith(needle);
            if(!starts){
                const QStringList words=normalized.split(QLatin1Char(' '),Qt::SkipEmptyParts);
                for(const QString &word:words) if(word.startsWith(needle)){starts=true;break;}
            }
            if(starts){prefix.append(candidate);if(prefix.size()>=limit) break;}
        }
        if(!prefix.isEmpty()) return prefix;

        struct Ranked{int distance;QString text;};
        QVector<Ranked> ranked;
        const int threshold=needle.size()<=4?1:(needle.size()<=7?2:3);
        for(const QString &candidate:candidates){
            const QString normalized=normalizedSearchText(candidate);
            int best=levenshteinDistance(needle,normalized);
            const QStringList words=normalized.split(QLatin1Char(' '),Qt::SkipEmptyParts);
            for(const QString &word:words) best=qMin(best,levenshteinDistance(needle,word));
            if(best<=threshold) ranked.append({best,candidate});
        }
        std::sort(ranked.begin(),ranked.end(),[](const Ranked &a,const Ranked &b){
            if(a.distance!=b.distance) return a.distance<b.distance;
            if(a.text.size()!=b.text.size()) return a.text.size()<b.text.size();
            return QString::localeAwareCompare(a.text,b.text)<0;
        });
        QStringList result;
        for(const Ranked &item:ranked){if(!result.contains(item.text,Qt::CaseInsensitive))result.append(item.text);if(result.size()>=limit)break;}
        return result;
    }

    void updateCompleter(const QString &typed,const QStringList &suggestions)
    {
        if(!m_completionModel||!m_completer) return;
        m_completionModel->setStringList(suggestions);
        if(typed.trimmed().size()<2||suggestions.isEmpty()){
            if(m_completer->popup()) m_completer->popup()->hide();
            return;
        }
        if(m_search&&m_search->hasFocus()) m_completer->complete();
    }

    void smartRefresh()
    {
        if(!m_database.isOpen()||!m_results) return;
        const QString typed=m_search?m_search->text().trimmed():QString();
        const QString generation=currentGenerationFilter();
        const QString previous=currentPartNumber();
        const QVector<SearchRow> all=loadRows(generation);
        QVector<SearchRow> matches;
        for(const SearchRow &row:all) if(rowMatches(row,typed)) matches.append(row);

        const QStringList suggestions=suggestionsFor(typed,generation);
        updateCompleter(typed,suggestions);
        QString corrected;
        if(matches.isEmpty()&&!typed.isEmpty()&&!suggestions.isEmpty()){
            corrected=suggestions.first();
            for(const SearchRow &row:all) if(rowMatches(row,corrected)) matches.append(row);
        }

        m_results->blockSignals(true);
        m_results->setRowCount(matches.size());
        int restore=-1;
        for(int rowIndex=0;rowIndex<matches.size();++rowIndex){
            const SearchRow &row=matches.at(rowIndex);
            const QString columns[]={row.part,row.system,row.injection,row.models,yearsText(row.yearFrom,row.yearTo)};
            for(int column=0;column<5;++column){
                QTableWidgetItem *item=new QTableWidgetItem(columns[column]);
                if(column==0){
                    item->setData(Qt::UserRole,row.part);
                    QFont font=item->font();font.setBold(true);item->setFont(font);
                }
                m_results->setItem(rowIndex,column,item);
            }
            m_results->setRowHeight(rowIndex,27);
            if(row.part==previous) restore=rowIndex;
        }
        m_results->blockSignals(false);

        if(m_resultCount){
            QString status=QStringLiteral("%1: %2").arg(I18n::text(7254),QString::number(matches.size()));
            if(!corrected.isEmpty()) status+=QStringLiteral(" — ")+I18n::text(7285).arg(corrected);
            m_resultCount->setText(status);
            m_resultCount->setToolTip(corrected.isEmpty()?QString():I18n::text(7285).arg(corrected));
        }
        if(!matches.isEmpty()){
            m_results->selectRow(restore>=0?restore:0);
            syncDocumentGeneration();
        }
    }

    QString currentPartNumber() const
    {
        if(!m_results||m_results->currentRow()<0) return QString();
        QTableWidgetItem *item=m_results->item(m_results->currentRow(),0);
        return item?item->data(Qt::UserRole).toString():QString();
    }

    void syncDocumentGeneration()
    {
        if(!m_results||!m_documentGeneration||m_results->currentRow()<0) return;
        QTableWidgetItem *item=m_results->item(m_results->currentRow(),1);
        if(!item) return;
        const QString token=generationToken(item->text());
        const int index=m_documentGeneration->findData(token);
        if(index>=0 && m_documentGeneration->currentIndex()!=index) m_documentGeneration->setCurrentIndex(index);
        updateDocumentButtons();
    }

    QString xmlPath(const QString &token) const
    {
        const QString root=QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)+QStringLiteral("/reference/fiches/");
        if(token==QStringLiteral("1.3")) return root+QStringLiteral("mems_1_3.xml");
        if(token==QStringLiteral("1.6")) return root+QStringLiteral("mems_1_6.xml");
        if(token==QStringLiteral("1.9")) return root+QStringLiteral("mems_1_9.xml");
        return QString();
    }

    QString pdfPath(const QString &token) const
    {
        const QString root=QCoreApplication::applicationDirPath()+QStringLiteral("/database/reference/pdf/");
        if(token==QStringLiteral("1.3")) return root+QStringLiteral("mems1.3_source.pdf");
        if(token==QStringLiteral("1.6")) return root+QStringLiteral("mems1.6_source.pdf");
        if(token==QStringLiteral("1.9")) return root+QStringLiteral("mems1.9_source.pdf");
        return QString();
    }

    void updateDocumentButtons()
    {
        if(!m_documentGeneration) return;
        const QString token=m_documentGeneration->currentData().toString();
        if(m_sheetButton) m_sheetButton->setEnabled(QFileInfo::exists(xmlPath(token)));
        if(m_pdfButton){const bool available=QFileInfo::exists(pdfPath(token));m_pdfButton->setEnabled(available);m_pdfButton->setVisible(true);}
    }

    QString renderXml(const QString &path) const
    {
        QFile file(path);
        if(!file.open(QIODevice::ReadOnly|QIODevice::Text)) return xmlHtmlStyle()+QStringLiteral("<p class='note'>%1</p>").arg(I18n::text(7232).toHtmlEscaped());
        QXmlStreamReader xml(&file);
        QString html=xmlHtmlStyle();
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
            }else if(name==QStringLiteral("table")){firstRow=true;html+=QStringLiteral("<table>");}
            else if(name==QStringLiteral("ligne")) html+=QStringLiteral("<tr>");
            else if(name==QStringLiteral("cellule")){
                const QString text=xml.readElementText(QXmlStreamReader::IncludeChildElements).trimmed().toHtmlEscaped();
                const QString tag=firstRow?QStringLiteral("th"):QStringLiteral("td");
                html+=QStringLiteral("<%1>%2</%1>").arg(tag,text);
            }
        }
        return html;
    }

    void showXml()
    {
        if(!m_detail||!m_documentGeneration) return;
        const QString path=xmlPath(m_documentGeneration->currentData().toString());
        m_detail->setHtml(renderXml(path));
        if(m_detail->verticalScrollBar()) m_detail->verticalScrollBar()->setValue(0);
    }

    void openPdf()
    {
        if(!m_documentGeneration) return;
        const QString path=pdfPath(m_documentGeneration->currentData().toString());
        if(!path.isEmpty()&&QFileInfo::exists(path)) QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    }

private:
    QPointer<QWidget> m_browser;
    QLineEdit *m_search;
    QComboBox *m_generation;
    QTableWidget *m_results;
    QTextBrowser *m_detail;
    QLabel *m_resultCount;
    QStringListModel *m_completionModel;
    QCompleter *m_completer;
    QComboBox *m_documentGeneration;
    QLabel *m_documentLabel;
    QPushButton *m_sheetButton;
    QPushButton *m_pdfButton;
    QSqlDatabase m_database;
    QString m_connectionName;
};

class DatabaseUsabilityInstaller : public QObject
{
public:
    explicit DatabaseUsabilityInstaller(QObject *parent=nullptr):QObject(parent){}

    bool eventFilter(QObject *watched,QEvent *event) override
    {
        QWidget *widget=qobject_cast<QWidget*>(watched);
        if(widget && event && (event->type()==QEvent::Show || event->type()==QEvent::Polish) && widget->objectName()==QStringLiteral("memsDatabaseBrowser"))
            schedulePatch(widget);
        return QObject::eventFilter(watched,event);
    }

    void scan()
    {
        const QList<QWidget*> widgets=QApplication::allWidgets();
        for(QWidget *widget:widgets) if(widget && widget->objectName()==QStringLiteral("memsDatabaseBrowser")) schedulePatch(widget);
    }

private:
    void schedulePatch(QWidget *widget)
    {
        if(!widget||widget->property("databaseUsabilityPatched").toBool()) return;
        widget->setProperty("databaseUsabilityPatched",true);
        QPointer<QWidget> guarded(widget);
        QTimer::singleShot(0,this,[guarded](){if(guarded) new DatabaseBrowserController(guarded);});
    }
};

void installDatabaseUsabilityPatch()
{
    QCoreApplication *core=QCoreApplication::instance();
    if(!core) return;
    DatabaseUsabilityInstaller *installer=new DatabaseUsabilityInstaller(core);
    core->installEventFilter(installer);
    QTimer::singleShot(0,installer,[installer](){installer->scan();});
}

}

Q_COREAPP_STARTUP_FUNCTION(installDatabaseUsabilityPatch)
