#!/usr/bin/env python3
from pathlib import Path

def replace_once(path: str, old: str, new: str, label: str) -> None:
    p = Path(path)
    text = p.read_text(encoding="utf-8")
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{label}: expected exactly one block, found {count}")
    p.write_text(text.replace(old, new), encoding="utf-8", newline="\n")

routing = Path("expert/IaMemsConversationRouting.h")
text = routing.read_text(encoding="utf-8")
anchor = '''inline QString enrichWithKnownGeneration(const QString &question, const QString &knownGeneration)
{'''
insert = '''enum class ClarificationKind {
    None,
    Induction,
    Generation
};

inline ClarificationKind clarificationKind(const QString &question,
                                           const QString &knownGeneration,
                                           bool hasResolvedLocalDiagram)
{
    // Variant safety always wins. A visually plausible result must never mix
    // Mini SPi and MPi when the user has not identified the induction type.
    if (needsInductionClarification(question))
        return ClarificationKind::Induction;

    // Any current or future local visual declared in manifest.diagrams can
    // bypass only the otherwise unnecessary MEMS-generation clarification
    // once the deterministic catalog has resolved it with positive evidence.
    if (hasResolvedLocalDiagram)
        return ClarificationKind::None;

    if (needsGenerationClarification(question, knownGeneration))
        return ClarificationKind::Generation;
    return ClarificationKind::None;
}

inline QString enrichWithKnownGeneration(const QString &question, const QString &knownGeneration)
{'''
if "enum class ClarificationKind" not in text:
    if text.count(anchor) != 1:
        raise SystemExit("routing insertion anchor mismatch")
    routing.write_text(text.replace(anchor, insert), encoding="utf-8", newline="\n")

old_prompt = '''QString IaMemsTab::clarificationPrompt(const QString &question) const
{
    if (IaMemsConversationRouting::needsInductionClarification(question))
        return QStringLiteral("Pour éviter de mélanger les brochages Mini, est-ce une SPi ou une MPi ? Si tu ne sais pas, réponds « cherche » : j'utiliserai l'ECU connecté et la base avant de te redemander une information.");
    if (IaMemsConversationRouting::needsGenerationClarification(question, m_detectedFamily))
        return QStringLiteral("Quelle génération MEMS faut-il utiliser : 1.2, 1.3, 1.6 ou 1.9 ? Si tu ne sais pas, réponds « cherche » et j'utiliserai d'abord le contexte ECU disponible.");
    return QString();
}
'''
new_prompt = '''QString IaMemsTab::clarificationPrompt(const QString &question) const
{
    const IaMemsDiagramSuggestion diagram =
        IaMemsDiagramCatalog::suggestionForQuestion(question);
    const IaMemsConversationRouting::ClarificationKind kind =
        IaMemsConversationRouting::clarificationKind(
            question, m_detectedFamily, diagram.isValid());

    if (kind == IaMemsConversationRouting::ClarificationKind::Induction)
        return QStringLiteral("Pour éviter de mélanger les brochages Mini, est-ce une SPi ou une MPi ? Si tu ne sais pas, réponds « cherche » : j'utiliserai l'ECU connecté et la base avant de te redemander une information.");
    if (kind == IaMemsConversationRouting::ClarificationKind::Generation)
        return QStringLiteral("Quelle génération MEMS faut-il utiliser : 1.2, 1.3, 1.6 ou 1.9 ? Si tu ne sais pas, réponds « cherche » et j'utiliserai d'abord le contexte ECU disponible.");
    return QString();
}
'''
if "clarificationKind(" not in Path("iamemstab.cpp").read_text(encoding="utf-8"):
    replace_once("iamemstab.cpp", old_prompt, new_prompt, "clarificationPrompt")

catalog = Path("expert/IaMemsDiagramCatalog.cpp")
text = catalog.read_text(encoding="utf-8")
old_ignored = '''        QStringLiteral("schema"), QStringLiteral("diagramme"), QStringLiteral("diagram"),
        QStringLiteral("montre"), QStringLiteral("ouvrir"), QStringLiteral("ouvre"),'''
new_ignored = '''        QStringLiteral("schema"), QStringLiteral("diagramme"), QStringLiteral("diagram"),
        QStringLiteral("image"), QStringLiteral("images"),
        QStringLiteral("illustration"), QStringLiteral("illustrations"),
        QStringLiteral("vue"), QStringLiteral("vues"),
        QStringLiteral("dessin"), QStringLiteral("dessins"),
        QStringLiteral("figure"), QStringLiteral("figures"),
        QStringLiteral("montre"), QStringLiteral("ouvrir"), QStringLiteral("ouvre"),'''
if 'QStringLiteral("illustration"), QStringLiteral("illustrations")' not in text:
    if text.count(old_ignored) != 1:
        raise SystemExit("catalog ignored-term block mismatch")
    text = text.replace(old_ignored, new_ignored)

old_intent = '''        QStringLiteral("schema"), QStringLiteral("broch"), QStringLiteral("pinout"),
        QStringLiteral("connecteur"), QStringLiteral("connector"), QStringLiteral("prise"),
        QStringLiteral("cablage"), QStringLiteral("wiring"), QStringLiteral("socket"),
        QStringLiteral("pin "), QStringLiteral(" pins"), QStringLiteral("broche")
    });'''
new_intent = '''        QStringLiteral("schema"), QStringLiteral("broch"), QStringLiteral("pinout"),
        QStringLiteral("connecteur"), QStringLiteral("connector"), QStringLiteral("prise"),
        QStringLiteral("cablage"), QStringLiteral("wiring"), QStringLiteral("socket"),
        QStringLiteral("image"), QStringLiteral("illustration"), QStringLiteral("vue"),
        QStringLiteral("dessin"), QStringLiteral("figure"),
        QStringLiteral("pin "), QStringLiteral(" pins"), QStringLiteral("broche")
    });'''
if 'QStringLiteral("image"), QStringLiteral("illustration"), QStringLiteral("vue")' not in text:
    if text.count(old_intent) != 1:
        raise SystemExit("catalog visual-intent block mismatch")
    text = text.replace(old_intent, new_intent)
catalog.write_text(text, encoding="utf-8", newline="\n")

test = Path("expert/IaMemsDiagramSelfTest.cpp")
text = test.read_text(encoding="utf-8")
if '#include "IaMemsConversationRouting.h"' not in text:
    text = text.replace(
        '#include "IaMemsDiagramCatalog.h"\n',
        '#include "IaMemsDiagramCatalog.h"\n#include "IaMemsConversationRouting.h"\n'
    )
if "#include <QJsonDocument>" not in text:
    text = text.replace(
        "#include <QFileInfo>\n",
        "#include <QFileInfo>\n#include <QJsonDocument>\n#include <QJsonObject>\n"
    )

helper_anchor = "\n} // namespace\n\nint main(int argc, char **argv)\n"
helpers = r'''
bool requireClarificationKind(const QString &root,
                              const QString &question,
                              IaMemsConversationRouting::ClarificationKind expected,
                              const QString &label)
{
    const IaMemsDiagramSuggestion suggestion =
        IaMemsDiagramCatalog::suggestionForQuestion(question, root);
    const auto actual = IaMemsConversationRouting::clarificationKind(
        question, QString(), suggestion.isValid());
    if (actual != expected) {
        printLine(QStringLiteral("FAIL clarification policy: %1").arg(label));
        return false;
    }
    printLine(QStringLiteral("PASS clarification policy: %1").arg(label));
    return true;
}

bool testAllDeclaredDiagramFilesExist(const QString &root)
{
    QFile manifest(QDir(root).filePath(QStringLiteral("manifest.json")));
    if (!manifest.open(QIODevice::ReadOnly)) {
        printLine(QStringLiteral("FAIL cannot open manifest for inventory guard"));
        return false;
    }

    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(manifest.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) {
        printLine(QStringLiteral("FAIL invalid manifest for inventory guard"));
        return false;
    }

    const QJsonObject diagrams =
        document.object().value(QStringLiteral("diagrams")).toObject();
    if (diagrams.size() < 40) {
        printLine(QStringLiteral("FAIL diagram inventory unexpectedly shrank: %1")
                      .arg(diagrams.size()));
        return false;
    }

    for (auto it = diagrams.constBegin(); it != diagrams.constEnd(); ++it) {
        const QString relative = QDir::cleanPath(it.value().toString());
        if (!relative.startsWith(QStringLiteral("images/"))
            || !QFileInfo::exists(QDir(root).filePath(relative))) {
            printLine(QStringLiteral("FAIL declared diagram missing: %1 -> %2")
                          .arg(it.key(), relative));
            return false;
        }
    }

    printLine(QStringLiteral("PASS declared diagram inventory: %1 files")
                  .arg(diagrams.size()));
    return true;
}
'''
if "requireClarificationKind(" not in text:
    if text.count(helper_anchor) != 1:
        raise SystemExit("self-test helper anchor mismatch")
    text = text.replace(helper_anchor, "\n" + helpers + helper_anchor)

call_anchor = '''    ok = requireSuggestion(referenceRoot,
                           QStringLiteral("Schéma de la prise OBD 16 broches MEMS 1.9 J1962"),
                           QStringLiteral("MEMS 1.9 OBD 16 broches"),
                           QStringLiteral("images/mems_1_9_obd_16pin.svg")) && ok;
'''
extra_calls = r'''    ok = requireSuggestion(referenceRoot,
                           QStringLiteral("cablage sonde lambda Mini SPi Japon 1997"),
                           QStringLiteral("RAVE RCL0194ENG 20.3 Mini SPi Japon 97MY MEMS lambda relais vilebrequin"),
                           QStringLiteral("images/rave/RCL0194ENG_20_3.png")) && ok;
    ok = requireSuggestion(referenceRoot,
                           QStringLiteral("Montre la vue du ventilateur Mini SPi Japon 1997"),
                           QStringLiteral("RAVE RCL0194ENG 39.3 Mini SPi Japon 97MY ventilateur refroidissement"),
                           QStringLiteral("images/rave/RCL0194ENG_39_3.png")) && ok;
    ok = requireSuggestion(referenceRoot,
                           QStringLiteral("illustration injecteur IAC TPS Mini SPi Japon 1997"),
                           QStringLiteral("RAVE RCL0194ENG 20.4 Mini SPi Japon 97MY MEMS injecteur IAC TPS ECT IAT"),
                           QStringLiteral("images/rave/RCL0194ENG_20_4.png")) && ok;

    ok = requireClarificationKind(
             referenceRoot,
             QStringLiteral("Montre le schéma sonde lambda Mini SPi Japon 1997"),
             IaMemsConversationRouting::ClarificationKind::None,
             QStringLiteral("resolved RAVE visual bypasses generation clarification")) && ok;
    ok = requireClarificationKind(
             referenceRoot,
             QStringLiteral("Montre le schéma sonde lambda Mini 1997"),
             IaMemsConversationRouting::ClarificationKind::Induction,
             QStringLiteral("SPi/MPi ambiguity remains higher priority than visual")) && ok;
    ok = requireClarificationKind(
             referenceRoot,
             QStringLiteral("Montre la documentation MEMS"),
             IaMemsConversationRouting::ClarificationKind::Generation,
             QStringLiteral("no exact visual still requests MEMS generation")) && ok;
    ok = testAllDeclaredDiagramFilesExist(referenceRoot) && ok;
'''
if "resolved RAVE visual bypasses generation clarification" not in text:
    if text.count(call_anchor) != 1:
        raise SystemExit("self-test call anchor mismatch")
    text = text.replace(call_anchor, call_anchor + extra_calls)

test.write_text(text, encoding="utf-8", newline="\n")

cmake = Path("CMakeLists.txt")
text = cmake.read_text(encoding="utf-8")
anchor = '''add_dependencies(${PNAME} ia_mems_diagram_selftest)
'''
replacement = '''add_dependencies(${PNAME} ia_mems_diagram_selftest)
add_custom_command(TARGET ${PNAME} POST_BUILD
    COMMAND "$<TARGET_FILE:ia_mems_diagram_selftest>")
'''
if 'COMMAND "$<TARGET_FILE:ia_mems_diagram_selftest>"' not in text:
    if text.count(anchor) != 1:
        raise SystemExit("CMake diagram self-test dependency anchor mismatch")
    cmake.write_text(text.replace(anchor, replacement), encoding="utf-8", newline="\n")
print("PASS: IA MEMS diagram/routing self-test is now an automatic post-build guard")

print("PASS: generic visual routing policy and automatic regression guards patched")
