#include "../i18n.h"
#include "MemsGlobalSearchIndex.h"

#include <QFile>
#include <QFileInfo>
#include <QSet>
#include <QStandardPaths>

namespace {

QString referenceLanguage()
{
    const QString language=I18n::language().left(2).toLower();
    static const QSet<QString> supported={QStringLiteral("fr"),QStringLiteral("en"),QStringLiteral("es"),
                                           QStringLiteral("it"),QStringLiteral("pt"),QStringLiteral("de")};
    return supported.contains(language)?language:QString();
}

bool sameFileContent(const QString &left,const QString &right)
{
    QFile a(left),b(right);
    if(!a.open(QIODevice::ReadOnly) || !b.open(QIODevice::ReadOnly)) return false;
    return a.readAll()==b.readAll();
}

bool replaceFile(const QString &source,const QString &destination)
{
    QFile input(source);
    if(!input.open(QIODevice::ReadOnly)) return false;
    const QByteArray bytes=input.readAll();
    QFile output(destination);
    if(!output.open(QIODevice::WriteOnly|QIODevice::Truncate)) return false;
    return output.write(bytes)==bytes.size();
}

void synchronizeReferenceLanguageFiles()
{
        const QString language=referenceLanguage();
        if(language.isEmpty()) return;

        const QString root=QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)
            +QStringLiteral("/reference/fiches/");
        const QStringList stems={QStringLiteral("mems_1_2"),QStringLiteral("mems_1_3"),
                                 QStringLiteral("mems_1_6"),QStringLiteral("mems_1_9")};
        bool changed=false;

        for(const QString &stem:stems){
            const QString canonical=root+stem+QStringLiteral(".xml");
            if(!QFileInfo::exists(canonical)) continue;

            const QString french=root+stem+QStringLiteral(".fr.xml");
            if(!QFileInfo::exists(french)) QFile::copy(canonical,french);

            const QString localized=(language==QStringLiteral("fr"))
                ? french
                : root+stem+QLatin1Char('.')+language+QStringLiteral(".xml");
            if(!QFileInfo::exists(localized)) continue;
            if(sameFileContent(localized,canonical)) continue;
            if(replaceFile(localized,canonical)) changed=true;
        }

        // The search index reads the canonical cache files. Removing it after
        // a language switch makes the next search rebuild from the selected
        // localized sheets, so results and full sheets stay in the UI language.
        if(changed) QFile::remove(MemsGlobalSearchIndex::indexPath());
}

} // namespace

void synchronizeMemsReferenceLanguage()
{
    // Called after the package has been extracted and before index creation.
    synchronizeReferenceLanguageFiles();
}
