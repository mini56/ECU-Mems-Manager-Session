#include "../i18n.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QEvent>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QPointer>
#include <QRegularExpression>
#include <QScrollBar>
#include <QSet>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTextBrowser>
#include <QTimer>
#include <QUrl>

namespace {

const int RoleCategory=Qt::UserRole+10;
const int RoleSourceTable=Qt::UserRole+11;
const int RoleSourceKey=Qt::UserRole+12;
const int RoleContent=Qt::UserRole+13;
const int RoleGeneration=Qt::UserRole+14;
const int RoleOriginalTitle=Qt::UserRole+20;

QString activeLanguage()
{
    const QString language=I18n::language().left(2).toLower();
    static const QSet<QString> supported={QStringLiteral("fr"),QStringLiteral("en"),QStringLiteral("es"),
                                           QStringLiteral("it"),QStringLiteral("pt"),QStringLiteral("de")};
    return supported.contains(language)?language:QStringLiteral("en");
}

QString htmlStyle()
{
    return QStringLiteral(
        "<style>body{background:#0a1015;color:#dce3e8;font-family:'Segoe UI',Arial,sans-serif;font-size:9pt;}"
        "h1{color:#ff9828;font-size:16pt;margin:0 0 6px 0;}"
        "h2{color:#ff9828;font-size:10.5pt;border-bottom:1px solid #34414b;padding-bottom:4px;margin-top:14px;}"
        "p{line-height:1.4;margin:5px 0}.muted{color:#98a5af}"
        "table{border-collapse:collapse;width:100%;margin-top:8px;}"
        "th{color:#ff9828;text-align:left;border-bottom:1px solid #394650;padding:5px;width:34%;}"
        "td{border-bottom:1px solid #26323b;padding:5px;vertical-align:top;}"
        ".resource{background:#0d151b;border:1px solid #34414b;padding:10px;margin:8px 0 12px 0;text-align:center;}"
        ".resource img{max-width:100%;height:auto;}a{color:#ff9828;text-decoration:none;}"
        "pre{white-space:pre-wrap;word-wrap:break-word;background:#0d151b;border:1px solid #34414b;padding:8px;}</style>");
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
    return I18n::text(7326);
}

QString humanField(QString field)
{
    field.replace(QLatin1Char('_'),QLatin1Char(' '));
    if(!field.isEmpty()) field[0]=field.at(0).toUpper();
    return field;
}

int fieldTranslationKey(const QString &field)
{
    const QString f=field.toLower();
    if(f==QStringLiteral("filename") || f.endsWith(QStringLiteral("_file"))) return 7332;
    if(f.contains(QStringLiteral("description"))) return 7334;
    if(f.contains(QStringLiteral("notes")) || f.contains(QStringLiteral("note"))) return 7335;
    if(f.contains(QStringLiteral("function")) || f.contains(QStringLiteral("fonction"))) return 7336;
    if(f==QStringLiteral("code") || f.endsWith(QStringLiteral("_code"))) return 7337;
    if(f.contains(QStringLiteral("command"))) return 7338;
    if(f.contains(QStringLiteral("response"))) return 7339;
    if(f.contains(QStringLiteral("confidence"))) return 7340;
    if(f.contains(QStringLiteral("safety"))) return 7341;
    if(f.contains(QStringLiteral("availability"))) return 7342;
    if(f.contains(QStringLiteral("access_method")) || f==QStringLiteral("access")) return 7343;
    if(f==QStringLiteral("unit") || f.endsWith(QStringLiteral("_unit"))) return 7344;
    if(f.contains(QStringLiteral("formula")) || f.contains(QStringLiteral("decoding"))) return 7345;
    if(f.contains(QStringLiteral("address")) || f.contains(QStringLiteral("offset"))) return 7346;
    if(f==QStringLiteral("size") || f.endsWith(QStringLiteral("_size"))) return 7347;
    if(f.contains(QStringLiteral("source"))) return 7348;
    if(f.contains(QStringLiteral("manufacturer"))) return 7349;
    if(f==QStringLiteral("brand")) return 7350;
    if(f.contains(QStringLiteral("model"))) return 7351;
    if(f.contains(QStringLiteral("engine"))) return 7352;
    if(f.contains(QStringLiteral("market"))) return 7353;
    if(f.contains(QStringLiteral("year"))) return 7354;
    if(f==QStringLiteral("category")) return 7355;
    if(f.contains(QStringLiteral("component"))) return 7356;
    if(f==QStringLiteral("pin") || f.contains(QStringLiteral("pinout")) || f.contains(QStringLiteral("broche"))) return 7357;
    if(f.contains(QStringLiteral("signal"))) return 7358;
    if(f==QStringLiteral("value") || f.endsWith(QStringLiteral("_value"))) return 7359;
    if(f.contains(QStringLiteral("setting")) || f.contains(QStringLiteral("adaptation"))) return 7360;
    if(f==QStringLiteral("file_type")) return 7361;
    if(f==QStringLiteral("relative_path")) return 7362;
    if(f==QStringLiteral("content")) return 7364;
    if(f==QStringLiteral("name") || f.endsWith(QStringLiteral("_name")) || f==QStringLiteral("title")) return 7333;
    return 0;
}

QString translatedFieldLabel(const QString &field)
{
    const int key=fieldTranslationKey(field);
    return key>0?I18n::text(key):humanField(field);
}

struct FieldValue
{
    QString originalName;
    QString value;
};

QHash<QString,FieldValue> parseIndexedContent(const QString &content,QStringList *order=nullptr)
{
    QHash<QString,FieldValue> fields;
    const QStringList lines=content.split(QLatin1Char('\n'),Qt::SkipEmptyParts);
    for(const QString &line:lines){
        const int separator=line.indexOf(QStringLiteral(": "));
        if(separator<=0) continue;
        const QString name=line.left(separator).trimmed();
        const QString value=line.mid(separator+2).trimmed();
        if(name.isEmpty() || value.isEmpty()) continue;
        const QString key=name.toLower();
        fields.insert(key,{name,value});
        if(order && !order->contains(key)) order->append(key);
    }
    return fields;
}

bool splitLanguageField(const QString &field,QString *base,QString *language)
{
    static const QRegularExpression expression(QStringLiteral("^(.+)_(fr|en|es|it|pt|de)$"),QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch match=expression.match(field);
    if(!match.hasMatch()) return false;
    if(base) *base=match.captured(1).toLower();
    if(language) *language=match.captured(2).toLower();
    return true;
}

QString localizedTitle(const QString &content,const QString &fallback)
{
    const QHash<QString,FieldValue> fields=parseIndexedContent(content);
    if(fields.isEmpty()) return fallback;
    const QString language=activeLanguage();
    const QStringList preferred={
        QStringLiteral("part_number"),QStringLiteral("code"),QStringLiteral("command_hex"),
        QStringLiteral("component_name"),QStringLiteral("field_name"),QStringLiteral("capability"),
        QStringLiteral("setting_name"),QStringLiteral("function_name"),QStringLiteral("function"),
        QStringLiteral("rule_name"),QStringLiteral("protocol_name"),QStringLiteral("parameter"),
        QStringLiteral("subject"),QStringLiteral("topic"),QStringLiteral("source_name"),
        QStringLiteral("title"),QStringLiteral("name"),QStringLiteral("model"),QStringLiteral("filename"),QStringLiteral("system")
    };

    for(const QString &base:preferred){
        const QString localized=base+QLatin1Char('_')+language;
        if(fields.contains(localized) && !fields.value(localized).value.isEmpty()) return fields.value(localized).value;
        if(fields.contains(base) && !fields.value(base).value.isEmpty()) return fields.value(base).value;
    }
    return fallback;
}

QString localizedContentTable(const QString &content)
{
    QStringList order;
    const QHash<QString,FieldValue> fields=parseIndexedContent(content,&order);
    if(fields.isEmpty()){
        QString plain=content.toHtmlEscaped();
        plain.replace(QStringLiteral("\n"),QStringLiteral("<br>"));
        return QStringLiteral("<div>%1</div>").arg(plain);
    }

    const QString language=activeLanguage();
    QString html=QStringLiteral("<table>");
    QSet<QString> emittedBases;
    for(const QString &field:order){
        if(field==QStringLiteral("id") || field==QStringLiteral("keywords")) continue;

        QString base=field;
        QString suffixLanguage;
        const bool languageVariant=splitLanguageField(field,&base,&suffixLanguage);
        if(languageVariant && suffixLanguage!=language) continue;

        // If an explicit translation for the active language exists, the
        // unsuffixed/source-language field must never be displayed beside it.
        if(!languageVariant && fields.contains(field+QLatin1Char('_')+language)) continue;
        if(emittedBases.contains(base)) continue;

        QString selectedField=field;
        const QString localizedField=base+QLatin1Char('_')+language;
        if(fields.contains(localizedField)) selectedField=localizedField;
        if(!fields.contains(selectedField)) continue;

        const QString value=fields.value(selectedField).value.trimmed();
        if(value.isEmpty()) continue;
        emittedBases.insert(base);
        html+=QStringLiteral("<tr><th>%1</th><td>%2</td></tr>")
                  .arg(translatedFieldLabel(base).toHtmlEscaped(),value.toHtmlEscaped());
    }
    html+=QStringLiteral("</table>");
    return html;
}

QString fieldValue(const QString &content,const QString &field)
{
    return parseIndexedContent(content).value(field.toLower()).value;
}

bool isAssetTable(const QString &sourceTable)
{
    return sourceTable==QStringLiteral("wiring_assets") || sourceTable==QStringLiteral("document_assets");
}

QString resourcePath(const QString &content)
{
    const QString relative=fieldValue(content,QStringLiteral("relative_path"));
    if(relative.isEmpty()) return QString();
    const QString clean=QDir::cleanPath(relative).replace(QLatin1Char('\\'),QLatin1Char('/'));
    if(clean==QStringLiteral("..") || clean.startsWith(QStringLiteral("../")) || clean.startsWith(QLatin1Char('/'))) return QString();
    return QCoreApplication::applicationDirPath()+QStringLiteral("/database/reference/")+clean;
}

QString assetHtml(const QString &title,const QString &content,const QString &category,const QString &generation)
{
    const QString path=resourcePath(content);
    const QFileInfo info(path);
    QString subtitle=categoryLabel(category).toHtmlEscaped();
    if(!generation.trimmed().isEmpty()) subtitle+=QStringLiteral(" — MEMS ")+generation.toHtmlEscaped();

    QString html=htmlStyle()+QStringLiteral("<h1>%1</h1><p class='muted'>%2</p>")
        .arg(title.toHtmlEscaped(),subtitle);

    if(path.isEmpty() || !info.exists()){
        html+=localizedContentTable(content);
        return html;
    }

    const QString suffix=info.suffix().toLower();
    const QSet<QString> images={QStringLiteral("svg"),QStringLiteral("png"),QStringLiteral("jpg"),QStringLiteral("jpeg"),QStringLiteral("webp"),QStringLiteral("gif")};
    const QString url=QUrl::fromLocalFile(path).toString().toHtmlEscaped();
    if(images.contains(suffix)){
        html+=QStringLiteral("<div class='resource'><img src='%1'></div>").arg(url);
    }else if(suffix==QStringLiteral("txt") || suffix==QStringLiteral("md") || suffix==QStringLiteral("csv") || suffix==QStringLiteral("xml") || suffix==QStringLiteral("html") || suffix==QStringLiteral("htm")){
        QFile file(path);
        if(file.open(QIODevice::ReadOnly|QIODevice::Text)){
            QByteArray bytes=file.read(1024*1024);
            QString text=QString::fromUtf8(bytes).toHtmlEscaped();
            html+=QStringLiteral("<h2>%1</h2><pre>%2</pre>").arg(I18n::text(7364).toHtmlEscaped(),text);
        }
    }

    html+=QStringLiteral("<p><a href='%1'>%2</a></p>").arg(url,I18n::text(7363).toHtmlEscaped());
    html+=localizedContentTable(content);
    return html;
}

class UniversalDatabaseAccessController : public QObject
{
public:
    explicit UniversalDatabaseAccessController(QWidget *browser)
        : QObject(browser),m_browser(browser)
    {
        if(!m_browser) return;
        m_results=m_browser->findChild<QTableWidget*>();
        m_detail=m_browser->findChild<QTextBrowser*>();
        if(!m_results || !m_detail) return;

        m_detail->setOpenExternalLinks(true);
        connect(m_results,&QTableWidget::itemSelectionChanged,this,[this](){
            QTimer::singleShot(0,this,[this](){synchronize(true);});
        });
        m_browser->installEventFilter(this);

        m_timer=new QTimer(this);
        m_timer->setInterval(280);
        connect(m_timer,&QTimer::timeout,this,[this](){synchronize(false);});
        m_timer->start();
        QTimer::singleShot(0,this,[this](){synchronize(true);});
    }

protected:
    bool eventFilter(QObject *watched,QEvent *event) override
    {
        if(watched==m_browser && event && event->type()==QEvent::LanguageChange){
            m_lastSnapshot.clear();
            QTimer::singleShot(40,this,[this](){synchronize(true);});
        }
        return QObject::eventFilter(watched,event);
    }

private:
    QString snapshot() const
    {
        if(!m_results) return QString();
        QString value=activeLanguage()+QLatin1Char('|')+QString::number(m_results->rowCount())+QLatin1Char('|')+QString::number(m_results->currentRow());
        if(m_results->rowCount()>0){
            if(QTableWidgetItem *first=m_results->item(0,0))
                value+=QLatin1Char('|')+first->data(RoleSourceTable).toString()+QLatin1Char('|')+first->data(RoleSourceKey).toString();
            if(QTableWidgetItem *last=m_results->item(m_results->rowCount()-1,0))
                value+=QLatin1Char('|')+last->data(RoleSourceTable).toString()+QLatin1Char('|')+last->data(RoleSourceKey).toString();
        }
        return value;
    }

    void localizeResultTitles()
    {
        if(!m_results) return;
        for(int row=0;row<m_results->rowCount();++row){
            QTableWidgetItem *item=m_results->item(row,0);
            if(!item) continue;
            if(!item->data(RoleOriginalTitle).isValid()) item->setData(RoleOriginalTitle,item->text());
            const QString original=item->data(RoleOriginalTitle).toString();
            const QString content=item->data(RoleContent).toString();
            const QString title=localizedTitle(content,original);
            if(!title.isEmpty() && item->text()!=title) item->setText(title);
        }
    }

    void showSelection()
    {
        if(!m_results || !m_detail || m_results->currentRow()<0) return;
        QTableWidgetItem *item=m_results->item(m_results->currentRow(),0);
        if(!item) return;

        const QString category=item->data(RoleCategory).toString();
        const QString sourceTable=item->data(RoleSourceTable).toString();
        const QString content=item->data(RoleContent).toString();
        const QString generation=item->data(RoleGeneration).toString();
        const QString title=localizedTitle(content,item->text());

        if(isAssetTable(sourceTable)){
            m_detail->setHtml(assetHtml(title,content,category,generation));
            if(m_detail->verticalScrollBar()) m_detail->verticalScrollBar()->setValue(0);
            return;
        }

        // ECU and complete XML sheets have richer dedicated renderers. Leave
        // those views intact. Every normal indexed database row is rendered
        // here with only the active language variant visible.
        if(category==QStringLiteral("ecu") || sourceTable==QStringLiteral("xml_documentation")) return;

        QString subtitle=categoryLabel(category).toHtmlEscaped();
        if(!generation.trimmed().isEmpty()) subtitle+=QStringLiteral(" — MEMS ")+generation.toHtmlEscaped();
        m_detail->setHtml(htmlStyle()+QStringLiteral("<h1>%1</h1><p class='muted'>%2</p>%3")
                          .arg(title.toHtmlEscaped(),subtitle,localizedContentTable(content)));
        if(m_detail->verticalScrollBar()) m_detail->verticalScrollBar()->setValue(0);
    }

    void synchronize(bool force)
    {
        const QString current=snapshot();
        if(!force && current==m_lastSnapshot) return;
        m_lastSnapshot=current;
        localizeResultTitles();
        showSelection();
    }

    QPointer<QWidget> m_browser;
    QPointer<QTableWidget> m_results;
    QPointer<QTextBrowser> m_detail;
    QTimer *m_timer=nullptr;
    QString m_lastSnapshot;
};

class UniversalDatabaseAccessInstaller : public QObject
{
public:
    explicit UniversalDatabaseAccessInstaller(QObject *parent=nullptr):QObject(parent){}

    bool eventFilter(QObject *watched,QEvent *event) override
    {
        QWidget *widget=qobject_cast<QWidget*>(watched);
        if(widget && event && (event->type()==QEvent::Show || event->type()==QEvent::Polish) &&
           widget->objectName()==QStringLiteral("memsDatabaseBrowser")) schedule(widget);
        return QObject::eventFilter(watched,event);
    }

    void scan()
    {
        for(QWidget *widget:QApplication::allWidgets())
            if(widget && widget->objectName()==QStringLiteral("memsDatabaseBrowser")) schedule(widget);
    }

private:
    void schedule(QWidget *widget)
    {
        if(!widget || widget->property("databaseUniversalAccessInstalled").toBool()) return;
        widget->setProperty("databaseUniversalAccessInstalled",true);
        QPointer<QWidget> guarded(widget);
        QTimer::singleShot(1050,this,[guarded](){
            if(guarded) new UniversalDatabaseAccessController(guarded);
        });
    }
};

void installUniversalDatabaseAccess()
{
    QCoreApplication *core=QCoreApplication::instance();
    if(!core) return;
    UniversalDatabaseAccessInstaller *installer=new UniversalDatabaseAccessInstaller(core);
    core->installEventFilter(installer);
    QTimer::singleShot(1900,installer,[installer](){installer->scan();});
}

}

Q_COREAPP_STARTUP_FUNCTION(installUniversalDatabaseAccess)
