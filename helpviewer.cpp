#include "helpviewer.h"
#include <QFile>
#include <QSettings>
#include <QTextDocument>
#include <QDesktopServices>
#include "i18n.h"
#define tr I18n::text

HelpViewer::HelpViewer(const QString title, QWidget * parent):QDialog(parent), m_vbox(0), m_viewer(0), m_closeButton(0)
{
  this->setWindowTitle(title + I18n::text(6250) /* EN:  - Help */);
  this->setMinimumWidth(850);
  this->setMinimumHeight(550);
  this->setWindowFlags(this->windowFlags() | Qt::WindowMaximizeButtonHint | Qt::WindowMinimizeButtonHint);

  m_vbox = new QVBoxLayout(this);
  m_closeButton = new QPushButton(I18n::text(6251) /* EN: Close */, this);
  connect(m_closeButton, SIGNAL(clicked()), this, SLOT(onCloseClicked()));
  m_viewer = new QTextBrowser(this);
  m_viewer->setOpenLinks(false);
  connect(m_viewer, SIGNAL(anchorClicked(QUrl)), this, SLOT(onAnchorClicked(QUrl)));

  QSettings settings(QSettings::IniFormat, QSettings::UserScope, PROJECTNAME);
  settings.beginGroup("Settings");
  const QString language = settings.value("Language", "fr").toString();
  settings.endGroup();
  QFile helpFile(language == "fr" ? ":/help/help.html" : ":/help/help_en.html");

  helpFile.open(QFile::ReadOnly);
  QString fileText = helpFile.readAll();

  helpFile.close();

  m_viewer->setHtml(fileText);

  m_vbox->addWidget(m_viewer);
  m_vbox->addWidget(m_closeButton);
}

void HelpViewer::onCloseClicked()
{
  this->hide();
}

void HelpViewer::onAnchorClicked(QUrl url)
{
  QDesktopServices::openUrl(url);
}
