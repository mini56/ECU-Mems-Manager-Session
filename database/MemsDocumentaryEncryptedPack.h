#ifndef MEMSDOCUMENTARYENCRYPTEDPACK_H
#define MEMSDOCUMENTARYENCRYPTEDPACK_H

#include <QString>

// Authenticated encrypted ZIP format MEMSDOC1:
// magic[8], version[1], salt[16], nonce[12], ciphertext_size_le[8],
// AES-256-GCM ciphertext[size], authentication tag[16].
// PBKDF2-HMAC-SHA256: 210000 iterations. Entire header is GCM AAD.
// Never writes decrypted data unless password and GCM authentication succeed.
class MemsDocumentaryEncryptedPack
{
public:
    static QString decryptToZip(const QString &encryptedPath,
                                const QString &password,
                                const QString &destinationZip);
};
#endif
