#include "MemsReferenceDatabase.h"
#include "MemsGlobalSearchIndex.h"
#include "../i18n.h"

#include <QApplication>
#include <QBoxLayout>
#include <QComboBox>
#include <QCoreApplication>
#include <QDialog>
#include <QEvent>
#include <QFile>
#include <QFileInfo>
#include <QGridLayout>
#include <QLineEdit>
#include <QPointer>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollBar>
#include <QSet>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTableWidget>
#include <QTextBlock>
#include <QTextBrowser>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextFragment>
#include <QTextImageFormat>
#include <QTimer>
#include <QUrl>
#include <QUuid>
#include <QVBoxLayout>
#include <QXmlStreamReader>

namespace {

const int RoleSourceTable=Qt::UserRole+11;
const int RoleGeneration=Qt::UserRole+14;

QString normalized(QString input)
{
    input=input.normalized(QString::NormalizationForm_D).toCaseFolded();
    QString out;
    bool space=true;
    for(const QChar ch:input){
        const QChar::Category category=ch.category();
        if(category==QChar::Mark_NonSpacing || category==QChar::Mark_SpacingCombining || category==QChar::Mark_Enclosing) continue;
        if(ch.isLetterOrNumber()){
            out+=ch;
            space=false;
        }else if(!space){
            out+=QLatin1Char(' ');
            space=true;
        }
    }
    return out.simplified();
}

QString languageText(const QString &fr,const QString &en,const QString &es,
                     const QString &it,const QString &pt,const QString &de)
{
    const QString lang=I18n::language().left(2).toLower();
    if(lang==QStringLiteral("en")) return en;
    if(lang==QStringLiteral("es")) return es;
    if(lang==QStringLiteral("it")) return it;
    if(lang==QStringLiteral("pt")) return pt;
    if(lang==QStringLiteral("de")) return de;
    return fr;
}

QString enlargeImageText()
{
    return languageText(QString::fromUtf8("Agrandir l’image"),QStringLiteral("Enlarge image"),
                        QStringLiteral("Ampliar imagen"),QStringLiteral("Ingrandisci immagine"),
                        QStringLiteral("Ampliar imagem"),QString::fromUtf8("Bild vergrößern"));
}

QString enlargedImageTitle()
{
    return languageText(QString::fromUtf8("Image agrandie"),QStringLiteral("Enlarged image"),
                        QStringLiteral("Imagen ampliada"),QStringLiteral("Immagine ingrandita"),
                        QStringLiteral("Imagem ampliada"),QString::fromUtf8("Vergrößertes Bild"));
}

QString sourceTitle(const QString &generation)
{
    QString text=languageText(QStringLiteral("Sources documentaires"),QStringLiteral("Documentary sources"),
                              QStringLiteral("Fuentes documentales"),QStringLiteral("Fonti documentali"),
                              QStringLiteral("Fontes documentais"),QStringLiteral("Dokumentationsquellen"));
    if(!generation.trimmed().isEmpty()) text+=QStringLiteral(" — MEMS ")+generation.trimmed();
    return text;
}

QString cssColor(const QString &word)
{
    const QString n=normalized(word);
    if(n==QStringLiteral("noir") || n==QStringLiteral("black")) return QStringLiteral("#101010");
    if(n==QStringLiteral("blanc") || n==QStringLiteral("white")) return QStringLiteral("#f2f2f2");
    if(n==QStringLiteral("rouge") || n==QStringLiteral("red")) return QStringLiteral("#d32f2f");
    if(n==QStringLiteral("vert") || n==QStringLiteral("green")) return QStringLiteral("#2e9b4f");
    if(n==QStringLiteral("bleu") || n==QStringLiteral("blue")) return QStringLiteral("#2878d0");
    if(n==QStringLiteral("jaune") || n==QStringLiteral("yellow")) return QStringLiteral("#f4d03f");
    if(n==QStringLiteral("orange")) return QStringLiteral("#f28c28");
    if(n==QStringLiteral("marron") || n==QStringLiteral("brun") || n==QStringLiteral("brown")) return QStringLiteral("#7a4a24");
    if(n==QStringLiteral("violet") || n==QStringLiteral("purple")) return QStringLiteral("#7e57c2");
    if(n==QStringLiteral("gris") || n==QStringLiteral("gray") || n==QStringLiteral("grey")) return QStringLiteral("#8c939a");
    if(n==QStringLiteral("rose") || n==QStringLiteral("pink")) return QStringLiteral("#e78ab3");
    return QString();
}

QString colorSwatches(const QString &text)
{
    QString html;
    const QStringList parts=text.split(QRegularExpression(QStringLiteral("[\\s/,+;-]+")),Qt::SkipEmptyParts);
    QSet<QString> used;
    int count=0;
    for(const QString &part:parts){
        const QString color=cssColor(part);
        if(color.isEmpty() || used.contains(color)) continue;
        used.insert(color);
        html+=QStringLiteral("<span style='background:%1;border:1px solid #b8c0c6;'>&nbsp;&nbsp;&nbsp;</span>&nbsp;").arg(color);
        if(++count>=3) break;
    }
    return html;
}

QString readCell(QXmlStreamReader &xml,const QString &endTag)
{
    Q_UNUSED(endTag)
    const QString text=xml.readElementText(QXmlStreamReader::IncludeChildElements).simplified();
    return colorSwatches(text)+text.toHtmlEscaped();
}

QString richXmlHtml(const QString &path)
{
    QFile file(path);
    if(!file.open(QIODevice::ReadOnly|QIODevice::Text)) return QString();
    QXmlStreamReader xml(&file);
    QString html=QStringLiteral(
        "<style>body{background:#0a1015;color:#dce3e8;font-family:'Segoe UI',Arial,sans-serif;font-size:9pt;}"
        "h1{color:#ff9828;font-size:16pt;margin:0 0 6px 0;}h2{color:#ff9828;font-size:10.5pt;border-bottom:1px solid #34414b;padding-bottom:4px;margin-top:14px;}"
        "p{line-height:1.4;margin:5px 0}.note{background:#15100b;border:1px solid #60401f;color:#ffd0a0;padding:7px;}"
        "table{border-collapse:collapse;width:100%;}th{color:#ff9828;text-align:left;border-bottom:1px solid #394650;padding:5px;}"
        "td{border-bottom:1px solid #26323b;padding:5px;vertical-align:top}</style>");
    bool firstRow=true;
    while(!xml.atEnd()){
        xml.readNext();
        if(xml.isEndElement()){
            if(xml.name()==QStringLiteral("ligne")){html+=QStringLiteral("</tr>");firstRow=false;}
            else if(xml.name()==QStringLiteral("table")) html+=QStringLiteral("</table>");
            continue;
        }
        if(!xml.isStartElement()) continue;
        const QString name=xml.name().toString();
        if(name==QStringLiteral("titre")) html+=QStringLiteral("<h1>%1</h1>").arg(xml.readElementText(QXmlStreamReader::IncludeChildElements).toHtmlEscaped());
        else if(name==QStringLiteral("sous-titre")) html+=QStringLiteral("<p>%1</p>").arg(xml.readElementText(QXmlStreamReader::IncludeChildElements).toHtmlEscaped());
        else if(name==QStringLiteral("section")) html+=QStringLiteral("<h2>%1</h2>").arg(xml.attributes().value(QStringLiteral("titre")).toString().toHtmlEscaped());
        else if(name==QStringLiteral("p")) html+=QStringLiteral("<p>%1</p>").arg(xml.readElementText(QXmlStreamReader::IncludeChildElements).toHtmlEscaped());
        else if(name==QStringLiteral("note")) html+=QStringLiteral("<div class='note'>%1</div>").arg(xml.readElementText(QXmlStreamReader::IncludeChildElements).toHtmlEscaped());
        else if(name==QStringLiteral("table")){firstRow=true;html+=QStringLiteral("<table>");}
        else if(name==QStringLiteral("ligne")) html+=QStringLiteral("<tr>");
        else if(name==QStringLiteral("cellule") || name==QStringLiteral("broche") || name==QStringLiteral("fonction") || name==QStringLiteral("couleur")){
            const QString tag=firstRow?QStringLiteral("th"):QStringLiteral("td");
            html+=QStringLiteral("<%1>%2</%1>").arg(tag,readCell(xml,name));
        }
    }
    return html;
}

QString generationFromBrowser(QWidget *browser)
{
    if(!browser) return QString();
    if(QComboBox *filter=browser->findChild<QComboBox*>()){
        const QString data=filter->currentData().toString();
        if(data.startsWith(QStringLiteral("gen:"))) return data.mid(4);
        if(data==QStringLiteral("1.2") || data==QStringLiteral("1.3") || data==QStringLiteral("1.6") || data==QStringLiteral("1.9")) return data;
    }
    if(QTableWidget *table=browser->findChild<QTableWidget*>()){
        if(table->currentRow()>=0 && table->columnCount()>1){
            if(QTableWidgetItem *item=table->item(table->currentRow(),1)){
                const QString text=item->text();
                for(const QString &g:{QStringLiteral("1.2"),QStringLiteral("1.3"),QStringLiteral("1.6"),QStringLiteral("1.9")}) if(text.contains(g)) return g;
            }
        }
    }
    if(QLineEdit *search=browser->findChild<QLineEdit*>()){
        const QString text=search->text();
        for(const QString &g:{QStringLiteral("1.2"),QStringLiteral("1.3"),QStringLiteral("1.6"),QStringLiteral("1.9")}) if(text.contains(g)) return g;
    }
    return QString();
}

void showRichSheet(QWidget *browser)
{
    const QString generation=generationFromBrowser(browser);
    if(generation.isEmpty()) return;
    MemsReferenceDatabase reference;
    if(!reference.open()) return;
    const QString path=reference.generationXmlPath(QStringLiteral("MEMS %1").arg(generation));
    if(path.isEmpty() || !QFileInfo::exists(path)) return;
    QDialog dialog(browser);
    dialog.setWindowTitle(QStringLiteral("MEMS %1 — fiche technique").arg(generation));
    dialog.resize(940,680);
    QVBoxLayout *layout=new QVBoxLayout(&dialog);
    layout->setContentsMargins(8,8,8,8);
    QTextBrowser *view=new QTextBrowser(&dialog);
    view->setHtml(richXmlHtml(path));
    view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    view->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    view->setFocusPolicy(Qt::StrongFocus);
    layout->addWidget(view);
    dialog.exec();
}

QString localImagePath(const QString &name)
{
    const QUrl url(name);
    if(url.isLocalFile()) return QDir::cleanPath(url.toLocalFile());
    const QFileInfo info(name);
    if(info.isAbsolute()) return QDir::cleanPath(name);
    return QString();
}

QSizeF svgNativeSize(const QString &path)
{
    if(QFileInfo(path).suffix().compare(QStringLiteral("svg"),Qt::CaseInsensitive)!=0) return QSizeF();
    QFile file(path);
    if(!file.open(QIODevice::ReadOnly|QIODevice::Text)) return QSizeF();
    const QString text=QString::fromUtf8(file.read(128*1024));

    static const QRegularExpression viewBox(
        QStringLiteral("viewBox\\s*=\\s*['\"]\\s*[-+0-9.eE]+\\s+[-+0-9.eE]+\\s+([-+0-9.eE]+)\\s+([-+0-9.eE]+)\\s*['\"]"),
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch match=viewBox.match(text);
    if(match.hasMatch()){
        const double width=match.captured(1).toDouble();
        const double height=match.captured(2).toDouble();
        if(width>0.0 && height>0.0) return QSizeF(width,height);
    }

    static const QRegularExpression widthRx(QStringLiteral("\\bwidth\\s*=\\s*['\"]\\s*([0-9.]+)"),QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression heightRx(QStringLiteral("\\bheight\\s*=\\s*['\"]\\s*([0-9.]+)"),QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch wm=widthRx.match(text);
    const QRegularExpressionMatch hm=heightRx.match(text);
    if(wm.hasMatch() && hm.hasMatch()) return QSizeF(wm.captured(1).toDouble(),hm.captured(1).toDouble());
    return QSizeF();
}

QString firstImagePath(QTextBrowser *detail)
{
    if(!detail || !detail->document()) return QString();
    QTextDocument *document=detail->document();
    for(QTextBlock block=document->begin();block.isValid();block=block.next()){
        for(QTextBlock::iterator it=block.begin();!it.atEnd();++it){
            const QTextFragment fragment=it.fragment();
            if(!fragment.isValid() || !fragment.charFormat().isImageFormat()) continue;
            const QTextImageFormat format=fragment.charFormat().toImageFormat();
            const QString path=localImagePath(format.name());
            if(!path.isEmpty() && QFileInfo::exists(path)) return path;
        }
    }
    return QString();
}

void fitDetailImages(QTextBrowser *detail)
{
    if(!detail || !detail->document() || !detail->viewport()) return;
    const qreal targetWidth=qMax(120,detail->viewport()->width()-36);
    QTextDocument *document=detail->document();

    for(QTextBlock block=document->begin();block.isValid();block=block.next()){
        for(QTextBlock::iterator it=block.begin();!it.atEnd();++it){
            const QTextFragment fragment=it.fragment();
            if(!fragment.isValid() || !fragment.charFormat().isImageFormat()) continue;
            QTextImageFormat format=fragment.charFormat().toImageFormat();
            const QString path=localImagePath(format.name());
            const QSizeF native=svgNativeSize(path);

            qreal width=targetWidth;
            qreal height=0.0;
            if(native.isValid() && native.width()>0.0 && native.height()>0.0){
                width=qMin(targetWidth,native.width());
                height=width*native.height()/native.width();
            }
            format.setWidth(width);
            if(height>0.0) format.setHeight(height);

            QTextCursor cursor(document);
            cursor.setPosition(fragment.position());
            cursor.setPosition(fragment.position()+fragment.length(),QTextCursor::KeepAnchor);
            cursor.setCharFormat(format);
        }
    }

    detail->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    detail->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    detail->setFocusPolicy(Qt::StrongFocus);
}

void openLargeImage(QWidget *browser,const QString &path)
{
    if(path.isEmpty() || !QFileInfo::exists(path)) return;
    QDialog dialog(browser);
    dialog.setWindowTitle(enlargedImageTitle());
    dialog.resize(1100,760);
    QVBoxLayout *layout=new QVBoxLayout(&dialog);
    layout->setContentsMargins(6,6,6,6);

    QTextBrowser *view=new QTextBrowser(&dialog);
    view->setOpenExternalLinks(false);
    view->setLineWrapMode(QTextEdit::NoWrap);
    view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    view->setFocusPolicy(Qt::StrongFocus);

    const QSizeF native=svgNativeSize(path);
    qreal displayWidth=native.isValid()?native.width():1400.0;
    qreal displayHeight=native.isValid()?native.height():900.0;
    if(displayWidth<1200.0 && displayWidth>0.0){
        const qreal ratio=1200.0/displayWidth;
        displayWidth=1200.0;
        displayHeight*=ratio;
    }
    if(displayWidth<=0.0) displayWidth=1400.0;
    if(displayHeight<=0.0) displayHeight=900.0;

    const QString url=QUrl::fromLocalFile(path).toString().toHtmlEscaped();
    view->setHtml(QStringLiteral(
        "<html><body style='margin:8px;background:#0a1015;'>"
        "<img src='%1' width='%2' height='%3'>"
        "</body></html>").arg(url).arg(int(displayWidth)).arg(int(displayHeight)));
    view->document()->setTextWidth(displayWidth+24.0);
    layout->addWidget(view);
    dialog.exec();
}

void cleanOversizedSourceTitle(QWidget *browser,QTextBrowser *detail)
{
    if(!browser || !detail || !detail->document()) return;
    QTableWidget *table=browser->findChild<QTableWidget*>();
    if(!table || table->currentRow()<0) return;
    QTableWidgetItem *item=table->item(table->currentRow(),0);
    if(!item) return;
    const QString sourceTable=item->data(RoleSourceTable).toString().toLower();
    if(!sourceTable.contains(QStringLiteral("source")) && !sourceTable.contains(QStringLiteral("document"))) return;

    QTextBlock block=detail->document()->begin();
    while(block.isValid() && block.text().trimmed().isEmpty()) block=block.next();
    if(!block.isValid()) return;
    const QString current=block.text().trimmed();
    if(current.size()<100) return;

    const QString generation=item->data(RoleGeneration).toString();
    QTextCursor cursor(block);
    cursor.select(QTextCursor::BlockUnderCursor);
    QTextCharFormat format;
    format.setForeground(QColor(QStringLiteral("#ff9828")));
    format.setFontPointSize(16.0);
    format.setFontWeight(QFont::Bold);
    cursor.insertText(sourceTitle(generation),format);
}

void ensureWiringSearchAliases()
{
    const QString indexPath=MemsGlobalSearchIndex::indexPath();
    if(indexPath.isEmpty() || !QFileInfo::exists(indexPath)) return;

    const QString connection=QStringLiteral("MEMS_WIRING_ALIASES_%1").arg(QUuid::createUuid().toString());
    QSqlDatabase database=QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"),connection);
    database.setDatabaseName(indexPath);
    if(!database.open()){
        database=QSqlDatabase();
        QSqlDatabase::removeDatabase(connection);
        return;
    }

    QSqlQuery marker(database);
    marker.prepare(QStringLiteral("SELECT value FROM search_meta WHERE key='wiring_alias_version'"));
    if(marker.exec() && marker.next() && marker.value(0).toString()==QStringLiteral("2")){
        database.close();
        database=QSqlDatabase();
        QSqlDatabase::removeDatabase(connection);
        return;
    }

    const QString aliases=QString::fromUtf8(
        "image images schéma schemas schema diagramme diagram diagrams "
        "connecteur connector connectors conector connettore stecker "
        "brochage pinout câblage cablage wiring faisceau harness "
        "prise diagnostic connecteur diagnostic diagnostic connector "
        "ROSCO OBD J1962 ECU MEMS broche broches pin pins pines");
    const QString normalizedAliases=normalized(aliases);

    if(database.transaction()){
        QSqlQuery updateDocs(database);
        updateDocs.prepare(QStringLiteral(
            "UPDATE search_documents SET searchable=searchable||' '||:aliases, normalized=normalized||' '||:normalized "
            "WHERE category='wiring'"));
        updateDocs.bindValue(QStringLiteral(":aliases"),aliases);
        updateDocs.bindValue(QStringLiteral(":normalized"),normalizedAliases);
        bool ok=updateDocs.exec();

        bool ftsEnabled=false;
        QSqlQuery meta(database);
        if(meta.exec(QStringLiteral("SELECT value FROM search_meta WHERE key='fts5_enabled'")) && meta.next())
            ftsEnabled=meta.value(0).toString()==QStringLiteral("1");
        if(ok && ftsEnabled){
            QSqlQuery updateFts(database);
            updateFts.prepare(QStringLiteral(
                "UPDATE search_fts SET searchable=searchable||' '||:aliases, normalized=normalized||' '||:normalized "
                "WHERE rowid IN (SELECT id FROM search_documents WHERE category='wiring')"));
            updateFts.bindValue(QStringLiteral(":aliases"),aliases);
            updateFts.bindValue(QStringLiteral(":normalized"),normalizedAliases);
            ok=updateFts.exec();
        }

        if(ok){
            QList<qlonglong> ids;
            QSqlQuery rows(database);
            if(rows.exec(QStringLiteral("SELECT id FROM search_documents WHERE category='wiring'")))
                while(rows.next()) ids.append(rows.value(0).toLongLong());
            else ok=false;

            if(ok){
                QSqlQuery insert(database);
                insert.prepare(QStringLiteral("INSERT OR IGNORE INTO search_terms(term,document_id) VALUES(:term,:id)"));
                const QStringList terms=normalizedAliases.split(QLatin1Char(' '),Qt::SkipEmptyParts);
                for(const qlonglong id:ids){
                    for(const QString &term:terms){
                        insert.bindValue(QStringLiteral(":term"),term);
                        insert.bindValue(QStringLiteral(":id"),id);
                        if(!insert.exec()){ok=false;break;}
                    }
                    if(!ok) break;
                }
            }
        }

        if(ok){
            QSqlQuery writeMarker(database);
            ok=writeMarker.exec(QStringLiteral("INSERT OR REPLACE INTO search_meta(key,value) VALUES('wiring_alias_version','2')"));
        }
        if(ok) database.commit(); else database.rollback();
    }

    database.close();
    database=QSqlDatabase();
    QSqlDatabase::removeDatabase(connection);
}

class MediaController : public QObject
{
public:
    explicit MediaController(QWidget *browser):QObject(browser),m_browser(browser)
    {
        if(!m_browser) return;
        m_detail=m_browser->findChild<QTextBrowser*>();
        m_results=m_browser->findChild<QTableWidget*>();
        if(m_detail){
            m_detail->installEventFilter(this);
            m_detail->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
            m_detail->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        }
        m_browser->installEventFilter(this);
        createImageButton();
        if(m_results) QObject::connect(m_results,&QTableWidget::itemSelectionChanged,this,[this](){scheduleRefresh();});
        QTimer::singleShot(600,this,[this](){refresh();});
    }

protected:
    bool eventFilter(QObject *watched,QEvent *event) override
    {
        if(!event) return QObject::eventFilter(watched,event);
        if((watched==m_detail || watched==m_browser) && event->type()==QEvent::Resize)
            QTimer::singleShot(30,this,[this](){refresh();});
        if(watched==m_browser && event->type()==QEvent::LanguageChange){
            if(m_imageButton) m_imageButton->setText(enlargeImageText());
            scheduleRefresh();
        }
        return QObject::eventFilter(watched,event);
    }

private:
    void createImageButton()
    {
        if(!m_browser || m_browser->findChild<QPushButton*>(QStringLiteral("memsEnlargeImageButton"))) return;
        QPushButton *sheet=nullptr;
        for(QPushButton *button:m_browser->findChildren<QPushButton*>()){
            if(!button) continue;
            const QString text=button->text();
            if(text==I18n::text(7325) || text==I18n::text(7253) || text==I18n::text(7287)){
                sheet=button;
                break;
            }
        }
        if(!sheet || !sheet->parentWidget() || !sheet->parentWidget()->layout()) return;

        m_imageButton=new QPushButton(enlargeImageText(),sheet->parentWidget());
        m_imageButton->setObjectName(QStringLiteral("memsEnlargeImageButton"));
        m_imageButton->setEnabled(false);
        QLayout *layout=sheet->parentWidget()->layout();
        if(QBoxLayout *box=qobject_cast<QBoxLayout*>(layout)){
            const int index=box->indexOf(sheet);
            box->insertWidget(index>=0?index+1:box->count(),m_imageButton);
        }else{
            layout->addWidget(m_imageButton);
        }
        QObject::connect(m_imageButton,&QPushButton::clicked,this,[this](){
            if(m_detail) openLargeImage(m_browser,firstImagePath(m_detail));
        });
    }

    void scheduleRefresh()
    {
        QTimer::singleShot(140,this,[this](){refresh();});
    }

    void refresh()
    {
        if(!m_detail) return;
        cleanOversizedSourceTitle(m_browser,m_detail);
        fitDetailImages(m_detail);
        if(m_imageButton) m_imageButton->setEnabled(!firstImagePath(m_detail).isEmpty());
    }

    QPointer<QWidget> m_browser;
    QPointer<QTextBrowser> m_detail;
    QPointer<QTableWidget> m_results;
    QPointer<QPushButton> m_imageButton;
};

void patchBrowser(QWidget *browser)
{
    if(!browser || browser->property("richSheetRendererV5").toBool()) return;
    browser->setProperty("richSheetRendererV5",true);
    ensureWiringSearchAliases();
    const QList<QPushButton*> buttons=browser->findChildren<QPushButton*>();
    for(QPushButton *button:buttons){
        if(!button) continue;
        const QString text=button->text();
        if(text==I18n::text(7325) || text==I18n::text(7253) || text==I18n::text(7287)){
            QObject::disconnect(button,nullptr,nullptr,nullptr);
            QObject::connect(button,&QPushButton::clicked,browser,[browser](){showRichSheet(browser);});
        }
    }
    new MediaController(browser);
}

class BrowserPatchInstaller : public QObject
{
public:
    explicit BrowserPatchInstaller(QObject *parent=nullptr):QObject(parent){}
    bool eventFilter(QObject *watched,QEvent *event) override
    {
        QWidget *widget=qobject_cast<QWidget*>(watched);
        if(widget && event && (event->type()==QEvent::Show || event->type()==QEvent::Polish) && widget->objectName()==QStringLiteral("memsDatabaseBrowser")){
            QPointer<QWidget> guarded(widget);
            QTimer::singleShot(100,this,[guarded](){if(guarded) patchBrowser(guarded);});
        }
        return QObject::eventFilter(watched,event);
    }
};

void installRichSheetRenderer()
{
    QCoreApplication *core=QCoreApplication::instance();
    if(!core) return;
    BrowserPatchInstaller *installer=new BrowserPatchInstaller(core);
    core->installEventFilter(installer);
    QTimer::singleShot(350,installer,[installer](){
        for(QWidget *widget:QApplication::allWidgets()) if(widget && widget->objectName()==QStringLiteral("memsDatabaseBrowser")) patchBrowser(widget);
    });
}

}

Q_COREAPP_STARTUP_FUNCTION(installRichSheetRenderer)
