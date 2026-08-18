#include "../i18n.h"

#include <QAbstractItemView>
#include <QAbstractScrollArea>
#include <QApplication>
#include <QColor>
#include <QCompleter>
#include <QCoreApplication>
#include <QDir>
#include <QEvent>
#include <QFileInfo>
#include <QLineEdit>
#include <QPointer>
#include <QScrollBar>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTextBrowser>
#include <QTimer>
#include <QUrl>
#include <QWheelEvent>

namespace {

const int RoleCategory=Qt::UserRole+10;
const int RoleSourceTable=Qt::UserRole+11;
const int RoleContent=Qt::UserRole+13;
const int RoleGeneration=Qt::UserRole+14;

QString fieldValue(const QString &content,const QString &field)
{
    const QString prefix=field+QStringLiteral(": ");
    const QStringList lines=content.split(QLatin1Char('\n'),Qt::SkipEmptyParts);
    for(const QString &line:lines){
        if(line.startsWith(prefix,Qt::CaseInsensitive)) return line.mid(prefix.size()).trimmed();
    }
    return QString();
}

int connectorTitleKey(const QString &content)
{
    const QString path=fieldValue(content,QStringLiteral("relative_path")).toLower();
    const QString file=fieldValue(content,QStringLiteral("filename")).toLower();
    const QString token=path+QLatin1Char(' ')+file;
    if(token.contains(QStringLiteral("mems_1_2_ecu_connector"))) return 7480;
    if(token.contains(QStringLiteral("mems_1_3_ecu_connector")) || token.contains(QStringLiteral("mems_1_3_ecu_connectors"))) return 7481;
    if(token.contains(QStringLiteral("mems_1_6_ecu_connector"))) return 7482;
    if(token.contains(QStringLiteral("mems_1_9_ecu_connector"))) return 7483;
    if(token.contains(QStringLiteral("rover_mems_amp_172201_3pin")) || token.contains(QStringLiteral("rover_rosco_3pin_black"))) return 7484;
    if(token.contains(QStringLiteral("mems_1_9_diagnostic_16pin")) || token.contains(QStringLiteral("mems_1_9_obd_16pin"))) return 7485;
    return 0;
}

QString connectorPath(const QString &content)
{
    QString relative=fieldValue(content,QStringLiteral("relative_path"));
    if(relative.isEmpty()) return QString();
    relative=QDir::cleanPath(relative).replace(QLatin1Char('\\'),QLatin1Char('/'));
    if(relative==QStringLiteral("..") || relative.startsWith(QStringLiteral("../")) || relative.startsWith(QLatin1Char('/'))) return QString();
    return QCoreApplication::applicationDirPath()+QStringLiteral("/database/reference/")+relative;
}

QString pageStyle()
{
    return QStringLiteral(
        "<style>body{background:#0a1015;color:#e7edf1;font-family:'Segoe UI',Arial,sans-serif;font-size:9.5pt;margin:8px;}"
        "h1{color:#ff8a00;font-size:16pt;margin:0 0 5px 0;}h2{color:#ff9a2f;font-size:10.5pt;margin:10px 0 5px;}"
        "p{color:#cbd5da;margin:5px 0;line-height:1.45}.muted{color:#aebbc3}.img{background:#0a1015;border:1px solid #34414b;text-align:center;padding:10px;}"
        ".img img{max-width:100%;height:auto;}.content{background:#0d151b;border:1px solid #34414b;padding:9px;white-space:pre-wrap;}</style>");
}

QString connectorHtml(const QString &title,const QString &path,const QString &generation)
{
    const QString url=QUrl::fromLocalFile(path).toString().toHtmlEscaped();
    QString subtitle=I18n::text(7486).toHtmlEscaped();
    if(!generation.trimmed().isEmpty()) subtitle+=QStringLiteral(" — MEMS ")+generation.toHtmlEscaped();
    return pageStyle()+QStringLiteral("<h1>%1</h1><p class='muted'>%2</p><div class='img'><img src='%3'></div>")
        .arg(title.toHtmlEscaped(),subtitle,url);
}

QString indexedXmlHtml(const QString &title,const QString &content,const QString &category,const QString &generation)
{
    QString subtitle;
    if(category==QStringLiteral("wiring")) subtitle=I18n::text(7316);
    else if(category==QStringLiteral("dtc")) subtitle=I18n::text(7314);
    else if(category==QStringLiteral("command")) subtitle=I18n::text(7315);
    else if(category==QStringLiteral("protocol")) subtitle=I18n::text(7317);
    else if(category==QStringLiteral("data")) subtitle=I18n::text(7320);
    else subtitle=I18n::text(7318);
    if(!generation.trimmed().isEmpty()) subtitle+=QStringLiteral(" — MEMS ")+generation;

    QStringList lines=content.split(QLatin1Char('\n'),Qt::SkipEmptyParts);
    QString body;
    for(QString line:lines){
        line=line.trimmed();
        if(line.isEmpty() || line.compare(QStringLiteral("MEMS ")+generation,Qt::CaseInsensitive)==0) continue;
        body+=QStringLiteral("<p>%1</p>").arg(line.toHtmlEscaped());
    }
    return pageStyle()+QStringLiteral("<h1>%1</h1><p class='muted'>%2</p><div class='content'>%3</div>")
        .arg(title.toHtmlEscaped(),subtitle.toHtmlEscaped(),body);
}

bool belongsToDatabaseBrowser(QWidget *widget)
{
    for(QWidget *current=widget;current;current=current->parentWidget()){
        if(current->objectName()==QStringLiteral("memsDatabaseBrowser")) return true;
    }
    return false;
}

class ForcedWheelScroller : public QObject
{
public:
    explicit ForcedWheelScroller(QAbstractScrollArea *area):QObject(area),m_area(area)
    {
        if(m_area && m_area->viewport()) m_area->viewport()->installEventFilter(this);
    }
protected:
    bool eventFilter(QObject *watched,QEvent *event) override
    {
        if(!m_area || watched!=m_area->viewport() || !event || event->type()!=QEvent::Wheel)
            return QObject::eventFilter(watched,event);
        QScrollBar *bar=m_area->verticalScrollBar();
        if(!bar || bar->maximum()<=bar->minimum()) return false;
        QWheelEvent *wheel=static_cast<QWheelEvent*>(event);
        int amount=0;
        if(!wheel->pixelDelta().isNull()) amount=wheel->pixelDelta().y();
        else if(!wheel->angleDelta().isNull()) amount=(wheel->angleDelta().y()/120)*qMax(42,bar->singleStep()*3);
        if(amount==0) return false;
        bar->setValue(qBound(bar->minimum(),bar->value()-amount,bar->maximum()));
        wheel->accept();
        return true;
    }
private:
    QPointer<QAbstractScrollArea> m_area;
};

void forceWheelScroll(QAbstractScrollArea *area)
{
    if(!area || area->property("memsForcedWheelScrollV2").toBool()) return;
    area->setProperty("memsForcedWheelScrollV2",true);
    area->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    if(QScrollBar *bar=area->verticalScrollBar()){
        bar->setSingleStep(42);
        bar->show();
    }
    new ForcedWheelScroller(area);
}

class DatabaseStabilityController : public QObject
{
public:
    explicit DatabaseStabilityController(QWidget *browser)
        : QObject(browser),m_browser(browser)
    {
        if(!m_browser) return;
        m_search=m_browser->findChild<QLineEdit*>();
        m_results=m_browser->findChild<QTableWidget*>();
        m_detail=m_browser->findChild<QTextBrowser*>();
        if(!m_search || !m_results || !m_detail) return;

        disableCompletion();
        configureViews();
        m_browser->installEventFilter(this);

        connect(m_results,&QTableWidget::itemSelectionChanged,this,[this](){
            QTimer::singleShot(80,this,[this](){normalizeRows();renderStableSelection();});
        });
        connect(m_search,&QLineEdit::textEdited,this,[this](const QString &text){
            m_lastTyped=text;
            QTimer::singleShot(260,this,[this](){
                disableCompletion();
                if(m_search && m_search->hasFocus() && !m_lastTyped.isNull() && m_search->text()!=m_lastTyped){
                    m_search->setText(m_lastTyped);
                    m_search->setCursorPosition(m_lastTyped.size());
                }
                normalizeRows();
            });
        });

        m_refresh=new QTimer(this);
        m_refresh->setInterval(240);
        connect(m_refresh,&QTimer::timeout,this,[this](){disableCompletion();normalizeRows();});
        m_refresh->start();
        QTimer::singleShot(0,this,[this](){normalizeRows();renderStableSelection();});
    }

protected:
    bool eventFilter(QObject *watched,QEvent *event) override
    {
        if(watched==m_browser && event && event->type()==QEvent::LanguageChange)
            QTimer::singleShot(60,this,[this](){normalizeRows();renderStableSelection();});
        if(watched==m_browser && event && (event->type()==QEvent::Show || event->type()==QEvent::Resize))
            QTimer::singleShot(0,this,[this](){configureViews();});
        return QObject::eventFilter(watched,event);
    }

private:
    void disableCompletion()
    {
        if(!m_search) return;
        if(QCompleter *completer=m_search->completer()){
            QObject::disconnect(completer,nullptr,nullptr,nullptr);
            if(completer->popup()) completer->popup()->hide();
            m_search->setCompleter(nullptr);
        }
    }

    void configureViews()
    {
        if(m_results){
            m_results->setEnabled(true);
            m_results->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
            m_results->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
            m_results->setStyleSheet(QStringLiteral(
                "QTableWidget{background:#0a1117;color:#e7edf1;alternate-background-color:#0e151b;border:1px solid #27323b;outline:0;}"
                "QTableWidget::item{color:#e7edf1;background:transparent;padding:4px;border:0;}"
                "QTableWidget::item:selected{background:#3a2614;color:#ffffff;}"
                "QTableWidget::item:disabled{color:#e7edf1;}"
                "QHeaderView::section{background:#141c23;color:#f3f6f8;border:0;border-right:1px solid #29343e;border-bottom:2px solid #ff7a00;padding:5px;font-weight:700;}"
                "QScrollBar:vertical{background:#111a21;width:16px;border-left:1px solid #53616c;}"
                "QScrollBar::handle:vertical{background:#7c8b96;min-height:38px;border:1px solid #a0abb3;border-radius:5px;margin:2px;}"
                "QScrollBar::handle:vertical:hover{background:#ff8a22;}QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical{height:0;}"));
            forceWheelScroll(m_results);
        }
        if(m_detail){
            m_detail->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
            m_detail->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
            m_detail->setFocusPolicy(Qt::StrongFocus);
            forceWheelScroll(m_detail);
        }
        for(QAbstractScrollArea *area:m_browser->findChildren<QAbstractScrollArea*>()) forceWheelScroll(area);
    }

    void normalizeRows()
    {
        if(!m_results) return;
        const QColor normal(QStringLiteral("#e7edf1"));
        for(int r=0;r<m_results->rowCount();++r){
            for(int c=0;c<qMin(3,m_results->columnCount());++c){
                if(QTableWidgetItem *item=m_results->item(r,c)){
                    item->setForeground(normal);
                    item->setFlags(item->flags()|Qt::ItemIsEnabled|Qt::ItemIsSelectable);
                }
            }
            QTableWidgetItem *title=m_results->item(r,0);
            if(!title) continue;
            const int key=connectorTitleKey(title->data(RoleContent).toString());
            if(key>0) title->setText(I18n::text(key));
        }
    }

    void renderStableSelection()
    {
        if(!m_results || !m_detail || m_results->currentRow()<0) return;
        QTableWidgetItem *titleItem=m_results->item(m_results->currentRow(),0);
        if(!titleItem) return;
        const QString source=titleItem->data(RoleSourceTable).toString();
        const QString content=titleItem->data(RoleContent).toString();
        const QString category=titleItem->data(RoleCategory).toString();
        const QString generation=titleItem->data(RoleGeneration).toString();

        if(source==QStringLiteral("wiring_assets")){
            const int key=connectorTitleKey(content);
            if(key<=0) return;
            const QString path=connectorPath(content);
            if(path.isEmpty() || !QFileInfo::exists(path)) return;
            m_detail->setHtml(connectorHtml(I18n::text(key),path,generation));
        }else if(source==QStringLiteral("xml_row") || source==QStringLiteral("xml_text")){
            m_detail->setHtml(indexedXmlHtml(titleItem->text(),content,category,generation));
        }else{
            return;
        }

        m_detail->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        m_detail->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        forceWheelScroll(m_detail);
        if(QScrollBar *bar=m_detail->verticalScrollBar()){bar->setValue(0);bar->show();}
    }

    QPointer<QWidget> m_browser;
    QPointer<QLineEdit> m_search;
    QPointer<QTableWidget> m_results;
    QPointer<QTextBrowser> m_detail;
    QTimer *m_refresh=nullptr;
    QString m_lastTyped;
};

class DatabaseStabilityInstaller : public QObject
{
public:
    explicit DatabaseStabilityInstaller(QObject *parent=nullptr):QObject(parent){}
    bool eventFilter(QObject *watched,QEvent *event) override
    {
        QWidget *widget=qobject_cast<QWidget*>(watched);
        if(!widget || !event) return QObject::eventFilter(watched,event);

        if((event->type()==QEvent::Show || event->type()==QEvent::Polish)){
            if(widget->objectName()==QStringLiteral("memsDatabaseBrowser")) schedule(widget);
            if(QAbstractScrollArea *area=qobject_cast<QAbstractScrollArea*>(widget)){
                if(belongsToDatabaseBrowser(area)) forceWheelScroll(area);
            }
        }
        return QObject::eventFilter(watched,event);
    }
    void scan()
    {
        for(QWidget *w:QApplication::allWidgets()){
            if(!w) continue;
            if(w->objectName()==QStringLiteral("memsDatabaseBrowser")) schedule(w);
            if(QAbstractScrollArea *area=qobject_cast<QAbstractScrollArea*>(w))
                if(belongsToDatabaseBrowser(area)) forceWheelScroll(area);
        }
    }
private:
    void schedule(QWidget *widget)
    {
        if(!widget || widget->property("databaseStabilityPatchV2").toBool()) return;
        widget->setProperty("databaseStabilityPatchV2",true);
        QPointer<QWidget> guarded(widget);
        QTimer::singleShot(1300,this,[guarded](){if(guarded) new DatabaseStabilityController(guarded);});
    }
};

void installDatabaseStabilityPatch()
{
    QCoreApplication *core=QCoreApplication::instance();
    if(!core) return;
    DatabaseStabilityInstaller *installer=new DatabaseStabilityInstaller(core);
    core->installEventFilter(installer);
    QTimer::singleShot(1500,installer,[installer](){installer->scan();});
}

}

Q_COREAPP_STARTUP_FUNCTION(installDatabaseStabilityPatch)
