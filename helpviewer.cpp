#include "helpviewer.h"

#include <QDesktopServices>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QSplitter>
#include <QTextBrowser>
#include <QTextCursor>
#include <QTextDocument>
#include <QVBoxLayout>
#include <QWidget>

#include "i18n.h"

namespace {

QString escapedBody(const QString &text)
{
    QString out = text.toHtmlEscaped();
    out.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    out.replace(QLatin1Char('\r'), QLatin1Char('\n'));
    out.replace(QStringLiteral("\n"), QStringLiteral("<br/>"));
    return out;
}

}

HelpViewer::HelpViewer(const QString title, QWidget *parent)
    : QDialog(parent),
      m_searchEdit(nullptr),
      m_chapterList(nullptr),
      m_viewer(nullptr),
      m_closeButton(nullptr),
      m_topButton(nullptr),
      m_zoomOutButton(nullptr),
      m_zoomResetButton(nullptr),
      m_zoomInButton(nullptr),
      m_zoomSteps(0)
{
    setWindowTitle(title + I18n::text(6250));
    setMinimumSize(900, 600);
    resize(1180, 760);
    setWindowFlags(windowFlags() | Qt::WindowMaximizeButtonHint | Qt::WindowMinimizeButtonHint);
    setStyleSheet(QStringLiteral(
        "QDialog{background:#10151d;color:#e8edf5;}"
        "QLabel{color:#e8edf5;}"
        "QPushButton{background:#1b2532;color:#e8edf5;border:1px solid #34465b;"
        "border-radius:5px;padding:7px 12px;min-height:20px;}"
        "QPushButton:hover{background:#26364a;border-color:#4d75a3;}"
        "QPushButton:pressed{background:#15202c;}"
        "QLineEdit{background:#0c1118;color:#edf3fb;border:1px solid #34465b;"
        "border-radius:5px;padding:8px;selection-background-color:#2e73d2;}"
        "QListWidget{background:#0c1118;color:#cfd9e6;border:1px solid #28384a;"
        "border-radius:6px;outline:0;padding:4px;}"
        "QListWidget::item{padding:8px 7px;border-radius:4px;}"
        "QListWidget::item:selected{background:#1f66b5;color:white;}"
        "QListWidget::item:hover{background:#172536;}"
        "QTextBrowser{background:#111821;color:#e7edf6;border:1px solid #28384a;"
        "border-radius:6px;padding:8px;selection-background-color:#2e73d2;}"
        "QSplitter::handle{background:#1b2735;width:2px;}"));

    QVBoxLayout *root = new QVBoxLayout(this);
    root->setContentsMargins(14, 14, 14, 14);
    root->setSpacing(10);

    QWidget *header = new QWidget(this);
    QHBoxLayout *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(8);

    QLabel *titleLabel = new QLabel(I18n::text(6252), header);
    titleLabel->setStyleSheet(QStringLiteral(
        "font-size:20px;font-weight:700;color:#f3f7fc;padding-right:12px;"));
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch(1);

    m_topButton = new QPushButton(I18n::text(6255), header);
    m_zoomOutButton = new QPushButton(I18n::text(6256), header);
    m_zoomResetButton = new QPushButton(I18n::text(6257), header);
    m_zoomInButton = new QPushButton(I18n::text(6258), header);
    m_closeButton = new QPushButton(I18n::text(6251), header);

    m_zoomOutButton->setToolTip(I18n::text(6256));
    m_zoomResetButton->setToolTip(I18n::text(6257));
    m_zoomInButton->setToolTip(I18n::text(6258));

    headerLayout->addWidget(m_topButton);
    headerLayout->addWidget(m_zoomOutButton);
    headerLayout->addWidget(m_zoomResetButton);
    headerLayout->addWidget(m_zoomInButton);
    headerLayout->addWidget(m_closeButton);
    root->addWidget(header);

    QSplitter *splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setChildrenCollapsible(false);

    QWidget *sidebar = new QWidget(splitter);
    sidebar->setMinimumWidth(235);
    QVBoxLayout *sidebarLayout = new QVBoxLayout(sidebar);
    sidebarLayout->setContentsMargins(0, 0, 8, 0);
    sidebarLayout->setSpacing(8);

    QLabel *contentsLabel = new QLabel(I18n::text(6254), sidebar);
    contentsLabel->setStyleSheet(QStringLiteral(
        "font-size:13px;font-weight:700;color:#9cc8ff;padding:2px 2px 0 2px;"));
    sidebarLayout->addWidget(contentsLabel);

    m_searchEdit = new QLineEdit(sidebar);
    m_searchEdit->setClearButtonEnabled(true);
    m_searchEdit->setPlaceholderText(I18n::text(6253));
    sidebarLayout->addWidget(m_searchEdit);

    m_chapterList = new QListWidget(sidebar);
    m_chapterList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_anchors.reserve(18);
    for (int i = 0; i < 18; ++i) {
        const QString anchor = QStringLiteral("chapter-%1").arg(i + 1);
        m_anchors.append(anchor);
        m_chapterList->addItem(QStringLiteral("%1. %2").arg(i + 1).arg(I18n::text(6300 + i)));
    }
    sidebarLayout->addWidget(m_chapterList, 1);

    m_viewer = new QTextBrowser(splitter);
    m_viewer->setOpenLinks(false);
    m_viewer->setOpenExternalLinks(false);
    m_viewer->setHtml(buildHelpHtml());

    splitter->addWidget(sidebar);
    splitter->addWidget(m_viewer);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes(QList<int>() << 285 << 895);
    root->addWidget(splitter, 1);

    connect(m_closeButton, SIGNAL(clicked()), this, SLOT(onCloseClicked()));
    connect(m_topButton, SIGNAL(clicked()), this, SLOT(onTopClicked()));
    connect(m_zoomOutButton, SIGNAL(clicked()), this, SLOT(onZoomOut()));
    connect(m_zoomResetButton, SIGNAL(clicked()), this, SLOT(onZoomReset()));
    connect(m_zoomInButton, SIGNAL(clicked()), this, SLOT(onZoomIn()));
    connect(m_chapterList, SIGNAL(currentRowChanged(int)), this, SLOT(onChapterChanged(int)));
    connect(m_searchEdit, SIGNAL(textChanged(QString)), this, SLOT(onSearchTextChanged(QString)));
    connect(m_viewer, SIGNAL(anchorClicked(QUrl)), this, SLOT(onAnchorClicked(QUrl)));

    m_chapterList->setCurrentRow(0);
    resetViewerToTop();
}

QString HelpViewer::buildHelpHtml() const
{
    QString html;
    html.reserve(18000);
    html += QStringLiteral(
        "<!DOCTYPE html><html><head><meta charset='utf-8'/>"
        "<style>"
        "body{font-family:'Segoe UI',Arial,sans-serif;background:#111821;color:#dce5ef;"
        "font-size:10.5pt;line-height:1.52;margin:12px 18px 30px 18px;}"
        ".hero{background:#142234;border:1px solid #2c4662;border-radius:9px;padding:18px 20px;margin:0 0 14px 0;}"
        "h1{color:#f4f8fd;font-size:20pt;margin:0 0 5px 0;}"
        ".subtitle{color:#a9bdd3;margin:0;}"
        ".warning{background:#2b2114;border:1px solid #765527;border-radius:8px;padding:12px 14px;"
        "margin:14px 0 20px 0;color:#f2d49d;}"
        ".warning b{color:#ffd88a;}"
        ".chapter{background:#151e29;border:1px solid #25374a;border-radius:8px;padding:2px 16px 12px 16px;"
        "margin:0 0 14px 0;}"
        "h2{color:#80b8f4;font-size:14pt;border-bottom:1px solid #29425d;padding:11px 0 7px 0;margin:0 0 10px 0;}"
        "p{margin:7px 0;color:#dce5ef;}"
        ".footer{color:#8195aa;border-top:1px solid #25374a;padding-top:12px;margin-top:18px;font-size:9pt;}"
        "code{color:#b8dcff;background:#0c1219;border:1px solid #26384b;border-radius:3px;padding:1px 4px;}"
        "</style></head><body>");

    html += QStringLiteral("<div class='hero'><h1>%1</h1><p class='subtitle'>%2</p></div>")
                .arg(I18n::text(6259).toHtmlEscaped())
                .arg(escapedBody(I18n::text(6260)));

    html += QStringLiteral("<div class='warning'><b>%1 :</b> %2</div>")
                .arg(I18n::text(6261).toHtmlEscaped())
                .arg(escapedBody(I18n::text(6262)));

    for (int i = 0; i < 18; ++i) {
        html += QStringLiteral(
                    "<div class='chapter' id='chapter-%1'><h2>%1. %2</h2><p>%3</p></div>")
                    .arg(i + 1)
                    .arg(I18n::text(6300 + i).toHtmlEscaped())
                    .arg(escapedBody(I18n::text(6400 + i)));
    }

    html += QStringLiteral("<div class='footer'>%1</div></body></html>")
                .arg(escapedBody(I18n::text(6263)));
    return html;
}

void HelpViewer::resetViewerToTop()
{
    QTextCursor cursor = m_viewer->textCursor();
    cursor.movePosition(QTextCursor::Start);
    m_viewer->setTextCursor(cursor);
    m_viewer->ensureCursorVisible();
}

void HelpViewer::onCloseClicked()
{
    hide();
}

void HelpViewer::onAnchorClicked(const QUrl &url)
{
    if (url.scheme().isEmpty()) {
        const QString anchor = url.fragment().isEmpty() ? url.toString() : url.fragment();
        if (!anchor.isEmpty())
            m_viewer->scrollToAnchor(anchor);
        return;
    }
    QDesktopServices::openUrl(url);
}

void HelpViewer::onChapterChanged(int row)
{
    if (row >= 0 && row < m_anchors.size())
        m_viewer->scrollToAnchor(m_anchors.at(row));
}

void HelpViewer::onSearchTextChanged(const QString &text)
{
    if (text.trimmed().isEmpty()) {
        QTextCursor cursor = m_viewer->textCursor();
        cursor.clearSelection();
        m_viewer->setTextCursor(cursor);
        return;
    }

    QTextCursor cursor = m_viewer->textCursor();
    cursor.movePosition(QTextCursor::Start);
    m_viewer->setTextCursor(cursor);
    m_viewer->find(text);
}

void HelpViewer::onZoomIn()
{
    if (m_zoomSteps >= 8)
        return;
    m_viewer->zoomIn(1);
    ++m_zoomSteps;
}

void HelpViewer::onZoomOut()
{
    if (m_zoomSteps <= -5)
        return;
    m_viewer->zoomOut(1);
    --m_zoomSteps;
}

void HelpViewer::onZoomReset()
{
    if (m_zoomSteps > 0)
        m_viewer->zoomOut(m_zoomSteps);
    else if (m_zoomSteps < 0)
        m_viewer->zoomIn(-m_zoomSteps);
    m_zoomSteps = 0;
    m_zoomResetButton->setText(I18n::text(6257));
}

void HelpViewer::onTopClicked()
{
    m_chapterList->setCurrentRow(-1);
    resetViewerToTop();
}
