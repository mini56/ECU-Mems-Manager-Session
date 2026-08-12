#include "DatabaseSeed.h"

namespace DatabaseSeed
{

static void seedParameters(DatabaseManager &database)
{
    database.addParameter("battery_voltage", "Tension batterie", "V", "decimal",
        "Tension d'alimentation ECU. Utilisée pour conditionner le contrôle du temps de bobine.",
        10.0, 16.0, 13.5, 14.5);
    database.addParameter("coil_time", "Temps de charge bobine", "ms", "decimal",
        "Temps de charge de la bobine. Contrôle de cohérence 1,9 à 3,1 ms avec une tension proche de 14 V.",
        0.0, 20.0, 1.9, 3.1);
    database.addParameter("idle_error_hot_corrected", "Erreur ralenti à chaud corrigée", "ECU", "signed16",
        "7D14-15 décodé autour de 32768 puis corrigé par le réglage Position ralenti chaud.",
        -32768.0, 32767.0, -15.0, 15.0);
    database.addParameter("idle_hot_correction", "Correction position ralenti chaud", "ECU", "signed",
        "Correction issue du réglage Position ralenti chaud ; jamais une constante codée en dur.",
        -128.0, 127.0, -15.0, 15.0);
    database.addParameter("engine_rpm", "Régime moteur", "tr/min", "uint16",
        "Régime moteur décodé depuis la trame 0x80.", 0.0, 8000.0, 0.0, 0.0);
    database.addParameter("map_kpa", "Pression MAP", "kPa", "uint8",
        "Pression absolue collecteur décodée depuis la trame 0x80.", 0.0, 160.0, 0.0, 0.0);
    database.addParameter("short_term_fuel_trim", "Correction carburant court terme", "%", "signed",
        "Correction court terme ; affichage centré sur 100.", -100.0, 100.0, -20.0, 20.0);
    database.addParameter("long_term_fuel_trim", "Correction carburant long terme", "%", "signed",
        "Correction long terme ; affichage centré sur 100.", -100.0, 100.0, -20.0, 20.0);
}

static void seedDiagnosticRules(DatabaseManager &database)
{
    database.addDiagnosticRule("coil_time", "coil_time", "range_with_battery_gate", 1.9, 3.1,
        "ANOMALIE", "Temps de charge bobine hors plage à tension proche de 14 V.",
        "Contrôler en priorité le circuit primaire de la bobine, la bobine et son câblage.",
        "Contrôle de cohérence ECU MEMS ; plage documentée");
    database.addDiagnosticRule("idle_error_hot", "idle_error_hot_corrected", "range", -15.0, 15.0,
        "SURVEILLER", "Erreur de ralenti à chaud hors de la plage indicative.",
        "Contrôler le réglage de ralenti, l'IAC et les paramètres associés avant conclusion.",
        "Décodage par analyse/corrélation de trames");
    database.addDiagnosticRule("fuel_trim", "short_term_fuel_trim", "absolute_gt", 20.0, 0.0,
        "SURVEILLER", "Correction carburant importante.",
        "Rechercher prise d'air, pression carburant, injection ou mesure lambda avant de modifier les réglages.",
        "Contrôle de cohérence");
    database.addDiagnosticRule("battery", "battery_voltage", "range", 11.5, 15.2,
        "SURVEILLER", "Tension batterie hors plage de fonctionnement retenue.",
        "Contrôler batterie, masses, alimentation ECU et régulation de charge.",
        "Contrôle de cohérence");
}

static void seedActuators(DatabaseManager &database)
{
    database.addActuator("fuel_pump", "Fuel Pump", "Fuel pump relay", true, true, true);
    database.addActuator("ptc_relay", "PTC Relay", "PTC relay", true, true, true);
    database.addActuator("ac_relay", "A/C Relay", "Air conditioning relay", true, true, true);
    database.addActuator("injector", "Injector", "Fuel injector test", true, false, false);
    database.addActuator("ignition_coil", "Ignition Coil", "Ignition coil test", true, false, false);
    database.addActuator("iac", "Idle Air Control", "Idle air control valve", true, false, false);
    database.addActuator("purge_valve", "Purge Valve", "Evaporative purge valve", true, true, true);
    database.addActuator("o2_heater", "O2 Heater", "Oxygen sensor heater", true, true, true);
    database.addActuator("boost_valve", "Boost Valve", "Boost control valve", true, true, true);
    database.addActuator("fan1", "Fan 1", "Cooling fan 1", true, true, true);
    database.addActuator("fan2", "Fan 2", "Cooling fan 2", true, true, true);
    database.addActuator("fan3", "Fan 3", "Cooling fan 3", true, true, true);
}

bool populate(DatabaseManager &database)
{
    if (!database.isOpen()) return false;
    seedParameters(database);
    seedDiagnosticRules(database);
    seedActuators(database);
    return true;
}

}
