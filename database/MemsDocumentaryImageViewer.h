#ifndef MEMSDOCUMENTARYIMAGEVIEWER_H
#define MEMSDOCUMENTARYIMAGEVIEWER_H

#include <QString>
class QWidget;

// Visionneuse partagée par la base de données et IA MEMS :
// ajustement, zoom et déplacement pour lire les légendes des pages constructeur.
class MemsDocumentaryImageViewer
{
public:
    static void show(QWidget *parent, const QString &path);
};

#endif
