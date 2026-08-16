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
#include <QFont>
#include <QFrame>
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
#include <QStandardPaths>
#include <QStringListModel>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTextBrowser>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>
#include <QVector>
#include <QXmlStreamReader>

#include <algorithm>
#include <climits>

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
        }else if(!previousSpace && !output.isEmpty()){
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

QString htmlStyle()
{
    return QStringLiteral(
        "<style>body{background:#0a1015;color:#dce3e8;font-family:'Segoe UI',Arial,sans-serif;font-size:9pt;}"
        "h1{color:#ff9828;font-size:16pt;margin:0 0 5px 0;}"
        "h2{color:#ff9828;font-size:10.5pt;border-bottom:1px solid #34414b;padding-bottom:4px;margin-top:14px;}"
        "p{margin:4px 0 7px 0;line-height:1.35}.muted{color:#94a1ab}.note{background:#15100b;border:1px solid #60401f;color:#ffd0a0;padding:7px;}"
        "table{border-collapse:collapse;width:100%;margin:5px 0 8px 0}th{background:#151e25;color:#ff9828;border-bottom:2px solid #ff7a00;text-align:left;padding:5px}"
        "td{border-bottom:1px solid #26323b;padding:5px;vertical-align:top}</style>");
}

QString generationTokenFromSystem(const QString &system)
{
    if(system.contains(QStringLiteral("1.9"))) return QStringLiteral("1.9");
    if(system.contains(QStringLiteral("1.6"))) return QStringLiteral("1.6");
    if(system.contains(QStringLiteral("1.3"))) return QStringLiteral("1.3");
    if(system.contains(QStringLiteral("1.2"))) return QStringLiteral("1.2");
    return QString();
}

QString detectedGeneration(const QString &raw)
{
    const QString normalized=normalizedSearchText(raw);
    if(normalized.contains(QStringLiteral("1 9"))) return QStringLiteral("1.9");
    if(normalized.contains(QStringLiteral("1 6"))) return QStringLiteral("1.6");
    if(normalized.contains(QStringLiteral("1 3"))) return QStringLiteral("1.3");
    if(normalized.contains(QStringLiteral("1 2"))) return QStringLiteral("1.2");
    return QString();
}

bool isDocumentWord(const QString &word)
{
    static const QSet<QString> words=QSet<QString>()
        << normalizedSearchText(QStringLiteral("fiche"))
        << normalizedSearchText(QStringLiteral("fiches"))
        << normalizedSearchText(QStringLiteral("documentation"))
        << normalizedSearchText(QStringLiteral("document"))
        << normalizedSearchText(QStringLiteral("documents"))
        << normalizedSearchText(QStringLiteral("xml"))
        << normalizedSearchText(QStringLiteral("pdf"))
        << normalizedSearchText(QStringLiteral("sheet"))
        << normalizedSearchText(QStringLiteral("sheets"))
        << normalizedSearchText(QStringLiteral("ficha"))
        << normalizedSearchText(QStringLiteral("fichas"))
        << normalizedSearchText(QStringLiteral("scheda"))
        << normalizedSearchText(QStringLiteral("schede"))
        << normalizedSearchText(QStringLiteral("datenblatt"));
    return words.contains(word);
}

bool isEcuWord(const QString &word)
{
    static const QSet<QString> words=QSet<QString>()
        << normalizedSearchText(QStringLiteral("ecu"))
        << normalizedSearchText(QStringLiteral("ecus"))
        << normalizedSearchText(QStringLiteral("calculateur"))
        << normalizedSearchText(QStringLiteral("calculateurs"));
    return words.contains(word);
}

bool isGenericWord(const QString &word)
{
    static const QSet<QString> words=QSet<QString>()
        << normalizedSearchText(QStringLiteral("ecu"))
        << normalizedSearchText(QStringLiteral("ecus"))
        << normalizedSearchText(QStringLiteral("mems"))
        << normalizedSearchText(QStringLiteral("base"))
        << normalizedSearchText(QStringLiteral("database"))
        << normalizedSearchText(QStringLiteral("datenbank"))
        << normalizedSearchText(QStringLiteral("recherche"))
        << normalizedSearchText(QStringLiteral("search"))
        << normalizedSearchText(QStringLiteral("chercher"))
        << normalizedSearchText(QStringLiteral("buscar"))
        << normalizedSearchText(QStringLiteral("cerca"))
        << normalizedSearchText(QStringLiteral("pesquisar"))
        << normalizedSearchText(QStringLiteral("suche"))
        << normalizedSearchText(QStringLiteral("calculateur"))
        << normalizedSearchText(QStringLiteral("calculateurs"))
        << normalizedSearchText(QStringLiteral("generation"))
        << normalizedSearchText(QStringLiteral("generations"))
        << normalizedSearchText(QStringLiteral("génération"))
        << normalizedSearchText(QStringLiteral("générations"))
        << normalizedSearchText(QStringLiteral("generazione"))
        << normalizedSearchText(QStringLiteral("generación"))
        << normalizedSearchText(QStringLiteral("geração"))
        << normalizedSearchText(QStringLiteral("vehicule"))
        << normalizedSearchText(QStringLiteral("véhicule"))
        << normalizedSearchText(QStringLiteral("vehicle"))
        << normalizedSearchText(QStringLiteral("modelo"))
        << normalizedSearchText(QStringLiteral("modele"))
        << normalizedSearchText(QStringLiteral("modèle"))
        << normalizedSearchText(QStringLiteral("model"))
        << normalizedSearchText(QStringLiteral("moteur"))
        << normalizedSearchText(QStringLiteral("motor"))
        << normalizedSearchText(QStringLiteral("engine"))
        << normalizedSearchText(QStringLiteral("marche"))
        << normalizedSearchText(QStringLiteral("marché"))
        << normalizedSearchText(QStringLiteral("market"))
        << normalizedSearchText(QStringLiteral("protocole"))
        << normalizedSearchText(QStringLiteral("protocol"))
        << normalizedSearchText(QStringLiteral("commande"))
        << normalizedSearchText(QStringLiteral("commandes"))
        << normalizedSearchText(QStringLiteral("commands"))
        << normalizedSearchText(QStringLiteral("dtc"))
        << normalizedSearchText(QStringLiteral("code"))
        << normalizedSearchText(QStringLiteral("codes"));
    if(words.contains(word)) return true;
    return isDocumentWord(word);
}

struct QuerySpec
{
    QString generation;
    QStringList terms;
    bool documentIntent=false;
    bool ecuIntent=false;
};

QuerySpec parseQuery(const QString &raw)
{
    QuerySpec spec;
    spec.generation=detectedGeneration(raw);
    QString normalized=normalizedSearchText(raw);
    if(!spec.generation.isEmpty()){
        const QString suffix=spec.generation.right(1);
        normalized.replace(QStringLiteral("1 ")+suffix,QStringLiteral(" "));
    }
    const QStringList rawTerms=normalized.split(QLatin1Char(' '),Qt::SkipEmptyParts);
    for(const QString &term:rawTerms){
        if(isDocumentWord(term)) spec.documentIntent=true;
        if(isEcuWord(term)) spec.ecuIntent=true;
        if(!isGenericWord(term)) spec.terms.append(term);
    }
    return spec;
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
    QString searchable;
};

class DatabaseBrowserController : public QObject
{
public:
    explicit DatabaseBrowserController(QWidget *browser)
        : QObject(browser),m_browser(browser),m_search(nullptr),m_generation(nullptr),m_results(nullptr),m_detail(nullptr),
          m_resultCount(nullptr),m_completionModel(nullptr),m_completer(nullptr),m_debounce(nullptr),m_quickAccess(nullptr),
          m_quickLabel(nullptr),m_allButton(nullptr),m_documentLabel(nullptr),m_currentGenerationLabel(nullptr),
          m_sheetButton(nullptr),m_pdfButton(nullptr)
    {
        if(!m_browser) return;
        m_search=m_browser->findChild<QLineEdit*>();
        const QList<QComboBox*> combos=m_browser->findChildren<QComboBox*>();
        if(!combos.isEmpty()) m_generation=combos.first();
        m_results=m_browser->findChild<QTableWidget*>();
        m_detail=m_browser->findChild<QTextBrowser*>();
        locateResultCount();
        openDatabase();
        loadCache();
        disconnectLegacySearch();
        installScrollBar();
        installCompleter();
        installQuickAccess();
        installDocumentTools();
        installDebounce();
        connectSignals();
        m_browser->installEventFilter(this);
        QTimer::singleShot(0,this,[this](){updateTranslations();refreshNow();});
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
            QTimer::singleShot(0,this,[this](){updateTranslations();refreshNow();});
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
        m_connectionName=QStringLiteral("MEMS_FAST_EXPLORER_%1").arg(reinterpret_cast<quintptr>(this));
        m_database=QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"),m_connectionName);
        m_database.setConnectOptions(QStringLiteral("QSQLITE_OPEN_READONLY"));
        m_database.setDatabaseName(dir.filePath(databases.first()));
        if(m_database.open()){
            QSqlQuery query(m_database);
            query.exec(QStringLiteral("PRAGMA query_only = ON"));
        }
    }

    void addCandidate(const QString &raw)
    {
        const QString cleaned=raw.trimmed();
        if(cleaned.size()<2 || cleaned.size()>100) return;
        const QString key=normalizedSearchText(cleaned);
        if(key.size()<2 || m_candidateKeys.contains(key)) return;
        m_candidateKeys.insert(key);
        m_candidates.append(cleaned);
    }

    void addCandidateParts(const QString &raw)
    {
        const QString cleaned=raw.trimmed();
        if(cleaned.isEmpty()) return;
        addCandidate(cleaned);
        const QStringList parts=cleaned.split(QRegularExpression(QStringLiteral("[|,;/()\\[\\]{}\\n\\r]+")),Qt::SkipEmptyParts);
        for(const QString &part:parts){
            const QString trimmed=part.trimmed();
            addCandidate(trimmed);
            const QStringList words=trimmed.split(QRegularExpression(QStringLiteral("\\s+")),Qt::SkipEmptyParts);
            for(const QString &word:words) addCandidate(word);
        }
    }

    void loadCache()
    {
        m_rows.clear();
        m_candidates.clear();
        m_candidateKeys.clear();
        if(!m_database.isOpen()) return;
        QSqlQuery query(m_database);
        const QString sql=QStringLiteral(
            "SELECT e.part_number,e.brand,e.system_family,e.injection,e.makes,e.models,e.year_from,e.year_to,e.notes,"
            "COALESCE((SELECT GROUP_CONCAT(COALESCE(f.make,'') || ' ' || COALESCE(f.model,'') || ' ' || COALESCE(f.variant,'') || ' ' || COALESCE(f.engine,'') || ' ' || COALESCE(f.market,'') || ' ' || COALESCE(f.alt_refs,''),' | ') "
            "FROM ecu_fitment_target f WHERE f.part_number=e.part_number),'') FROM ecu_target e ORDER BY e.system_family,e.part_number");
        if(query.exec(sql)){
            while(query.next()){
                SearchRow row;
                row.part=query.value(0).toString().trimmed();row.brand=query.value(1).toString().trimmed();row.system=query.value(2).toString().trimmed();
                row.injection=query.value(3).toString().trimmed();row.makes=query.value(4).toString().trimmed();row.models=query.value(5).toString().trimmed();
                row.yearFrom=query.value(6).toString().trimmed();row.yearTo=query.value(7).toString().trimmed();row.notes=query.value(8).toString().trimmed();row.fitments=query.value(9).toString().trimmed();
                row.searchable=normalizedSearchText(row.part+QLatin1Char(' ')+row.brand+QLatin1Char(' ')+row.system+QLatin1Char(' ')+row.injection+QLatin1Char(' ')+row.makes+QLatin1Char(' ')+row.models+QLatin1Char(' ')+row.notes+QLatin1Char(' ')+row.fitments);
                m_rows.append(row);
                addCandidateParts(row.part);addCandidateParts(row.brand);addCandidateParts(row.system);addCandidateParts(row.injection);addCandidateParts(row.makes);addCandidateParts(row.models);
            }
        }
        QSqlQuery fit(m_database);
        if(fit.exec(QStringLiteral("SELECT make,model,variant,engine,market,alt_refs FROM ecu_fitment_target")))
            while(fit.next()) for(int i=0;i<6;++i) addCandidateParts(fit.value(i).toString());
        std::sort(m_candidates.begin(),m_candidates.end(),[](const QString &a,const QString &b){return QString::localeAwareCompare(a.toCaseFolded(),b.toCaseFolded())<0;});
    }

    void disconnectLegacySearch()
    {
        if(m_search) QObject::disconnect(m_search,nullptr,m_browser.data(),nullptr);
        if(m_generation) QObject::disconnect(m_generation,nullptr,m_browser.data(),nullptr);
    }

    void installScrollBar()
    {
        if(!m_detail) return;
        m_detail->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        m_detail->verticalScrollBar()->setStyleSheet(QStringLiteral("QScrollBar:vertical{background:#0c1217;width:12px;margin:0;border-left:1px solid #27323b;}QScrollBar::handle:vertical{background:#53616c;min-height:32px;border-radius:5px;margin:2px;}QScrollBar::handle:vertical:hover{background:#ff8a22;}QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical{height:0;}QScrollBar::add-page:vertical,QScrollBar::sub-page:vertical{background:transparent;}"));
    }

    void installCompleter()
    {
        if(!m_search) return;
        m_completionModel=new QStringListModel(this);
        m_completer=new QCompleter(m_completionModel,this);
        m_completer->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
        m_completer->setCaseSensitivity(Qt::CaseInsensitive);
        m_completer->setMaxVisibleItems(10);
        m_search->setCompleter(m_completer);
        connect(m_completer,QOverload<const QString &>::of(&QCompleter::activated),this,[this](const QString &text){
            if(!m_search) return;
            m_search->setText(text);m_search->setCursorPosition(text.size());
            if(m_completer->popup()) m_completer->popup()->hide();
            refreshNow();
        });
    }

    QPushButton *quickButton(QWidget *parent)
    {
        QPushButton *button=new QPushButton(parent);
        button->setMinimumHeight(28);
        button->setStyleSheet(QStringLiteral("QPushButton{background:#111920;color:#e7ecef;border:1px solid #3b4852;border-radius:4px;padding:3px 10px;font-weight:600;}QPushButton:hover{border-color:#ff7a00;color:#ffad5c;}QPushButton:pressed{background:#2a1d11;}"));
        return button;
    }

    void installQuickAccess()
    {
        if(!m_browser||!m_search) return;
        QVBoxLayout *root=qobject_cast<QVBoxLayout*>(m_browser->layout());
        if(!root) return;
        m_quickAccess=new QFrame(m_browser);m_quickAccess->setAttribute(Qt::WA_StyledBackground,true);m_quickAccess->setStyleSheet(QStringLiteral("QFrame{background:#0d141a;border:1px solid #29343e;border-radius:5px;}"));
        QHBoxLayout *layout=new QHBoxLayout(m_quickAccess);layout->setContentsMargins(9,4,9,4);layout->setSpacing(6);
        m_quickLabel=new QLabel(m_quickAccess);m_quickLabel->setStyleSheet(QStringLiteral("color:#ff9828;background:transparent;border:0;font-weight:700;"));layout->addWidget(m_quickLabel);
        m_allButton=quickButton(m_quickAccess);layout->addWidget(m_allButton);
        for(int i=0;i<4;++i){m_generationButtons[i]=quickButton(m_quickAccess);layout->addWidget(m_generationButtons[i]);}
        layout->addStretch(1);
        const int searchIndex=root->indexOf(m_search->parentWidget());if(searchIndex>=0) root->insertWidget(searchIndex+1,m_quickAccess);
        connect(m_allButton,&QPushButton::clicked,this,[this](){setGenerationFilter(QString());if(m_search){m_search->blockSignals(true);m_search->clear();m_search->blockSignals(false);}refreshNow();});
        const QString tokens[]={QStringLiteral("1.2"),QStringLiteral("1.3"),QStringLiteral("1.6"),QStringLiteral("1.9")};
        for(int i=0;i<4;++i){const QString token=tokens[i];connect(m_generationButtons[i],&QPushButton::clicked,this,[this,token](){setGenerationFilter(token);if(m_search){m_search->blockSignals(true);m_search->setText(generationLabel(token));m_search->setCursorPosition(m_search->text().size());m_search->blockSignals(false);}refreshNow();});}
    }

    void installDocumentTools()
    {
        if(!m_detail||!m_detail->parentWidget()) return;
        QWidget *detailCard=m_detail->parentWidget();QVBoxLayout *vertical=qobject_cast<QVBoxLayout*>(detailCard->layout());if(!vertical||vertical->count()<1) return;
        QHBoxLayout *tools=qobject_cast<QHBoxLayout*>(vertical->itemAt(0)->layout());if(!tools) return;
        QList<QPushButton*> buttons;for(int i=0;i<tools->count();++i) if(QPushButton *button=qobject_cast<QPushButton*>(tools->itemAt(i)->widget())) buttons.append(button);
        if(buttons.size()<2) return;m_sheetButton=buttons.at(0);m_pdfButton=buttons.at(1);
        QObject::disconnect(m_sheetButton,nullptr,m_browser.data(),nullptr);QObject::disconnect(m_pdfButton,nullptr,m_browser.data(),nullptr);
        m_documentLabel=new QLabel(detailCard);m_documentLabel->setStyleSheet(QStringLiteral("color:#ff9828;background:transparent;border:0;font-weight:700;"));
        m_currentGenerationLabel=new QLabel(detailCard);m_currentGenerationLabel->setStyleSheet(QStringLiteral("color:#e7ecef;background:#172027;border:1px solid #46525c;border-radius:4px;padding:4px 8px;font-weight:700;"));
        const int sheetIndex=tools->indexOf(m_sheetButton);const int insertAt=sheetIndex>=0?sheetIndex:tools->count();tools->insertWidget(insertAt,m_documentLabel);tools->insertWidget(insertAt+1,m_currentGenerationLabel);
        connect(m_sheetButton,&QPushButton::clicked,this,[this](){const QString token=currentDocumentGeneration();if(!token.isEmpty()) showGenerationPage(token);});
        connect(m_pdfButton,&QPushButton::clicked,this,[this](){const QString path=pdfPath(currentDocumentGeneration());if(!path.isEmpty()&&QFileInfo::exists(path)) QDesktopServices::openUrl(QUrl::fromLocalFile(path));});
    }

    void installDebounce(){m_debounce=new QTimer(this);m_debounce->setSingleShot(true);m_debounce->setInterval(90);connect(m_debounce,&QTimer::timeout,this,[this](){refreshNow();});}

    void connectSignals()
    {
        if(m_search) connect(m_search,&QLineEdit::textChanged,this,[this](const QString &text){updateCompleter(text);if(m_debounce) m_debounce->start();});
        if(m_generation) connect(m_generation,QOverload<int>::of(&QComboBox::currentIndexChanged),this,[this](int){if(m_debounce) m_debounce->start();});
        if(m_results) connect(m_results,&QTableWidget::itemSelectionChanged,this,[this](){syncDocumentGenerationFromSelection();});
    }

    QString generationLabel(const QString &token) const
    {
        if(token==QStringLiteral("1.2")) return I18n::text(7291);if(token==QStringLiteral("1.3")) return I18n::text(7292);if(token==QStringLiteral("1.6")) return I18n::text(7293);if(token==QStringLiteral("1.9")) return I18n::text(7294);return QString();
    }

    void updateTranslations()
    {
        if(m_search){m_search->setPlaceholderText(I18n::text(7217));m_search->setToolTip(I18n::text(7286));}
        if(m_quickLabel) m_quickLabel->setText(I18n::text(7289));if(m_allButton) m_allButton->setText(I18n::text(7290));
        if(m_generationButtons[0]) m_generationButtons[0]->setText(I18n::text(7291));if(m_generationButtons[1]) m_generationButtons[1]->setText(I18n::text(7292));if(m_generationButtons[2]) m_generationButtons[2]->setText(I18n::text(7293));if(m_generationButtons[3]) m_generationButtons[3]->setText(I18n::text(7294));
        if(m_documentLabel) m_documentLabel->setText(I18n::text(7288));if(m_sheetButton) m_sheetButton->setText(I18n::text(7287));if(m_pdfButton) m_pdfButton->setText(I18n::text(7261));updateDocumentLabels();
    }

    QString comboGeneration() const{return m_generation?m_generation->currentData().toString().trimmed():QString();}

    void setGenerationFilter(const QString &token)
    {
        if(!m_generation) return;int index=token.isEmpty()?m_generation->findData(QString()):m_generation->findData(token);if(index<0&&token.isEmpty()) index=0;
        if(index>=0&&m_generation->currentIndex()!=index){m_generation->blockSignals(true);m_generation->setCurrentIndex(index);m_generation->blockSignals(false);}if(!token.isEmpty()) m_documentGeneration=token;updateDocumentLabels();
    }

    bool rowMatchesGeneration(const SearchRow &row,const QString &token) const{return token.isEmpty()||generationTokenFromSystem(row.system)==token;}
    bool rowMatchesTerms(const SearchRow &row,const QStringList &terms) const{for(const QString &term:terms) if(!row.searchable.contains(term)) return false;return true;}

    QStringList suggestionsFor(const QString &typed) const
    {
        const QuerySpec spec=parseQuery(typed);QString needle;if(!spec.terms.isEmpty()) needle=spec.terms.last();else{const QString normalized=normalizedSearchText(typed);const QStringList parts=normalized.split(QLatin1Char(' '),Qt::SkipEmptyParts);if(!parts.isEmpty()&&!isGenericWord(parts.last())) needle=parts.last();}
        if(needle.size()<2) return QStringList();QStringList result;
        for(const QString &candidate:m_candidates){const QString normalized=normalizedSearchText(candidate);bool starts=normalized.startsWith(needle);if(!starts){const QStringList words=normalized.split(QLatin1Char(' '),Qt::SkipEmptyParts);for(const QString &word:words) if(word.startsWith(needle)){starts=true;break;}}if(starts){result.append(candidate);if(result.size()>=10) break;}}
        return result;
    }

    void updateCompleter(const QString &typed)
    {
        if(!m_completionModel||!m_completer) return;const QStringList suggestions=suggestionsFor(typed);m_completionModel->setStringList(suggestions);
        if(suggestions.isEmpty()||typed.trimmed().size()<2){if(m_completer->popup()) m_completer->popup()->hide();return;}if(m_search&&m_search->hasFocus()) m_completer->complete();
    }

    QString nearestCandidate(const QString &term) const
    {
        if(term.size()<3) return QString();int best=INT_MAX;QString bestText;
        for(const QString &candidate:m_candidates){const QString normalized=normalizedSearchText(candidate);int distance=levenshteinDistance(term,normalized);const QStringList words=normalized.split(QLatin1Char(' '),Qt::SkipEmptyParts);for(const QString &word:words) distance=qMin(distance,levenshteinDistance(term,word));if(distance<best){best=distance;bestText=candidate;}}
        const int allowed=term.size()<=4?1:(term.size()<=8?2:3);return best<=allowed?bestText:QString();
    }

    QString selectedPartNumber() const{if(!m_results||m_results->currentRow()<0) return QString();QTableWidgetItem *item=m_results->item(m_results->currentRow(),0);return item?item->data(Qt::UserRole).toString():QString();}

    void populateResults(const QVector<const SearchRow*> &rows,bool selectFirst)
    {
        if(!m_results) return;const QString previous=selectedPartNumber();m_results->blockSignals(true);m_results->setRowCount(rows.size());int restore=-1;
        for(int r=0;r<rows.size();++r){const SearchRow &row=*rows.at(r);const QString columns[]={row.part,row.system,row.injection,row.models,yearsText(row.yearFrom,row.yearTo)};for(int c=0;c<5;++c){QTableWidgetItem *item=new QTableWidgetItem(columns[c]);if(c==0){item->setData(Qt::UserRole,row.part);QFont font=item->font();font.setBold(true);item->setFont(font);}m_results->setItem(r,c,item);}m_results->setRowHeight(r,27);if(row.part==previous) restore=r;}
        m_results->blockSignals(false);m_results->clearSelection();if(selectFirst&&!rows.isEmpty()) m_results->selectRow(restore>=0?restore:0);
    }

    void setResultStatus(int count,const QString &correction=QString())
    {
        if(!m_resultCount) return;QString text=QStringLiteral("%1: %2").arg(I18n::text(7254),QString::number(count));if(!correction.isEmpty()) text+=QStringLiteral(" — ")+I18n::text(7285).arg(correction);m_resultCount->setText(text);m_resultCount->setToolTip(correction.isEmpty()?QString():I18n::text(7285).arg(correction));
    }

    void refreshNow()
    {
        if(!m_results) return;if(m_debounce) m_debounce->stop();const QString raw=m_search?m_search->text().trimmed():QString();QuerySpec spec=parseQuery(raw);QString generation=spec.generation.isEmpty()?comboGeneration():spec.generation;if(!spec.generation.isEmpty()) setGenerationFilter(spec.generation);
        QVector<const SearchRow*> matches;for(const SearchRow &row:m_rows) if(rowMatchesGeneration(row,generation)&&rowMatchesTerms(row,spec.terms)) matches.append(&row);
        QString correction;if(matches.isEmpty()&&spec.terms.size()==1){const QString nearest=nearestCandidate(spec.terms.first());if(!nearest.isEmpty()){const QString corrected=normalizedSearchText(nearest);for(const SearchRow &row:m_rows) if(rowMatchesGeneration(row,generation)&&row.searchable.contains(corrected)) matches.append(&row);if(!matches.isEmpty()) correction=nearest;}}
        const bool generationLanding=!generation.isEmpty()&&spec.terms.isEmpty();const bool documentationLanding=spec.documentIntent&&spec.generation.isEmpty()&&spec.terms.isEmpty();const bool selectFirst=!generationLanding&&!documentationLanding&&!matches.isEmpty();
        populateResults(matches,selectFirst);setResultStatus(matches.size(),correction);
        if(generationLanding){m_documentGeneration=generation;updateDocumentLabels();showGenerationPage(generation);}else if(documentationLanding){showDocumentationHome();}else if(matches.isEmpty()){if(m_detail){m_detail->setHtml(htmlStyle()+QStringLiteral("<h1>%1</h1><p class='muted'>%2</p>").arg(I18n::text(7216).toHtmlEscaped(),I18n::text(7303).toHtmlEscaped()));m_detail->verticalScrollBar()->setValue(0);}}else if(selectFirst){syncDocumentGenerationFromSelection();}
    }

    void syncDocumentGenerationFromSelection(){if(!m_results||m_results->currentRow()<0) return;QTableWidgetItem *item=m_results->item(m_results->currentRow(),1);if(!item) return;const QString token=generationTokenFromSystem(item->text());if(!token.isEmpty()) m_documentGeneration=token;updateDocumentLabels();}

    QString currentDocumentGeneration() const
    {
        if(!m_documentGeneration.isEmpty()) return m_documentGeneration;const QString combo=comboGeneration();if(!combo.isEmpty()) return combo;if(m_results&&m_results->currentRow()>=0){QTableWidgetItem *item=m_results->item(m_results->currentRow(),1);if(item) return generationTokenFromSystem(item->text());}return QString();
    }

    void updateDocumentLabels()
    {
        const QString token=currentDocumentGeneration();if(m_currentGenerationLabel){const QString label=generationLabel(token);m_currentGenerationLabel->setText(label.isEmpty()?I18n::text(7218):label);}const QString xml=xmlPath(token);const QString pdf=pdfPath(token);if(m_sheetButton) m_sheetButton->setEnabled(!xml.isEmpty()&&QFileInfo::exists(xml));if(m_pdfButton){const bool available=!pdf.isEmpty()&&QFileInfo::exists(pdf);m_pdfButton->setEnabled(available);m_pdfButton->setVisible(true);}
    }

    QString xmlPath(const QString &token) const
    {
        const QString root=QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)+QStringLiteral("/reference/fiches/");if(token==QStringLiteral("1.3")) return root+QStringLiteral("mems_1_3.xml");if(token==QStringLiteral("1.6")) return root+QStringLiteral("mems_1_6.xml");if(token==QStringLiteral("1.9")) return root+QStringLiteral("mems_1_9.xml");return QString();
    }

    QString pdfPath(const QString &token) const
    {
        const QString root=QCoreApplication::applicationDirPath()+QStringLiteral("/database/reference/pdf/");if(token==QStringLiteral("1.3")) return root+QStringLiteral("mems1.3_source.pdf");if(token==QStringLiteral("1.6")) return root+QStringLiteral("mems1.6_source.pdf");if(token==QStringLiteral("1.9")) return root+QStringLiteral("mems1.9_source.pdf");return QString();
    }

    QString renderXml(const QString &path) const
    {
        QFile file(path);if(!file.open(QIODevice::ReadOnly|QIODevice::Text)) return htmlStyle()+QStringLiteral("<p class='note'>%1</p>").arg(I18n::text(7232).toHtmlEscaped());QXmlStreamReader xml(&file);QString html=htmlStyle();bool firstRow=true;
        while(!xml.atEnd()){xml.readNext();if(xml.isEndElement()){if(xml.name()==QStringLiteral("ligne")){html+=QStringLiteral("</tr>");firstRow=false;}else if(xml.name()==QStringLiteral("table")) html+=QStringLiteral("</table>");continue;}if(!xml.isStartElement()) continue;const QStringRef name=xml.name();if(name==QStringLiteral("titre")) html+=QStringLiteral("<h1>%1</h1>").arg(xml.readElementText(QXmlStreamReader::IncludeChildElements).toHtmlEscaped());else if(name==QStringLiteral("sous-titre")) html+=QStringLiteral("<p class='muted'>%1</p>").arg(xml.readElementText(QXmlStreamReader::IncludeChildElements).toHtmlEscaped());else if(name==QStringLiteral("section")) html+=QStringLiteral("<h2>%1</h2>").arg(xml.attributes().value(QStringLiteral("titre")).toString().toHtmlEscaped());else if(name==QStringLiteral("p")){QString text=xml.readElementText(QXmlStreamReader::IncludeChildElements).toHtmlEscaped();text.replace(QStringLiteral("\n"),QStringLiteral("<br>"));html+=QStringLiteral("<p>%1</p>").arg(text);}else if(name==QStringLiteral("note")){QString text=xml.readElementText(QXmlStreamReader::IncludeChildElements).toHtmlEscaped();text.replace(QStringLiteral("\n"),QStringLiteral("<br>"));html+=QStringLiteral("<div class='note'>%1</div>").arg(text);}else if(name==QStringLiteral("table")){firstRow=true;html+=QStringLiteral("<table>");}else if(name==QStringLiteral("ligne")) html+=QStringLiteral("<tr>");else if(name==QStringLiteral("cellule")){const QString text=xml.readElementText(QXmlStreamReader::IncludeChildElements).trimmed().toHtmlEscaped();const QString tag=firstRow?QStringLiteral("th"):QStringLiteral("td");html+=QStringLiteral("<%1>%2</%1>").arg(tag,text);}}
        return html;
    }

    int generationCount(const QString &token) const{int count=0;for(const SearchRow &row:m_rows) if(rowMatchesGeneration(row,token)) ++count;return count;}

    void showGenerationPage(const QString &token)
    {
        if(!m_detail) return;m_documentGeneration=token;updateDocumentLabels();const QString xml=xmlPath(token);if(!xml.isEmpty()&&QFileInfo::exists(xml)){m_detail->setHtml(renderXml(xml));}else{QString html=htmlStyle();html+=QStringLiteral("<h1>%1</h1>").arg(I18n::text(7296).arg(generationLabel(token)).toHtmlEscaped());html+=QStringLiteral("<p>%1</p>").arg(I18n::text(7297).arg(QString::number(generationCount(token))).toHtmlEscaped());html+=QStringLiteral("<div class='note'>%1</div>").arg(I18n::text(7298).toHtmlEscaped());html+=QStringLiteral("<p class='muted'>%1</p>").arg(I18n::text(7299).toHtmlEscaped());m_detail->setHtml(html);}if(m_detail->verticalScrollBar()) m_detail->verticalScrollBar()->setValue(0);
    }

    void showDocumentationHome()
    {
        if(!m_detail) return;QString html=htmlStyle();html+=QStringLiteral("<h1>%1</h1>").arg(I18n::text(7300).toHtmlEscaped());html+=QStringLiteral("<p>%1</p>").arg(I18n::text(7301).toHtmlEscaped());html+=QStringLiteral("<p class='muted'>%1</p>").arg(I18n::text(7302).toHtmlEscaped());m_detail->setHtml(html);if(m_detail->verticalScrollBar()) m_detail->verticalScrollBar()->setValue(0);
    }

private:
    QPointer<QWidget> m_browser;QLineEdit *m_search;QComboBox *m_generation;QTableWidget *m_results;QTextBrowser *m_detail;QLabel *m_resultCount;QStringListModel *m_completionModel;QCompleter *m_completer;QTimer *m_debounce;QFrame *m_quickAccess;QLabel *m_quickLabel;QPushButton *m_allButton;QPushButton *m_generationButtons[4]={nullptr,nullptr,nullptr,nullptr};QLabel *m_documentLabel;QLabel *m_currentGenerationLabel;QPushButton *m_sheetButton;QPushButton *m_pdfButton;QVector<SearchRow> m_rows;QStringList m_candidates;QSet<QString> m_candidateKeys;QString m_documentGeneration;QSqlDatabase m_database;QString m_connectionName;
};

class DatabaseUsabilityInstaller : public QObject
{
public:
    explicit DatabaseUsabilityInstaller(QObject *parent=nullptr):QObject(parent){}
    bool eventFilter(QObject *watched,QEvent *event) override{QWidget *widget=qobject_cast<QWidget*>(watched);if(widget&&event&&(event->type()==QEvent::Show||event->type()==QEvent::Polish)&&widget->objectName()==QStringLiteral("memsDatabaseBrowser")) schedulePatch(widget);return QObject::eventFilter(watched,event);}
    void scan(){const QList<QWidget*> widgets=QApplication::allWidgets();for(QWidget *widget:widgets) if(widget&&widget->objectName()==QStringLiteral("memsDatabaseBrowser")) schedulePatch(widget);}
private:
    void schedulePatch(QWidget *widget){if(!widget||widget->property("databaseUsabilityPatchedV2").toBool()) return;widget->setProperty("databaseUsabilityPatchedV2",true);QPointer<QWidget> guarded(widget);QTimer::singleShot(0,this,[guarded](){if(guarded) new DatabaseBrowserController(guarded);});}
};

void installDatabaseUsabilityPatch(){QCoreApplication *core=QCoreApplication::instance();if(!core) return;DatabaseUsabilityInstaller *installer=new DatabaseUsabilityInstaller(core);core->installEventFilter(installer);QTimer::singleShot(0,installer,[installer](){installer->scan();});}

}

Q_COREAPP_STARTUP_FUNCTION(installDatabaseUsabilityPatch)
