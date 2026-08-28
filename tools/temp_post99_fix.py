from pathlib import Path


def replace_once(path, old, new, marker=None):
    p = Path(path)
    text = p.read_text(encoding="utf-8")
    if marker and marker in text:
        print(f"SKIP already patched: {path} :: {marker}")
        return False
    if old not in text:
        raise SystemExit(f"TARGET MISSING {path} :: {old[:120]!r}")
    text = text.replace(old, new, 1)
    p.write_text(text, encoding="utf-8")
    print(f"PATCHED {path}")
    return True


routing = "expert/IaMemsConversationRouting.h"
service = "expert/IaMemsService.cpp"
test = "expert/IaResponseLogicTest.cpp"

replace_once(
    routing,
    '''inline QString knowledgeStatementSignature(const QString &statement)\n{\n    return normalize(statement);\n}\n''',
    '''inline QString knowledgeStatementSignature(const QString &statement)\n{\n    QString signature = normalize(statement);\n    const int illustration = signature.indexOf(QStringLiteral("illustration locale:"));\n    if (illustration >= 0)\n        signature = signature.left(illustration).trimmed();\n    return signature;\n}\n\ninline bool isMiniSpiMapPinoutQuestion(const QString &question)\n{\n    const QString text = normalize(question);\n    return mentionsMini(question)\n        && explicitInduction(question) == QStringLiteral("SPi")\n        && containsWord(text, QStringLiteral("map"))\n        && (containsWord(text, QStringLiteral("broche"))\n            || text.contains(QStringLiteral("brochage"))\n            || text.contains(QStringLiteral("pinout"))\n            || text.contains(QStringLiteral("connecteur"))\n            || text.contains(QStringLiteral("cablage"))\n            || text.contains(QStringLiteral("wiring")));\n}\n\ninline QString miniSpiMapPinoutAnswer()\n{\n    return QStringLiteral(\n        "Sur les Mini SPi documentées, le capteur MAP est intégré au calculateur MEMS et relié au collecteur d'admission par une durite de dépression. "\n        "Il n'existe donc pas de broche de signal MAP externe comparable au montage MPi. Je ne vais pas inventer une broche.");\n}\n\ninline QString wireColourRoleLabel(const QString &statement)\n{\n    const QString text = normalize(statement);\n    if (containsAny(text, {QStringLiteral("screen ground"), QStringLiteral("blindage"),\n                           QStringLiteral("masse ecran"), QStringLiteral("ecran")}))\n        return QStringLiteral("Blindage / masse écran");\n    if (text.contains(QStringLiteral("relais")))\n        return QStringLiteral("Commande relais/chauffage");\n    if (containsAny(text, {QStringLiteral("+ve"), QStringLiteral("-ve"),\n                           QStringLiteral("signal sonde"), QStringLiteral("signal lambda")}))\n        return QStringLiteral("Signal sonde");\n    return QString();\n}\n''',
    marker='inline bool isMiniSpiMapPinoutQuestion'
)

replace_once(
    service,
    '        if (queryKind == KnowledgeQueryKind::WireColor && !hasWireColorEvidence(directEvidence))\n            continue;\n',
    '        if (queryKind == KnowledgeQueryKind::WireColor && !hasWireColorEvidence(fact.statement))\n            continue;\n',
    marker='!hasWireColorEvidence(fact.statement)'
)

replace_once(
    service,
    '''        if (queryKind == KnowledgeQueryKind::Pinout)\n            return QStringLiteral(\n                "Je n'ai pas de brochage vérifié correspondant exactement à cette demande dans le contexte MEMS sélectionné. Précise le signal, le capteur ou le connecteur recherché ; je ne vais pas inventer une broche.");\n''',
    '''        if (queryKind == KnowledgeQueryKind::Pinout) {\n            if (IaMemsConversationRouting::isMiniSpiMapPinoutQuestion(question))\n                return IaMemsConversationRouting::miniSpiMapPinoutAnswer();\n            return QStringLiteral(\n                "Je n'ai pas de brochage vérifié correspondant exactement à cette demande dans le contexte MEMS sélectionné. Précise le signal, le capteur ou le connecteur recherché ; je ne vais pas inventer une broche.");\n        }\n''',
    marker='return IaMemsConversationRouting::miniSpiMapPinoutAnswer();'
)

replace_once(
    service,
    '''    ranked = uniqueRanked;\n\n    const int maximum = qMin(queryKind == KnowledgeQueryKind::General ? 3 : 4, ranked.size());\n    if (maximum == 1) {\n''',
    '''    ranked = uniqueRanked;\n\n    const bool lambdaWireColour = queryKind == KnowledgeQueryKind::WireColor\n        && (questionText.contains(QStringLiteral("lambda"))\n            || questionText.contains(QStringLiteral("oxygen"))\n            || questionText.contains(QStringLiteral("o2")));\n    const int maximum = qMin(queryKind == KnowledgeQueryKind::General ? 3 : 4, ranked.size());\n    if (maximum == 1 && !lambdaWireColour) {\n''',
    marker='const bool lambdaWireColour = queryKind == KnowledgeQueryKind::WireColor'
)

replace_once(
    service,
    '''    QStringList answers;\n    bool relayOnlyLambdaWiring = queryKind == KnowledgeQueryKind::WireColor\n        && (questionText.contains(QStringLiteral("lambda")) || questionText.contains(QStringLiteral("oxygen")));\n    if (relayOnlyLambdaWiring) {\n        const int checked = qMin(4, ranked.size());\n        for (int i = 0; i < checked; ++i) {\n            if (!normalized(ranked.at(i).fact.statement).contains(QStringLiteral("relais"))) {\n                relayOnlyLambdaWiring = false;\n                break;\n            }\n        }\n    }\n    if (relayOnlyLambdaWiring)\n        answers << QStringLiteral("La base ne donne pas directement les couleurs des fils de la sonde lambda pour ce contexte. Fait de câblage vérifié le plus proche :");\n    else if (queryKind == KnowledgeQueryKind::WireColor)\n        answers << QStringLiteral("Couleurs de fil vérifiées les plus pertinentes :");\n    else if (queryKind == KnowledgeQueryKind::Pinout)\n''',
    '''    QStringList answers;\n    if (lambdaWireColour) {\n        bool topFactsAreSpiJapan = true;\n        bool hasSignalColour = false;\n        bool hasRelayColour = false;\n        bool hasShieldColour = false;\n        for (int i = 0; i < maximum; ++i) {\n            const QString statement = normalized(ranked.at(i).fact.statement);\n            const bool spiJapan = containsWord(statement, QStringLiteral("spi"))\n                && (statement.contains(QStringLiteral("japan")) || statement.contains(QStringLiteral("japon")));\n            if (!spiJapan)\n                topFactsAreSpiJapan = false;\n            const QString role = IaMemsConversationRouting::wireColourRoleLabel(ranked.at(i).fact.statement);\n            if (role == QStringLiteral("Signal sonde"))\n                hasSignalColour = true;\n            else if (role == QStringLiteral("Commande relais/chauffage"))\n                hasRelayColour = true;\n            else if (role == QStringLiteral("Blindage / masse écran"))\n                hasShieldColour = true;\n        }\n        answers << QStringLiteral("Couleurs de fil vérifiées disponibles :");\n        if (requestedScope.induction.isEmpty() && requestedScope.market.isEmpty() && topFactsAreSpiJapan)\n            answers << QStringLiteral("Portée des faits trouvés : Mini SPi Japon 97MY.");\n        if (!hasSignalColour && (hasRelayColour || hasShieldColour))\n            answers << QStringLiteral("La base ne fournit pas la couleur des deux voies de signal +VE/-VE de la sonde. Les faits ci-dessous concernent uniquement les liaisons directement vérifiées ; je ne complète pas par déduction.");\n    } else if (queryKind == KnowledgeQueryKind::WireColor)\n        answers << QStringLiteral("Couleurs de fil vérifiées les plus pertinentes :");\n    else if (queryKind == KnowledgeQueryKind::Pinout)\n''',
    marker='Portée des faits trouvés : Mini SPi Japon 97MY.'
)

replace_once(
    service,
    '''        QString line = QStringLiteral("• %1").arg(fact.statement.trimmed());\n        if (!fact.verificationLevel.trimmed().isEmpty())\n''',
    '''        QString line = QStringLiteral("• %1").arg(fact.statement.trimmed());\n        if (lambdaWireColour) {\n            const QString role = IaMemsConversationRouting::wireColourRoleLabel(fact.statement);\n            if (!role.isEmpty())\n                line = QStringLiteral("• %1 — %2").arg(role, fact.statement.trimmed());\n        }\n        if (!fact.verificationLevel.trimmed().isEmpty())\n''',
    marker='line = QStringLiteral("• %1 — %2").arg(role, fact.statement.trimmed());'
)

replace_once(
    test,
    '''    ok &= require(IaMemsConversationRouting::knowledgeStatementSignature(QStringLiteral("  Même fait ECT. "))\n                      == IaMemsConversationRouting::knowledgeStatementSignature(QStringLiteral("Même fait ECT.")),\n                  "identical statements dedupe across family/source mirrors");\n''',
    '''    ok &= require(IaMemsConversationRouting::knowledgeStatementSignature(QStringLiteral("  Même fait ECT. "))\n                      == IaMemsConversationRouting::knowledgeStatementSignature(QStringLiteral("Même fait ECT.")),\n                  "identical statements dedupe across family/source mirrors");\n    ok &= require(IaMemsConversationRouting::knowledgeStatementSignature(\n                      QStringLiteral("Capteur MAP vérifié.\\nIllustration locale: RCL0194ENG:20.2"))\n                      == IaMemsConversationRouting::knowledgeStatementSignature(QStringLiteral("Capteur MAP vérifié.")),\n                  "illustration suffix does not defeat mirror deduplication");\n    ok &= require(IaMemsConversationRouting::isMiniSpiMapPinoutQuestion(\n                      QStringLiteral("Broche MAP Mini SPi Japan 1997")),\n                  "Mini SPi MAP pinout gets the architecture fallback");\n    ok &= require(!IaMemsConversationRouting::isMiniSpiMapPinoutQuestion(\n                      QStringLiteral("Broche MAP Mini MPi 1997")),\n                  "Mini MPi MAP pinout keeps the external MPi database path");\n    const QString miniSpiMapAnswer = IaMemsConversationRouting::miniSpiMapPinoutAnswer();\n    ok &= require(miniSpiMapAnswer.contains(QStringLiteral("intégré au calculateur"))\n                      && miniSpiMapAnswer.contains(QStringLiteral("durite de dépression"))\n                      && miniSpiMapAnswer.contains(QStringLiteral("pas de broche de signal MAP externe")),\n                  "Mini SPi MAP fallback explains internal sensor without inventing a pin");\n    ok &= require(IaMemsConversationRouting::wireColourRoleLabel(\n                      QStringLiteral("Commande du relais de sonde oxygène par fil BG noir/vert"))\n                      == QStringLiteral("Commande relais/chauffage"),\n                  "lambda relay colour is labelled as relay/heater wiring");\n    ok &= require(IaMemsConversationRouting::wireColourRoleLabel(\n                      QStringLiteral("SCREEN GROUND de la sonde par fil B noir"))\n                      == QStringLiteral("Blindage / masse écran"),\n                  "lambda screen colour is labelled as shielding");\n''',
    marker='illustration suffix does not defeat mirror deduplication'
)

print("POST99 PATCH DRIVER COMPLETE")
