#ifndef MEMSREFERENCEPACKAGEREFRESH_H
#define MEMSREFERENCEPACKAGEREFRESH_H

enum class MemsReferencePackageAction
{
    None,
    Install,
    Update,
    Unavailable
};

MemsReferencePackageAction memsReferencePackageAction();
bool refreshMemsReferencePackage(MemsReferencePackageAction *detectedAction = nullptr);

#endif // MEMSREFERENCEPACKAGEREFRESH_H
