#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSslSocket>
#include <QTimer>
#include <QUrl>
#include <cstdio>

int main(int argc,char **argv)
{
    QCoreApplication app(argc,argv);
    std::fprintf(stderr,"QT_VERSION=%s\nSSL_SUPPORTED=%s\nSSL_LIBRARY=%s\nSSL_BUILD=%s\n",
        QT_VERSION_STR,QSslSocket::supportsSsl()?"yes":"no",
        qPrintable(QSslSocket::sslLibraryVersionString()),
        qPrintable(QSslSocket::sslLibraryBuildVersionString()));
    const QUrl url(QStringLiteral(
        "https://raw.githubusercontent.com/mini56/"
        "ECU-Mems-Manager-Session-MAJ/main/manifest.json"));
    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setRawHeader("User-Agent","ECU-MEMS-Manager-SESSION-documentary-updater");
    QNetworkAccessManager manager;
    QNetworkReply *reply=manager.get(request);
    QObject::connect(reply,QOverload<const QList<QSslError> &>::of(
        &QNetworkReply::sslErrors),[&](const QList<QSslError> &errors){
        for(const QSslError &error:errors)
            std::fprintf(stderr,"SSL_ERROR=%s\n",qPrintable(error.errorString()));
    });
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(reply,&QNetworkReply::finished,&loop,&QEventLoop::quit);
    QObject::connect(&timer,&QTimer::timeout,&loop,&QEventLoop::quit);
    timer.start(30000);
    loop.exec();
    if(!timer.isActive()){
        reply->abort();
        std::fprintf(stderr,"NETWORK_TIMEOUT\n");
        return 2;
    }
    const QByteArray body=reply->readAll();
    const int status=reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    std::fprintf(stderr,"NETWORK_ERROR=%d\nNETWORK_ERROR_TEXT=%s\nSTATUS=%d\nFINAL_URL=%s\nBYTES=%lld\n",
        int(reply->error()),qPrintable(reply->errorString()),status,
        qPrintable(reply->url().toString()),static_cast<long long>(body.size()));
    QJsonParseError jsonError;
    std::fprintf(stderr,"BODY_PREFIX=%s\\n",body.left(240).toHex().constData());
    const QJsonDocument manifest=QJsonDocument::fromJson(body,&jsonError);
    if(jsonError.error!=QJsonParseError::NoError||!manifest.isObject()){
        std::fprintf(stderr,"INVALID_JSON=%s\n",qPrintable(jsonError.errorString()));
        return 3;
    }
    const QJsonObject bases=manifest.object().value(QStringLiteral("documentary_bases")).toObject();
    const bool fr=bases.value(QStringLiteral("fr")).toObject().value(QStringLiteral("available")).toBool();
    const bool en=bases.value(QStringLiteral("en")).toObject().value(QStringLiteral("available")).toBool();
    std::fprintf(stderr,"BASE_FR_AVAILABLE=%d\nBASE_EN_AVAILABLE=%d\n",int(fr),int(en));
    if(reply->error()!=QNetworkReply::NoError||status!=200||!fr||!en)
        return 4;
    return 0;
}
