#pragma once
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringList>
#include <QVariant>

// The generated reference snapshot is immutable and separate from user data.
// The installer contains it; only the build worker creates it.
inline QString memsPreparedReferenceLanguage()
{
    const QString lang=QCoreApplication::instance()
        ? QCoreApplication::instance()->property("memsPreparedReferenceLanguage").toString() : QString();
    const QStringList supported{QStringLiteral("fr"),QStringLiteral("en"),QStringLiteral("de"),
                                QStringLiteral("es"),QStringLiteral("it"),QStringLiteral("pt")};
    return supported.contains(lang)?lang:QStringLiteral("fr");
}
inline QString memsPreparedReferenceRoot()
{
    if(!QCoreApplication::instance() ||
       QCoreApplication::instance()->property("memsDisablePreparedReference").toBool()) return QString();
    // Installed reference assets are immutable during a process lifetime.
    static const QString root=[](){
        const QString app=QCoreApplication::applicationDirPath();
        const QString path=app+QStringLiteral("/database/prepared-reference");
        QFile manifest(path+QStringLiteral("/manifest.json"));
        if(!manifest.open(QIODevice::ReadOnly))return QString();
        const QJsonObject m=QJsonDocument::fromJson(manifest.readAll()).object();
        if(m.value(QStringLiteral("schema")).toInt()!=1)return QString();
        QFile source(app+QStringLiteral("/database/reference/manifest.json"));
        if(!source.open(QIODevice::ReadOnly))return QString();
        if(QString::fromLatin1(QCryptographicHash::hash(source.readAll(),QCryptographicHash::Sha256).toHex())
           !=m.value(QStringLiteral("source_manifest_sha256")).toString())return QString();
        const QJsonObject sizes=m.value(QStringLiteral("sizes")).toObject();
        auto valid=[&](const QString &name){
            const QFileInfo info(path+QLatin1Char('/')+name);
            return info.isFile() && info.size()>0 && double(info.size())==sizes.value(name).toDouble(-1);
        };
        if(!valid(QStringLiteral("ecu_mems_reference_r5.sqlite")))return QString();
        for(const QString &lang:{QStringLiteral("fr"),QStringLiteral("en"),QStringLiteral("de"),
                                 QStringLiteral("es"),QStringLiteral("it"),QStringLiteral("pt")}){
            if(!valid(QStringLiteral("indexes/")+lang+QStringLiteral(".sqlite")))return QString();
            for(const QString &stem:{QStringLiteral("mems_1_2"),QStringLiteral("mems_1_3"),
                                    QStringLiteral("mems_1_6"),QStringLiteral("mems_1_9")})
                if(!valid(QStringLiteral("fiches/")+lang+QLatin1Char('/')+stem+QStringLiteral(".xml")))return QString();
        }
        return path;
    }();
    return root;
}
inline void memsDisablePreparedReference()
{
    if(QCoreApplication::instance())QCoreApplication::instance()->setProperty("memsDisablePreparedReference",true);
}
inline QString memsPreparedReferenceIndex()
{
    const QString root=memsPreparedReferenceRoot();
    return root.isEmpty()?QString():root+QStringLiteral("/indexes/")+memsPreparedReferenceLanguage()+QStringLiteral(".sqlite");
}
