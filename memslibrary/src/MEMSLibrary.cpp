#include "MEMSLibrary.h"

namespace {
constexpr char kLibraryName[] = "MEMSLibrary";
constexpr char kEngineRole[] = "generic_knowledge_library_engine";
}

std::uint32_t MEMSLibrary_GetAbiVersion()
{
    return MEMSLIBRARY_ABI_VERSION;
}

const char* MEMSLibrary_GetName()
{
    return kLibraryName;
}

const char* MEMSLibrary_GetEngineRole()
{
    return kEngineRole;
}
