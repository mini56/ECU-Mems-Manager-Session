#include "MemsDocumentaryUpdateDialog.h"
#include "../i18n.h"
#include "../nativewindowstyle.h"
#include <QApplication>
#include "MemsDocumentaryLocalInstaller.h"
#ifdef Q_OS_WIN
#include "MemsDocumentaryWindowsTransport.h"
#include <QSharedPointer>
#include <QThread>
#endif

#include <QDir>
#include <QFileDialog>
#include <QInputDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProgressBar>
#include <QPushButton>
#include <QSaveFile>
#include <QSettings>
#include <QStandardPaths>
#include <QUrl>
#include <QVBoxLayout>

namespace {
constexpr qint64 kMaxManifestBytes = 256 * 1024;
constexpr qint64 kMaxEncryptedPackBytes = 256LL * 1024 * 1024;

QNetworkRequest secureRequest(const QUrl &url)
{
    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setRawHeader("User-Agent", "ECU-MEMS-Manager-SESSION-documentary-updater");
    return request;
}

QString languageName(const QString &language)
{
    if (language == QStringLiteral("fr")) return QStringLiteral("Français");
    if (language == QStringLiteral("pt")) return QStringLiteral("Português");
    if (language == QStringLiteral("es")) return QStringLiteral("Español");
    if (language == QStringLiteral("it")) return QStringLiteral("Italiano");
    if (language == QStringLiteral("de")) return QStringLiteral("Deutsch");
    return QStringLiteral("English");
}
}

MemsDocumentaryUpdateDialog::MemsDocumentaryUpdateDialog(
    const QString &language, QWidget *parent)
    : QDialog(parent),
      m_language(language.left(2).toLower())
{
    if (m_language != QStringLiteral("fr") &&
        m_language != QStringLiteral("en") &&
        m_language != QStringLiteral("pt") &&
        m_language != QStringLiteral("es") &&
        m_language != QStringLiteral("it") &&
        m_language != QStringLiteral("de"))
        m_language = QStringLiteral("fr");

    setWindowTitle(ui(I18n::text(16300),
                      QStringLiteral("Documentary database updates")));
    setObjectName(QStringLiteral("documentaryUpdateDialog"));
    setMinimumWidth(430);
    setModal(true);
    const bool dark=qApp&&qApp->property("ecuDarkTheme").toBool();
    setStyleSheet(dark?QStringLiteral(
        "QDialog{background:#090e13;color:#e7ecef;}"
        "QLabel{color:#e7ecef;background:transparent;}"
        "QPushButton{background:#18232b;color:#edf2f5;border:1px solid #3b4b58;border-radius:5px;padding:6px;}"
        "QPushButton:hover,QPushButton:focus{border-color:#ff8a00;}"
        "QPushButton:disabled{background:#171e24;color:#69747d;border-color:#29323a;}"):
        QStringLiteral(
        "QDialog{background:#ffffff;color:#202936;}"
        "QLabel{color:#202936;background:transparent;}"
        "QPushButton{background:#f0f4f8;color:#202936;border:1px solid #bfcbd7;border-radius:5px;padding:6px;}"
        "QPushButton:hover,QPushButton:focus{border-color:#f07800;}"
        "QPushButton:disabled{background:#e9edf1;color:#8b939c;border-color:#d5dde5;}"));
    applyNativeTitleBar(this,dark);
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(18,18,18,16);
    layout->setSpacing(14);
    QLabel *heading = new QLabel(
        ui(I18n::text(16301),
           QStringLiteral("Documentary database: %1")).arg(
               languageName(m_language)), this);
    heading->setWordWrap(true);
    heading->setStyleSheet(QStringLiteral(
        "font-size:14px;font-weight:700;color:#ff9828;"));
    layout->addWidget(heading);

    m_status = new QLabel(this);
    m_status->setObjectName(QStringLiteral("documentaryUpdateStatus"));
    m_status->setWordWrap(true);
    m_status->setMinimumHeight(64);
    layout->addWidget(m_status);

    m_progress = new QProgressBar(this);
    m_progress->setObjectName(QStringLiteral("documentaryDownloadProgress"));
    m_progress->setRange(0,100);
    m_progress->setValue(0);
    m_progress->setVisible(false);
    layout->addWidget(m_progress);

    QHBoxLayout *actions = new QHBoxLayout;
    m_check = new QPushButton(
        ui(I18n::text(16302),
           QStringLiteral("Check for updates")), this);
    m_check->setObjectName(QStringLiteral("documentaryCheckUpdate"));
    m_download = new QPushButton(
        ui(I18n::text(16303),
           QStringLiteral("Download database")), this);
    m_download->setObjectName(QStringLiteral("documentaryDownloadUpdate"));
    m_download->setEnabled(false);
    m_installLocal = new QPushButton(
        ui(I18n::text(16304),
           QStringLiteral("Install local encrypted pack")),this);
    m_installLocal->setObjectName(QStringLiteral("documentaryInstallLocalEncrypted"));
    QPushButton *close = new QPushButton(
        ui(I18n::text(16305), QStringLiteral("Close")), this);
    actions->addWidget(m_check);
    actions->addWidget(m_download);
    actions->addWidget(m_installLocal);
    actions->addWidget(close);
    layout->addLayout(actions);

    connect(m_check,&QPushButton::clicked,this,[this](){checkManifest();});
    connect(m_download,&QPushButton::clicked,this,[this](){startDownload();});
    connect(m_installLocal,&QPushButton::clicked,this,[this](){installLocalEncrypted();});
    connect(close,&QPushButton::clicked,this,&QDialog::reject);
    checkManifest();
}

MemsDocumentaryUpdateDialog::~MemsDocumentaryUpdateDialog()
{
    if (m_reply)
        m_reply->abort();
    if (m_target)
        m_target->cancelWriting();
}

QString MemsDocumentaryUpdateDialog::ui(const QString &fr, const QString &en) const
{
    return m_language == QStringLiteral("en") ? en : fr;
}

void MemsDocumentaryUpdateDialog::setStatus(const QString &message)
{
    if (m_status)
        m_status->setText(message);
}

void MemsDocumentaryUpdateDialog::setBusy(bool busy)
{
    m_check->setEnabled(!busy);
    m_download->setEnabled(!busy && m_offer.available);
    if(m_installLocal) m_installLocal->setEnabled(!busy);
}

void MemsDocumentaryUpdateDialog::checkManifest()
{
#ifdef Q_OS_WIN
    checkManifestWindows();
    return;
#endif
    if (m_reply)
        return;
    m_offer = MemsDocumentaryOffer{};
    m_manifestBytes.clear();
    setBusy(true);
    setStatus(ui(I18n::text(16306),
                 QStringLiteral("Checking GitHub for updates…")));

    QNetworkReply *reply = m_network.get(secureRequest(
        QUrl(MemsDocumentaryPackCatalog::manifestUrl())));
    m_reply = reply;
    connect(reply,&QNetworkReply::readyRead,this,[this,reply](){
        const QByteArray data = reply->readAll();
        if (m_manifestBytes.size() + data.size() > kMaxManifestBytes) {
            reply->abort();
            return;
        }
        m_manifestBytes.append(data);
    });
    connect(reply,&QNetworkReply::finished,this,[this,reply](){
        if (m_reply != reply)
            return;
        const QByteArray tail = reply->readAll();
        if (m_manifestBytes.size() + tail.size() <= kMaxManifestBytes)
            m_manifestBytes.append(tail);
        m_reply = nullptr;
        const int httpStatus=reply->attribute(
            QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QString networkReason=reply->errorString();
        const bool success = reply->error() == QNetworkReply::NoError &&
            httpStatus == 200 &&
            reply->url().scheme() == QStringLiteral("https") &&
            !m_manifestBytes.isEmpty() &&
            m_manifestBytes.size() <= kMaxManifestBytes;
        reply->deleteLater();
        if (!success) {
            // Une erreur SSL, réseau ou HTTP doit être visible et diagnosticable,
            // sans prétendre qu'aucune base n'est publiée sur GitHub.
            const QString reason=QStringLiteral("HTTP %1 / %2")
                .arg(httpStatus).arg(networkReason.left(180));
            setStatus(ui(I18n::text(16307)
                    .arg(reason),
                QStringLiteral(
                "Unable to retrieve the update catalogue over HTTPS. "
                "Details: %1. Installed databases are unchanged.")
                    .arg(reason)));
        } else {
            showManifestResult(m_manifestBytes);
        }
        setBusy(false);
    });
}

void MemsDocumentaryUpdateDialog::showManifestResult(const QByteArray &data)
{
            m_offer = MemsDocumentaryPackCatalog::parseManifest(
                data, m_language);
            if (!m_offer.error.isEmpty()) {
                setStatus(ui(I18n::text(16308),
                    QStringLiteral(
                    "Invalid update manifest. Download refused.")));
            } else if (!m_offer.available) {
                setStatus(ui(I18n::text(16309).arg(
                        languageName(m_language)),
                    QStringLiteral(
                    "No %1 documentary database is published yet. "
                    "ECU diagnostic and expert databases remain unchanged.").arg(
                        languageName(m_language))));
            } else {
                QSettings versionSettings(QSettings::IniFormat,
                    QSettings::UserScope,QStringLiteral("ECU_Mems_Manager"));
                const QString savedVersion=versionSettings.value(
                    QStringLiteral("DocumentaryVersions/%1").arg(m_language))
                    .toString();
                if(savedVersion==m_offer.version &&
                   QFileInfo::exists(MemsDocumentaryPackCatalog::installedDatabasePath(
                        m_language))){
                    m_offer.available=false;
                    setStatus(ui(I18n::text(16310)
                            .arg(languageName(m_language),savedVersion),
                        QStringLiteral(
                        "Documentary database %1 is up to date (version %2).")
                            .arg(languageName(m_language),savedVersion)));
                }else setStatus(ui(I18n::text(16311)
                        .arg(m_offer.version),
                    QStringLiteral(
                    "Encrypted pack version %1 available. "
                    "Download verifies SHA-256 and installs after "
                    "password and SQLite validation.").arg(m_offer.version)));
            }

}


#ifdef Q_OS_WIN
void MemsDocumentaryUpdateDialog::checkManifestWindows()
{
    if (m_windowsTransportBusy)
        return;
    m_offer=MemsDocumentaryOffer{};
    m_windowsTransportBusy=true;
    setBusy(true);
    setStatus(ui(I18n::text(16312),
                 QStringLiteral("Connecting to GitHub using Windows HTTPS…")));

    const QString url=MemsDocumentaryPackCatalog::manifestUrl();
    auto result=QSharedPointer<MemsDocumentaryHttpResult>::create();
    QThread *worker=QThread::create([result,url](){
        *result=MemsDocumentaryWindowsTransport::get(url,kMaxManifestBytes);
    });
    connect(worker,&QThread::finished,this,[this,result](){
        m_windowsTransportBusy=false;
        if(result->ok()){
            showManifestResult(result->body);
        }else{
            setStatus(ui(
                I18n::text(16313).arg(result->error),
                QStringLiteral("Windows HTTPS failed: %1. "
                               "Existing databases remain unchanged.").arg(result->error)));
        }
        setBusy(false);
    });
    connect(worker,&QThread::finished,worker,&QObject::deleteLater);
    worker->start();
}

void MemsDocumentaryUpdateDialog::downloadPackWindows()
{
    if(m_windowsTransportBusy || !m_offer.available)
        return;
    const QString root=QStandardPaths::writableLocation(
        QStandardPaths::AppLocalDataLocation);
    if(root.isEmpty()){
        setStatus(ui(I18n::text(16314),
                     QStringLiteral("Local storage unavailable.")));
        return;
    }
    const QString directory=QDir(root).filePath(
        QStringLiteral("documentary/downloads/%1").arg(m_language));
    if(!QDir().mkpath(directory)){
        setStatus(ui(I18n::text(16315),
                     QStringLiteral("Unable to create the package directory.")));
        return;
    }
    m_savedFilePath=QDir(directory).filePath(
        QStringLiteral("mems_documentary_%1_%2.encrypted")
            .arg(m_language,m_offer.version));
    const QString destination=m_savedFilePath;
    const QString url=m_offer.releaseUrl;
    const QByteArray expectedHash=m_offer.sha256.toLatin1();

    m_windowsTransportBusy=true;
    m_progress->setRange(0,0);
    m_progress->setVisible(true);
    setBusy(true);
    setStatus(ui(I18n::text(16316),
        QStringLiteral(
        "Downloading with native Windows HTTPS and verifying SHA-256…")));

    auto result=QSharedPointer<MemsDocumentaryHttpResult>::create();
    QThread *worker=QThread::create([result,url,destination,expectedHash](){
        *result=MemsDocumentaryWindowsTransport::get(
            url,kMaxEncryptedPackBytes,destination,expectedHash);
    });
    connect(worker,&QThread::finished,this,[this,result](){
        m_windowsTransportBusy=false;
        m_progress->setRange(0,100);
        m_progress->setVisible(false);
        if(result->ok()){
            installVerifiedDownloadedPackage();
        }else{
            setStatus(ui(
                I18n::text(16317).arg(result->error),
                QStringLiteral("Download rejected: %1. "
                               "Installed database unchanged.").arg(result->error)));
        }
        setBusy(false);
    });
    connect(worker,&QThread::finished,worker,&QObject::deleteLater);
    worker->start();
}
#endif

void MemsDocumentaryUpdateDialog::startDownload()
{
#ifdef Q_OS_WIN
    downloadPackWindows();
    return;
#endif
    if (m_reply || !m_offer.available)
        return;
    const QString root = QStandardPaths::writableLocation(
        QStandardPaths::AppLocalDataLocation);
    if (root.isEmpty()) {
        setStatus(ui(I18n::text(16318),
                     QStringLiteral("Local download directory unavailable.")));
        return;
    }
    const QString dir = QDir(root).filePath(
        QStringLiteral("documentary/downloads/%1").arg(m_language));
    if (!QDir().mkpath(dir)) {
        setStatus(ui(I18n::text(16319),
                     QStringLiteral("Cannot create the download directory.")));
        return;
    }
    // Version and language were validated before any filesystem use.
    m_savedFilePath = QDir(dir).filePath(
        QStringLiteral("mems_documentary_%1_%2.encrypted")
            .arg(m_language, m_offer.version));
    m_target.reset(new QSaveFile(m_savedFilePath));
    if (!m_target->open(QIODevice::WriteOnly)) {
        m_target.reset();
        setStatus(ui(I18n::text(16320),
                     QStringLiteral("Cannot write the pack on this computer.")));
        return;
    }
    m_sha.reset();
    m_progress->setRange(0,100);
    m_progress->setValue(0);
    m_progress->setVisible(true);
    setBusy(true);
    setStatus(ui(I18n::text(16321),
                 QStringLiteral("Downloading and verifying SHA-256…")));

    QNetworkReply *reply = m_network.get(secureRequest(QUrl(m_offer.releaseUrl)));
    m_reply = reply;
    connect(reply,&QNetworkReply::readyRead,this,[this,reply](){
        const QByteArray data = reply->readAll();
        if (!m_target || m_target->size() + data.size() > kMaxEncryptedPackBytes) {
            reply->abort();
            return;
        }
        if (m_target->write(data) != data.size()) {
            reply->abort();
            return;
        }
        m_sha.addData(data);
    });
    connect(reply,&QNetworkReply::downloadProgress,this,[this](qint64 received,qint64 total){
        if (received > kMaxEncryptedPackBytes || total > kMaxEncryptedPackBytes) {
            if (m_reply)
                m_reply->abort();
            return;
        }
        if (total > 0)
            m_progress->setValue(static_cast<int>(qMin<qint64>(100,received*100/total)));
    });
    connect(reply,&QNetworkReply::finished,this,[this,reply](){
        finishDownload(reply);
    });
}

void MemsDocumentaryUpdateDialog::finishDownload(QNetworkReply *reply)
{
    if (m_reply != reply)
        return;
    const QByteArray tail = reply->readAll();
    bool success = reply->error() == QNetworkReply::NoError &&
        reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 200 &&
        reply->url().scheme() == QStringLiteral("https") &&
        m_target && m_target->size() + tail.size() <= kMaxEncryptedPackBytes;
    if (success) {
        success = m_target->write(tail) == tail.size();
        if (success)
            m_sha.addData(tail);
    }
    if (success) {
        success = m_sha.result().toHex() == m_offer.sha256.toLatin1() &&
            m_target->size() > 0;
    }
    if (success)
        success = m_target->commit();
    else if (m_target)
        m_target->cancelWriting();
    m_target.reset();
    m_reply = nullptr;
    reply->deleteLater();
    m_progress->setVisible(false);
    if (success) {
        installVerifiedDownloadedPackage();
    } else {
        setStatus(ui(
            I18n::text(16322),
            QStringLiteral("Download rejected (network, file or SHA-256). "
                           "Existing databases were not modified.")));
    }
    setBusy(false);
}

void MemsDocumentaryUpdateDialog::installVerifiedDownloadedPackage()
{
        bool accepted=false;
        QString password=QInputDialog::getText(
            this,ui(I18n::text(16323),
                    QStringLiteral("Database installation authorization")),
            ui(I18n::text(16324),
               QStringLiteral("Password provided by the administrator:")),
            QLineEdit::Password,QString(),&accepted);
        if(!accepted || password.isEmpty()){
            setStatus(ui(I18n::text(16325),
                QStringLiteral(
                "Verified download; installation cancelled. Database unchanged.")));
            setBusy(false);
            return;
        }
        const QString error=MemsDocumentaryLocalInstaller::installEncrypted(
            m_savedFilePath,m_language,password);
        password.fill(QChar(' '));
        if(error.isEmpty()){
            QSettings settings(QSettings::IniFormat,QSettings::UserScope,
                               QStringLiteral("ECU_Mems_Manager"));
            settings.setValue(
                QStringLiteral("DocumentaryVersions/%1").arg(m_language),
                m_offer.version);
            settings.sync();
            setStatus(ui(I18n::text(16326)
                   .arg(languageName(m_language),m_offer.version),
                QStringLiteral(
                "Documentary database %1 version %2 installed. "
                "Integrity, password, images and provenance verified. "
                "Reopen the database screen.")
                   .arg(languageName(m_language),m_offer.version)));
        }else{
            setStatus(error);
            QMessageBox::warning(this,
                ui(I18n::text(16327),
                   QStringLiteral("Update rejected")),error);
        }

}

void MemsDocumentaryUpdateDialog::installLocalEncrypted()
{
    const QString archive=QFileDialog::getOpenFileName(
        this,ui(I18n::text(16328),
                QStringLiteral("Choose the administrator-encrypted pack")),
        QString(),QStringLiteral("Packs documentaires chiffrés (*.encrypted)"));
    if(archive.isEmpty())
        return;
    bool accepted=false;
    QString password=QInputDialog::getText(
        this,ui(I18n::text(16329),
                QStringLiteral("Database installation authorization")),
        ui(I18n::text(16330),
           QStringLiteral("Password provided by the administrator:")),
        QLineEdit::Password,QString(),&accepted);
    if(!accepted || password.isEmpty()){
        password.fill(QChar(' '));
        setStatus(ui(I18n::text(16331),
            QStringLiteral(
            "Installation cancelled: administrator password is required.")));
        return;
    }
    setBusy(true);
    setStatus(ui(I18n::text(16332),
        QStringLiteral(
        "Authenticating the package, then verifying SQLite and images…")));
    // Both online and offline delivery use the same AES-GCM gate. An
    // unencrypted ZIP must never be a supported installation shortcut.
    const QString error=MemsDocumentaryLocalInstaller::installEncrypted(
        archive,m_language,password);
    password.fill(QChar(' '));
    setBusy(false);
    if(!error.isEmpty()){
        setStatus(error);
        QMessageBox::warning(this,
            ui(I18n::text(16333),
               QStringLiteral("Package rejected")),error);
        return;
    }
    const QString message=ui(I18n::text(16334)
        .arg(languageName(m_language)),
        QStringLiteral(
        "Documentary database %1 installed and verified. "
        "Previous version retained as backup. "
        "Reopen the database screen to read new documents.")
        .arg(languageName(m_language)));
    setStatus(message);
    QMessageBox::information(this,
        ui(I18n::text(16335),
           QStringLiteral("Database installed")),message);
}
