#include "MemsGlobalSearchIndex.h"
#include "MemsReferenceDatabase.h"

#include <QAbstractScrollArea>
#include <QApplication>
#include <QCoreApplication>
#include <QDateTime>
#include <QEvent>
#include <QFile>
#include <QFileInfo>
#include <QPointer>
#include <QRegularExpression>
#include <QScrollBar>
#include <QSet>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QTableWidget>
#include <QTextBrowser>
#include <QTimer>
#include <QUuid>
#include <QWidget>
#include <QXmlStreamReader>

namespace {

QString normalizeText(const QString &input)
{
    const QString decomposed=input.normalized(QString::NormalizationForm_D).toCaseFolded();
    QString out;
    bool previousSpace=true;
    for(const QChar ch:decomposed){
        const QChar::Category category=ch.category();
        if(category==QChar::Mark_NonSpacing || category==QChar::Mark_SpacingCombining || category==QChar::Mark_Enclosing)
            continue;
        if(ch.isLetterOrNumber()){
            out+=ch;
            previousSpace=false;
        }else if(!previousSpace){
            out+=QLatin1Char(' ');
            previousSpace=true;
        }
    }
    QStringList words=out.simplified().split(QLatin1Char(' '),Qt::SkipEmptyParts);
    for(QString &word:words){
        bool hasDigit=false;
        for(const QChar ch:word){if(ch.isDigit()){hasDigit=true;break;}}
        if(hasDigit) word.replace(QLatin1Char('o'),QLatin1Char('0'));
    }
    return words.join(QLatin1Char(' '));
}

QString categoryForSection(const QString &section)
{
    const QString n=normalizeText(section);
    if(n.contains(QStringLiteral("brochage")) || n.contains(QStringLiteral("connecteur")) ||
       n.contains(QStringLiteral("prise diagnostic")) || n.contains(QStringLiteral("pinout")))
        return QStringLiteral("wiring");
    if(n.contains(QStringLiteral("dtc")) || n.contains(QStringLiteral("defaut")) ||
       n.contains(QStringLiteral("code panne")) || n.contains(QStringLiteral("codes panne")))
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

QString aliasesForCategory(const QString &category)
{
    if(category==QStringLiteral("wiring"))
        return QStringLiteral("broche broches pin pins connecteur connectique cablage câblage fil fils wire wires wiring couleur color faisceau");
    if(category==QStringLiteral("dtc"))
        return QStringLiteral("dtc code codes defaut défaut défauts panne pannes fault faults diagnostic");
    if(category==QStringLiteral("command"))
        return QStringLiteral("commande commandes command commands requete requête diagnostic");
    if(category==QStringLiteral("protocol"))
        return QStringLiteral("protocole protocol communication liaison diagnostic");
    if(category==QStringLiteral("actuator"))
        return QStringLiteral("actionneur actionneurs actuator actuators test composant component");
    if(category==QStringLiteral("data"))
        return QStringLiteral("trame trames mesure mesures pid capteur sensor donnee donnée data octet byte");
    return QStringLiteral("documentation document fiche xml technique technical");
}

QString readElementText(QXmlStreamReader &xml)
{
    return xml.readElementText(QXmlStreamReader::IncludeChildElements).simplified();
}

struct XmlRow
{
    QString section;
    QStringList cells;
};

QList<XmlRow> parseXmlRows(const QString &path)
{
    QList<XmlRow> rows;
    QFile file(path);
    if(!file.open(QIODevice::ReadOnly|QIODevice::Text)) return rows;

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

        XmlRow row;
        row.section=section;
        while(!xml.atEnd()){
            xml.readNext();
            if(xml.isEndElement() && xml.name()==QStringLiteral("ligne")) break;
            if(!xml.isStartElement()) continue;
            const QStringRef name=xml.name();
            if(name==QStringLiteral("cellule") || name==QStringLiteral("broche") ||
               name==QStringLiteral("fonction") || name==QStringLiteral("couleur")){
                const QString text=readElementText(xml);
                if(!text.isEmpty()) row.cells.append(text);
            }
        }
        if(row.cells.size()>=2) rows.append(row);
    }
    return rows;
}

QString xmlSignature(const QStringList &paths)
{
    QStringList parts;
    parts<<QStringLiteral("xml-row-index-v2");
    for(const QString &path:paths){
        const QFileInfo info(path);
        parts<<path<<QString::number(info.size())<<QString::number(info.lastModified().toMSecsSinceEpoch());
    }
    return parts.join(QLatin1Char('|'));
}

bool currentSignature(QSqlDatabase &database,const QString &signature)
{
    QSqlQuery query(database);
    query.prepare(QStringLiteral("SELECT value FROM search_meta WHERE key='xml_row_index_signature'"));
    return query.exec() && query.next() && query.value(0).toString()==signature;
}

bool ftsEnabled(QSqlDatabase &database)
{
    QSqlQuery query(database);
    return query.exec(QStringLiteral("SELECT value FROM search_meta WHERE key='fts5_enabled'")) &&
           query.next() && query.value(0).toString()==QStringLiteral("1");
}

bool removeOldRows(QSqlDatabase &database,bool fts)
{
    QList<qlonglong> ids;
    QSqlQuery select(database);
    if(!select.exec(QStringLiteral("SELECT id FROM search_documents WHERE source_table='xml_table_row'"))) return false;
    while(select.next()) ids.append(select.value(0).toLongLong());

    QSqlQuery removeTerms(database);
    QSqlQuery removeFts(database);
    removeTerms.prepare(QStringLiteral("DELETE FROM search_terms WHERE document_id=:id"));
    if(fts) removeFts.prepare(QStringLiteral("DELETE FROM search_fts WHERE rowid=:id"));
    for(const qlonglong id:ids){
        removeTerms.bindValue(QStringLiteral(":id"),id);
        if(!removeTerms.exec()) return false;
        if(fts){
            removeFts.bindValue(QStringLiteral(":id"),id);
            if(!removeFts.exec()) return false;
        }
    }
    QSqlQuery removeDocs(database);
    return removeDocs.exec(QStringLiteral("DELETE FROM search_documents WHERE source_table='xml_table_row'"));
}

bool insertRow(QSqlDatabase &database,bool fts,const QString &generation,const XmlRow &row,int rowNumber)
{
    if(row.cells.isEmpty()) return true;
    const QString first=normalizeText(row.cells.first());
    if(first==QStringLiteral("broche") || first==QStringLiteral("pin") || first==QStringLiteral("fonction"))
        return true;

    const QString category=categoryForSection(row.section);
    const QString title=row.cells.join(QStringLiteral(" — "));
    const QString content=QStringLiteral("MEMS %1\n%2\n%3")
        .arg(generation,row.section,row.cells.join(QStringLiteral(" | ")));
    const QString searchable=QStringLiteral("MEMS %1 %2 %3 %4")
        .arg(generation,row.section,row.cells.join(QLatin1Char(' ')),aliasesForCategory(category));
    const QString normalized=normalizeText(searchable);
    if(normalized.isEmpty()) return true;

    QSqlQuery insert(database);
    insert.prepare(QStringLiteral(
        "INSERT INTO search_documents(category,source_table,source_key,generation,title,content,searchable,normalized) "
        "VALUES(:category,'xml_table_row',:source_key,:generation,:title,:content,:searchable,:normalized)"));
    insert.bindValue(QStringLiteral(":category"),category);
    insert.bindValue(QStringLiteral(":source_key"),QStringLiteral("%1:%2").arg(generation).arg(rowNumber));
    insert.bindValue(QStringLiteral(":generation"),generation);
    insert.bindValue(QStringLiteral(":title"),title);
    insert.bindValue(QStringLiteral(":content"),content);
    insert.bindValue(QStringLiteral(":searchable"),searchable);
    insert.bindValue(QStringLiteral(":normalized"),normalized);
    if(!insert.exec()) return false;
    const qlonglong id=insert.lastInsertId().toLongLong();

    QSet<QString> terms;
    for(const QString &term:normalized.split(QLatin1Char(' '),Qt::SkipEmptyParts)){
        if(!term.isEmpty() && term.size()<=80) terms.insert(term);
    }
    QSqlQuery insertTerm(database);
    insertTerm.prepare(QStringLiteral("INSERT OR IGNORE INTO search_terms(term,document_id) VALUES(:term,:id)"));
    for(const QString &term:terms){
        insertTerm.bindValue(QStringLiteral(":term"),term);
        insertTerm.bindValue(QStringLiteral(":id"),id);
        if(!insertTerm.exec()) return false;
    }

    if(fts){
        QSqlQuery insertFts(database);
        insertFts.prepare(QStringLiteral(
            "INSERT INTO search_fts(rowid,title,searchable,normalized,category,source_table,source_key,generation) "
            "VALUES(:id,:title,:searchable,:normalized,:category,'xml_table_row',:source_key,:generation)"));
        insertFts.bindValue(QStringLiteral(":id"),id);
        insertFts.bindValue(QStringLiteral(":title"),title);
        insertFts.bindValue(QStringLiteral(":searchable"),searchable);
        insertFts.bindValue(QStringLiteral(":normalized"),normalized);
        insertFts.bindValue(QStringLiteral(":category"),category);
        insertFts.bindValue(QStringLiteral(":source_key"),QStringLiteral("%1:%2").arg(generation).arg(rowNumber));
        insertFts.bindValue(QStringLiteral(":generation"),generation);
        if(!insertFts.exec()) return false;
    }
    return true;
}

void buildXmlRowIndex()
{
    if(!MemsGlobalSearchIndex::ensureBuilt()) return;

    MemsReferenceDatabase reference;
    if(!reference.open()) return;
    const QStringList generations={QStringLiteral("1.3"),QStringLiteral("1.6"),QStringLiteral("1.9")};
    QStringList paths;
    for(const QString &generation:generations)
        paths<<reference.generationXmlPath(QStringLiteral("MEMS %1").arg(generation));

    const QString signature=xmlSignature(paths);
    const QString connection=QStringLiteral("MEMS_XML_ROW_INDEX_%1").arg(QUuid::createUuid().toString());
    QSqlDatabase database=QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"),connection);
    database.setDatabaseName(MemsGlobalSearchIndex::indexPath());
    if(!database.open()){
        database=QSqlDatabase();
        QSqlDatabase::removeDatabase(connection);
        return;
    }

    if(currentSignature(database,signature)){
        database.close();
        database=QSqlDatabase();
        QSqlDatabase::removeDatabase(connection);
        return;
    }

    const bool fts=ftsEnabled(database);
    bool ok=database.transaction();
    if(ok) ok=removeOldRows(database,fts);
    if(ok){
        for(int i=0;i<paths.size() && ok;++i){
            const QList<XmlRow> rows=parseXmlRows(paths.at(i));
            int rowNumber=0;
            for(const XmlRow &row:rows){
                ++rowNumber;
                if(!insertRow(database,fts,generations.at(i),row,rowNumber)){ok=false;break;}
            }
        }
    }
    if(ok){
        QSqlQuery meta(database);
        meta.prepare(QStringLiteral("INSERT OR REPLACE INTO search_meta(key,value) VALUES('xml_row_index_signature',:value)"));
        meta.bindValue(QStringLiteral(":value"),signature);
        ok=meta.exec();
    }
    if(ok) database.commit(); else database.rollback();

    database.close();
    database=QSqlDatabase();
    QSqlDatabase::removeDatabase(connection);
}

void styleVerticalBar(QScrollBar *bar)
{
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

void styleResultScroll(QWidget *browser)
{
    if(!browser) return;

    if(QTableWidget *table=browser->findChild<QTableWidget*>()){
        table->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        styleVerticalBar(table->verticalScrollBar());
    }

    const QList<QTextBrowser*> documents=browser->findChildren<QTextBrowser*>();
    for(QTextBrowser *view:documents){
        if(!view) continue;
        view->setSizeAdjustPolicy(QAbstractScrollArea::AdjustIgnored);
        view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        view->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        view->setFocusPolicy(Qt::StrongFocus);
        QScrollBar *bar=view->verticalScrollBar();
        if(!bar) continue;
        bar->setSingleStep(28);
        const int page=view->viewport()?view->viewport()->height()-40:0;
        bar->setPageStep(page>64?page:64);
        styleVerticalBar(bar);
    }
}

class ResultScrollInstaller : public QObject
{
public:
    explicit ResultScrollInstaller(QObject *parent=nullptr):QObject(parent){}
    bool eventFilter(QObject *watched,QEvent *event) override
    {
        QWidget *widget=qobject_cast<QWidget*>(watched);
        if(widget && event && (event->type()==QEvent::Show || event->type()==QEvent::Polish) &&
           widget->objectName()==QStringLiteral("memsDatabaseBrowser")){
            QPointer<QWidget> guarded(widget);
            QTimer::singleShot(400,this,[guarded](){if(guarded) styleResultScroll(guarded);});
        }
        return QObject::eventFilter(watched,event);
    }
};

void installXmlRowIndexAndScroll()
{
    QCoreApplication *core=QCoreApplication::instance();
    if(!core) return;
    QTimer::singleShot(20,core,[](){buildXmlRowIndex();});
    ResultScrollInstaller *installer=new ResultScrollInstaller(core);
    core->installEventFilter(installer);
    QTimer::singleShot(700,installer,[installer](){
        for(QWidget *widget:QApplication::allWidgets())
            if(widget && widget->objectName()==QStringLiteral("memsDatabaseBrowser")) styleResultScroll(widget);
    });
}

}

Q_COREAPP_STARTUP_FUNCTION(installXmlRowIndexAndScroll)
