#include "../../database/MemsDocumentaryWindowsTransport.h"
#include "../../database/MemsDocumentaryPackCatalog.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QTemporaryDir>
#include <cstdio>

int main(int argc,char **argv)
{
    QCoreApplication app(argc,argv);
    const MemsDocumentaryHttpResult manifest=MemsDocumentaryWindowsTransport::get(
        MemsDocumentaryPackCatalog::manifestUrl(),256*1024);
    if(!manifest.ok()){
        std::fprintf(stderr,"MANIFEST_HTTPS_FAIL=%s HTTP=%d\n",
            qPrintable(manifest.error),manifest.httpStatus);
        return 1;
    }
    std::fprintf(stderr,"WINHTTP_MANIFEST_VERIFIED HTTP=%d BYTES=%lld\n",
        manifest.httpStatus,static_cast<long long>(manifest.bytes));
    QTemporaryDir destination;
    if(!destination.isValid())return 2;
    for(const QString &language:{QStringLiteral("fr"),QStringLiteral("en")}){
        const MemsDocumentaryOffer offer=
            MemsDocumentaryPackCatalog::parseManifest(manifest.body,language);
        if(!offer.available||!offer.error.isEmpty()){
            std::fprintf(stderr,"MANIFEST_OFFER_INVALID_%s=%s\n",
                qPrintable(language),qPrintable(offer.error));
            return 3;
        }
        const QString local=destination.filePath(language+QStringLiteral(".encrypted"));
        const MemsDocumentaryHttpResult file=MemsDocumentaryWindowsTransport::get(
            offer.releaseUrl,256LL*1024*1024,local,offer.sha256.toLatin1());
        if(!file.ok()){
            std::fprintf(stderr,"PACK_DOWNLOAD_FAILED_%s=%s HTTP=%d\n",
                qPrintable(language),qPrintable(file.error),file.httpStatus);
            return 4;
        }
        if(QFileInfo(local).size()!=file.bytes || file.bytes<1000){
            std::fprintf(stderr,"PACK_NOT_SAVED_%s\n",qPrintable(language));
            return 5;
        }
        std::fprintf(stderr,"WINHTTP_PACK_%s_VERIFIED BYTES=%lld SHA256=%s\n",
            qPrintable(language),static_cast<long long>(file.bytes),
            qPrintable(offer.sha256));
    }
    std::fprintf(stderr,"WINHTTP_FR_EN_MANIFEST_DOWNLOAD_AND_SHA256_PASS\n");
    return 0;
}
