#!/usr/bin/env python3
from pathlib import Path

header = Path('iamemstab.h')
h = header.read_text(encoding='utf-8')
if 'class QShowEvent;' not in h:
    h = h.replace('class QTextBrowser;\n', 'class QTextBrowser;\nclass QShowEvent;\n', 1)
if 'void showEvent(QShowEvent *event) override;' not in h:
    anchor = 'public:\n    explicit IaMemsTab(MainWindow *mainWindow, QWidget *parent = nullptr);\n    ~IaMemsTab() override;\n\n'
    repl = anchor + 'protected:\n    void showEvent(QShowEvent *event) override;\n\n'
    if anchor not in h:
        raise SystemExit('iamemstab.h public anchor not found')
    h = h.replace(anchor, repl, 1)
header.write_text(h, encoding='utf-8')

cpp = Path('iamemstab.cpp')
c = cpp.read_text(encoding='utf-8')
if '#include <QShowEvent>' not in c:
    c = c.replace('#include <QSet>\n', '#include <QSet>\n#include <QShowEvent>\n', 1)

old_timers = '''    updateStatus();\n    QTimer::singleShot(250, this, &IaMemsTab::startKnowledgeLoad);\n    QTimer::singleShot(400, m_localAi, &LocalAiClient::initialize);\n}\n\nIaMemsTab::~IaMemsTab()\n'''
new_timers = '''    updateStatus();\n}\n\nvoid IaMemsTab::showEvent(QShowEvent *event)\n{\n    QWidget::showEvent(event);\n\n    // Keep the main ECU program light: the knowledge cache and the local\n    // language model are started only when IA MEMS is actually opened.\n    QTimer::singleShot(0, this, &IaMemsTab::startKnowledgeLoad);\n    if (m_localAi)\n        QTimer::singleShot(0, m_localAi, &LocalAiClient::initialize);\n}\n\nIaMemsTab::~IaMemsTab()\n'''
if 'void IaMemsTab::showEvent(QShowEvent *event)' not in c:
    if old_timers not in c:
        raise SystemExit('iamemstab.cpp startup timer anchor not found')
    c = c.replace(old_timers, new_timers, 1)
else:
    c = c.replace('    QTimer::singleShot(250, this, &IaMemsTab::startKnowledgeLoad);\n', '')
    c = c.replace('    QTimer::singleShot(400, m_localAi, &LocalAiClient::initialize);\n', '')

cpp.write_text(c, encoding='utf-8')
print('IA MEMS local model and knowledge cache now start lazily on first tab display')
