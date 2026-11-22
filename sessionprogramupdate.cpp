#include "sessionprogramupdate.h"
#include "database/MemsDocumentaryWindowsTransport.h"
#include "i18n.h"
#include <QAction>
#include <QApplication>
#include <QDesktopServices>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMainWindow>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPen>
#include <QPixmap>
#include <QPushButton>
#include <QRegularExpression>
#include <QThread>
#include <QTimer>
#include <QVersionNumber>
#include <memory>
namespace {
const QString kManifest=QStringLiteral(
    "https://raw.githubusercontent.com/mini56/"
    "ECU-Mems-Manager-Session-MAJ/main/manifest.json");
bool officialSetup(const QUrl &url){
    return url.isValid() && url.scheme()==QStringLiteral("https") &&
        url.host()==QStringLiteral("github.com") && url.port(443)==443 &&
        url.userInfo().isEmpty() && url.fragment().isEmpty() &&
        !url.hasQuery() && url.path().startsWith(QStringLiteral(
           "/mini56/ECU-Mems-Manager-Session-MAJ/releases/download/")) &&
        url.path().endsWith(QStringLiteral("-Setup.exe"));
}

// Same thin orange outline as SESSION's Sommaire / About menu icons.
// Presentation only; the release manifest check remains unchanged.
QIcon programUpdateMenuIcon()
{
    QPixmap image(24,24);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing,true);
    painter.setPen(QPen(QColor(QStringLiteral("#ff8a1c")),1.8,
                    Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));
    painter.setBrush(Qt::NoBrush);
    painter.drawArc(QRectF(5.0,5.0,14.0,14.0),45*16,270*16);
    // Arrowhead at the end of the circular refresh arrow.
    painter.drawLine(QPointF(16.95,16.95),QPointF(16.95,12.9));
    painter.drawLine(QPointF(16.95,16.95),QPointF(12.9,16.95));
    painter.end();
    return QIcon(image);
}
}
SessionProgramUpdate::SessionProgramUpdate(QMainWindow *window)
    :QObject(window),m_window(window)
{
    if(!window)return;
    m_indicator=window->findChild<QPushButton*>(
        QStringLiteral("uiProgramUpdateButton"));
    if(m_indicator){
        m_indicator->hide();
        connect(m_indicator,&QPushButton::clicked,this,[this](){
            if(officialSetup(m_downloadUrl))
                QDesktopServices::openUrl(m_downloadUrl);
        });
    }
    if(QMenu *help=window->findChild<QMenu*>(
           QStringLiteral("m_helpMenu"))){
        QAction *manual=new QAction(I18n::text(17902),this);
        manual->setObjectName(QStringLiteral("programUpdateCheckAction"));
        manual->setIcon(programUpdateMenuIcon());
        if(QAction *about=window->findChild<QAction*>(
           QStringLiteral("m_helpAboutAction")))
            help->insertAction(about,manual);
        else help->addAction(manual);
        connect(manual,&QAction::triggered,this,[this](){check(true);});
    }
    // No external network access during installed offscreen UI smoke tests.
    if(!qApp->arguments().contains(QStringLiteral("--session-ui-smoke-test")))
        QTimer::singleShot(1400,this,[this](){check(false);});
}
void SessionProgramUpdate::tellUser(const QString &message) const
{
    if(m_window) QMessageBox::information(
        m_window,I18n::text(17903),message);
}
void SessionProgramUpdate::check(bool interactive)
{
    if(m_checking){
        if(interactive)tellUser(I18n::text(17904));
        return;
    }
    m_checking=true;
    // Reuse native WinHTTP TLS (Windows trust store, no OpenSSL dependency).
    // Blocking network work runs away from both GUI and ECU polling threads.
    const auto result=std::make_shared<MemsDocumentaryHttpResult>();
    QThread *worker=QThread::create([result](){
        *result=MemsDocumentaryWindowsTransport::get(kManifest,256*1024);
    });
    connect(worker,&QThread::finished,this,[this,result,interactive](){
        m_checking=false;
        if(!result->ok()){
            if(interactive)tellUser(I18n::text(17907));
            return;
        }
        handleManifest(result->body,interactive);
    });
    connect(worker,&QThread::finished,worker,&QObject::deleteLater);
    worker->start();
}
void SessionProgramUpdate::handleManifest(
    const QByteArray &data,bool interactive)
{
    QJsonParseError error;
    const QJsonDocument doc=QJsonDocument::fromJson(data,&error);
    if(error.error!=QJsonParseError::NoError || !doc.isObject()){
        if(interactive)tellUser(I18n::text(17907));
        return;
    }
    const QJsonObject info=doc.object().value(
        QStringLiteral("program")).toObject();
    const QString version=info.value(QStringLiteral("version")).toString();
    const QString checksum=info.value(QStringLiteral("sha256")).toString();
    const QUrl url(info.value(QStringLiteral("release_url")).toString());
    const QRegularExpression sha256(QStringLiteral("^[0-9a-fA-F]{64}$"));
    const QVersionNumber available=QVersionNumber::fromString(version);
    const QVersionNumber current=QVersionNumber::fromString(
        QStringLiteral(APP_VERSION));
    if(!info.value(QStringLiteral("available")).toBool() ||
       available.isNull() || current.isNull() ||
       !sha256.match(checksum).hasMatch() || !officialSetup(url)){
        if(interactive)tellUser(I18n::text(17907));
        return;
    }
    if(QVersionNumber::compare(available,current)>0){
        m_downloadUrl=url;
        if(m_indicator){
            m_indicator->setText(I18n::text(17900).arg(version));
            m_indicator->setToolTip(I18n::text(17901));
            // Preserve the entire semantic version at any supported DPI.
            // Width comes from the current translated text, not a guessed
            // fixed pixel value (and the bubble icon is disabled on this button).
            m_indicator->setMinimumWidth(
                m_indicator->fontMetrics().horizontalAdvance(m_indicator->text())+30);
            m_indicator->show();
        }
        if(interactive)tellUser(I18n::text(17905).arg(version));
    }else{
        m_downloadUrl=QUrl();
        if(m_indicator)m_indicator->hide();
        if(interactive)tellUser(I18n::text(17906).arg(
            QStringLiteral(APP_VERSION)));
    }
}
