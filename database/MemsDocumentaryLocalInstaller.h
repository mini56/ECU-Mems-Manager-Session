#ifndef MEMSDOCUMENTARYLOCALINSTALLER_H
#define MEMSDOCUMENTARYLOCALINSTALLER_H

#include <QString>

// Installe UNIQUEMENT un pack documentaire local dans AppLocalDataLocation.
// Jamais de base ECU, d'index expert, ni de fichier dans le dossier programme.
// Le mot de passe est défini lors de la première installation et demandé
// pour les suivantes. Le contenu des archives publiques reste non publié.
class MemsDocumentaryLocalInstaller
{
public:
    static bool hasPassword();
    static bool passwordValid(const QString &password);
    static QString installDirectory(const QString &sourceDirectory,
                                    const QString &language,
                                    const QString &password);
    static QString installZip(const QString &archivePath,
                              const QString &language,
                              const QString &password);
};

#endif
