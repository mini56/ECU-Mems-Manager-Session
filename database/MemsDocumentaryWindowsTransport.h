#ifndef MEMSDOCUMENTARYWINDOWSTRANSPORT_H
#define MEMSDOCUMENTARYWINDOWSTRANSPORT_H

#include <QByteArray>
#include <QString>
#include <QtGlobal>

// HTTPS du système Windows (WinHTTP + magasins de certificats Windows).
// Aucune DLL OpenSSL extérieure ni désactivation de vérification TLS.
struct MemsDocumentaryHttpResult
{
    QByteArray body;
    QString error;
    qint64 bytes = 0;
    int httpStatus = 0;
    bool ok() const { return error.isEmpty() && httpStatus == 200; }
};

class MemsDocumentaryWindowsTransport
{
public:
    // Le manifeste reste en mémoire ; les gros packs sont écrits atomiquement
    // hors de l'installation du programme et vérifiés en SHA-256.
    static MemsDocumentaryHttpResult get(
        const QString &url, qint64 maxBytes,
        const QString &destinationFile = QString(),
        const QByteArray &expectedSha256 = QByteArray());
};

#endif
