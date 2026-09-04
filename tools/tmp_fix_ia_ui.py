from pathlib import Path

def replace_once(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{label}: expected exactly one source match, found {count}")
    return text.replace(old, new, 1)

# --- Whole-window IA scroll ---
cpp_path = Path("iamemstab.cpp")
cpp = cpp_path.read_text(encoding="utf-8")

cpp = replace_once(
    cpp,
    '#include <QPushButton>\n#include <QScrollBar>\n',
    '#include <QPushButton>\n#include <QScrollArea>\n#include <QScrollBar>\n#include <QAbstractTextDocumentLayout>\n#include <QTextDocument>\n',
    "IA scroll includes",
)

cpp = replace_once(
    cpp,
    '''    QVBoxLayout *root = new QVBoxLayout(this);
    root->setContentsMargins(18, 14, 18, 14);
    root->setSpacing(10);
''',
    '''    QVBoxLayout *shell = new QVBoxLayout(this);
    shell->setContentsMargins(0, 0, 0, 0);
    shell->setSpacing(0);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setObjectName(QStringLiteral("iaMemsWindowScroll"));
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    QWidget *scrollContent = new QWidget(m_scrollArea);
    scrollContent->setObjectName(QStringLiteral("iaMemsScrollContent"));
    scrollContent->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    QVBoxLayout *root = new QVBoxLayout(scrollContent);
    root->setContentsMargins(18, 14, 18, 14);
    root->setSpacing(10);
''',
    "whole-window scroll shell",
)

cpp = replace_once(
    cpp,
    '''    QWidget *transcriptPane = new QWidget(this);
    QHBoxLayout *transcriptLayout = new QHBoxLayout(transcriptPane);
    transcriptLayout->setContentsMargins(0, 0, 0, 0);
    transcriptLayout->setSpacing(6);

    m_transcript = new QTextBrowser(transcriptPane);
    m_transcript->setObjectName(QStringLiteral("iaMemsTranscript"));
    m_transcript->setOpenExternalLinks(false);
    m_transcript->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_transcript->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_transcript->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    transcriptLayout->addWidget(m_transcript, 1);

    QScrollBar *transcriptScroll = new QScrollBar(Qt::Vertical, transcriptPane);
    transcriptScroll->setObjectName(QStringLiteral("iaMemsTranscriptScroll"));
    transcriptScroll->setMinimumWidth(18);
    transcriptScroll->setMaximumWidth(18);
    transcriptLayout->addWidget(transcriptScroll);

    QScrollBar *internalScroll = m_transcript->verticalScrollBar();
    QObject::connect(internalScroll, &QScrollBar::rangeChanged,
                     transcriptPane,
                     [internalScroll, transcriptScroll](int minimum, int maximum) {
                         transcriptScroll->setRange(minimum, maximum);
                         transcriptScroll->setPageStep(internalScroll->pageStep());
                         transcriptScroll->setSingleStep(internalScroll->singleStep());
                     });
    QObject::connect(internalScroll, &QScrollBar::valueChanged,
                     transcriptScroll, &QScrollBar::setValue);
    QObject::connect(transcriptScroll, &QScrollBar::valueChanged,
                     internalScroll, &QScrollBar::setValue);
    transcriptScroll->setRange(internalScroll->minimum(), internalScroll->maximum());
    transcriptScroll->setPageStep(internalScroll->pageStep());
    transcriptScroll->setSingleStep(internalScroll->singleStep());

    root->addWidget(transcriptPane, 1);
''',
    '''    m_transcript = new QTextBrowser(scrollContent);
    m_transcript->setObjectName(QStringLiteral("iaMemsTranscript"));
    m_transcript->setOpenExternalLinks(false);
    m_transcript->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_transcript->setMinimumHeight(160);
    m_transcript->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_transcript->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    root->addWidget(m_transcript);

    auto resizeTranscript = [this](const QSizeF &documentSize) {
        if (!m_transcript)
            return;
        const int targetHeight =
            qMax(160, static_cast<int>(documentSize.height() + 0.5) + 12);
        if (m_transcript->height() != targetHeight)
            m_transcript->setFixedHeight(targetHeight);
    };
    QObject::connect(m_transcript->document()->documentLayout(),
                     &QAbstractTextDocumentLayout::documentSizeChanged,
                     m_transcript,
                     resizeTranscript);
    resizeTranscript(m_transcript->document()->size());
''',
    "replace transcript-only scrollbar",
)

cpp = replace_once(
    cpp,
    '''    root->addLayout(input);

    connect(m_sendButton, &QPushButton::clicked,
''',
    '''    root->addLayout(input);

    m_scrollArea->setWidget(scrollContent);
    shell->addWidget(m_scrollArea);

    connect(m_sendButton, &QPushButton::clicked,
''',
    "attach whole-window scroll content",
)

cpp = replace_once(
    cpp,
    '''    if (QScrollBar *bar = m_transcript->verticalScrollBar()) {
        if (speaker == QStringLiteral("IA MEMS"))
            m_transcript->scrollToAnchor(messageAnchor);
        else
            bar->setValue(bar->maximum());
    }
''',
    '''    if (m_scrollArea) {
        QTimer::singleShot(0, this, [this]() {
            if (!m_scrollArea)
                return;
            if (QScrollBar *bar = m_scrollArea->verticalScrollBar())
                bar->setValue(bar->maximum());
        });
    }
''',
    "whole-window auto-scroll",
)

cpp_path.write_text(cpp, encoding="utf-8", newline="\n")

header_path = Path("iamemstab.h")
header = header_path.read_text(encoding="utf-8")
header = replace_once(
    header,
    "class QPushButton;\nclass QTextBrowser;\n",
    "class QPushButton;\nclass QScrollArea;\nclass QTextBrowser;\n",
    "QScrollArea forward declaration",
)
header = replace_once(
    header,
    "    QTextBrowser *m_transcript = nullptr;\n",
    "    QScrollArea *m_scrollArea = nullptr;\n    QTextBrowser *m_transcript = nullptr;\n",
    "QScrollArea member",
)
header_path.write_text(header, encoding="utf-8", newline="\n")

# --- Permanent implicit visual suggestion for the button ---
catalog_path = Path("expert/IaMemsDiagramCatalog.cpp")
catalog = catalog_path.read_text(encoding="utf-8")

catalog = replace_once(
    catalog,
    '''        QStringLiteral("un"), QStringLiteral("une"), QStringLiteral("des"),
        QStringLiteral("est")
    };
''',
    '''        QStringLiteral("un"), QStringLiteral("une"), QStringLiteral("des"),
        QStringLiteral("est"), QStringLiteral("mon"), QStringLiteral("ma"),
        QStringLiteral("mes"), QStringLiteral("ton"), QStringLiteral("ta"),
        QStringLiteral("tes"), QStringLiteral("son"), QStringLiteral("sa"),
        QStringLiteral("ses"), QStringLiteral("ce"), QStringLiteral("cet"),
        QStringLiteral("cette"), QStringLiteral("ces"), QStringLiteral("il"),
        QStringLiteral("elle"), QStringLiteral("ils"), QStringLiteral("elles"),
        QStringLiteral("je"), QStringLiteral("tu"), QStringLiteral("nous"),
        QStringLiteral("vous"), QStringLiteral("on"), QStringLiteral("au"),
        QStringLiteral("aux"), QStringLiteral("en"), QStringLiteral("dans"),
        QStringLiteral("sur"), QStringLiteral("sous"), QStringLiteral("pour"),
        QStringLiteral("par"), QStringLiteral("avec"), QStringLiteral("sans"),
        QStringLiteral("trop"), QStringLiteral("plus"), QStringLiteral("moins"),
        QStringLiteral("et"), QStringLiteral("ou"), QStringLiteral("que"),
        QStringLiteral("qui"), QStringLiteral("quoi"), QStringLiteral("comment"),
        QStringLiteral("pourquoi")
    };
''',
    "ignore conversational stopwords",
)

catalog = replace_once(
    catalog,
    '''        if (!result.contains(word))
            result.append(word);
    }
    return result;
}''',
    '''        if (!result.contains(word))
            result.append(word);
    }

    auto appendAlias = [&result](const QString &term) {
        if (!result.contains(term))
            result.append(term);
    };
    if (text.contains(QStringLiteral("batterie")))
        appendAlias(QStringLiteral("battery"));
    if (text.contains(QStringLiteral("restauration")))
        appendAlias(QStringLiteral("restoration"));
    if (text.contains(QStringLiteral("pignon")) || text.contains(QStringLiteral("engrenage")))
        appendAlias(QStringLiteral("gear"));
    if (text.contains(QStringLiteral("primaire")))
        appendAlias(QStringLiteral("primary"));
    if (text.contains(QStringLiteral("jeu axial"))) {
        appendAlias(QStringLiteral("end"));
        appendAlias(QStringLiteral("float"));
    }
    if (text.contains(QStringLiteral("vilebrequin")))
        appendAlias(QStringLiteral("crankshaft"));
    return result;
}''',
    "visual bilingual aliases",
)

catalog = replace_once(
    catalog,
    '''int runtimeCandidateScore(const QString &question,
                          const QString &generation,
                          const QStringList &terms,
                          const QJsonObject &entry)''',
    '''int runtimeCandidateScore(const QString &question,
                          const QString &generation,
                          const QStringList &terms,
                          const QJsonObject &entry,
                          bool strongMatch)''',
    "runtime candidate signature",
)

catalog = replace_once(
    catalog,
    '''    if (sourceType == QStringLiteral("legacy")) {
        const QString treatment = normalize(entry.value(QStringLiteral("treatment")).toString());
        if (!treatment.isEmpty() && treatment != QStringLiteral("conserver migrer legacy"))
            return -1;
    }

    const QString searchable = normalize(
''',
    '''    if (sourceType == QStringLiteral("legacy")) {
        const QString treatment = normalize(entry.value(QStringLiteral("treatment")).toString());
        if (!treatment.isEmpty() && treatment != QStringLiteral("conserver migrer legacy"))
            return -1;
    }
    if (strongMatch && sourceType != QStringLiteral("ravemems"))
        return -1;

    const QString searchable = normalize(
''',
    "implicit suggestions only from RAVEMEMS",
)

catalog = replace_once(
    catalog,
    '''    if (matched == 0)
        return -1;

    const QString publication = normalize(entry.value(QStringLiteral("publication_code")).toString());
''',
    '''    if (matched == 0 || (strongMatch && matched < 3))
        return -1;

    if (strongMatch) {
        const QString rawContext = entry.value(QStringLiteral("context_text")).toString();
        const QString normalizedContext = normalize(rawContext);
        if (rawContext.count(QStringLiteral(". .")) > 20)
            score -= 12;
        if (containsAny(normalizedContext, {
                QStringLiteral("measure"), QStringLiteral("check"),
                QStringLiteral("procedure"), QStringLiteral("adjust"),
                QStringLiteral("controle"), QStringLiteral("reglage")
            }))
            score += 4;
    }

    const QString publication = normalize(entry.value(QStringLiteral("publication_code")).toString());
''',
    "strong implicit visual threshold",
)

catalog = replace_once(
    catalog,
    '''IaMemsDiagramSuggestion runtimeSuggestion(const QString &question,
                                           const QString &generation,
                                           const QStringList &terms,
                                           const QString &root)''',
    '''IaMemsDiagramSuggestion runtimeSuggestion(const QString &question,
                                           const QString &generation,
                                           const QStringList &terms,
                                           const QString &root,
                                           bool strongMatch)''',
    "runtime suggestion signature",
)

catalog = replace_once(
    catalog,
    "runtimeCandidateScore(question, generation, runtimeTerms, entry);",
    "runtimeCandidateScore(question, generation, runtimeTerms, entry, strongMatch);",
    "runtime candidate call",
)

catalog = replace_once(
    catalog,
    '''    if (!diagramIntent)
        return IaMemsDiagramSuggestion();

    const bool asksRosco''',
    '''    // Explicit diagram requests retain the historical behavior. Ordinary
    // technical questions may expose "Voir le schéma" only through a strong
    // multi-term match in the validated RAVEMEMS runtime catalog.

    const bool asksRosco''',
    "allow strong implicit runtime visual",
)

catalog = replace_once(
    catalog,
    '''    const IaMemsDiagramSuggestion runtime = runtimeSuggestion(text, generation, terms, root);
    if (runtime.isValid())
        return runtime;

    QFile manifest''',
    '''    const IaMemsDiagramSuggestion runtime =
        runtimeSuggestion(text, generation, terms, root, !diagramIntent);
    if (runtime.isValid())
        return runtime;

    // Static legacy manifest diagrams remain opt-in: no implicit fallback.
    if (!diagramIntent)
        return IaMemsDiagramSuggestion();

    QFile manifest''',
    "implicit RAVEMEMS explicit legacy split",
)

catalog_path.write_text(catalog, encoding="utf-8", newline="\n")

# The old temporary package-time patch must no longer mutate the permanent source.
temporary_patch = Path("tools/tmp_patch_ia_implicit_visual.py")
old_temp = temporary_patch.read_text(encoding="utf-8")
if "strongMatch" not in old_temp or "matched < 2" not in old_temp:
    raise SystemExit("temporary implicit visual patch is not the expected pre-fix version")
temporary_patch.write_text(
    '''from pathlib import Path

text = Path("expert/IaMemsDiagramCatalog.cpp").read_text(encoding="utf-8")
required = [
    "bool strongMatch",
    "(strongMatch && matched < 3)",
    'strongMatch && sourceType != QStringLiteral("ravemems")',
    'appendAlias(QStringLiteral("battery"))',
    'appendAlias(QStringLiteral("primary"))',
]
missing = [marker for marker in required if marker not in text]
if missing:
    raise SystemExit("Permanent IA visual routing is incomplete: " + ", ".join(missing))
print("PERMANENT_IA_IMPLICIT_VISUAL_ROUTING_PASS")
''',
    encoding="utf-8",
    newline="\n",
)

# Keep the final package guard aligned with the new single whole-window scrollbar.
package_workflow = Path(".github/workflows/tmp-ai-memslibrary-visual-package.yml")
workflow = package_workflow.read_text(encoding="utf-8")
workflow = replace_once(
    workflow,
    '''          if ($tab -notmatch 'iaMemsTranscriptScroll') { throw 'Validated IA scroll is absent from current source' }
''',
    '''          if ($tab -notmatch 'iaMemsWindowScroll') { throw 'Validated IA whole-window scroll is absent from current source' }
''',
    "visual package scroll guard",
)
package_workflow.write_text(workflow, encoding="utf-8", newline="\n")

print("IA_UI_FIX_PATCH_PASS")
