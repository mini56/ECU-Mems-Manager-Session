#include "MemsDocumentaryUpdateDialog.h"
#include "MemsDocumentaryLocalInstaller.h"

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

    setWindowTitle(ui(QStringLiteral("Mise à jour des bases documentaires"),
                      QStringLiteral("Documentary database updates")));
    setObjectName(QStringLiteral("documentaryUpdateDialog"));
    setMinimumWidth(430);
    setModal(true);
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(18,18,18,16);
    layout->setSpacing(14);
    QLabel *heading = new QLabel(
        ui(QStringLiteral("Base documentaire : %1"),
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
        ui(QStringLiteral("Vérifier les mises à jour"),
           QStringLiteral("Check for updates")), this);
    m_check->setObjectName(QStringLiteral("documentaryCheckUpdate"));
    m_download = new QPushButton(
        ui(QStringLiteral("Télécharger la base"),
           QStringLiteral("Download database")), this);
    m_download->setObjectName(QStringLiteral("documentaryDownloadUpdate"));
    m_download->setEnabled(false);
    m_installLocal = new QPushButton(
        ui(QStringLiteral("Installer un pack sécurisé local"),
           QStringLiteral("Install local encrypted pack")),this);
    m_installLocal->setObjectName(QStringLiteral("documentaryInstallLocalEncrypted"));
    QPushButton *close = new QPushButton(
        ui(QStringLiteral("Fermer"), QStringLiteral("Close")), this);
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
    return m_language == QStringLiteral("fr") ? fr : en;
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
    if (m_reply)
        return;
    m_offer = MemsDocumentaryOffer{};
    m_manifestBytes.clear();
    setBusy(true);
    setStatus(ui(QStringLiteral("Consultation des mises à jour sur GitHub…"),
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
        const bool success = reply->error() == QNetworkReply::NoError &&
            reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 200 &&
            reply->url().scheme() == QStringLiteral("https") &&
            m_manifestBytes.size() <= kMaxManifestBytes;
        reply->deleteLater();
        if (!success) {
            setStatus(ui(QStringLiteral(
                "Impossible de vérifier les mises à jour. "
                "Aucune base installée n'a été modifiée."),
                QStringLiteral(
                "Unable to check updates. Installed databases are unchanged.")));
        } else {
            m_offer = MemsDocumentaryPackCatalog::parseManifest(
                m_manifestBytes, m_language);
            if (!m_offer.error.isEmpty()) {
                setStatus(ui(QStringLiteral(
                    "Manifeste de mise à jour non conforme. Téléchargement refusé."),
                    QStringLiteral(
                    "Invalid update manifest. Download refused.")));
            } else if (!m_offer.available) {
                setStatus(ui(QStringLiteral(
                    "Aucune base documentaire %1 n'est encore publiée. "
                    "Les bases techniques nécessaires au diagnostic ECU "
                    "restent disponibles et inchangées.").arg(
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
                    setStatus(ui(QStringLiteral(
                        "Base documentaire %1 à jour (version %2).")
                            .arg(languageName(m_language),savedVersion),
                        QStringLiteral(
                        "Documentary database %1 is up to date (version %2).")
                            .arg(languageName(m_language),savedVersion)));
                }else setStatus(ui(QStringLiteral(
                    "Pack chiffré version %1 disponible. "
                    "Le bouton Téléchargement installera la base après "
                    "vérification SHA-256 et saisie du mot de passe.")
                        .arg(m_offer.version),
                    QStringLiteral(
                    "Encrypted pack version %1 available. "
                    "Download verifies SHA-256 and installs after "
                    "password and SQLite validation.").arg(m_offer.version)));
            }
        }
        setBusy(false);
    });
}

void MemsDocumentaryUpdateDialog::startDownload()
{
    if (m_reply || !m_offer.available)
        return;
    const QString root = QStandardPaths::writableLocation(
        QStandardPaths::AppLocalDataLocation);
    if (root.isEmpty()) {
        setStatus(ui(QStringLiteral("Dossier de téléchargement local indisponible."),
                     QStringLiteral("Local download directory unavailable.")));
        return;
    }
    const QString dir = QDir(root).filePath(
        QStringLiteral("documentary/downloads/%1").arg(m_language));
    if (!QDir().mkpath(dir)) {
        setStatus(ui(QStringLiteral("Impossible de créer le dossier de téléchargement."),
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
        setStatus(ui(QStringLiteral("Impossible d'écrire le pack sur ce PC."),
                     QStringLiteral("Cannot write the pack on this computer.")));
        return;
    }
    m_sha.reset();
    m_progress->setRange(0,100);
    m_progress->setValue(0);
    m_progress->setVisible(true);
    setBusy(true);
    setStatus(ui(QStringLiteral("Téléchargement et vérification SHA-256…"),
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
        bool accepted=false;
        QString password=QInputDialog::getText(
            this,ui(QStringLiteral("Autorisation d'installation"),
                    QStringLiteral("Database installation authorization")),
            ui(QStringLiteral("Mot de passe donné par l'administrateur :"),
               QStringLiteral("Password provided by the administrator:")),
            QLineEdit::Password,QString(),&accepted);
        if(!accepted || password.isEmpty()){
            setStatus(ui(QStringLiteral(
                "Téléchargement vérifié, installation annulée : aucune base modifiée."),
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
            setStatus(ui(QStringLiteral(
                "Base documentaire %1 version %2 installée. "
                "Intégrité, mot de passe, images et provenance vérifiés. "
                "Redémarrer l'écran Base de données.")
                   .arg(languageName(m_language),m_offer.version),
                QStringLiteral(
                "Documentary database %1 version %2 installed. "
                "Integrity, password, images and provenance verified. "
                "Reopen the database screen.")
                   .arg(languageName(m_language),m_offer.version)));
        }else{
            setStatus(error);
            QMessageBox::warning(this,
                ui(QStringLiteral("Mise à jour refusée"),
                   QStringLiteral("Update rejected")),error);
        }
    } else {
        setStatus(ui(
            QStringLiteral("Téléchargement rejeté (réseau, fichier ou SHA-256). "
                           "Aucune base existante n'a été modifiée."),
            QStringLiteral("Download rejected (network, file or SHA-256). "
                           "Existing databases were not modified.")));
    }
    setBusy(false);
}

void MemsDocumentaryUpdateDialog::installLocalEncrypted()
{
    const QString archive=QFileDialog::getOpenFileName(
        this,ui(QStringLiteral("Choisir le pack sécurisé fourni par l'administrateur"),
                QStringLiteral("Choose the administrator-encrypted pack")),
        QString(),QStringLiteral("Packs documentaires chiffrés (*.encrypted)"));
    if(archive.isEmpty())
        return;
    bool accepted=false;
    QString password=QInputDialog::getText(
        this,ui(QStringLiteral("Autorisation d'installation"),
                QStringLiteral("Database installation authorization")),
        ui(QStringLiteral("Mot de passe communiqué par l'administrateur :"),
           QStringLiteral("Password provided by the administrator:")),
        QLineEdit::Password,QString(),&accepted);
    if(!accepted || password.isEmpty()){
        password.fill(QChar(' '));
        setStatus(ui(QStringLiteral(
            "Installation annulée : mot de passe administrateur obligatoire."),
            QStringLiteral(
            "Installation cancelled: administrator password is required.")));
        return;
    }
    setBusy(true);
    setStatus(ui(QStringLiteral(
        "Authentification du pack, puis contrôle SQLite et des images…"),
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
            ui(QStringLiteral("Pack refusé"),
               QStringLiteral("Package rejected")),error);
        return;
    }
    const QString message=ui(QStringLiteral(
        "Base documentaire %1 installée et contrôlée. "
        "Ancienne version conservée en sauvegarde. "
        "Rouvrir Base de données pour consulter les nouveaux documents.")
        .arg(languageName(m_language)),
        QStringLiteral(
        "Documentary database %1 installed and verified. "
        "Previous version retained as backup. "
        "Reopen the database screen to read new documents.")
        .arg(languageName(m_language)));
    setStatus(message);
    QMessageBox::information(this,
        ui(QStringLiteral("Base installée"),
           QStringLiteral("Database installed")),message);
}
