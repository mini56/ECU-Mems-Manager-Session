#include "MultilingualKnowledgeReader.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QFile>
#include <QTextStream>

namespace {

QByteArray fileSha256(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return QByteArray();
    QCryptographicHash hash(QCryptographicHash::Sha256);
    if (!hash.addData(&file))
        return QByteArray();
    return hash.result().toHex();
}

int fail(const QString &message)
{
    QTextStream err(stderr);
    err << "FAIL " << message << Qt::endl;
    return 1;
}

void pass(const QString &message)
{
    QTextStream out(stdout);
    out << "PASS " << message << Qt::endl;
}

const MultilingualTableCell *cellAt(const MultilingualTable &table, int row, int column)
{
    for (const MultilingualTableCell &cell : table.cells) {
        if (cell.row == row && cell.column == column)
            return &cell;
    }
    return nullptr;
}

} // namespace

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    if (app.arguments().size() != 2)
        return fail(QStringLiteral("usage: multilingual_knowledge_runtime_selftest <sqlite>"));

    const QString databasePath = app.arguments().at(1);
    const QByteArray beforeHash = fileSha256(databasePath);
    if (beforeHash.isEmpty())
        return fail(QStringLiteral("cannot hash input SQLite"));

    MultilingualKnowledgeReader reader;
    if (!reader.openReadOnly(databasePath))
        return fail(QStringLiteral("openReadOnly: %1").arg(reader.lastError()));
    if (!reader.supportsFoundation())
        return fail(QStringLiteral("multilingual foundation tables not detected"));
    pass(QStringLiteral("RUNTIME OPEN READ-ONLY + FOUNDATION DETECTED"));

    const QString knowledgeKey = QStringLiteral("DEMO-K-RCL0194-ECT-WIRING");
    const MultilingualText fr = reader.text(knowledgeKey, QStringLiteral("display"), QStringLiteral("fr"));
    const MultilingualText en = reader.text(knowledgeKey, QStringLiteral("display"), QStringLiteral("en"));
    const MultilingualText de = reader.text(knowledgeKey, QStringLiteral("display"), QStringLiteral("de"));
    const MultilingualText ja = reader.text(knowledgeKey, QStringLiteral("display"), QStringLiteral("ja"));
    const MultilingualText zh = reader.text(knowledgeKey, QStringLiteral("display"), QStringLiteral("zh-CN"));
    if (!fr.isValid() || fr.resolvedLocale != QStringLiteral("fr") || fr.text.isEmpty())
        return fail(QStringLiteral("direct FR text resolution"));
    if (!en.isValid() || en.resolvedLocale != QStringLiteral("en") || en.text.isEmpty())
        return fail(QStringLiteral("direct EN text resolution"));
    if (!de.isValid() || de.resolvedLocale != QStringLiteral("fr") || !de.usedFallback())
        return fail(QStringLiteral("DE -> FR fallback"));
    if (!ja.isValid() || ja.resolvedLocale != QStringLiteral("en") || !ja.usedFallback())
        return fail(QStringLiteral("JA -> EN fallback"));
    if (!zh.isValid() || zh.resolvedLocale != QStringLiteral("en") || !zh.usedFallback())
        return fail(QStringLiteral("ZH-CN -> EN fallback"));
    pass(QStringLiteral("LOCALE RESOLUTION FR/EN + DE->FR + JA/ZH-CN->EN"));

    const MultilingualVisual visual = reader.visual(QStringLiteral("DEMO-V-RCL0194ENG-20.4"), QStringLiteral("fr"));
    if (!visual.found
        || visual.relativePath != QStringLiteral("images/rave/RCL0194ENG_20_4.png")
        || visual.sha256 != QStringLiteral("7a04d84ee0296ab41333650f369aa3624f3ac7988a528584237a7b8fb021bea0")
        || visual.sourceSize != QSize(1696, 1190)
        || !visual.originalIntact
        || visual.regions.size() != 1)
        return fail(QStringLiteral("constructor visual identity"));

    const MultilingualVisualRegion regionFr = visual.regions.first();
    if (!regionFr.hasRect || regionFr.anchorRef != QStringLiteral("COOLANT TEMPERATURE SENSOR (094)")
        || !regionFr.label.isValid() || regionFr.label.resolvedLocale != QStringLiteral("fr"))
        return fail(QStringLiteral("FR translated visual region"));

    const MultilingualVisual visualEn = reader.visual(QStringLiteral("DEMO-V-RCL0194ENG-20.4"), QStringLiteral("en"));
    if (!visualEn.found || visualEn.regions.size() != 1
        || visualEn.regions.first().label.text != QStringLiteral("COOLANT TEMPERATURE SENSOR (094)")
        || visualEn.regions.first().label.translationStatus != QStringLiteral("source"))
        return fail(QStringLiteral("EN source visual label"));
    pass(QStringLiteral("ONE ORIGINAL VISUAL + TRANSLATABLE REGION LABEL"));

    const QList<MultilingualProcedureStep> stepsFr =
        reader.procedureSteps(QStringLiteral("DEMO-P-ECT-REMOVE"), QStringLiteral("fr"));
    if (stepsFr.size() != 3)
        return fail(QStringLiteral("ECT FR procedure step count"));
    const MultilingualProcedureStep &step3Fr = stepsFr.at(2);
    if (!step3Fr.sourceExact.isValid()
        || !step3Fr.sourceExact.text.contains(QStringLiteral("SYSTEME DE GESTION MOTEUR - MEMS"))
        || !step3Fr.display.isValid()
        || step3Fr.display.text.contains(QStringLiteral("SYSTEME DE GESTION MOTEUR - MEMS")))
        return fail(QStringLiteral("source_exact/display separation"));

    const QList<MultilingualProcedureStep> stepsEn =
        reader.procedureSteps(QStringLiteral("DEMO-P-ECT-REMOVE"), QStringLiteral("en"));
    if (stepsEn.size() != 3 || !stepsEn.at(2).display.isValid()
        || stepsEn.at(2).display.resolvedLocale != QStringLiteral("en"))
        return fail(QStringLiteral("ECT EN procedure retrieval"));
    pass(QStringLiteral("PROCEDURE SOURCE EXACT + CLEAN LOCALIZED DISPLAY"));

    const MultilingualValue opening =
        reader.value(QStringLiteral("DEMO-N-THERMOSTAT-OPEN"), QStringLiteral("en"));
    const MultilingualValue torque =
        reader.value(QStringLiteral("DEMO-N-THERMOSTAT-HOUSING-TORQUE"), QStringLiteral("fr"));
    if (!opening.found || !opening.numeric.isValid() || opening.numeric.toDouble() != 88.0
        || opening.unitCode != QString::fromUtf8("\xC2\xB0" "C")
        || !opening.label.isValid())
        return fail(QStringLiteral("thermostat opening structured value"));
    if (!torque.found || !torque.numeric.isValid() || torque.numeric.toDouble() != 11.0
        || torque.unitCode != QStringLiteral("Nm")
        || !torque.sourceExact.text.contains(QStringLiteral("11 N.m.")))
        return fail(QStringLiteral("thermostat torque structured value"));
    pass(QStringLiteral("LANGUAGE-INDEPENDENT STRUCTURED VALUES 88C + 11Nm"));

    const MultilingualTable table =
        reader.table(QStringLiteral("DEMO-T-THERMOSTAT"), QStringLiteral("en"));
    if (!table.found || table.rowCount != 3 || table.columnCount != 2
        || table.headerRows != 1 || table.headerColumns != 1 || table.cells.size() != 6)
        return fail(QStringLiteral("structured thermostat table shape"));
    const MultilingualTableCell *openingCell = cellAt(table, 1, 1);
    const MultilingualTableCell *torqueCell = cellAt(table, 2, 1);
    const MultilingualTableCell *openingLabel = cellAt(table, 1, 0);
    if (!openingCell || openingCell->numeric.toDouble() != 88.0
        || openingCell->unitCode != QString::fromUtf8("\xC2\xB0" "C")
        || !torqueCell || torqueCell->numeric.toDouble() != 11.0
        || torqueCell->unitCode != QStringLiteral("Nm")
        || !openingLabel || !openingLabel->display.isValid()
        || openingLabel->display.resolvedLocale != QStringLiteral("en"))
        return fail(QStringLiteral("structured thermostat table content"));
    pass(QStringLiteral("LOCALIZED TABLE LABELS + LANGUAGE-INDEPENDENT CELLS"));

    reader.close();
    const QByteArray afterHash = fileSha256(databasePath);
    if (afterHash != beforeHash)
        return fail(QStringLiteral("read-only runtime changed SQLite bytes"));
    pass(QStringLiteral("SQLITE BYTES UNCHANGED BY RUNTIME READER"));
    pass(QStringLiteral("MULTILINGUAL KNOWLEDGE RUNTIME SELFTEST"));
    return 0;
}
