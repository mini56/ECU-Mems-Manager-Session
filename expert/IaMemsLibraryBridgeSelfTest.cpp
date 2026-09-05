#include "IaMemsLibraryBridge.h"

#include <QCoreApplication>
#include <QTextStream>

namespace {

bool require(bool condition, const QString &message, QTextStream &err)
{
    if (condition)
        return true;
    err << "FAIL IA_MEMSLIBRARY_BRIDGE: " << message << Qt::endl;
    return false;
}

void printDiagnosticCase(const QString &name,
                         const QString &question,
                         const QStringList &keywords,
                         QTextStream &out)
{
    out << "DIAG_CASE_BEGIN name=" << name << Qt::endl;
    out << "DIAG_QUESTION " << question << Qt::endl;
    out << "DIAG_KEYWORDS " << keywords.join(QStringLiteral(" | ")) << Qt::endl;

    const IaMemsLibraryGrounding grounding = IaMemsLibraryBridge::retrieve(question, keywords);
    const bool expectedPrimaryPage = grounding.text.contains(QStringLiteral("DOC_RCL0193ENG"))
        && grounding.text.contains(QStringLiteral("page 53"));
    const bool expectedAxialValue = grounding.text.contains(QStringLiteral("0.089"))
        || grounding.text.contains(QStringLiteral("0.165"));

    out << "DIAG_RESULT ready=" << (grounding.libraryReady ? 1 : 0)
        << " count=" << grounding.resultCount
        << " rcl0193_p53=" << (expectedPrimaryPage ? 1 : 0)
        << " axial_value=" << (expectedAxialValue ? 1 : 0)
        << Qt::endl;
    out << "DIAG_GROUNDING_BEGIN" << Qt::endl;
    out << grounding.text << Qt::endl;
    out << "DIAG_GROUNDING_END" << Qt::endl;
    out << "DIAG_CASE_END name=" << name << Qt::endl;
}

} // namespace

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QTextStream out(stdout);
    QTextStream err(stderr);
    out.setCodec("UTF-8");
    err.setCodec("UTF-8");

    if (argc != 3) {
        err << "usage: ia_memslibrary_bridge_selftest <MEMSLibrary.dll> <Pack001-dir>" << Qt::endl;
        return 1;
    }

    qputenv("MEMS_LIBRARY_DLL", QByteArray(argv[1]));
    qputenv("MEMS_LIBRARY_PACK", QByteArray(argv[2]));

    const IaMemsLibraryGrounding primary = IaMemsLibraryBridge::retrieve(
        QStringLiteral("primary gear end float"),
        {QStringLiteral("primary"), QStringLiteral("gear"), QStringLiteral("end"), QStringLiteral("float")});
    if (!require(primary.libraryReady, QStringLiteral("Pack001 non disponible : %1").arg(primary.error), err)
        || !require(primary.resultCount > 0, QStringLiteral("aucun résultat primary gear"), err)
        || !require(primary.text.contains(QStringLiteral("DOC_RCL0193ENG")), QStringLiteral("provenance RCL0193ENG absente"), err)
        || !require(primary.text.contains(QStringLiteral("page 53")), QStringLiteral("page 53 absente"), err)
        || !require(primary.text.contains(QStringLiteral("0.089")) || primary.text.contains(QStringLiteral("0.165")),
                    QStringLiteral("valeur de jeu axial attendue absente"), err)) {
        return 2;
    }

    const IaMemsLibraryGrounding battery = IaMemsLibraryBridge::retrieve(
        QStringLiteral("battery restoration procedure"),
        {QStringLiteral("battery"), QStringLiteral("restoration"), QStringLiteral("procedure")});
    if (!require(battery.libraryReady, QStringLiteral("bibliothèque perdue au second appel"), err)
        || !require(battery.resultCount > 0, QStringLiteral("aucun résultat battery restoration"), err)
        || !require(battery.text.contains(QStringLiteral("DOC_RCL0221ENG")), QStringLiteral("provenance RCL0221ENG absente"), err)
        || !require(battery.text.contains(QStringLiteral("page 20")), QStringLiteral("page 20 absente"), err)) {
        return 3;
    }

    const IaMemsLibraryGrounding noHit = IaMemsLibraryBridge::retrieve(
        QStringLiteral("zzzxqv nonexistent mems evidence 987654321"),
        {QStringLiteral("zzzxqv"), QStringLiteral("987654321")});
    if (!require(noHit.libraryReady, QStringLiteral("bibliothèque indisponible pendant no-hit"), err)
        || !require(noHit.resultCount == 0, QStringLiteral("le no-hit a produit une fausse preuve"), err)
        || !require(noHit.text.trimmed().isEmpty(), QStringLiteral("le no-hit a produit un grounding documentaire"), err)) {
        return 4;
    }

    // Diagnostic only: these two cases mirror natural French questions as they reach
    // IaMemsLibraryBridge after the current MEMS Manager keyword enrichment.  They do
    // not change pass/fail status; the trace shows exactly what the bridge selected.
    printDiagnosticCase(
        QStringLiteral("french_short_axial"),
        QStringLiteral("Quel est le jeu axial du pignon primaire ?"),
        {QStringLiteral("jeu"), QStringLiteral("axial"), QStringLiteral("pignon"), QStringLiteral("primaire"),
         QStringLiteral("primary"), QStringLiteral("end"), QStringLiteral("float")},
        out);

    printDiagnosticCase(
        QStringLiteral("french_primary_removal"),
        QStringLiteral("Comment demonter le pignon primaire sur le moteur ?"),
        {QStringLiteral("comment"), QStringLiteral("demonter"), QStringLiteral("pignon"),
         QStringLiteral("primaire"), QStringLiteral("moteur"), QStringLiteral("primary")},
        out);

    out << "IA_MEMSLIBRARY_BRIDGE_PASS "
        << "primary=DOC_RCL0193ENG:p53 "
        << "battery=DOC_RCL0221ENG:p20 "
        << "provenance=1 no_hit_safe=1 diagnostics=2" << Qt::endl;
    return 0;
}
