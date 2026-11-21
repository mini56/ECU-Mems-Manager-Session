#include "protocolcontext.h"

#include <cstdlib>
#include <iostream>

static void require(bool condition, const char *message)
{
    if (!condition)
    {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

int main()
{
    using namespace MemsProtocolSafety;

    require(allowsGenericCommand(MemsEcuFamily::Unknown, MemsDiagnosticMode::Normal, 0xD1),
            "read-only D1 identification must remain available before family classification");
    require(allowsGenericCommand(MemsEcuFamily::Rosco13_16, MemsDiagnosticMode::Normal, 0xD1),
            "D1 identification must remain available in normal ROSCO mode");
    require(!allowsGenericCommand(MemsEcuFamily::Unknown, MemsDiagnosticMode::Mode4, 0xD1),
            "Mode4 D1 must be refused even when ECU family is still unknown");
    require(!allowsGenericCommand(MemsEcuFamily::Rosco13_16, MemsDiagnosticMode::Mode4, 0xD1),
            "Mode4 D1 must be refused because it programs ROM");
    require(!allowsGenericCommand(MemsEcuFamily::Rosco13_16, MemsDiagnosticMode::Normal, 0xD3),
            "generic D3 recoding/programming must remain unavailable");
    require(allowsGenericCommand(MemsEcuFamily::Unknown, MemsDiagnosticMode::Unknown, 0xF0),
            "F0 mode query must remain available to recover an unknown context");
    require(allowsGenericCommand(MemsEcuFamily::Rosco13_16, MemsDiagnosticMode::Mode4, 0xF0),
            "F0 mode query must remain available in Mode4");
    require(!allowsGenericCommand(MemsEcuFamily::Rosco13_16, MemsDiagnosticMode::Normal, 0xF3),
            "generic mode-change F3 must be refused");
    require(!allowsGenericCommand(MemsEcuFamily::Rosco13_16, MemsDiagnosticMode::Normal, 0xF4),
            "generic F4 must be refused because its meaning is context-sensitive");
    require(!allowsGenericCommand(MemsEcuFamily::Rosco13_16, MemsDiagnosticMode::Normal, 0xF5),
            "generic F5 mode change must be refused");

    require(allowsMutation(MemsEcuFamily::Rosco13_16, MemsDiagnosticMode::Normal, 0xCC),
            "connected legacy ROSCO must permit existing clear fault control");
    require(allowsMutation(MemsEcuFamily::Rosco13_16, MemsDiagnosticMode::Normal, 0x81),
            "original ROSCO normal-mode Save command must be available");
    require(allowsMutation(MemsEcuFamily::Mems19, MemsDiagnosticMode::Normal, 0x81),
            "MEMS 1.9 K-Line Save requires its own transport and normal mode");
    require(!allowsMutation(MemsEcuFamily::Rosco13_16, MemsDiagnosticMode::Mode4, 0x81),
            "never reinterpret normal-mode Save as calibration operation");
    require(!allowsMutation(MemsEcuFamily::Mems2J, MemsDiagnosticMode::Normal, 0x81) &&
            !allowsMutation(MemsEcuFamily::Mems3, MemsDiagnosticMode::Normal, 0x81),
            "KWP 2J/3 must not receive a ROSCO Save byte");
    require(allowsMutation(MemsEcuFamily::Rosco13_16, MemsDiagnosticMode::Normal, 0x0F),
            "connected legacy ROSCO must permit existing reset adjustment control");
    require(allowsMutation(MemsEcuFamily::Rosco13_16, MemsDiagnosticMode::Normal, 0xFA),
            "connected legacy ROSCO must permit existing ECU reset control");
    require(allowsMutation(MemsEcuFamily::Rosco13_16, MemsDiagnosticMode::Normal, 0xF8),
            "normal ROSCO coil test remains authorized by mode guard once family is proven");
    require(!allowsMutation(MemsEcuFamily::Rosco13_16, MemsDiagnosticMode::Mode4, 0xF8),
            "Mode4 F8 must be refused because it writes calibration");
    require(!allowsMutation(MemsEcuFamily::Rosco13_16, MemsDiagnosticMode::Mode4, 0xD1),
            "Mode4 D1 mutation must be refused");
    require(!allowsMutation(MemsEcuFamily::Rosco13_16, MemsDiagnosticMode::Mode4, 0xD3),
            "Mode4 D3 mutation must be refused");
    require(!allowsMutation(MemsEcuFamily::Rosco13_16, MemsDiagnosticMode::Normal, 0xF7),
            "injector F7 must stay blocked until exact family subtype is proven");
    require(!allowsMutation(MemsEcuFamily::Rosco13_16, MemsDiagnosticMode::Normal, 0xEF),
            "MPI injector EF must stay blocked until exact family subtype is proven");
    require(allowsMutation(MemsEcuFamily::Mems19, MemsDiagnosticMode::Normal, 0xCC),
            "MEMS1.9 connected ECU must permit its existing fault clear command");
    require(allowsMutation(MemsEcuFamily::Mems19, MemsDiagnosticMode::Normal, 0x0F),
            "MEMS1.9 connected ECU must permit its existing reset adjustments command");
    require(allowsMutation(MemsEcuFamily::Mems19, MemsDiagnosticMode::Normal, 0xFA),
            "MEMS1.9 connected ECU must permit its existing reset ECU command");
    require(!allowsMutation(MemsEcuFamily::Mems19, MemsDiagnosticMode::Normal, 0xF8),
            "MEMS1.9 unknown subtype must not silently expand command catalog");
    require(!allowsMutation(MemsEcuFamily::Mems3, MemsDiagnosticMode::Normal, 0xCC),
            "never send ROSCO clear command to MEMS 3");
    require(!allowsGenericCommand(MemsEcuFamily::Mems3, MemsDiagnosticMode::Normal, 0xCC),
            "MEMS3 has no ROSCO generic command path");
    require(!allowsMutation(MemsEcuFamily::Mems2J, MemsDiagnosticMode::Normal, 0xCC),
            "do not send ROSCO fault reset command to MEMS 2J");
    require(!allowsMutation(MemsEcuFamily::Unknown, MemsDiagnosticMode::Normal, 0x11),
            "unknown family actuator command must fail closed");
    require(!allowsMutation(MemsEcuFamily::Rosco13_16, MemsDiagnosticMode::Transition, 0x11),
            "transition state actuator command must fail closed");

    std::cout << "PASS protocol context safety policy\n";
    return 0;
}
