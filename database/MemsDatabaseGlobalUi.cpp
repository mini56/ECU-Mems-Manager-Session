#include "MemsGlobalSearchIndex.h"
#include "../i18n.h"

#include <QApplication>
#include <QComboBox>
#include <QCompleter>
#include <QCoreApplication>
#include <QDialog>
#include <QEvent>
#include <QFile>
#include <QFileInfo>
#include <QFont>
#include <QFrame>
#include <QGridLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMetaObject>
#include <QPointer>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollBar>
#include <QSet>
#include <QStringListModel>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTextBrowser>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>
#include <QVariantMap>
#include <QXmlStreamReader>

#include <algorithm>

namespace {

QString normalized(QString input)
{
    input=input.normalized(QString::NormalizationForm_D).toCaseFolded();
    QString out;
    bool space=true;
    for(QChar ch:input){
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
        for(QChar c:word) if(c.isDigit()){hasDigit=true;break;}
        if(hasDigit) word.replace(QLatin1Char('o'),QLatin1Char('0'));
    }
    return words.join(QLatin1Char(' '));
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
    static const QSet<QString> protocol={QStringLiteral("protocole"),QStringLiteral("protocol")};
    static const QSet<QString> documentation={QStringLiteral("documentation"),QStringLiteral("document"),QStringLiteral("documents"),QStringLiteral("xml"),QStringLiteral("fiche"),QStringLiteral("fiches")};
    static const QSet<QString> actuator={QStringLiteral("actionneur"),QStringLiteral("actionneurs"),QStringLiteral("actuator"),QStringLiteral("actuators")};
    static const QSet<QString> data={QStringLiteral("trame"),QStringLiteral("trames"),QStringLiteral("mesure"),QStringLiteral("mesures"),QStringLiteral("pid"),QStringLiteral("capteur"),QStringLiteral("capteurs")};
    if(ecu.contains(n)) return QStringLiteral("ecu");
    if(dtc.contains(n)) return QStringLiteral("dtc");
    if(vehicle.contains(n)) return QStringLiteral("vehicle");
    if(command.contains(n)) return QStringLiteral("command");
    if(protocol.contains(n)) return QStringLiteral("protocol");
    if(documentation.contains(n)) return QStringLiteral("documentation");
    if(actuator.contains(n)) return QStringLiteral("actuator");
    if(data.contains(n)) return QStringLiteral("data");
    return QString();
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

int categoryPriority(const QString &category,const QString &raw)
{
    const QString n=normalized(raw);
    const bool wiring=n.contains(QStringLiteral("broche")) || n.contains(QStringLiteral("pin")) ||
                      n.contains(QStringLiteral("cablage")) || n.contains(QStringLiteral("fil")) ||
                      n.contains(QStringLiteral("wire")) || n.contains(QStringLiteral("connecteur"));
    const bool dtc=n.contains(QStringLiteral("dtc")) || n.contains(QStringLiteral("code")) ||
                   n.contains(QStringLiteral("defaut")) || n.contains(QStringLiteral("fault"));
    const bool generation=!generationFromText(raw).isEmpty();
    if(wiring){
        if(category==QStringLiteral("wiring")) return 0;
        if(category==QStringLiteral("documentation")) return 1;
        if(category==QStringLiteral("protocol")) return 2;
    }
    if(dtc && category==QStringLiteral("dtc")) return 0;
    if(generation){
        if(category==QStringLiteral("documentation")) return 0;
        if(category==QStringLiteral("ecu")) return 1;
        if(category==QStringLiteral("protocol")) return 2;
        if(category==QStringLiteral("command")) return 3;
        if(category==QStringLiteral("data")) return 4;
        if(category==QStringLiteral("dtc")) return 5;
    }
    if(category==QStringLiteral("ecu")) return 10;
    if(category==QStringLiteral("vehicle")) return 11;
    if(category==QStringLiteral("dtc")) return 12;
    if(category==QStringLiteral("command")) return 13;
    if(category==QStringLiteral("wiring")) return 14;
    if(category==QStringLiteral("protocol")) return 15;
    if(category==QStringLiteral("documentation")) return 16;
    if(category==QStringLiteral("actuator")) return 17;
    if(category==QStringLiteral("data")) return 18;
    return 30;
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

QString xmlPath(const QString &generation)
{
    const QString root=QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)+QStringLiteral("/reference/fiches/");
    if(generation.contains(QStringLiteral("1.3"))) return root+QStringLiteral("mems_1_3.xml");
    if(generation.contains(QStringLiteral("1.6"))) return root+QStringLiteral("mems_1_6.xml");
    if(generation.contains(QStringLiteral("1.9"))) return root+QStringLiteral("mems_1_9.xml");
    return QString();
}

QString pdfPath(const QString &generation)
{
    const QString root=QCoreApplication::applicationDirPath()+QStringLiteral("/database/reference/pdf/");
    if(generation.contains(QStringLiteral("1.3"))) return root+QStringLiteral("mems1.3_source.pdf");
    if(generation.contains(QStringLiteral("1.6"))) return root+QStringLiteral("mems1.6_source.pdf");
    if(generation.contains(QStringLiteral("1.9"))) return root+QStringLiteral("mems1.9_source.pdf");
    return QString();
}

class GlobalDatabaseController : public QObject
{
public:
    explicit GlobalDatabaseController(QWidget *browser)
        : QObject(browser),m_browser(browser),m_search(nullptr),m_generation(nullptr),m_results(nullptr),m_detail(nullptr),
          m_count(nullptr),m_debounce(new QTimer(this)),m_completionModel(new QStringListModel(this)),m_completer(nullptr),
          m_categoryFrame(nullptr),m_sheetButton(nullptr),m_pdfButton(nullptr),m_currentGenerationLabel(nullptr)
    {
        if(!m_browser) return;
        m_search=m_browser->findChild<QLineEdit*>();
        const QList<QComboBox*> combos=m_browser->findChildren<QComboBox*>();
        if(!combos.isEmpty()) m_generation=combos.first();
        m_results=m_browser->findChild<QTableWidget*>();
        m_detail=m_browser->findChild<QTextBrowser*>();
        locateResultCount();
        if(!m_search||!m_generation||!m_results||!m_detail) return;

        MemsGlobalSearchIndex::ensureBuilt();
        disconnectCompetingSearch();
        findAndRewireGenerationButtons();
        buildCategoryExplorer();
        configureResultsTable();
        configureDetailScroll();
        configureDocumentationButtons();
        configureSearch();
        retranslate();
        m_browser->installEventFilter(this);
        QTimer::singleShot(0,this,[this](){runSearch();});
    }

protected:
    bool eventFilter(QObject *watched,QEvent *event) override
    {
        if(watched==m_browser && event){
            if(event->type()==QEvent::LanguageChange)
                QTimer::singleShot(60,this,[this](){retranslate();runSearch();});
            else if(event->type()==QEvent::Resize || event->type()==QEvent::Show)
                QTimer::singleShot(0,this,[this](){forceDetailScroll();});
        }
        return QObject::eventFilter(watched,event);
    }

private:
    void locateResultCount()
    {
        if(!m_search||!m_search->parentWidget()) return;
        const QList<QLabel*> labels=m_search->parentWidget()->findChildren<QLabel*>(QString(),Qt::FindDirectChildrenOnly);
        if(!labels.isEmpty()) m_count=labels.last();
    }

    void disconnectCompetingSearch()
    {
        QObject::disconnect(m_search,nullptr,nullptr,nullptr);
        QObject::disconnect(m_generation,nullptr,nullptr,nullptr);
        QObject::disconnect(m_results,nullptr,nullptr,nullptr);
    }

    void findAndRewireGenerationButtons()
    {
        const QList<QPushButton*> buttons=m_browser->findChildren<QPushButton*>();
        const QString allText=I18n::text(7290);
        const QString genTexts[]={I18n::text(7291),I18n::text(7292),I18n::text(7293),I18n::text(7294)};
        const QString tokens[]={QStringLiteral("1.2"),QStringLiteral("1.3"),QStringLiteral("1.6"),QStringLiteral("1.9")};
        for(QPushButton *button:buttons){
            if(!button) continue;
            if(button->text()==allText){
                m_allEcuButton=button;
                QObject::disconnect(button,nullptr,nullptr,nullptr);
                connect(button,&QPushButton::clicked,this,[this](){setGeneration(QString());m_activeCategory=QStringLiteral("ecu");clearSearchWithoutSignals();runSearch();});
            }
            for(int i=0;i<4;++i){
                if(button->text()==genTexts[i]){
                    m_generationButtons[i]=button;
                    QObject::disconnect(button,nullptr,nullptr,nullptr);
                    const QString token=tokens[i];
                    connect(button,&QPushButton::clicked,this,[this,token](){m_activeCategory.clear();setGeneration(token);clearSearchWithoutSignals();runSearch();});
                }
            }
        }
    }

    QPushButton *categoryButton(int key,const QString &category,QWidget *parent)
    {
        QPushButton *button=new QPushButton(parent);
        button->setProperty("i18nKey",key);
        button->setProperty("searchCategory",category);
        button->setMinimumHeight(27);
        button->setStyleSheet(QStringLiteral(
            "QPushButton{background:#111920;color:#e7ecef;border:1px solid #3b4852;border-radius:4px;padding:3px 8px;font-weight:600;}"
            "QPushButton:hover{border-color:#ff7a00;color:#ffad5c;}QPushButton:pressed{background:#2a1d11;}"));
        connect(button,&QPushButton::clicked,this,[this,category](){
            m_activeCategory=category;
            setGeneration(QString());
            clearSearchWithoutSignals();
            runSearch();
        });
        m_categoryButtons.append(button);
        return button;
    }

    void buildCategoryExplorer()
    {
        QVBoxLayout *root=qobject_cast<QVBoxLayout*>(m_browser->layout());
        if(!root) return;
        m_categoryFrame=new QFrame(m_browser);
        m_categoryFrame->setAttribute(Qt::WA_StyledBackground,true);
        m_categoryFrame->setStyleSheet(QStringLiteral("QFrame{background:#0d141a;border:1px solid #29343e;border-radius:5px;}"));
        QGridLayout *grid=new QGridLayout(m_categoryFrame);
        grid->setContentsMargins(9,4,9,4);grid->setHorizontalSpacing(6);grid->setVerticalSpacing(4);
        m_explorerLabel=new QLabel(m_categoryFrame);
        m_explorerLabel->setProperty("i18nKey",7310);
        m_explorerLabel->setStyleSheet(QStringLiteral("color:#ff9828;background:transparent;border:0;font-weight:700;"));
        grid->addWidget(m_explorerLabel,0,0,2,1);
        grid->addWidget(categoryButton(7330,QString(),m_categoryFrame),0,1);
        grid->addWidget(categoryButton(7312,QStringLiteral("ecu"),m_categoryFrame),0,2);
        grid->addWidget(categoryButton(7313,QStringLiteral("vehicle"),m_categoryFrame),0,3);
        grid->addWidget(categoryButton(7314,QStringLiteral("dtc"),m_categoryFrame),0,4);
        grid->addWidget(categoryButton(7315,QStringLiteral("command"),m_categoryFrame),0,5);
        grid->addWidget(categoryButton(7316,QStringLiteral("wiring"),m_categoryFrame),1,1);
        grid->addWidget(categoryButton(7317,QStringLiteral("protocol"),m_categoryFrame),1,2);
        grid->addWidget(categoryButton(7318,QStringLiteral("documentation"),m_categoryFrame),1,3);
        grid->addWidget(categoryButton(7319,QStringLiteral("actuator"),m_categoryFrame),1,4);
        grid->addWidget(categoryButton(7320,QStringLiteral("data"),m_categoryFrame),1,5);
        for(int c=1;c<=5;++c) grid->setColumnStretch(c,1);
        const int searchIndex=root->indexOf(m_search->parentWidget());
        int insertAt=searchIndex>=0?searchIndex+1:0;
        if(searchIndex>=0 && root->count()>searchIndex+1){
            QWidget *candidate=root->itemAt(searchIndex+1)->widget();
            if(candidate) insertAt=searchIndex+2;
        }
        root->insertWidget(insertAt,m_categoryFrame);
    }

    void configureResultsTable()
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
        QScrollBar *bar=m_detail->verticalScrollBar();
        if(!bar) return;
        bar->setFixedWidth(15);
        bar->setStyleSheet(QStringLiteral(
            "QScrollBar:vertical{background:#111a21;width:15px;margin:0;border-left:1px solid #485661;}"
            "QScrollBar::handle:vertical{background:#71818d;min-height:36px;border:1px solid #8d9aa3;border-radius:5px;margin:2px;}"
            "QScrollBar::handle:vertical:hover{background:#ff8a22;border-color:#ffad5c;}"
            "QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical{height:0;}"
            "QScrollBar::add-page:vertical,QScrollBar::sub-page:vertical{background:#111a21;}"));
        bar->show();
        bar->update();
    }

    void configureDetailScroll()
    {
        forceDetailScroll();
        QTimer::singleShot(150,this,[this](){forceDetailScroll();});
    }

    void configureDocumentationButtons()
    {
        const QList<QPushButton*> buttons=m_browser->findChildren<QPushButton*>();
        for(QPushButton *button:buttons){
            if(!button) continue;
            if(button->text()==I18n::text(7287) || button->text()==I18n::text(7253)) m_sheetButton=button;
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
            if(!label) continue;
            if(label->text().startsWith(QStringLiteral("MEMS "),Qt::CaseInsensitive)){
                m_currentGenerationLabel=label;
                break;
            }
        }
        updateDocumentControls();
    }

    void configureSearch()
    {
        m_debounce->setSingleShot(true);
        m_debounce->setInterval(110);
        connect(m_debounce,&QTimer::timeout,this,[this](){runSearch();});
        connect(m_search,&QLineEdit::textEdited,this,[this](const QString &){m_activeCategory.clear();m_debounce->start();});
        connect(m_generation,QOverload<int>::of(&QComboBox::currentIndexChanged),this,[this](int){m_activeCategory.clear();m_debounce->start();});

        m_completer=new QCompleter(m_completionModel,this);
        m_completer->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
        m_completer->setCaseSensitivity(Qt::CaseInsensitive);
        m_completer->setMaxVisibleItems(10);
        m_search->setCompleter(m_completer);
        connect(m_completer,QOverload<const QString&>::of(&QCompleter::activated),this,[this](const QString &text){
            m_search->setText(text);m_search->setCursorPosition(text.size());m_activeCategory.clear();runSearch();
        });
    }

    void retranslate()
    {
        if(m_explorerLabel) m_explorerLabel->setText(I18n::text(7310));
        for(QPushButton *button:m_categoryButtons){
            const QVariant key=button->property("i18nKey");
            if(key.isValid()) button->setText(I18n::text(key.toInt()));
        }
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

    void clearSearchWithoutSignals()
    {
        m_search->blockSignals(true);m_search->clear();m_search->blockSignals(false);
    }

    void setGeneration(const QString &token)
    {
        int index=token.isEmpty()?m_generation->findData(QString()):m_generation->findData(token);
        if(index<0&&token.isEmpty()) index=0;
        if(index>=0){m_generation->blockSignals(true);m_generation->setCurrentIndex(index);m_generation->blockSignals(false);}
    }

    QString generationFilter() const
    {
        return m_generation?m_generation->currentData().toString().trimmed():QString();
    }

    bool generationMatches(const QVariantMap &row,const QString &token) const
    {
        if(token.isEmpty()) return true;
        const QString rowGeneration=row.value(QStringLiteral("generation")).toString();
        if(rowGeneration.trimmed().isEmpty()) return true;
        return rowGeneration.contains(token,Qt::CaseInsensitive);
    }

    QVariantList uniqueCombined(const QVariantList &a,const QVariantList &b) const
    {
        QVariantList result=a;
        QSet<QString> seen;
        for(const QVariant &v:a){
            const QVariantMap r=v.toMap();
            seen.insert(r.value(QStringLiteral("source_table")).toString()+QLatin1Char('|')+
                        r.value(QStringLiteral("source_key")).toString()+QLatin1Char('|')+
                        r.value(QStringLiteral("title")).toString());
        }
        for(const QVariant &v:b){
            const QVariantMap r=v.toMap();
            const QString key=r.value(QStringLiteral("source_table")).toString()+QLatin1Char('|')+
                              r.value(QStringLiteral("source_key")).toString()+QLatin1Char('|')+
                              r.value(QStringLiteral("title")).toString();
            if(!seen.contains(key)){seen.insert(key);result.append(v);}
        }
        return result;
    }

    QVariantList fetchResults(const QString &raw,const QString &category) const
    {
        if(category==QStringLiteral("wiring")){
            QVariantList rows=MemsGlobalSearchIndex::search(QString(),QStringLiteral("wiring"),500);
            rows=uniqueCombined(rows,MemsGlobalSearchIndex::search(QStringLiteral("broche"),QStringLiteral("documentation"),100));
            rows=uniqueCombined(rows,MemsGlobalSearchIndex::search(QStringLiteral("connecteur"),QStringLiteral("documentation"),100));
            return rows;
        }
        if(!category.isEmpty()) return MemsGlobalSearchIndex::search(QString(),category,500);
        return MemsGlobalSearchIndex::search(raw,QString(),500);
    }

    void updateSuggestions(const QString &raw,const QVariantList &rows)
    {
        QStringList suggestions;
        QSet<QString> seen;
        if(raw.trimmed().size()>=2){
            const QString needle=normalized(raw);
            for(const QVariant &v:rows){
                const QString title=v.toMap().value(QStringLiteral("title")).toString().trimmed();
                if(title.isEmpty()) continue;
                const QString key=normalized(title);
                if((key.startsWith(needle)||key.contains(needle))&&!seen.contains(key)){
                    seen.insert(key);suggestions.append(title);
                    if(suggestions.size()>=10) break;
                }
            }
        }
        m_completionModel->setStringList(suggestions);
        if(!suggestions.isEmpty()&&m_search->hasFocus()) m_completer->complete();
    }

    void runSearch()
    {
        if(!m_results) return;
        if(m_debounce) m_debounce->stop();
        const QString raw=m_search?m_search->text().trimmed():QString();
        QString category=m_activeCategory;
        if(category.isEmpty()) category=exactCategoryIntent(raw);
        QVariantList fetched=fetchResults(category.isEmpty()?raw:QString(),category);
        const QString generation=generationFilter();
        if(generation.isEmpty()){
            const QString detected=generationFromText(raw);
            if(!detected.isEmpty()) setGeneration(detected);
        }
        const QString effectiveGeneration=generationFilter();

        QVector<QVariantMap> rows;
        rows.reserve(fetched.size());
        for(const QVariant &v:fetched){
            const QVariantMap row=v.toMap();
            if(generationMatches(row,effectiveGeneration)) rows.append(row);
        }
        std::sort(rows.begin(),rows.end(),[raw](const QVariantMap &a,const QVariantMap &b){
            const int pa=categoryPriority(a.value(QStringLiteral("category")).toString(),raw);
            const int pb=categoryPriority(b.value(QStringLiteral("category")).toString(),raw);
            if(pa!=pb) return pa<pb;
            const QString ta=a.value(QStringLiteral("title")).toString();
            const QString tb=b.value(QStringLiteral("title")).toString();
            return QString::localeAwareCompare(ta,tb)<0;
        });

        populate(rows);
        if(m_count) m_count->setText(I18n::text(7324).arg(QString::number(rows.size())));
        QVariantList suggestionRows;for(const QVariantMap &r:rows) suggestionRows.append(r);
        updateSuggestions(raw,suggestionRows);
        if(rows.isEmpty()){
            m_detail->setHtml(htmlStyle()+QStringLiteral("<h1>%1</h1><p class='muted'>%2</p>")
                .arg(I18n::text(7216).toHtmlEscaped(),I18n::text(7328).toHtmlEscaped()));
            forceDetailScroll();
        }else{
            m_results->selectRow(0);
            showSelectedResult();
        }
    }

    void populate(const QVector<QVariantMap> &rows)
    {
        m_results->blockSignals(true);
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
            if(category==QStringLiteral("ecu")) titleItem->setData(Qt::UserRole,title);
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
        const QVariantMap row=selectedResult();
        QString generation=row.value(QStringLiteral("generation")).toString();
        if(!generation.isEmpty()) return generation;
        generation=generationFilter();
        if(!generation.isEmpty()) return generation;
        return generationFromText(m_search?m_search->text():QString());
    }

    void updateDocumentControls()
    {
        const QString generation=currentGeneration();
        if(m_currentGenerationLabel) m_currentGenerationLabel->setText(generation.isEmpty()?QStringLiteral("—"):QStringLiteral("MEMS %1").arg(generation));
        const QString xml=xmlPath(generation);
        const QString pdf=pdfPath(generation);
        if(m_sheetButton) m_sheetButton->setEnabled(!xml.isEmpty()&&QFileInfo::exists(xml));
        if(m_pdfButton){const bool exists=!pdf.isEmpty()&&QFileInfo::exists(pdf);m_pdfButton->setEnabled(exists);m_pdfButton->setVisible(true);}
    }

    void showSelectedResult()
    {
        const QVariantMap row=selectedResult();
        if(row.isEmpty()) return;
        const QString category=row.value(QStringLiteral("category")).toString();
        const QString source=row.value(QStringLiteral("source_table")).toString();
        const QString generation=row.value(QStringLiteral("generation")).toString();
        if(category==QStringLiteral("ecu")){
            QMetaObject::invokeMethod(m_browser.data(),"showSelectedEcu",Qt::DirectConnection);
            updateDocumentControls();
            QTimer::singleShot(0,this,[this](){forceDetailScroll();});
            return;
        }
        if(category==QStringLiteral("documentation") && source==QStringLiteral("xml_documentation") && !generation.isEmpty()){
            QMetaObject::invokeMethod(m_browser.data(),"showGenerationSheet",Qt::DirectConnection);
            updateDocumentControls();
            QTimer::singleShot(0,this,[this](){forceDetailScroll();});
            return;
        }
        const QString title=row.value(QStringLiteral("title")).toString();
        QString content=row.value(QStringLiteral("content")).toString().toHtmlEscaped();
        content.replace(QStringLiteral("\n"),QStringLiteral("<br>"));
        QString subtitle=categoryLabel(category).toHtmlEscaped();
        if(!generation.isEmpty()) subtitle+=QStringLiteral(" — MEMS ")+generation.toHtmlEscaped();
        m_detail->setHtml(htmlStyle()+QStringLiteral("<h1>%1</h1><p class='muted'>%2</p><div class='content'>%3</div>")
            .arg(title.toHtmlEscaped(),subtitle,content));
        updateDocumentControls();
        forceDetailScroll();
    }

    void openXmlDialog()
    {
        const QString generation=currentGeneration();
        const QString path=xmlPath(generation);
        if(path.isEmpty()||!QFileInfo::exists(path)) return;
        QDialog dialog(m_browser);
        dialog.setWindowTitle(I18n::text(7325)+QStringLiteral(" — MEMS ")+generation);
        dialog.resize(940,680);
        QVBoxLayout *layout=new QVBoxLayout(&dialog);
        layout->setContentsMargins(8,8,8,8);
        QTextBrowser *view=new QTextBrowser(&dialog);
        view->setHtml(renderXml(path));
        view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        view->setStyleSheet(QStringLiteral(
            "QTextBrowser{background:#0a1015;color:#dce3e8;border:1px solid #34414b;}"
            "QScrollBar:vertical{background:#111a21;width:15px;border-left:1px solid #485661;}"
            "QScrollBar::handle:vertical{background:#71818d;min-height:36px;border:1px solid #8d9aa3;border-radius:5px;margin:2px;}"
            "QScrollBar::handle:vertical:hover{background:#ff8a22;}QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical{height:0;}"));
        layout->addWidget(view);
        dialog.exec();
    }

private:
    QPointer<QWidget> m_browser;
    QLineEdit *m_search;
    QComboBox *m_generation;
    QTableWidget *m_results;
    QTextBrowser *m_detail;
    QLabel *m_count;
    QTimer *m_debounce;
    QStringListModel *m_completionModel;
    QCompleter *m_completer;
    QFrame *m_categoryFrame;
    QLabel *m_explorerLabel=nullptr;
    QList<QPushButton*> m_categoryButtons;
    QPointer<QPushButton> m_allEcuButton;
    QPointer<QPushButton> m_generationButtons[4];
    QPushButton *m_sheetButton;
    QPushButton *m_pdfButton;
    QLabel *m_currentGenerationLabel;
    QString m_activeCategory;
};

class GlobalDatabaseInstaller : public QObject
{
public:
    explicit GlobalDatabaseInstaller(QObject *parent=nullptr):QObject(parent){}

    bool eventFilter(QObject *watched,QEvent *event) override
    {
        QWidget *widget=qobject_cast<QWidget*>(watched);
        if(widget&&event&&(event->type()==QEvent::Show||event->type()==QEvent::Polish)&&widget->objectName()==QStringLiteral("memsDatabaseBrowser"))
            schedule(widget);
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
        if(!widget||widget->property("databaseGlobalUiPatched").toBool()) return;
        widget->setProperty("databaseGlobalUiPatched",true);
        QPointer<QWidget> guarded(widget);
        QTimer::singleShot(650,this,[guarded](){if(guarded) new GlobalDatabaseController(guarded);});
    }
};

void installGlobalDatabaseUi()
{
    QCoreApplication *core=QCoreApplication::instance();
    if(!core) return;
    GlobalDatabaseInstaller *installer=new GlobalDatabaseInstaller(core);
    core->installEventFilter(installer);
    QTimer::singleShot(1800,installer,[installer](){installer->scan();});
}

}

Q_COREAPP_STARTUP_FUNCTION(installGlobalDatabaseUi)
