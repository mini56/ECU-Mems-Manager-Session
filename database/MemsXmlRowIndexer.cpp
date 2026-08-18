#include "../i18n.h"

#include <QAbstractItemView>
#include <QAbstractScrollArea>
#include <QApplication>
#include <QCompleter>
#include <QCoreApplication>
#include <QDialog>
#include <QEvent>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPointer>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollBar>
#include <QStringListModel>
#include <QTableWidget>
#include <QTextBrowser>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <QWidget>
#include <QtGlobal>

namespace {

const int RoleCategory=Qt::UserRole+10;
const int RoleSourceTable=Qt::UserRole+11;
const int RoleContent=Qt::UserRole+13;
const int RoleGeneration=Qt::UserRole+14;

QString normalized(QString input)
{
    input=input.normalized(QString::NormalizationForm_D).toCaseFolded();
    QString out;
    bool space=true;
    for(const QChar ch:input){
        const QChar::Category category=ch.category();
        if(category==QChar::Mark_NonSpacing || category==QChar::Mark_SpacingCombining || category==QChar::Mark_Enclosing) continue;
        if(ch.isLetterOrNumber()){out+=ch;space=false;}
        else if(!space){out+=QLatin1Char(' ');space=true;}
    }
    return out.simplified();
}

bool containsForbiddenSource(const QString &text)
{
    const QString n=normalized(text);
    return n.contains(QStringLiteral("memsfcr")) || n.contains(QStringLiteral("mems fcr"));
}

QString stripForbiddenSourceHtml(QString html)
{
    const auto opts=QRegularExpression::CaseInsensitiveOption|QRegularExpression::DotMatchesEverythingOption;
    const QString token=QStringLiteral("MEMS\\s*FCR|MemsFCR|MEMSFCR");
    for(const QString &tag:{QStringLiteral("p"),QStringLiteral("tr"),QStringLiteral("div")})
        html.remove(QRegularExpression(QStringLiteral("<%1\\b[^>]*>.*?(?:%2).*?</%1>").arg(tag,token),opts));
    html.remove(QRegularExpression(QStringLiteral("(?:%1)").arg(token),QRegularExpression::CaseInsensitiveOption));
    return html;
}

bool hasDatabaseBrowserAncestor(QObject *object)
{
    for(QObject *node=object;node;node=node->parent()){
        if(QWidget *widget=qobject_cast<QWidget*>(node))
            if(widget->objectName()==QStringLiteral("memsDatabaseBrowser")) return true;
    }
    return false;
}

void sanitizeTextBrowser(QTextBrowser *view)
{
    if(!view || view->property("memsForbiddenSanitizerInstalled").toBool()) return;
    view->setProperty("memsForbiddenSanitizerInstalled",true);
    QObject::connect(view,&QTextBrowser::textChanged,view,[view](){
        if(!view || view->property("memsForbiddenSanitizing").toBool() || !containsForbiddenSource(view->toPlainText())) return;
        const int v=view->verticalScrollBar()?view->verticalScrollBar()->value():0;
        const int h=view->horizontalScrollBar()?view->horizontalScrollBar()->value():0;
        view->setProperty("memsForbiddenSanitizing",true);
        view->setHtml(stripForbiddenSourceHtml(view->toHtml()));
        view->setProperty("memsForbiddenSanitizing",false);
        if(view->verticalScrollBar()) view->verticalScrollBar()->setValue(v);
        if(view->horizontalScrollBar()) view->horizontalScrollBar()->setValue(h);
    });
    if(containsForbiddenSource(view->toPlainText())){
        view->setProperty("memsForbiddenSanitizing",true);
        view->setHtml(stripForbiddenSourceHtml(view->toHtml()));
        view->setProperty("memsForbiddenSanitizing",false);
    }
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

QString imageRoot()
{
    return QCoreApplication::applicationDirPath()+QStringLiteral("/database/reference/images/");
}

QString ecuImage(const QString &generation)
{
    if(generation==QStringLiteral("1.2")) return imageRoot()+QStringLiteral("mems_1_2_ecu_connector.svg");
    if(generation==QStringLiteral("1.3")) return imageRoot()+QStringLiteral("mems_1_3_ecu_connector.svg");
    if(generation==QStringLiteral("1.6")) return imageRoot()+QStringLiteral("mems_1_6_ecu_connector.svg");
    if(generation==QStringLiteral("1.9")) return imageRoot()+QStringLiteral("mems_1_9_ecu_connector.svg");
    return QString();
}

QString diagnosticImage(const QString &generation)
{
    if(generation==QStringLiteral("1.9")) return imageRoot()+QStringLiteral("mems_1_9_diagnostic_16pin.svg");
    if(generation==QStringLiteral("1.2") || generation==QStringLiteral("1.3") || generation==QStringLiteral("1.6"))
        return imageRoot()+QStringLiteral("rover_mems_amp_172201_3pin.svg");
    return QString();
}

int ecuCaptionKey(const QString &generation)
{
    if(generation==QStringLiteral("1.2")) return 7367;
    if(generation==QStringLiteral("1.3")) return 7368;
    if(generation==QStringLiteral("1.6")) return 7369;
    if(generation==QStringLiteral("1.9")) return 7370;
    return 0;
}

int diagnosticCaptionKey(const QString &generation)
{
    return generation==QStringLiteral("1.9")?7372:7371;
}

QString generationFromRow(QTableWidget *table,int row)
{
    if(!table || row<0) return QString();
    if(QTableWidgetItem *item=table->item(row,0)){
        const QString role=item->data(RoleGeneration).toString();
        for(const QString &g:{QStringLiteral("1.2"),QStringLiteral("1.3"),QStringLiteral("1.6"),QStringLiteral("1.9")})
            if(role.contains(g)) return g;
    }
    if(table->columnCount()>1){
        if(QTableWidgetItem *item=table->item(row,1)){
            for(const QString &g:{QStringLiteral("1.2"),QStringLiteral("1.3"),QStringLiteral("1.6"),QStringLiteral("1.9")})
                if(item->text().contains(g)) return g;
        }
    }
    return QString();
}

bool diagnosticFirst(QTableWidget *table,int row)
{
    QString text;
    if(!table || row<0) return false;
    for(int c=0;c<table->columnCount();++c)
        if(QTableWidgetItem *item=table->item(row,c)) text+=QLatin1Char(' ')+item->text();
    if(QTableWidgetItem *item=table->item(row,0)){
        text+=QLatin1Char(' ')+item->data(RoleContent).toString();
        text+=QLatin1Char(' ')+item->data(RoleSourceTable).toString();
    }
    const QString n=normalized(text);
    return n.contains(QStringLiteral("diagnostic")) || n.contains(QStringLiteral("rosco")) ||
           n.contains(QStringLiteral("j1962")) || n.contains(QStringLiteral("obd")) ||
           n.contains(QStringLiteral("k line")) || n.contains(QStringLiteral("kline"));
}

QString diagramHtml(const QString &generation,bool diagFirst,int targetWidth)
{
    struct Entry{QString path;int key;};
    QList<Entry> entries;
    const Entry ecu{ecuImage(generation),ecuCaptionKey(generation)};
    const Entry diag{diagnosticImage(generation),diagnosticCaptionKey(generation)};
    if(diagFirst){entries<<diag<<ecu;} else {entries<<ecu<<diag;}

    QString html=QStringLiteral(
        "<style>body{background:#0a1015;color:#dce3e8;font-family:'Segoe UI',Arial,sans-serif;font-size:9pt;}"
        "h1{color:#ff9828;font-size:16pt;margin:0 0 8px 0;}"
        ".diagram{background:#0a1015;border:1px solid #34414b;padding:8px;margin:8px 0 12px;text-align:center;}"
        ".caption{color:#b5c0c7;margin:7px 0 1px;font-weight:600;}</style>");
    html+=QStringLiteral("<h1>%1</h1>").arg(I18n::text(7373).arg(generation).toHtmlEscaped());
    for(const Entry &entry:entries){
        if(entry.key==0 || entry.path.isEmpty() || !QFileInfo::exists(entry.path)) continue;
        html+=QStringLiteral("<div class='diagram'><img src='%1' width='%2'><p class='caption'>%3</p></div>")
            .arg(QUrl::fromLocalFile(entry.path).toString().toHtmlEscaped())
            .arg(targetWidth)
            .arg(I18n::text(entry.key).toHtmlEscaped());
    }
    return html;
}

void openLargeImage(QWidget *parent,const QString &path)
{
    if(path.isEmpty() || !QFileInfo::exists(path)) return;
    QDialog dialog(parent);
    dialog.setWindowTitle(I18n::text(7366));
    dialog.resize(1160,780);
    QVBoxLayout *layout=new QVBoxLayout(&dialog);
    layout->setContentsMargins(6,6,6,6);
    QTextBrowser *view=new QTextBrowser(&dialog);
    view->setLineWrapMode(QTextEdit::NoWrap);
    view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    view->setFocusPolicy(Qt::StrongFocus);
    const QString url=QUrl::fromLocalFile(path).toString().toHtmlEscaped();
    view->setHtml(QStringLiteral("<html><body style='margin:8px;background:#0a1015;text-align:left;'><img src='%1' width='1500'></body></html>").arg(url));
    layout->addWidget(view);
    dialog.exec();
}

class DatabaseUiController:public QObject
{
public:
    explicit DatabaseUiController(QWidget *browser):QObject(browser),m_browser(browser)
    {
        m_table=browser?browser->findChild<QTableWidget*>():nullptr;
        m_detail=browser?browser->findChild<QTextBrowser*>():nullptr;
        m_search=browser?browser->findChild<QLineEdit*>():nullptr;
        if(!m_table || !m_detail) return;
        configureScroll();
        configureEnlargeButton();
        sanitizeTextBrowser(m_detail);
        m_table->installEventFilter(this);
        m_table->viewport()->installEventFilter(this);
        QObject::connect(m_table,&QTableWidget::itemSelectionChanged,this,[this](){scheduleRefresh();});
        if(m_search) QObject::connect(m_search,&QLineEdit::textChanged,this,[this](){
            QTimer::singleShot(80,this,[this](){filterForbiddenRows();sanitizeCompleter();});
        });
        m_browser->installEventFilter(this);
        QTimer::singleShot(0,this,[this](){filterForbiddenRows();sanitizeCompleter();refreshSelection();});
    }

protected:
    bool eventFilter(QObject *watched,QEvent *event) override
    {
        if(!event) return QObject::eventFilter(watched,event);
        if((watched==m_table || watched==m_table->viewport()) && event->type()==QEvent::Wheel){
            QWheelEvent *wheel=static_cast<QWheelEvent*>(event);
            if(QScrollBar *bar=m_table->verticalScrollBar()){
                if(bar->maximum()>bar->minimum()){
                    const int delta=wheel->angleDelta().y();
                    const int steps=delta==0?0:qMax(1,qAbs(delta)/120);
                    bar->setValue(bar->value()+(delta>0?-1:1)*steps*qMax(28,bar->singleStep()));
                    return true;
                }
            }
        }
        if(watched==m_browser && (event->type()==QEvent::Resize || event->type()==QEvent::Show))
            QTimer::singleShot(0,this,[this](){configureScroll();configureEnlargeButton();refreshSelection();});
        if(watched==m_browser && event->type()==QEvent::LanguageChange)
            QTimer::singleShot(20,this,[this](){if(m_enlarge) m_enlarge->setText(I18n::text(7365));refreshSelection();});
        return QObject::eventFilter(watched,event);
    }

private:
    void configureScroll()
    {
        m_table->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        m_table->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        m_table->setFocusPolicy(Qt::StrongFocus);
        if(QScrollBar *bar=m_table->verticalScrollBar()){
            bar->setSingleStep(28);
            bar->setPageStep(qMax(64,m_table->viewport()->height()-40));
            styleVerticalBar(bar);
        }
        m_detail->setSizeAdjustPolicy(QAbstractScrollArea::AdjustIgnored);
        m_detail->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        m_detail->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        m_detail->setFocusPolicy(Qt::StrongFocus);
        if(QScrollBar *bar=m_detail->verticalScrollBar()){
            bar->setSingleStep(28);
            bar->setPageStep(qMax(64,m_detail->viewport()->height()-40));
            styleVerticalBar(bar);
        }
    }

    void configureEnlargeButton()
    {
        if(!m_detail || !m_detail->parentWidget()) return;
        QWidget *card=m_detail->parentWidget();
        QVBoxLayout *vertical=qobject_cast<QVBoxLayout*>(card->layout());
        if(!vertical) return;
        const int detailIndex=vertical->indexOf(m_detail);
        for(int i=vertical->count()-1;i>detailIndex;--i)
            if(QPushButton *button=qobject_cast<QPushButton*>(vertical->itemAt(i)->widget())) button->hide();
        if(m_enlarge) return;
        QHBoxLayout *tools=vertical->count()>0?qobject_cast<QHBoxLayout*>(vertical->itemAt(0)->layout()):nullptr;
        if(!tools) return;
        m_enlarge=new QPushButton(I18n::text(7365),card);
        m_enlarge->setMinimumHeight(28);
        m_enlarge->setStyleSheet(QStringLiteral(
            "QPushButton{background:#182129;color:#e9eef1;border:1px solid #3b4852;border-radius:4px;padding:4px 9px;}"
            "QPushButton:hover{border-color:#ff7a00;color:#ffad5c;}"));
        int insert=qMax(0,tools->count()-1);
        for(int i=0;i<tools->count();++i)
            if(QPushButton *button=qobject_cast<QPushButton*>(tools->itemAt(i)->widget()))
                if(button->text()==I18n::text(7325)){insert=i+1;break;}
        tools->insertWidget(insert,m_enlarge);
        QObject::connect(m_enlarge,&QPushButton::clicked,this,[this](){openLargeImage(m_browser,m_currentImage);});
    }

    void sanitizeCompleter()
    {
        if(!m_search || !m_search->completer()) return;
        QStringListModel *model=qobject_cast<QStringListModel*>(m_search->completer()->model());
        if(!model) return;
        const QStringList list=model->stringList();
        QStringList filtered;
        for(const QString &item:list) if(!containsForbiddenSource(item)) filtered<<item;
        if(filtered!=list) model->setStringList(filtered);
    }

    void filterForbiddenRows()
    {
        for(int row=m_table->rowCount()-1;row>=0;--row){
            bool forbidden=false;
            for(int c=0;c<m_table->columnCount() && !forbidden;++c)
                if(QTableWidgetItem *item=m_table->item(row,c)) forbidden=containsForbiddenSource(item->text());
            if(!forbidden){
                if(QTableWidgetItem *item=m_table->item(row,0))
                    forbidden=containsForbiddenSource(item->data(RoleContent).toString()) ||
                              containsForbiddenSource(item->data(RoleSourceTable).toString());
            }
            if(forbidden) m_table->removeRow(row);
        }
    }

    void scheduleRefresh(){QTimer::singleShot(25,this,[this](){filterForbiddenRows();refreshSelection();});}

    void refreshSelection()
    {
        if(m_table->currentRow()<0){if(m_enlarge)m_enlarge->hide();return;}
        sanitizeTextBrowser(m_detail);
        QTableWidgetItem *first=m_table->item(m_table->currentRow(),0);
        if(!first || first->data(RoleCategory).toString()!=QStringLiteral("wiring")){
            if(m_enlarge) m_enlarge->hide();
            return;
        }
        const QString generation=generationFromRow(m_table,m_table->currentRow());
        if(generation.isEmpty()){if(m_enlarge)m_enlarge->hide();return;}
        const bool diagFirst=diagnosticFirst(m_table,m_table->currentRow());
        m_currentImage=diagFirst?diagnosticImage(generation):ecuImage(generation);
        if(!QFileInfo::exists(m_currentImage)) m_currentImage.clear();
        const int width=qMax(120,m_detail->viewport()->width()-42);
        m_detail->setHtml(diagramHtml(generation,diagFirst,width));
        m_detail->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        m_detail->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        if(m_detail->verticalScrollBar()) m_detail->verticalScrollBar()->setValue(0);
        if(m_enlarge){
            m_enlarge->setText(I18n::text(7365));
            m_enlarge->setVisible(!m_currentImage.isEmpty());
            m_enlarge->setEnabled(!m_currentImage.isEmpty());
        }
    }

    QPointer<QWidget> m_browser;
    QPointer<QTableWidget> m_table;
    QPointer<QTextBrowser> m_detail;
    QPointer<QLineEdit> m_search;
    QPointer<QPushButton> m_enlarge;
    QString m_currentImage;
};

class Installer:public QObject
{
public:
    explicit Installer(QObject *parent=nullptr):QObject(parent){}
    bool eventFilter(QObject *watched,QEvent *event) override
    {
        if(!event) return QObject::eventFilter(watched,event);
        if(QWidget *widget=qobject_cast<QWidget*>(watched)){
            if((event->type()==QEvent::Show || event->type()==QEvent::Polish) &&
               widget->objectName()==QStringLiteral("memsDatabaseBrowser")) installFor(widget);
        }
        if(QTextBrowser *view=qobject_cast<QTextBrowser*>(watched)){
            if((event->type()==QEvent::Show || event->type()==QEvent::Polish) && hasDatabaseBrowserAncestor(view))
                sanitizeTextBrowser(view);
        }
        return QObject::eventFilter(watched,event);
    }
    void scan()
    {
        for(QWidget *widget:QApplication::allWidgets()){
            if(widget && widget->objectName()==QStringLiteral("memsDatabaseBrowser")) installFor(widget);
            if(QTextBrowser *view=qobject_cast<QTextBrowser*>(widget))
                if(hasDatabaseBrowserAncestor(view)) sanitizeTextBrowser(view);
        }
    }
private:
    void installFor(QWidget *browser)
    {
        if(!browser || browser->property("memsDatabaseUiControllerInstalled").toBool()) return;
        browser->setProperty("memsDatabaseUiControllerInstalled",true);
        QPointer<QWidget> guarded(browser);
        QTimer::singleShot(450,this,[guarded](){if(guarded)new DatabaseUiController(guarded);});
    }
};

void installDatabaseUiController()
{
    QCoreApplication *core=QCoreApplication::instance();
    if(!core) return;
    Installer *installer=new Installer(core);
    core->installEventFilter(installer);
    QTimer::singleShot(900,installer,[installer](){installer->scan();});
}

}

Q_COREAPP_STARTUP_FUNCTION(installDatabaseUiController)
