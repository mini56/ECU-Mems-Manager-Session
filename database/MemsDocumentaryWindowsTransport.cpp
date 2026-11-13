#include "MemsDocumentaryWindowsTransport.h"

#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QUrl>

#ifdef Q_OS_WIN
#include <windows.h>
#include <winhttp.h>
#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace {

constexpr int kAttempts = 3;
constexpr qint64 kPackLimit = 256LL * 1024 * 1024;
class WinHandle
{
public:
    explicit WinHandle(HINTERNET value=nullptr):m_value(value){}
    ~WinHandle(){if(m_value)WinHttpCloseHandle(m_value);}
    HINTERNET get() const {return m_value;}
    WinHandle(const WinHandle&)=delete;
    WinHandle& operator=(const WinHandle&)=delete;
private:
    HINTERNET m_value=nullptr;
};

QString lastWinError(const QString &message)
{
    return QStringLiteral("%1 (WinHTTP %2)").arg(message).arg(GetLastError());
}

bool validUrl(const QUrl &url)
{
    return url.isValid() &&
        url.scheme()==QStringLiteral("https") &&
        !url.host().isEmpty() &&
        url.userInfo().isEmpty() &&
        url.fragment().isEmpty() &&
        url.port(443)==443;
}

bool hashMatches(const QString &path,const QByteArray &expected)
{
    QFile file(path);
    if(!file.open(QIODevice::ReadOnly))return false;
    QCryptographicHash sha(QCryptographicHash::Sha256);
    while(!file.atEnd()){
        QByteArray bytes=file.read(1024*1024);
        if(bytes.isEmpty()&&!file.atEnd())return false;
        sha.addData(bytes);
    }
    return sha.result().toHex()==expected;
}

QString responseHeader(HINTERNET request,const wchar_t *header)
{
    DWORD bytes=0;
    WinHttpQueryHeaders(request,WINHTTP_QUERY_CUSTOM,header,
                        WINHTTP_NO_OUTPUT_BUFFER,&bytes,WINHTTP_NO_HEADER_INDEX);
    if(GetLastError()!=ERROR_INSUFFICIENT_BUFFER || !bytes || bytes>1024)
        return QString();
    std::vector<wchar_t> data(bytes/sizeof(wchar_t)+1,0);
    if(!WinHttpQueryHeaders(request,WINHTTP_QUERY_CUSTOM,header,
                            data.data(),&bytes,WINHTTP_NO_HEADER_INDEX))
        return QString();
    return QString::fromWCharArray(data.data());
}

bool setLegacyUserProxy(HINTERNET session,HINTERNET request,const QUrl &url)
{
    // Windows 8.1+ already handles PAC/WPAD using AUTOMATIC_PROXY.
    // On Windows 7, use the current user's Internet proxy configuration.
    WINHTTP_CURRENT_USER_IE_PROXY_CONFIG ie{};
    if(!WinHttpGetIEProxyConfigForCurrentUser(&ie))
        return true; // WinHTTP may still use its machine-level proxy
    auto release=[&ie](){
        if(ie.lpszAutoConfigUrl)GlobalFree(ie.lpszAutoConfigUrl);
        if(ie.lpszProxy)GlobalFree(ie.lpszProxy);
        if(ie.lpszProxyBypass)GlobalFree(ie.lpszProxyBypass);
    };
    if(ie.lpszProxy && *ie.lpszProxy){
        WINHTTP_PROXY_INFO p{};
        p.dwAccessType=WINHTTP_ACCESS_TYPE_NAMED_PROXY;
        p.lpszProxy=ie.lpszProxy;
        p.lpszProxyBypass=ie.lpszProxyBypass;
        const bool ok=WinHttpSetOption(request,WINHTTP_OPTION_PROXY,&p,sizeof(p))!=FALSE;
        release();
        return ok;
    }
    if(ie.lpszAutoConfigUrl || ie.fAutoDetect){
        WINHTTP_AUTOPROXY_OPTIONS options{};
        options.dwFlags=ie.lpszAutoConfigUrl ?
            WINHTTP_AUTOPROXY_CONFIG_URL : WINHTTP_AUTOPROXY_AUTO_DETECT;
        options.dwAutoDetectFlags=WINHTTP_AUTO_DETECT_TYPE_DHCP |
                                  WINHTTP_AUTO_DETECT_TYPE_DNS_A;
        options.lpszAutoConfigUrl=ie.lpszAutoConfigUrl;
        options.fAutoLogonIfChallenged=TRUE;
        WINHTTP_PROXY_INFO p{};
        const std::wstring wideUrl=url.toString(QUrl::FullyEncoded).toStdWString();
        if(WinHttpGetProxyForUrl(session,wideUrl.c_str(),&options,&p)){
            const bool ok=WinHttpSetOption(request,WINHTTP_OPTION_PROXY,&p,sizeof(p))!=FALSE;
            if(p.lpszProxy)GlobalFree(p.lpszProxy);
            if(p.lpszProxyBypass)GlobalFree(p.lpszProxyBypass);
            release();
            return ok;
        }
        // Proxy auto-discovery can be unavailable on home networks; don't
        // make that fatal if the server is reachable directly.
    }
    release();
    return true;
}

MemsDocumentaryHttpResult singleRequest(
    const QUrl &source,qint64 maxBytes,
    const QString &partFile,const QByteArray &expected,
    bool automaticProxy)
{
    MemsDocumentaryHttpResult result;
    WinHandle session(WinHttpOpen(L"ECU MEMS Manager SESSION/141",
        automaticProxy ? WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY :
                         WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,WINHTTP_NO_PROXY_BYPASS,0));
    if(!session.get()){
        result.error=lastWinError(QStringLiteral("Ouverture HTTPS impossible"));
        return result;
    }
    WinHttpSetTimeouts(session.get(),12000,12000,15000,30000);
    // TLS 1.2 works with current GitHub endpoints; Windows 7 requires its
    // TLS 1.2 system support and updates enabled.
    DWORD protocols=WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2;
    if(!WinHttpSetOption(session.get(),WINHTTP_OPTION_SECURE_PROTOCOLS,
                         &protocols,sizeof(protocols))){
        result.error=lastWinError(QStringLiteral("TLS 1.2 indisponible sur Windows"));
        return result;
    }
    const std::wstring host=source.host().toStdWString();
    WinHandle connection(WinHttpConnect(session.get(),host.c_str(),443,0));
    if(!connection.get()){
        result.error=lastWinError(QStringLiteral("Connexion au serveur impossible"));
        return result;
    }
    const QString encoded=source.path(QUrl::FullyEncoded)+
        (source.hasQuery()?QStringLiteral("?")+source.query(QUrl::FullyEncoded):QString());
    const std::wstring requestPath=(encoded.isEmpty()?QStringLiteral("/"):encoded).toStdWString();
    WinHandle request(WinHttpOpenRequest(connection.get(),L"GET",
        requestPath.c_str(),nullptr,WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,WINHTTP_FLAG_SECURE));
    if(!request.get()){
        result.error=lastWinError(QStringLiteral("Requête HTTPS impossible"));
        return result;
    }
    DWORD noDowngrade=WINHTTP_OPTION_REDIRECT_POLICY_DISALLOW_HTTPS_TO_HTTP;
    if(!WinHttpSetOption(request.get(),WINHTTP_OPTION_REDIRECT_POLICY,
                         &noDowngrade,sizeof(noDowngrade))){
        result.error=lastWinError(QStringLiteral("Protection HTTPS indisponible"));
        return result;
    }
    if(!automaticProxy && !setLegacyUserProxy(session.get(),request.get(),source)){
        result.error=lastWinError(QStringLiteral("Configuration du proxy impossible"));
        return result;
    }
    qint64 offset=0;
    if(!partFile.isEmpty()&&QFileInfo::exists(partFile))
        offset=QFileInfo(partFile).size();
    if(offset>0 && offset<maxBytes){
        const QString range=QStringLiteral("Range: bytes=%1-\r\n").arg(offset);
        const std::wstring rangeHeader=range.toStdWString();
        if(!WinHttpAddRequestHeaders(request.get(),rangeHeader.c_str(),
                                   static_cast<DWORD>(-1L),WINHTTP_ADDREQ_FLAG_ADD)){
            result.error=lastWinError(QStringLiteral("Reprise HTTPS impossible"));
            return result;
        }
    }else if(offset>=maxBytes){
        QFile::remove(partFile);
        offset=0;
    }
    if(!WinHttpSendRequest(request.get(),WINHTTP_NO_ADDITIONAL_HEADERS,0,
                           WINHTTP_NO_REQUEST_DATA,0,0,0) ||
       !WinHttpReceiveResponse(request.get(),nullptr)){
        result.error=lastWinError(QStringLiteral("Échec du téléchargement HTTPS"));
        return result;
    }
    DWORD status=0,headerBytes=sizeof(status);
    if(!WinHttpQueryHeaders(request.get(),
        WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,
        WINHTTP_HEADER_NAME_BY_INDEX,&status,&headerBytes,WINHTTP_NO_HEADER_INDEX)){
        result.error=lastWinError(QStringLiteral("Statut HTTPS illisible"));
        return result;
    }
    result.httpStatus=static_cast<int>(status);
    if(status!=200 && status!=206){
        result.error=QStringLiteral("Serveur HTTP %1").arg(status);
        return result;
    }
    if(status==206){
        const QString range=responseHeader(request.get(),L"Content-Range");
        const QRegularExpression expression(QStringLiteral("^bytes (\\d+)-(\\d+)/(\\d+)$"));
        const auto m=expression.match(range);
        if(!offset || !m.hasMatch() ||
           m.captured(1).toLongLong()!=offset ||
           m.captured(2).toLongLong()<offset ||
           m.captured(3).toLongLong()>maxBytes){
            result.error=QStringLiteral("Réponse de reprise HTTP invalide.");
            return result;
        }
    }else{
        offset=0; // Server ignored Range: restart the file from the beginning.
    }
    std::unique_ptr<QFile> output;
    if(!partFile.isEmpty()){
        output.reset(new QFile(partFile));
        if(!output->open(offset?QIODevice::WriteOnly|QIODevice::Append:
                                QIODevice::WriteOnly|QIODevice::Truncate)){
            result.error=QStringLiteral("Impossible d'écrire le fichier de reprise.");
            return result;
        }
    }
    result.bytes=offset;
    QByteArray buffer(64*1024,'\0');
    for(;;){
        DWORD available=0;
        if(!WinHttpQueryDataAvailable(request.get(),&available)){
            result.error=lastWinError(QStringLiteral("Téléchargement interrompu"));
            return result;
        }
        if(!available)break;
        while(available){
            DWORD wanted=qMin<DWORD>(available,static_cast<DWORD>(buffer.size()));
            DWORD received=0;
            if(!WinHttpReadData(request.get(),buffer.data(),wanted,&received)){
                result.error=lastWinError(QStringLiteral("Lecture HTTPS interrompue"));
                return result;
            }
            if(!received){
                result.error=QStringLiteral("Réception interrompue.");
                return result;
            }
            result.bytes+=received;
            if(result.bytes>maxBytes){
                result.error=QStringLiteral("Fichier trop volumineux.");
                return result;
            }
            if(output){
                if(output->write(buffer.constData(),received)!=received){
                    result.error=QStringLiteral("Écriture du pack interrompue.");
                    return result;
                }
            }else{
                result.body.append(buffer.constData(),static_cast<int>(received));
            }
            available-=received;
        }
    }
    if(output){
        output->close();
        if(!hashMatches(partFile,expected)){
            QFile::remove(partFile); // New retry starts clean if truncated/corrupt.
            result.error=QStringLiteral("Fichier incomplet ou SHA-256 non conforme.");
            return result;
        }
    }else if(result.body.isEmpty()){
        result.error=QStringLiteral("Catalogue GitHub vide.");
        return result;
    }
    result.error.clear();
    result.httpStatus=200;
    return result;
}

} // namespace
#endif

MemsDocumentaryHttpResult MemsDocumentaryWindowsTransport::get(
    const QString &url,qint64 maxBytes,
    const QString &destinationFile,const QByteArray &expectedSha256)
{
    MemsDocumentaryHttpResult result;
#ifdef Q_OS_WIN
    const QUrl source(url,QUrl::StrictMode);
    if(!validUrl(source) || maxBytes<=0 || maxBytes>kPackLimit){
        result.error=QStringLiteral("Adresse HTTPS ou limite de fichier invalide.");
        return result;
    }
    if(!destinationFile.isEmpty()){
        if(!QRegularExpression(QStringLiteral("^[a-f0-9]{64}$"))
            .match(QString::fromLatin1(expectedSha256)).hasMatch()){
            result.error=QStringLiteral("Empreinte SHA-256 absente ou invalide.");
            return result;
        }
        if(QFileInfo::exists(destinationFile) &&
           hashMatches(destinationFile,expectedSha256)){
            result.bytes=QFileInfo(destinationFile).size();
            result.httpStatus=200;
            return result;
        }
        if(QFileInfo::exists(destinationFile))QFile::remove(destinationFile);
    }
    const QString part=destinationFile.isEmpty()?QString():
        destinationFile+QStringLiteral(".part");
    // WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY is supported by Windows 8.1+;
    // older versions use the current user's IE/WinHTTP proxy settings.
    for(int attempt=0;attempt<kAttempts;++attempt){
        result=singleRequest(source,maxBytes,part,expectedSha256,true);
        if(!result.ok() &&
           (result.error.contains(QStringLiteral("WinHTTP 87")) ||
            result.error.contains(QStringLiteral("WinHTTP 12009")))){
            result=singleRequest(source,maxBytes,part,expectedSha256,false);
        }
        if(result.ok()){
            if(!destinationFile.isEmpty()){
                if(!QFile::rename(part,destinationFile)){
                    result.error=QStringLiteral("Impossible de finaliser le pack vérifié.");
                    return result;
                }
                result.bytes=QFileInfo(destinationFile).size();
            }
            return result;
        }
        // Do not retry authorization failures, untrusted responses or bad paths.
        if(result.httpStatus==401 || result.httpStatus==403 ||
           result.httpStatus==404 ||
           result.error.contains(QStringLiteral("reprise HTTP invalide")) ||
           result.error.contains(QStringLiteral("Protection HTTPS indisponible")))
            break;
        if(attempt+1<kAttempts)
            std::this_thread::sleep_for(std::chrono::milliseconds(350*(attempt+1)));
    }
#else
    Q_UNUSED(url)
    Q_UNUSED(maxBytes)
    Q_UNUSED(destinationFile)
    Q_UNUSED(expectedSha256)
    result.error=QStringLiteral("Transport Windows indisponible.");
#endif
    return result;
}
