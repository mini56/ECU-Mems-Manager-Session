#ifndef PROTOCOLCONTEXT_H
#define PROTOCOLCONTEXT_H

#include <cstdint>

enum class MemsEcuFamily : int
{
    Unknown = 0,
    Rosco13_16 = 1,
    Mems19 = 2
};

enum class MemsDiagnosticMode : int
{
    Unknown = 0,
    Normal = 1,
    Mode3 = 2,
    Mode4 = 3,
    Transition = 4
};

namespace MemsProtocolSafety
{
inline bool familyKnown(MemsEcuFamily family)
{
    return family != MemsEcuFamily::Unknown;
}

inline bool allowsGenericCommand(MemsEcuFamily family,
                                 MemsDiagnosticMode mode,
                                 std::uint8_t command)
{
    if (!familyKnown(family))
        return false;

    // F0 only reads the current diagnostic mode and is the sole generic
    // command allowed while outside the normal diagnostic session.
    if (command == 0xF0u)
        return true;

    if (mode != MemsDiagnosticMode::Normal)
        return false;

    // Generic ECU/ROSCO access is deliberately read-only. D1 is safe here
    // only because Mode 4 is rejected above; in Mode 4 D1 programs ROM.
    return command == 0xD0u || command == 0xD1u || command == 0xD2u;
}

inline bool isKnownLegacyMutation(std::uint8_t command)
{
    switch (command)
    {
    case 0xCCu: // clear faults
    case 0xFAu: // reset ECU - historical DLL semantics
    case 0x0Fu: // reset adjustments - historical DLL semantics
    case 0x11u: case 0x01u: // fuel pump
    case 0x12u: case 0x02u: // PTC
    case 0x13u: case 0x03u: // A/C
    case 0x18u: case 0x08u: // purge
    case 0x19u: case 0x09u: // O2 heater
    case 0x1Bu: case 0x0Bu: // boost/wastegate
    case 0x1Du: case 0x0Du: // fan 1
    case 0x1Eu: case 0x0Eu: // fan 2
    case 0x6Fu: case 0x67u: // fan 3
    case 0xF8u:             // fire coil in normal diagnostic mode
    case 0xFDu: case 0xFEu: // IAC step
    case 0xF4u:             // all actuators off / heartbeat in normal context
    case 0x79u: case 0x7Au: // fuel trim
    case 0x89u: case 0x8Au: // idle decay
    case 0x91u: case 0x92u: // idle speed
    case 0x93u: case 0x94u: // ignition advance offset
        return true;
    default:
        return false;
    }
}

inline bool allowsMutation(MemsEcuFamily family,
                           MemsDiagnosticMode mode,
                           std::uint8_t command)
{
    // Until a family-specific command table is proven, MEMS 1.9 writes and
    // actuator commands are refused. This is intentionally fail-closed.
    if (family != MemsEcuFamily::Rosco13_16 ||
        mode != MemsDiagnosticMode::Normal)
        return false;

    // Injector command F7 is family-sensitive (SPi vs MPI/1.9) and remains
    // blocked until the exact ECU subtype is known.
    if (command == 0xF7u || command == 0xEFu)
        return false;

    return isKnownLegacyMutation(command);
}
}

#endif // PROTOCOLCONTEXT_H
