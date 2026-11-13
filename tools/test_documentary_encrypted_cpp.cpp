#include "../database/MemsDocumentaryEncryptedPack.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <cstdio>

static bool check(bool ok,const char *name)
{
    if(!ok) std::fprintf(stderr,"MEMSDOC1_NATIVE_TEST_FAILED %s\n",name);
    return ok;
}

int main(int argc,char **argv)
{
    QCoreApplication app(argc,argv);
    if(!check(argc==3,"encrypted file and original ZIP needed")) return 1;
    const QString encrypted=QString::fromLocal8Bit(argv[1]);
    const QString original=QString::fromLocal8Bit(argv[2]);
    QTemporaryDir tmp;
    if(!check(tmp.isValid(),"temporary test folder")) return 1;
    const QString output=tmp.filePath(QStringLiteral("plaintext.zip"));
    QString failure=MemsDocumentaryEncryptedPack::decryptToZip(
        encrypted,QStringLiteral("SyntheticTestOnlyPassword456!"),output);
    if(!check(failure.isEmpty() && QFileInfo::exists(output),
              "authenticated AES-GCM decrypt with correct password")){
        std::fprintf(stderr,"%s\n",qPrintable(failure));
        return 1;
    }
    QFile actual(output), expected(original);
    if(!check(actual.open(QIODevice::ReadOnly) &&
              expected.open(QIODevice::ReadOnly) &&
              QCryptographicHash::hash(actual.readAll(),QCryptographicHash::Sha256)==
              QCryptographicHash::hash(expected.readAll(),QCryptographicHash::Sha256),
              "byte-exact authentic ZIP recovery")) return 1;
    const QString wrong=tmp.filePath(QStringLiteral("rejected.zip"));
    failure=MemsDocumentaryEncryptedPack::decryptToZip(
        encrypted,QStringLiteral("IncorrectPassword"),wrong);
    if(!check(!failure.isEmpty() && !QFileInfo::exists(wrong),
              "wrong password must never write plaintext")) return 1;
    const QString corruption=tmp.filePath(QStringLiteral("corrupted.encrypted"));
    {
        QFile encoded(encrypted), corrupted(corruption);
        if(!check(encoded.open(QIODevice::ReadOnly) &&
                  corrupted.open(QIODevice::WriteOnly),"tamper test IO"))
            return 1;
        QByteArray bytes=encoded.readAll();
        if(!check(bytes.size()>70,"envelope minimum")) return 1;
        bytes[60]=char(bytes.at(60)^0x01);
        if(!check(corrupted.write(bytes)==bytes.size(),"tamper test write"))
            return 1;
    }
    failure=MemsDocumentaryEncryptedPack::decryptToZip(
        corruption,QStringLiteral("SyntheticTestOnlyPassword456!"),wrong);
    if(!check(!failure.isEmpty() && !QFileInfo::exists(wrong),
              "GCM rejects modified ciphertext")) return 1;
    std::puts("MEMSDOC1_WINDOWS_AES256_GCM_PASSWORD_TAMPER_PASS");
    return 0;
}
