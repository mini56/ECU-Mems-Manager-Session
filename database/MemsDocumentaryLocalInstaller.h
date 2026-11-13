#ifndef MEMSDOCUMENTARYLOCALINSTALLER_H
#define MEMSDOCUMENTARYLOCALINSTALLER_H

#include <QString>

// Installation documentaire isolée : aucune base ECU, session ou experte modifiée.
// Le propriétaire du projet choisit LE secret de distribution, lors de la
// fabrication de ses packs AES-GCM ; l'utilisateur final ne choisit rien.
// La possession du bon secret est prouvée par l'authentification du pack.
class MemsDocumentaryLocalInstaller
{
public:
    static QString installEncrypted(const QString &encryptedPath,
                                    const QString &language,
                                    const QString &publisherPassword);

#ifdef MEMS_DOCUMENTARY_INSTALL_SELFTEST
    // Contrôles CI uniquement : ces entrées ne sont PAS exposées dans le
    // programme publié et n'acceptent pas de mot de passe inventé.
public:
#else
private:
#endif
    static QString installDirectory(const QString &sourceDirectory,
                                    const QString &language);
    static QString installZip(const QString &archivePath,
                              const QString &language);
};

#endif
