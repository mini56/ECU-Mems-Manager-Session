#include "helpviewer.h"
#include "i18n.h"

#include <QEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPixmap>
#include <QPushButton>
#include <QScrollArea>
#include <QSplitter>
#include <QTextBrowser>
#include <QVBoxLayout>

#ifndef APP_VERSION
#define APP_VERSION "local"
#endif

namespace {
const int kChapterCount = 25;

QString paragraphHtml(QString text)
{
    text.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    text.replace(QLatin1Char('\r'), QLatin1Char('\n'));
    const QStringList parts = text.split(QStringLiteral("\n\n"), Qt::SkipEmptyParts);
    QString html;
    for (QString part : parts) {
        part = part.toHtmlEscaped();
        part.replace(QLatin1Char('\n'), QStringLiteral("<br>"));
        html += QStringLiteral("<p>%1</p>").arg(part);
    }
    return html;
}

QString darkButtonStyle()
{
    return QStringLiteral(
        "QPushButton{background:#111a21;color:#eef3f5;border:1px solid #3a4851;border-radius:5px;padding:6px 10px;font-weight:600;}"
        "QPushButton:hover{border-color:#ff8a00;color:#ffb35f;background:#172129;}"
        "QPushButton:pressed{background:#21170d;}");
}
}

HelpViewer::HelpViewer(const QString title, QWidget *parent)
    : QDialog(parent), m_baseTitle(title), m_search(nullptr), m_chapters(nullptr), m_viewer(nullptr),
      m_topButton(nullptr), m_zoomOutButton(nullptr), m_zoomResetButton(nullptr),
      m_zoomInButton(nullptr), m_closeButton(nullptr), m_zoomLevel(0)
{
    setMinimumSize(980, 650);
    resize(1240, 790);
    setWindowFlags(windowFlags() | Qt::WindowMaximizeButtonHint | Qt::WindowMinimizeButtonHint);
    buildUi();
    retranslateUi();
}

void HelpViewer::buildUi()
{
    setStyleSheet(QStringLiteral(
        "QDialog{background:#080e13;color:#edf3f6;}"
        "QLabel{color:#edf3f6;background:transparent;}"
        "QLineEdit{background:#091117;color:#edf3f6;border:1px solid #34414b;border-radius:5px;padding:7px;selection-background-color:#8a4b00;}"
        "QLineEdit:focus{border-color:#ff8a00;}"
        "QListWidget{background:#091117;color:#d5e0e5;border:1px solid #263640;outline:0;padding:4px;}"
        "QListWidget::item{padding:8px 7px;border-radius:4px;}"
        "QListWidget::item:selected{background:#3b260f;color:#ffad4d;border-left:3px solid #ff8a00;}"
        "QListWidget::item:hover{background:#121d24;color:#ffffff;}"
        "QTextBrowser{background:#080e13;color:#edf3f6;border:1px solid #263640;padding:4px;selection-background-color:#8a4b00;}"
        "QSplitter::handle{background:#263640;width:2px;}"
        "QScrollBar:vertical{background:#111a21;width:15px;border-left:1px solid #46545e;}"
        "QScrollBar::handle:vertical{background:#768792;min-height:36px;border-radius:5px;margin:2px;}"
        "QScrollBar::handle:vertical:hover{background:#ff8a22;}"
        "QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical{height:0;}"
        "QScrollBar:horizontal{background:#111a21;height:15px;border-top:1px solid #46545e;}"
        "QScrollBar::handle:horizontal{background:#768792;min-width:36px;border-radius:5px;margin:2px;}"));

    QVBoxLayout *root = new QVBoxLayout(this);
    root->setContentsMargins(12, 10, 12, 10);
    root->setSpacing(8);

    QFrame *header = new QFrame(this);
    header->setStyleSheet(QStringLiteral("QFrame{background:#0d1820;border:1px solid #2f4652;border-left:4px solid #ff8a00;border-radius:5px;}"));
    QHBoxLayout *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(14, 8, 10, 8);
    QVBoxLayout *titles = new QVBoxLayout;
    QLabel *title = new QLabel(header);
    title->setObjectName(QStringLiteral("helpMainTitle"));
    title->setStyleSheet(QStringLiteral("color:#f6f9fa;font-size:17pt;font-weight:800;"));
    QLabel *subtitle = new QLabel(header);
    subtitle->setObjectName(QStringLiteral("helpSubtitle"));
    subtitle->setStyleSheet(QStringLiteral("color:#b9c7ce;font-size:9.5pt;"));
    titles->addWidget(title);
    titles->addWidget(subtitle);
    headerLayout->addLayout(titles, 1);

    m_topButton = new QPushButton(header);
    m_zoomOutButton = new QPushButton(header);
    m_zoomResetButton = new QPushButton(header);
    m_zoomInButton = new QPushButton(header);
    m_closeButton = new QPushButton(header);
    for (QPushButton *button : {m_topButton, m_zoomOutButton, m_zoomResetButton, m_zoomInButton, m_closeButton}) {
        button->setStyleSheet(darkButtonStyle());
        button->setMinimumHeight(31);
        headerLayout->addWidget(button);
    }
    root->addWidget(header);

    QSplitter *splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setChildrenCollapsible(false);

    QWidget *left = new QWidget(splitter);
    left->setMinimumWidth(250);
    left->setMaximumWidth(330);
    QVBoxLayout *leftLayout = new QVBoxLayout(left);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(7);
    QLabel *toc = new QLabel(left);
    toc->setObjectName(QStringLiteral("helpTocTitle"));
    toc->setStyleSheet(QStringLiteral("color:#ff8a00;font-weight:800;font-size:10pt;padding:3px;"));
    m_search = new QLineEdit(left);
    m_search->setClearButtonEnabled(true);
    m_chapters = new QListWidget(left);
    leftLayout->addWidget(toc);
    leftLayout->addWidget(m_search);
    leftLayout->addWidget(m_chapters, 1);

    m_viewer = new QTextBrowser(splitter);
    m_viewer->setOpenLinks(false);
    m_viewer->setOpenExternalLinks(false);
    m_viewer->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    m_viewer->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    splitter->addWidget(left);
    splitter->addWidget(m_viewer);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    root->addWidget(splitter, 1);

    connect(m_closeButton, &QPushButton::clicked, this, &HelpViewer::onCloseClicked);
    connect(m_topButton, &QPushButton::clicked, this, &HelpViewer::goTop);
    connect(m_zoomOutButton, &QPushButton::clicked, this, &HelpViewer::zoomOutHelp);
    connect(m_zoomResetButton, &QPushButton::clicked, this, &HelpViewer::resetZoom);
    connect(m_zoomInButton, &QPushButton::clicked, this, &HelpViewer::zoomInHelp);
    connect(m_chapters, &QListWidget::currentRowChanged, this, &HelpViewer::onChapterChanged);
    connect(m_search, &QLineEdit::textChanged, this, &HelpViewer::onSearchChanged);
    connect(m_search, &QLineEdit::returnPressed, this, &HelpViewer::onSearchActivated);
    connect(m_viewer, &QTextBrowser::anchorClicked, this, &HelpViewer::onAnchorClicked);
}

void HelpViewer::changeEvent(QEvent *event)
{
    if (event && event->type() == QEvent::LanguageChange)
        retranslateUi();
    QDialog::changeEvent(event);
}

void HelpViewer::retranslateUi()
{
    setWindowTitle(QStringLiteral("%1 %2%3").arg(m_baseTitle, QStringLiteral(APP_VERSION), I18n::text(6250)));
    if (QLabel *label = findChild<QLabel*>(QStringLiteral("helpMainTitle")))
        label->setText(I18n::text(6480));
    if (QLabel *label = findChild<QLabel*>(QStringLiteral("helpSubtitle")))
        label->setText(I18n::text(6481));
    if (QLabel *label = findChild<QLabel*>(QStringLiteral("helpTocTitle")))
        label->setText(I18n::text(6487));
    m_search->setPlaceholderText(I18n::text(6486));
    m_topButton->setText(I18n::text(6488));
    m_zoomOutButton->setText(I18n::text(6489));
    m_zoomResetButton->setText(I18n::text(6490));
    m_zoomInButton->setText(I18n::text(6491));
    m_closeButton->setText(I18n::text(6251));

    const int oldRow = qMax(0, m_chapters->currentRow());
    m_chapters->blockSignals(true);
    m_chapters->clear();
    for (int i = 0; i < kChapterCount; ++i)
        m_chapters->addItem(QStringLiteral("%1. %2").arg(i + 1).arg(I18n::text(6500 + i)));
    m_chapters->setCurrentRow(qMin(oldRow, kChapterCount - 1));
    m_chapters->blockSignals(false);
    rebuildDocument();
    onSearchChanged(m_search->text());
}

QStringList HelpViewer::chapterImages() const
{
    return {
        QStringLiteral(":/help/screens/overview.jpg"),
        QStringLiteral(":/help/screens/options.jpg"),
        QStringLiteral(":/help/screens/options.jpg"),
        QStringLiteral(":/help/screens/overview.jpg"),
        QStringLiteral(":/help/screens/overview.jpg"),
        QStringLiteral(":/help/screens/settings.jpg"),
        QString(),
        QStringLiteral(":/help/screens/errors.jpg"),
        QStringLiteral(":/help/screens/autodiag.jpg"),
        QStringLiteral(":/help/screens/analysis.jpg"),
        QStringLiteral(":/help/screens/measures.jpg"),
        QStringLiteral(":/help/screens/rosco.jpg"),
        QStringLiteral(":/help/screens/alldata.jpg"),
        QString(),
        QStringLiteral(":/help/screens/overview.jpg"),
        QStringLiteral(":/help/screens/database.jpg"),
        QStringLiteral(":/help/screens/database.jpg"),
        QStringLiteral(":/help/screens/database.jpg"),
        QStringLiteral(":/help/screens/database.jpg"),
        QStringLiteral(":/help/screens/options.jpg"),
        QStringLiteral(":/help/screens/database.jpg"),
        QStringLiteral(":/help/screens/rosco.jpg"),
        QStringLiteral(":/help/screens/options.jpg"),
        QString(),
        QStringLiteral(":/help/screens/overview.jpg")
    };
}

QString HelpViewer::buildDocumentHtml() const
{
    const QStringList images = chapterImages();
    QString html = QStringLiteral(
        "<!doctype html><html><head><meta charset='utf-8'><style>"
        "body{background:#080e13;color:#edf3f6;font-family:'Segoe UI',Arial,sans-serif;font-size:10pt;line-height:1.5;margin:18px 22px 34px;}"
        ".hero{background:#0d1820;border:1px solid #2f4652;border-left:4px solid #ff8a00;padding:16px 18px;margin-bottom:17px;}"
        ".eyebrow{color:#ff8a00;font-weight:800;font-size:9pt;}h1{font-size:21pt;color:#f6f9fa;margin:5px 0 4px;}"
        ".subtitle{color:#b9c7ce;font-size:10.5pt}.chapter{background:#101a21;border:1px solid #263640;margin:0 0 17px;padding:14px 16px;}"
        ".kicker{color:#ff8a00;font-size:8.5pt;font-weight:800;}h2{color:#f4f7f8;font-size:15pt;margin:4px 0 9px;border-bottom:1px solid #2b3a43;padding-bottom:7px;}"
        "p{color:#cbd6db;margin:7px 0}.screen{background:#050a0d;border:1px solid #344650;margin:13px 0 6px;text-align:center;padding:8px;}"
        ".screen img{max-width:100%;height:auto}.screenhint{color:#ff9a2f;font-size:9pt;margin-top:5px}.footer{border-top:1px solid #263640;color:#94a4ad;padding-top:12px;margin-top:22px;}"
        "a{color:#ff9a2f;text-decoration:none;}"
        "</style></head><body>");
    html += QStringLiteral("<div class='hero'><div class='eyebrow'>%1</div><h1>%2</h1><div class='subtitle'>%3</div></div>")
        .arg(I18n::text(6480).toHtmlEscaped(), m_baseTitle.toHtmlEscaped(), I18n::text(6481).toHtmlEscaped());

    for (int i = 0; i < kChapterCount; ++i) {
        html += QStringLiteral("<div class='chapter' id='chapter-%1'><div class='kicker'>%2 %1</div><h2>%1. %3</h2>%4")
            .arg(i + 1)
            .arg(I18n::text(6482).toHtmlEscaped())
            .arg(I18n::text(6500 + i).toHtmlEscaped())
            .arg(paragraphHtml(I18n::text(6600 + i)));
        if (!images.value(i).isEmpty()) {
            const QString qrc = QStringLiteral("qrc") + images.at(i);
            html += QStringLiteral("<div class='screen'><a href='helpimg:%1'><img src='%2' width='900'></a><div class='screenhint'>%3</div></div>")
                .arg(i)
                .arg(qrc.toHtmlEscaped())
                .arg(I18n::text(6483).toHtmlEscaped());
        }
        html += QStringLiteral("</div>");
    }
    html += QStringLiteral("<div class='footer'>%1 — %2 %3</div></body></html>")
        .arg(I18n::text(6485).toHtmlEscaped(), I18n::text(6492).toHtmlEscaped(), QStringLiteral(APP_VERSION).toHtmlEscaped());
    return html;
}

void HelpViewer::rebuildDocument()
{
    m_viewer->setHtml(buildDocumentHtml());
    setZoomLevel(m_zoomLevel);
    const int row = m_chapters->currentRow();
    if (row >= 0)
        m_viewer->scrollToAnchor(QStringLiteral("chapter-%1").arg(row + 1));
}

void HelpViewer::onChapterChanged(int row)
{
    if (row >= 0 && row < kChapterCount)
        m_viewer->scrollToAnchor(QStringLiteral("chapter-%1").arg(row + 1));
}

void HelpViewer::onSearchChanged(const QString &text)
{
    const QString query = text.trimmed();
    int firstVisible = -1;
    for (int i = 0; i < kChapterCount; ++i) {
        const QString haystack = I18n::text(6500 + i) + QLatin1Char(' ') + I18n::text(6600 + i);
        const bool visible = query.isEmpty() || haystack.contains(query, Qt::CaseInsensitive);
        m_chapters->item(i)->setHidden(!visible);
        if (visible && firstVisible < 0)
            firstVisible = i;
    }
    if (!query.isEmpty() && firstVisible >= 0)
        m_chapters->setCurrentRow(firstVisible);
}

void HelpViewer::onSearchActivated()
{
    for (int i = 0; i < m_chapters->count(); ++i) {
        if (!m_chapters->item(i)->isHidden()) {
            m_chapters->setCurrentRow(i);
            return;
        }
    }
}

void HelpViewer::onAnchorClicked(const QUrl &url)
{
    if (url.scheme() == QStringLiteral("helpimg")) {
        QString token = url.toString().section(QLatin1Char(':'), 1);
        token.remove(QLatin1Char('/'));
        bool ok = false;
        const int index = token.toInt(&ok);
        if (ok)
            openImage(index);
        return;
    }
    if (!url.fragment().isEmpty())
        m_viewer->scrollToAnchor(url.fragment());
}

void HelpViewer::openImage(int chapterIndex)
{
    const QString resource = chapterImages().value(chapterIndex);
    if (resource.isEmpty())
        return;
    QPixmap pixmap(resource);
    if (pixmap.isNull())
        return;

    QDialog dialog(this);
    dialog.setWindowTitle(I18n::text(6484));
    dialog.resize(1180, 760);
    dialog.setStyleSheet(QStringLiteral("QDialog{background:#080e13;}QScrollArea{background:#080e13;border:1px solid #34414b;}QLabel{background:#050a0d;}"));
    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    QScrollArea *scroll = new QScrollArea(&dialog);
    scroll->setWidgetResizable(false);
    scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    QLabel *image = new QLabel(scroll);
    image->setPixmap(pixmap);
    image->resize(pixmap.size());
    scroll->setWidget(image);
    layout->addWidget(scroll);
    dialog.exec();
}

void HelpViewer::goTop()
{
    m_viewer->verticalScrollBar()->setValue(m_viewer->verticalScrollBar()->minimum());
}

void HelpViewer::setZoomLevel(int level)
{
    if (m_zoomLevel > 0)
        m_viewer->zoomOut(m_zoomLevel);
    else if (m_zoomLevel < 0)
        m_viewer->zoomIn(-m_zoomLevel);
    m_zoomLevel = 0;
    if (level > 0)
        m_viewer->zoomIn(level);
    else if (level < 0)
        m_viewer->zoomOut(-level);
    m_zoomLevel = level;
}

void HelpViewer::zoomOutHelp()
{
    setZoomLevel(qMax(-4, m_zoomLevel - 1));
}

void HelpViewer::resetZoom()
{
    setZoomLevel(0);
}

void HelpViewer::zoomInHelp()
{
    setZoomLevel(qMin(5, m_zoomLevel + 1));
}

void HelpViewer::onCloseClicked()
{
    close();
}
