#ifndef MEMSDOCUMENTARYUPDATEDIALOG_H
#define MEMSDOCUMENTARYUPDATEDIALOG_H

#include "MemsDocumentaryPackCatalog.h"

#include <QByteArray>
#include <QCryptographicHash>
#include <QDialog>
#include <QNetworkAccessManager>
#include <QPointer>
#include <QScopedPointer>

class QLabel;
class QPushButton;
class QProgressBar;
class QNetworkReply;
class QSaveFile;

// GitHub downloads remain protected. A local ZIP supplied by the user can
// be installed with a password after verifying the documentary-only SQLite.
// The ECU diagnostic and expert databases are never opened or modified here.
class MemsDocumentaryUpdateDialog : public QDialog
{
public:
    explicit MemsDocumentaryUpdateDialog(const QString &language, QWidget *parent=nullptr);
    ~MemsDocumentaryUpdateDialog() override;

private:
    void checkManifest();
    void startDownload();
    void installLocalZip();
    void setBusy(bool busy);
    void setStatus(const QString &message);
    void finishDownload(QNetworkReply *reply);
    QString ui(const QString &fr, const QString &en) const;

    QString m_language;
    QNetworkAccessManager m_network;
    QPointer<QNetworkReply> m_reply;
    QLabel *m_status = nullptr;
    QPushButton *m_check = nullptr;
    QPushButton *m_download = nullptr;
    QPushButton *m_installLocal = nullptr;
    QProgressBar *m_progress = nullptr;
    QByteArray m_manifestBytes;
    MemsDocumentaryOffer m_offer;
    QScopedPointer<QSaveFile> m_target;
    QCryptographicHash m_sha{QCryptographicHash::Sha256};
    QString m_savedFilePath;
};

#endif
