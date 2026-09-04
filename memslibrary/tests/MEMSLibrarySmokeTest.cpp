#include <windows.h>

#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>

namespace {
using GetAbiVersionFn = std::uint32_t (*)();
using GetTextFn = const char* (*)();

std::wstring executableDirectory()
{
    wchar_t path[MAX_PATH] = {};
    const DWORD length = GetModuleFileNameW(nullptr, path, MAX_PATH);
    if (length == 0 || length >= MAX_PATH) {
        return {};
    }
    std::wstring full(path, length);
    const auto separator = full.find_last_of(L"\\/");
    return separator == std::wstring::npos ? std::wstring{} : full.substr(0, separator);
}
}

int main()
{
    const std::wstring directory = executableDirectory();
    if (directory.empty()) {
        std::cerr << "FAIL executable directory\n";
        return 1;
    }

    const std::wstring dllPath = directory + L"\\MEMSLibrary.dll";
    HMODULE module = LoadLibraryW(dllPath.c_str());
    if (!module) {
        std::cerr << "FAIL LoadLibrary MEMSLibrary.dll error=" << GetLastError() << "\n";
        return 2;
    }

    const auto abi = reinterpret_cast<GetAbiVersionFn>(GetProcAddress(module, "MEMSLibrary_GetAbiVersion"));
    const auto name = reinterpret_cast<GetTextFn>(GetProcAddress(module, "MEMSLibrary_GetName"));
    const auto role = reinterpret_cast<GetTextFn>(GetProcAddress(module, "MEMSLibrary_GetEngineRole"));

    if (!abi || !name || !role) {
        std::cerr << "FAIL required export missing\n";
        FreeLibrary(module);
        return 3;
    }

    const char* libraryName = name();
    const char* engineRole = role();
    const bool valid = abi() == 1u &&
        libraryName && std::strcmp(libraryName, "MEMSLibrary") == 0 &&
        engineRole && std::strcmp(engineRole, "generic_knowledge_library_engine") == 0;

    if (!valid) {
        std::cerr << "FAIL MEMSLibrary ABI identity mismatch\n";
        FreeLibrary(module);
        return 4;
    }

    std::cout << "MEMSLIBRARY_SMOKE_PASS abi=" << abi()
              << " name=" << libraryName
              << " role=" << engineRole << "\n";

    FreeLibrary(module);
    return 0;
}
