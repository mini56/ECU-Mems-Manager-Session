#!/usr/bin/env python3
"""Fail the SESSION build if historical source patches or layered UI hooks return.

This is a read-only validation of the checked-in C++ architecture. It does
not rewrite, generate or alter any source file.
"""
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]


def text(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def verify(condition: bool, name: str) -> None:
    if not condition:
        raise AssertionError(name)


def main() -> int:
    tools = ROOT / "tools"
    leftovers = sorted(
        list(tools.glob("patch_*.py"))
        + list(tools.glob("temp_patch_*.py"))
        + list(ROOT.glob("*PATCH_STARTED*"))
    )
    verify(not leftovers, "OBSOLETE_SOURCE_PATCH_SCRIPT: " + ", ".join(map(str, leftovers)))

    for workflow in (ROOT / ".github" / "workflows").glob("*.yml"):
        value = workflow.read_text(encoding="utf-8")
        verify("python tools/patch_" not in value
               and "python tools/temp_patch_" not in value,
               "WORKFLOW_REINTRODUCES_RUNTIME_SOURCE_PATCH: " + workflow.name)

    owner = text("database/MemsDatabaseBrowser.cpp")
    explorer = text("database/MemsDatabaseExplorerV2.cpp")
    ia_view = text("iamemstab.cpp")
    legacy = text("expert/IaMemsDocumentaryPipeline.cpp")
    verify(owner.count('setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff)') == 1,
           "DATABASE_DOCUMENT_SCROLL_CONFIG_NOT_UNIQUE")
    verify('memsDatabaseDocumentScroll' in owner
           and 'QScrollBar::rangeChanged' in owner
           and 'QScrollBar::valueChanged' in owner,
           "DATABASE_DOCUMENT_SCROLL_NOT_OWNED_BY_LAYOUT")
    verify('forceDetailScroll' not in explorer
           and 'QEvent::Resize' not in explorer,
           "DATABASE_DOCUMENT_RUNTIME_SCROLL_OVERRIDE")
    verify('connect(m_sendButton,&QPushButton::clicked,this,&IaMemsTab::sendQuestion)' in ia_view
           and 'connectIaMemsDocumentaryPipeline(this,m_question,m_sendButton,m_transcript)' in ia_view,
           "IA_SEND_ACTION_NOT_OWNED_BY_VIEW")
    verify('MemsDocumentaryRepository::isPackInstalled()' in ia_view
           and 'MemsDocumentaryRepository::isPackInstalled()' not in legacy,
           "IA_LEGACY_INTERCEPTOR_STILL_PATCHES_ROVER")
    print("PASS SESSION ZERO PATCH SCRIPTS; SINGLE DATABASE SCROLL OWNER; IA SOURCE ROUTING")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except AssertionError as error:
        print(f"FAIL SESSION ZERO PATCH: {error}", file=sys.stderr)
        sys.exit(1)
