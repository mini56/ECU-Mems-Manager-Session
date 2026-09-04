#include "IaMemsDiagramCatalog.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

#include <cstdio>

namespace {

void printLine(const QString &line)
{
    const QByteArray bytes = line.toUtf8();
    std::fprintf(stderr, "%s\n", bytes.constData());
    std::fflush(stderr);
}

bool requirePrimaryGear(const QString &root)
{
    const QString question = QStringLiteral("Quel est le jeu axial du pignon primaire ?");
    const IaMemsDiagramSuggestion suggestion =
        IaMemsDiagramCatalog::suggestionForQuestion(question, root);
    if (!suggestion.isValid()) {
        printLine(QStringLiteral("FAIL primary gear: no visual suggestion"));
        return false;
    }
    if (suggestion.key != QStringLiteral("RCL0193ENG p.53")) {
        printLine(QStringLiteral("FAIL primary gear: wrong source %1 / %2")
                      .arg(suggestion.key, suggestion.relativePath));
        return false;
    }
    if (!QFileInfo::exists(suggestion.absolutePath)) {
        printLine(QStringLiteral("FAIL primary gear: resolved image missing"));
        return false;
    }
    printLine(QStringLiteral("PASS primary gear -> %1 / %2")
                  .arg(suggestion.key, suggestion.relativePath));
    return true;
}

bool requireBatteryRestoration(const QString &root)
{
    const QString question = QStringLiteral("Quelle est la procédure de restauration de la batterie ?");
    const IaMemsDiagramSuggestion suggestion =
        IaMemsDiagramCatalog::suggestionForQuestion(question, root);
    const QString expected = QStringLiteral(
        "ravemems/assets/DOC_0037_GENERAL_TESTBOOK_TB22382E_PDF/"
        "DOC_0037_GENERAL_TESTBOOK_TB22382E_PDF_X425.png");
    if (!suggestion.isValid()) {
        printLine(QStringLiteral("FAIL battery restoration: no visual suggestion"));
        return false;
    }
    if (QDir::cleanPath(suggestion.relativePath) != QDir::cleanPath(expected)) {
        printLine(QStringLiteral("FAIL battery restoration: wrong visual %1 / %2")
                      .arg(suggestion.key, suggestion.relativePath));
        return false;
    }
    if (!QFileInfo::exists(suggestion.absolutePath)) {
        printLine(QStringLiteral("FAIL battery restoration: resolved image missing"));
        return false;
    }
    printLine(QStringLiteral("PASS battery restoration -> %1 / %2")
                  .arg(suggestion.key, suggestion.relativePath));
    return true;
}

bool requireGenericProcedureSafe(const QString &root)
{
    const IaMemsDiagramSuggestion suggestion =
        IaMemsDiagramCatalog::suggestionForQuestion(
            QStringLiteral("Quelle est la procédure ?"), root);
    if (suggestion.isValid()) {
        printLine(QStringLiteral("FAIL generic procedure unexpectedly proposed %1 / %2")
                      .arg(suggestion.key, suggestion.relativePath));
        return false;
    }
    printLine(QStringLiteral("PASS generic procedure -> no visual"));
    return true;
}

} // namespace

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    if (argc != 2) {
        printLine(QStringLiteral("usage: ia_visual_question_selftest <database/reference>"));
        return 2;
    }

    const QString root = QDir::cleanPath(QString::fromLocal8Bit(argv[1]));
    if (!QFileInfo(root).isDir()) {
        printLine(QStringLiteral("FAIL reference root missing: %1").arg(root));
        return 3;
    }

    bool ok = true;
    ok = requirePrimaryGear(root) && ok;
    ok = requireBatteryRestoration(root) && ok;
    ok = requireGenericProcedureSafe(root) && ok;
    if (!ok)
        return 4;

    printLine(QStringLiteral(
        "IA_VISUAL_QUESTION_PASS primary=RCL0193ENG:p53 battery=TB22382E:p28 generic_safe=1"));
    return 0;
}
