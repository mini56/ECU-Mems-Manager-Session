from pathlib import Path

path = Path('iamemstab.cpp')
text = path.read_text(encoding='utf-8')
original = text


def replace_once(old: str, new: str, label: str):
    global text
    if old not in text:
        raise SystemExit(f'missing patch anchor: {label}')
    text = text.replace(old, new, 1)


replace_once(
    '#include "expert/ExpertRuntimeDatabase.h"\n',
    '#include "expert/ExpertRuntimeDatabase.h"\n#include "expert/LocalAiClient.h"\n',
    'local ai include')

replace_once(
'''    m_engine.setContext(m_context);\n    appendSystemMessage(QStringLiteral(\n        "Vous pouvez me poser des questions sur les mesures ECU, leur évolution, "\n        "les hypothèses du moteur expert et les connaissances MEMS. "\n        "Si les données disponibles ne permettent pas de conclure, je le dirai explicitement."));\n\n    updateStatus();\n    QTimer::singleShot(250, this, &IaMemsTab::startKnowledgeLoad);\n''',
'''    m_engine.setContext(m_context);\n\n    m_localAi = new LocalAiClient(this);\n    connect(m_localAi, &LocalAiClient::responseReady,\n            this, &IaMemsTab::onLocalAiResponse);\n    connect(m_localAi, &LocalAiClient::responseError,\n            this, &IaMemsTab::onLocalAiError);\n    connect(m_localAi, &LocalAiClient::stateChanged,\n            this, &IaMemsTab::onLocalAiStateChanged);\n\n    appendSystemMessage(QStringLiteral(\n        "Bonjour, je suis IA MEMS, l'assistant intégré à ECU MEMS Manager. "\n        "Posez-moi votre question : je peux vous aider à utiliser le logiciel, "\n        "expliquer les fonctions MEMS, analyser les mesures ECU et commenter les hypothèses du moteur expert. "\n        "Si les informations disponibles ne permettent pas de conclure, je vous le dirai clairement."));\n\n    updateStatus();\n    QTimer::singleShot(250, this, &IaMemsTab::startKnowledgeLoad);\n    QTimer::singleShot(400, m_localAi, &LocalAiClient::initialize);\n''',
    'constructor greeting and local ai')

replace_once(
'''    if (!m_context.firmware.isEmpty())\n        parts << m_context.firmware;\n\n    m_status->setText(parts.join(QStringLiteral("  •  ")));\n''',
'''    if (!m_context.firmware.isEmpty())\n        parts << m_context.firmware;\n    if (m_localAi)\n        parts << m_localAi->statusText();\n\n    m_status->setText(parts.join(QStringLiteral("  •  ")));\n''',
    'status local ai')

old_send = '''void IaMemsTab::sendQuestion()\n{\n    if (!m_question)\n        return;\n    const QString question = m_question->text().trimmed();\n    if (question.isEmpty())\n        return;\n\n    m_question->clear();\n    appendMessage(QStringLiteral("Vous"), question);\n    updateContextFromQuestion(question);\n    appendMessage(QStringLiteral("IA MEMS"), answerQuestion(question));\n}\n\n'''
new_send = '''void IaMemsTab::sendQuestion()\n{\n    if (!m_question)\n        return;\n    const QString question = m_question->text().trimmed();\n    if (question.isEmpty())\n        return;\n\n    m_question->clear();\n    appendMessage(QStringLiteral("Vous"), question);\n    updateContextFromQuestion(question);\n\n    // The deterministic layer supplies the technical/software grounding.\n    // The local model is responsible for natural language and conversation,\n    // never for inventing ECU facts.\n    m_pendingGrounding = answerQuestion(question);\n    if (m_localAi && m_localAi->isReady()) {\n        if (m_sendButton)\n            m_sendButton->setEnabled(false);\n        if (m_question)\n            m_question->setEnabled(false);\n        m_localAi->ask(question, m_pendingGrounding);\n        return;\n    }\n\n    appendMessage(QStringLiteral("IA MEMS"), m_pendingGrounding);\n}\n\nvoid IaMemsTab::onLocalAiResponse(const QString &text)\n{\n    appendMessage(QStringLiteral("IA MEMS"), text);\n    m_pendingGrounding.clear();\n    if (m_sendButton)\n        m_sendButton->setEnabled(true);\n    if (m_question) {\n        m_question->setEnabled(true);\n        m_question->setFocus();\n    }\n    updateStatus();\n}\n\nvoid IaMemsTab::onLocalAiError(const QString &message)\n{\n    QString fallback = m_pendingGrounding.trimmed();\n    if (fallback.isEmpty())\n        fallback = QStringLiteral("Je ne peux pas répondre avec le moteur conversationnel local pour le moment.");\n    appendMessage(QStringLiteral("IA MEMS"), fallback);\n    appendSystemMessage(QStringLiteral("Moteur conversationnel local indisponible : %1").arg(message));\n    m_pendingGrounding.clear();\n    if (m_sendButton)\n        m_sendButton->setEnabled(true);\n    if (m_question) {\n        m_question->setEnabled(true);\n        m_question->setFocus();\n    }\n    updateStatus();\n}\n\nvoid IaMemsTab::onLocalAiStateChanged()\n{\n    updateStatus();\n}\n\n'''
replace_once(old_send, new_send, 'send question local ai')

replace_once(
'''QString IaMemsTab::answerQuestion(const QString &question)\n{\n    const QString text = normalized(question);\n\n    if (containsAny(text, {QStringLiteral("aide"), QStringLiteral("que peux tu"),\n''',
'''QString IaMemsTab::answerQuestion(const QString &question)\n{\n    const QString text = normalized(question);\n\n    if (containsAny(text, {QStringLiteral("bonjour"), QStringLiteral("bonsoir"),\n                           QStringLiteral("salut"), QStringLiteral("hello")}))\n        return QStringLiteral(\n            "Bonjour. Je suis IA MEMS. Je peux répondre à vos questions sur ECU MEMS Manager, "\n            "sur les systèmes MEMS et sur les mesures de l'ECU connecté. Que souhaitez-vous savoir ?");\n\n    const QString software = softwareAnswer(question);\n    if (!software.isEmpty())\n        return software;\n\n    if (containsAny(text, {QStringLiteral("aide"), QStringLiteral("que peux tu"),\n''',
    'answer greeting and software')

marker = 'QString IaMemsTab::currentValuesAnswer() const\n'
software_function = r'''QString IaMemsTab::softwareAnswer(const QString &question) const
{
    const QString text = normalized(question);
    const bool softwareIntent = containsAny(text, {
        QStringLiteral("onglet"), QStringLiteral("mems manager"),
        QStringLiteral("logiciel"), QStringLiteral("programme"),
        QStringLiteral("a quoi sert"), QStringLiteral("comment fonctionne"),
        QStringLiteral("c'est quoi"), QStringLiteral("c est quoi")
    });

    auto wants = [&text, softwareIntent](const QStringList &terms) {
        if (!softwareIntent)
            return false;
        return containsAny(text, terms);
    };

    if (wants({QStringLiteral("analyse")}))
        return QStringLiteral(
            "L'onglet Analyse sert à étudier des journaux de diagnostic enregistrés. "
            "Il peut lire les fichiers CSV/TXT pris en charge, vous laisser choisir les canaux de mesure "
            "et afficher ou superposer leurs courbes pour observer l'évolution des valeurs dans le temps.");

    if (wants({QStringLiteral("apercu"), QStringLiteral("vue d'ensemble")}))
        return QStringLiteral(
            "L'onglet Aperçu présente les principales mesures ECU en direct sous forme de cadrans et d'indicateurs : "
            "régime, MAP, températures, batterie, papillon, lambda, ralenti et allumage. Il sert au contrôle rapide du moteur.");

    if (wants({QStringLiteral("injection")}))
        return QStringLiteral(
            "L'onglet Injection regroupe les mesures d'injection acquises par le mode de lecture prévu pour cela, "
            "notamment le temps d'injection final, le temps de base, la correction transitoire et son état. "
            "L'IA n'active pas un autre mode de polling de son propre chef.");

    if (wants({QStringLiteral("reglage"), QStringLiteral("reglages")}))
        return QStringLiteral(
            "L'onglet Réglage contient les ajustements de service disponibles, notamment ceux liés au ralenti, "
            "au carburant ou à l'allumage selon l'ECU. Les réglages agissent sur l'ECU : ils doivent être utilisés avec prudence.");

    if (wants({QStringLiteral("erreur"), QStringLiteral("defaut")}))
        return QStringLiteral(
            "L'onglet Erreurs affiche les informations de défaut et les états associés renvoyés par l'ECU. "
            "Il faut lire et comprendre les défauts avant de les effacer.");

    if (wants({QStringLiteral("actionneur"), QStringLiteral("actionneurs")}))
        return QStringLiteral(
            "L'onglet Actionneurs permet d'exécuter les tests d'actionneurs supportés par l'ECU, par exemple certains relais, "
            "la pompe, le ventilateur ou la commande de ralenti selon le système. Ces tests peuvent réellement actionner des organes du véhicule.");

    if (wants({QStringLiteral("diagnostic automatique"), QStringLiteral("diagnostique automatique"),
               QStringLiteral("diagnostic auto"), QStringLiteral("diagnostique auto")}))
        return QStringLiteral(
            "L'onglet Diagnostic automatique contrôle les valeurs ECU courantes, signale les anomalies ou points à surveiller, "
            "peut capturer une référence et produire un rapport. IA MEMS va plus loin en croisant ces contrôles avec l'historique, "
            "la base de connaissances et le dialogue avec l'utilisateur.");

    if (wants({QStringLiteral("toutes les mesures")}))
        return QStringLiteral(
            "L'onglet Toutes les mesures rassemble les paramètres décodés dans une vue détaillée. "
            "Il permet de comparer le paramètre, son aide, la valeur reçue de l'ECU et sa valeur interprétée lorsque le décodage est connu.");

    if (wants({QStringLiteral("ecu/rosco"), QStringLiteral("ecu rosco"), QStringLiteral("rosco")}))
        return QStringLiteral(
            "L'onglet ECU/ROSCO permet d'observer et d'utiliser les commandes de session et de diagnostic ROSCO prises en charge. "
            "Les échanges TX/RX peuvent y être visualisés en hexadécimal. Les fonctions dangereuses ou non validées restent désactivées.");

    if (wants({QStringLiteral("toutes les donnees"), QStringLiteral("donnees brutes")}))
        return QStringLiteral(
            "L'onglet Toutes les données est la vue détaillée des champs MEMS et de leurs valeurs brutes ou décodées. "
            "Il est surtout utile pour comparer les octets du protocole avec les mesures interprétées.");

    if (wants({QStringLiteral("base donnees"), QStringLiteral("base de donnees"), QStringLiteral("base données")}))
        return QStringLiteral(
            "L'onglet Base de données donne accès à la base documentaire et technique intégrée à MEMS Manager. "
            "IA MEMS utilise également une partie structurée de cette connaissance avec son niveau de provenance et de confiance.");

    if (wants({QStringLiteral("capture"), QStringLiteral("captures")}))
        return QStringLiteral(
            "La fonction Capture enregistre une image de la fenêtre de MEMS Manager. Le visualiseur intégré permet ensuite "
            "de prévisualiser, ouvrir ou supprimer les captures enregistrées.");

    if (wants({QStringLiteral("option"), QStringLiteral("parametre"), QStringLiteral("parametres")}))
        return QStringLiteral(
            "Les Options regroupent les paramètres généraux du logiciel, notamment l'interface série, l'unité de température, "
            "le thème, la langue et certains réglages d'intégration au bureau.");

    if (wants({QStringLiteral("ia mems"), QStringLiteral("ia")}))
        return QStringLiteral(
            "L'onglet IA MEMS réunit le dialogue en langage naturel, les mesures ECU déjà acquises, leur historique, "
            "le moteur expert et la base de connaissances. Le modèle conversationnel formule les réponses, mais les faits techniques "
            "et les diagnostics restent ancrés dans les données et règles de MEMS Manager.");

    return QString();
}

'''
if marker not in text:
    raise SystemExit('missing patch anchor: software function insertion')
text = text.replace(marker, software_function + marker, 1)

replace_once(
'''        "• Que sait-on sur AANMP002 ?\\n"\n        "• Cette information est-elle certaine ?\\n"\n        "Je distingue toujours les mesures réelles, les hypothèses et les connaissances externes.");\n''',
'''        "• Que sait-on sur AANMP002 ?\\n"\n        "• C'est quoi l'onglet Analyse ?\\n"\n        "• À quoi sert l'onglet Injection ?\\n"\n        "• Cette information est-elle certaine ?\\n"\n        "Je peux également expliquer le fonctionnement de MEMS Manager. "\n        "Je distingue toujours les mesures réelles, les hypothèses et les connaissances externes.");\n''',
    'help software examples')

if text == original:
    raise SystemExit('no changes applied')

path.write_text(text, encoding='utf-8', newline='\n')
print('IA MEMS local conversational model wiring patched')
