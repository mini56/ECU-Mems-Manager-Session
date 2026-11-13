#include "MemsDocumentaryEncryptedPack.h"

#include <QCryptographicHash>
#include <QFile>
#include <QPasswordDigestor>
#include <QSaveFile>
#include <QtEndian>

#ifdef Q_OS_WIN
#include <windows.h>
#include <bcrypt.h>
#endif

namespace {

constexpr int kHeaderLength = 45;
constexpr int kTagLength = 16;
constexpr qint64 kMaxDocumentaryPackBytes = 256LL * 1024 * 1024;
constexpr int kIterations = 210000;

QString reject(const QString &detail)
{
    return QStringLiteral("Pack documentaire chiffré refusé : %1").arg(detail);
}

#ifdef Q_OS_WIN
QString authenticatedAesDecrypt(const QByteArray &bytes,
                                const QByteArray &secret,
                                const QByteArray &nonce,
                                const QByteArray &salt,
                                QByteArray *output)
{
    QByteArray keyMaterial=QPasswordDigestor::deriveKeyPbkdf2(
        QCryptographicHash::Sha256,secret,salt,kIterations,32);
    if(keyMaterial.size()!=32)
        return reject(QStringLiteral("dérivation de la clé impossible."));
    BCRYPT_ALG_HANDLE algorithm=nullptr;
    BCRYPT_KEY_HANDLE key=nullptr;
    NTSTATUS result=BCryptOpenAlgorithmProvider(
        &algorithm,BCRYPT_AES_ALGORITHM,nullptr,0);
    if(!BCRYPT_SUCCESS(result)){
        keyMaterial.fill('\0');
        return reject(QStringLiteral("moteur cryptographique Windows indisponible."));
    }
    result=BCryptSetProperty(
        algorithm,BCRYPT_CHAINING_MODE,
        reinterpret_cast<PUCHAR>(const_cast<wchar_t *>(BCRYPT_CHAIN_MODE_GCM)),
        sizeof(BCRYPT_CHAIN_MODE_GCM),0);
    DWORD keyObjectLength=0;
    DWORD obtained=0;
    if(BCRYPT_SUCCESS(result)){
        result=BCryptGetProperty(
            algorithm,BCRYPT_OBJECT_LENGTH,
            reinterpret_cast<PUCHAR>(&keyObjectLength),
            sizeof(keyObjectLength),&obtained,0);
    }
    QByteArray keyObject;
    if(BCRYPT_SUCCESS(result) && keyObjectLength>0 &&
       keyObjectLength<1024*1024){
        keyObject.resize(static_cast<int>(keyObjectLength));
        keyObject.fill('\0');
        result=BCryptGenerateSymmetricKey(
            algorithm,&key,
            reinterpret_cast<PUCHAR>(keyObject.data()),
            static_cast<ULONG>(keyObject.size()),
            reinterpret_cast<PUCHAR>(keyMaterial.data()),
            static_cast<ULONG>(keyMaterial.size()),0);
    }else{
        result=static_cast<NTSTATUS>(0xC000000DL);
    }
    keyMaterial.fill('\0');
    if(!BCRYPT_SUCCESS(result)){
        keyObject.fill('\0');
        BCryptCloseAlgorithmProvider(algorithm,0);
        return reject(QStringLiteral("initialisation AES-256 impossible."));
    }
    const int ciphertextLength=bytes.size()-kHeaderLength-kTagLength;
    output->resize(ciphertextLength);
    BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO info;
    BCRYPT_INIT_AUTH_MODE_INFO(info);
    info.pbNonce=reinterpret_cast<PUCHAR>(const_cast<char *>(nonce.constData()));
    info.cbNonce=static_cast<ULONG>(nonce.size());
    info.pbAuthData=reinterpret_cast<PUCHAR>(
        const_cast<char *>(bytes.constData()));
    info.cbAuthData=kHeaderLength;
    info.pbTag=reinterpret_cast<PUCHAR>(
        const_cast<char *>(bytes.constData()+kHeaderLength+ciphertextLength));
    info.cbTag=kTagLength;
    DWORD actual=0;
    result=BCryptDecrypt(
        key,
        reinterpret_cast<PUCHAR>(
            const_cast<char *>(bytes.constData()+kHeaderLength)),
        static_cast<ULONG>(ciphertextLength),
        &info,
        nullptr,0,
        reinterpret_cast<PUCHAR>(output->data()),
        static_cast<ULONG>(output->size()),
        &actual,0);
    BCryptDestroyKey(key);
    keyObject.fill('\0');
    BCryptCloseAlgorithmProvider(algorithm,0);
    if(!BCRYPT_SUCCESS(result) ||
       actual!=static_cast<DWORD>(ciphertextLength)){
        output->fill('\0');
        output->clear();
        return reject(QStringLiteral(
            "mot de passe incorrect ou paquet altéré (authentification AES-GCM)."));
    }
    return QString();
}
#endif
} // namespace

QString MemsDocumentaryEncryptedPack::decryptToZip(
    const QString &encryptedPath,const QString &password,
    const QString &destinationZip)
{
    const QFileInfo info(encryptedPath);
    if(!info.isFile() || info.isSymLink() ||
       info.size()<kHeaderLength+kTagLength+1 ||
       info.size()>kMaxDocumentaryPackBytes)
        return reject(QStringLiteral("fichier absent ou taille invalide."));
    if(password.isEmpty())
        return reject(QStringLiteral("mot de passe obligatoire."));
    QFile source(encryptedPath);
    if(!source.open(QIODevice::ReadOnly))
        return reject(QStringLiteral("fichier inaccessible."));
    QByteArray encoded=source.readAll();
    source.close();
    if(encoded.size()!=info.size() ||
       encoded.size()<kHeaderLength+kTagLength+1)
        return reject(QStringLiteral("archive tronquée."));
    if(encoded.left(8)!=QByteArrayLiteral("MEMSDOC1") ||
       static_cast<unsigned char>(encoded.at(8))!=1)
        return reject(QStringLiteral("format ou version inconnue."));
    const quint64 payloadBytes=qFromLittleEndian<quint64>(
        reinterpret_cast<const uchar *>(encoded.constData()+37));
    if(payloadBytes==0 ||
       payloadBytes!=quint64(encoded.size()-kHeaderLength-kTagLength))
        return reject(QStringLiteral("longueur ou en-tête invalide."));
    const QByteArray salt=encoded.mid(9,16);
    const QByteArray nonce=encoded.mid(25,12);
    QByteArray plaintext;
#ifdef Q_OS_WIN
    QByteArray passwordBytes=password.toUtf8();
    const QString decryptError=authenticatedAesDecrypt(
        encoded,passwordBytes,nonce,salt,&plaintext);
    passwordBytes.fill('\0');
    if(!decryptError.isEmpty())
        return decryptError;
#else
    Q_UNUSED(salt)
    Q_UNUSED(nonce)
    Q_UNUSED(plaintext)
    return reject(QStringLiteral("AES-GCM disponible uniquement dans le Setup Windows."));
#endif
    if(!plaintext.startsWith(QByteArrayLiteral("PK\x03\x04"))){
        plaintext.fill('\0');
        return reject(QStringLiteral("le contenu déchiffré n'est pas une archive ZIP."));
    }
    QSaveFile target(destinationZip);
    if(!target.open(QIODevice::WriteOnly)){
        plaintext.fill('\0');
        return reject(QStringLiteral("destination temporaire inaccessible."));
    }
    const qint64 written=target.write(plaintext);
    plaintext.fill('\0');
    if(written!=payloadBytes || !target.commit()){
        target.cancelWriting();
        return reject(QStringLiteral("échec de l'écriture sécurisée du ZIP."));
    }
    return QString();
}
