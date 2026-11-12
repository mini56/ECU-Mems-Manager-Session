#ifndef MEMSDOCUMENTARYPACKCATALOG_H
#define MEMSDOCUMENTARYPACKCATALOG_H

#include <QByteArray>
#include <QString>

// The public update catalogue is descriptive.  No document is installed,
// decrypted or given diagnostic authority merely because it appears here.
struct MemsDocumentaryOffer
{
    bool available = false;
    QString language;
    QString version;
    QString releaseUrl;
    QString sha256;
    QString error;
};

class MemsDocumentaryPackCatalog
{
public:
    static QString manifestUrl();
    static QString selectedLanguage();
    static QString installedDatabasePath(const QString &language);
    static MemsDocumentaryOffer parseManifest(const QByteArray &json,
                                              const QString &language);
};

#endif
