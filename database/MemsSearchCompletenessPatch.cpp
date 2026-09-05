#include "MemsReferenceDatabase.h"
#include "MemsGlobalSearchIndex.h"
#include "../i18n.h"

#include <QApplication>
#include <QComboBox>
#include <QCoreApplication>
#include <QDialog>
#include <QEvent>
#include <QFile>
#include <QFileInfo>
#include <QLineEdit>
#include <QMetaObject>
#include <QPointer>
#include <QPushButton>
#include <QRegularExpression>
#include <QSet>
#include <QStandardPaths>
#include <QTableWidget>
#include <QTextBrowser>
#include <QTimer>
#include <QVariantMap>
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
    layout->addWidget(view);
    dialog.exec();
}

void patchBrowser(QWidget *browser)
{
    if(!browser || browser->property("richSheetRendererV4").toBool()) return;
    browser->setProperty("richSheetRendererV4",true);
    const QList<QPushButton*> buttons=browser->findChildren<QPushButton*>();
    for(QPushButton *button:buttons){
        if(!button) continue;
        const QString text=button->text();
        if(text==I18n::text(7325) || text==I18n::text(7253) || text==I18n::text(7287)){
            QObject::disconnect(button,nullptr,nullptr,nullptr);
            QObject::connect(button,&QPushButton::clicked,browser,[browser](){showRichSheet(browser);});
        }
    }
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

namespace {

QStringList meaningfulIaTerms(const QString &question)
{
    static const QSet<QString> stop={
        QStringLiteral("que"),QStringLiteral("quoi"),QStringLiteral("quel"),QStringLiteral("quelle"),
        QStringLiteral("le"),QStringLiteral("la"),QStringLiteral("les"),QStringLiteral("un"),QStringLiteral("une"),
        QStringLiteral("de"),QStringLiteral("du"),QStringLiteral("des"),QStringLiteral("sur"),QStringLiteral("dans"),
        QStringLiteral("pour"),QStringLiteral("avec"),QStringLiteral("est"),QStringLiteral("sais"),QStringLiteral("dire")
    };
    QStringList out;
    for(const QString &word:normalized(question).split(QLatin1Char(' '),Qt::SkipEmptyParts)){
        if(word.size()<3 || stop.contains(word)) continue;
        if(!out.contains(word)) out.append(word);
    }
    return out;
}

QString contentField(const QString &content,const QString &field)
{
    const QString prefix=field+QStringLiteral(":");
    for(const QString &rawLine:content.split(QLatin1Char('\n'))){
        const QString line=rawLine.trimmed();
        if(line.startsWith(prefix,Qt::CaseInsensitive))
            return line.mid(prefix.size()).trimmed();
    }
    return QString();
}

QString humanScopeLabel(QString value)
{
    value=value.simplified();
    value.replace(QLatin1Char('_'),QLatin1Char(' '));
    value.replace(QLatin1Char('-'),QLatin1Char(' '));
    const QString n=normalized(value);
    if(n==QStringLiteral("rear brake drum")) return QStringLiteral("Tambour de frein arrière");
    if(n==QStringLiteral("front brake caliper")) return QStringLiteral("Étrier de frein avant");
    if(n==QStringLiteral("front brake pads")) return QStringLiteral("Plaquettes de frein avant");
    if(n==QStringLiteral("rear brake shoes")) return QStringLiteral("Segments de frein arrière");
    if(n==QStringLiteral("handbrake") || n==QStringLiteral("parking brake")) return QStringLiteral("Frein à main");
    if(n==QStringLiteral("master cylinder")) return QStringLiteral("Maître-cylindre");
    if(n==QStringLiteral("brake servo")) return QStringLiteral("Servocommande de frein");
    if(value.size()>88) value=value.left(85)+QStringLiteral("...");
    return value;
}

bool genericScopeLabel(const QString &label,const QString &queryTerm)
{
    const QString n=normalized(label);
    return n.isEmpty()
        || n==queryTerm
        || n==QStringLiteral("freins")
        || n==QStringLiteral("brakes")
        || n==QStringLiteral("couple de serrage")
        || n==QStringLiteral("torque")
        || n.contains(QStringLiteral("table des matieres"));
}

QStringList documentaryScopeChoices(const QString &question)
{
    const QStringList terms=meaningfulIaTerms(question);
    if(terms.size()!=1) return QStringList();

    static const QSet<QString> preciseTokens={
        QStringLiteral("ckp"),QStringLiteral("ect"),QStringLiteral("iat"),QStringLiteral("map"),
        QStringLiteral("tps"),QStringLiteral("iac"),QStringLiteral("iacv"),QStringLiteral("ecu"),
        QStringLiteral("spi"),QStringLiteral("mpi"),QStringLiteral("obd"),QStringLiteral("rosco")
    };
    const QString queryTerm=terms.first();
    if(preciseTokens.contains(queryTerm)) return QStringList();

    const QVariantList rows=MemsGlobalSearchIndex::search(question,QString(),40);
    if(rows.size()<10) return QStringList();

    QStringList choices;
    QSet<QString> seen;
    const QStringList fields={
        QStringLiteral("topic"),QStringLiteral("component_name"),QStringLiteral("subject"),
        QStringLiteral("component_key"),QStringLiteral("title_source"),QStringLiteral("operation"),
        QStringLiteral("source_section")
    };

    for(const QVariant &item:rows){
        const QVariantMap row=item.toMap();
        const QString content=row.value(QStringLiteral("content")).toString();
        QStringList candidates;
        for(const QString &field:fields){
            const QString value=contentField(content,field);
            if(!value.isEmpty()) candidates.append(value);
        }
        candidates.append(row.value(QStringLiteral("title")).toString());

        for(QString candidate:candidates){
            candidate=humanScopeLabel(candidate);
            if(genericScopeLabel(candidate,queryTerm)) continue;
            QString signature=normalized(candidate);
            if(signature.isEmpty() || seen.contains(signature)) continue;
            if(!signature.contains(queryTerm) && queryTerm.size()>3)
                candidate=QStringLiteral("%1 — %2").arg(queryTerm,candidate);
            signature=normalized(candidate);
            if(seen.contains(signature)) continue;
            seen.insert(signature);
            choices.append(candidate);
            break;
        }
        if(choices.size()>=5) break;
    }

    return choices.size()>=3?choices:QStringList();
}

void appendIaTranscript(QTextBrowser *browser,const QString &speaker,const QString &text)
{
    if(!browser) return;
    QString safe=text.toHtmlEscaped();
    safe.replace(QLatin1Char('\n'),QStringLiteral("<br>"));
    browser->append(QStringLiteral("<p><b>%1</b><br>%2</p>").arg(speaker.toHtmlEscaped(),safe));
}

class IaDocumentaryClarificationController : public QObject
{
public:
    explicit IaDocumentaryClarificationController(QWidget *tab)
        :QObject(tab),m_tab(tab)
    {
        if(!m_tab) return;
        m_question=m_tab->findChild<QLineEdit*>(QStringLiteral("iaMemsQuestion"));
        m_send=m_tab->findChild<QPushButton*>(QStringLiteral("iaMemsSend"));
        m_transcript=m_tab->findChild<QTextBrowser*>(QStringLiteral("iaMemsTranscript"));
        if(!m_question || !m_send) return;

        QObject::disconnect(m_send,nullptr,m_tab,nullptr);
        QObject::disconnect(m_question,nullptr,m_tab,nullptr);
        QObject::connect(m_send,&QPushButton::clicked,this,[this](){handleSend();});
        QObject::connect(m_question,&QLineEdit::returnPressed,this,[this](){handleSend();});
        m_tab->setProperty("iaDocumentaryClarificationTest",true);
    }

private:
    void invokeOriginalSend()
    {
        if(m_tab)
            QMetaObject::invokeMethod(m_tab.data(),"sendQuestion",Qt::DirectConnection);
    }

    void handleSend()
    {
        if(!m_question || !m_tab) return;
        const QString raw=m_question->text().trimmed();
        if(raw.isEmpty()) return;

        if(!m_pendingQuestion.isEmpty()){
            bool ok=false;
            const int selected=raw.toInt(&ok);
            QString refined=raw;
            if(ok && selected>=1 && selected<=m_choices.size())
                refined=m_choices.at(selected-1);
            else if(!normalized(raw).contains(normalized(m_pendingQuestion)))
                refined=QStringLiteral("%1 %2").arg(m_pendingQuestion,raw).simplified();
            m_pendingQuestion.clear();
            m_choices.clear();
            m_question->setText(refined);
            invokeOriginalSend();
            return;
        }

        const QStringList choices=documentaryScopeChoices(raw);
        if(choices.isEmpty()){
            invokeOriginalSend();
            return;
        }

        m_pendingQuestion=raw;
        m_choices=choices;
        m_question->clear();
        appendIaTranscript(m_transcript,QStringLiteral("Vous"),raw);
        QString prompt=QStringLiteral("J'ai trouvé de nombreuses réponses correspondant à « %1 ». Quel périmètre voulez-vous consulter ?").arg(raw);
        for(int i=0;i<m_choices.size();++i)
            prompt+=QStringLiteral("\n%1. %2").arg(i+1).arg(m_choices.at(i));
        prompt+=QStringLiteral("\nRépondez par le numéro ou précisez directement le périmètre.");
        appendIaTranscript(m_transcript,QStringLiteral("IA MEMS"),prompt);
        m_question->setFocus();
    }

    QPointer<QWidget> m_tab;
    QPointer<QLineEdit> m_question;
    QPointer<QPushButton> m_send;
    QPointer<QTextBrowser> m_transcript;
    QString m_pendingQuestion;
    QStringList m_choices;
};

void patchIaDocumentaryClarification(QWidget *tab)
{
    if(!tab || tab->property("iaDocumentaryClarificationTest").toBool()) return;
    new IaDocumentaryClarificationController(tab);
}

class IaDocumentaryClarificationInstaller : public QObject
{
public:
    explicit IaDocumentaryClarificationInstaller(QObject *parent=nullptr):QObject(parent){}
    bool eventFilter(QObject *watched,QEvent *event) override
    {
        QWidget *widget=qobject_cast<QWidget*>(watched);
        if(widget && event && (event->type()==QEvent::Show || event->type()==QEvent::Polish)
           && widget->objectName()==QStringLiteral("ia_mems_tab")){
            QPointer<QWidget> guarded(widget);
            QTimer::singleShot(100,this,[guarded](){if(guarded) patchIaDocumentaryClarification(guarded);});
        }
        return QObject::eventFilter(watched,event);
    }
};

void installIaDocumentaryClarification()
{
    QCoreApplication *core=QCoreApplication::instance();
    if(!core) return;
    IaDocumentaryClarificationInstaller *installer=new IaDocumentaryClarificationInstaller(core);
    core->installEventFilter(installer);
    QTimer::singleShot(350,installer,[installer](){
        Q_UNUSED(installer)
        for(QWidget *widget:QApplication::allWidgets())
            if(widget && widget->objectName()==QStringLiteral("ia_mems_tab"))
                patchIaDocumentaryClarification(widget);
    });
}

}

Q_COREAPP_STARTUP_FUNCTION(installIaDocumentaryClarification)
