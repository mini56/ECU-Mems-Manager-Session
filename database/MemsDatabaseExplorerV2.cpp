#include "MemsGlobalSearchIndex.h"
#include "MemsDocumentaryRepository.h"
#include "MemsDatabaseExplorerV2.h"
#include "MemsDatabaseUniversalAccess.h"
#include "MemsReferenceSheetRenderer.h"
#include "../i18n.h"
#include "../expert/IaMemsService.h"

#include <QApplication>
#include <QComboBox>
#include <QCompleter>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>
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

QString htmlStyle()
{
    return QStringLiteral(
        "<style>body{background:#0a1015;color:#dce3e8;font-family:'Segoe UI',Arial,sans-serif;font-size:9pt;}"
        "h1{color:#ff9828;font-size:16pt;margin:0 0 6px 0;}"
        "h2{color:#ff9828;font-size:10.5pt;border-bottom:1px solid #34414b;padding-bottom:4px;margin-top:14px;}"
        "p{line-height:1.4;margin:5px 0}.muted{color:#98a5af}.note{background:#15100b;border:1px solid #60401f;color:#ffd0a0;padding:7px;}"
        ".diagram{background:#0d151b;border:1px solid #34414b;padding:8px;margin:8px 0 12px 0;text-align:center;}"
        ".diagram img{max-width:100%;height:auto;}"
        "table{border-collapse:collapse;width:100%;}th{color:#ff9828;text-align:left;border-bottom:1px solid #394650;padding:5px;}"
        "td{border-bottom:1px solid #26323b;padding:5px;vertical-align:top}.content{white-space:pre-wrap;line-height:1.45}</style>");
}

QString diagramHtml(const QString &generation)
{
    const QString base=QStringLiteral("mems_%1").arg(QString(generationFromText(generation)).replace(QLatin1Char('.'), QLatin1Char('_')));
    const QString root=QCoreApplication::applicationDirPath()+QStringLiteral("/database/reference/images/");
    QStringList files;
    QStringList captions;
    if(base==QStringLiteral("mems_1_2")){
        files << QStringLiteral("mems_1_2_ecu_connector.svg") << QStringLiteral("rover_rosco_3pin_black.svg");
        captions << QStringLiteral("Connecteur ECU MEMS 1.2 — 36 voies") << QStringLiteral("Prise diagnostic Rover / ROSCO — 3 broches");
    }else if(base==QStringLiteral("mems_1_3")){
        files << QStringLiteral("mems_1_3_ecu_connectors.svg") << QStringLiteral("rover_rosco_3pin_black.svg");
        captions << QStringLiteral("Connecteurs ECU MEMS 1.3 — 36 + 18 voies") << QStringLiteral("Prise diagnostic Rover / ROSCO — 3 broches");
    }else if(base==QStringLiteral("mems_1_6")){
        files << QStringLiteral("mems_1_6_ecu_connector.svg") << QStringLiteral("rover_rosco_3pin_black.svg");
        captions << QStringLiteral("Connecteur ECU MEMS 1.6 — 36 voies ; variante 36 + 18 selon véhicule") << QStringLiteral("Prise diagnostic Rover / ROSCO — 3 broches");
    }else if(base==QStringLiteral("mems_1_9")){
        files << QStringLiteral("mems_1_9_ecu_connector.svg") << QStringLiteral("mems_1_9_obd_16pin.svg");
        captions << QStringLiteral("Connecteur ECU MEMS 1.9 — 36 voies") << QStringLiteral("Prise diagnostic MEMS 1.9 — 16 broches type J1962 / OBD");
    }
    QString html;
    for(int i=0;i<files.size();++i){
        const QString filePath=root+files.at(i);
        if(!QFileInfo::exists(filePath)) continue;
        const QString url=QUrl::fromLocalFile(filePath).toString().toHtmlEscaped();
        const QString caption=captions.value(i).toHtmlEscaped();
        html+=QStringLiteral("<div class='diagram'><img src='%1'><p class='muted'>%2</p></div>").arg(url,caption);
    }
    return html;
}

QString renderXml(const QString &path)
{
    // Common XML renderer is used by both the browser and the indexed explorer.
    QString html = MemsReferenceSheetRenderer::renderFile(path,I18n::text(7232));
    const QString diagrams = diagramHtml(QFileInfo(path).completeBaseName());
    if(!diagrams.isEmpty()){
        const QString styleEnd=QStringLiteral("</style>");
        const int pos=html.indexOf(styleEnd);
        if(pos>=0) html.insert(pos+styleEnd.size(),diagrams);
        else html.prepend(diagrams);
    }
    return html;
}

QString xmlPath(const QString &generation)
{
    const QString root=QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)+QStringLiteral("/reference/fiches/");
    if(generation==QStringLiteral("1.2")) return root+QStringLiteral("mems_1_2.xml");
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
          m_debounce(new QTimer(this)),m_completionModel(new QStringListModel(this)),m_completer(nullptr),m_ftsEnabled(false),m_ready(false)
    {
        if(!m_browser) return;
        m_search=m_browser->findChild<QLineEdit*>();
        m_filter=m_browser->findChild<QComboBox*>();
        m_results=m_browser->findChild<QTableWidget*>();
        m_detail=m_browser->findChild<QTextBrowser*>();
        locateResultCount();
        if(!m_search||!m_filter||!m_results||!m_detail) return;

        const bool existingIndexReady=MemsGlobalSearchIndex::ensureBuilt() && openIndex();
        // Do not suppress the documentary page when the legacy index fails:
        // the shared 482-page repository can still be queried independently.
        if(!existingIndexReady && !MemsDocumentaryRepository::isPackInstalled()) return;

        compactHeader();
        configureFilter();
        configureResults();
        configureDetail();
        connect(IaMemsService::instance(), &IaMemsService::documentTranslationReady,
                this, [this](const QString &requestId, const QString &translated,
                             bool complete) {
            if (requestId != m_translationRequestId || !m_detail)
                return;
            m_translationRequestId.clear();
            const QVariantMap row = selectedResult();
            if (row.isEmpty() ||
                !row.value(QStringLiteral("source_table")).toString().startsWith(
                    QStringLiteral("rover_")))
                return;
            const QString language = I18n::language();
            const QString title = row.value(QStringLiteral("title")).toString();
            QString translatedTitle = title;
            const QString firstLine = translated.section(QLatin1Char('\n'),0,0);
            const int colon = firstLine.indexOf(QLatin1Char(':'));
            if (colon >= 0 && colon < 24 &&
                firstLine.mid(colon+1).trimmed().size() > 3)
                translatedTitle = firstLine.mid(colon+1).trimmed();
            const QString notice = IaMemsService::documentTranslationNotice(
                language, false);
            QString html = MemsDatabaseContent::renderResultHtml(
                translatedTitle, translated,
                row.value(QStringLiteral("category")).toString(),
                row.value(QStringLiteral("generation")).toString(),
                row.value(QStringLiteral("source_table")).toString(), QString());
            html.replace(QStringLiteral("<h1>"),
                QStringLiteral("<p style='color:#ffad5c;font-weight:700;'>%1</p><h1>")
                    .arg(notice.toHtmlEscaped()));
            if (!complete)
                html.replace(QStringLiteral("<h1>"),
                    QStringLiteral("<p style='color:#ffb44e;'>%1</p><h1>")
                       .arg(IaMemsService::documentTranslationIncompleteNotice(language).toHtmlEscaped()));
            m_detail->setHtml(html);
            if (m_detail->verticalScrollBar())
                m_detail->verticalScrollBar()->setValue(0);
        });
        configureDocumentation();
        configureSearch();
        retranslate();
        m_browser->installEventFilter(this);
        m_ready=true;
        QTimer::singleShot(0,this,[this](){runSearch();});
    }

    bool ready() const { return m_ready; }

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

    // Show a RAVE image inside the database page, not via an external
    // browser. Scroll/drag and fit/zoom remain available for technical
    // drawings containing small component labels.
    void openReferenceImage(const QString &path)
    {
        const QPixmap original(path);
        if(original.isNull()){
            QMessageBox::warning(m_browser.data(),QStringLiteral("Image RAVE"),
                                 QStringLiteral("Impossible de lire le fichier image :\n%1").arg(path));
            return;
        }
        QDialog dialog(m_browser.data());
        dialog.setWindowTitle(QStringLiteral("Image RAVE — %1").arg(QFileInfo(path).fileName()));
        dialog.resize(1000,740);
        QVBoxLayout *layout=new QVBoxLayout(&dialog);
        layout->setContentsMargins(8,8,8,8);
        layout->setSpacing(7);

        QHBoxLayout *actions=new QHBoxLayout;
        QPushButton *smaller=new QPushButton(QStringLiteral("−"),&dialog);
        QPushButton *bigger=new QPushButton(QStringLiteral("+"),&dialog);
        QPushButton *fit=new QPushButton(QStringLiteral("Ajuster"),&dialog);
        QPushButton *actual=new QPushButton(QStringLiteral("100 %"),&dialog);
        actions->addWidget(new QLabel(QFileInfo(path).fileName(),&dialog));
        actions->addStretch(1);
        actions->addWidget(smaller);
        actions->addWidget(bigger);
        actions->addWidget(fit);
        actions->addWidget(actual);
        layout->addLayout(actions);

        QGraphicsScene scene(&dialog);
        scene.addPixmap(original);
        scene.setSceneRect(QRectF(original.rect()));
        QGraphicsView *view=new QGraphicsView(&scene,&dialog);
        view->setDragMode(QGraphicsView::ScrollHandDrag);
        view->setRenderHint(QPainter::SmoothPixmapTransform,true);
        view->setBackgroundBrush(QColor(QStringLiteral("#0a1015")));
        layout->addWidget(view,1);
        connect(smaller,&QPushButton::clicked,view,[view](){view->scale(0.8,0.8);});
        connect(bigger,&QPushButton::clicked,view,[view](){view->scale(1.25,1.25);});
        connect(fit,&QPushButton::clicked,view,[view,&scene](){
            view->fitInView(scene.sceneRect(),Qt::KeepAspectRatio);
        });
        connect(actual,&QPushButton::clicked,view,[view](){view->resetTransform();});
        QTimer::singleShot(0,&dialog,[view,&scene](){
            view->fitInView(scene.sceneRect(),Qt::KeepAspectRatio);
        });
        dialog.exec();
    }

    void openResultLink(const QUrl &url)
    {
        if(url.isLocalFile()){
            const QString root=QCoreApplication::applicationDirPath()+
                               QStringLiteral("/database/reference");
            const QString canonicalRoot=QFileInfo(root).canonicalFilePath();
            const QString path=QFileInfo(url.toLocalFile()).canonicalFilePath();
            if(canonicalRoot.isEmpty() ||
               !path.startsWith(canonicalRoot+QLatin1Char('/'),Qt::CaseInsensitive))
                return; // Never follow paths outside the installed reference package.
            const QString relative=QDir(canonicalRoot).relativeFilePath(path);
            const QString image=MemsDatabaseContent::localImagePath(relative);
            if(!image.isEmpty() && image==path){
                openReferenceImage(image);
                return;
            }
            // Preserve existing file opening for trusted, packaged PDF/text.
            QDesktopServices::openUrl(QUrl::fromLocalFile(path));
        }else if(url.scheme()==QStringLiteral("http") ||
                 url.scheme()==QStringLiteral("https")){
            QDesktopServices::openUrl(url);
        }
    }

    void configureDetail()
    {
        m_detail->setOpenLinks(false);
        m_detail->setOpenExternalLinks(false);
        connect(m_detail,&QTextBrowser::anchorClicked,this,
                [this](const QUrl &url){openResultLink(url);});
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
            connect(m_sheetButton,&QPushButton::clicked,this,[this](){openXmlDialog();});
        }
        if(m_pdfButton){
            connect(m_pdfButton,&QPushButton::clicked,this,[this](){
                const QString source=selectedResult().value(QStringLiteral("source_table")).toString();
                const QString path=source.startsWith(QStringLiteral("rover_"))
                    ? MemsDocumentaryRepository::originalPdfPath()
                    : pdfPath(currentGeneration());
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
        const bool documentaryInstalled=MemsDocumentaryRepository::isPackInstalled();
        if(documentaryInstalled && generation.isEmpty() &&
           (category.isEmpty() || category==QStringLiteral("documentation"))){
            MemsDocumentaryRepository corpus;
            if(corpus.open()) rows=corpus.search(text,limit,true);
        }
        // A mounted corpus is the single documentary source. Never add old
        // Pack001/reference documentary rows beside it as competing evidence.
        if(documentaryInstalled && category==QStringLiteral("documentation"))
            return rows;
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
            if(documentaryInstalled &&
               row.value(QStringLiteral("category")).toString()==QStringLiteral("documentation"))
                continue;
            rows.append(row);
        }
        return rows;
    }

    QVariantList fetchRows(const QString &raw,const QString &category,const QString &generation) const
    {
        // The documentary view must be able to browse all currently detected
        // procedures (238 candidates), not silently truncate to the first 30.
        const int limit=category==QStringLiteral("documentation")?300:
                        (category.isEmpty()?140:220);
        return executeQuery(raw,category,generation,limit);
    }

    QVector<QVariantMap> cleanAndSort(const QVariantList &fetched,const QString &requestedCategory) const
    {
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
            const QString key=category+QLatin1Char('|')+source+QLatin1Char('|')+
                              row.value(QStringLiteral("source_key")).toString()+QLatin1Char('|')+
                              title.toCaseFolded()+QLatin1Char('|')+row.value(QStringLiteral("generation")).toString();
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
        const QString category=filterCategory();
        QString generation=filterGeneration();
        if(generation.isEmpty()) generation=generationFromText(raw);

        if(raw.isEmpty() && category.isEmpty() && generation.isEmpty()){
            m_results->setRowCount(0);
            if(m_count) m_count->setText(I18n::text(7324).arg(QStringLiteral("0")));
            m_detail->setHtml(htmlStyle()+QStringLiteral("<h1>%1</h1><p class='muted'>%2</p>").arg(I18n::text(7216).toHtmlEscaped(),I18n::text(7295).toHtmlEscaped()));
            updateDocumentControls();
            return;
        }

        const QVariantList fetched=fetchRows(raw,category,generation);
        QVector<QVariantMap> rows=cleanAndSort(fetched,category);
        populate(rows);
        if(m_count) m_count->setText(I18n::text(7324).arg(QString::number(rows.size())));
        updateSuggestions(raw,rows);

        if(rows.isEmpty()){
            QString missing=I18n::text(7328);
            if(MemsDocumentaryRepository::isPackInstalled() &&
               category==QStringLiteral("documentation")){
                MemsDocumentaryRepository corpus;
                if(!corpus.open()) missing=corpus.lastError();
            }
            m_detail->setHtml(htmlStyle()+QStringLiteral("<h1>%1</h1><p class='muted'>%2</p>").arg(I18n::text(7216).toHtmlEscaped(),missing.toHtmlEscaped()));
            updateDocumentControls();
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
            const QString localized =
                row.value(QStringLiteral("source_table")).toString().startsWith(
                    QStringLiteral("rover_"))
                ? MemsDocumentaryRepository::localizedTitle(
                    title, I18n::language())
                : MemsDatabaseContent::localizedTitle(
                    row.value(QStringLiteral("content")).toString(),title);
            QTableWidgetItem *titleItem=new QTableWidgetItem(localized);
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
        const QString source=selectedResult().value(QStringLiteral("source_table")).toString();
        const QString pdf=source.startsWith(QStringLiteral("rover_"))
            ? MemsDocumentaryRepository::originalPdfPath() : pdfPath(generation);
        if(m_sheetButton) m_sheetButton->setEnabled(!xml.isEmpty()&&QFileInfo::exists(xml));
        if(m_pdfButton){
            const bool available=!pdf.isEmpty()&&QFileInfo::exists(pdf);
            m_pdfButton->setEnabled(available);
            m_pdfButton->setVisible(true);
        }
    }

    void showSelectedResult()
    {
        m_translationRequestId.clear();
        const QVariantMap row=selectedResult();
        if(row.isEmpty()) return;
        const QString category=row.value(QStringLiteral("category")).toString();
        const QString source=row.value(QStringLiteral("source_table")).toString();
        const QString generation=row.value(QStringLiteral("generation")).toString();

        if(category==QStringLiteral("ecu") && source==QStringLiteral("ecu_target")){
            QMetaObject::invokeMethod(m_browser.data(),"showSelectedEcu",Qt::DirectConnection);
            updateDocumentControls();
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
                updateDocumentControls();
                return;
            }
        }

        const QString title=row.value(QStringLiteral("title")).toString();
        const QString content=row.value(QStringLiteral("content")).toString();
        const QString language=I18n::language().left(2).toLower();
        const QString localizedTitle = source.startsWith(QStringLiteral("rover_"))
            ? MemsDocumentaryRepository::localizedTitle(title, language)
            : title;
        if (source.startsWith(QStringLiteral("rover_")) &&
            language!=QStringLiteral("en")) {
            m_translationRequestId=QUuid::createUuid().toString(QUuid::WithoutBraces);
            m_detail->setHtml(QStringLiteral(
                "<h1>%1</h1><p class='muted'>%2</p>")
                .arg(localizedTitle.toHtmlEscaped(),
                     IaMemsService::documentTranslationNotice(language,true).toHtmlEscaped()));
            IaMemsService::instance()->requestDocumentTranslation(
                m_translationRequestId,content,language);
            updateDocumentControls();
            return;
        }
        const QString diagrams=category==QStringLiteral("wiring")
            ? diagramHtml(generation) : QString();
        m_detail->setHtml(MemsDatabaseContent::renderResultHtml(
            title,content,category,generation,source,diagrams));
        if(m_detail->verticalScrollBar()) m_detail->verticalScrollBar()->setValue(0);
        updateDocumentControls();
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
    QString m_translationRequestId;
    bool m_ftsEnabled;
    bool m_ready;
};

} // namespace

bool initializeMemsDatabaseExplorer(QWidget *browser)
{
    if(!browser) return false;
    DatabaseExplorerV2 *explorer=new DatabaseExplorerV2(browser);
    if(explorer->ready()) return true;
    // An unavailable index must leave the original ECU-only reader usable.
    delete explorer;
    return false;
}

