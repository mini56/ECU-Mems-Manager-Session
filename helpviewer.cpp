#include "helpviewer.h"

#include "i18n.h"
#include "nativewindowstyle.h"
#include <QApplication>
#include <QDesktopServices>
#include <QFile>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QRegularExpression>
#include <QScreen>
#include <QShowEvent>
#include <QSplitter>
#include <QTextDocumentFragment>
#include <QTextDocument>
#include <QSettings>

HelpViewer::HelpViewer(const QString &title,QWidget *parent)
    :QDialog(parent)
{
    setObjectName(QStringLiteral("memsProfessionalHelp"));
    setWindowTitle(title+I18n::text(6250));
    setWindowFlags(windowFlags() | Qt::WindowMaximizeButtonHint |
                   Qt::WindowMinimizeButtonHint);
    setMinimumSize(850,590);


    QScreen *screen=QGuiApplication::primaryScreen();
    const QSize available=screen?screen->availableGeometry().size():QSize(1366,768);
    resize(qMax(850,qMin(1180,available.width()-70)),
           qMax(590,qMin(840,available.height()-55)));

    m_vbox=new QVBoxLayout(this);
    m_vbox->setContentsMargins(17,15,17,14);
    m_vbox->setSpacing(12);

    QLabel *heading=new QLabel(this);
    heading->setObjectName(QStringLiteral("memsHelpHeading"));
    heading->setText(QStringLiteral("ECU MEMS Manager    |    %1").arg(
        I18n::language().startsWith(QStringLiteral("fr"),Qt::CaseInsensitive)
        ?QStringLiteral("GUIDE UTILISATEUR"):QStringLiteral("USER GUIDE")));

    m_vbox->addWidget(heading);

    QLabel *subtitle=new QLabel(this);
    subtitle->setText(I18n::language().startsWith(QStringLiteral("fr"),Qt::CaseInsensitive)
        ?QStringLiteral("Guide intégré · documentation hors ligne · recherche par rubrique")
        :QStringLiteral("Built-in guide · offline documentation · section navigation"));
    subtitle->setObjectName(QStringLiteral("memsHelpSubtitle"));
    m_vbox->addWidget(subtitle);

    QSplitter *body=new QSplitter(Qt::Horizontal,this);
    body->setObjectName(QStringLiteral("memsHelpBody"));
    body->setChildrenCollapsible(false);
    body->setHandleWidth(9);

    m_navigation=new QListWidget(body);
    m_navigation->setObjectName(QStringLiteral("memsHelpNavigation"));
    m_navigation->setMinimumWidth(225);
    m_navigation->setMaximumWidth(305);
    m_navigation->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_navigation->setWordWrap(true);
    m_navigation->setSpacing(2);

    m_viewer=new QTextBrowser(body);
    m_viewer->setObjectName(QStringLiteral("memsHelpContent"));
    m_viewer->setMinimumWidth(500);
    m_viewer->setOpenLinks(false);
    m_viewer->setOpenExternalLinks(false);
    m_viewer->document()->setDefaultFont(QFont(QStringLiteral("Segoe UI"),11));
    m_viewer->setSearchPaths(QStringList{QStringLiteral(":/help/")});

    body->addWidget(m_navigation);
    body->addWidget(m_viewer);
    body->setStretchFactor(0,0);
    body->setStretchFactor(1,1);
    body->setSizes(QList<int>{255,805});
    m_vbox->addWidget(body,1);

    connect(m_navigation,&QListWidget::currentItemChanged,this,
        [this](QListWidgetItem *current,QListWidgetItem*){
            if(current)
                m_viewer->scrollToAnchor(current->data(Qt::UserRole).toString());
        });
    connect(m_viewer,&QTextBrowser::anchorClicked,
            this,&HelpViewer::onAnchorClicked);

    m_closeButton=new QPushButton(I18n::text(6251),this);
    m_closeButton->setObjectName(QStringLiteral("memsHelpClose"));
    connect(m_closeButton,&QPushButton::clicked,
            this,&HelpViewer::onCloseClicked);
    QHBoxLayout *footer=new QHBoxLayout;
    footer->addStretch(1);
    footer->addWidget(m_closeButton);
    m_vbox->addLayout(footer);

    const bool dark=qApp && qApp->property("ecuDarkTheme").toBool();
    updateAppearance(dark);
    loadDocument();
    applyNativeTitleBar(this,dark);
}

void HelpViewer::updateAppearance(bool dark)
{
    setStyleSheet(dark
        ?QStringLiteral(
            "#memsProfessionalHelp{background:#090e13;color:#e6edf2;}"
            "#memsHelpHeading{color:#ff9828;font-size:19px;font-weight:700;}"
            "#memsHelpSubtitle{color:#9fafbc;font-size:12px;}"
            "QLabel{color:#e6edf2;background:transparent;}"
            "QListWidget{background:#101920;color:#d6e0e7;border:1px solid #2e414b;"
            "border-radius:6px;font-size:12px;padding:6px;outline:0;}"
            "QListWidget::item{padding:9px 10px;border-bottom:1px solid #23323c;}"
            "QListWidget::item:selected{background:#302418;color:#ffaf60;"
            "border-left:3px solid #ff8200;font-weight:700;}"
            "QTextBrowser{background:#0e171d;color:#e4ebf1;border:1px solid #30424c;"
            "border-radius:6px;padding:16px;selection-background-color:#5b361a;}"
            "QPushButton{background:#17232c;color:#f0f4f6;border:1px solid #40525e;"
            "border-radius:5px;padding:8px 20px;font-weight:700;}"
            "QPushButton:hover{border-color:#ff8c27;color:#ffaf60;}")
        :QStringLiteral(
            "#memsProfessionalHelp{background:#ffffff;color:#202936;}"
            "#memsHelpHeading{color:#b65c00;font-size:19px;font-weight:700;}"
            "#memsHelpSubtitle{color:#677685;font-size:12px;}"
            "QLabel{color:#202936;background:transparent;}"
            "QListWidget{background:#f5f7fa;color:#202936;border:1px solid #cbd6e0;"
            "border-radius:6px;font-size:12px;padding:6px;outline:0;}"
            "QListWidget::item{padding:9px 10px;border-bottom:1px solid #e1e8ef;}"
            "QListWidget::item:selected{background:#fff0df;color:#a45100;"
            "border-left:3px solid #f07800;font-weight:700;}"
            "QTextBrowser{background:#ffffff;color:#202936;border:1px solid #cbd6e0;"
            "border-radius:6px;padding:16px;selection-background-color:#ffe0bb;}"
            "QPushButton{background:#f0f4f8;color:#202936;border:1px solid #c0ccd8;"
            "border-radius:5px;padding:8px 20px;font-weight:700;}"
            "QPushButton:hover{border-color:#f07800;color:#a45100;}"));
}

void HelpViewer::loadDocument()
{
    QSettings settings(QSettings::IniFormat,QSettings::UserScope,PROJECTNAME);
    const QString lang=settings.value(QStringLiteral("Settings/Language"),
                                      QStringLiteral("fr")).toString().left(2);
    QFile document(lang==QStringLiteral("fr")
        ?QStringLiteral(":/help/help.html")
        :QStringLiteral(":/help/help_en.html"));
    if(!document.open(QIODevice::ReadOnly)){
        m_viewer->setPlainText(lang==QStringLiteral("fr")
            ?QStringLiteral("Guide indisponible dans cette installation.")
            :QStringLiteral("The installed help guide is unavailable."));
        return;
    }
    QString html=QString::fromUtf8(document.readAll());
    const bool dark=qApp && qApp->property("ecuDarkTheme").toBool();
    if(!dark){
        // Retain the exact offline guide content and chapter anchors.
        // Only its presentation changes to the real white/black theme.
        const int begin=html.indexOf(QStringLiteral("<style>"));
        const int end=html.indexOf(QStringLiteral("</style>"),begin);
        if(begin>=0 && end>begin){
            const QString css=QStringLiteral(R"HTML(<style>
body{font-family:"Segoe UI",Arial,sans-serif;background:#ffffff;color:#202936;margin:17px;line-height:1.56;font-size:12pt}
h1{color:#ac5900;border-bottom:2px solid #ef7c00;padding-bottom:13px;margin-top:3px;font-size:23pt}
h2{color:#a95100;background:#f5f8fb;border-left:3px solid #f07800;padding:11px;margin-top:25px;font-size:16pt}
h3{color:#a95100;margin-top:15px;font-size:13pt}
p,li{color:#202936;margin-bottom:11px}
b,strong{color:#182230}
table{border-collapse:collapse;width:100%;margin:13px 0}
th,td{border:1px solid #c8d5e0;padding:9px;text-align:left}
th{background:#edf2f7;color:#8c4700}
td{background:#ffffff;color:#202936}
.warn{background:#fff2df;border:1px solid #ddb475;color:#513515;padding:13px}
.tip{background:#edf8f2;border:1px solid #92c5a9;color:#204934;padding:12px}
.card{background:#f6f8fb;border:1px solid #d6dfe8;padding:13px}
code{background:#edf2f6;color:#8c4b08;padding:3px}
a{color:#a65500;text-decoration:none}
small{color:#647483}
</style>)HTML");
            html.replace(begin,end+QStringLiteral("</style>").size()-begin,css);
        }
    }
    m_viewer->setHtml(html);
    m_navigation->clear();

    // A single source of truth for chapter titles: the HTML bundled into
    // the Setup. New chapters therefore automatically appear in navigation.
    const QRegularExpression heading(
        QStringLiteral("<h2\\s+id=\"([^\"]+)\"[^>]*>(.*?)</h2>"),
        QRegularExpression::DotMatchesEverythingOption |
        QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatchIterator iter=heading.globalMatch(html);
    while(iter.hasNext()){
        const QRegularExpressionMatch match=iter.next();
        const QString label=QTextDocumentFragment::fromHtml(
            match.captured(2)).toPlainText().trimmed();
        if(label.isEmpty())continue;
        QListWidgetItem *item=new QListWidgetItem(label,m_navigation);
        item->setData(Qt::UserRole,match.captured(1));
    }
}

void HelpViewer::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    const bool dark=qApp && qApp->property("ecuDarkTheme").toBool();
    updateAppearance(dark);
    loadDocument();
    applyNativeTitleBar(this,dark);
}

void HelpViewer::onCloseClicked()
{
    hide();
}

void HelpViewer::onAnchorClicked(const QUrl &url)
{
    if(!url.fragment().isEmpty() && (url.scheme().isEmpty()
                                     || url.scheme()==QStringLiteral("qrc"))){
        m_viewer->scrollToAnchor(url.fragment());
        return;
    }
    if(url.scheme()==QStringLiteral("https"))
        QDesktopServices::openUrl(url);
}
