from pathlib import Path

catalog = Path('expert/IaMemsDiagramCatalog.cpp')
text = catalog.read_text(encoding='utf-8')
old = '''IaMemsDiagramSuggestion runtimeSuggestion(const QString &question,\n                                           const QString &generation,\n                                           const QStringList &terms,\n                                           const QString &root)\n{\n    IaMemsDiagramSuggestion best;\n    if (terms.isEmpty() || publicationOnlyTerms(terms))\n        return best;\n'''
new = '''IaMemsDiagramSuggestion runtimeSuggestion(const QString &question,\n                                           const QString &generation,\n                                           const QStringList &terms,\n                                           const QString &root)\n{\n    IaMemsDiagramSuggestion best;\n    QStringList runtimeTerms = terms;\n    if (!generation.isEmpty())\n        runtimeTerms.removeAll(generation);\n    if (runtimeTerms.isEmpty() || publicationOnlyTerms(runtimeTerms))\n        return best;\n'''
if text.count(old) != 1:
    raise SystemExit('catalog precondition failed')
text = text.replace(old, new, 1)
old2 = 'const int score = runtimeCandidateScore(question, generation, terms, entry);'
new2 = 'const int score = runtimeCandidateScore(question, generation, runtimeTerms, entry);'
if text.count(old2) != 1:
    raise SystemExit('catalog score precondition failed')
text = text.replace(old2, new2, 1)
catalog.write_text(text, encoding='utf-8')

selftest = Path('expert/IaMemsDiagramSelfTest.cpp')
text = selftest.read_text(encoding='utf-8')
old = '''    ok = requireNoSuggestion(\n             temporary.path(),\n             QStringLiteral("Voir le schéma old purge legacy uniqueold"),\n             QStringLiteral("legacy visual explicitly replaced by RAVEMEMS")) && ok;'''
new = '''    ok = requireSuggestion(\n             temporary.path(),\n             QStringLiteral("Voir le schéma old purge legacy uniqueold"),\n             QStringLiteral("RCL0193ENG p.99"),\n             QStringLiteral("ravemems/purge.png")) && ok;'''
if text.count(old) != 1:
    raise SystemExit('self-test precondition failed')
text = text.replace(old, new, 1)
selftest.write_text(text, encoding='utf-8')

workflow = Path('.github/workflows/tmp-ravemems-ia-visual-validation.yml')
text = workflow.read_text(encoding='utf-8')
old = "    paths: ['.github/workflows/tmp-ravemems-ia-visual-validation.yml']"
new = "    paths: ['.github/workflows/tmp-ravemems-ia-visual-validation.yml', 'expert/IaMemsDiagramCatalog.cpp', 'expert/IaMemsDiagramSelfTest.cpp']"
if text.count(old) != 1:
    raise SystemExit('workflow precondition failed')
workflow.write_text(text.replace(old, new, 1), encoding='utf-8')
