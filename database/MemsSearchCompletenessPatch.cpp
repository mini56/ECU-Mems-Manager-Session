#include "MemsGlobalSearchIndex.h"
#include "MemsReferenceDatabase.h"
#include "../i18n.h"

#include <QApplication>
#include <QComboBox>
#include <QCoreApplication>
#include <QDialog>
#include <QEvent>
#include <QFile>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPointer>
#include <QPushButton>
#include <QRegularExpression>
#include <QSet>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QTableWidget>
#include <QTextBrowser>
#include <QTimer>
#include <QUuid>
#include <QVBoxLayout>
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
    return out.simplified();
}

QString categoryForRow(const QString &section,const QStringList &tags)
{
    const QString n=normalized(section+QLatin1Char(' ')+tags.join(QLatin1Char(' ')));
    if(n.contains(QStringLiteral("broch")) || n.contains(QStringLiteral("connecteur")) ||
       n.contains(QStringLiteral("cablage")) || n.contains(QStringLiteral("pinout")) ||
       tags.contains(QStringLiteral("broche")) || tags.contains(QStringLiteral("couleur")) ||
       tags.contains(QStringLiteral("fonction")))
        return QStringLiteral("wiring");
    if(n.contains(QStringLiteral("dtc")) || n.contains(QStringLiteral("defaut")) || n.contains(QStringLiteral("panne")))
        return QStringLiteral("dtc");
    if(n.contains(QStringLiteral("commande")) || n.contains(QStringLiteral("command")))
        return QStringLiteral("command");
    if(n.contains(QStringLiteral("protocole")) || n.contains(QStringLiteral("communication")))
        return QStringLiteral("protocol");
    if(n.contains(QStringLiteral("actionneur")) || n.contains(QStringLiteral("actuator")))
        return QStringLiteral("actuator");
    if(n.contains(QStringLiteral("trame")) || n.contains(QStringLiteral("mesure")) || n.contains(QStringLiteral("pid")))
        return QStringLiteral("data");
    return QStringLiteral("documentation");
}

bool ftsEnabled(QSqlDatabase &database)
{
    QSqlQuery q(database);
    return q.exec(QStringLiteral("SELECT value FROM search_meta WHERE key='fts5_enabled'")) &&
           q.next() && q.value(0).toString()==QStringLiteral("1");
}

bool removePreviousCompleteXmlRows(QSqlDatabase &database,bool fts)
{
    QList<qlonglong> ids;
    QSqlQuery select(database);
    if(!select.exec(QStringLiteral("SELECT id FROM search_documents WHERE source_table LIKE 'xml_complete_%'"))) return false;
    while(select.next()) ids.append(select.value(0).toLongLong());

    QSqlQuery terms(database);
    terms.prepare(QStringLiteral("DELETE FROM search_terms WHERE document_id=:id"));
    QSqlQuery ftsDelete(database);
    if(fts) ftsDelete.prepare(QStringLiteral("DELETE FROM search_fts WHERE rowid=:id"));
    for(const qlonglong id:ids){
        terms.bindValue(QStringLiteral(":id"),id);
        if(!terms.exec()) return false;
        if(fts){
            ftsDelete.bindValue(QStringLiteral(":id"),id);
            if(!ftsDelete.exec()) return false;
        }
    }
    QSqlQuery docs(database);
    return docs.exec(QStringLiteral("DELETE FROM search_documents WHERE source_table LIKE 'xml_complete_%'"));
}

bool insertDocument(QSqlDatabase &database,bool fts,const QString &category,
                    const QString &sourceTable,const QString &sourceKey,const QString &generation,
                    const QString &title,const QString &content,const QString &searchable)
{
    const QString n=normalized(searchable);
    if(n.isEmpty()) return true;

    QSqlQuery insert(database);
    insert.prepare(QStringLiteral(
        "INSERT INTO search_documents(category,source_table,source_key,generation,title,content,searchable,normalized) "
        "VALUES(:category,:source_table,:source_key,:generation,:title,:content,:searchable,:normalized)"));
    insert.bindValue(QStringLiteral(":category"),category);
    insert.bindValue(QStringLiteral(":source_table"),sourceTable);
    insert.bindValue(QStringLiteral(":source_key"),sourceKey);
    insert.bindValue(QStringLiteral(":generation"),generation);
    insert.bindValue(QStringLiteral(":title"),title);
    insert.bindValue(QStringLiteral(":content"),content);
    insert.bindValue(QStringLiteral(":searchable"),searchable);
    insert.bindValue(QStringLiteral(":normalized"),n);
    if(!insert.exec()) return false;
    const qlonglong id=insert.lastInsertId().toLongLong();

    QSet<QString> uniqueTerms;
    for(const QString &term:n.split(QLatin1Char(' '),Qt::SkipEmptyParts))
        if(!term.isEmpty() && term.size()<=80) uniqueTerms.insert(term);

    QSqlQuery termInsert(database);
    termInsert.prepare(QStringLiteral("INSERT OR IGNORE INTO search_terms(term,document_id) VALUES(:term,:id)"));
    for(const QString &term:uniqueTerms){
        termInsert.bindValue(QStringLiteral(":term"),term);
        termInsert.bindValue(QStringLiteral(":id"),id);
        if(!termInsert.exec()) return false;
    }

    if(fts){
        QSqlQuery ftsInsert(database);
        ftsInsert.prepare(QStringLiteral(
            "INSERT INTO search_fts(rowid,title,searchable,normalized,category,source_table,source_key,generation) "
            "VALUES(:id,:title,:searchable,:normalized,:category,:source_table,:source_key,:generation)"));
        ftsInsert.bindValue(QStringLiteral(":id"),id);
        ftsInsert.bindValue(QStringLiteral(":title"),title);
        ftsInsert.bindValue(QStringLiteral(":searchable"),searchable);
        ftsInsert.bindValue(QStringLiteral(":normalized"),n);
        ftsInsert.bindValue(QStringLiteral(":category"),category);
        ftsInsert.bindValue(QStringLiteral(":source_table"),sourceTable);
        ftsInsert.bindValue(QStringLiteral(":source_key"),sourceKey);
        ftsInsert.bindValue(QStringLiteral(":generation"),generation);
        if(!ftsInsert.exec()) return false;
    }
    return true;
}

struct XmlLine
{
    QString section;
    QStringList cells;
    QStringList tags;
    QString searchable;
};

QList<XmlLine> parseXmlLines(const QString &path)
{
    QList<XmlLine> lines;
    QFile file(path);
    if(!file.open(QIODevice::ReadOnly|QIODevice::Text)) return lines;
    QXmlStreamReader xml(&file);
    QString section;
    while(!xml.atEnd()){
        xml.readNext();
        if(!xml.isStartElement()) continue;
        if(xml.name()==QStringLiteral("section")){
            section=xml.attributes().value(QStringLiteral("titre")).toString().simplified();
            continue;
        }
        if(xml.name()!=QStringLiteral("ligne")) continue;

        XmlLine line;
        line.section=section;
        QStringList rawParts;
        while(!xml.atEnd()){
            xml.readNext();
            if(xml.isEndElement() && xml.name()==QStringLiteral("ligne")) break;
            if(!xml.isStartElement()) continue;

            const QString tag=xml.name().toString().toCaseFolded();
            line.tags.append(tag);
            rawParts.append(tag);
            const auto attrs=xml.attributes();
            for(const QXmlStreamAttribute &attr:attrs){
                rawParts.append(attr.name().toString());
                rawParts.append(attr.value().toString());
            }

            const QString text=xml.readElementText(QXmlStreamReader::IncludeChildElements).simplified();
            if(!text.isEmpty()){
                rawParts.append(text);
                if(tag==QStringLiteral("cellule") || tag==QStringLiteral("broche") ||
                   tag==QStringLiteral("fonction") || tag==QStringLiteral("couleur"))
                    line.cells.append(text);
            }
        }
        line.searchable=rawParts.join(QLatin1Char(' '));
        if(!line.searchable.trimmed().isEmpty()) lines.append(line);
    }
    return lines;
}

bool indexCompleteXml(QSqlDatabase &database,bool fts,const QString &path,const QString &generation)
{
    QFile file(path);
    if(!file.open(QIODevice::ReadOnly|QIODevice::Text)) return false;
    const QString raw=QString::fromUtf8(file.readAll());
    if(raw.trimmed().isEmpty()) return false;

    QString plain=raw;
    plain.replace(QRegularExpression(QStringLiteral("<[^>]+>")),QStringLiteral(" "));
    plain=plain.simplified();
    if(!insertDocument(database,fts,QStringLiteral("documentation"),QStringLiteral("xml_complete_document"),
                       generation,generation,QStringLiteral("MEMS %1 XML complet").arg(generation),
                       plain,QStringLiteral("MEMS %1 %2").arg(generation,raw)))
        return false;

    const QList<XmlLine> lines=parseXmlLines(path);
    int rowNumber=0;
    for(const XmlLine &line:lines){
        ++rowNumber;
        const QString category=categoryForRow(line.section,line.tags);
        const QString visible=line.cells.isEmpty()?line.searchable:line.cells.join(QStringLiteral(" — "));
        const QString title=visible.left(220);
        const QString content=QStringLiteral("MEMS %1\n%2\n%3")
            .arg(generation,line.section,visible);
        const QString searchable=QStringLiteral("MEMS %1 %2 %3 %4 %5")
            .arg(generation,line.section,line.tags.join(QLatin1Char(' ')),line.searchable,visible);
        if(!insertDocument(database,fts,category,QStringLiteral("xml_complete_row"),
                           QStringLiteral("%1:%2").arg(generation).arg(rowNumber),generation,
                           title,content,searchable))
            return false;
    }
    return true;
}

void augmentGlobalIndex()
{
    if(!MemsGlobalSearchIndex::ensureBuilt()) return;
    MemsReferenceDatabase reference;
    if(!reference.open()) return;

    const QString connection=QStringLiteral("MEMS_COMPLETE_XML_%1").arg(QUuid::createUuid().toString());
    QSqlDatabase database=QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"),connection);
    database.setDatabaseName(MemsGlobalSearchIndex::indexPath());
    if(!database.open()){
        database=QSqlDatabase();
        QSqlDatabase::removeDatabase(connection);
        return;
    }

    const bool fts=ftsEnabled(database);
    bool ok=database.transaction();
    if(ok) ok=removePreviousCompleteXmlRows(database,fts);

    int indexedFiles=0;
    const QStringList generations={QStringLiteral("1.2"),QStringLiteral("1.3"),QStringLiteral("1.6"),QStringLiteral("1.9")};
    for(const QString &generation:generations){
        if(!ok) break;
        const QString path=reference.generationXmlPath(QStringLiteral("MEMS %1").arg(generation));
        if(path.isEmpty() || !QFileInfo::exists(path)) continue;
        ok=indexCompleteXml(database,fts,path,generation);
        if(ok) ++indexedFiles;
    }

    if(ok){
        QSqlQuery meta(database);
        meta.prepare(QStringLiteral("INSERT OR REPLACE INTO search_meta(key,value) VALUES('complete_xml_index_version','3')"));
        ok=meta.exec();
    }
    if(ok){
        QSqlQuery meta(database);
        meta.prepare(QStringLiteral("INSERT OR REPLACE INTO search_meta(key,value) VALUES('complete_xml_files',:value)"));
        meta.bindValue(QStringLiteral(":value"),QString::number(indexedFiles));
        ok=meta.exec();
    }

    if(ok) database.commit(); else database.rollback();
    database.close();
    database=QSqlDatabase();
    QSqlDatabase::removeDatabase(connection);
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

QString readCell(QXmlStreamReader &xml,const QString &endTag,bool decorate)
{
    const QString text=xml.readElementText(QXmlStreamReader::IncludeChildElements).simplified();
    if(!decorate) return text.toHtmlEscaped();
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
            const bool decorate=name==QStringLiteral("couleur");
            html+=QStringLiteral("<%1>%2</%1>").arg(tag,readCell(xml,name,decorate));
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
                for(const QString &g:{QStringLiteral("1.2"),QStringLiteral("1.3"),QStringLiteral("1.6"),QStringLiteral("1.9")})
                    if(text.contains(g)) return g;
            }
        }
    }
    if(QLineEdit *search=browser->findChild<QLineEdit*>()){
        const QString text=search->text();
        for(const QString &g:{QStringLiteral("1.2"),QStringLiteral("1.3"),QStringLiteral("1.6"),QStringLiteral("1.9")})
            if(text.contains(g)) return g;
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
    layout->addWidget(view);
    dialog.exec();
}

void patchBrowser(QWidget *browser)
{
    if(!browser || browser->property("completeSearchPatchV3").toBool()) return;
    browser->setProperty("completeSearchPatchV3",true);

    const QList<QPushButton*> buttons=browser->findChildren<QPushButton*>();
    for(QPushButton *button:buttons){
        if(!button) continue;
        const QString text=button->text();
        if(text==I18n::text(7325) || text==I18n::text(7253) || text==I18n::text(7287)){
            QObject::disconnect(button,nullptr,nullptr,nullptr);
            QObject::connect(button,&QPushButton::clicked,browser,[browser](){showRichSheet(browser);});
        }
    }

    if(QTableWidget *table=browser->findChild<QTableWidget*>()){
        QObject::connect(table,&QTableWidget::itemSelectionChanged,browser,[browser,table](){
            const int row=table->currentRow();
            if(row<0) return;
            QTableWidgetItem *titleItem=table->item(row,0);
            if(!titleItem) return;
            if(titleItem->data(Qt::UserRole+11).toString()!=QStringLiteral("xml_documentation")) return;
            const QString generation=titleItem->data(Qt::UserRole+14).toString();
            MemsReferenceDatabase reference;
            if(!reference.open()) return;
            const QString path=reference.generationXmlPath(QStringLiteral("MEMS %1").arg(generation));
            if(path.isEmpty() || !QFileInfo::exists(path)) return;
            if(QTextBrowser *detail=browser->findChild<QTextBrowser*>()) detail->setHtml(richXmlHtml(path));
        });
    }
}

class BrowserPatchInstaller : public QObject
{
public:
    explicit BrowserPatchInstaller(QObject *parent=nullptr):QObject(parent){}
    bool eventFilter(QObject *watched,QEvent *event) override
    {
        QWidget *widget=qobject_cast<QWidget*>(watched);
        if(widget && event && (event->type()==QEvent::Show || event->type()==QEvent::Polish) &&
           widget->objectName()==QStringLiteral("memsDatabaseBrowser")){
            QPointer<QWidget> guarded(widget);
            QTimer::singleShot(650,this,[guarded](){if(guarded) patchBrowser(guarded);});
        }
        return QObject::eventFilter(watched,event);
    }
};

void installCompletenessPatch()
{
    QCoreApplication *core=QCoreApplication::instance();
    if(!core) return;
    QTimer::singleShot(120,core,[](){augmentGlobalIndex();});
    BrowserPatchInstaller *installer=new BrowserPatchInstaller(core);
    core->installEventFilter(installer);
    QTimer::singleShot(900,installer,[installer](){
        for(QWidget *widget:QApplication::allWidgets())
            if(widget && widget->objectName()==QStringLiteral("memsDatabaseBrowser")) patchBrowser(widget);
    });
}

}

Q_COREAPP_STARTUP_FUNCTION(installCompletenessPatch)
