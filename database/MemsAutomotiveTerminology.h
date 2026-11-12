#ifndef MEMS_AUTOMOTIVE_TERMINOLOGY_H
#define MEMS_AUTOMOTIVE_TERMINOLOGY_H

#include <QRegularExpression>
#include <QString>
#include <QVector>
#include <algorithm>

// Manufacturer references and source documents are never changed. This
// small, reviewed interface glossary localizes *navigation titles* instantly;
// it is not a substitute for a verified translation of workshop procedures.
namespace MemsAutomotiveTerminology {
struct Term { const char *en; const char *fr; const char *es; const char *it; const char *pt; const char *de; };

inline QString localizedTitle(const QString &manufacturerTitle, const QString &language)
{
    const QString code = language.left(2).toLower();
    if (code == QLatin1String("en")) return manufacturerTitle;
    static const Term terms[] = {
    {"brake master cylinder", "maître-cylindre de frein", "cilindro maestro de freno", "pompa freni", "bomba central dos travões", "Hauptbremszylinder"},
    {"brake servo air filter", "filtre à air du servofrein", "filtro de aire del servofreno", "filtro aria servofreno", "filtro de ar do servofreio", "Luftfilter des Bremskraftverstärkers"},
    {"brake pressure reducing valve", "répartiteur de freinage", "válvula reductora de presión de freno", "valvola di riduzione pressione freni", "válvula redutora da pressão de travagem", "Bremsdruckminderer"},
    {"rear brake shoes", "mâchoires de frein arrière", "zapatas de freno traseras", "ganasce freno posteriori", "sapatas dos travões traseiros", "Hinterradbremsbacken"},
    {"front brake pads", "plaquettes de frein avant", "pastillas de freno delanteras", "pastiglie freno anteriori", "pastilhas dos travões dianteiros", "Vorderradbremsbeläge"},
    {"rear brake drum", "tambour de frein arrière", "tambor de freno trasero", "tamburo freno posteriore", "tambor do travão traseiro", "hintere Bremstrommel"},
    {"front brake disc", "disque de frein avant", "disco de freno delantero", "disco freno anteriore", "disco do travão dianteiro", "vordere Bremsscheibe"},
    {"rear brake caliper", "étrier de frein arrière", "pinza de freno trasera", "pinza freno posteriore", "pinça do travão traseiro", "hinterer Bremssattel"},
    {"front brake caliper", "étrier de frein avant", "pinza de freno delantera", "pinza freno anteriore", "pinça do travão dianteiro", "vorderer Bremssattel"},
    {"handbrake cable", "câble de frein à main", "cable de freno de mano", "cavo freno a mano", "cabo do travão de mão", "Handbremsseil"},
    {"brake light switch", "contacteur de feux stop", "interruptor de luz de freno", "interruttore luce stop", "interruptor da luz de travão", "Bremslichtschalter"},
    {"brake hydraulic system", "circuit hydraulique de freinage", "sistema hidráulico de frenos", "impianto idraulico freni", "sistema hidráulico dos travões", "hydraulisches Bremssystem"},
    {"brake system", "système de freinage", "sistema de frenos", "impianto frenante", "sistema de travagem", "Bremssystem"},
    {"brake bands", "bandes de frein", "bandas de freno", "nastri dei freni", "cintas de travão", "Bremsbänder"},
    {"brake shoes", "mâchoires de frein", "zapatas de freno", "ganasce freno", "sapatas dos travões", "Bremsbacken"},
    {"brake pads", "plaquettes de frein", "pastillas de freno", "pastiglie dei freni", "pastilhas dos travões", "Bremsbeläge"},
    {"brake servo", "servofrein", "servofreno", "servofreno", "servofreio", "Bremskraftverstärker"},
    {"brake disc", "disque de frein", "disco de freno", "disco freno", "disco de travão", "Bremsscheibe"},
    {"brake drum", "tambour de frein", "tambor de freno", "tamburo freno", "tambor de travão", "Bremstrommel"},
    {"brake caliper", "étrier de frein", "pinza de freno", "pinza freno", "pinça de travão", "Bremssattel"},
    {"brake fluid", "liquide de frein", "líquido de frenos", "liquido freni", "líquido dos travões", "Bremsflüssigkeit"},
    {"brake pedal", "pédale de frein", "pedal de freno", "pedale del freno", "pedal do travão", "Bremspedal"},
    {"brake bleeding", "purge du circuit de freinage", "purga de frenos", "spurgo freni", "purga dos travões", "Bremsenentlüftung"},
    {"brake adjustment", "réglage des freins", "ajuste de frenos", "registrazione freni", "afinação dos travões", "Bremseneinstellung"},
    {"cylinder head gasket", "joint de culasse", "junta de culata", "guarnizione della testata", "junta da cabeça do motor", "Zylinderkopfdichtung"},
    {"cylinder head", "culasse", "culata", "testata", "cabeça do motor", "Zylinderkopf"},
    {"rocker shaft assembly", "ensemble axe de culbuteurs", "conjunto eje de balancines", "gruppo albero bilancieri", "conjunto do eixo dos balanceiros", "Kipphebelwelle komplett"},
    {"rocker cover", "cache-culbuteurs", "tapa de balancines", "coperchio punterie", "tampa das válvulas", "Ventildeckel"},
    {"timing chain and gears", "chaîne et pignons de distribution", "cadena y engranajes de distribución", "catena e ingranaggi distribuzione", "corrente e carretos de distribuição", "Steuerkette und Zahnräder"},
    {"timing chain", "chaîne de distribution", "cadena de distribución", "catena di distribuzione", "corrente de distribuição", "Steuerkette"},
    {"timing belt", "courroie de distribution", "correa de distribución", "cinghia di distribuzione", "correia de distribuição", "Zahnriemen"},
    {"tightening torque", "couple de serrage", "par de apriete", "coppia di serraggio", "binário de aperto", "Anzugsdrehmoment"},
    {"torque wrench", "clé dynamométrique", "llave dinamométrica", "chiave dinamometrica", "chave dinamométrica", "Drehmomentschlüssel"},
    {"valve clearance", "jeu aux soupapes", "holgura de válvulas", "gioco valvole", "folga das válvulas", "Ventilspiel"},
    {"valve stem seal", "joint de queue de soupape", "retén de vástago de válvula", "paraolio stelo valvola", "vedante da haste da válvula", "Ventilschaftdichtung"},
    {"oil pressure switch", "contacteur de pression d'huile", "presostato de aceite", "interruttore pressione olio", "interruptor de pressão do óleo", "Öldruckschalter"},
    {"oil seal", "joint d'étanchéité d'huile", "retén de aceite", "paraolio", "retentor de óleo", "Öldichtung"},
    {"fuel injection", "injection de carburant", "inyección de combustible", "iniezione carburante", "injeção de combustível", "Kraftstoffeinspritzung"},
    {"fuel pressure regulator", "régulateur de pression de carburant", "regulador de presión de combustible", "regolatore pressione carburante", "regulador de pressão de combustível", "Kraftstoffdruckregler"},
    {"cooling system", "circuit de refroidissement", "sistema de refrigeración", "impianto di raffreddamento", "sistema de arrefecimento", "Kühlsystem"},
    {"water pump", "pompe à eau", "bomba de agua", "pompa acqua", "bomba de água", "Wasserpumpe"},
    {"oil pump", "pompe à huile", "bomba de aceite", "pompa olio", "bomba de óleo", "Ölpumpe"},
    {"exhaust manifold", "collecteur d'échappement", "colector de escape", "collettore di scarico", "coletor de escape", "Abgaskrümmer"},
    {"inlet manifold", "collecteur d'admission", "colector de admisión", "collettore di aspirazione", "coletor de admissão", "Ansaugkrümmer"},
    {"intake manifold", "collecteur d'admission", "colector de admisión", "collettore di aspirazione", "coletor de admissão", "Ansaugkrümmer"},
    {"throttle body", "boîtier papillon", "cuerpo de mariposa", "corpo farfallato", "corpo da borboleta", "Drosselklappengehäuse"},
    {"crankshaft", "vilebrequin", "cigüeñal", "albero motore", "cambota", "Kurbelwelle"},
    {"camshaft", "arbre à cames", "árbol de levas", "albero a camme", "árvore de cames", "Nockenwelle"},
    {"rocker shaft", "axe de culbuteurs", "eje de balancines", "albero bilancieri", "eixo dos balanceiros", "Kipphebelwelle"},
    {"flywheel", "volant moteur", "volante motor", "volano", "volante do motor", "Schwungrad"},
    {"clutch", "embrayage", "embrague", "frizione", "embraiagem", "Kupplung"},
    {"gearbox", "boîte de vitesses", "caja de cambios", "cambio", "caixa de velocidades", "Getriebe"},
    {"differential", "différentiel", "diferencial", "differenziale", "diferencial", "Differenzial"},
    {"starter motor", "démarreur", "motor de arranque", "motorino di avviamento", "motor de arranque", "Anlasser"},
    {"alternator", "alternateur", "alternador", "alternatore", "alternador", "Lichtmaschine"},
    {"wheel bearing", "roulement de roue", "rodamiento de rueda", "cuscinetto ruota", "rolamento da roda", "Radlager"},
    {"handbrake", "frein à main", "freno de mano", "freno a mano", "travão de mão", "Handbremse"},
    {"brakes", "freins", "frenos", "freni", "travões", "Bremsen"},
    {"brake", "frein", "freno", "freno", "travão", "Bremse"},
    {"servo", "servo", "servo", "servo", "servo", "Servo"},
    {"master", "maître", "maestro", "principale", "principal", "Haupt"},
    {"rear", "arrière", "trasero", "posteriore", "traseiro", "hinten"},
    {"front", "avant", "delantero", "anteriore", "dianteiro", "vorn"},
    {"gasket", "joint", "junta", "guarnizione", "junta", "Dichtung"},
    {"seal", "joint d'étanchéité", "retén", "guarnizione", "vedante", "Dichtung"},
    {"valves", "soupapes", "válvulas", "valvole", "válvulas", "Ventile"},
    {"valve", "soupape", "válvula", "valvola", "válvula", "Ventil"},
    {"cylinder", "cylindre", "cilindro", "cilindro", "cilindro", "Zylinder"},
    {"engine", "moteur", "motor", "motore", "motor", "Motor"},
    {"pump", "pompe", "bomba", "pompa", "bomba", "Pumpe"},
    {"cable", "câble", "cable", "cavo", "cabo", "Seilzug"},
    {"hoses", "durites", "mangueras", "tubi flessibili", "mangueiras", "Schläuche"},
    {"hose", "durite", "manguera", "tubo flessibile", "mangueira", "Schlauch"},
    {"pipes", "tuyaux", "tuberías", "tubazioni", "tubos", "Leitungen"},
    {"pipe", "tuyau", "tubería", "tubo", "tubo", "Leitung"},
    {"filter", "filtre", "filtro", "filtro", "filtro", "Filter"},
    {"bleeding", "purge", "purga", "spurgo", "purga", "Entlüftung"},
    {"adjusting", "réglage", "ajuste", "registrazione", "afinação", "Einstellung"},
    {"adjustment", "réglage", "ajuste", "registrazione", "afinação", "Einstellung"},
    {"overhaul", "révision complète", "revisión general", "revisione completa", "revisão completa", "Überholung"},
    {"inspection", "contrôle", "inspección", "ispezione", "inspeção", "Prüfung"},
    {"checking", "contrôle", "comprobación", "controllo", "verificação", "Prüfung"},
    {"removal", "dépose", "desmontaje", "smontaggio", "remoção", "Ausbau"},
    {"removing", "dépose", "desmontaje", "smontaggio", "remoção", "Ausbau"},
    {"refitting", "repose", "montaje", "rimontaggio", "montagem", "Einbau"},
    {"reassembly", "remontage", "montaje", "rimontaggio", "remontagem", "Zusammenbau"},
    {"assembly", "ensemble", "conjunto", "gruppo", "conjunto", "Baugruppe"},
    {"fitting", "montage", "montaje", "montaggio", "montagem", "Einbau"},
    {"suspension", "suspension", "suspensión", "sospensione", "suspensão", "Federung"},
    {"steering", "direction", "dirección", "sterzo", "direção", "Lenkung"},
    {"exhaust", "échappement", "escape", "scarico", "escape", "Auspuff"},
    {"inlet", "admission", "admisión", "aspirazione", "admissão", "Einlass"},
    {"timing", "distribution", "distribución", "distribuzione", "distribuição", "Steuerung"},
    {"piston", "piston", "pistón", "pistone", "pistão", "Kolben"},
    {"bolts", "boulons", "pernos", "bulloni", "parafusos", "Schrauben"},
    {"bolt", "boulon", "perno", "bullone", "parafuso", "Schraube"},
    {"nuts", "écrous", "tuercas", "dadi", "porcas", "Muttern"},
    {"nut", "écrou", "tuerca", "dado", "porca", "Mutter"},
    {"cover", "couvercle", "tapa", "coperchio", "tampa", "Deckel"},
    {"spring", "ressort", "muelle", "molla", "mola", "Feder"},
    {"bearings", "roulements", "rodamientos", "cuscinetti", "rolamentos", "Lager"},
    {"bearing", "roulement", "rodamiento", "cuscinetto", "rolamento", "Lager"},
    {"torque", "couple de serrage", "par de apriete", "coppia di serraggio", "binário de aperto", "Drehmoment"},
    {"gear", "pignon", "engranaje", "ingranaggio", "carreto", "Zahnrad"},
    {"gears", "pignons", "engranajes", "ingranaggi", "carretos", "Zahnräder"},
    };
    QVector<const Term *> ordered;
    ordered.reserve(sizeof(terms) / sizeof(terms[0]));
    for (const Term &term : terms) ordered.append(&term);
    std::stable_sort(ordered.begin(), ordered.end(), [](const Term *a, const Term *b) {
        return qstrlen(a->en) > qstrlen(b->en);
    });
    QString result = manufacturerTitle;
    for (const Term *term : ordered) {
        const char *translation = nullptr;
        if (code == QLatin1String("fr")) translation = term->fr;
        else if (code == QLatin1String("es")) translation = term->es;
        else if (code == QLatin1String("it")) translation = term->it;
        else if (code == QLatin1String("pt")) translation = term->pt;
        else if (code == QLatin1String("de")) translation = term->de;
        if (!translation) return manufacturerTitle;
        const QRegularExpression pattern(QStringLiteral("\\b%1\\b").arg(
            QRegularExpression::escape(QString::fromLatin1(term->en))),
            QRegularExpression::CaseInsensitiveOption);
        result.replace(pattern, QString::fromUtf8(translation));
    }
    return result;
}

inline QString menuHeading(const QString &language)
{
    const QString code = language.left(2).toLower();
    if (code == QLatin1String("fr")) return QString::fromUtf8("Rubriques retrouvées pour « %1 » :");
    if (code == QLatin1String("en")) return QString::fromUtf8("Sections found for “%1”:");
    if (code == QLatin1String("es")) return QString::fromUtf8("Secciones encontradas para « %1 » :");
    if (code == QLatin1String("it")) return QString::fromUtf8("Sezioni trovate per « %1 » :");
    if (code == QLatin1String("pt")) return QString::fromUtf8("Rubricas encontradas para « %1 » :");
    if (code == QLatin1String("de")) return QString::fromUtf8("Gefundene Abschnitte für „%1“:");
    return QString::fromUtf8("Sections found for “%1”:");
}

inline QString menuInstruction(const QString &language)
{
    const QString code = language.left(2).toLower();
    if (code == QLatin1String("fr")) return QString::fromUtf8("Réponds par le numéro de la rubrique à consulter, ou écris une autre recherche.");
    if (code == QLatin1String("en")) return QString::fromUtf8("Reply with the number of the section to open, or enter another search.");
    if (code == QLatin1String("es")) return QString::fromUtf8("Responde con el número de la sección que deseas consultar o introduce otra búsqueda.");
    if (code == QLatin1String("it")) return QString::fromUtf8("Rispondi con il numero della sezione da consultare oppure inserisci una nuova ricerca.");
    if (code == QLatin1String("pt")) return QString::fromUtf8("Responda com o número da rubrica que pretende consultar ou introduza outra pesquisa.");
    if (code == QLatin1String("de")) return QString::fromUtf8("Antworte mit der Nummer des gewünschten Abschnitts oder gib eine neue Suche ein.");
    return QStringLiteral("Reply with the section number or enter a new search.");
}
} // namespace MemsAutomotiveTerminology

#endif
