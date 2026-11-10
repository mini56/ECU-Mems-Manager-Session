#ifndef MEMS_DATABASE_UNIVERSAL_ACCESS_H
#define MEMS_DATABASE_UNIVERSAL_ACCESS_H

#include <QString>

// Rendering and localization for indexed MEMS reference results.
// Called directly from the authoritative database explorer (no UI polling).
namespace MemsDatabaseContent {
QString localizedTitle(const QString &content,const QString &fallback);
QString renderResultHtml(const QString &title,const QString &content,
                         const QString &category,const QString &generation,
                         const QString &sourceTable,const QString &diagrams);
}

#endif
