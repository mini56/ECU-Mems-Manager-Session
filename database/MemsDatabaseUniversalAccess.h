#ifndef MEMS_DATABASE_UNIVERSAL_ACCESS_H
#define MEMS_DATABASE_UNIVERSAL_ACCESS_H

#include <QString>

// Rendering and localization for indexed MEMS reference results.
// Called directly from the authoritative database explorer (no UI polling).
namespace MemsDatabaseContent {
// Only resolves a local, installed image beneath database/reference/images.
// Empty when the reference is absent or unsafe (including parent traversal).
QString localImagePath(const QString &reference);
QString localizedTitle(const QString &content,const QString &fallback);
QString renderResultHtml(const QString &title,const QString &content,
                         const QString &category,const QString &generation,
                         const QString &sourceTable,const QString &diagrams);
}

#endif
