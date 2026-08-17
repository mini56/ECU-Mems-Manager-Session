#include "../i18n.h"

#include <QApplication>
#include <QCoreApplication>
#include <QEvent>
#include <QFileInfo>
#include <QPointer>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTextBrowser>
#include <QTimer>
#include <QUrl>

namespace {

QString diagramStyle()
{
    return QStringLiteral(
        "<style>body{background:#0a1015;color:#dce3e8;font-family:'Segoe UI',Arial,sans-serif;font-size:9pt;}"
        "h1{color:#ff9828;font-size:16pt;margin:0 0 6px 0;}"
        "p{line-height:1.4;margin:5px 0}.muted{color:#98a5af}"
        ".diagram{background:#0d151b;border:1px solid #34414b;padding:8px;margin:8px 0 12px 0;text-align:center;}"
        ".diagram img{max-width:100%;height:auto;}"
        ".content{white-space:pre-wrap;line-height:1.45}</style>");
}

QString generationToken(const QString &value)
{
    for(const QString &token : {QStringLiteral("1.2"), QStringLiteral("1.3"),
                                QStringLiteral("1.6"), QStringLiteral("1.9")}) {
        if(value.contains(token))
            return token;
    }
    return QString();
}

QString diagramsForGeneration(const QString &generation)
{
    const QString root=QCoreApplication::applicationDirPath()+QStringLiteral("/database/reference/images/");
    QStringList files;
    QStringList captions;

    if(generation==QStringLiteral("1.2")) {
        files << QStringLiteral("mems_1_2_ecu_connector.svg")
              << QStringLiteral("rover_rosco_3pin_black.svg");
        captions << QStringLiteral("Connecteur ECU MEMS 1.2 — 36 voies")
                 << QStringLiteral("Prise diagnostic Rover / ROSCO — 3 broches");
    } else if(generation==QStringLiteral("1.3")) {
        files << QStringLiteral("mems_1_3_ecu_connectors.svg")
              << QStringLiteral("rover_rosco_3pin_black.svg");
        captions << QStringLiteral("Connecteurs ECU MEMS 1.3 — 36 + 18 voies")
                 << QStringLiteral("Prise diagnostic Rover / ROSCO — 3 broches");
    } else if(generation==QStringLiteral("1.6")) {
        files << QStringLiteral("mems_1_6_ecu_connector.svg")
              << QStringLiteral("rover_rosco_3pin_black.svg");
        captions << QStringLiteral("Connecteur ECU MEMS 1.6 — 36 voies ; variante 36 + 18 selon véhicule")
                 << QStringLiteral("Prise diagnostic Rover / ROSCO — 3 broches");
    } else if(generation==QStringLiteral("1.9")) {
        files << QStringLiteral("mems_1_9_ecu_connector.svg")
              << QStringLiteral("mems_1_9_obd_16pin.svg");
        captions << QStringLiteral("Connecteur ECU MEMS 1.9 — 36 voies")
                 << QStringLiteral("Prise diagnostic MEMS 1.9 — 16 broches type J1962 / OBD");
    }

    QString html;
    for(int i=0;i<files.size();++i) {
        const QString path=root+files.at(i);
        if(!QFileInfo::exists(path))
            continue;
        const QString url=QUrl::fromLocalFile(path).toString().toHtmlEscaped();
        html+=QStringLiteral("<div class='diagram'><img src='%1'><p class='muted'>%2</p></div>")
                  .arg(url,captions.value(i).toHtmlEscaped());
    }
    return html;
}

class DiagramSearchController : public QObject
{
public:
    explicit DiagramSearchController(QWidget *browser)
        : QObject(browser),m_browser(browser)
    {
        if(!m_browser)
            return;
        m_results=m_browser->findChild<QTableWidget*>();
        m_detail=m_browser->findChild<QTextBrowser*>();
        if(!m_results||!m_detail)
            return;

        connect(m_results,&QTableWidget::itemSelectionChanged,this,[this](){
            QTimer::singleShot(0,this,[this](){showDiagramForSelection();});
        });
        QTimer::singleShot(0,this,[this](){showDiagramForSelection();});
    }

private:
    void showDiagramForSelection()
    {
        if(!m_results||!m_detail||m_results->currentRow()<0)
            return;
        QTableWidgetItem *item=m_results->item(m_results->currentRow(),0);
        if(!item)
            return;

        const QString category=item->data(Qt::UserRole+10).toString();
        if(category!=QStringLiteral("wiring"))
            return;

        const QString generation=generationToken(item->data(Qt::UserRole+14).toString());
        if(generation.isEmpty())
            return;
        const QString diagrams=diagramsForGeneration(generation);
        if(diagrams.isEmpty())
            return;

        const QString title=item->text().toHtmlEscaped();
        QString content=item->data(Qt::UserRole+13).toString().toHtmlEscaped();
        content.replace(QStringLiteral("\n"),QStringLiteral("<br>"));
        const QString subtitle=QStringLiteral("%1 — MEMS %2")
                                   .arg(I18n::text(7316).toHtmlEscaped(),generation.toHtmlEscaped());
        m_detail->setHtml(diagramStyle()+QStringLiteral("<h1>%1</h1><p class='muted'>%2</p>%3<div class='content'>%4</div>")
                          .arg(title,subtitle,diagrams,content));
        if(m_detail->verticalScrollBar())
            m_detail->verticalScrollBar()->setValue(0);
    }

    QPointer<QWidget> m_browser;
    QPointer<QTableWidget> m_results;
    QPointer<QTextBrowser> m_detail;
};

class DiagramSearchInstaller : public QObject
{
public:
    explicit DiagramSearchInstaller(QObject *parent=nullptr):QObject(parent){}

    bool eventFilter(QObject *watched,QEvent *event) override
    {
        QWidget *widget=qobject_cast<QWidget*>(watched);
        if(widget&&event&&(event->type()==QEvent::Show||event->type()==QEvent::Polish)&&
           widget->objectName()==QStringLiteral("memsDatabaseBrowser"))
            schedule(widget);
        return QObject::eventFilter(watched,event);
    }

    void scan()
    {
        for(QWidget *widget:QApplication::allWidgets())
            if(widget&&widget->objectName()==QStringLiteral("memsDatabaseBrowser"))
                schedule(widget);
    }

private:
    void schedule(QWidget *widget)
    {
        if(!widget||widget->property("databaseDiagramSearchPatched").toBool())
            return;
        widget->setProperty("databaseDiagramSearchPatched",true);
        QPointer<QWidget> guarded(widget);
        QTimer::singleShot(420,this,[guarded](){
            if(guarded)
                new DiagramSearchController(guarded);
        });
    }
};

void installDiagramSearchPatch()
{
    QCoreApplication *core=QCoreApplication::instance();
    if(!core)
        return;
    DiagramSearchInstaller *installer=new DiagramSearchInstaller(core);
    core->installEventFilter(installer);
    QTimer::singleShot(900,installer,[installer](){installer->scan();});
}

}

Q_COREAPP_STARTUP_FUNCTION(installDiagramSearchPatch)
