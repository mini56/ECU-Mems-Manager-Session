#include "helpviewer.h"

#include "i18n.h"
#include "nativewindowstyle.h"
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
    setStyleSheet(QStringLiteral(
        "#memsProfessionalHelp{background:#090e13;color:#e6edf2;}"
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
        "QPushButton:hover{border-color:#ff8c27;color:#ffaf60;}"));

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
    heading->setStyleSheet(QStringLiteral(
        "color:#ff9828;font-size:19px;font-weight:700;"));
    m_vbox->addWidget(heading);

    QLabel *subtitle=new QLabel(this);
    subtitle->setText(I18n::language().startsWith(QStringLiteral("fr"),Qt::CaseInsensitive)
        ?QStringLiteral("Guide intégré · documentation hors ligne · recherche par rubrique")
        :QStringLiteral("Built-in guide · offline documentation · section navigation"));
    subtitle->setStyleSheet(QStringLiteral("color:#9fafbc;font-size:12px;"));
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

    loadDocument();
    applyNativeTitleBar(this,true);
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
    const QString html=QString::fromUtf8(document.readAll());
    m_viewer->setHtml(html);

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
    applyNativeTitleBar(this,true);
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
