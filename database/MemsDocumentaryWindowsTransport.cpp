#include "MemsDocumentaryWindowsTransport.h"

#include <QCryptographicHash>
#include <QFileInfo>
#include <QSaveFile>
#include <QRegularExpression>
#include <QUrl>

#ifdef Q_OS_WIN
#include <windows.h>
#include <winhttp.h>
#include <memory>

namespace {

class HttpHandle
{
public:
    explicit HttpHandle(HINTERNET handle=nullptr) : m_handle(handle) {}
    ~HttpHandle() { if (m_handle) WinHttpCloseHandle(m_handle); }
    HINTERNET get() const { return m_handle; }
    HttpHandle(const HttpHandle &) = delete;
    HttpHandle &operator=(const HttpHandle &) = delete;
private:
    HINTERNET m_handle=nullptr;
};

QString windowsError(const QString &operation)
{
    return QStringLiteral("%1 (WinHTTP %2)").arg(operation).arg(GetLastError());
}

} // namespace
#endif

MemsDocumentaryHttpResult MemsDocumentaryWindowsTransport::get(
    const QString &url, qint64 maxBytes,
    const QString &destinationFile, const QByteArray &expectedSha256)
{
    MemsDocumentaryHttpResult result;
#ifdef Q_OS_WIN
    if (maxBytes<=0 || maxBytes>256LL*1024*1024) {
        result.error=QStringLiteral("Limite de téléchargement invalide.");
        return result;
    }
    const QUrl source(url,QUrl::StrictMode);
    if (!source.isValid() || source.scheme()!=QStringLiteral("https") ||
        source.host().isEmpty() || !source.userInfo().isEmpty() ||
        !source.fragment().isEmpty()) {
        result.error=QStringLiteral("Adresse HTTPS invalide.");
        return result;
    }
    if (!destinationFile.isEmpty() &&
        (expectedSha256.size()!=64 ||
         !QRegularExpression(QStringLiteral("^[a-f0-9]{64}$"))
             .match(QString::fromLatin1(expectedSha256)).hasMatch())) {
        result.error=QStringLiteral("Empreinte attendue SHA-256 invalide.");
        return result;
    }
    const std::wstring host=source.host().toStdWString();
    const std::wstring requestPath=source.toString(
        QUrl::FullyEncoded|QUrl::RemoveScheme|QUrl::RemoveAuthority|
        QUrl::RemoveFragment).toStdWString();
    const INTERNET_PORT port=static_cast<INTERNET_PORT>(source.port(443));

    HttpHandle session(WinHttpOpen(
        L"ECU MEMS Manager SESSION - documentary/141",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,WINHTTP_NO_PROXY_BYPASS,0));
    if(!session.get()){
        result.error=windowsError(QStringLiteral("Connexion système Windows indisponible"));
        return result;
    }
    WinHttpSetTimeouts(session.get(),12000,12000,15000,30000);
    HttpHandle connection(WinHttpConnect(session.get(),host.c_str(),port,0));
    if(!connection.get()){
        result.error=windowsError(QStringLiteral("Connexion au serveur impossible"));
        return result;
    }
    const std::wstring path = requestPath.empty() ? L"/" : requestPath;
    HttpHandle request(WinHttpOpenRequest(
        connection.get(),L"GET",path.c_str(),nullptr,
        WINHTTP_NO_REFERER,WINHTTP_DEFAULT_ACCEPT_TYPES,WINHTTP_FLAG_SECURE));
    if(!request.get()){
        result.error=windowsError(QStringLiteral("Requête HTTPS impossible"));
        return result;
    }
    // Ne jamais suivre un renvoi d'un site HTTPS vers HTTP.
    DWORD redirectPolicy=WINHTTP_OPTION_REDIRECT_POLICY_DISALLOW_HTTPS_TO_HTTP;
    if(!WinHttpSetOption(request.get(),WINHTTP_OPTION_REDIRECT_POLICY,
                         &redirectPolicy,sizeof(redirectPolicy))){
        result.error=windowsError(QStringLiteral("Politique TLS non garantie"));
        return result;
    }
    if(!WinHttpSendRequest(request.get(),WINHTTP_NO_ADDITIONAL_HEADERS,0,
                           WINHTTP_NO_REQUEST_DATA,0,0,0) ||
       !WinHttpReceiveResponse(request.get(),nullptr)){
        result.error=windowsError(QStringLiteral("Lecture HTTPS impossible"));
        return result;
    }
    DWORD code=0,headerSize=sizeof(code);
    if(!WinHttpQueryHeaders(request.get(),
        WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,
        WINHTTP_HEADER_NAME_BY_INDEX,&code,&headerSize,WINHTTP_NO_HEADER_INDEX)){
        result.error=windowsError(QStringLiteral("Statut du serveur inaccessible"));
        return result;
    }
    result.httpStatus=static_cast<int>(code);
    if(code!=200){
        result.error=QStringLiteral("Le serveur GitHub a répondu HTTP %1.")
            .arg(static_cast<int>(code));
        return result;
    }
    std::unique_ptr<QSaveFile> output;
    if(!destinationFile.isEmpty()){
        output.reset(new QSaveFile(destinationFile));
        if(!output->open(QIODevice::WriteOnly)){
            result.error=QStringLiteral("Impossible d'ouvrir le fichier temporaire.");
            return result;
        }
    }
    QCryptographicHash digest(QCryptographicHash::Sha256);
    QByteArray block;
    block.resize(64*1024);
    for(;;){
        DWORD available=0;
        if(!WinHttpQueryDataAvailable(request.get(),&available)){
            result.error=windowsError(QStringLiteral("Téléchargement interrompu"));
            return result;
        }
        if(!available)break;
        while(available>0){
            const DWORD requested=qMin<DWORD>(available,static_cast<DWORD>(block.size()));
            DWORD received=0;
            if(!WinHttpReadData(request.get(),block.data(),requested,&received)){
                result.error=windowsError(QStringLiteral("Lecture des données interrompue"));
                return result;
            }
            if(!received){
                result.error=QStringLiteral("Téléchargement incomplet.");
                return result;
            }
            result.bytes+=received;
            if(result.bytes>maxBytes){
                result.error=QStringLiteral("Fichier trop volumineux.");
                return result;
            }
            digest.addData(block.constData(),static_cast<int>(received));
            if(output){
                if(output->write(block.constData(),received)!=received){
                    result.error=QStringLiteral("Écriture locale incomplète.");
                    return result;
                }
            }else{
                result.body.append(block.constData(),static_cast<int>(received));
            }
            available-=received;
        }
    }
    if(!result.bytes){
        result.error=QStringLiteral("Réponse du serveur vide.");
        return result;
    }
    if(output){
        if(digest.result().toHex()!=expectedSha256){
            result.error=QStringLiteral("SHA-256 du pack téléchargé incorrect.");
            return result;
        }
        if(!output->commit()){
            result.error=QStringLiteral("Échec de l'enregistrement sécurisé du pack.");
            return result;
        }
    }
#else
    Q_UNUSED(url)
    Q_UNUSED(maxBytes)
    Q_UNUSED(destinationFile)
    Q_UNUSED(expectedSha256)
    result.error=QStringLiteral("Transport Windows uniquement.");
#endif
    return result;
}
