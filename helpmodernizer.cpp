#include "i18n.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QDialog>
#include <QEvent>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPixmap>
#include <QPointer>
#include <QPushButton>
#include <QScrollArea>
#include <QTextBrowser>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>

namespace {

const int ChapterCount=25;

QStringList chapterImages()
{
    return {
        QStringLiteral(":/help/screens/overview.jpg"),
        QStringLiteral(":/help/screens/options.jpg"),
        QStringLiteral(":/help/screens/options.jpg"),
        QStringLiteral(":/help/screens/overview.jpg"),
        QStringLiteral(":/help/screens/overview.jpg"),
        QStringLiteral(":/help/screens/settings.jpg"),
        QString(),
        QStringLiteral(":/help/screens/faults.jpg"),
        QStringLiteral(":/help/screens/diagnostic.jpg"),
        QStringLiteral(":/help/screens/analysis.jpg"),
        QStringLiteral(":/help/screens/measures.jpg"),
        QStringLiteral(":/help/screens/rosco.jpg"),
        QStringLiteral(":/help/screens/measures.jpg"),
        QString(),
        QStringLiteral(":/help/screens/overview.jpg"),
        QStringLiteral(":/help/screens/database.jpg"),
        QStringLiteral(":/help/screens/database.jpg"),
        QStringLiteral(":/help/screens/database.jpg"),
        QStringLiteral(":/help/screens/database.jpg"),
        QStringLiteral(":/help/screens/options.jpg"),
        QStringLiteral(":/help/screens/database.jpg"),
        QStringLiteral(":/help/screens/database.jpg"),
        QStringLiteral(":/help/screens/options.jpg"),
        QString(),
        QStringLiteral(":/help/screens/overview.jpg")
    };
}

QString qrcUrl(const QString &resource)
{
    if(!resource.startsWith(QStringLiteral(":/"))) return QString();
    return QStringLiteral("qrc")+resource;
}

QString paragraphs(QString text)
{
    text.replace(QStringLiteral("\r\n"),QStringLiteral("\n"));
    text.replace(QLatin1Char('\r'),QLatin1Char('\n'));
    QStringList parts=text.split(QStringLiteral("\n\n"),Qt::SkipEmptyParts);
    QString html;
    for(QString part:parts){
        part=part.toHtmlEscaped();
        part.replace(QLatin1Char('\n'),QStringLiteral("<br>"));
        html+=QStringLiteral("<p>%1</p>").arg(part);
    }
    return html;
}

QString buildModernHelpHtml()
{
    const QStringList images=chapterImages();
    QString html=QStringLiteral(
        "<!doctype html><html><head><meta charset='utf-8'><style>"
        "body{background:#080e13;color:#edf3f6;font-family:'Segoe UI',Arial,sans-serif;font-size:10pt;line-height:1.48;margin:18px 24px 34px;}"
        ".hero{background:#0d1820;border:1px solid #2f4652;border-left:4px solid #ff8a00;padding:18px 20px;margin-bottom:16px;}"
        ".eyebrow{color:#ff8a00;font-weight:800;font-size:8.5pt;}h1{font-size:22pt;color:#f6f9fa;margin:6px 0;}"
        ".subtitle{color:#b9c7ce;font-size:11pt}.warning{background:#171007;border:1px solid #6c471d;border-left:4px solid #ff8a00;color:#ffd6a6;padding:12px 14px;margin:14px 0 22px;}"
        ".chapter{background:#101a21;border:1px solid #263640;margin:0 0 18px;padding:14px 16px;}"
        ".kicker{color:#ff8a00;font-size:8.5pt;font-weight:800;}h2{color:#f4f7f8;font-size:16pt;margin:4px 0 9px;border-bottom:1px solid #2b3a43;padding-bottom:7px;}"
        "p{color:#c7d3d8;margin:7px 0}.screen{background:#050a0d;border:1px solid #344650;margin:13px 0 9px;text-align:center;padding:7px;}"
        ".screen img{max-width:100%;height:auto}.screenlink{color:#ff9a2f;font-size:9pt}.note{background:#0d151b;border-left:3px solid #ff8a00;padding:8px 10px;color:#aebbc3;margin-top:10px;}"
        ".footer{border-top:1px solid #263640;color:#81939d;padding-top:12px;margin-top:24px;}a{color:#ff9a2f;text-decoration:none;}"
        "</style></head><body>");
    html+=QStringLiteral("<div class='hero'><div class='eyebrow'>%1</div><h1>%2</h1><div class='subtitle'>%3</div></div>")
        .arg(I18n::text(6480).toHtmlEscaped(),I18n::text(6259).toHtmlEscaped(),I18n::text(6481).toHtmlEscaped());
    html+=QStringLiteral("<div class='warning'><b>%1 :</b> %2</div>")
        .arg(I18n::text(6261).toHtmlEscaped(),I18n::text(6262).toHtmlEscaped());

    for(int i=0;i<ChapterCount;++i){
        html+=QStringLiteral("<div class='chapter' id='modern-chapter-%1'><div class='kicker'>%2 %1</div><h2>%1. %3</h2>%4")
            .arg(i+1).arg(I18n::text(6482).toHtmlEscaped()).arg(I18n::text(6500+i).toHtmlEscaped()).arg(paragraphs(I18n::text(6600+i)));
        const QString image=images.value(i);
        if(!image.isEmpty()){
            html+=QStringLiteral("<div class='screen'><a href='helpimg:%1'><img src='%2'></a><div class='screenlink'>%3</div></div>")
                .arg(i).arg(qrcUrl(image).toHtmlEscaped()).arg(I18n::text(6483).toHtmlEscaped());
        }
        html+=QStringLiteral("</div>");
    }
    html+=QStringLiteral("<div class='footer'>%1</div></body></html>").arg(I18n::text(6485).toHtmlEscaped());
    return html;
}

class ModernHelpController : public QObject
{
public:
    explicit ModernHelpController(QDialog *dialog):QObject(dialog),m_dialog(dialog)
    {
        if(!m_dialog) return;
        m_list=m_dialog->findChild<QListWidget*>();
        m_viewer=m_dialog->findChild<QTextBrowser*>();
        m_search=m_dialog->findChild<QLineEdit*>();
        if(!m_list || !m_viewer) return;
        m_dialog->installEventFilter(this);
        apply();
    }

protected:
    bool eventFilter(QObject *watched,QEvent *event) override
    {
        if(watched==m_dialog && event){
            if(event->type()==QEvent::LanguageChange) QTimer::singleShot(20,this,[this](){apply();});
            else if(event->type()==QEvent::Show) QTimer::singleShot(0,this,[this](){applyStyle();});
        }
        return QObject::eventFilter(watched,event);
    }

private:
    void applyStyle()
    {
        if(!m_dialog) return;
        m_dialog->setStyleSheet(QStringLiteral(
            "QDialog{background:#080e13;color:#edf3f6;}QLabel{color:#edf3f6;}"
            "QPushButton{background:#111b22;color:#eef3f5;border:1px solid #3a4851;border-radius:5px;padding:6px 11px;}"
            "QPushButton:hover{border-color:#ff8a00;color:#ffb35f;background:#172129;}"
            "QLineEdit{background:#0a1117;color:#edf3f6;border:1px solid #34414b;border-radius:5px;padding:7px;selection-background-color:#8a4b00;}"
            "QListWidget{background:#091117;color:#d5e0e5;border:1px solid #263640;outline:0;padding:4px;}"
            "QListWidget::item{padding:7px;border-radius:4px;}QListWidget::item:selected{background:#3b260f;color:#ffad4d;border-left:3px solid #ff8a00;}"
            "QListWidget::item:hover{background:#121d24;color:#ffffff;}"
            "QTextBrowser{background:#080e13;color:#edf3f6;border:1px solid #263640;padding:4px;selection-background-color:#8a4b00;}"
            "QSplitter::handle{background:#263640;width:2px;}"
            "QScrollBar:vertical{background:#111a21;width:16px;border-left:1px solid #53616c;}"
            "QScrollBar::handle:vertical{background:#7c8b96;min-height:38px;border:1px solid #a0abb3;border-radius:5px;margin:2px;}"
            "QScrollBar::handle:vertical:hover{background:#ff8a22;}QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical{height:0;}"));
    }

    void apply()
    {
        if(!m_list || !m_viewer) return;
        applyStyle();
        QObject::disconnect(m_list,nullptr,m_dialog,nullptr);
        QObject::disconnect(m_viewer,nullptr,m_dialog,nullptr);
        m_list->clear();
        for(int i=0;i<ChapterCount;++i) m_list->addItem(QStringLiteral("%1. %2").arg(i+1).arg(I18n::text(6500+i)));
        m_viewer->setOpenLinks(false);
        m_viewer->setOpenExternalLinks(false);
        m_viewer->setHtml(buildModernHelpHtml());
        m_viewer->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        connect(m_list,&QListWidget::currentRowChanged,this,[this](int row){
            if(row>=0 && row<ChapterCount) m_viewer->scrollToAnchor(QStringLiteral("modern-chapter-%1").arg(row+1));
        });
        connect(m_viewer,&QTextBrowser::anchorClicked,this,[this](const QUrl &url){
            if(url.scheme()==QStringLiteral("helpimg")){
                bool ok=false; QString token=url.path(); if(token.isEmpty()) token=url.toString().section(QLatin1Char(':'),1); const int index=token.toInt(&ok); if(ok) openImage(index);
            }else if(url.scheme().isEmpty()){
                const QString anchor=url.fragment().isEmpty()?url.toString():url.fragment(); if(!anchor.isEmpty()) m_viewer->scrollToAnchor(anchor);
            }else QDesktopServices::openUrl(url);
        });
        if(m_search) m_search->setPlaceholderText(I18n::text(6253));
        if(m_list->count()>0) m_list->setCurrentRow(0);
    }

    void openImage(int index)
    {
        const QString resource=chapterImages().value(index);
        if(resource.isEmpty()) return;
        QPixmap pixmap(resource);
        if(pixmap.isNull()) return;
        QDialog dialog(m_dialog);
        dialog.setWindowTitle(I18n::text(6484));
        dialog.resize(1180,780);
        dialog.setStyleSheet(QStringLiteral("QDialog{background:#080e13;}QScrollArea{background:#080e13;border:1px solid #34414b;}"));
        QVBoxLayout *layout=new QVBoxLayout(&dialog);
        QScrollArea *scroll=new QScrollArea(&dialog);
        scroll->setWidgetResizable(false);
        scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        QLabel *image=new QLabel(scroll);
        image->setPixmap(pixmap);
        image->resize(pixmap.size());
        scroll->setWidget(image);
        layout->addWidget(scroll);
        dialog.exec();
    }

    QPointer<QDialog> m_dialog;
    QPointer<QListWidget> m_list;
    QPointer<QTextBrowser> m_viewer;
    QPointer<QLineEdit> m_search;
};

class ModernHelpInstaller : public QObject
{
public:
    explicit ModernHelpInstaller(QObject *parent=nullptr):QObject(parent){}
    bool eventFilter(QObject *watched,QEvent *event) override
    {
        QDialog *dialog=qobject_cast<QDialog*>(watched);
        if(dialog && event && (event->type()==QEvent::Show || event->type()==QEvent::Polish)) schedule(dialog);
        return QObject::eventFilter(watched,event);
    }
private:
    void schedule(QDialog *dialog)
    {
        if(!dialog || dialog->property("modernHelpPatchV3").toBool()) return;
        QListWidget *list=dialog->findChild<QListWidget*>();
        QTextBrowser *viewer=dialog->findChild<QTextBrowser*>();
        if(!list || !viewer || list->count()!=18) return;
        dialog->setProperty("modernHelpPatchV3",true);
        QPointer<QDialog> guarded(dialog);
        QTimer::singleShot(40,this,[guarded](){if(guarded) new ModernHelpController(guarded);});
    }
};

void installModernHelp()
{
    QCoreApplication *core=QCoreApplication::instance();
    if(!core) return;
    ModernHelpInstaller *installer=new ModernHelpInstaller(core);
    core->installEventFilter(installer);
}

}

Q_COREAPP_STARTUP_FUNCTION(installModernHelp)
